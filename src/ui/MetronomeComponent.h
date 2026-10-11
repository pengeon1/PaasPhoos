#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../audio/AudioEngine.h"

namespace paasphoos
{

class MetronomeComponent : public juce::Component,
                           public juce::Timer
{
public:
    explicit MetronomeComponent(AudioEngine& engine);
    ~MetronomeComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    void handleTapTempo();

    AudioEngine& audioEngine;

    juce::Label headerLabel;
    juce::TextButton playButton;

    juce::Label bpmDisplayLabel;
    juce::Slider bpmSlider;
    juce::TextButton minusBpmButton;
    juce::TextButton plusBpmButton;

    juce::TextButton tapTempoButton;
    juce::ComboBox timeSignatureCombo;

    juce::Label volumeLabel;
    juce::Slider volumeSlider;

    std::vector<juce::int64> tapTimes;

    int lastDrawnBeat = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MetronomeComponent)
};

} // namespace paasphoos
