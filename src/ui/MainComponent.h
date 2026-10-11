#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../audio/AudioEngine.h"
#include "AudioSettingsPanel.h"
#include "SessionPanel.h"
#include "MixerPanel.h"
#include "TunerComponent.h"
#include "MetronomeComponent.h"

namespace paasphoos
{

class MainComponent : public juce::Component,
                      public juce::Timer
{
public:
    explicit MainComponent(AudioEngine& engine);
    ~MainComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    AudioEngine& audioEngine;

    // Header Components
    juce::Label logoLabel;
    juce::Label subtitleLabel;
    juce::Label modeBadgeLabel;
    juce::Label cpuBadgeLabel;

    // Sub-panels
    SessionPanel sessionPanel;
    AudioSettingsPanel audioSettingsPanel;
    MetronomeComponent metronomeComponent;
    MixerPanel mixerPanel;
    TunerComponent tunerComponent;

    // Status Bar
    juce::Label statusBarLabel;
    juce::Label mmcssStatusLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace paasphoos
