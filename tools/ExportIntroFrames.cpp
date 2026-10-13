#include <juce_gui_basics/juce_gui_basics.h>
#include "../src/ui/IntroScreenComponent.h"
#include "../src/common/LookAndFeelCustom.h"
#include <iostream>

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI guiInit;
    auto lookAndFeel = std::make_unique<paasphoos::DarkStudioLookAndFeel>();
    juce::LookAndFeel::setDefaultLookAndFeel(lookAndFeel.get());

    std::cout << "Starting Intro Frame Export Tool...\n";

    auto outputDir = juce::File::getCurrentWorkingDirectory().getChildFile("artifacts/intro_frames");
    outputDir.createDirectory();

    paasphoos::IntroScreenComponent intro;
    const int width = 1366;
    const int height = 768;
    intro.addToDesktop(0);
    intro.setVisible(true);

    auto exportFrame = [&](float timestamp, const juce::String& filename)
    {
        auto img = intro.captureFrameAtTime(timestamp, width, height);
        auto outFile = outputDir.getChildFile(filename);
        if (outFile.existsAsFile())
            outFile.deleteFile();

        juce::FileOutputStream stream(outFile);
        if (stream.openedOk())
        {
            juce::PNGImageFormat png;
            png.writeImageToStream(img, stream);
            std::cout << "Saved: " << outFile.getFullPathName().toStdString() << "\n";
        }
        else
        {
            std::cerr << "Failed to write: " << outFile.getFullPathName().toStdString() << "\n";
        }
    };

    // Export key moments for streamlined PaasPhoos intro
    exportFrame(0.4f, "frame_t0_4s_waveform_pulse.png");
    exportFrame(1.1f, "frame_t1_1s_wordmark_reveal.png");
    exportFrame(1.8f, "frame_t1_8s_tagline_reveal.png");
    exportFrame(2.4f, "frame_t2_4s_glide_transition.png");
    exportFrame(2.7f, "frame_t2_7s_name_input_screen.png");

    std::cout << "All streamlined intro frames successfully exported to: " << outputDir.getFullPathName().toStdString() << "\n";
    return 0;
}
