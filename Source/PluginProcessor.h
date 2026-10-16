#pragma once

#include <JuceHeader.h>
#include "Voice.h"

class LarzAudioProcessor final : public juce::AudioProcessor
{
public:
    inline static constexpr char attackParam[] = "attack";
    inline static constexpr char decayParam[] = "decay";
    inline static constexpr char sustainParam[] = "sustain";
    inline static constexpr char holdParam[] = "hold";
    inline static constexpr char releaseParam[] = "release";
    inline static constexpr char keyTrackParam[] = "keyTrack";
    inline static constexpr char rootParam[] = "root";
    inline static constexpr char phaseParam[] = "phase";
    inline static constexpr char gainParam[] = "gain";
    inline static constexpr char viewParam[] = "view";

    using PitchNodes = std::array<larz::PitchNode, 16>;

    LarzAudioProcessor();
    ~LarzAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destinationData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void getPitchNodes (PitchNodes& destination, int& count) const;
    void setPitchNodes (const PitchNodes& source, int count);

    juce::AudioProcessorValueTreeState parameters;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void loadPitchNodes (const juce::ValueTree& state);

    mutable juce::SpinLock pitchNodesLock;
    PitchNodes pitchNodes {{
        { 0.0f, 400.0f }, { 40.0f, 100.0f }, { 250.0f, 55.0f }, { 1500.0f, 40.0f }
    }};
    int pitchNodeCount = 4;
    PitchNodes audioPitchNodes {};
    int audioPitchNodeCount = 4;

    std::array<larz::Voice, 8> voices {};
    int nextVoice = 0;
    double currentSampleRate = 44100.0;

    std::atomic<float>* attackValue = nullptr;
    std::atomic<float>* decayValue = nullptr;
    std::atomic<float>* sustainValue = nullptr;
    std::atomic<float>* holdValue = nullptr;
    std::atomic<float>* releaseValue = nullptr;
    std::atomic<float>* keyTrackValue = nullptr;
    std::atomic<float>* rootValue = nullptr;
    std::atomic<float>* phaseValue = nullptr;
    std::atomic<float>* gainValue = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LarzAudioProcessor)
};
