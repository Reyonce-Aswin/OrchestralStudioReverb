#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "../UI/OrchestraLayoutComponent.h"

class OrchestralStudioReverbAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                          private juce::Timer
{
public:
    explicit OrchestralStudioReverbAudioProcessorEditor(OrchestralStudioReverbAudioProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void syncStageSelectionFromHost();
    void updateValueLabels();

    OrchestralStudioReverbAudioProcessor& processor;
    OrchestraLayoutComponent stageMap;
    juce::Slider decay, damping, roomSize, preDelay, early, dry, wet;
    juce::Label title, presetLabel, decayLabel, dampingLabel, roomSizeLabel, preDelayLabel;
    juce::Label earlyLabel, dryLabel, wetLabel, stagePositionLabel;
    juce::Label roomSizeValue, preDelayValue, decayValue, dampingValue;
    juce::Label earlyValue, dryValue, wetValue;
    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> decayAttachment, dampingAttachment, roomSizeAttachment;
    std::unique_ptr<Attachment> preDelayAttachment, earlyAttachment, dryAttachment, wetAttachment;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrchestralStudioReverbAudioProcessorEditor)
};
