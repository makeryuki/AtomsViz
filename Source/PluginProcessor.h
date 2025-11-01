#pragma once

#include <JuceHeader.h>
#include <vector>

class AtmosVizAudioProcessor : public juce::AudioProcessor
{
public:
    static constexpr int fftOrder = 9;
    static constexpr int fftSize = 1 << fftOrder;

    struct FrequencyBands
    {
        float low{ 0.0f };
        float mid{ 0.0f };
        float high{ 0.0f };
    };

    struct SpeakerDefinition
    {
        juce::String id;
        juce::String displayName;
        float azimuthDegrees;
        float elevationDegrees;
        float radius;
        bool isLfe;
        juce::Vector3D<float> position;
        juce::Vector3D<float> aimDirection;
        juce::AudioChannelSet::ChannelType channelType{};
    };

    struct SpeakerMetrics
    {
        float rms{ 0.0f };
        float peak{ 0.0f };
        FrequencyBands bands{};
    };

    struct RoomDimensions
    {
        float width;
        float height;
        float depth;
        float earHeight;
    };

    using SpeakerDefinitions = std::vector<SpeakerDefinition>;
    using SpeakerMetricsArray = std::vector<SpeakerMetrics>;

    AtmosVizAudioProcessor();
    ~AtmosVizAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    const SpeakerDefinitions& getSpeakerDefinitions() const noexcept;
    void copyLatestMetrics(SpeakerMetricsArray& dest) const noexcept;
    const RoomDimensions& getRoomDimensions() const noexcept;

private:
    void updateBandSplit();
    float computeRms(const float* data, int numSamples) const noexcept;
    float computePeak(const float* data, int numSamples) const noexcept;
    FrequencyBands analyseFrequencyContent(const float* data, int numSamples) noexcept;
    SpeakerDefinitions buildSpeakerDefinitions (const juce::AudioChannelSet& layout) const;
    void rebuildSpeakerLayout();
    int findDefinitionIndexForChannel(juce::AudioChannelSet::ChannelType type) const noexcept;

    static const RoomDimensions defaultRoom;

    SpeakerDefinitions speakerDefinitions;
    SpeakerMetricsArray latestMetrics;
    mutable juce::SpinLock metricsLock;

    juce::HeapBlock<float> fftBuffer;
    juce::dsp::FFT fft{ fftOrder };
    juce::dsp::WindowingFunction<float> window{ fftSize, juce::dsp::WindowingFunction<float>::hann };

    double currentSampleRate = 48000.0;
    int lowBandLimit = 8;
    int midBandLimit = fftSize / 4;

    RoomDimensions roomDimensions = defaultRoom;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AtmosVizAudioProcessor)
};
