#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../audio/AudioEngine.h"

namespace paasphoos
{

class TunerComponent : public juce::Component,
                       public juce::Timer
{
public:
    explicit TunerComponent(AudioEngine& engine);
    ~TunerComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    AudioEngine& audioEngine;

    juce::Label headerLabel;
    juce::ComboBox channelSelectCombo;
    juce::ToggleButton enableToggle;

    TunerResult currentResult;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TunerComponent)
};

} // namespace paasphoos
