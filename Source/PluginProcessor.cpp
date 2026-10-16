#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

LarzAudioProcessor::LarzAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "LARZ_PARAMETERS", createParameterLayout())
{
    attackValue = parameters.getRawParameterValue (attackParam);
    decayValue = parameters.getRawParameterValue (decayParam);
    sustainValue = parameters.getRawParameterValue (sustainParam);
    holdValue = parameters.getRawParameterValue (holdParam);
    releaseValue = parameters.getRawParameterValue (releaseParam);
    keyTrackValue = parameters.getRawParameterValue (keyTrackParam);
    rootValue = parameters.getRawParameterValue (rootParam);
    phaseValue = parameters.getRawParameterValue (phaseParam);
    gainValue = parameters.getRawParameterValue (gainParam);
}

juce::AudioProcessorValueTreeState::ParameterLayout LarzAudioProcessor::createParameterLayout()
{
    using FloatParameter = juce::AudioParameterFloat;
    using IntParameter = juce::AudioParameterInt;
    using BoolParameter = juce::AudioParameterBool;
    using ID = juce::ParameterID;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<FloatParameter> (ID { attackParam, 1 }, "Attack",
                                                   juce::NormalisableRange<float> (0.0f, 500.0f, 1.0f), 10.0f));
    layout.add (std::make_unique<FloatParameter> (ID { decayParam, 1 }, "Decay",
                                                   juce::NormalisableRange<float> (1.0f, 4000.0f, 1.0f), 400.0f));
    layout.add (std::make_unique<FloatParameter> (ID { sustainParam, 1 }, "Sustain",
                                                   juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));
    layout.add (std::make_unique<FloatParameter> (ID { holdParam, 1 }, "Hold",
                                                   juce::NormalisableRange<float> (0.0f, 4000.0f, 1.0f), 500.0f));
    layout.add (std::make_unique<FloatParameter> (ID { releaseParam, 1 }, "Release",
                                                   juce::NormalisableRange<float> (1.0f, 4000.0f, 1.0f), 600.0f));
    layout.add (std::make_unique<BoolParameter> (ID { keyTrackParam, 1 }, "Key Track", false));
    layout.add (std::make_unique<IntParameter> (ID { rootParam, 1 }, "Root", 0, 127, 36));
    layout.add (std::make_unique<FloatParameter> (ID { phaseParam, 1 }, "Start Phase",
                                                   juce::NormalisableRange<float> (0.0f, 360.0f, 1.0f), 0.0f));
    layout.add (std::make_unique<FloatParameter> (ID { gainParam, 1 }, "Gain",
                                                   juce::NormalisableRange<float> (-48.0f, 12.0f, 0.1f), -6.0f));
    layout.add (std::make_unique<FloatParameter> (ID { viewParam, 1 }, "View",
                                                   juce::NormalisableRange<float> (500.0f, 8000.0f, 10.0f), 2000.0f));
    return layout;
}

void LarzAudioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;
    for (auto& voice : voices)
        voice.active = false;
}

void LarzAudioProcessor::releaseResources()
{
}

bool LarzAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.inputBuses.isEmpty()
        && layouts.outputBuses.size() == 1
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void LarzAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    {
        juce::SpinLock::ScopedTryLockType lock (pitchNodesLock);
        if (lock.isLocked())
        {
            audioPitchNodes = pitchNodes;
            audioPitchNodeCount = pitchNodeCount;
        }
    }

    const larz::EnvelopeParameters envelope {
        attackValue->load (std::memory_order_relaxed),
        decayValue->load (std::memory_order_relaxed),
        sustainValue->load (std::memory_order_relaxed),
        holdValue->load (std::memory_order_relaxed),
        releaseValue->load (std::memory_order_relaxed)
    };
    const auto keyTrack = keyTrackValue->load (std::memory_order_relaxed) >= 0.5f;
    const auto root = rootValue->load (std::memory_order_relaxed);
    const auto startPhase = phaseValue->load (std::memory_order_relaxed) * (3.14159265358979323846 / 180.0);
    const auto gain = std::pow (10.0, gainValue->load (std::memory_order_relaxed) / 20.0);

    buffer.clear();
    auto* left = buffer.getWritePointer (0);
    auto* right = buffer.getWritePointer (1);

    auto midiIterator = midiMessages.begin();
    const auto midiEnd = midiMessages.end();

    for (auto sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
    {
        while (midiIterator != midiEnd && (*midiIterator).samplePosition <= sampleIndex)
        {
            const auto metadata = *midiIterator++;
            if (metadata.numBytes >= 3 && (metadata.data[0] & 0xf0) == 0x90 && metadata.data[2] != 0)
            {
                auto& voice = voices[static_cast<size_t> (nextVoice)];
                nextVoice = (nextVoice + 1) % static_cast<int> (voices.size());
                const auto pitchMultiplier = keyTrack
                    ? std::pow (2.0f, (static_cast<float> (metadata.data[1] & 0x7f) - root) / 12.0f)
                    : 1.0f;
                voice.start (startPhase, pitchMultiplier, static_cast<float> (metadata.data[2]) / 127.0f);
            }
        }

        auto output = 0.0f;
        for (auto& voice : voices)
            output += voice.render (currentSampleRate, audioPitchNodes.data(), audioPitchNodeCount, envelope);

        output *= static_cast<float> (gain);
        left[sampleIndex] = output;
        right[sampleIndex] = output;
    }

    midiMessages.clear();
}

bool LarzAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* LarzAudioProcessor::createEditor()
{
    return new LarzAudioProcessorEditor (*this);
}

const juce::String LarzAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool LarzAudioProcessor::acceptsMidi() const
{
    return true;
}

bool LarzAudioProcessor::producesMidi() const
{
    return false;
}

bool LarzAudioProcessor::isMidiEffect() const
{
    return false;
}

double LarzAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int LarzAudioProcessor::getNumPrograms()
{
    return 1;
}

int LarzAudioProcessor::getCurrentProgram()
{
    return 0;
}

void LarzAudioProcessor::setCurrentProgram (int)
{
}

const juce::String LarzAudioProcessor::getProgramName (int)
{
    return {};
}

void LarzAudioProcessor::changeProgramName (int, const juce::String&)
{
}

void LarzAudioProcessor::getStateInformation (juce::MemoryBlock& destinationData)
{
    auto state = parameters.copyState();
    juce::ValueTree nodeState ("PITCH_NODES");

    PitchNodes nodes;
    int count = 0;
    getPitchNodes (nodes, count);
    for (auto i = 0; i < count; ++i)
    {
        juce::ValueTree node ("NODE");
        node.setProperty ("timeMs", nodes[static_cast<size_t> (i)].timeMs, nullptr);
        node.setProperty ("frequencyHz", nodes[static_cast<size_t> (i)].frequencyHz, nullptr);
        nodeState.addChild (node, -1, nullptr);
    }

    state.addChild (nodeState, -1, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destinationData);
}

void LarzAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr)
    {
        juce::Logger::writeToLog ("LARZ ignored invalid state data.");
        return;
    }

    auto state = juce::ValueTree::fromXml (*xml);
    if (! state.isValid() || state.getType() != parameters.state.getType())
    {
        juce::Logger::writeToLog ("LARZ ignored incompatible state data.");
        return;
    }

    loadPitchNodes (state);
    state.removeChild (state.getChildWithName ("PITCH_NODES"), nullptr);
    parameters.replaceState (state);
}

void LarzAudioProcessor::getPitchNodes (PitchNodes& destination, int& count) const
{
    const juce::SpinLock::ScopedLockType lock (pitchNodesLock);
    destination = pitchNodes;
    count = pitchNodeCount;
}

void LarzAudioProcessor::setPitchNodes (const PitchNodes& source, int count)
{
    count = std::clamp (count, 2, static_cast<int> (pitchNodes.size()));
    PitchNodes validated {};
    const auto firstFrequency = std::isfinite (source[0].frequencyHz) ? source[0].frequencyHz : 20.0f;
    validated[0] = { 0.0f, std::clamp (firstFrequency, 20.0f, 2000.0f) };

    for (auto i = 1; i < count; ++i)
    {
        const auto index = static_cast<size_t> (i);
        const auto previousTime = validated[index - 1].timeMs;
        auto time = std::isfinite (source[index].timeMs) ? source[index].timeMs : previousTime + 1.0f;
        auto frequency = std::isfinite (source[index].frequencyHz) ? source[index].frequencyHz : 20.0f;
        const auto latestTime = 8000.0f - static_cast<float> (count - i - 1);
        time = std::clamp (time, previousTime + 1.0f, latestTime);
        frequency = std::clamp (frequency, 20.0f, 2000.0f);
        validated[index] = { time, frequency };
    }

    const juce::SpinLock::ScopedLockType lock (pitchNodesLock);
    pitchNodes = validated;
    pitchNodeCount = count;
}

void LarzAudioProcessor::loadPitchNodes (const juce::ValueTree& state)
{
    const auto nodeState = state.getChildWithName ("PITCH_NODES");
    if (! nodeState.isValid() || nodeState.getNumChildren() < 2)
        return;

    PitchNodes nodes {};
    const auto count = std::min (nodeState.getNumChildren(), static_cast<int> (nodes.size()));
    for (auto i = 0; i < count; ++i)
    {
        const auto node = nodeState.getChild (i);
        nodes[static_cast<size_t> (i)] = {
            static_cast<float> (node.getProperty ("timeMs", 0.0f)),
            static_cast<float> (node.getProperty ("frequencyHz", 20.0f))
        };
    }

    setPitchNodes (nodes, count);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LarzAudioProcessor();
}
