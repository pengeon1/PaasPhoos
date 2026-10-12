#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../audio/BuiltInFxChain.h"
#include "../audio/PluginHostManager.h"

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
    juce::ToggleButton gateToggle;
    juce::Slider gateThreshSlider;

    juce::ToggleButton driveToggle;
    juce::Slider driveAmountSlider;
    juce::Slider driveToneSlider;

    juce::ToggleButton compToggle;
    juce::Slider compThreshSlider;
    juce::Slider compMakeupSlider;

    juce::ToggleButton eqToggle;
    juce::Slider eqLowSlider;
    juce::Slider eqMid1Slider;
    juce::Slider eqMid2Slider;
    juce::Slider eqHighSlider;

    juce::ToggleButton reverbToggle;
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
