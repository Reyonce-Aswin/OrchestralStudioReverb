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
    setResizable(true, true);
    setResizeLimits(850, 620, 1400, 1000);
    setSize(1000, 700);
}

void OrchestralStudioReverbAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(background);
    g.setColour(border);
    const int sectionTop = 76;
    const int controlsTop = getHeight() - 166;
    g.drawLine(24.0f, 64.0f, static_cast<float>(getWidth() - 24), 64.0f, 1.0f);
    g.drawLine(24.0f, static_cast<float>(controlsTop - 10),
               static_cast<float>(getWidth() - 24), static_cast<float>(controlsTop - 10), 1.0f);
    g.setColour(muted);
    g.setFont(10.5f);
    g.drawText("ORCHESTRA LAYOUT", 26, sectionTop, 220, 16, juce::Justification::centredLeft);
    g.drawText("SPACE  /  EARLY REFLECTIONS", getWidth() - 280, sectionTop, 254, 16,
               juce::Justification::centredRight);
}

void OrchestralStudioReverbAudioProcessorEditor::resized()
{
    constexpr int margin = 26;
    title.setBounds(margin, 16, 390, 34);
    presetLabel.setBounds(getWidth() / 2 - 100, 16, 200, 34);
    stagePositionLabel.setBounds(getWidth() - 300, 18, 274, 30);

    const int controlsTop = getHeight() - 166;
    const int mapTop = 96;
    const int mapHeight = juce::jmax(220, controlsTop - mapTop - 14);
    stageMap.setBounds(0, mapTop, getWidth(), mapHeight);

    const int count = 7;
    const int availableWidth = getWidth() - margin * 2;
    const int cellWidth = availableWidth / count;
    juce::Slider* sliders[] = { &roomSize, &preDelay, &decay, &damping, &early, &dry, &wet };
    juce::Label* labels[] = { &roomSizeLabel, &preDelayLabel, &decayLabel, &dampingLabel,
                              &earlyLabel, &dryLabel, &wetLabel };
    for (int i = 0; i < count; ++i)
    {
        const int x = margin + i * cellWidth;
        labels[i]->setBounds(x, controlsTop + 1, cellWidth, 16);
        const int dialWidth = juce::jmin(88, cellWidth - 8);
        sliders[i]->setBounds(x + (cellWidth - dialWidth) / 2,
                              controlsTop + 17, dialWidth, 142);
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
