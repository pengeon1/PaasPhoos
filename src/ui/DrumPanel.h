#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../audio/DrumEngine.h"
#include "../audio/DrumSequencer.h"

namespace paasphoos
{

class DrumPanel : public juce::Component,
                  public juce::Timer
{
public:
    DrumPanel(DrumEngine& engine, DrumSequencer& sequencer);
    ~DrumPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    bool keyPressed(const juce::KeyPress& key) override;

private:
    DrumEngine& drumEngine;
    DrumSequencer& drumSequencer;

    juce::Label headerLabel;
    juce::ToggleButton enableToggle;

    // Playback & Groove controls
    juce::TextButton playButton;
    juce::TextButton fillButton;
    juce::TextButton clearButton;

    juce::ComboBox grooveCombo;
    juce::ComboBox kitPresetCombo;

    juce::Slider swingSlider;
    juce::Label swingLabel;

    juce::Slider latencyCompSlider;
    juce::Label latencyCompLabel;

    juce::TextButton exportMidiButton;

    // Live Trigger Pads (8 Pads)
    std::array<juce::TextButton, DrumEngine::NUM_VOICES> padButtons;

    // Step Sequencer Matrix (8 Tracks x 16 Steps)
    std::array<juce::Label, DrumEngine::NUM_VOICES> trackLabels;
    std::array<std::array<juce::Button*, DrumSequencer::NUM_STEPS>, DrumEngine::NUM_VOICES> stepButtons;

    // Raw button objects allocated
    std::vector<std::unique_ptr<juce::TextButton>> stepButtonObjects;

    int lastDrawnStep{-1};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumPanel)
};

} // namespace paasphoos
