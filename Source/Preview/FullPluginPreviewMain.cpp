#include "../Plugin/PluginProcessor.h"
#include "../Plugin/PluginEditor.h"

class FullPluginPreviewContent final : public juce::Component
{
public:
    FullPluginPreviewContent()
    {
        processor = std::make_unique<OrchestralStudioReverbAudioProcessor>();
        editor.reset(dynamic_cast<OrchestralStudioReverbAudioProcessorEditor*>(
            processor->createEditor()));
        jassert(editor != nullptr);
        addAndMakeVisible(*editor);
        setSize(1000, 700);
    }

    void resized() override
    {
        if (editor != nullptr)
            editor->setBounds(getLocalBounds());
    }

private:
    std::unique_ptr<OrchestralStudioReverbAudioProcessor> processor;
    std::unique_ptr<OrchestralStudioReverbAudioProcessorEditor> editor;
};

class FullPluginPreviewWindow final : public juce::DocumentWindow
{
public:
    FullPluginPreviewWindow()
        : DocumentWindow("Orchestral Studio Reverb — Full UI Preview",
                         juce::Colour(0x0F, 0x11, 0x1A), DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(new FullPluginPreviewContent(), true);
        setResizable(true, true);
        centreWithSize(1000, 700);
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }
};

class FullPluginPreviewApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "Orchestral Studio Reverb UI Preview"; }
    const juce::String getApplicationVersion() override { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override { return false; }
    void initialise(const juce::String&) override { window = std::make_unique<FullPluginPreviewWindow>(); }
    void shutdown() override { window.reset(); }
    void systemRequestedQuit() override { quit(); }

private:
    std::unique_ptr<FullPluginPreviewWindow> window;
};

START_JUCE_APPLICATION(FullPluginPreviewApplication)
