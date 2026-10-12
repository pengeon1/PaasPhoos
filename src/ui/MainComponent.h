#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../audio/AudioEngine.h"
#include "AudioSettingsPanel.h"
#include "SessionPanel.h"
#include "MixerPanel.h"
#include "TunerComponent.h"
#include "MetronomeComponent.h"
#include "EffectsPanel.h"
#include "DrumPanel.h"
#include "RehearsalToolsPanel.h"

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
    void switchView(int viewIndex);

    AudioEngine& audioEngine;

    // Header Components
    juce::Label logoLabel;
    juce::Label subtitleLabel;
    juce::Label modeBadgeLabel;
    juce::Label cpuBadgeLabel;

    // Navigation Buttons
    juce::TextButton viewMixerBtn;
    juce::TextButton viewFxBtn;
    juce::TextButton viewDrumsBtn;
    juce::TextButton viewRehearsalBtn;
    int currentViewIndex{0};

    // Sub-panels
    SessionPanel sessionPanel;
    AudioSettingsPanel audioSettingsPanel;
    MetronomeComponent metronomeComponent;
    MixerPanel mixerPanel;
    TunerComponent tunerComponent;
    EffectsPanel effectsPanel;
    DrumPanel drumPanel;
    RehearsalToolsPanel rehearsalToolsPanel;

    // Status Bar
    juce::Label statusBarLabel;
    juce::Label mmcssStatusLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace paasphoos
