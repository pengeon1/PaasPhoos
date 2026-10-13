#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../audio/BuiltInFxChain.h"
#include "../audio/PluginHostManager.h"
#include "../common/Typography.h"

namespace paasphoos
{

class EffectsPanel : public juce::Component,
                     public juce::ChangeListener
{
public:
    EffectsPanel(BuiltInFxChain& fxChain, PluginHostManager& hostManager);
    ~EffectsPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

private:
    void refreshPluginList();

    BuiltInFxChain& fxChain;
    PluginHostManager& pluginHostManager;

    juce::Label headerLabel;
    juce::ToggleButton masterFxToggle;
    juce::ComboBox presetCombo;

    // Built-in DSP Controls
    // Module 1: Noise Gate
    juce::ToggleButton gateToggle;
    juce::Label gateThreshLabel;
    juce::Slider gateThreshSlider;

    // Module 2: Overdrive / Distortion
    juce::ToggleButton driveToggle;
    juce::Label driveAmountLabel;
    juce::Slider driveAmountSlider;
    juce::Label driveToneLabel;
    juce::Slider driveToneSlider;

    // Module 3: Compressor
    juce::ToggleButton compToggle;
    juce::Label compThreshLabel;
    juce::Slider compThreshSlider;
    juce::Label compMakeupLabel;
    juce::Slider compMakeupSlider;

    // Module 4: 4-Band Parametric EQ
    juce::ToggleButton eqToggle;
    juce::Label eqLowLabel;
    juce::Slider eqLowSlider;
    juce::Label eqMid1Label;
    juce::Slider eqMid1Slider;
    juce::Label eqMid2Label;
    juce::Slider eqMid2Slider;
    juce::Label eqHighLabel;
    juce::Slider eqHighSlider;

    // Module 5: Studio Reverb
    juce::ToggleButton reverbToggle;
    juce::Label reverbWetLabel;
    juce::Slider reverbWetSlider;

    // VST3 Hosting Controls
    juce::Label vst3SectionLabel;
    juce::TextButton scanPluginsButton;
    juce::ComboBox pluginListCombo;
    juce::TextButton loadPluginButton;
    juce::TextButton openPluginGuiButton;
    juce::TextButton unloadPluginButton;
    juce::ToggleButton sendWetOnlyToggle;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectsPanel)
};

} // namespace paasphoos
