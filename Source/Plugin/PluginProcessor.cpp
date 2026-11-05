#include "PluginProcessor.h"
#include "PluginEditor.h"

OrchestralStudioReverbAudioProcessor::OrchestralStudioReverbAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "OSR_STATE", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout
OrchestralStudioReverbAudioProcessor::createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID { "decay", 1 }, "Decay",
        NormalisableRange<float>(0.15f, 20.0f, 0.01f, 0.35f), 2.2f, "s"));
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID { "damping", 1 }, "Damping",
        NormalisableRange<float>(500.0f, 18000.0f, 1.0f, 0.4f), 9000.0f, "Hz"));
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID { "roomSize", 1 }, "Room Size",
        NormalisableRange<float>(0.5f, 2.0f, 0.001f, 0.6f), 1.0f, "x"));
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID { "preDelay", 1 }, "Pre-delay",
        NormalisableRange<float>(0.0f, 100.0f, 0.1f), 12.0f, "ms"));
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID { "early", 1 }, "Early Level",
        NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.45f));
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID { "dry", 1 }, "Dry",
        NormalisableRange<float>(0.0f, 1.0f, 0.001f), 1.0f));
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID { "wet", 1 }, "Wet",
        NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.35f));

    const StringArray stageNames {
        "Piano", "Violin I", "Violin II", "Violas", "Celli", "Basses",
        "Piccolos", "Flutes", "Oboes", "Clarinets", "Bassoons",
        "Harp & Celeste", "Timpani & Untuned Percussion", "Tuned Percussion",
        "Horns", "Trumpets", "Tenor Trombones", "Bass Trombones", "Tuba"
    };
    layout.add(std::make_unique<AudioParameterChoice>(
        ParameterID { "stagePosition", 1 }, "Stage Position", stageNames, 1));
    return layout;
}

void OrchestralStudioReverbAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    reverb.prepare(sampleRate, static_cast<std::size_t>(samplesPerBlock));
    earlyReflections.prepare(sampleRate);
    drySmoother.reset(sampleRate, 0.03);
    wetSmoother.reset(sampleRate, 0.03);
    earlySmoother.reset(sampleRate, 0.03);
    drySmoother.setCurrentAndTargetValue(parameters.getRawParameterValue("dry")->load());
    wetSmoother.setCurrentAndTargetValue(parameters.getRawParameterValue("wet")->load());
    earlySmoother.setCurrentAndTargetValue(parameters.getRawParameterValue("early")->load());
}

void OrchestralStudioReverbAudioProcessor::releaseResources()
{
    reverb.reset();
    earlyReflections.reset();
}

bool OrchestralStudioReverbAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& input = layouts.getMainInputChannelSet();
    const auto& output = layouts.getMainOutputChannelSet();
    return input == output && (output == juce::AudioChannelSet::mono()
                               || output == juce::AudioChannelSet::stereo());
}

void OrchestralStudioReverbAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                                         juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = buffer.getNumChannels();
    const int samples = buffer.getNumSamples();
    if (channels == 0)
        return;

    const int stageIndex = juce::roundToInt(
        parameters.getRawParameterValue("stagePosition")->load());

    orchestral::dsp::FDNReverb::Parameters lateParameters;
    lateParameters.decaySeconds = parameters.getRawParameterValue("decay")->load();
    lateParameters.dampingHz = parameters.getRawParameterValue("damping")->load();
    lateParameters.inputGain = 0.45f;
    lateParameters.wetGain = 1.0f;
    lateParameters.stagePosition = stageIndex;
    reverb.setParameters(lateParameters);

    orchestral::dsp::EarlyReflections::Parameters earlyParameters;
    earlyParameters.roomSize = parameters.getRawParameterValue("roomSize")->load();
    earlyParameters.preDelayMs = parameters.getRawParameterValue("preDelay")->load();
    earlyParameters.stagePosition = stageIndex;
    earlyReflections.setParameters(earlyParameters);

    drySmoother.setTargetValue(parameters.getRawParameterValue("dry")->load());
    wetSmoother.setTargetValue(parameters.getRawParameterValue("wet")->load());
    earlySmoother.setTargetValue(parameters.getRawParameterValue("early")->load());

    auto* left = buffer.getWritePointer(0);
    auto* right = channels > 1 ? buffer.getWritePointer(1) : nullptr;
    for (int i = 0; i < samples; ++i)
    {
        const float dryL = left[i];
        const float dryR = right != nullptr ? right[i] : dryL;
        float lateL = 0.0f, lateR = 0.0f, earlyL = 0.0f, earlyR = 0.0f;
        reverb.processSample(dryL, dryR, lateL, lateR);
        earlyReflections.processSample(dryL, dryR, earlyL, earlyR);

        const float dryGain = drySmoother.getNextValue();
        const float lateGain = wetSmoother.getNextValue();
        const float earlyGain = earlySmoother.getNextValue();
        left[i] = dryGain * dryL + lateGain * lateL + earlyGain * earlyL;
        if (right != nullptr)
            right[i] = dryGain * dryR + lateGain * lateR + earlyGain * earlyR;
    }
}

juce::AudioProcessorEditor* OrchestralStudioReverbAudioProcessor::createEditor()
{
    return new OrchestralStudioReverbAudioProcessorEditor(*this);
}

void OrchestralStudioReverbAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OrchestralStudioReverbAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OrchestralStudioReverbAudioProcessor();
}
