#include "PluginEditor.h"
#include <cmath>
#include <vector>
#include <functional>

namespace
{
const juce::Colour background { 25, 27, 34 };
const juce::Colour panel { 34, 37, 47 };
const juce::Colour border { 126, 131, 148 };
const juce::Colour text { 226, 228, 237 };
const juce::Colour muted { 157, 162, 179 };
const juce::Colour accent { 210, 212, 226 };
}

OrchestraStageMap::OrchestraStageMap()
{
    // Main orchestra: strings in the inner ring, woodwinds in the middle,
    // percussion in the outer ring. Brass sits on its own outer-right arc.
    // Angles are measured from the right-hand baseline towards the left.
    sectors = {
        { "Piano", 0.0f, 180.0f, 0.0f, 70.0f, 0 },

        { "Violin I", 145.0f, 180.0f, 78.0f, 133.0f, 2 },
        { "Violin II", 105.0f, 145.0f, 78.0f, 133.0f, 2 },
        { "Violas", 65.0f, 105.0f, 78.0f, 133.0f, 2 },
        { "Celli", 0.0f, 65.0f, 78.0f, 139.0f, 2 },
        { "Basses", 0.0f, 43.0f, 139.0f, 192.0f, 2 },

        { "Piccolos", 165.0f, 180.0f, 139.0f, 192.0f, 1 },
        { "Flutes", 145.0f, 165.0f, 139.0f, 192.0f, 1 },
        { "Oboes", 105.0f, 145.0f, 139.0f, 192.0f, 1 },
        { "Clarinets", 65.0f, 105.0f, 139.0f, 192.0f, 1 },
        { "Bassoons", 43.0f, 65.0f, 139.0f, 192.0f, 1 },

        { "Harp & Celeste", 145.0f, 165.0f, 198.0f, 247.0f, 4 },
        { "Timpani & Untuned Percussion", 65.0f, 145.0f, 198.0f, 247.0f, 4 },
        { "Tuned Percussion", 43.0f, 65.0f, 198.0f, 247.0f, 4 },
        { "Horns", 165.0f, 180.0f, 198.0f, 247.0f, 3 },

        // Reverse the previous brass order: trumpets at the top, tuba at
        // the bottom, with all four sections outside the percussion ring.
        { "Trumpets", 67.5f, 90.0f, 265.0f, 325.0f, 3 },
        { "Tenor Trombones", 45.0f, 67.5f, 265.0f, 325.0f, 3 },
        { "Bass Trombones", 22.5f, 45.0f, 265.0f, 325.0f, 3 },
        { "Tuba", 0.0f, 22.5f, 265.0f, 325.0f, 3 }
    };
}

juce::Path OrchestraStageMap::makeSector(const Sector& sector,
                                          juce::Point<float> centre,
                                          float baseline) const
{
    const float scale = juce::jmin(getWidth() / 900.0f, getHeight() / 360.0f);
    const float inner = sector.innerRadius * scale;
    const float outer = sector.outerRadius * scale;

    juce::Path path;
    const float step = (sector.endDegrees - sector.startDegrees) / 40.0f;
    for (int i = 0; i <= 40; ++i)
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
        for (int i = 40; i >= 0; --i)
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

    // Draw from the innermost sections outwards so every boundary remains crisp.
    for (int i = 0; i < static_cast<int>(sectors.size()); ++i)
    {
        const auto& sector = sectors[static_cast<size_t>(i)];
        const auto path = makeSector(sector, centre, baseline);
        g.setColour(i == selectedSection ? accent : border);
        g.strokePath(path, juce::PathStrokeType(i == selectedSection ? 2.0f : 1.15f));

        if (sector.name != "Piano")
        {
            const float mid = (sector.startDegrees + sector.endDegrees) * 0.5f;
            const float radius = (sector.innerRadius + sector.outerRadius) * 0.5f;
            const float scale = juce::jmin(getWidth() / 900.0f, getHeight() / 360.0f);
            const float a = juce::degreesToRadians(mid);
            const float x = centre.x + std::cos(a) * radius * scale;
            const float y = baseline - std::sin(a) * radius * scale;

            juce::String label = sector.name;
            if (label == "Timpani & Untuned Percussion") label = "TIMPANI / PERC";
            if (label == "Harp & Celeste") label = "HARP / CELESTE";
            if (label == "Tuned Percussion") label = "TUBULAR BELLS / MALLETS";
            if (label == "Tenor Trombones") label = "TENOR TROMBONES";
            if (label == "Bass Trombones") label = "BASS TROMBONES";

            const bool brass = sector.family == 3 && sector.name != "Horns";
            const float labelWidth = brass ? 112.0f : 132.0f;
            const float labelScale = brass ? 0.78f : 0.86f;
            g.setColour(text);
            g.setFont(juce::Font(juce::FontOptions(
                juce::jmax(8.5f, 11.0f * scale * labelScale))));
            g.drawFittedText(label,
                juce::Rectangle<int>(static_cast<int>(x - labelWidth * scale * 0.5f),
                    static_cast<int>(y - 18.0f * scale),
                    static_cast<int>(labelWidth * scale),
                    static_cast<int>(36.0f * scale)),
                juce::Justification::centred, 2);
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

    presetLabel.setText("CONCERT HALL", juce::dontSendNotification);
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
    g.drawLine(28.0f, 520.0f, static_cast<float>(getWidth() - 28), 520.0f, 1.0f);
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
    stagePositionLabel.setBounds(30, 490, 180, 20);
    stageMap.setBounds(80, 128, getWidth() - 160, 350);

    const int y = 535;
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
