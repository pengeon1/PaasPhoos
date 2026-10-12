#include "EffectsPanel.h"

namespace paasphoos
{

EffectsPanel::EffectsPanel(BuiltInFxChain& chain, PluginHostManager& host)
    : fxChain(chain),
      pluginHostManager(host)
{
    pluginHostManager.addChangeListener(this);

    headerLabel.setText("GUITAR & VOCAL FX RACK (DSP + VST3 HOST)", juce::dontSendNotification);
    headerLabel.setFont(juce::FontOptions(12.5f, juce::Font::bold));
    headerLabel.setColour(juce::Label::textColourId, juce::Colour(0xff38bdf8));
    addAndMakeVisible(headerLabel);

    masterFxToggle.setButtonText("FX ON");
    masterFxToggle.setToggleState(true, juce::dontSendNotification);
    masterFxToggle.onStateChange = [this]() { fxChain.setEnabled(masterFxToggle.getToggleState()); };
    addAndMakeVisible(masterFxToggle);

    // Presets
    presetCombo.addItem("Preset: Acoustic Clean", 1);
    presetCombo.addItem("Preset: Crunch Rhythm", 2);
    presetCombo.addItem("Preset: High Gain Lead", 3);
    presetCombo.addItem("Preset: Vocal Warm", 4);
    presetCombo.addItem("Preset: Vocal Bright Hall", 5);
    presetCombo.setSelectedId(1, juce::dontSendNotification);
    presetCombo.onChange = [this]()
    {
        int id = presetCombo.getSelectedId();
        if (id == 1) fxChain.loadPreset(FxPreset::CLEAN_ACOUSTIC);
        else if (id == 2) fxChain.loadPreset(FxPreset::CRUNCH_RHYTHM);
        else if (id == 3) fxChain.loadPreset(FxPreset::HIGH_GAIN_LEAD);
        else if (id == 4) fxChain.loadPreset(FxPreset::VOCAL_WARM);
        else if (id == 5) fxChain.loadPreset(FxPreset::VOCAL_BRIGHT_HALL);
    };
    addAndMakeVisible(presetCombo);

    // Gate
    gateToggle.setButtonText("Gate");
    gateToggle.setToggleState(true, juce::dontSendNotification);
    gateToggle.onStateChange = [this]() { fxChain.setGateEnabled(gateToggle.getToggleState()); };
    addAndMakeVisible(gateToggle);

    gateThreshSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    gateThreshSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 14);
    gateThreshSlider.setRange(-70.0, -10.0, 1.0);
    gateThreshSlider.setValue(-48.0, juce::dontSendNotification);
    gateThreshSlider.textFromValueFunction = [](double v) { return juce::String((int)v) + "dB"; };
    gateThreshSlider.onValueChange = [this]() { fxChain.setGateThreshold((float)gateThreshSlider.getValue()); };
    addAndMakeVisible(gateThreshSlider);

    // Drive
    driveToggle.setButtonText("Drive");
    driveToggle.setToggleState(false, juce::dontSendNotification);
    driveToggle.onStateChange = [this]() { fxChain.setDriveEnabled(driveToggle.getToggleState()); };
    addAndMakeVisible(driveToggle);

    driveAmountSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    driveAmountSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 14);
    driveAmountSlider.setRange(0.0, 10.0, 0.1);
    driveAmountSlider.setValue(3.5, juce::dontSendNotification);
    driveAmountSlider.onValueChange = [this]() { fxChain.setDriveAmount((float)driveAmountSlider.getValue()); };
    addAndMakeVisible(driveAmountSlider);

    driveToneSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    driveToneSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 14);
    driveToneSlider.setRange(0.0, 1.0, 0.05);
    driveToneSlider.setValue(0.6, juce::dontSendNotification);
    driveToneSlider.onValueChange = [this]() { fxChain.setDriveTone((float)driveToneSlider.getValue()); };
    addAndMakeVisible(driveToneSlider);

    // Compressor
    compToggle.setButtonText("Comp");
    compToggle.setToggleState(true, juce::dontSendNotification);
    compToggle.onStateChange = [this]() { fxChain.setCompEnabled(compToggle.getToggleState()); };
    addAndMakeVisible(compToggle);

    compThreshSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    compThreshSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 14);
    compThreshSlider.setRange(-50.0, 0.0, 1.0);
    compThreshSlider.setValue(-18.0, juce::dontSendNotification);
    compThreshSlider.textFromValueFunction = [](double v) { return juce::String((int)v) + "dB"; };
    compThreshSlider.onValueChange = [this]() { fxChain.setCompThreshold((float)compThreshSlider.getValue()); };
    addAndMakeVisible(compThreshSlider);

    compMakeupSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    compMakeupSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 14);
    compMakeupSlider.setRange(0.0, 18.0, 0.5);
    compMakeupSlider.setValue(3.0, juce::dontSendNotification);
    compMakeupSlider.textFromValueFunction = [](double v) { return "+" + juce::String((int)v) + "dB"; };
    compMakeupSlider.onValueChange = [this]() { fxChain.setCompMakeup((float)compMakeupSlider.getValue()); };
    addAndMakeVisible(compMakeupSlider);

    // EQ
    eqToggle.setButtonText("4-Band EQ");
    eqToggle.setToggleState(true, juce::dontSendNotification);
    eqToggle.onStateChange = [this]() { fxChain.setEqEnabled(eqToggle.getToggleState()); };
    addAndMakeVisible(eqToggle);

    auto setupEqKnob = [this](juce::Slider& s, float val)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 36, 14);
        s.setRange(-12.0, 12.0, 0.5);
        s.setValue(val, juce::dontSendNotification);
        s.textFromValueFunction = [](double v) { return juce::String::formatted("%+.0f", v); };
        addAndMakeVisible(s);
    };

    setupEqKnob(eqLowSlider, 0.0f);
    eqLowSlider.onValueChange = [this]() { fxChain.setEqLowGain((float)eqLowSlider.getValue()); };
    setupEqKnob(eqMid1Slider, 0.0f);
    eqMid1Slider.onValueChange = [this]() { fxChain.setEqMid1Gain((float)eqMid1Slider.getValue()); };
    setupEqKnob(eqMid2Slider, 0.0f);
    eqMid2Slider.onValueChange = [this]() { fxChain.setEqMid2Gain((float)eqMid2Slider.getValue()); };
    setupEqKnob(eqHighSlider, 0.0f);
    eqHighSlider.onValueChange = [this]() { fxChain.setEqHighGain((float)eqHighSlider.getValue()); };

    // Reverb
    reverbToggle.setButtonText("Reverb");
    reverbToggle.setToggleState(true, juce::dontSendNotification);
    reverbToggle.onStateChange = [this]() { fxChain.setReverbEnabled(reverbToggle.getToggleState()); };
    addAndMakeVisible(reverbToggle);

    reverbWetSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    reverbWetSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 14);
    reverbWetSlider.setRange(0.0, 1.0, 0.02);
    reverbWetSlider.setValue(0.25, juce::dontSendNotification);
    reverbWetSlider.textFromValueFunction = [](double v) { return juce::String((int)(v * 100)) + "%"; };
    reverbWetSlider.onValueChange = [this]() { fxChain.setReverbWet((float)reverbWetSlider.getValue()); };
    addAndMakeVisible(reverbWetSlider);

    // VST3 Hosting Section
    vst3SectionLabel.setText("VST3 HOST:", juce::dontSendNotification);
    vst3SectionLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    vst3SectionLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa5b4fc));
    addAndMakeVisible(vst3SectionLabel);

    scanPluginsButton.setButtonText("SCAN VST3");
    scanPluginsButton.onClick = [this]() { pluginHostManager.scanPlugins(); };
    addAndMakeVisible(scanPluginsButton);

    pluginListCombo.setTextWhenNoChoicesAvailable("No VST3 scanned");
    pluginListCombo.setTextWhenNothingSelected("Select VST3 amp/fx...");
    addAndMakeVisible(pluginListCombo);

    loadPluginButton.setButtonText("LOAD");
    loadPluginButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff0284c7));
    loadPluginButton.onClick = [this]()
    {
        int idx = pluginListCombo.getSelectedId() - 1;
        pluginHostManager.loadPlugin(idx);
    };
    addAndMakeVisible(loadPluginButton);

    openPluginGuiButton.setButtonText("OPEN GUI");
    openPluginGuiButton.onClick = [this]() { pluginHostManager.openPluginWindow(); };
    addAndMakeVisible(openPluginGuiButton);

    unloadPluginButton.setButtonText("UNLOAD");
    unloadPluginButton.onClick = [this]() { pluginHostManager.unloadPlugin(); };
    addAndMakeVisible(unloadPluginButton);

    sendWetOnlyToggle.setButtonText("Send Wet Only");
    sendWetOnlyToggle.setToggleState(true, juce::dontSendNotification);
    sendWetOnlyToggle.onStateChange = [this]() { fxChain.setSendWetOnly(sendWetOnlyToggle.getToggleState()); };
    addAndMakeVisible(sendWetOnlyToggle);
}

EffectsPanel::~EffectsPanel()
{
    pluginHostManager.removeChangeListener(this);
}

void EffectsPanel::changeListenerCallback(juce::ChangeBroadcaster* /*source*/)
{
    refreshPluginList();
}

void EffectsPanel::refreshPluginList()
{
    auto& list = pluginHostManager.getPluginList();
    pluginListCombo.clear();
    for (int i = 0; i < list.getNumTypes(); ++i)
    {
        auto desc = list.getType(i);
        pluginListCombo.addItem(desc->name + " (" + desc->manufacturerName + ")", i + 1);
    }
}

void EffectsPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff121419));
    g.setColour(juce::Colour(0xff27272a));
    g.drawRect(getLocalBounds(), 1);
}

void EffectsPanel::resized()
{
    auto area = getLocalBounds().reduced(8, 4);

    // Row 1: Header + Presets + VST3 quick bar
    auto topRow = area.removeFromTop(22);
    headerLabel.setBounds(topRow.removeFromLeft(270));
    masterFxToggle.setBounds(topRow.removeFromLeft(75));
    presetCombo.setBounds(topRow.removeFromLeft(150));
    topRow.removeFromLeft(8);

    vst3SectionLabel.setBounds(topRow.removeFromLeft(70));
    scanPluginsButton.setBounds(topRow.removeFromLeft(75));
    topRow.removeFromLeft(4);
    pluginListCombo.setBounds(topRow.removeFromLeft(160));
    topRow.removeFromLeft(4);
    loadPluginButton.setBounds(topRow.removeFromLeft(50));
    topRow.removeFromLeft(4);
    openPluginGuiButton.setBounds(topRow.removeFromLeft(70));
    topRow.removeFromLeft(4);
    unloadPluginButton.setBounds(topRow.removeFromLeft(65));
    topRow.removeFromLeft(8);
    sendWetOnlyToggle.setBounds(topRow);

    area.removeFromTop(4);

    // Row 2: DSP Knobs Rack
    auto knobArea = area;
    int colW = knobArea.getWidth() / 8;

    // Gate
    auto col0 = knobArea.removeFromLeft(colW).reduced(2);
    gateToggle.setBounds(col0.removeFromTop(18));
    gateThreshSlider.setBounds(col0);

    // Drive (Amount & Tone)
    auto col1 = knobArea.removeFromLeft(colW).reduced(2);
    driveToggle.setBounds(col1.removeFromTop(18));
    driveAmountSlider.setBounds(col1);

    auto col2 = knobArea.removeFromLeft(colW).reduced(2);
    driveToneSlider.setBounds(col2);

    // Comp (Thresh & Makeup)
    auto col3 = knobArea.removeFromLeft(colW).reduced(2);
    compToggle.setBounds(col3.removeFromTop(18));
    compThreshSlider.setBounds(col3);

    auto col4 = knobArea.removeFromLeft(colW).reduced(2);
    compMakeupSlider.setBounds(col4);

    // 4-Band EQ (Low & Mid1)
    auto col5 = knobArea.removeFromLeft(colW).reduced(2);
    eqToggle.setBounds(col5.removeFromTop(18));
    eqLowSlider.setBounds(col5.removeFromLeft(col5.getWidth() / 2));
    eqMid1Slider.setBounds(col5);

    // EQ (Mid2 & High)
    auto col6 = knobArea.removeFromLeft(colW).reduced(2);
    eqMid2Slider.setBounds(col6.removeFromLeft(col6.getWidth() / 2));
    eqHighSlider.setBounds(col6);

    // Reverb
    auto col7 = knobArea.reduced(2);
    reverbToggle.setBounds(col7.removeFromTop(18));
    reverbWetSlider.setBounds(col7);
}

} // namespace paasphoos
