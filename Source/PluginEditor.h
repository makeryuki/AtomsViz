#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>
#include <deque>
#include <array>
#include <limits>
#include <utility>
#include <functional>

#include "PluginProcessor.h"

class SpeakerVisualizerComponent final : public juce::Component,
                                         private juce::Timer
{
public:
    enum class VisualizationMode
    {
        DirectionalLobes,
        LayeredLobes,
        DirectivityBalloon,
        RadiationHeatmap,
        TemporalTrail
    };

    enum class CameraPreset
    {
        OutsideHome,
        OutsideUser,
        OutsideFront,
        OutsideBack,
        OutsideLeft,
        OutsideRight,
        OutsideTop,
        InsideHome,
        InsideUser,
        InsideFront,
        InsideBack,
        InsideLeft,
        InsideRight,
        InsideTop
    };

    struct CameraParameters
    {
        float yawDegrees;
        float pitchDegrees;
        float baseDistance;
        bool  inside;
    };

    struct CameraState
    {
        float yaw{ 0.0f };
        float pitch{ 0.0f };
        float baseDistance{ 1.0f };
        float zoom{ 1.0f };
    };

    explicit SpeakerVisualizerComponent (AtmosVizAudioProcessor&);
    ~SpeakerVisualizerComponent() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    void setCameraPreset (CameraPreset preset);
    CameraPreset getCameraPreset() const noexcept;

    void setZoomFactor (float newFactor, bool fromPreset = false);
    float getZoomFactor() const noexcept { return zoomFactor; }
    float getMinZoomFactor() const noexcept { return minZoomFactor; }
    float getMaxZoomFactor() const noexcept { return maxZoomFactor; }

    void setVisualizationMode (VisualizationMode mode);
    VisualizationMode getVisualizationMode() const noexcept { return visualizationMode; }

    void setVisualizationScaleAdjustment (float value);
    float getVisualizationScaleAdjustment() const noexcept { return visualizationScaleSliderValue; }

    std::function<void (float)> onZoomFactorChanged;
    std::function<void (CameraPreset)> onPresetChanged;
    std::function<void (VisualizationMode)> onVisualizationModeChanged;
    std::function<void (float)> onVisualizationScaleChanged;

private:
    struct DisplaySpeaker
    {
        AtmosVizAudioProcessor::SpeakerDefinition definition;
        AtmosVizAudioProcessor::SpeakerMetrics metrics;
        juce::Point<float> projected;
        juce::Point<float> orientation2D;
        float depth = 0.0f;
        float maxReachScreen = 120.0f;
        float maxReachWorld = 1.0f;
        std::deque<juce::Point<float>> trail;
    };

    struct ProjectedPoint
    {
        juce::Point<float> screen;
        float depth = 0.0f;
        float maxReachScreen = 120.0f;
        float maxReachWorld = 1.0f;
        std::deque<juce::Point<float>> trail;
    };

    void timerCallback() override;
    void refreshMetrics();
    void updateProjections();
    void applyOrbit (DisplaySpeaker& speaker) const;
    void drawRoom (juce::Graphics& g);
    void drawGizmo (juce::Graphics& g);
    void updateRoomProjection();
    void updateProjectionScale();
    ProjectedPoint projectPoint (const juce::Vector3D<float>& point) const;

    using DrawOrder = std::vector<const DisplaySpeaker*>;
    void drawDirectionalLobes (juce::Graphics& g, const DrawOrder& order);
    void drawLayeredLobes (juce::Graphics& g, const DrawOrder& order);
    void drawDirectivityBalloons (juce::Graphics& g, const DrawOrder& order);
    void drawRadiationHeatmap (juce::Graphics& g);
    void drawTemporalTrails (juce::Graphics& g, const DrawOrder& order);
    void drawSpeakerBaseMarkers (juce::Graphics& g, const DrawOrder& order);
    void updateHeatmapCache();
    void updateTrails();
    void updateMaxReach (DisplaySpeaker& speaker) const;
    float distanceToRoomBoundary (const juce::Vector3D<float>& position, const juce::Vector3D<float>& direction) const;
    float reachForLevel (const DisplaySpeaker& speaker, float level, float shaping = 0.65f) const;
    float visualLevelForSpeaker (const DisplaySpeaker& speaker) const;
    juce::Colour colourForLevel (float level, float maxLevel) const;
    juce::AffineTransform rotationTransform (juce::Point<float> centre, juce::Point<float> direction, float width, float height) const;

    juce::Colour colourForBands (const AtmosVizAudioProcessor::FrequencyBands& bands, bool isLfe) const;

    void applyZoomFactorToCamera();
    void promoteToUserPreset();
    void captureUserState();
    void restoreUserState (bool inside);
    CameraState& getUserState (bool inside) noexcept;
    const CameraState& getUserState (bool inside) const noexcept;
    static bool isInsidePreset (CameraPreset preset) noexcept;
    static bool isUserPreset (CameraPreset preset) noexcept;
    static bool isHomePreset (CameraPreset preset) noexcept;
    const CameraParameters* findPresetDefinition (CameraPreset preset) const noexcept;
    void notifyPresetChanged();

    static const std::array<std::pair<int, int>, 12> roomEdges;

    AtmosVizAudioProcessor& processor;
    AtmosVizAudioProcessor::RoomDimensions roomDimensions;
    std::vector<DisplaySpeaker> speakers;

    std::array<juce::Vector3D<float>, 8> roomVerticesModel {};
    std::array<ProjectedPoint, 8> roomVerticesProjected {};

    CameraPreset currentPreset { CameraPreset::OutsideHome };
    float yaw   = 0.0f;
    float pitch = 0.0f;
    bool cameraInside = false;

    VisualizationMode visualizationMode { VisualizationMode::DirectionalLobes };

    float cameraDistance = 6.0f;
    float cameraBaseDistance = 6.0f;
    float baseProjectionScale = 1.0f;
    float projectionScale = 1.0f;

    const float minZoomFactor = 0.1f;
    const float maxZoomFactor = 2.0f;
    float zoomFactor          = 1.0f;

    const float gizmoAxisLength = 1.8f;

    CameraState outsideUserState{};
    CameraState insideUserState{};

    static constexpr size_t trailHistoryLength = 32;
    std::vector<juce::Vector3D<float>> heatmapPoints;
    float cachedHeatmapMaxLevel = 0.0f;
    float visualizationScale = 1.0f;
    float visualizationScaleSliderValue = 0.0f;

    juce::Point<float> dragAnchor;
    float yawAnchor = 0.0f;
    float pitchAnchor = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpeakerVisualizerComponent)
};

class AtmosVizAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                      private juce::Slider::Listener
{
public:
    explicit AtmosVizAudioProcessorEditor (AtmosVizAudioProcessor&);
    ~AtmosVizAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void createCameraButtons();
    void setupZoomSlider();
    void setupSliderModeSelector();
    void setupVisualizationSelector();
    void setCameraPreset (SpeakerVisualizerComponent::CameraPreset preset);
    void updateCameraButtonStates();
    void updateVisualizationSelector();
    void updateSliderConfiguration();

    void sliderValueChanged (juce::Slider* slider) override;

    AtmosVizAudioProcessor& audioProcessor;
    std::unique_ptr<SpeakerVisualizerComponent> visualizer;

    enum class SliderControlMode { Zoom, DrawScale };
    SliderControlMode sliderControlMode { SliderControlMode::Zoom };

    struct CameraButtonInfo
    {
        SpeakerVisualizerComponent::CameraPreset preset;
        std::unique_ptr<juce::TextButton> button;
        int row = 0;
    };

    std::vector<CameraButtonInfo> cameraButtons;
    std::vector<size_t> outsideButtonIndices;
    std::vector<size_t> insideButtonIndices;
    juce::Slider zoomSlider;
    juce::ComboBox sliderModeCombo;
    juce::ComboBox visualizationCombo;
    juce::Label  visualizationLabel;
    juce::Label  outsideLabel;
    juce::Label  insideLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AtmosVizAudioProcessorEditor)
};
