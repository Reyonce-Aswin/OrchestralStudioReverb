#include "PluginEditor.h"
#include <cmath>

namespace
{
const juce::Colour background { 0x0F, 0x11, 0x1A };
const juce::Colour panel { 0x17, 0x1A, 0x26 };
const juce::Colour border { 0x3A, 0x3E, 0x4C };
const juce::Colour text { 0xEE, 0xF0, 0xF7 };
const juce::Colour muted { 0x8B, 0x90, 0xA3 };
const juce::Colour knobTrack { 0x3A, 0x3E, 0x4C };
const juce::Colour knobValue { 0xE8, 0xEA, 0xF0 };
const juce::Colour knobName { 0xD8, 0xDB, 0xE6 };
const juce::Colour sliderTrack { 0x2C, 0x2F, 0x3C };
const juce::Colour sliderFill { 0xC9, 0xCC, 0xD8 };
constexpr float kPi = 3.14159265358979323846f;

juce::Font uiFont(float height, bool bold = false)
{
    juce::FontOptions options;
    auto font = juce::Font(options.withName("Inter").withHeight(height)
                                .withStyle(bold ? "SemiBold" : "Regular"));
    if (!font.getTypefaceName().containsIgnoreCase("Inter"))
        font = juce::Font(options.withName("Arial").withHeight(height)
                              .withStyle(bold ? "Bold" : "Regular"));
    return font;
}

void drawLetterSpaced(juce::Graphics& g, const juce::String& value,
                      juce::Rectangle<float> area, float fontHeight, juce::Colour colour)
{
    const auto font = uiFont(fontHeight, true);
    float totalWidth = 0.0f;
    for (int i = 0; i < value.length(); ++i)
        totalWidth += static_cast<float>(font.getStringWidth(value.substring(i, i + 1))) + 1.5f;
    float x = area.getX() + (area.getWidth() - totalWidth) * 0.5f;
    g.setColour(colour);
    g.setFont(font);
    for (int i = 0; i < value.length(); ++i)
    {
        const auto character = value.substring(i, i + 1);
        const float width = static_cast<float>(font.getStringWidth(character));
        g.drawText(character, juce::Rectangle<float>(x, area.getY(), width + 1.0f, area.getHeight()),
                   juce::Justification::centred, false);
        x += width + 1.5f;
    }
}
}

void OSRControlLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                              float proportion, float startAngle, float endAngle,
                                              juce::Slider&)
{
    const float diameter = static_cast<float>(juce::jmin(width, height)) * 0.70f;
    const float radius = diameter * 0.5f;
    const float cx = static_cast<float>(x) + static_cast<float>(width) * 0.5f;
    const float cy = static_cast<float>(y) + static_cast<float>(height) * 0.5f;
    const float angle = startAngle + proportion * (endAngle - startAngle);
    juce::Path track, value;
    track.addCentredArc(cx, cy, radius, radius, 0.0f, startAngle, endAngle, true);
    value.addCentredArc(cx, cy, radius, radius, 0.0f, startAngle, angle, true);
    const juce::PathStrokeType arcStroke(3.0f, juce::PathStrokeType::curved,
                                         juce::PathStrokeType::rounded);
    g.setColour(knobTrack);
    g.strokePath(track, arcStroke);
    g.setColour(knobValue);
    g.strokePath(value, arcStroke);

    const juce::Point<float> pointer(std::cos(angle), std::sin(angle));
    g.setColour(juce::Colours::white);
    g.drawLine(cx + pointer.x * radius * 0.35f, cy + pointer.y * radius * 0.35f,
               cx + pointer.x * radius * 0.88f, cy + pointer.y * radius * 0.88f, 2.0f);
}

void OSRControlLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                              float sliderPos, float, float,
                                              juce::Slider::SliderStyle style, juce::Slider&)
{
    if (style == juce::Slider::LinearHorizontal)
    {
        const float cy = static_cast<float>(y) + static_cast<float>(height) * 0.5f;
        const float left = static_cast<float>(x);
        const float right = static_cast<float>(x + width);
        g.setColour(sliderTrack);
        g.fillRoundedRectangle(left, cy - 2.0f, right - left, 4.0f, 2.0f);
        g.setColour(sliderFill);
        g.fillRoundedRectangle(left, cy - 2.0f, sliderPos - left, 4.0f, 2.0f);
        g.setColour(juce::Colours::white);
        g.fillEllipse(sliderPos - 5.0f, cy - 5.0f, 10.0f, 10.0f);
    }
    else
    {
        const float cx = static_cast<float>(x) + static_cast<float>(width) * 0.5f;
        const float cy = static_cast<float>(y) + static_cast<float>(height) * 0.5f;
        g.setColour(sliderTrack);
        g.fillRoundedRectangle(static_cast<float>(x), cy - 2.0f,
                               static_cast<float>(width), 4.0f, 2.0f);
        g.setColour(sliderFill);
        g.fillRoundedRectangle(static_cast<float>(x), cy - 2.0f,
                               juce::jlimit(0.0f, static_cast<float>(width), sliderPos - static_cast<float>(x)),
                               4.0f, 2.0f);
        g.setColour(juce::Colours::white);
        g.fillEllipse(sliderPos - 5.0f, cy - 5.0f, 10.0f, 10.0f);
        juce::ignoreUnused(cx);
    }
}

OrchestralStudioReverbAudioProcessorEditor::OrchestralStudioReverbAudioProcessorEditor(
    OrchestralStudioReverbAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    title.setText("ORCHESTRAL STUDIO REVERB", juce::dontSendNotification);
    title.setFont(uiFont(18.0f, true));
    title.setColour(juce::Label::textColourId, text);
    addAndMakeVisible(title);

    presetLabel.setText("CONCERT HALL", juce::dontSendNotification);
    presetLabel.setFont(uiFont(12.0f));
    presetLabel.setColour(juce::Label::textColourId, text);
    presetLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(presetLabel);

    stagePositionLabel.setFont(uiFont(10.0f));
    stagePositionLabel.setColour(juce::Label::textColourId, muted);
    stagePositionLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(stagePositionLabel);
    addAndMakeVisible(stageMap);

    auto setupLayoutButton = [this](juce::TextButton& button)
    {
        button.setClickingTogglesState(false);
        button.setColour(juce::TextButton::buttonColourId, panel);
        button.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0x25, 0x34, 0x5E));
        button.setColour(juce::TextButton::textColourOffId, text);
        button.setColour(juce::TextButton::textColourOnId, text);
        button.setLookAndFeel(&controlLookAndFeel);
        button.setFont(uiFont(9.5f, true));
        addAndMakeVisible(button);
    };
    setupLayoutButton(standardLayoutButton);
    setupLayoutButton(classicalLayoutButton);
    standardLayoutButton.onClick = [this]
    {
        stageMap.setLayout(OrchestraLayoutComponent::Layout::Standard);
        updateLayoutButtons();
    };
    classicalLayoutButton.onClick = [this]
    {
        stageMap.setLayout(OrchestraLayoutComponent::Layout::Classical);
        updateLayoutButtons();
    };

    const auto setupSlider = [this](juce::Slider& slider, juce::Label& label,
                                    juce::Label& valueLabel, const juce::String& name)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider.setRotaryParameters(juce::degreesToRadians(135.0f),
                                   juce::degreesToRadians(405.0f), true);
        slider.setLookAndFeel(&controlLookAndFeel);
        slider.setColour(juce::Slider::rotarySliderFillColourId, knobValue);
        slider.setColour(juce::Slider::rotarySliderOutlineColourId, knobTrack);
        slider.setColour(juce::Slider::thumbColourId, juce::Colours::white);

        label.setText(name, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, knobName);
        label.setFont(uiFont(9.5f));

        valueLabel.setJustificationType(juce::Justification::centred);
        valueLabel.setColour(juce::Label::textColourId, muted);
        valueLabel.setFont(uiFont(11.0f));

        addAndMakeVisible(slider);
        addAndMakeVisible(label);
        addAndMakeVisible(valueLabel);
    };

    setupSlider(roomSize, roomSizeLabel, roomSizeValue, "ROOM SIZE");
    setupSlider(preDelay, preDelayLabel, preDelayValue, "PRE-DELAY");
    setupSlider(decay, decayLabel, decayValue, "DECAY");
    setupSlider(damping, dampingLabel, dampingValue, "DAMPING");
    setupSlider(early, earlyLabel, earlyValue, "EARLY LEVEL");
    setupSlider(dry, dryLabel, dryValue, "DRY");
    setupSlider(wet, wetLabel, wetValue, "LATE WET");

    auto& state = processor.getParameters();
    decayAttachment = std::make_unique<Attachment>(state, "decay", decay);
    dampingAttachment = std::make_unique<Attachment>(state, "damping", damping);
    roomSizeAttachment = std::make_unique<Attachment>(state, "roomSize", roomSize);
    preDelayAttachment = std::make_unique<Attachment>(state, "preDelay", preDelay);
    earlyAttachment = std::make_unique<Attachment>(state, "early", early);
    dryAttachment = std::make_unique<Attachment>(state, "dry", dry);
    wetAttachment = std::make_unique<Attachment>(state, "wet", wet);

    const auto refreshValues = [this] { updateValueLabels(); };
    roomSize.onValueChange = refreshValues;
    preDelay.onValueChange = refreshValues;
    decay.onValueChange = refreshValues;
    damping.onValueChange = refreshValues;
    early.onValueChange = refreshValues;
    dry.onValueChange = refreshValues;
    wet.onValueChange = refreshValues;
    updateValueLabels();

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
    stageMap.onLayoutChanged = [this](OrchestraLayoutComponent::Layout) { updateLayoutButtons(); };

    stageMap.setLayout(OrchestraLayoutComponent::Layout::Standard);
    syncStageSelectionFromHost();
    updateLayoutButtons();
    startTimerHz(20);
    setResizable(true, true);
    setResizeLimits(850, 620, 1400, 1000);
    setSize(1000, 700);
}

OrchestralStudioReverbAudioProcessorEditor::~OrchestralStudioReverbAudioProcessorEditor()
{
    for (auto* slider : { &roomSize, &preDelay, &decay, &damping, &early, &dry, &wet })
        slider->setLookAndFeel(nullptr);
    standardLayoutButton.setLookAndFeel(nullptr);
    classicalLayoutButton.setLookAndFeel(nullptr);
}

void OrchestralStudioReverbAudioProcessorEditor::updateLayoutButtons()
{
    const bool standard = stageMap.getLayout() == OrchestraLayoutComponent::Layout::Standard;
    standardLayoutButton.setColour(juce::TextButton::buttonColourId,
                                   standard ? juce::Colour(0x25, 0x34, 0x5E) : panel);
    classicalLayoutButton.setColour(juce::TextButton::buttonColourId,
                                    standard ? panel : juce::Colour(0x25, 0x34, 0x5E));
    standardLayoutButton.repaint();
    classicalLayoutButton.repaint();
}

void OrchestralStudioReverbAudioProcessorEditor::updateValueLabels()
{
    roomSizeValue.setText(juce::String(roomSize.getValue(), 2) + " "
                              + juce::String::charToString(0x00D7),
                          juce::dontSendNotification);
    preDelayValue.setText(juce::String(preDelay.getValue(), 1) + " ms", juce::dontSendNotification);
    decayValue.setText(juce::String(decay.getValue(), 2) + " s", juce::dontSendNotification);

    const double dampingHz = damping.getValue();
    dampingValue.setText(dampingHz >= 1000.0
        ? juce::String(dampingHz / 1000.0, 2) + " kHz"
        : juce::String(dampingHz, 0) + " Hz", juce::dontSendNotification);

    earlyValue.setText(juce::String(early.getValue() * 100.0, 1) + "%", juce::dontSendNotification);
    dryValue.setText(juce::String(dry.getValue() * 100.0, 1) + "%", juce::dontSendNotification);
    wetValue.setText(juce::String(wet.getValue() * 100.0, 1) + "%", juce::dontSendNotification);
}

void OrchestralStudioReverbAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(background);
    const int controlsTop = getHeight() - 166;
    g.setColour(border);
    g.drawLine(24.0f, 64.0f, static_cast<float>(getWidth() - 24), 64.0f, 1.0f);
    g.drawLine(24.0f, static_cast<float>(controlsTop - 10),
               static_cast<float>(getWidth() - 24), static_cast<float>(controlsTop - 10), 1.0f);
    drawLetterSpaced(g, "ORCHESTRA LAYOUT", { 26.0f, 70.0f, 190.0f, 24.0f }, 10.0f, muted);
    drawLetterSpaced(g, "ROOM / REVERB / MIX", { 26.0f, static_cast<float>(controlsTop - 7),
                                                  220.0f, 15.0f }, 9.0f, muted);
}

void OrchestralStudioReverbAudioProcessorEditor::resized()
{
    constexpr int margin = 26;
    title.setBounds(margin, 14, 360, 34);
    presetLabel.setBounds(getWidth() / 2 - 100, 14, 200, 34);

    standardLayoutButton.setBounds(236, 69, 112, 27);
    classicalLayoutButton.setBounds(354, 69, 178, 27);
    stagePositionLabel.setBounds(getWidth() - 420, 69, 394, 27);

    const int controlsTop = getHeight() - 166;
    const int mapTop = 106;
    const int mapHeight = juce::jmax(220, controlsTop - mapTop - 14);
    stageMap.setBounds(0, mapTop, getWidth(), mapHeight);

    const int count = 7;
    const int availableWidth = getWidth() - margin * 2;
    const int cellWidth = availableWidth / count;
    juce::Slider* sliders[] = { &roomSize, &preDelay, &decay, &damping, &early, &dry, &wet };
    juce::Label* labels[] = { &roomSizeLabel, &preDelayLabel, &decayLabel, &dampingLabel,
                              &earlyLabel, &dryLabel, &wetLabel };
    juce::Label* values[] = { &roomSizeValue, &preDelayValue, &decayValue, &dampingValue,
                              &earlyValue, &dryValue, &wetValue };
    for (int i = 0; i < count; ++i)
    {
        const int x = margin + i * cellWidth;
        labels[i]->setBounds(x, controlsTop + 3, cellWidth, 15);
        const int dialWidth = juce::jmin(88, cellWidth - 8);
        sliders[i]->setBounds(x + (cellWidth - dialWidth) / 2, controlsTop + 19, dialWidth, 88);
        values[i]->setBounds(x, controlsTop + 107, cellWidth, 20);
    }
}

void OrchestralStudioReverbAudioProcessorEditor::syncStageSelectionFromHost()
{
    if (auto* parameter = processor.getParameters().getParameter("stagePosition"))
    {
        const auto name = parameter->getCurrentValueAsText();
        if (stageMap.getSelectedParameterName() != name)
            stageMap.setSelectedSection(name);
        stagePositionLabel.setText("SELECTED: " + name.toUpperCase(), juce::dontSendNotification);
    }
}

void OrchestralStudioReverbAudioProcessorEditor::timerCallback()
{
    syncStageSelectionFromHost();
}
