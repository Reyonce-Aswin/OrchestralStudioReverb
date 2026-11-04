#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

class OrchestralStudioReverbAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit OrchestralStudioReverbAudioProcessorEditor(OrchestralStudioReverbAudioProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    OrchestralStudioReverbAudioProcessor& processor;
    juce::Slider decay, damping, wet;
    juce::Label title, decayLabel, dampingLabel, wetLabel;
    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> decayAttachment, dampingAttachment, wetAttachment;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrchestralStudioReverbAudioProcessorEditor)
};
