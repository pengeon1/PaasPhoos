#include <juce_gui_basics/juce_gui_basics.h>
#include "audio/AudioEngine.h"
#include "ui/MainComponent.h"
#include "common/LookAndFeelCustom.h"
#include "common/ThemeConfig.h"

namespace paasphoos
{

class MainWindow : public juce::DocumentWindow
{
public:
    MainWindow(juce::String name, AudioEngine& engine, bool introPreviewMode = false)
        : DocumentWindow(name,
                         juce::Colour(ThemeConfig::kBaseBackgroundHex),
                         DocumentWindow::allButtons),
          audioEngine(engine)
    {
        setUsingNativeTitleBar(true);

        // Set application window & taskbar thumbnail icon
        auto logo = Typography::getLogoImage();
        if (logo.isValid())
        {
            setIcon(logo);
        }

        setContentOwned(new MainComponent(audioEngine, introPreviewMode), true);

        setResizable(true, true);
        setResizeLimits(800, 560, 3840, 2160);

        if (introPreviewMode)
        {
            centreWithSize(1366, 768);
        }
        else
        {
            setFullScreen(true);
        }

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
    const juce::String getApplicationVersion() override    { return "0.3.0"; }
    bool moreThanOneInstanceAllowed() override             { return true; }

    void initialise(const juce::String& commandLine) override
    {
        bool isPreview = commandLine.contains("--intro-preview");
        bool isExportScreenshots = commandLine.contains("--export-screenshots");

        // Set custom studio dark look and feel
        lookAndFeel = std::make_unique<DarkStudioLookAndFeel>();
        juce::LookAndFeel::setDefaultLookAndFeel(lookAndFeel.get());

        // Initialize Audio Engine in background (never blocks intro)
        audioEngine = std::make_unique<AudioEngine>();
        audioEngine->initialize();

        if (isExportScreenshots)
        {
            auto comp = std::make_unique<MainComponent>(*audioEngine, false);
            auto outDir = juce::File::getCurrentWorkingDirectory().getChildFile("artifacts/screenshots");
            comp->exportScreenshots(outDir);
            juce::Logger::writeToLog("All requested verification screenshots exported to: " + outDir.getFullPathName());
            quit();
            return;
        }

        // Create main application window
        mainWindow = std::make_unique<MainWindow>(
            isPreview ? (getApplicationName() + " [Intro Preview Mode]")
                      : (getApplicationName() + " - Remote Rehearsal Console"),
            *audioEngine,
            isPreview
        );
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
