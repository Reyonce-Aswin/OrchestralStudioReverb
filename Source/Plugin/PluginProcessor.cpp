#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../DSP/StagePosition.h"

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

    // Keep this order identical to OrchestraLayoutComponent's 19 sectors.
    const StringArray stageNames {
        "Piano", "Violins I", "Violins II", "Violas", "Celli", "Basses",
        "Piccolos", "Flutes", "Oboes", "Clarinets", "Bassoons", "Horns",
        "Harp / Celeste", "Percussion", "Tuned Percussion",
        "Trumpets", "Tenor Trombones", "Bass Trombones", "Tuba"
    };
    const StringArray cutSlopeChoices { "12 dB/oct", "24 dB/oct", "36 dB/oct", "48 dB/oct" };
    const StringArray lowTypeChoices { "Low Cut", "Low Shelf" };
    const StringArray highTypeChoices { "High Shelf", "High Cut" };
    const float eqFrequencies[8] { 30.0f, 120.0f, 300.0f, 700.0f, 1500.0f, 3500.0f, 7000.0f, 16000.0f };
    for (int band = 0; band < 8; ++band)
    {
        const auto prefix = "eqBand" + String(band + 1);
        layout.add(std::make_unique<AudioParameterFloat>(ParameterID { prefix + "Freq", 1 },
            "EQ " + String(band + 1) + " Frequency",
            NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.25f), eqFrequencies[band], "Hz"));
        layout.add(std::make_unique<AudioParameterFloat>(ParameterID { prefix + "Gain", 1 },
            "EQ " + String(band + 1) + " Gain",
            NormalisableRange<float>(-18.0f, 18.0f, 0.1f), 0.0f, "dB"));
        layout.add(std::make_unique<AudioParameterFloat>(ParameterID { prefix + "Q", 1 },
            "EQ " + String(band + 1) + " Q",
            NormalisableRange<float>(0.3f, 8.0f, 0.01f, 0.5f),
            (band == 0 || band == 7) ? 1.0f : 0.7071f));
        layout.add(std::make_unique<AudioParameterChoice>(ParameterID { prefix + "Slope", 1 },
            "EQ " + String(band + 1) + " Slope", cutSlopeChoices, 1));
        if (band == 0)
            layout.add(std::make_unique<AudioParameterChoice>(ParameterID { prefix + "Type", 1 },
                "EQ Low Band Type", lowTypeChoices, 0));
        else if (band == 7)
            layout.add(std::make_unique<AudioParameterChoice>(ParameterID { prefix + "Type", 1 },
                "EQ High Band Type", highTypeChoices, 1));
        else
            layout.add(std::make_unique<AudioParameterChoice>(ParameterID { prefix + "Type", 1 },
                "EQ " + String(band + 1) + " Type", StringArray { "Bell" }, 0));
        layout.add(std::make_unique<AudioParameterBool>(ParameterID { prefix + "Enabled", 1 },
            "EQ " + String(band + 1) + " Enabled", false));
    }

    layout.add(std::make_unique<AudioParameterBool>(ParameterID { "eqBypass", 1 },
        "EQ Bypass", false));

    layout.add(std::make_unique<AudioParameterChoice>(
        ParameterID { "stagePosition", 1 }, "Stage Position", stageNames, 3));
    layout.add(std::make_unique<AudioParameterChoice>(
        ParameterID { "stageLayout", 1 }, "Stage Layout",
        StringArray { "Full Modern", "Full Classical", "Chamber Modern", "Chamber Classical" }, 0));
    return layout;
}

void OrchestralStudioReverbAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    reverb.prepare(sampleRate, static_cast<std::size_t>(samplesPerBlock));
    earlyReflections.prepare(sampleRate);
    parametricEQ.prepare(sampleRate);
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
    parametricEQ.reset();
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

    const int stageIndex = juce::jlimit(0, 18, juce::roundToInt(
        parameters.getRawParameterValue("stagePosition")->load()));
    const int layoutIndex = juce::jlimit(0, 3, juce::roundToInt(
        parameters.getRawParameterValue("stageLayout")->load()));
    const auto stage = orchestral::dsp::getStagePosition(
        static_cast<orchestral::dsp::StageLayout>(layoutIndex), stageIndex);

    orchestral::dsp::FDNReverb::Parameters lateParameters;
    lateParameters.decaySeconds = parameters.getRawParameterValue("decay")->load();
    lateParameters.dampingHz = parameters.getRawParameterValue("damping")->load();
    lateParameters.inputGain = 0.45f;
    lateParameters.wetGain = 1.0f;
    lateParameters.stagePosition = stageIndex;
    lateParameters.stagePan = stage.pan;
    lateParameters.stageDepth = stage.depth;
    lateParameters.useExplicitStagePosition = true;
    reverb.setParameters(lateParameters);

    orchestral::dsp::EarlyReflections::Parameters earlyParameters;
    earlyParameters.roomSize = parameters.getRawParameterValue("roomSize")->load();
    earlyParameters.preDelayMs = parameters.getRawParameterValue("preDelay")->load();
    earlyParameters.stagePosition = stageIndex;
    earlyParameters.stagePan = stage.pan;
    earlyParameters.stageDepth = stage.depth;
    earlyParameters.stageLevel = stage.level;
    earlyParameters.useExplicitStagePosition = true;
    earlyReflections.setParameters(earlyParameters);

    const bool eqBypassed = parameters.getRawParameterValue("eqBypass")->load() > 0.5f;
    if (!eqBypassed)
    {
        for (int band = 0; band < 8; ++band)
        {
            const auto prefix = "eqBand" + juce::String(band + 1);
            orchestral::dsp::ParametricEQ::Band eqBand;
            const int typeChoice = juce::roundToInt(parameters.getRawParameterValue(prefix + "Type")->load());
            if (band == 0)
                eqBand.type = typeChoice == 0 ? orchestral::dsp::ParametricEQ::Type::LowCut
                                              : orchestral::dsp::ParametricEQ::Type::LowShelf;
            else if (band == 7)
                eqBand.type = typeChoice == 0 ? orchestral::dsp::ParametricEQ::Type::HighShelf
                                              : orchestral::dsp::ParametricEQ::Type::HighCut;
            else
                eqBand.type = orchestral::dsp::ParametricEQ::Type::Bell;
            eqBand.frequency = parameters.getRawParameterValue(prefix + "Freq")->load();
            eqBand.gainDb = parameters.getRawParameterValue(prefix + "Gain")->load();
            eqBand.q = parameters.getRawParameterValue(prefix + "Q")->load();
            eqBand.slope = juce::roundToInt(parameters.getRawParameterValue(prefix + "Slope")->load());
            eqBand.enabled = parameters.getRawParameterValue(prefix + "Enabled")->load() > 0.5f;
            parametricEQ.setBand(band, eqBand);
        }
    }
    else
    {
        for (int band = 0; band < 8; ++band)
        {
            orchestral::dsp::ParametricEQ::Band bypassBand;
            bypassBand.enabled = false;
            parametricEQ.setBand(band, bypassBand);
        }
    }

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
        float wetL = lateGain * lateL + earlyGain * earlyL;
        float wetR = lateGain * lateR + earlyGain * earlyR;
        parametricEQ.processSample(wetL, wetR);
        left[i] = dryGain * dryL + wetL;
        if (right != nullptr)
            right[i] = dryGain * dryR + wetR;
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
