#include <algorithm>
#include <cmath>

#include "PluginProcessor.h"
#include "PluginEditor.h"

const std::array<std::pair<int, int>, 12> SpeakerVisualizerComponent::roomEdges = { {
    { 0, 1 }, { 1, 3 }, { 3, 2 }, { 2, 0 },
    { 4, 5 }, { 5, 7 }, { 7, 6 }, { 6, 4 },
    { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }
} };

namespace
{
    using CameraPreset = SpeakerVisualizerComponent::CameraPreset;
    using CameraParameters = SpeakerVisualizerComponent::CameraParameters;

    struct PresetDefinition
    {
        CameraPreset preset;
        CameraParameters params;
    };

    constexpr float outsideHomeBaseDistance  = 40.0f;
    constexpr float outsideOrbitBaseDistance = 25.0f;
    constexpr float outsideTopBaseDistance   = 32.0f;
    constexpr float insideHomeBaseDistance   = 7.0f;
    constexpr float insideOrbitBaseDistance  = 6.0f;
    constexpr float insideTopBaseDistance    = 8.0f;

    constexpr float insideProjectionScale  = 0.35f;
    constexpr float outsideProjectionScale = 0.45f;
    constexpr float insideNearPlane        = 0.05f;
    constexpr float insideDefaultZoom      = 0.25f;
    constexpr float insideHomeDefaultZoom  = 0.20f;
    constexpr float insideTopDefaultZoom   = 0.10f;
    constexpr float outsideDefaultZoom     = 1.0f;

    constexpr std::array<PresetDefinition, 12> presetDefinitions = { {
        { CameraPreset::OutsideHome,  { -110.0f, -18.0f, outsideHomeBaseDistance,  false } },
        { CameraPreset::OutsideFront, {  -90.0f,   0.0f, outsideOrbitBaseDistance, false } },
        { CameraPreset::OutsideBack,  {   90.0f,   0.0f, outsideOrbitBaseDistance, false } },
        { CameraPreset::OutsideLeft,  {    0.0f,   0.0f, outsideOrbitBaseDistance, false } },
        { CameraPreset::OutsideRight, {  180.0f,   0.0f, outsideOrbitBaseDistance, false } },
        { CameraPreset::OutsideTop,   {  -90.0f, -90.0f, outsideTopBaseDistance,   false } },
        { CameraPreset::InsideHome,   {  -90.0f, -32.0f, insideHomeBaseDistance,   true  } },
        { CameraPreset::InsideFront,  {  -90.0f,   0.0f, insideOrbitBaseDistance,  true  } },
        { CameraPreset::InsideBack,   {   90.0f,   0.0f, insideOrbitBaseDistance,  true  } },
        { CameraPreset::InsideLeft,   {    0.0f,   0.0f, insideOrbitBaseDistance,  true  } },
        { CameraPreset::InsideRight,  {  180.0f,   0.0f, insideOrbitBaseDistance,  true  } },
        { CameraPreset::InsideTop,    {  -90.0f,  -90.0f, insideTopBaseDistance,    true  } }
    } };
}

SpeakerVisualizerComponent::SpeakerVisualizerComponent (AtmosVizAudioProcessor& p)
    : processor (p), roomDimensions (processor.getRoomDimensions())
{
    const auto& defs = processor.getSpeakerDefinitions();
    speakers.reserve (defs.size());

    for (const auto& def : defs)
        speakers.push_back ({ def, {}, {}, {}, 0.0f });

    updateHeatmapCache();

    const auto widthHalf  = roomDimensions.width * 0.5f;
    const auto depthHalf  = roomDimensions.depth * 0.5f;
    const auto floorY     = -roomDimensions.earHeight;
    const auto ceilingY   = roomDimensions.height - roomDimensions.earHeight;

    roomVerticesModel = { {
        {  depthHalf, floorY, -widthHalf }, {  depthHalf, floorY,  widthHalf },
        {  depthHalf, ceilingY, -widthHalf }, {  depthHalf, ceilingY,  widthHalf },
        { -depthHalf, floorY, -widthHalf }, { -depthHalf, floorY,  widthHalf },
        { -depthHalf, ceilingY, -widthHalf }, { -depthHalf, ceilingY,  widthHalf }
    } };

    if (const auto* outsideHome = findPresetDefinition (CameraPreset::OutsideHome))
    {
        outsideUserState.yaw          = juce::degreesToRadians (outsideHome->yawDegrees);
        outsideUserState.pitch        = juce::degreesToRadians (outsideHome->pitchDegrees);
        outsideUserState.baseDistance = outsideHome->baseDistance;
        outsideUserState.zoom         = outsideDefaultZoom;
    }

    if (const auto* insideHome = findPresetDefinition (CameraPreset::InsideHome))
    {
        insideUserState.yaw          = juce::degreesToRadians (insideHome->yawDegrees);
        insideUserState.pitch        = juce::degreesToRadians (insideHome->pitchDegrees);
        insideUserState.baseDistance = insideHome->baseDistance;
        insideUserState.zoom         = insideHomeDefaultZoom;
    }

    setCameraPreset (CameraPreset::OutsideHome);
    captureUserState();

    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    startTimerHz (30);
}

void SpeakerVisualizerComponent::timerCallback()
{
    refreshMetrics();
    repaint();
}

void SpeakerVisualizerComponent::refreshMetrics()
{
    AtmosVizAudioProcessor::SpeakerMetricsArray latest {};
    processor.copyLatestMetrics (latest);

    for (size_t i = 0; i < speakers.size(); ++i)
        speakers[i].metrics = latest[i];
}

void SpeakerVisualizerComponent::updateProjectionScale()
{
    const auto bounds = getLocalBounds().toFloat();
    const auto minDimension = std::max (1.0f, std::min (bounds.getWidth(), bounds.getHeight()));

    if (cameraInside)
    {
        baseProjectionScale = minDimension * insideProjectionScale;
        projectionScale = baseProjectionScale * zoomFactor;
        return;
    }

    const auto fill = outsideProjectionScale;
    const auto cosYaw   = std::cos (yaw);
    const auto sinYaw   = std::sin (yaw);
    const auto cosPitch = std::cos (pitch);
    const auto sinPitch = std::sin (pitch);

    float maxExtent = 0.0f;

    for (const auto& vertex : roomVerticesModel)
    {
        const auto x1 = vertex.x * cosYaw - vertex.z * sinYaw;
        const auto z1 = vertex.x * sinYaw + vertex.z * cosYaw;
        const auto y2 = vertex.y * cosPitch - z1 * sinPitch;

        maxExtent = std::max (maxExtent, std::abs (x1));
        maxExtent = std::max (maxExtent, std::abs (y2));
    }

    if (maxExtent < 1.0e-4f)
        baseProjectionScale = minDimension * fill;
    else
        baseProjectionScale = (minDimension * fill) / maxExtent;

    projectionScale = baseProjectionScale * zoomFactor;
}

SpeakerVisualizerComponent::ProjectedPoint SpeakerVisualizerComponent::projectPoint (const juce::Vector3D<float>& point) const
{
    const auto cosYaw   = std::cos (yaw);
    const auto sinYaw   = std::sin (yaw);
    const auto cosPitch = std::cos (pitch);
    const auto sinPitch = std::sin (pitch);

    const auto bounds = getLocalBounds().toFloat();
    const auto centre = bounds.getCentre();
    const auto scale = projectionScale;

    const auto x1 = point.x * cosYaw - point.z * sinYaw;
    const auto z1 = point.x * sinYaw + point.z * cosYaw;
    const auto y2 = point.y * cosPitch - z1 * sinPitch;
    const auto z2 = point.y * sinPitch + z1 * cosPitch;

    const auto depth  = cameraInside ? -z2 : cameraDistance - z2;

    float perspectiveFactor = 1.0f;

    if (cameraInside)
    {
        const auto referenceDistance = juce::jmax (insideNearPlane * 4.0f, cameraBaseDistance);
        const auto clampedDepth = juce::jmax (insideNearPlane, std::abs (depth));
        perspectiveFactor = referenceDistance / clampedDepth;
    }

    return {
        { centre.x + x1 * scale * perspectiveFactor,
          centre.y - y2 * scale * perspectiveFactor },
        depth
    };
}

void SpeakerVisualizerComponent::updateProjections()
{
    for (auto& speaker : speakers)
        applyOrbit (speaker);

    updateTrails();
}

void SpeakerVisualizerComponent::applyOrbit (DisplaySpeaker& speaker) const
{
    const auto projected = projectPoint (speaker.definition.position);
    speaker.projected = projected.screen;
    speaker.depth     = projected.depth;

    updateMaxReach (speaker);

    if (speaker.definition.isLfe)
    {
        speaker.orientation2D = { 0.0f, -1.0f };
        return;
    }

    const auto aim = speaker.definition.aimDirection;

    const auto cosYaw   = std::cos (yaw);
    const auto sinYaw   = std::sin (yaw);
    const auto cosPitch = std::cos (pitch);
    const auto sinPitch = std::sin (pitch);

    const auto aimX1 = aim.x * cosYaw - aim.z * sinYaw;
    const auto aimZ1 = aim.x * sinYaw + aim.z * cosYaw;
    const auto aimY2 = aim.y * cosPitch - aimZ1 * sinPitch;
    juce::ignoreUnused (aimZ1);

    auto dir2D = juce::Point<float> (aimX1, -aimY2);
    const auto len = dir2D.getDistanceFromOrigin();

    if (len > 1.0e-4f) dir2D /= len;
    else               dir2D = { 0.0f, -1.0f };

    speaker.orientation2D = dir2D;
}

void SpeakerVisualizerComponent::drawRoom (juce::Graphics& g)
{
    updateRoomProjection();

    if (cameraInside)
    {
        g.setColour (juce::Colours::whitesmoke.withAlpha (0.9f));

        const float clipDepth = -insideNearPlane;

        auto clipToPlane = [&] (ProjectedPoint point, const ProjectedPoint& other) -> ProjectedPoint
        {
            const float denom = other.depth - point.depth;
            if (std::abs (denom) < 1.0e-6f)
                return point;

            const float t = (clipDepth - point.depth) / denom;
            point.screen = point.screen + (other.screen - point.screen) * t;
            point.depth = clipDepth;
            return point;
        };

        for (const auto& edge : roomEdges)
        {
            auto start = roomVerticesProjected[(size_t) edge.first];
            auto end   = roomVerticesProjected[(size_t) edge.second];

            if (start.depth <= clipDepth && end.depth <= clipDepth)
                continue;

            if (start.depth <= clipDepth)
                start = clipToPlane (start, end);

            if (end.depth <= clipDepth)
                end = clipToPlane (end, start);

            g.drawLine (juce::Line<float> (start.screen, end.screen), 3.0f);
        }
        return;
    }

    const auto cosYaw   = std::cos (yaw);
    const auto sinYaw   = std::sin (yaw);
    const auto cosPitch = std::cos (pitch);
    const auto sinPitch = std::sin (pitch);

    static constexpr int faceIndices[6][4] =
    {
        { 0, 1, 3, 2 },
        { 4, 6, 7, 5 },
        { 0, 4, 5, 1 },
        { 2, 3, 7, 6 },
        { 1, 5, 7, 3 },
        { 0, 2, 6, 4 }
    };

    static constexpr int edgeFaceLookup[12][2] =
    {
        { 0, 2 }, { 0, 4 }, { 0, 3 }, { 0, 5 },
        { 1, 2 }, { 1, 4 }, { 1, 3 }, { 1, 5 },
        { 2, 5 }, { 2, 4 }, { 3, 5 }, { 3, 4 }
    };

    static const std::array<juce::Vector3D<float>, 6> faceNormals =
    {
        juce::Vector3D<float> {  1.0f,  0.0f,  0.0f },
        juce::Vector3D<float> { -1.0f,  0.0f,  0.0f },
        juce::Vector3D<float> {  0.0f, -1.0f,  0.0f },
        juce::Vector3D<float> {  0.0f,  1.0f,  0.0f },
        juce::Vector3D<float> {  0.0f,  0.0f,  1.0f },
        juce::Vector3D<float> {  0.0f,  0.0f, -1.0f }
    };

    std::array<bool, 6> faceVisible {};
    for (size_t face = 0; face < faceVisible.size(); ++face)
    {
        const auto& normalModel = faceNormals[face];
        const float z1 = normalModel.x * sinYaw + normalModel.z * cosYaw;
        const float z2 = normalModel.y * sinPitch + z1 * cosPitch;

        faceVisible[face] = z2 < 0.0f;
    }

    struct EdgeProjection
    {
        juce::Point<float> start;
        juce::Point<float> end;
    };

    std::vector<EdgeProjection> hiddenEdges;
    std::vector<EdgeProjection> visibleEdges;
    hiddenEdges.reserve (roomEdges.size());
    visibleEdges.reserve (roomEdges.size());

    for (size_t edgeIndex = 0; edgeIndex < roomEdges.size(); ++edgeIndex)
    {
        const auto faces = edgeFaceLookup[edgeIndex];
        const bool visible = faceVisible[(size_t) faces[0]] || faceVisible[(size_t) faces[1]];
        const auto& edge = roomEdges[edgeIndex];
        const auto& a = roomVerticesProjected[(size_t) edge.first];
        const auto& b = roomVerticesProjected[(size_t) edge.second];
        EdgeProjection projection { a.screen, b.screen };

        (visible ? visibleEdges : hiddenEdges).emplace_back (projection);
    }

    g.setColour (juce::Colours::darkslategrey.withAlpha (0.55f));
    static constexpr float dashPattern[2] = { 8.0f, 5.5f };
    for (const auto& edge : hiddenEdges)
        g.drawDashedLine (juce::Line<float> (edge.start, edge.end), dashPattern, 2, 3.0f);

    g.setColour (juce::Colours::whitesmoke.withAlpha (0.85f));
    for (const auto& edge : visibleEdges)
        g.drawLine (juce::Line<float> (edge.start, edge.end), 3.0f);
}

void SpeakerVisualizerComponent::drawGizmo (juce::Graphics& g)
{
    const auto origin = projectPoint ({ 0.0f, 0.0f, 0.0f });
    const auto xAxis  = projectPoint ({  gizmoAxisLength, 0.0f, 0.0f });
    const auto yAxis  = projectPoint ({ 0.0f,  gizmoAxisLength, 0.0f });
    const auto zAxis  = projectPoint ({ 0.0f, 0.0f,  gizmoAxisLength });

    const auto arrowWidth = cameraInside ? 3.0f : 2.2f;

    auto drawAxis = [&] (juce::Colour colour, const ProjectedPoint& end, const juce::String& label)
    {
        g.setColour (colour.withAlpha (0.9f));
        g.drawLine (juce::Line<float> (origin.screen, end.screen), arrowWidth);

        auto labelBounds = juce::Rectangle<float> (end.screen.x - 20.0f,
                                                   end.screen.y - 14.0f,
                                                   40.0f,
                                                   18.0f).toNearestInt();
        g.setColour (colour.withAlpha (0.85f));
        g.drawFittedText (label, labelBounds, juce::Justification::centred, 1);
    };

    drawAxis (juce::Colours::red,   xAxis, "X");
    drawAxis (juce::Colours::green, yAxis, "Y");
    drawAxis (juce::Colours::blue,  zAxis, "Z");
}

void SpeakerVisualizerComponent::updateRoomProjection()
{
    updateProjectionScale();

    for (size_t i = 0; i < roomVerticesModel.size(); ++i)
        roomVerticesProjected[i] = projectPoint (roomVerticesModel[i]);
}

void SpeakerVisualizerComponent::setZoomFactor (float newFactor, bool fromPreset)
{
    const auto clamped = juce::jlimit (minZoomFactor, maxZoomFactor, newFactor);
    if (std::abs (clamped - zoomFactor) < 1.0e-6f && ! fromPreset)
        return;

    zoomFactor = clamped;
    applyZoomFactorToCamera();

    if (! fromPreset)
        promoteToUserPreset();

    repaint();

    if (onZoomFactorChanged)
        onZoomFactorChanged (zoomFactor);
}

void SpeakerVisualizerComponent::setVisualizationMode (VisualizationMode mode)
{
    if (mode == visualizationMode)
        return;

    visualizationMode = mode;

    repaint();

    if (onVisualizationModeChanged)
        onVisualizationModeChanged (visualizationMode);
}


void SpeakerVisualizerComponent::setVisualizationScaleAdjustment (float value)
{
    const auto clamped = juce::jlimit (-100.0f, 100.0f, value);
    if (std::abs (clamped - visualizationScaleSliderValue) < 1.0e-4f)
        return;

    visualizationScaleSliderValue = clamped;
    visualizationScale = std::pow (2.0f, visualizationScaleSliderValue / 100.0f);

    repaint();

    if (onVisualizationScaleChanged)
        onVisualizationScaleChanged (visualizationScaleSliderValue);
}

void SpeakerVisualizerComponent::setBandColourWeights (BandColourWeights weights)
{
    weights.low = std::max (0.0f, weights.low);
    weights.mid = std::max (0.0f, weights.mid);
    weights.high = std::max (0.0f, weights.high);

    if (weights.low + weights.mid + weights.high <= 1.0e-6f)
    {
        weights.low = weights.mid = weights.high = 1.0f;
    }

    const auto changed = std::abs (weights.low - bandColourWeights.low) > 1.0e-4f
                       || std::abs (weights.mid - bandColourWeights.mid) > 1.0e-4f
                       || std::abs (weights.high - bandColourWeights.high) > 1.0e-4f;

    if (! changed)
        return;

    bandColourWeights = weights;
    repaint();
}

void SpeakerVisualizerComponent::setHeatmapDensity (int level)
{
    const auto clamped = juce::jlimit (1, 5, level);
    if (clamped == heatmapDensityLevel)
        return;

    heatmapDensityLevel = clamped;
    updateHeatmapCache();
    repaint();
}

void SpeakerVisualizerComponent::setCameraPreset (CameraPreset preset)
{
    currentPreset = preset;

    bool zoomHandled = false;

    if (isUserPreset (preset))
    {
        const bool inside = isInsidePreset (preset);
        cameraInside = inside;
        restoreUserState (inside);
    }
    else if (const auto* params = findPresetDefinition (preset))
    {
        cameraInside = params->inside;
        yaw          = juce::degreesToRadians (params->yawDegrees);
        pitch        = juce::degreesToRadians (params->pitchDegrees);
        cameraBaseDistance = params->baseDistance;

        float targetZoom = outsideDefaultZoom;
        if (cameraInside)
        {
            if (preset == CameraPreset::InsideTop)
                targetZoom = insideTopDefaultZoom;
            else if (preset == CameraPreset::InsideHome)
                targetZoom = insideHomeDefaultZoom;
            else
                targetZoom = insideDefaultZoom;
        }


        if (cameraInside)
        {
            setZoomFactor (targetZoom, true);
            zoomHandled = true;
        }
        else if (isHomePreset (preset))
        {
            setZoomFactor (targetZoom, true);
            zoomHandled = true;
        }
        else
        {
            applyZoomFactorToCamera();
        }

    }


    if (! zoomHandled)
    {
        repaint();

        if (onZoomFactorChanged)
            onZoomFactorChanged (zoomFactor);
    }

    notifyPresetChanged();
}

SpeakerVisualizerComponent::CameraPreset SpeakerVisualizerComponent::getCameraPreset() const noexcept
{
    return currentPreset;
}

void SpeakerVisualizerComponent::mouseDown (const juce::MouseEvent& e)
{
    dragAnchor  = e.position;
    yawAnchor   = yaw;
    pitchAnchor = pitch;
}

void SpeakerVisualizerComponent::mouseDrag (const juce::MouseEvent& e)
{
    const auto delta = e.position - dragAnchor;
    const auto newYaw = yawAnchor + delta.x * 0.005f;
    const auto newPitch = juce::jlimit (juce::degreesToRadians (-85.0f),
                                        juce::degreesToRadians ( 85.0f),
                                        pitchAnchor - delta.y * 0.005f);

    if (std::abs (newYaw - yaw) > 1.0e-4f || std::abs (newPitch - pitch) > 1.0e-4f)
    {
        yaw = newYaw;
        pitch = newPitch;
        promoteToUserPreset();
        repaint();
    }
}

void SpeakerVisualizerComponent::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    juce::ignoreUnused (e);

    const auto delta = zoomFactor - wheel.deltaY * 0.1f;
    setZoomFactor (delta, false);

    juce::Component::mouseWheelMove (e, wheel);
}

juce::Colour SpeakerVisualizerComponent::colourForBands (const AtmosVizAudioProcessor::FrequencyBands& bands, bool isLfe) const
{
    if (isLfe)
        return juce::Colour::fromFloatRGBA (0.95f, 0.58f, 0.18f, 1.0f);

    const auto totalEnergy = bands.low + bands.mid + bands.high;
    if (totalEnergy <= 1.0e-6f)
        return juce::Colour::fromFloatRGBA (0.22f, 0.24f, 0.28f, 1.0f);

    const auto lowShare  = bands.low  / totalEnergy;
    const auto midShare  = bands.mid  / totalEnergy;
    const auto highShare = bands.high / totalEnergy;
    const auto brightness = juce::jlimit (0.4f, 1.0f, 0.45f + juce::jlimit (0.0f, 1.0f, totalEnergy) * 0.35f);

    return colourFromShares (lowShare, midShare, highShare, brightness);
}

juce::Colour SpeakerVisualizerComponent::colourFromShares (float lowShare, float midShare, float highShare, float brightness) const
{
    const auto lowWeight  = std::max (0.0f, bandColourWeights.low);
    const auto midWeight  = std::max (0.0f, bandColourWeights.mid);
    const auto highWeight = std::max (0.0f, bandColourWeights.high);

    auto weightedLow  = lowShare  * lowWeight;
    auto weightedMid  = midShare  * midWeight;
    auto weightedHigh = highShare * highWeight;

    const auto weightSum = weightedLow + weightedMid + weightedHigh;
    if (weightSum <= 1.0e-6f)
        return juce::Colour::fromFloatRGBA (0.22f, 0.24f, 0.28f, 1.0f);

    weightedLow  /= weightSum;
    weightedMid  /= weightSum;
    weightedHigh /= weightSum;

    const juce::Vector3D<float> lowBase  { 0.16f, 0.32f, 0.92f };
    const juce::Vector3D<float> midBase  { 0.24f, 0.82f, 0.34f };
    const juce::Vector3D<float> highBase { 0.93f, 0.34f, 0.28f };

    auto mix = lowBase * weightedLow + midBase * weightedMid + highBase * weightedHigh;
    mix *= juce::jlimit (0.25f, 1.2f, brightness);
    mix += juce::Vector3D<float> { 0.035f, 0.035f, 0.035f };

    mix.x = juce::jlimit (0.0f, 1.0f, mix.x);
    mix.y = juce::jlimit (0.0f, 1.0f, mix.y);
    mix.z = juce::jlimit (0.0f, 1.0f, mix.z);

    return juce::Colour::fromFloatRGBA (mix.x, mix.y, mix.z, 1.0f);
}

juce::Colour SpeakerVisualizerComponent::colourForBandMix (float lowShare, float midShare, float highShare) const
{
    const auto sum = std::max (1.0e-6f, std::abs (lowShare) + std::abs (midShare) + std::abs (highShare));
    return colourFromShares (std::max (0.0f, lowShare) / sum,
                             std::max (0.0f, midShare) / sum,
                             std::max (0.0f, highShare) / sum,
                             0.75f);
}

juce::Colour SpeakerVisualizerComponent::colourForHeatmapRatio (float ratio) const
{
    ratio = juce::jlimit (0.0f, 1.0f, ratio);

    float lowShare = 0.0f;
    float midShare = 0.0f;
    float highShare = 0.0f;

    if (ratio < 0.5f)
    {
        const auto t = ratio / 0.5f;
        lowShare = 1.0f - t;
        midShare = t;
    }
    else
    {
        const auto t = (ratio - 0.5f) / 0.5f;
        midShare = 1.0f - t;
        highShare = t;
    }

    const auto brightness = juce::jlimit (0.35f, 1.0f, 0.45f + ratio * 0.5f);
    return colourFromShares (lowShare, midShare, highShare, brightness);
}

void SpeakerVisualizerComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);

    drawRoom (g);
    drawGizmo (g);
    updateProjections();

    DrawOrder drawOrder;
    drawOrder.reserve (speakers.size());
    for (const auto& speaker : speakers)
        drawOrder.push_back (&speaker);

    std::sort (drawOrder.begin(), drawOrder.end(),
               [] (const DisplaySpeaker* a, const DisplaySpeaker* b) { return a->depth > b->depth; });

    if (cameraInside)
    {
        const auto newEnd = std::remove_if (drawOrder.begin(), drawOrder.end(),
                                            [] (const DisplaySpeaker* speakerPtr)
                                            {
                                                return speakerPtr->depth < -insideNearPlane;
                                            });
        drawOrder.erase (newEnd, drawOrder.end());
    }

    switch (visualizationMode)
    {
        case VisualizationMode::DirectionalLobes:
            drawDirectionalLobes (g, drawOrder);
            break;
        case VisualizationMode::LayeredLobes:
            drawLayeredLobes (g, drawOrder);
            break;
        case VisualizationMode::DirectivityBalloon:
            drawDirectivityBalloons (g, drawOrder);
            break;
        case VisualizationMode::RadiationHeatmap:
            drawRadiationHeatmap (g);
            break;
        case VisualizationMode::TemporalTrail:
            drawTemporalTrails (g, drawOrder);
            break;
    }

    drawSpeakerBaseMarkers (g, drawOrder);
}



float SpeakerVisualizerComponent::distanceToRoomBoundary (const juce::Vector3D<float>& position,
                                                                  const juce::Vector3D<float>& direction) const
{
    constexpr float eps = 1.0e-5f;
    const float depthHalf  = roomDimensions.depth * 0.5f;
    const float widthHalf  = roomDimensions.width * 0.5f;
    const float floorY     = -roomDimensions.earHeight;
    const float ceilingY   = roomDimensions.height - roomDimensions.earHeight;

    float closest = std::numeric_limits<float>::infinity();

    auto update = [&] (float origin, float dir, float minBound, float maxBound)
    {
        if (std::abs (dir) < eps)
            return;

        const float bound = dir > 0.0f ? maxBound : minBound;
        const float t = (bound - origin) / dir;
        if (t > eps)
            closest = std::min (closest, t);
    };

    update (position.x, direction.x, -depthHalf, depthHalf);
    update (position.y, direction.y, floorY, ceilingY);
    update (position.z, direction.z, -widthHalf, widthHalf);

    if (! std::isfinite (closest))
        closest = std::sqrt (roomDimensions.depth * roomDimensions.depth
                              + roomDimensions.width * roomDimensions.width
                              + roomDimensions.height * roomDimensions.height);

    return juce::jmax (eps, closest);
}

void SpeakerVisualizerComponent::updateMaxReach (DisplaySpeaker& speaker) const
{
    const auto& def = speaker.definition;
    auto aim = def.aimDirection;
    const auto aimLen = aim.length();

    if (aimLen <= 1.0e-5f)
    {
        auto toCentre = juce::Vector3D<float> { 0.0f, 0.0f, 0.0f } - def.position;
        const auto centreDist = toCentre.length();
        if (centreDist > 1.0e-5f)
            toCentre /= centreDist;
        else
            toCentre = { 1.0f, 0.0f, 0.0f };

        const auto diag = std::sqrt (roomDimensions.depth * roomDimensions.depth
                                     + roomDimensions.width * roomDimensions.width
                                     + roomDimensions.height * roomDimensions.height);
        speaker.maxReachWorld = diag * 0.5f;
        const auto endpoint = def.position + toCentre * speaker.maxReachWorld;
        const auto projectedEndpoint = projectPoint (endpoint);
        speaker.maxReachScreen = juce::jmax (80.0f,
                                             projectedEndpoint.screen.getDistanceFrom (speaker.projected));
        return;
    }

    aim /= aimLen;
    const auto distance = distanceToRoomBoundary (def.position, aim);
    speaker.maxReachWorld = distance;

    const auto endpoint = def.position + aim * distance;
    const auto projectedEndpoint = projectPoint (endpoint);
    speaker.maxReachScreen = projectedEndpoint.screen.getDistanceFrom (speaker.projected);

    if (speaker.maxReachScreen < 1.0f)
        speaker.maxReachScreen = 1.0f;
}

float SpeakerVisualizerComponent::visualLevelForSpeaker (const DisplaySpeaker& speaker) const
{
    return juce::jlimit (0.0f, 1.0f, std::max (speaker.metrics.peak, speaker.metrics.rms));
}

float SpeakerVisualizerComponent::reachForLevel (const DisplaySpeaker& speaker, float level, float shaping) const
{
    const auto clamped = juce::jlimit (0.0f, 1.0f, level);
    const auto shaped = std::pow (clamped, shaping);
    const auto floor = 0.2f;
    const auto reachFactor = juce::jlimit (0.0f, 1.0f, floor + (1.0f - floor) * shaped);
    return speaker.maxReachScreen * visualizationScale * reachFactor;
}

void SpeakerVisualizerComponent::drawDirectionalLobes (juce::Graphics& g, const DrawOrder& order)
{
    for (const auto* speakerPtr : order)
    {
        const auto& speaker = *speakerPtr;

        if (speaker.definition.isLfe)
            continue;

        auto dir2D = speaker.orientation2D;
        auto len = dir2D.getDistanceFromOrigin();
        if (len <= 1.0e-4f)
            continue;
        dir2D /= len;

        const auto colour = colourForBands (speaker.metrics.bands, false);
        const auto level  = visualLevelForSpeaker (speaker);
        const auto reach  = reachForLevel (speaker, level, 0.55f);
        if (reach < 1.5f)
            continue;

        const auto spread = juce::jlimit (12.0f, reach * 0.45f, reach * 0.85f);
        const auto tail   = juce::jlimit (4.0f, reach * 0.22f, reach * 0.35f);

        const auto orientation = dir2D * reach;
        const auto perp        = juce::Point<float> (-dir2D.y, dir2D.x) * spread;
        const auto tailPoint   = speaker.projected - dir2D * tail;

        juce::Path lobe;
        lobe.startNewSubPath (tailPoint - perp * 0.35f);
        lobe.lineTo           (tailPoint + perp * 0.35f);
        lobe.lineTo           (speaker.projected + orientation);
        lobe.closeSubPath();

        g.setColour (colour.withAlpha (0.4f));
        g.fillPath (lobe);

        g.setColour (colour.withAlpha (0.22f));
        g.strokePath (lobe, juce::PathStrokeType (juce::jmax (1.4f, spread * 0.06f)));
    }
}

void SpeakerVisualizerComponent::drawLayeredLobes (juce::Graphics& g, const DrawOrder& order)
{
    static constexpr std::array<float, 3> reachMultipliers { 0.45f, 0.75f, 1.0f };
    static constexpr std::array<float, 3> spreadFactors    { 0.55f, 0.85f, 1.2f };
    static constexpr std::array<float, 3> alphaFactors     { 0.32f, 0.22f, 0.14f };

    for (const auto* speakerPtr : order)
    {
        const auto& speaker = *speakerPtr;

        if (speaker.definition.isLfe)
            continue;

        auto dir2D = speaker.orientation2D;
        auto len = dir2D.getDistanceFromOrigin();
        if (len <= 1.0e-4f)
            continue;
        dir2D /= len;

        const auto baseColour = colourForBands (speaker.metrics.bands, false);
        const auto level      = visualLevelForSpeaker (speaker);
        const auto baseReach  = reachForLevel (speaker, level, 0.62f);
        if (baseReach < 1.5f)
            continue;

        const auto tail = speaker.projected - dir2D * juce::jlimit (3.0f, baseReach * 0.18f, baseReach * 0.3f);

        for (size_t layer = 0; layer < reachMultipliers.size(); ++layer)
        {
            const auto reach = baseReach * reachMultipliers[layer];
            const auto spread = juce::jlimit (8.0f, reach * spreadFactors[layer], reach * 1.1f);

            juce::Path lobe;
            const auto orientation = dir2D * reach;
            const auto perp        = juce::Point<float> (-dir2D.y, dir2D.x) * spread;

            lobe.startNewSubPath (tail - perp * 0.42f);
            lobe.lineTo           (tail + perp * 0.42f);
            lobe.lineTo           (speaker.projected + orientation);
            lobe.closeSubPath();

            const auto alpha = juce::jlimit (0.05f, 0.7f, alphaFactors[layer] + level * 0.28f);

            g.setColour (baseColour.withAlpha (alpha));
            g.fillPath (lobe);

            g.setColour (baseColour.withAlpha (alpha * 0.55f));
            g.strokePath (lobe, juce::PathStrokeType (juce::jmax (1.2f, spread * 0.045f)));
        }
    }
}

void SpeakerVisualizerComponent::drawDirectivityBalloons (juce::Graphics& g, const DrawOrder& order)
{
    static constexpr std::array<float, 3> shellScales { 0.55f, 0.85f, 1.0f };
    static constexpr std::array<float, 3> shellAlphas { 0.26f, 0.18f, 0.12f };

    juce::Path unitCircle;
    unitCircle.addEllipse (-0.5f, -0.5f, 1.0f, 1.0f);

    for (const auto* speakerPtr : order)
    {
        const auto& speaker = *speakerPtr;

        auto dir2D = speaker.orientation2D;
        auto len2D = dir2D.getDistanceFromOrigin();
        if (len2D <= 1.0e-4f)
            dir2D = { 0.0f, -1.0f };
        else
            dir2D /= len2D;

        const auto colour = colourForBands (speaker.metrics.bands, speaker.definition.isLfe);
        const auto level  = visualLevelForSpeaker (speaker);
        const auto baseReach = reachForLevel (speaker, level, speaker.definition.isLfe ? 0.5f : 0.68f);
        if (baseReach < 2.0f)
            continue;

        for (size_t i = 0; i < shellScales.size(); ++i)
        {
            const auto reach = baseReach * shellScales[i];
            const auto major = juce::jlimit (12.0f, reach, reach * 1.1f);
            const auto minor = speaker.definition.isLfe ? major : juce::jlimit (8.0f, major * 0.55f, major);

            const auto centreOffset = speaker.definition.isLfe ? juce::Point<float> { 0.0f, 0.0f }
                                                                : dir2D * (major * 0.18f);
            const auto centre = speaker.projected + centreOffset;

            juce::Path shell (unitCircle);
            const auto orientation2D = speaker.definition.isLfe ? juce::Point<float> { 0.0f, -1.0f } : dir2D;
            shell.applyTransform (rotationTransform (centre, orientation2D, major, minor));

            const auto alpha = juce::jlimit (0.05f, 0.6f, shellAlphas[i] + level * 0.25f);
            g.setColour (colour.withAlpha (alpha));
            g.fillPath (shell);

            g.setColour (colour.withAlpha (alpha * 0.7f));
            g.strokePath (shell, juce::PathStrokeType (juce::jmax (1.0f, minor * 0.012f)));
        }
    }
}

void SpeakerVisualizerComponent::drawRadiationHeatmap (juce::Graphics& g)
{
    if (heatmapPoints.empty())
        return;

    std::vector<float> levels (heatmapPoints.size(), 0.0f);
    float frameMax = 0.0f;

    for (size_t i = 0; i < heatmapPoints.size(); ++i)
    {
        const auto& point = heatmapPoints[i];
        float level = 0.0f;

        for (const auto& speaker : speakers)
        {
            const auto amplitude = juce::jlimit (0.0f, 1.0f, speaker.metrics.rms);
            if (amplitude <= 1.0e-4f)
                continue;

            auto delta = point - speaker.definition.position;
            auto distance = delta.length();
            distance = std::max (distance, 0.65f);

            float directivity = 1.0f;
            if (! speaker.definition.isLfe)
            {
                auto ray = delta;
                auto rayLen = ray.length();
                if (rayLen > 1.0e-4f)
                {
                    ray /= rayLen;
                    auto aim = speaker.definition.aimDirection;
                    const auto aimLen = aim.length();
                    if (aimLen > 1.0e-4f)
                        aim /= aimLen;
                    directivity = std::max (0.0f, aim.x * ray.x + aim.y * ray.y + aim.z * ray.z);
                }
            }

            level += amplitude * directivity / (distance * distance);
        }

        levels[i] = level;
        frameMax = std::max (frameMax, level);
    }

    cachedHeatmapMaxLevel = juce::jmax (cachedHeatmapMaxLevel * 0.85f, frameMax);
    const auto normaliser = juce::jmax (0.12f, cachedHeatmapMaxLevel);

    for (size_t i = 0; i < heatmapPoints.size(); ++i)
    {
        const auto level = levels[i];
        if (level <= 1.0e-5f)
            continue;

        const auto projected = projectPoint (heatmapPoints[i]);
        if (cameraInside && projected.depth < -insideNearPlane)
            continue;

        const auto normalised = juce::jlimit (0.0f, 1.0f, level / normaliser);
        const auto colour = colourForLevel (level, normaliser);
        const auto size = juce::jmap (normalised, 0.0f, 1.0f, 4.0f, 18.0f * visualizationScale);

        g.setColour (colour.withAlpha (juce::jlimit (0.08f, 0.6f, normalised * 0.8f)));
        g.fillEllipse (projected.screen.x - size * 0.5f,
                       projected.screen.y - size * 0.5f,
                       size,
                       size);
    }
}

void SpeakerVisualizerComponent::drawTemporalTrails (juce::Graphics& g, const DrawOrder& order)
{
    for (const auto* speakerPtr : order)
    {
        const auto& speaker = *speakerPtr;

        if (speaker.trail.size() < 2)
            continue;

        const auto colour = colourForBands (speaker.metrics.bands, speaker.definition.isLfe);
        const auto thickness = speaker.definition.isLfe ? 2.0f : 2.4f;

        for (size_t i = 1; i < speaker.trail.size(); ++i)
        {
            const auto t = static_cast<float> (i) / static_cast<float> (speaker.trail.size() - 1);
            g.setColour (colour.withAlpha (juce::jlimit (0.05f, 0.65f, t * 0.65f)));
            g.drawLine (juce::Line<float> (speaker.trail[i - 1], speaker.trail[i]), thickness);
        }

        if (! speaker.definition.isLfe)
        {
            auto dir2D = speaker.orientation2D;
            auto len = dir2D.getDistanceFromOrigin();
            if (len <= 1.0e-4f)
                continue;
            dir2D /= len;

            const auto level  = visualLevelForSpeaker (speaker);
            const auto reach  = reachForLevel (speaker, level, 0.6f);
            g.setColour (colour.withAlpha (0.55f));
            g.drawLine (juce::Line<float> (speaker.projected,
                                           speaker.projected + dir2D * juce::jlimit (20.0f, reach, reach * 1.05f)),
                        2.2f);
        }
    }
}

void SpeakerVisualizerComponent::drawSpeakerBaseMarkers (juce::Graphics& g, const DrawOrder& order)
{
    for (const auto* speakerPtr : order)
    {
        const auto& speaker = *speakerPtr;
        const auto colour = colourForBands (speaker.metrics.bands, speaker.definition.isLfe);
        const auto level  = visualLevelForSpeaker (speaker);
        const auto size   = juce::jlimit (20.0f, 65.0f, 26.0f + level * 135.0f);
        const auto baseDiameter = size * 0.45f * juce::jlimit (0.6f, 1.5f, std::pow (visualizationScale, 0.25f));

        g.setColour (colour.withAlpha (0.95f));
        g.fillEllipse (speaker.projected.x - baseDiameter * 0.5f,
                       speaker.projected.y - baseDiameter * 0.5f,
                       baseDiameter,
                       baseDiameter);

        g.setColour (juce::Colours::white.withAlpha (0.5f));
        g.drawEllipse (speaker.projected.x - baseDiameter * 0.5f,
                       speaker.projected.y - baseDiameter * 0.5f,
                       baseDiameter,
                       baseDiameter,
                       1.4f);

        const auto labelBounds = juce::Rectangle<float> (speaker.projected.x - 70.0f,
                                                         speaker.projected.y + baseDiameter * 0.75f,
                                                         140.0f,
                                                         18.0f).toNearestInt();

        g.setColour (juce::Colours::white);
        g.drawFittedText (speaker.definition.displayName, labelBounds, juce::Justification::centred, 1);
    }
}

void SpeakerVisualizerComponent::updateHeatmapCache()
{
    heatmapPoints.clear();

    static constexpr std::array<int, 5> lateralSteps { 5, 7, 9, 11, 13 };
    static constexpr std::array<int, 5> verticalSteps { 3, 5, 7, 9, 11 };

    const auto index = juce::jlimit (0, (int) lateralSteps.size() - 1, heatmapDensityLevel - 1);
    const int depthSteps = lateralSteps[(size_t) index];
    const int widthSteps = lateralSteps[(size_t) index];
    const int heightSteps = verticalSteps[(size_t) index];

    const auto depthHalf = roomDimensions.depth * 0.5f;
    const auto widthHalf = roomDimensions.width * 0.5f;
    const auto floorY = -roomDimensions.earHeight;
    const auto ceilingY = roomDimensions.height - roomDimensions.earHeight;

    heatmapPoints.reserve (depthSteps * widthSteps * heightSteps);

    for (int y = 0; y < heightSteps; ++y)
    {
        const auto fy = juce::jmap ((float) y, 0.0f, (float) (heightSteps - 1), floorY + 0.12f, ceilingY - 0.12f);

        for (int z = 0; z < widthSteps; ++z)
        {
            const auto fz = juce::jmap ((float) z, 0.0f, (float) (widthSteps - 1), -widthHalf * 0.92f, widthHalf * 0.92f);

            for (int x = 0; x < depthSteps; ++x)
            {
                const auto fx = juce::jmap ((float) x, 0.0f, (float) (depthSteps - 1), -depthHalf * 0.92f, depthHalf * 0.92f);
                heatmapPoints.emplace_back (fx, fy, fz);
            }
        }
    }

    cachedHeatmapMaxLevel = 0.0f;
}

void SpeakerVisualizerComponent::updateTrails()
{
    for (auto& speaker : speakers)
    {
        if (cameraInside && speaker.depth < -insideNearPlane)
        {
            if (! speaker.trail.empty())
                speaker.trail.clear();
            continue;
        }

        if (! speaker.trail.empty()
            && speaker.projected.getDistanceFrom (speaker.trail.back()) < 0.25f)
        {
            continue;
        }

        speaker.trail.push_back (speaker.projected);
        while (speaker.trail.size() > trailHistoryLength)
            speaker.trail.pop_front();
    }
}

juce::Colour SpeakerVisualizerComponent::colourForLevel (float level, float maxLevel) const
{
    if (maxLevel <= 1.0e-6f)
        return juce::Colours::transparentBlack;

    const auto ratio = juce::jlimit (0.0f, 1.0f, level / juce::jmax (maxLevel, 1.0e-6f));

    if (ratio <= 1.0e-5f)
        return juce::Colour::fromFloatRGBA (0.18f, 0.21f, 0.28f, 1.0f);

    return colourForHeatmapRatio (ratio);
}

juce::AffineTransform SpeakerVisualizerComponent::rotationTransform (juce::Point<float> centre,
                                                                     juce::Point<float> direction,
                                                                     float width,
                                                                     float height) const
{
    auto dir = direction;
    auto len = dir.getDistanceFromOrigin();
    if (len > 1.0e-4f)
        dir /= len;
    else
        dir = { 0.0f, -1.0f };

    const auto angle = std::atan2 (dir.y, dir.x);

    return juce::AffineTransform::scale (width, height)
        .rotated (angle)
        .translated (centre.x, centre.y);
}

void SpeakerVisualizerComponent::applyZoomFactorToCamera()
{
    cameraBaseDistance = juce::jmax (0.001f, cameraBaseDistance);
    zoomFactor = juce::jlimit (minZoomFactor, maxZoomFactor, zoomFactor);

    if (cameraInside)
        cameraDistance = 0.0f;
    else
        cameraDistance = juce::jmax (0.05f, cameraBaseDistance);

    updateProjectionScale();
}

void SpeakerVisualizerComponent::captureUserState()
{
    auto& state = getUserState (cameraInside);
    state.yaw = yaw;
    state.pitch = pitch;
    state.baseDistance = cameraBaseDistance;
    state.zoom = zoomFactor;
}

void SpeakerVisualizerComponent::restoreUserState (bool inside)
{
    const auto& state = getUserState (inside);
    yaw = state.yaw;
    pitch = state.pitch;
    cameraBaseDistance = juce::jmax (0.001f, state.baseDistance);
    zoomFactor = juce::jlimit (minZoomFactor, maxZoomFactor, state.zoom);
    applyZoomFactorToCamera();
}

SpeakerVisualizerComponent::CameraState& SpeakerVisualizerComponent::getUserState (bool inside) noexcept
{
    return inside ? insideUserState : outsideUserState;
}

const SpeakerVisualizerComponent::CameraState& SpeakerVisualizerComponent::getUserState (bool inside) const noexcept
{
    return inside ? insideUserState : outsideUserState;
}

bool SpeakerVisualizerComponent::isInsidePreset (CameraPreset preset) noexcept
{
    switch (preset)
    {
        case CameraPreset::InsideHome:
        case CameraPreset::InsideUser:
        case CameraPreset::InsideFront:
        case CameraPreset::InsideBack:
        case CameraPreset::InsideLeft:
        case CameraPreset::InsideRight:
        case CameraPreset::InsideTop:
            return true;
        default:
            return false;
    }
}

bool SpeakerVisualizerComponent::isUserPreset (CameraPreset preset) noexcept
{
    return preset == CameraPreset::OutsideUser
        || preset == CameraPreset::InsideUser;
}

bool SpeakerVisualizerComponent::isHomePreset (CameraPreset preset) noexcept
{
    return preset == CameraPreset::OutsideHome
        || preset == CameraPreset::InsideHome;
}

const SpeakerVisualizerComponent::CameraParameters* SpeakerVisualizerComponent::findPresetDefinition (CameraPreset preset) const noexcept
{
    const auto iter = std::find_if (presetDefinitions.begin(), presetDefinitions.end(),
                                    [preset] (const PresetDefinition& entry)
                                    {
                                        return entry.preset == preset;
                                    });
    return iter != presetDefinitions.end() ? &iter->params : nullptr;
}

void SpeakerVisualizerComponent::promoteToUserPreset()
{
    captureUserState();
    const auto target = cameraInside ? CameraPreset::InsideUser : CameraPreset::OutsideUser;
    if (currentPreset != target)
        currentPreset = target;
    notifyPresetChanged();
}

void SpeakerVisualizerComponent::notifyPresetChanged()
{
    if (onPresetChanged)
        onPresetChanged (currentPreset);
}

void SpeakerVisualizerComponent::resized() {}

//==============================================================================
// AtmosVizAudioProcessorEditor
//==============================================================================
namespace
{
    constexpr float kPadHandleRadius = 9.0f;
    constexpr float kPadHandleOutline = 1.4f;

    SpeakerVisualizerComponent::BandColourWeights normalisedWeights (const SpeakerVisualizerComponent::BandColourWeights& weights)
    {
        const auto total = weights.low + weights.mid + weights.high;
        if (total <= 1.0e-6f)
            return { 1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f };

        return { weights.low / total, weights.mid / total, weights.high / total };
    }

    juce::String heatmapDensityLabelText (int level)
    {
        static const char* const labels[] { "Coarse", "Low", "Medium", "High", "Ultra" };
        const auto index = juce::jlimit (0, 4, level - 1);
        return juce::String (labels[index]);
    }
}

ColourMixPadComponent::ColourMixPadComponent()
{
    setInterceptsMouseClicks (true, true);
}

void ColourMixPadComponent::setWeights (SpeakerVisualizerComponent::BandColourWeights newWeights)
{
    updateWeightsInternal (newWeights, false);
}

juce::Rectangle<float> ColourMixPadComponent::getTriangleBounds() const
{
    auto area = getLocalBounds().toFloat().reduced (20.0f, 28.0f);
    if (area.getHeight() < 1.0f || area.getWidth() < 1.0f)
        return getLocalBounds().toFloat();
    return area;
}

juce::Point<float> ColourMixPadComponent::getCorner (int index) const
{
    const auto bounds = getTriangleBounds();
    const auto bottomY = bounds.getBottom();
    switch (index)
    {
        case 0: return { bounds.getX(), bottomY };
        case 1: return { bounds.getRight(), bottomY };
        case 2: return { bounds.getCentreX(), bounds.getY() };
        default: return bounds.getCentre();
    }
}

juce::Point<float> ColourMixPadComponent::getCentre() const
{
    const auto a = getCorner (0);
    const auto b = getCorner (1);
    const auto c = getCorner (2);
    return { (a.x + b.x + c.x) / 3.0f, (a.y + b.y + c.y) / 3.0f };
}

SpeakerVisualizerComponent::BandColourWeights ColourMixPadComponent::getNormalisedWeights() const
{
    return normalisedWeights (weights);
}

juce::Point<float> ColourMixPadComponent::getHandlePosition (int index) const
{
    const auto centre = getCentre();
    const auto corner = getCorner (index);
    const auto normalised = getNormalisedWeights();
    const std::array<float, 3> shares { normalised.low, normalised.mid, normalised.high };
    const auto share = juce::jlimit (0.0f, 1.0f, shares[juce::jlimit (0, 2, index)]);
    return centre + (corner - centre) * share;
}

void ColourMixPadComponent::updateWeightsInternal (SpeakerVisualizerComponent::BandColourWeights newWeights, bool sendCallback)
{
    newWeights.low = std::max (0.0f, newWeights.low);
    newWeights.mid = std::max (0.0f, newWeights.mid);
    newWeights.high = std::max (0.0f, newWeights.high);

    if (std::abs (newWeights.low - weights.low) < 1.0e-4f
        && std::abs (newWeights.mid - weights.mid) < 1.0e-4f
        && std::abs (newWeights.high - weights.high) < 1.0e-4f)
        return;

    weights = newWeights;
    repaint();

    if (sendCallback && onWeightsChanged)
        onWeightsChanged (weights);
}

float ColourMixPadComponent::distanceToHandle (int index, juce::Point<float> point) const
{
    return getHandlePosition (index).getDistanceFrom (point);
}

float ColourMixPadComponent::shareForPoint (int index, juce::Point<float> point) const
{
    const auto centre = getCentre();
    const auto corner = getCorner (index);
    const auto vector = corner - centre;
    const auto lengthSquared = vector.getDistanceSquaredFromOrigin();
    if (lengthSquared <= 1.0e-6f)
        return 0.0f;

    const auto relative = (point - centre).getDotProduct (vector) / lengthSquared;
    return juce::jlimit (0.0f, 1.0f, relative);
}

void ColourMixPadComponent::resized()
{
    repaint();
}

void ColourMixPadComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.82f));

    const auto bounds = getTriangleBounds();
    juce::Path triangle;
    triangle.addTriangle (getCorner (0), getCorner (1), getCorner (2));

    g.setColour (juce::Colours::darkgrey.withAlpha (0.35f));
    g.fillPath (triangle);

    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.strokePath (triangle, juce::PathStrokeType (1.4f));

    const auto centre = getCentre();
    const std::array<juce::Colour, 3> bandColours
    {
        juce::Colour::fromFloatRGBA (0.35f, 0.55f, 0.95f, 1.0f),
        juce::Colour::fromFloatRGBA (0.35f, 0.85f, 0.45f, 1.0f),
        juce::Colour::fromFloatRGBA (0.98f, 0.45f, 0.35f, 1.0f)
    };
    const std::array<juce::String, 3> labels { "Low", "Mid", "High" };

    for (int i = 0; i < 3; ++i)
    {
        const auto corner = getCorner (i);
        g.setColour (bandColours[i].withAlpha (0.35f));
        g.drawLine (juce::Line<float> (centre, corner), 1.1f);

        auto labelArea = juce::Rectangle<float> (corner.x - 32.0f, corner.y - 20.0f, 64.0f, 18.0f);
        if (i == 2)
            labelArea.setCentre (corner.x, corner.y - 18.0f);
        g.setColour (juce::Colours::white.withAlpha (0.8f));
        g.drawFittedText (labels[i], labelArea.toNearestInt(), juce::Justification::centred, 1);
    }

    const auto normalised = getNormalisedWeights();
    const std::array<float, 3> shares { normalised.low, normalised.mid, normalised.high };

    for (int i = 0; i < 3; ++i)
    {
        const auto handle = getHandlePosition (i);
        g.setColour (bandColours[i].withAlpha (0.9f));
        g.fillEllipse (handle.x - kPadHandleRadius,
                       handle.y - kPadHandleRadius,
                       kPadHandleRadius * 2.0f,
                       kPadHandleRadius * 2.0f);

        g.setColour (juce::Colours::white.withAlpha (0.75f));
        g.drawEllipse (handle.x - kPadHandleRadius,
                       handle.y - kPadHandleRadius,
                       kPadHandleRadius * 2.0f,
                       kPadHandleRadius * 2.0f,
                       kPadHandleOutline);

        auto valueLabel = juce::Rectangle<int> ((int) std::round (handle.x - 28.0f),
                                                (int) std::round (handle.y - 36.0f),
                                                56,
                                                18);
        g.drawFittedText (juce::String (juce::roundToInt (shares[i] * 100.0f)) + " %",
                          valueLabel,
                          juce::Justification::centred,
                          1);
    }
}

void ColourMixPadComponent::mouseDown (const juce::MouseEvent& e)
{
    const auto pos = e.position;
    activeHandle = -1;
    for (int i = 0; i < 3; ++i)
    {
        if (distanceToHandle (i, pos) <= kPadHandleRadius * 1.6f)
        {
            activeHandle = i;
            break;
        }
    }

    if (activeHandle < 0)
        return;

    startWeights = weights;
    startNormalised = getNormalisedWeights();
    startTotal = weights.low + weights.mid + weights.high;
    if (startTotal <= 1.0e-6f)
    {
        startTotal = 3.0f;
        startWeights = { 1.0f, 1.0f, 1.0f };
        startNormalised = normalisedWeights (startWeights);
    }
}

void ColourMixPadComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (activeHandle < 0)
        return;

    const auto newShare = shareForPoint (activeHandle, e.position);

    std::array<float, 3> startShares { startNormalised.low, startNormalised.mid, startNormalised.high };
    std::array<float, 3> newShares = startShares;

    const float otherSum = startShares[0] + startShares[1] + startShares[2] - startShares[activeHandle];
    const float remaining = 1.0f - newShare;
    newShares[activeHandle] = newShare;

    if (otherSum <= 1.0e-6f)
    {
        const int first = (activeHandle + 1) % 3;
        const int second = (activeHandle + 2) % 3;
        newShares[first] = remaining * 0.5f;
        newShares[second] = remaining - newShares[first];
    }
    else
    {
        for (int i = 0; i < 3; ++i)
        {
            if (i == activeHandle)
                continue;
            newShares[i] = juce::jlimit (0.0f, 1.0f, (startShares[i] / otherSum) * remaining);
        }
    }

    const auto total = std::max (startTotal, 1.0e-3f);
    SpeakerVisualizerComponent::BandColourWeights newWeights
    {
        newShares[0] * total,
        newShares[1] * total,
        newShares[2] * total
    };

    updateWeightsInternal (newWeights, true);
}

void ColourMixPadComponent::mouseUp (const juce::MouseEvent&)
{
    activeHandle = -1;
}void ColourLegendComponent::setLegend (juce::String newTitle,
                                                 std::vector<Stop> newStops,
                                                 juce::String left,
                                                 juce::String centre,
                                                 juce::String right)
{
    title = std::move (newTitle);
    leftLabel = std::move (left);
    centreLabel = std::move (centre);
    rightLabel = std::move (right);

    std::sort (newStops.begin(), newStops.end(),
               [] (const Stop& a, const Stop& b) { return a.position < b.position; });

    for (auto& stop : newStops)
        stop.position = juce::jlimit (0.0f, 1.0f, stop.position);

    stops = std::move (newStops);
    repaint();
}

void ColourLegendComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    if (bounds.getWidth() < 1.0f || bounds.getHeight() < 1.0f)
        return;

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (bounds, 6.0f);

    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.drawRoundedRectangle (bounds, 6.0f, 1.0f);

    auto content = bounds.reduced (8.0f, 6.0f);
    auto titleArea = content.removeFromTop (16.0f);
    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.setFont (juce::Font (13.0f, juce::Font::bold));
    g.drawFittedText (title, titleArea.toNearestInt(), juce::Justification::centredLeft, 1);

    content.removeFromTop (4.0f);
    auto gradientArea = content.removeFromTop (content.getHeight() * 0.55f).reduced (4.0f, 0.0f);

    if (! stops.empty())
    {
        if (stops.size() == 1)
        {
            g.setColour (stops.front().colour);
            g.fillRoundedRectangle (gradientArea, 4.0f);
        }
        else
        {
            auto first = stops.front();
            auto last = stops.back();
            juce::ColourGradient gradient (first.colour, gradientArea.getX(), gradientArea.getCentreY(),
                                           last.colour, gradientArea.getRight(), gradientArea.getCentreY(), false);
            for (size_t i = 1; i + 1 < stops.size(); ++i)
                gradient.addColour (stops[i].position, stops[i].colour);

            g.setGradientFill (gradient);
            g.fillRoundedRectangle (gradientArea, 4.0f);
        }

        g.setColour (juce::Colours::white.withAlpha (0.25f));
        g.drawRoundedRectangle (gradientArea, 4.0f, 1.0f);
    }

    auto labelsArea = content.removeFromBottom (16.0f);
    auto thirdWidth = labelsArea.getWidth() / 3.0f;
    auto leftArea = labelsArea.removeFromLeft (thirdWidth);
    auto centreArea = labelsArea.removeFromLeft (thirdWidth);
    auto rightArea = labelsArea;

    g.setColour (juce::Colours::white.withAlpha (0.75f));
    g.setFont (juce::Font (11.0f));
    g.drawFittedText (leftLabel, leftArea.toNearestInt(), juce::Justification::centred, 1);
    g.drawFittedText (centreLabel, centreArea.toNearestInt(), juce::Justification::centred, 1);
    g.drawFittedText (rightLabel, rightArea.toNearestInt(), juce::Justification::centred, 1);
}

AtmosVizAudioProcessorEditor::AtmosVizAudioProcessorEditor (AtmosVizAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    visualizer = std::make_unique<SpeakerVisualizerComponent> (audioProcessor);
    addAndMakeVisible (*visualizer);

    visualizer->onZoomFactorChanged = [this] (float factor)
    {
        if (sliderControlMode == SliderControlMode::Zoom)
            zoomSlider.setValue (factor, juce::dontSendNotification);
    };

    createCameraButtons();

    visualizer->onPresetChanged = [this] (SpeakerVisualizerComponent::CameraPreset preset)
    {
        juce::ignoreUnused (preset);
        updateCameraButtonStates();
    };

    visualizer->onVisualizationModeChanged = [this] (SpeakerVisualizerComponent::VisualizationMode)
    {
        updateVisualizationSelector();
        updateVisualizationControlsVisibility();
        updateLegendContent();
    };

    visualizer->onVisualizationScaleChanged = [this] (float value)
    {
        if (sliderControlMode == SliderControlMode::DrawScale)
            zoomSlider.setValue (value, juce::dontSendNotification);
    };

    setupZoomSlider();
    setupSliderModeSelector();
    setupVisualizationSelector();
    setupHeatmapDensitySlider();
    setupBandWeightControls();
    setupColourLegend();
    if (visualizer != nullptr)
        syncBandControlsWithWeights (visualizer->getBandColourWeights());


    outsideLabel.setText ("Outside", juce::dontSendNotification);
    outsideLabel.setJustificationType (juce::Justification::centredLeft);
    outsideLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.85f));
    outsideLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (outsideLabel);

    insideLabel.setText ("Inside", juce::dontSendNotification);
    insideLabel.setJustificationType (juce::Justification::centredLeft);
    insideLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.85f));
    insideLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (insideLabel);

    updateCameraButtonStates();
    updateSliderConfiguration();
    updateVisualizationSelector();
    updateVisualizationControlsVisibility();
    updateLegendContent();

    setResizable (true, true);
    setResizeLimits (600, 400, 1600, 1000);
    setSize (1000, 650);
}

AtmosVizAudioProcessorEditor::~AtmosVizAudioProcessorEditor()
{
    closeColourMixPad();
    zoomSlider.removeListener (this);
}

void AtmosVizAudioProcessorEditor::createCameraButtons()
{
    cameraButtons.clear();
    outsideButtonIndices.clear();
    insideButtonIndices.clear();

    struct ButtonSpec
    {
        const char* text;
        SpeakerVisualizerComponent::CameraPreset preset;
        int row;
    };

    static constexpr std::array<ButtonSpec, 7> outsideSpecs
    {{
        { "Home",  SpeakerVisualizerComponent::CameraPreset::OutsideHome, 0 },
        { "User",  SpeakerVisualizerComponent::CameraPreset::OutsideUser, 0 },
        { "Front", SpeakerVisualizerComponent::CameraPreset::OutsideFront, 0 },
        { "Back",  SpeakerVisualizerComponent::CameraPreset::OutsideBack, 0 },
        { "Left",  SpeakerVisualizerComponent::CameraPreset::OutsideLeft, 0 },
        { "Right", SpeakerVisualizerComponent::CameraPreset::OutsideRight, 0 },
        { "Top",   SpeakerVisualizerComponent::CameraPreset::OutsideTop, 0 }
    }};

    static constexpr std::array<ButtonSpec, 7> insideSpecs
    {{
        { "Home",  SpeakerVisualizerComponent::CameraPreset::InsideHome, 1 },
        { "User",  SpeakerVisualizerComponent::CameraPreset::InsideUser, 1 },
        { "Front", SpeakerVisualizerComponent::CameraPreset::InsideFront, 1 },
        { "Back",  SpeakerVisualizerComponent::CameraPreset::InsideBack, 1 },
        { "Left",  SpeakerVisualizerComponent::CameraPreset::InsideLeft, 1 },
        { "Right", SpeakerVisualizerComponent::CameraPreset::InsideRight, 1 },
        { "Top",   SpeakerVisualizerComponent::CameraPreset::InsideTop, 1 }
    }};

    auto addButton = [this] (const ButtonSpec& spec)
    {
        CameraButtonInfo info;
        info.preset = spec.preset;
        info.row = spec.row;
        info.button = std::make_unique<juce::TextButton> (spec.text);
        info.button->setClickingTogglesState (false);
        info.button->onClick = [this, preset = spec.preset]
        {
            setCameraPreset (preset);
        };
        addAndMakeVisible (*info.button);
        cameraButtons.emplace_back (std::move (info));
        const auto index = cameraButtons.size() - 1;
        if (spec.row == 0)
            outsideButtonIndices.push_back (index);
        else
            insideButtonIndices.push_back (index);
    };

    for (const auto& spec : outsideSpecs)
        addButton (spec);

    for (const auto& spec : insideSpecs)
        addButton (spec);
}

void AtmosVizAudioProcessorEditor::setupZoomSlider()
{
    zoomSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    zoomSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 64, 20);
    zoomSlider.setDoubleClickReturnValue (true, 1.0);
    zoomSlider.addListener (this);
    addAndMakeVisible (zoomSlider);
}

void AtmosVizAudioProcessorEditor::setupSliderModeSelector()
{
    sliderModeCombo.addItem ("Zoom (10%-200%)", 1);
    sliderModeCombo.addItem ("Draw Scale (relative %)", 2);
    sliderModeCombo.onChange = [this]
    {
        const auto id = sliderModeCombo.getSelectedId();
        sliderControlMode = (id == 2) ? SliderControlMode::DrawScale : SliderControlMode::Zoom;
        updateSliderConfiguration();
    };
    sliderModeCombo.setSelectedId (1, juce::dontSendNotification);
    sliderModeCombo.setJustificationType (juce::Justification::centredLeft);
    sliderModeCombo.setTooltip ("Select how the main slider behaves");
    addAndMakeVisible (sliderModeCombo);
}

void AtmosVizAudioProcessorEditor::setupVisualizationSelector()
{
    visualizationLabel.setText ("View Mode", juce::dontSendNotification);
    visualizationLabel.setJustificationType (juce::Justification::centredLeft);
    visualizationLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    visualizationLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (visualizationLabel);

    visualizationCombo.addItem ("Directional Lobes", 1 + (int) SpeakerVisualizerComponent::VisualizationMode::DirectionalLobes);
    visualizationCombo.addItem ("Layered Lobes",      1 + (int) SpeakerVisualizerComponent::VisualizationMode::LayeredLobes);
    visualizationCombo.addItem ("Directivity Balloon",1 + (int) SpeakerVisualizerComponent::VisualizationMode::DirectivityBalloon);
    visualizationCombo.addItem ("Radiation Heatmap",  1 + (int) SpeakerVisualizerComponent::VisualizationMode::RadiationHeatmap);
    visualizationCombo.addItem ("Temporal Trails",    1 + (int) SpeakerVisualizerComponent::VisualizationMode::TemporalTrail);
    visualizationCombo.setTooltip ("Select how speaker radiation is visualized");
    visualizationCombo.setJustificationType (juce::Justification::centredLeft);

    if (visualizer != nullptr)
        visualizationCombo.setSelectedId (1 + (int) visualizer->getVisualizationMode(), juce::dontSendNotification);

    visualizationCombo.onChange = [this]
    {
        if (visualizer != nullptr)
        {
            const auto selected = visualizationCombo.getSelectedId();
            if (selected > 0)
            {
                const auto mode = static_cast<SpeakerVisualizerComponent::VisualizationMode> (selected - 1);
                visualizer->setVisualizationMode (mode);
            }
        }
    };

    addAndMakeVisible (visualizationCombo);
}

void AtmosVizAudioProcessorEditor::setupHeatmapDensitySlider()
{
    heatmapDensityLabel.setText ("Heatmap Density", juce::dontSendNotification);
    heatmapDensityLabel.setJustificationType (juce::Justification::centredRight);
    heatmapDensityLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.85f));
    heatmapDensityLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (heatmapDensityLabel);

    heatmapDensityValueLabel.setJustificationType (juce::Justification::centredLeft);
    heatmapDensityValueLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.85f));
    heatmapDensityValueLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (heatmapDensityValueLabel);

    heatmapDensitySlider.setSliderStyle (juce::Slider::LinearHorizontal);
    heatmapDensitySlider.setRange (1.0, 5.0, 1.0);
    heatmapDensitySlider.setDoubleClickReturnValue (true, 2.0);
    heatmapDensitySlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    heatmapDensitySlider.setPopupDisplayEnabled (true, false, this);
    heatmapDensitySlider.textFromValueFunction = [] (double value)
    {
        return heatmapDensityLabelText ((int) std::round (value));
    };
    heatmapDensitySlider.valueFromTextFunction = [] (const juce::String& text)
    {
        auto trimmed = text.trim().toLowerCase();
        if (trimmed.startsWith ("coarse")) return 1.0;
        if (trimmed.startsWith ("low"))    return 2.0;
        if (trimmed.startsWith ("medium")) return 3.0;
        if (trimmed.startsWith ("high"))   return 4.0;
        if (trimmed.startsWith ("ultra"))  return 5.0;
        return juce::jlimit (1.0, 5.0, text.getDoubleValue());
    };
    heatmapDensitySlider.setTooltip ("Adjust the sampling density used for the heatmap");
    heatmapDensitySlider.onValueChange = [this]
    {
        if (visualizer == nullptr)
            return;

        const auto density = (int) std::round (heatmapDensitySlider.getValue());
        visualizer->setHeatmapDensity (density);
        updateHeatmapDensityValueLabel();
        updateLegendContent();
    };

    const auto initialDensity = (visualizer != nullptr)
                                  ? visualizer->getHeatmapDensity()
                                  : 2;
    heatmapDensitySlider.setValue (initialDensity, juce::dontSendNotification);

    updateHeatmapDensityValueLabel();

    heatmapDensityLabel.setVisible (false);
    heatmapDensitySlider.setVisible (false);
    heatmapDensityValueLabel.setVisible (false);
    addAndMakeVisible (heatmapDensitySlider);
}

void AtmosVizAudioProcessorEditor::setupBandWeightControls()
{
    auto configureSlider = [] (juce::Slider& slider)
    {
        slider.setSliderStyle (juce::Slider::LinearHorizontal);
        slider.setRange (0.0, 200.0, 1.0);
        slider.setDoubleClickReturnValue (true, 100.0);
        slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 20);
        slider.textFromValueFunction = [] (double value)
        {
            return juce::String (juce::roundToInt (value)) + " %";
        };
        slider.valueFromTextFunction = [] (const juce::String& text)
        {
            return juce::jlimit (0.0, 200.0, text.upToFirstOccurrenceOf ("%", false, false).getDoubleValue());
        };
    };

    configureSlider (lowWeightSlider);
    configureSlider (midWeightSlider);
    configureSlider (highWeightSlider);

    auto configureLabel = [] (juce::Label& label, const juce::String& text, juce::Colour colour)
    {
        label.setText (text, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centredLeft);
        label.setColour (juce::Label::textColourId, colour);
        label.setInterceptsMouseClicks (false, false);
    };

    configureLabel (lowWeightLabel,  "Low",  juce::Colour::fromFloatRGBA (0.45f, 0.65f, 1.0f, 0.95f));
    configureLabel (midWeightLabel,  "Mid",  juce::Colour::fromFloatRGBA (0.45f, 0.9f, 0.45f, 0.95f));
    configureLabel (highWeightLabel, "High", juce::Colour::fromFloatRGBA (0.98f, 0.42f, 0.42f, 0.95f));

    bandWeightTitleLabel.setText ("Band Colour Weights", juce::dontSendNotification);
    bandWeightTitleLabel.setJustificationType (juce::Justification::centredLeft);
    bandWeightTitleLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.9f));
    bandWeightTitleLabel.setInterceptsMouseClicks (false, false);
    bandWeightTitleLabel.setTooltip ("Adjust how strongly each frequency band influences the colour mix");
    addAndMakeVisible (bandWeightTitleLabel);

    lowWeightSlider.setTooltip ("Adjust colour emphasis for low-frequency content");
    midWeightSlider.setTooltip ("Adjust colour emphasis for mid-frequency content");
    highWeightSlider.setTooltip ("Adjust colour emphasis for high-frequency content");

    addAndMakeVisible (lowWeightLabel);
    addAndMakeVisible (midWeightLabel);
    addAndMakeVisible (highWeightLabel);
    addAndMakeVisible (lowWeightSlider);
    addAndMakeVisible (midWeightSlider);
    addAndMakeVisible (highWeightSlider);

    colourPadButton.setButtonText ("Colour Mix Pad");
    colourPadButton.setTooltip ("Open the triangular colour mixing pad");
    colourPadButton.onClick = [this] { showColourMixPad(); };
    addAndMakeVisible (colourPadButton);

    lowWeightSlider.onValueChange = [this] { applyBandWeightChanges(); };
    midWeightSlider.onValueChange = [this] { applyBandWeightChanges(); };
    highWeightSlider.onValueChange = [this] { applyBandWeightChanges(); };

    if (visualizer != nullptr)
        syncBandControlsWithWeights (visualizer->getBandColourWeights());
    else
        syncBandControlsWithWeights ({ 1.0f, 1.0f, 1.0f });
}

void AtmosVizAudioProcessorEditor::setupColourLegend()
{
    colourLegend.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (colourLegend);
}



void AtmosVizAudioProcessorEditor::updateSliderConfiguration()
{
    if (visualizer == nullptr)
        return;

    const bool useDrawScale = (sliderControlMode == SliderControlMode::DrawScale);
    sliderModeCombo.changeItemText (1, "Zoom (10%-200%)");
    sliderModeCombo.changeItemText (2, "Draw Scale (relative %)");
    sliderModeCombo.setSelectedId (useDrawScale ? 2 : 1, juce::dontSendNotification);

    if (! useDrawScale)
    {
        zoomSlider.setRange (visualizer->getMinZoomFactor(), visualizer->getMaxZoomFactor(), 0.01);
        zoomSlider.setDoubleClickReturnValue (true, 1.0);
        zoomSlider.textFromValueFunction = [] (double value)
        {
            return juce::String (juce::roundToInt (value * 100.0)) + " %";
        };
        zoomSlider.valueFromTextFunction = [] (const juce::String& text)
        {
            auto numeric = text.upToFirstOccurrenceOf ("%", false, false)
                               .retainCharacters ("0123456789.,-")
                               .replaceCharacter (',', '.')
                               .getDoubleValue();
            return juce::jlimit (0.1, 2.0, numeric / 100.0);
        };
        zoomSlider.setTooltip ("Zoom (10%-200%)");
        zoomSlider.setValue (visualizer->getZoomFactor(), juce::dontSendNotification);
    }
    else
    {
        zoomSlider.setRange (-100.0, 100.0, 1.0);
        zoomSlider.setDoubleClickReturnValue (true, 0.0);
        zoomSlider.textFromValueFunction = [] (double value)
        {
            const auto factor = std::pow (2.0, value / 100.0);
            const auto percent = (factor - 1.0) * 100.0;
            const auto prefix = percent >= 0.0 ? "+" : "";
            return juce::String (prefix) + juce::String (juce::roundToInt (percent)) + " %";
        };
        zoomSlider.valueFromTextFunction = [] (const juce::String& text)
        {
            auto numeric = text.retainCharacters ("0123456789+-.%")
                               .replaceCharacters (",", ".")
                               .upToFirstOccurrenceOf ("%", false, false)
                               .getDoubleValue();
            const auto factor = juce::jmax (0.05, 1.0 + numeric / 100.0);
            return juce::jlimit (-100.0, 100.0, std::log2 (factor) * 100.0);
        };
        zoomSlider.setTooltip ("Draw Scale (adjust reach of lobes at full level)");
        zoomSlider.setValue (visualizer->getVisualizationScaleAdjustment(), juce::dontSendNotification);
    }

    sliderModeCombo.setTooltip (useDrawScale ? "Slider controls Draw Scale (relative reach)" : "Slider controls camera zoom (10%-200%)");
}
void AtmosVizAudioProcessorEditor::updateVisualizationControlsVisibility()
{
    if (visualizer == nullptr)
    {
        heatmapDensitySlider.setVisible (false);
        heatmapDensityLabel.setVisible (false);
        heatmapDensityValueLabel.setVisible (false);
        return;
    }

    const bool showHeatmap = visualizer->getVisualizationMode() == SpeakerVisualizerComponent::VisualizationMode::RadiationHeatmap;
    heatmapDensitySlider.setVisible (showHeatmap);
    heatmapDensityLabel.setVisible (showHeatmap);
    heatmapDensityValueLabel.setVisible (showHeatmap);

    if (showHeatmap)
    {
        heatmapDensitySlider.setValue (visualizer->getHeatmapDensity(), juce::dontSendNotification);
        updateHeatmapDensityValueLabel();
    }

    if (getWidth() > 0 && getHeight() > 0)
        resized();
}

void AtmosVizAudioProcessorEditor::updateHeatmapDensityValueLabel()
{
    const auto density = (int) std::round (heatmapDensitySlider.getValue());
    heatmapDensityValueLabel.setText (heatmapDensityLabelText (density), juce::dontSendNotification);
}

void AtmosVizAudioProcessorEditor::updateLegendContent()
{
    if (visualizer == nullptr)
        return;

    std::vector<ColourLegendComponent::Stop> stops;
    const auto mode = visualizer->getVisualizationMode();

    if (mode == SpeakerVisualizerComponent::VisualizationMode::RadiationHeatmap)
    {
        static const std::array<float, 5> positions { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };
        for (auto position : positions)
            stops.push_back ({ position, visualizer->colourForHeatmapRatio (position) });

        colourLegend.setLegend ("Heatmap Intensity", std::move (stops), "Low", "Medium", "High");
        return;
    }

    stops.push_back ({ 0.0f, visualizer->colourForBandMix (1.0f, 0.0f, 0.0f) });
    stops.push_back ({ 0.5f, visualizer->colourForBandMix (0.0f, 1.0f, 0.0f) });
    stops.push_back ({ 1.0f, visualizer->colourForBandMix (0.0f, 0.0f, 1.0f) });

    colourLegend.setLegend ("Frequency Emphasis", std::move (stops), "Low", "Mid", "High");
}

void AtmosVizAudioProcessorEditor::applyBandWeightChanges()
{
    if (visualizer == nullptr || suppressBandSliderCallbacks)
        return;

    SpeakerVisualizerComponent::BandColourWeights weights
    {
        (float) (lowWeightSlider.getValue()  / 100.0),
        (float) (midWeightSlider.getValue()  / 100.0),
        (float) (highWeightSlider.getValue() / 100.0)
    };

    visualizer->setBandColourWeights (weights);

    if (colourPadComponent != nullptr && !suppressPadCallback)
        colourPadComponent->setWeights (weights);

    updateLegendContent();
}


void AtmosVizAudioProcessorEditor::updateVisualizationSelector()
{
    if (visualizer == nullptr)
        return;

    const auto mode = visualizer->getVisualizationMode();
    const auto targetId = 1 + (int) mode;

    if (visualizationCombo.getSelectedId() != targetId)
        visualizationCombo.setSelectedId (targetId, juce::dontSendNotification);

    updateVisualizationControlsVisibility();
    updateLegendContent();
}

void AtmosVizAudioProcessorEditor::syncBandControlsWithWeights (SpeakerVisualizerComponent::BandColourWeights weights)
{
    suppressBandSliderCallbacks = true;
    lowWeightSlider.setValue (weights.low * 100.0f, juce::dontSendNotification);
    midWeightSlider.setValue (weights.mid * 100.0f, juce::dontSendNotification);
    highWeightSlider.setValue (weights.high * 100.0f, juce::dontSendNotification);
    suppressBandSliderCallbacks = false;

    if (colourPadComponent != nullptr)
    {
        suppressPadCallback = true;
        colourPadComponent->setWeights (weights);
        suppressPadCallback = false;
    }
}

void AtmosVizAudioProcessorEditor::showColourMixPad()
{
    if (colourPadCallout != nullptr)
    {
        closeColourMixPad();
        return;
    }

    auto pad = std::make_unique<ColourMixPadComponent>();
    auto* padPtr = pad.get();

    constexpr int padWidth = 320;
    constexpr int padHeight = 280;
    padPtr->setSize (padWidth, padHeight);

    if (visualizer != nullptr)
        padPtr->setWeights (visualizer->getBandColourWeights());
    else
        padPtr->setWeights ({ 1.0f, 1.0f, 1.0f });

    padPtr->onWeightsChanged = [this] (SpeakerVisualizerComponent::BandColourWeights newWeights)
    {
        if (visualizer == nullptr)
            return;

        suppressPadCallback = true;
        visualizer->setBandColourWeights (newWeights);
        suppressPadCallback = false;

        syncBandControlsWithWeights (newWeights);
        updateLegendContent();
    };

    auto bounds = colourPadButton.getBoundsInParent();
    auto& callout = juce::CallOutBox::launchAsynchronously (std::move(pad), bounds, this);
    callout.addComponentListener (this);
    colourPadCallout = &callout;
    colourPadComponent = padPtr;
}

void AtmosVizAudioProcessorEditor::closeColourMixPad()
{
    if (colourPadCallout != nullptr)
    {
        colourPadCallout->removeComponentListener (this);
        colourPadCallout->dismiss();
        colourPadCallout = nullptr;
        colourPadComponent = nullptr;
        suppressPadCallback = false;
    }
}

void AtmosVizAudioProcessorEditor::componentBeingDeleted (juce::Component& component)
{
    if (colourPadCallout != nullptr && &component == colourPadCallout)
    {
        component.removeComponentListener (this);
        colourPadCallout = nullptr;
        colourPadComponent = nullptr;
        suppressPadCallback = false;
    }
}

void AtmosVizAudioProcessorEditor::setCameraPreset (SpeakerVisualizerComponent::CameraPreset preset)
{
    if (visualizer != nullptr)
        visualizer->setCameraPreset (preset);

    updateCameraButtonStates();
    zoomSlider.setValue (visualizer->getZoomFactor(), juce::dontSendNotification);
}

void AtmosVizAudioProcessorEditor::updateCameraButtonStates()
{
    if (visualizer == nullptr)
        return;

    const auto current = visualizer->getCameraPreset();

    for (auto& entry : cameraButtons)
    {
        const bool active = entry.preset == current;
        entry.button->setToggleState (active, juce::dontSendNotification);
        entry.button->setAlpha (active ? 1.0f : 0.8f);
    }
}

void AtmosVizAudioProcessorEditor::sliderValueChanged (juce::Slider* slider)
{
    if (slider != &zoomSlider || visualizer == nullptr)
        return;

    if (sliderControlMode == SliderControlMode::Zoom)
    {
        visualizer->setZoomFactor ((float) zoomSlider.getValue());
        updateCameraButtonStates();
    }
    else
    {
        visualizer->setVisualizationScaleAdjustment ((float) zoomSlider.getValue());
    }
}

void AtmosVizAudioProcessorEditor::paint (juce::Graphics& g)
{
    juce::ColourGradient gradient (juce::Colours::black, {},
                                   juce::Colours::darkgrey.darker (0.35f), {},
                                   true);
    gradient.point1 = { 0.0f, 0.0f };
    gradient.point2 = { (float) getWidth(), (float) getHeight() };

    g.setGradientFill (gradient);
    g.fillAll();

    g.setColour (juce::Colours::white.withAlpha (0.14f));
    for (const auto& divider : sectionDividers)
        g.fillRect (divider);
}

void AtmosVizAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    const float scale = juce::jlimit (0.6f, 1.0f, bounds.getWidth() / 1100.0f);
    const int controlHeight = juce::roundToInt (30.0f * scale);
    const int spacing = juce::roundToInt (5.0f * scale);
    const int marginX = juce::roundToInt (10.0f * scale);
    const int marginY = juce::roundToInt (6.0f * scale);

    sectionDividers.clear();
    auto addDivider = [this, marginX] (int y)
    {
        const int width = std::max (0, getWidth() - marginX * 2);
        if (width > 0)
            sectionDividers.emplace_back (marginX, y, width, 1);
    };

    const int sliderModeMinWidth = juce::roundToInt (juce::jmax (120.0f, 140.0f * scale));
    const int sliderMinWidth = juce::roundToInt (juce::jmax (240.0f, 280.0f * scale));
    const int viewLabelWidth = juce::roundToInt (juce::jmax (55.0f, 60.0f * scale));
    const int viewComboMinWidth = juce::roundToInt (juce::jmax (140.0f, 160.0f * scale));

    const int widthRequirement = marginX * 2
                                 + sliderModeMinWidth
                                 + spacing
                                 + sliderMinWidth
                                 + spacing
                                 + viewLabelWidth + viewComboMinWidth;

    const bool stackCombos = bounds.getWidth() < widthRequirement;

    auto headerArea = bounds.reduced (marginX, marginY);
    int headerBottom = bounds.getY() + marginY;

    auto layoutButtonRow = [this, controlHeight, spacing, scale] (juce::Rectangle<int> area,
                                                                  const std::vector<size_t>& indices,
                                                                  juce::Label& label)
    {
        const int labelWidth = juce::roundToInt (juce::jmax (60.0f * scale, 52.0f));
        auto labelArea = area.removeFromLeft (labelWidth);
        label.setBounds (labelArea.withHeight (controlHeight));
        area.removeFromLeft (spacing);

        juce::FlexBox box;
        box.flexWrap = juce::FlexBox::Wrap::wrap;
        box.justifyContent = juce::FlexBox::JustifyContent::flexStart;
        box.alignContent = juce::FlexBox::AlignContent::center;

        const float minButtonWidth = juce::jmax (70.0f, 90.0f * scale);
        const float buttonHeight = (float) controlHeight;
        const float buttonMargin = juce::jlimit (2.0f, 10.0f, 6.0f * scale);

        for (auto index : indices)
        {
            if (index >= cameraButtons.size())
                continue;

            auto& entry = cameraButtons[index];

            juce::FlexItem item (*entry.button);
            item.minWidth = minButtonWidth;
            item.minHeight = buttonHeight;
            item.maxHeight = buttonHeight;
            item.margin = juce::FlexItem::Margin (0.0f, buttonMargin, buttonMargin, 0.0f);
            box.items.add (item);
        }

        box.performLayout (area.toFloat());
    };

    auto sliderRow = headerArea.removeFromTop (controlHeight);
    headerBottom = sliderRow.getBottom();

    if (! stackCombos)
    {
        auto row = sliderRow;

        auto modeBounds = row.removeFromLeft (sliderModeMinWidth);
        sliderModeCombo.setBounds (modeBounds.withHeight (controlHeight));

        row.removeFromLeft (spacing);

        auto viewSectionWidth = juce::jmax (viewComboMinWidth + viewLabelWidth + spacing,
                                            juce::roundToInt (bounds.getWidth() * 0.18f));
        viewSectionWidth = juce::jmin (viewSectionWidth, row.getWidth() / 2);

        auto viewSection = row.removeFromRight (viewSectionWidth);
        auto viewLabelArea = viewSection.removeFromLeft (viewLabelWidth);
        visualizationLabel.setBounds (viewLabelArea.withHeight (controlHeight));
        viewSection.removeFromLeft (spacing);
        visualizationCombo.setBounds (viewSection.withHeight (controlHeight));

        const int sliderWidthMax = juce::roundToInt (juce::jmax (280.0f, 360.0f * scale));
        const int sliderWidth = juce::jlimit (sliderMinWidth, sliderWidthMax, row.getWidth());
        auto sliderBounds = row.removeFromLeft (sliderWidth);
        zoomSlider.setBounds (sliderBounds.withHeight (controlHeight));

        headerBottom = std::max (headerBottom, sliderBounds.getBottom());
        if (heatmapDensitySlider.isVisible())
        {
            headerArea.removeFromTop (spacing);
            auto heatmapRow = headerArea.removeFromTop (controlHeight);
            const int heatmapLabelWidth = juce::roundToInt (juce::jmax (95.0f, 110.0f * scale));
            const int heatmapValueWidth = juce::roundToInt (juce::jmax (60.0f, 72.0f * scale));
            const int heatmapSliderWidth = juce::jlimit (juce::roundToInt (140.0f * scale),
                                                         juce::roundToInt (240.0f * scale),
                                                         heatmapRow.getWidth());

            auto valueArea = heatmapRow.removeFromRight (heatmapValueWidth);
            heatmapDensityValueLabel.setBounds (valueArea.withHeight (controlHeight));

            heatmapRow.removeFromRight (spacing);
            auto sliderArea = heatmapRow.removeFromRight (heatmapSliderWidth);
            heatmapDensitySlider.setBounds (sliderArea.withHeight (controlHeight));

            heatmapRow.removeFromRight (spacing);
            auto labelArea = heatmapRow.removeFromRight (heatmapLabelWidth);
            heatmapDensityLabel.setBounds (labelArea.withHeight (controlHeight));

            headerBottom = std::max (headerBottom, std::max (valueArea.getBottom(), std::max (sliderArea.getBottom(), labelArea.getBottom())));
        }
        else
        {
            heatmapDensityLabel.setBounds ({});
            heatmapDensitySlider.setBounds ({});
            heatmapDensityValueLabel.setBounds ({});
        }

        headerArea.removeFromTop (spacing);
    }
    else
    {
        sliderModeCombo.setBounds (sliderRow.withHeight (controlHeight));

        headerArea.removeFromTop (spacing);
        auto sliderOnlyRow = headerArea.removeFromTop (controlHeight);
        const int sliderWidthMax = juce::roundToInt (juce::jmax (280.0f, 360.0f * scale));
        const int sliderWidth = juce::jlimit (sliderMinWidth, sliderWidthMax, sliderOnlyRow.getWidth());
        auto sliderBounds = sliderOnlyRow.withSizeKeepingCentre (sliderWidth, controlHeight);
        zoomSlider.setBounds (sliderBounds);
        headerBottom = std::max (headerBottom, sliderBounds.getBottom());

        headerArea.removeFromTop (spacing);
        auto viewRow = headerArea.removeFromTop (controlHeight);
        auto viewLabelArea = viewRow.removeFromLeft (viewLabelWidth);
        visualizationLabel.setBounds (viewLabelArea.withHeight (controlHeight));
        viewRow.removeFromLeft (spacing);
        visualizationCombo.setBounds (viewRow.withHeight (controlHeight));
        headerBottom = std::max (headerBottom, viewRow.getBottom());

        headerArea.removeFromTop (spacing);
        if (heatmapDensitySlider.isVisible())
        {
            auto heatmapRow = headerArea.removeFromTop (controlHeight);
            const int heatmapLabelWidth = juce::roundToInt (juce::jmax (95.0f, 110.0f * scale));
            const int heatmapValueWidth = juce::roundToInt (juce::jmax (60.0f, 72.0f * scale));
            const int heatmapSliderWidth = juce::jlimit (juce::roundToInt (140.0f * scale),
                                                         juce::roundToInt (240.0f * scale),
                                                         heatmapRow.getWidth());

            auto valueArea = heatmapRow.removeFromRight (heatmapValueWidth);
            heatmapDensityValueLabel.setBounds (valueArea.withHeight (controlHeight));

            heatmapRow.removeFromRight (spacing);
            auto sliderArea = heatmapRow.removeFromRight (heatmapSliderWidth);
            heatmapDensitySlider.setBounds (sliderArea.withHeight (controlHeight));

            heatmapRow.removeFromRight (spacing);
            auto labelArea = heatmapRow.removeFromRight (heatmapLabelWidth);
            heatmapDensityLabel.setBounds (labelArea.withHeight (controlHeight));

            headerBottom = std::max (headerBottom, std::max (valueArea.getBottom(), std::max (sliderArea.getBottom(), labelArea.getBottom())));
            addDivider (heatmapRow.getBottom());
            headerArea.removeFromTop (spacing);
        }
        else
        {
            heatmapDensityLabel.setBounds ({});
            heatmapDensitySlider.setBounds ({});
            heatmapDensityValueLabel.setBounds ({});
        }
    }

    const int weightTitleHeight = juce::roundToInt (juce::jmax (18.0f, 22.0f * scale));
    auto weightTitleRow = headerArea.removeFromTop (weightTitleHeight);
    bandWeightTitleLabel.setBounds (weightTitleRow.withHeight (weightTitleHeight));
    headerBottom = std::max (headerBottom, weightTitleRow.getBottom());
    addDivider (weightTitleRow.getBottom());

    headerArea.removeFromTop (juce::roundToInt (3.0f * scale));

    juce::Rectangle<int> legendReference;

    const int weightRowHeight = juce::roundToInt (juce::jmax ((float) controlHeight, 28.0f * scale));
    auto weightRow = headerArea.removeFromTop (weightRowHeight);

    auto weightArea = weightRow;
    const int padButtonWidth = juce::roundToInt (juce::jmax (130.0f, 150.0f * scale));
    auto buttonArea = weightArea.removeFromRight (padButtonWidth);
    colourPadButton.setBounds (buttonArea.withSizeKeepingCentre (padButtonWidth, controlHeight));
    weightArea.removeFromRight (juce::roundToInt (6.0f * scale));

    auto sliderArea = weightArea;
    legendReference = sliderArea;

    auto columnSpacing = juce::roundToInt (6.0f * scale);
    auto usableWidth = sliderArea.getWidth() - columnSpacing * 2;
    auto columnWidth = juce::jmax (juce::roundToInt (180.0f * scale), usableWidth / 3);

    auto layoutWeightColumn = [controlHeight] (juce::Rectangle<int> area, juce::Label& label, juce::Slider& slider)
    {
        const int labelWidth = juce::jmin (juce::roundToInt (70.0f), area.getWidth() / 3);
        auto labelArea = area.removeFromLeft (labelWidth);
        label.setBounds (labelArea.withHeight (controlHeight));
        slider.setBounds (area.withHeight (controlHeight));
    };

    auto columnArea = sliderArea;
    auto firstCol = columnArea.removeFromLeft (columnWidth);
    layoutWeightColumn (firstCol, lowWeightLabel, lowWeightSlider);
    columnArea.removeFromLeft (columnSpacing);
    auto secondCol = columnArea.removeFromLeft (columnWidth);
    layoutWeightColumn (secondCol, midWeightLabel, midWeightSlider);
    columnArea.removeFromLeft (columnSpacing);
    layoutWeightColumn (columnArea, highWeightLabel, highWeightSlider);

    headerBottom = std::max (headerBottom, weightRow.getBottom());
    addDivider (weightRow.getBottom());

    headerArea.removeFromTop (spacing);

    auto outsideRowArea = headerArea.removeFromTop (controlHeight);
    layoutButtonRow (outsideRowArea, outsideButtonIndices, outsideLabel);
    headerBottom = std::max (headerBottom, outsideRowArea.getBottom());
    addDivider (outsideRowArea.getBottom());

    headerArea.removeFromTop (spacing);

    auto insideRowArea = headerArea.removeFromTop (controlHeight);
    layoutButtonRow (insideRowArea, insideButtonIndices, insideLabel);
    headerBottom = std::max (headerBottom, insideRowArea.getBottom());
    addDivider (insideRowArea.getBottom());

    headerArea.removeFromTop (spacing);

    const int legendRowHeight = juce::roundToInt (juce::jmax ((float) controlHeight, 34.0f * scale));
    auto legendRow = headerArea.removeFromTop (legendRowHeight);

    if (legendReference.isEmpty())
        legendReference = legendRow;

    const int legendMinWidth = juce::roundToInt (juce::jmax (160.0f, 180.0f * scale));
    const int legendMaxWidth = juce::roundToInt (juce::jmax (220.0f, 280.0f * scale));
    const int legendWidth = juce::jlimit (legendMinWidth, legendMaxWidth, legendReference.getWidth());
    const int legendRight = juce::jmin (legendReference.getRight(), legendRow.getRight());
    const int legendLeft = juce::jmax (legendRow.getX(), legendRight - legendWidth);
    juce::Rectangle<int> legendArea (legendLeft, legendRow.getY(), legendRight - legendLeft, legendRowHeight);
    colourLegend.setBounds (legendArea.reduced (juce::roundToInt (4.0f * scale), juce::roundToInt (2.0f * scale)));
    headerBottom = std::max (headerBottom, legendArea.getBottom());
    addDivider (legendArea.getBottom());

    headerArea.removeFromTop (spacing);
    const int textBoxWidth = juce::roundToInt (juce::jlimit (56.0f, 86.0f, 68.0f * scale));
    const int textBoxHeight = juce::roundToInt (juce::jlimit (18.0f, 24.0f, 20.0f * scale + 4.0f));
    zoomSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, textBoxWidth, textBoxHeight);

    auto viewerBounds = bounds.withTrimmedTop (headerBottom + marginY)
                               .reduced (juce::roundToInt (16.0f * scale), juce::roundToInt (10.0f * scale));
    visualizer->setBounds (viewerBounds);
}

























