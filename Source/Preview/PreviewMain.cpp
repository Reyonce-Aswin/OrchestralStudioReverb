#include "../UI/OrchestraLayoutComponent.h"

class PreviewContent final : public juce::Component
{
public:
    PreviewContent()
    {
        setOpaque(true);
        addAndMakeVisible(layout);
        layout.setSelectedSection("Violas");
        layout.onSelectionChanged = [this](const juce::String& name)
        {
            selected.setText("Selected: " + name, juce::dontSendNotification);
        };

        heading.setText("ORCHESTRA LAYOUT", juce::dontSendNotification);
        heading.setFont(juce::Font(juce::FontOptions(13.0f).withName("Inter").withStyle("Bold")));
        heading.setColour(juce::Label::textColourId, juce::Colour(0xA8, 0xAE, 0xC1));
        heading.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(heading);

        selected.setText("Selected: Violas", juce::dontSendNotification);
        selected.setFont(juce::Font(juce::FontOptions(13.0f).withName("Inter")));
        selected.setColour(juce::Label::textColourId, juce::Colour(0xA8, 0xAE, 0xC1));
        selected.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(selected);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0x0F, 0x11, 0x1A));
    }

    void resized() override
    {
        heading.setBounds(40, 0, 250, 60);
        selected.setBounds(getWidth() - 300, 0, 260, 60);
        layout.setBounds(0, 60, getWidth(), getHeight() - 60);
    }

private:
    OrchestraLayoutComponent layout;
    juce::Label heading, selected;
};

class PreviewWindow final : public juce::DocumentWindow
{
public:
    PreviewWindow()
        : DocumentWindow("Orchestra Layout Preview",
                         juce::Colour(0x0F, 0x11, 0x1A),
                         DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(new PreviewContent(), true);
        centreWithSize(1050, 720);
        setResizable(true, true);
        setResizeLimits(760, 560, 1600, 1100);
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }
};

class OrchestraLayoutPreviewApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "Orchestra Layout Preview"; }
    const juce::String getApplicationVersion() override { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override { return false; }

    void initialise(const juce::String&) override
    {
        window = std::make_unique<PreviewWindow>();
    }

    void shutdown() override { window.reset(); }
    void systemRequestedQuit() override { quit(); }

private:
    std::unique_ptr<PreviewWindow> window;
};

START_JUCE_APPLICATION(OrchestraLayoutPreviewApplication)
