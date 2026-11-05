#include "PluginEditor.h"
#include <cmath>

namespace
{
const juce::Colour background { 25, 27, 34 };
const juce::Colour panel { 34, 37, 47 };
const juce::Colour border { 126, 131, 148 };
const juce::Colour text { 226, 228, 237 };
const juce::Colour muted { 157, 162, 179 };
const juce::Colour accent { 210, 212, 226 };
}

OrchestralStudioReverbAudioProcessorEditor::OrchestralStudioReverbAudioProcessorEditor(
    OrchestralStudioReverbAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    title.setText("ORCHESTRAL STUDIO REVERB", juce::dontSendNotification);
    title.setFont(juce::Font(juce::FontOptions(20.0f).withStyle("Bold")));
    title.setColour(juce::Label::textColourId, text);
    addAndMakeVisible(title);

    presetLabel.setText("CONCERT HALL", juce::dontSendNotification);
    presetLabel.setFont(juce::Font(juce::FontOptions(13.0f)));
    presetLabel.setColour(juce::Label::textColourId, text);
    presetLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(presetLabel);

    stagePositionLabel.setText("STAGE POSITION", juce::dontSendNotification);
    stagePositionLabel.setColour(juce::Label::textColourId, muted);
    stagePositionLabel.setFont(juce::Font(juce::FontOptions(11.0f)));
    stagePositionLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(stagePositionLabel);
    addAndMakeVisible(stageMap);

    const auto setupSlider = [this](juce::Slider& slider, juce::Label& label,
                                    const juce::String& name)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 68, 18);
        slider.setColour(juce::Slider::rotarySliderFillColourId, accent);
        slider.setColour(juce::Slider::rotarySliderOutlineColourId, border);
        slider.setColour(juce::Slider::thumbColourId, text);
        slider.setColour(juce::Slider::textBoxTextColourId, muted);
        slider.setColour(juce::Slider::textBoxBackgroundColourId, background);
        label.setText(name, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, text);
        label.setFont(juce::Font(juce::FontOptions(10.0f)));
        addAndMakeVisible(slider);
        addAndMakeVisible(label);
    };

    setupSlider(roomSize, roomSizeLabel, "ROOM SIZE");
    setupSlider(preDelay, preDelayLabel, "PRE-DELAY");
    setupSlider(decay, decayLabel, "DECAY");
    setupSlider(damping, dampingLabel, "DAMPING");
    setupSlider(early, earlyLabel, "EARLY LEVEL");
    setupSlider(dry, dryLabel, "DRY");
    setupSlider(wet, wetLabel, "LATE WET");

    auto& state = processor.getParameters();
    decayAttachment = std::make_unique<Attachment>(state, "decay", decay);
    dampingAttachment = std::make_unique<Attachment>(state, "damping", damping);
    roomSizeAttachment = std::make_unique<Attachment>(state, "roomSize", roomSize);
    preDelayAttachment = std::make_unique<Attachment>(state, "preDelay", preDelay);
    earlyAttachment = std::make_unique<Attachment>(state, "early", early);
    dryAttachment = std::make_unique<Attachment>(state, "dry", dry);
    wetAttachment = std::make_unique<Attachment>(state, "wet", wet);

    stageMap.onSelectionChanged = [this](const juce::String& sectionName)
    {
        if (auto* parameter = processor.getParameters().getParameter("stagePosition"))
        {
            if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(parameter))
            {
                const int index = choice->choices.indexOf(sectionName);
                if (index >= 0)
                    parameter->setValueNotifyingHost(parameter->convertTo0to1(static_cast<float>(index)));
            }
        }
        syncStageSelectionFromHost();
    };

    syncStageSelectionFromHost();
    startTimerHz(20);
    setSize(1180, 840);
}

void OrchestralStudioReverbAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(background);
    g.setColour(border);
    g.drawLine(28.0f, 70.0f, static_cast<float>(getWidth() - 28), 70.0f, 1.0f);
    g.drawLine(28.0f, 650.0f, static_cast<float>(getWidth() - 28), 650.0f, 1.0f);
    g.setColour(muted);
    g.setFont(11.0f);
    g.drawText("ORCHESTRA LAYOUT", 30, 82, 220, 18, juce::Justification::centredLeft);
    g.drawText("SPACE  /  EARLY REFLECTIONS", getWidth() - 300, 82, 270, 18,
               juce::Justification::centredRight);
}

void OrchestralStudioReverbAudioProcessorEditor::resized()
{
    title.setBounds(30, 20, 420, 34);
    presetLabel.setBounds(getWidth() / 2 - 115, 20, 230, 34);
    stagePositionLabel.setBounds(getWidth() - 330, 24, 300, 28);
    stageMap.setBounds(0, 100, getWidth(), 540);

    const int y = 665;
    const int count = 7;
    const int width = (getWidth() - 60) / count;
    juce::Slider* sliders[] = { &roomSize, &preDelay, &decay, &damping, &early, &dry, &wet };
    juce::Label* labels[] = { &roomSizeLabel, &preDelayLabel, &decayLabel, &dampingLabel,
                              &earlyLabel, &dryLabel, &wetLabel };
    for (int i = 0; i < count; ++i)
    {
        const int x = 30 + i * width;
        labels[i]->setBounds(x, y, width, 18);
        sliders[i]->setBounds(x + (width - 100) / 2, y + 18, 100, 112);
    }
}

void OrchestralStudioReverbAudioProcessorEditor::syncStageSelectionFromHost()
{
    if (auto* parameter = processor.getParameters().getParameter("stagePosition"))
    {
        const auto name = parameter->getCurrentValueAsText();
        if (stageMap.getSelectedSection() != name)
            stageMap.setSelectedSection(name);

        stagePositionLabel.setText("SELECTED: " + name.toUpperCase(),
                                   juce::dontSendNotification);
    }
}

void OrchestralStudioReverbAudioProcessorEditor::timerCallback()
{
    syncStageSelectionFromHost();
}
