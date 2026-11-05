#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "../UI/OrchestraLayoutComponent.h"

class OSRControlLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider&) override;
    void drawLinearSlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle, juce::Slider&) override;
    void drawButtonText(juce::Graphics&, juce::TextButton&,
                        bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

class OrchestralStudioReverbAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                          private juce::Timer
{
public:
    explicit OrchestralStudioReverbAudioProcessorEditor(OrchestralStudioReverbAudioProcessor&);
    ~OrchestralStudioReverbAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void syncStageSelectionFromHost();
    void updateValueLabels();
    void updateLayoutButtons();

    OrchestralStudioReverbAudioProcessor& processor;
    OSRControlLookAndFeel controlLookAndFeel;
    OrchestraLayoutComponent stageMap;
    juce::TextButton standardLayoutButton { "STANDARD" };
    juce::TextButton classicalLayoutButton { "CLASSICAL / EUROPEAN" };
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
