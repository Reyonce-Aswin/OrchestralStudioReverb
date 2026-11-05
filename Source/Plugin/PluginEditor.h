#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

class OrchestraStageMap final : public juce::Component
{
public:
    OrchestraStageMap();
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void setSelectedSection(int section, bool notify = false);
    std::function<void(int)> onSelectionChanged;

private:
    struct Sector
    {
        juce::String name;
        float startDegrees = 0.0f;
        float endDegrees = 0.0f;
        float innerRadius = 0.0f;
        float outerRadius = 0.0f;
        int family = 0;
    };

    juce::Path makeSector(const Sector&, juce::Point<float> centre, float baseline) const;
    std::vector<Sector> sectors;
    int selectedSection = 0;
};

class OrchestralStudioReverbAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit OrchestralStudioReverbAudioProcessorEditor(OrchestralStudioReverbAudioProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    OrchestralStudioReverbAudioProcessor& processor;
    OrchestraStageMap stageMap;
    juce::Slider decay, damping, roomSize, preDelay, early, dry, wet;
    juce::Label title, presetLabel, decayLabel, dampingLabel, roomSizeLabel, preDelayLabel;
    juce::Label earlyLabel, dryLabel, wetLabel, stagePositionLabel;
    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> decayAttachment, dampingAttachment, roomSizeAttachment;
    std::unique_ptr<Attachment> preDelayAttachment, earlyAttachment, dryAttachment, wetAttachment;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrchestralStudioReverbAudioProcessorEditor)
};
