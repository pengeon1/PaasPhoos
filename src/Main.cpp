#include <juce_gui_basics/juce_gui_basics.h>
#include "audio/AudioEngine.h"
#include "ui/MainComponent.h"
#include "common/LookAndFeelCustom.h"

namespace paasphoos
{

class MainWindow : public juce::DocumentWindow
{
public:
    MainWindow(juce::String name, AudioEngine& engine)
        : DocumentWindow(name,
                         juce::Colour(0xff121316),
                         DocumentWindow::allButtons),
          audioEngine(engine)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(new MainComponent(audioEngine), true);

       #if JUCE_IOS || JUCE_ANDROID
        setFullScreen(true);
       #else
        setResizable(true, true);
        setResizeLimits(800, 560, 2560, 1440);
        centreWithSize(getWidth(), getHeight());
       #endif

        setVisible(true);
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }

private:
    AudioEngine& audioEngine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};

class PaasPhoosApplication : public juce::JUCEApplication
{
public:
    PaasPhoosApplication() {}

    const juce::String getApplicationName() override       { return "PaasPhoos"; }
    const juce::String getApplicationVersion() override    { return "0.1.0-M1"; }
    bool moreThanOneInstanceAllowed() override             { return true; }

    void initialise(const juce::String& /*commandLine*/) override
    {
        // Set custom studio dark look and feel
        lookAndFeel = std::make_unique<DarkStudioLookAndFeel>();
        juce::LookAndFeel::setDefaultLookAndFeel(lookAndFeel.get());

        // Initialize Audio Engine
        audioEngine = std::make_unique<AudioEngine>();
        audioEngine->initialize();

        // Create main application window
        mainWindow = std::make_unique<MainWindow>(getApplicationName() + " - Remote Rehearsal Console", *audioEngine);
    }

    void shutdown() override
    {
        mainWindow = nullptr;
        if (audioEngine != nullptr)
        {
            audioEngine->shutdown();
            audioEngine = nullptr;
        }
        juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
        lookAndFeel = nullptr;
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted(const juce::String& /*commandLine*/) override
    {
    }

private:
    std::unique_ptr<DarkStudioLookAndFeel> lookAndFeel;
    std::unique_ptr<AudioEngine> audioEngine;
    std::unique_ptr<MainWindow> mainWindow;
};

} // namespace paasphoos

// JUCE Macro to generate application entry point
START_JUCE_APPLICATION(paasphoos::PaasPhoosApplication)
