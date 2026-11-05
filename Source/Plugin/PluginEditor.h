#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <functional>
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

class OSREQGraph final : public juce::Component
{
public:
    explicit OSREQGraph(OrchestralStudioReverbAudioProcessor& p) : processor(p) {}
    std::function<void(int)> onBandSelected;
    void setSelectedBand(int band) { selectedBand = juce::jlimit(0, 7, band); repaint(); }
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
private:
    juce::AudioProcessorValueTreeState& state() noexcept { return processor.getParameters(); }
    float frequencyToX(float frequency) const;
    float xToFrequency(float x) const;
    float gainToY(float gain) const;
    float yToGain(float y) const;
    OrchestralStudioReverbAudioProcessor& processor;
    int selectedBand = 0;
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
    void setActiveView(bool showEQ);
    void selectEQBand(int band);
    void rebuildEQAttachments();
    void updateEQBandTypeControls();

    OrchestralStudioReverbAudioProcessor& processor;
    OSRControlLookAndFeel controlLookAndFeel;
    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    OrchestraLayoutComponent stageMap;
    OSREQGraph eqGraph;
    juce::TextButton stageViewButton { "STAGE" };
    juce::TextButton eqViewButton { "EQ" };
    juce::TextButton eqBypassButton { "EQ BYPASS" };
    juce::TextButton eqBandButtons[8];
    juce::TextButton eqEnableButton { "BAND ON" };
    juce::ComboBox eqTypeBox, eqSlopeBox;
    juce::Slider eqFrequency, eqGain, eqQ, eqMix;
    juce::Label eqFrequencyLabel, eqGainLabel, eqQLabel, eqMixLabel;
    juce::Label eqFrequencyValue, eqGainValue, eqQValue, eqMixValue;
    juce::Label eqTypeLabel, eqSlopeLabel;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<ButtonAttachment> eqBypassAttachment, eqEnableAttachment;
    std::unique_ptr<ComboAttachment> eqTypeAttachment, eqSlopeAttachment;
    std::unique_ptr<Attachment> eqFrequencyAttachment, eqGainAttachment, eqQAttachment, eqMixAttachment;
    int selectedEQBand = 0;
    bool showingEQ = false;
    juce::String lastDisplayedStageSelection;
    juce::Label selectorTitle;
    juce::TextButton standardLayoutButton { "FULL MODERN" };
    juce::TextButton classicalLayoutButton { "FULL CLASSICAL" };
    juce::TextButton chamberModernButton { "CHAMBER MODERN" };
    juce::TextButton chamberClassicalButton { "CHAMBER CLASSICAL" };
    juce::Slider decay, damping, roomSize, preDelay, early, dry, wet;
    juce::Label title, presetLabel, decayLabel, dampingLabel, roomSizeLabel, preDelayLabel;
    juce::Label earlyLabel, dryLabel, wetLabel, stagePositionLabel;
    juce::Label roomSizeValue, preDelayValue, decayValue, dampingValue;
    juce::Label earlyValue, dryValue, wetValue;
    std::unique_ptr<Attachment> decayAttachment, dampingAttachment, roomSizeAttachment;
    std::unique_ptr<Attachment> preDelayAttachment, earlyAttachment, dryAttachment, wetAttachment;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrchestralStudioReverbAudioProcessorEditor)
};
