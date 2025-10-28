#include <cmath>
#include <algorithm>

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr float degToRad(float degrees) noexcept
    {
        return degrees * juce::MathConstants<float>::pi / 180.0f;
    }

    juce::Vector3D<float> unitVectorFromAngles(float azimuthDeg, float elevationDeg) noexcept
    {
        const auto azimuth = degToRad(azimuthDeg);
        const auto elevation = degToRad(elevationDeg);
        const auto cosElevation = std::cos(elevation);

        return {
            cosElevation * std::cos(azimuth),
            std::sin(elevation),
            cosElevation * std::sin(azimuth)
        };
    }

    juce::Vector3D<float> mapUnitToRoom(const juce::Vector3D<float>& unit,
        const AtmosVizAudioProcessor::RoomDimensions& room,
        bool isLfe) noexcept
    {
        if (isLfe)
            return { room.depth * 0.48f, -room.earHeight * 0.85f, 0.0f };

        const auto depthHalf = room.depth * 0.5f;
        const auto widthHalf = room.width * 0.5f;
        const auto ceiling = room.height - room.earHeight;
        const auto floor = room.earHeight;

        const auto x = juce::jlimit(-depthHalf, depthHalf, unit.x * depthHalf);
        const auto z = juce::jlimit(-widthHalf, widthHalf, unit.z * widthHalf);
        const auto y = unit.y >= 0.0f
            ? juce::jlimit(0.0f, ceiling, unit.y * ceiling)
            : juce::jlimit(-floor, 0.0f, unit.y * floor);

        return { x, y, z };
    }
}

const AtmosVizAudioProcessor::RoomDimensions AtmosVizAudioProcessor::defaultRoom{ 6.4f, 3.05f, 7.6f, 1.2f };
const AtmosVizAudioProcessor::SpeakerDefinitions AtmosVizAudioProcessor::speakerDefinitions =
AtmosVizAudioProcessor::buildSpeakerDefinitions();

AtmosVizAudioProcessor::AtmosVizAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
        .withInput("Atmos Input", juce::AudioChannelSet::create7point1point4(), true)
        .withOutput("Atmos Output", juce::AudioChannelSet::create7point1point4(), true))
#endif
{
    fftBuffer.allocate(2 * fftSize, true);
}

AtmosVizAudioProcessor::~AtmosVizAudioProcessor() = default;

const juce::String AtmosVizAudioProcessor::getName() const { return JucePlugin_Name; }
bool AtmosVizAudioProcessor::acceptsMidi() const { return JucePlugin_WantsMidiInput; }
bool AtmosVizAudioProcessor::producesMidi() const { return JucePlugin_ProducesMidiOutput; }
bool AtmosVizAudioProcessor::isMidiEffect() const { return JucePlugin_IsMidiEffect; }
double AtmosVizAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int AtmosVizAudioProcessor::getNumPrograms() { return 1; }
int AtmosVizAudioProcessor::getCurrentProgram() { return 0; }
void AtmosVizAudioProcessor::setCurrentProgram(int) {}
const juce::String AtmosVizAudioProcessor::getProgramName(int) { return {}; }
void AtmosVizAudioProcessor::changeProgramName(int, const juce::String&) {}

void AtmosVizAudioProcessor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate = sampleRate;
    updateBandSplit();
    juce::FloatVectorOperations::clear(fftBuffer.get(), 2 * fftSize);
}

void AtmosVizAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AtmosVizAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    const auto targetSet = juce::AudioChannelSet::create7point1point4();
    return layouts.getMainInputChannelSet() == targetSet
        && layouts.getMainOutputChannelSet() == targetSet;
#endif
}
#endif

void AtmosVizAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = std::min<int>(buffer.getNumChannels(), (int)kNumAtmosSpeakers);

    const auto& layout = getBusesLayout().getMainInputChannelSet();

    SpeakerMetricsArray metrics{};
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const auto channelType = layout.getTypeOfChannel (ch);
        const auto defIndex = findDefinitionIndexForChannel (channelType);
        if (defIndex < 0)
            continue;

        const auto* channelData = buffer.getReadPointer (ch);

        auto& entry = metrics[(size_t) defIndex];
        entry.rms = computeRms (channelData, numSamples);
        entry.peak = computePeak (channelData, numSamples);
        entry.bands = analyseFrequencyContent (channelData, numSamples);

        if (speakerDefinitions[(size_t) defIndex].isLfe)
        {
            entry.bands.mid = 0.0f;
            entry.bands.high = 0.0f;
        }
    }

    const juce::SpinLock::ScopedLockType lock (metricsLock);
    latestMetrics = metrics;
}

bool AtmosVizAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* AtmosVizAudioProcessor::createEditor() {
    return new AtmosVizAudioProcessorEditor
    (*this);
}

void AtmosVizAudioProcessor::getStateInformation(juce::MemoryBlock&) {}
void AtmosVizAudioProcessor::setStateInformation(const void*, int) {}

const AtmosVizAudioProcessor::SpeakerDefinitions& AtmosVizAudioProcessor::getSpeakerDefinitions() const noexcept
{
    return speakerDefinitions;
}

void AtmosVizAudioProcessor::copyLatestMetrics(SpeakerMetricsArray& dest) const noexcept
{
    const juce::SpinLock::ScopedLockType lock(metricsLock);
    dest = latestMetrics;
}

const AtmosVizAudioProcessor::RoomDimensions& AtmosVizAudioProcessor::getRoomDimensions() const noexcept
{
    return roomDimensions;
}

void AtmosVizAudioProcessor::updateBandSplit()
{
    const auto hzPerBin = (float)currentSampleRate / (float)fftSize;
    lowBandLimit = juce::jlimit(1, fftSize / 2, (int)std::ceil(200.0f / hzPerBin));
    midBandLimit = juce::jlimit(lowBandLimit + 1, fftSize / 2, (int)std::ceil(2000.0f / hzPerBin));
}

float AtmosVizAudioProcessor::computeRms(const float* data, int numSamples) const noexcept
{
    if (numSamples <= 0)
        return 0.0f;

    double sumSquares = 0.0;
    for (int i = 0; i < numSamples; ++i)
        sumSquares += (double)data[i] * (double)data[i];

    return (float)std::sqrt(sumSquares / numSamples);
}

float AtmosVizAudioProcessor::computePeak(const float* data, int numSamples) const noexcept
{
    float peak = 0.0f;
    for (int i = 0; i < numSamples; ++i)
        peak = std::max(peak, std::abs(data[i]));
    return peak;
}

AtmosVizAudioProcessor::FrequencyBands AtmosVizAudioProcessor::analyseFrequencyContent(const float* data, int
    numSamples) noexcept
{
    FrequencyBands bands;
    auto* fftData = fftBuffer.get();

    const auto copyCount = std::min(fftSize, numSamples);
    juce::FloatVectorOperations::copy(fftData, data, copyCount);
    if (copyCount < fftSize)
        juce::FloatVectorOperations::clear(fftData + copyCount, fftSize - copyCount);
    juce::FloatVectorOperations::clear(fftData + fftSize, fftSize);

    window.multiplyWithWindowingTable(fftData, fftSize);
    fft.performFrequencyOnlyForwardTransform(fftData);

    const auto nyquist = fftSize / 2;
    for (int bin = 1; bin < nyquist; ++bin)
    {
        const auto magnitude = fftData[bin];

        if (bin < lowBandLimit)      bands.low += magnitude;
        else if (bin < midBandLimit) bands.mid += magnitude;
        else                         bands.high += magnitude;
    }

    const auto lowNorm = std::max(1, lowBandLimit - 1);
    const auto midNorm = std::max(1, midBandLimit - lowBandLimit);
    const auto highNorm = std::max(1, nyquist - midBandLimit);

    bands.low /= (float)lowNorm;
    bands.mid /= (float)midNorm;
    bands.high /= (float)highNorm;

    return bands;
}

AtmosVizAudioProcessor::SpeakerDefinitions AtmosVizAudioProcessor::buildSpeakerDefinitions()
{
    struct Seed { juce::AudioChannelSet::ChannelType type; const char* id; const char* name; float azimuth; float elevation; bool isLfe; };

    static constexpr Seed seeds[kNumAtmosSpeakers] =
    {
        { juce::AudioChannelSet::left,            "L",   "Left",             -30.0f,   0.0f, false },
        { juce::AudioChannelSet::right,           "R",   "Right",             30.0f,   0.0f, false },
        { juce::AudioChannelSet::centre,          "C",   "Centre",             0.0f,   0.0f, false },
        { juce::AudioChannelSet::LFE,             "LFE", "LFE",                0.0f, -30.0f, true  },
        { juce::AudioChannelSet::leftSurround,    "Ls",  "Surround L",      -110.0f,   0.0f, false },
        { juce::AudioChannelSet::rightSurround,   "Rs",  "Surround R",       110.0f,   0.0f, false },
        { juce::AudioChannelSet::leftSurroundRear,  "Lrs", "Rear Surround L", -150.0f,   0.0f, false },
        { juce::AudioChannelSet::rightSurroundRear, "Rrs", "Rear Surround R",  150.0f,   0.0f, false },
        { juce::AudioChannelSet::topFrontLeft,    "Ltf", "Top Front L",      -45.0f,  45.0f, false },
        { juce::AudioChannelSet::topFrontRight,   "Rtf", "Top Front R",       45.0f,  45.0f, false },
        { juce::AudioChannelSet::topRearLeft,     "Ltr", "Top Rear L",      -135.0f,  45.0f, false },
        { juce::AudioChannelSet::topRearRight,    "Rtr", "Top Rear R",       135.0f,  45.0f, false }
    };

    SpeakerDefinitions defs{};

    for (size_t i = 0; i < defs.size(); ++i)
    {
        const auto& seed = seeds[i];
        const auto unit = unitVectorFromAngles(seed.azimuth, seed.elevation);
        const auto position = mapUnitToRoom(unit, defaultRoom, seed.isLfe);

        auto aim = -position;
        const auto aimLength = aim.length();

        if (aimLength > 1.0e-4f) aim /= aimLength;
        else                     aim = { -1.0f, 0.0f, 0.0f };

        defs[i] = SpeakerDefinition
        {
            seed.id,
            seed.name,
            seed.azimuth,
            seed.elevation,
            position.length(),
            seed.isLfe,
            position,
            aim,
            seed.type
        };
    }

    return defs;
}

int AtmosVizAudioProcessor::findDefinitionIndexForChannel(juce::AudioChannelSet::ChannelType type) const noexcept
{
    for (size_t i = 0; i < speakerDefinitions.size(); ++i)
    {
        if (speakerDefinitions[i].channelType == type)
            return static_cast<int>(i);
    }

    return -1;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AtmosVizAudioProcessor();
}
