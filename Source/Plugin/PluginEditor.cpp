#include "PluginEditor.h"
#include <cmath>
#include <vector>
#include <functional>

namespace
{
const juce::Colour background { 25, 27, 34 };
const juce::Colour panel { 34, 37, 47 };
const juce::Colour border { 78, 82, 96 };
const juce::Colour text { 226, 228, 237 };
const juce::Colour muted { 157, 162, 179 };
const juce::Colour accent { 184, 184, 218 };

juce::Colour familyColour(int family)
{
    switch (family)
    {
        case 1: return juce::Colour(66, 91, 112);
        case 2: return juce::Colour(67, 91, 82);
        case 3: return juce::Colour(91, 75, 91);
        case 4: return juce::Colour(102, 91, 68);
        default: return juce::Colour(75, 72, 91);
    }
}
}

OrchestraStageMap::OrchestraStageMap()
{
    sectors = {
        { "Piano", 0.0f, 180.0f, 0.0f, 0.0f, 0 },
        { "Violin I", 145.0f, 180.0f, 0.0f, 0.0f, 2 },
        { "Violin II", 112.0f, 145.0f, 0.0f, 0.0f, 2 },
        { "Violas", 78.0f, 112.0f, 0.0f, 0.0f, 2 },
        { "Celli", 43.0f, 78.0f, 0.0f, 0.0f, 2 },
        { "Basses", 0.0f, 43.0f, 0.0f, 0.0f, 2 },
        { "Piccolos", 145.0f, 180.0f, 0.0f, 0.0f, 1 },
        { "Flutes", 116.0f, 145.0f, 0.0f, 0.0f, 1 },
        { "Oboes", 87.0f, 116.0f, 0.0f, 0.0f, 1 },
        { "Clarinets", 58.0f, 87.0f, 0.0f, 0.0f, 1 },
        { "Bassoons", 29.0f, 58.0f, 0.0f, 0.0f, 1 },
        { "Harp & Celeste", 120.0f, 155.0f, 0.0f, 0.0f, 4 },
        { "Timpani & Untuned Percussion", 65.0f, 115.0f, 0.0f, 0.0f, 4 },
        { "Tuned Percussion", 25.0f, 60.0f, 0.0f, 0.0f, 4 },
        { "Horns", 155.0f, 180.0f, 0.0f, 0.0f, 3 },
        { "Trumpets", 0.0f, 25.0f, 0.0f, 0.0f, 3 },
        { "Tenor Trombones", 0.0f, 25.0f, 0.0f, 0.0f, 3 },
        { "Bass Trombones", 0.0f, 25.0f, 0.0f, 0.0f, 3 },
        { "Tuba", 0.0f, 25.0f, 0.0f, 0.0f, 3 }
    };
}

juce::Path OrchestraStageMap::makeSector(const Sector& sector,
                                          juce::Point<float> centre,
                                          float baseline) const
{
    const float scale = juce::jmin(getWidth() / 900.0f, getHeight() / 360.0f);
    float inner = 0.0f;
    float outer = 70.0f * scale;

    if (sector.family == 2) { inner = 78.0f * scale; outer = 133.0f * scale; }
    if (sector.family == 1) { inner = 139.0f * scale; outer = 192.0f * scale; }
    if (sector.family == 4) { inner = 198.0f * scale; outer = 247.0f * scale; }
    if (sector.family == 3) { inner = 139.0f * scale; outer = 247.0f * scale; }

    if (sector.family == 3 && sector.name != "Horns")
    {
        const int idx = sector.name == "Trumpets" ? 0 : sector.name == "Tenor Trombones" ? 1
                      : sector.name == "Bass Trombones" ? 2 : 3;
        const float band = (outer - inner) / 4.0f;
        inner += band * static_cast<float>(idx);
        outer = inner + band;
    }
    if (sector.family == 3 && sector.name == "Horns")
        inner = 198.0f * scale;

    juce::Path path;
    const float step = (sector.endDegrees - sector.startDegrees) / 32.0f;
    for (int i = 0; i <= 32; ++i)
    {
        const float a = juce::degreesToRadians(
            sector.startDegrees + step * static_cast<float>(i));
        const juce::Point<float> point(centre.x + std::cos(a) * outer,
                                        baseline - std::sin(a) * outer);
        if (i == 0) path.startNewSubPath(point);
        else path.lineTo(point);
    }

    if (inner > 0.0f)
    {
        for (int i = 32; i >= 0; --i)
        {
            const float a = juce::degreesToRadians(
                sector.startDegrees + step * static_cast<float>(i));
            path.lineTo(centre.x + std::cos(a) * inner,
                        baseline - std::sin(a) * inner);
        }
    }
    else
    {
        path.lineTo(centre.x, baseline);
    }
    path.closeSubPath();
    return path;
}

void OrchestraStageMap::paint(juce::Graphics& g)
{
    g.fillAll(panel);
    const auto centre = juce::Point<float>(getWidth() * 0.5f, getHeight() - 12.0f);
    const float baseline = getHeight() - 12.0f;
    const float scale = juce::jmin(getWidth() / 900.0f, getHeight() / 360.0f);

    for (int i = 0; i < static_cast<int>(sectors.size()); ++i)
    {
        const auto& sector = sectors[static_cast<size_t>(i)];
        const auto path = makeSector(sector, centre, baseline);
        g.setColour(i == selectedSection ? accent.withAlpha(0.82f)
                                          : familyColour(sector.family).withAlpha(0.42f));
        g.fillPath(path);
        g.setColour(i == selectedSection ? accent : border);
        g.strokePath(path, juce::PathStrokeType(i == selectedSection ? 2.0f : 1.0f));

        if (sector.name != "Piano")
        {
            const float mid = (sector.startDegrees + sector.endDegrees) * 0.5f;
            float radius = sector.family == 2 ? 105.0f : sector.family == 1 ? 165.0f
                         : sector.family == 4 ? 222.0f : 222.0f;
            if (sector.family == 3 && sector.name != "Horns")
                radius = 204.0f + (sector.name == "Trumpets" ? 0.0f :
                         sector.name == "Tenor Trombones" ? 13.0f :
                         sector.name == "Bass Trombones" ? 26.0f : 39.0f);
            const float a = juce::degreesToRadians(mid);
            const float x = centre.x + std::cos(a) * radius * scale;
            const float y = baseline - std::sin(a) * radius * scale;
            auto label = sector.name;
            if (label == "Timpani & Untuned Percussion") label = "TIMPANI / PERC.";
            if (label == "Harp & Celeste") label = "HARP / CELESTE";
            if (label == "Tuned Percussion") label = "TUBULAR BELLS / MALLETS";
            g.setColour(text);
            g.setFont(juce::Font(juce::FontOptions(juce::jmax(9.0f, 11.0f * scale))));
            g.drawFittedText(label, juce::Rectangle<int>(static_cast<int>(x - 58.0f * scale),
                static_cast<int>(y - 13.0f * scale), static_cast<int>(116.0f * scale),
                static_cast<int>(28.0f * scale)), juce::Justification::centred, 2);
        }
    }

    g.setColour(text);
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText("PIANO", juce::Rectangle<int>(getWidth() / 2 - 42, getHeight() - 55, 84, 18),
               juce::Justification::centred);
}

void OrchestraStageMap::mouseDown(const juce::MouseEvent& event)
{
    const auto centre = juce::Point<float>(getWidth() * 0.5f, getHeight() - 12.0f);
    const float baseline = getHeight() - 12.0f;
    for (int i = static_cast<int>(sectors.size()) - 1; i >= 0; --i)
    {
        if (makeSector(sectors[static_cast<size_t>(i)], centre, baseline).contains(event.position))
        {
            setSelectedSection(i, true);
            return;
        }
    }
}

void OrchestraStageMap::setSelectedSection(int section, bool notify)
{
    selectedSection = juce::jlimit(0, static_cast<int>(sectors.size()) - 1, section);
    repaint();
    if (notify && onSelectionChanged)
        onSelectionChanged(selectedSection);
}

OrchestralStudioReverbAudioProcessorEditor::OrchestralStudioReverbAudioProcessorEditor(
    OrchestralStudioReverbAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    title.setText("ORCHESTRAL STUDIO REVERB", juce::dontSendNotification);
    title.setFont(juce::Font(juce::FontOptions(20.0f).withStyle("Bold")));
    title.setColour(juce::Label::textColourId, text);
    addAndMakeVisible(title);

    presetLabel.setText("Concert Hall   ▾", juce::dontSendNotification);
    presetLabel.setFont(juce::Font(juce::FontOptions(13.0f)));
    presetLabel.setColour(juce::Label::textColourId, text);
    presetLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(presetLabel);

    stagePositionLabel.setText("STAGE POSITION", juce::dontSendNotification);
    stagePositionLabel.setColour(juce::Label::textColourId, muted);
    stagePositionLabel.setFont(juce::Font(juce::FontOptions(11.0f)));
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

    if (auto* stage = state.getParameter("stagePosition"))
        stageMap.setSelectedSection(static_cast<int>(stage->convertFrom0to1(stage->getValue())));

    stageMap.onSelectionChanged = [this](int section)
    {
        if (auto* parameter = processor.getParameters().getParameter("stagePosition"))
            parameter->setValueNotifyingHost(parameter->convertTo0to1(static_cast<float>(section)));
    };

    setSize(1180, 760);
}

void OrchestralStudioReverbAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(background);
    g.setColour(border);
    g.drawLine(28.0f, 70.0f, static_cast<float>(getWidth() - 28), 70.0f, 1.0f);
    g.drawLine(28.0f, 604.0f, static_cast<float>(getWidth() - 28), 604.0f, 1.0f);
    g.setColour(muted);
    g.setFont(11.0f);
    g.drawText("SPACE", 30, 112, 100, 18, juce::Justification::centredLeft);
    g.drawText("ORCHESTRA STAGE", 0, 100, getWidth(), 18, juce::Justification::centred);
    g.drawText("REFLECTIONS", getWidth() - 150, 112, 120, 18, juce::Justification::centredRight);
}

void OrchestralStudioReverbAudioProcessorEditor::resized()
{
    title.setBounds(30, 20, 420, 34);
    presetLabel.setBounds(getWidth() / 2 - 115, 20, 230, 34);
    stagePositionLabel.setBounds(30, 575, 180, 20);
    stageMap.setBounds(220, 126, getWidth() - 440, 430);

    const int y = 620;
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
