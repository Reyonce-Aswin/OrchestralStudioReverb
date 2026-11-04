#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "FDNReverb.h"

class OrchestralStudioReverbAudioProcessor final : public juce::AudioProcessor
{
public:
    OrchestralStudioReverbAudioProcessor();
    ~OrchestralStudioReverbAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 20.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState& getParameters() noexcept { return parameters; }

private:
    orchestral::dsp::FDNReverb reverb;
    juce::AudioProcessorValueTreeState parameters;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> wetSmoother;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrchestralStudioReverbAudioProcessor)
};
