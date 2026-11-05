#include "PluginEditor.h"

OrchestralStudioReverbAudioProcessorEditor::OrchestralStudioReverbAudioProcessorEditor(
    OrchestralStudioReverbAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    const auto bronze = juce::Colour::fromRGB(190, 158, 111);
    title.setText("ORCHESTRAL STUDIO REVERB", juce::dontSendNotification);
    title.setFont(juce::Font(juce::FontOptions(21.0f).withStyle("Bold")));
    title.setColour(juce::Label::textColourId, juce::Colour::fromRGB(224, 211, 190));
    addAndMakeVisible(title);

    auto setupSlider = [this, bronze](juce::Slider& slider, juce::Label& label,
                                      const juce::String& text)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 22);
        slider.setColour(juce::Slider::rotarySliderFillColourId, bronze);
        slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour::fromRGB(72, 69, 64));
        slider.setColour(juce::Slider::thumbColourId, juce::Colour::fromRGB(232, 215, 183));
        slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour::fromRGB(230, 224, 214));
        slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour::fromRGB(39, 39, 38));
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, juce::Colour::fromRGB(190, 177, 154));
        addAndMakeVisible(slider);
        addAndMakeVisible(label);
    };

    setupSlider(decay, decayLabel, "DECAY");
    setupSlider(damping, dampingLabel, "DAMPING");
    setupSlider(roomSize, roomSizeLabel, "ROOM SIZE");
    setupSlider(preDelay, preDelayLabel, "PRE-DELAY");
    setupSlider(early, earlyLabel, "EARLY");
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
    setSize(1080, 370);
}

void OrchestralStudioReverbAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(29, 30, 30));
    g.setColour(juce::Colour::fromRGB(111, 91, 64));
    g.drawLine(28.0f, 68.0f, static_cast<float>(getWidth() - 28), 68.0f, 1.0f);
    g.setColour(juce::Colour::fromRGB(53, 53, 51));
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(18.0f).withTop(88.0f), 8.0f);
    g.setColour(juce::Colour::fromRGB(177, 163, 140));
    g.setFont(12.0f);
    g.drawText("EARLY REFLECTIONS  +  LATE FDN  /  PARALLEL RETURNS",
               34, getHeight() - 32, getWidth() - 68, 18,
               juce::Justification::centredLeft);
}

void OrchestralStudioReverbAudioProcessorEditor::resized()
{
    title.setBounds(30, 22, getWidth() - 60, 34);
    const int sectionWidth = (getWidth() - 60) / 7;
    const int x0 = 30;
    const int top = 112;
    const int sliderSize = 112;
    juce::Slider* sliders[] = {
        &decay, &damping, &roomSize, &preDelay, &early, &dry, &wet
    };
    juce::Label* labels[] = {
        &decayLabel, &dampingLabel, &roomSizeLabel, &preDelayLabel,
        &earlyLabel, &dryLabel, &wetLabel
    };
    for (int i = 0; i < 7; ++i)
    {
        const int x = x0 + i * sectionWidth;
        labels[i]->setBounds(x, top, sectionWidth, 22);
        sliders[i]->setBounds(x + (sectionWidth - sliderSize) / 2, top + 24,
                              sliderSize, sliderSize);
    }
}
