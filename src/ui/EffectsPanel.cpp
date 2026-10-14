#include "EffectsPanel.h"
#include "../common/ThemeConfig.h"
#include "../common/Typography.h"

namespace paasphoos
{

EffectsPanel::EffectsPanel(BuiltInFxChain& chain, PluginHostManager& host)
    : fxChain(chain),
      pluginHostManager(host)
{
    pluginHostManager.addChangeListener(this);

    headerLabel.setText("GUITAR & VOCAL FX RACK (DSP + VST3 HOST)", juce::dontSendNotification);
    headerLabel.setFont(Typography::h3());
    headerLabel.setColour(juce::Label::textColourId, juce::Colour(0xff3da5ff));
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

    // Helper to style knob labels
    auto setupLabel = [this](juce::Label& lbl, const juce::String& text, juce::Colour col)
    {
        lbl.setText(text, juce::dontSendNotification);
        lbl.setFont(Typography::labelCaps(10.5f));
        lbl.setJustificationType(juce::Justification::centred);
        lbl.setColour(juce::Label::textColourId, col);
        addAndMakeVisible(lbl);
    };

    // Helper to style rotary knobs
    auto setupKnob = [this](juce::Slider& s, juce::Colour fillCol, juce::Colour thumbCol)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 48, 14);
        s.setColour(juce::Slider::rotarySliderFillColourId, fillCol);
        s.setColour(juce::Slider::thumbColourId, thumbCol);
        s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        s.setColour(juce::Slider::textBoxTextColourId, juce::Colour(ThemeConfig::kTextPrimaryHex));
        addAndMakeVisible(s);
    };

    // ==========================================
    // Module 1: Noise Gate (Deep Violet / Indigo)
    // ==========================================
    const juce::Colour kGateCol(0xff6366f1);
    gateToggle.setButtonText("Gate");
    gateToggle.setToggleState(true, juce::dontSendNotification);
    gateToggle.onStateChange = [this]() { fxChain.setGateEnabled(gateToggle.getToggleState()); };
    addAndMakeVisible(gateToggle);

    setupLabel(gateThreshLabel, "THRESH", kGateCol.brighter(0.2f));
    setupKnob(gateThreshSlider, kGateCol, juce::Colour(0xffa5b4fc));
    gateThreshSlider.setRange(-70.0, -10.0, 1.0);
    gateThreshSlider.setValue(-48.0, juce::dontSendNotification);
    gateThreshSlider.textFromValueFunction = [](double v) { return juce::String((int)v) + " dB"; };
    gateThreshSlider.onValueChange = [this]() { fxChain.setGateThreshold((float)gateThreshSlider.getValue()); };

    // ==========================================
    // Module 2: Overdrive (Electric Royal Blue)
    // ==========================================
    const juce::Colour kDriveCol(0xff2563eb);
    driveToggle.setButtonText("Drive");
    driveToggle.setToggleState(false, juce::dontSendNotification);
    driveToggle.onStateChange = [this]() { fxChain.setDriveEnabled(driveToggle.getToggleState()); };
    addAndMakeVisible(driveToggle);

    setupLabel(driveAmountLabel, "DRIVE", kDriveCol.brighter(0.2f));
    setupKnob(driveAmountSlider, kDriveCol, juce::Colour(0xff60a5fa));
    driveAmountSlider.setRange(0.0, 10.0, 0.1);
    driveAmountSlider.setValue(3.5, juce::dontSendNotification);
    driveAmountSlider.textFromValueFunction = [](double v) { return juce::String(v, 1); };
    driveAmountSlider.onValueChange = [this]() { fxChain.setDriveAmount((float)driveAmountSlider.getValue()); };

    setupLabel(driveToneLabel, "TONE", kDriveCol.brighter(0.2f));
    setupKnob(driveToneSlider, juce::Colour(0xff1d4ed8), juce::Colour(0xff93c5fd));
    driveToneSlider.setRange(0.0, 1.0, 0.05);
    driveToneSlider.setValue(0.6, juce::dontSendNotification);
    driveToneSlider.textFromValueFunction = [](double v) { return juce::String((int)(v * 100)) + "%"; };
    driveToneSlider.onValueChange = [this]() { fxChain.setDriveTone((float)driveToneSlider.getValue()); };

    // ==========================================
    // Module 3: Compressor (Electric Blue)
    // ==========================================
    const juce::Colour kCompCol(0xff3da5ff);
    compToggle.setButtonText("Comp");
    compToggle.setToggleState(true, juce::dontSendNotification);
    compToggle.onStateChange = [this]() { fxChain.setCompEnabled(compToggle.getToggleState()); };
    addAndMakeVisible(compToggle);

    setupLabel(compThreshLabel, "THRESH", kCompCol.brighter(0.2f));
    setupKnob(compThreshSlider, kCompCol, juce::Colour(0xff38e1c6));
    compThreshSlider.setRange(-50.0, 0.0, 1.0);
    compThreshSlider.setValue(-18.0, juce::dontSendNotification);
    compThreshSlider.textFromValueFunction = [](double v) { return juce::String((int)v) + " dB"; };
    compThreshSlider.onValueChange = [this]() { fxChain.setCompThreshold((float)compThreshSlider.getValue()); };

    setupLabel(compMakeupLabel, "MAKEUP", kCompCol.brighter(0.2f));
    setupKnob(compMakeupSlider, juce::Colour(0xff00b0ff), juce::Colour(0xff80d8ff));
    compMakeupSlider.setRange(0.0, 18.0, 0.5);
    compMakeupSlider.setValue(3.0, juce::dontSendNotification);
    compMakeupSlider.textFromValueFunction = [](double v) { return "+" + juce::String((int)v) + " dB"; };
    compMakeupSlider.onValueChange = [this]() { fxChain.setCompMakeup((float)compMakeupSlider.getValue()); };

    // ==========================================
    // Module 4: 4-Band EQ (Sky Blue / Cyan / Indigo / Violet)
    // ==========================================
    const juce::Colour kEqCol(0xff38e1c6);
    eqToggle.setButtonText("4-Band EQ");
    eqToggle.setToggleState(true, juce::dontSendNotification);
    eqToggle.onStateChange = [this]() { fxChain.setEqEnabled(eqToggle.getToggleState()); };
    addAndMakeVisible(eqToggle);

    auto setupEqSlider = [this](juce::Slider& s, float val, juce::Colour c1, juce::Colour c2)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 44, 14);
        s.setColour(juce::Slider::rotarySliderFillColourId, c1);
        s.setColour(juce::Slider::thumbColourId, c2);
        s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        s.setColour(juce::Slider::textBoxTextColourId, juce::Colour(ThemeConfig::kTextPrimaryHex));
        s.setRange(-12.0, 12.0, 0.5);
        s.setValue(val, juce::dontSendNotification);
        s.textFromValueFunction = [](double v) { return juce::String::formatted("%+.0f dB", v); };
        addAndMakeVisible(s);
    };

    setupLabel(eqLowLabel, "LOW 80Hz", juce::Colour(0xff7dd3fc));
    setupEqSlider(eqLowSlider, 0.0f, juce::Colour(0xff0ea5e9), juce::Colour(0xff7dd3fc));
    eqLowSlider.onValueChange = [this]() { fxChain.setEqLowGain((float)eqLowSlider.getValue()); };

    setupLabel(eqMid1Label, "MID 500Hz", juce::Colour(0xff38e1c6));
    setupEqSlider(eqMid1Slider, 0.0f, juce::Colour(0xff0284c7), juce::Colour(0xff38e1c6));
    eqMid1Slider.onValueChange = [this]() { fxChain.setEqMid1Gain((float)eqMid1Slider.getValue()); };

    setupLabel(eqMid2Label, "MID 2.5k", juce::Colour(0xffa5b4fc));
    setupEqSlider(eqMid2Slider, 0.0f, juce::Colour(0xff6366f1), juce::Colour(0xffa5b4fc));
    eqMid2Slider.onValueChange = [this]() { fxChain.setEqMid2Gain((float)eqMid2Slider.getValue()); };

    setupLabel(eqHighLabel, "HIGH 8k", juce::Colour(0xffc084fc));
    setupEqSlider(eqHighSlider, 0.0f, juce::Colour(0xff7c5cff), juce::Colour(0xffc084fc));
    eqHighSlider.onValueChange = [this]() { fxChain.setEqHighGain((float)eqHighSlider.getValue()); };

    // ==========================================
    // Module 5: Reverb (Radiant Violet)
    // ==========================================
    const juce::Colour kReverbCol(0xff8b5cf6);
    reverbToggle.setButtonText("Reverb");
    reverbToggle.setToggleState(true, juce::dontSendNotification);
    reverbToggle.onStateChange = [this]() { fxChain.setReverbEnabled(reverbToggle.getToggleState()); };
    addAndMakeVisible(reverbToggle);

    setupLabel(reverbWetLabel, "MIX / WET", kReverbCol.brighter(0.2f));
    setupKnob(reverbWetSlider, kReverbCol, juce::Colour(0xffc084fc));
    reverbWetSlider.setRange(0.0, 1.0, 0.02);
    reverbWetSlider.setValue(0.25, juce::dontSendNotification);
    reverbWetSlider.textFromValueFunction = [](double v) { return juce::String((int)(v * 100)) + "%"; };
    reverbWetSlider.onValueChange = [this]() { fxChain.setReverbWet((float)reverbWetSlider.getValue()); };

    // ==========================================
    // VST3 Hosting Section
    // ==========================================
    vst3SectionLabel.setText("VST3 HOST:", juce::dontSendNotification);
    vst3SectionLabel.setFont(Typography::labelCaps(11.0f));
    vst3SectionLabel.setColour(juce::Label::textColourId, juce::Colour(0xff7c5cff));
    addAndMakeVisible(vst3SectionLabel);

    scanPluginsButton.setButtonText("SCAN VST3");
    scanPluginsButton.onClick = [this]() { pluginHostManager.scanPlugins(); };
    addAndMakeVisible(scanPluginsButton);

    pluginListCombo.setTextWhenNoChoicesAvailable("No VST3 scanned");
    pluginListCombo.setTextWhenNothingSelected("Select VST3 amp/fx...");
    addAndMakeVisible(pluginListCombo);

    loadPluginButton.setButtonText("LOAD");
    loadPluginButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff7c5cff));
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
    // Background fill
    g.fillAll(juce::Colour(0xff090a0d));

    // Outer container stroke
    g.setColour(juce::Colour(ThemeConfig::kBorderSubtleHex));
    g.drawRect(getLocalBounds(), 1);

    auto area = getLocalBounds().reduced(8, 4);
    area.removeFromTop(26); // Skip top bar
    area.removeFromTop(4);

    // Module Card backgrounds
    auto knobArea = area;
    int totalUnits = 1 + 2 + 2 + 4 + 1; // 10 units
    float unitW = (float)knobArea.getWidth() / (float)totalUnits;

    auto drawModuleCard = [&](float startUnit, float numUnits, const juce::Colour& accentCol)
    {
        auto cardRect = juce::Rectangle<float>(
            knobArea.getX() + startUnit * unitW + 2.0f,
            (float)knobArea.getY() + 1.0f,
            numUnits * unitW - 4.0f,
            (float)knobArea.getHeight() - 2.0f);

        // Subtle dark tinted background
        g.setColour(juce::Colour(ThemeConfig::kSurfaceHex).withAlpha(0.65f));
        g.fillRoundedRectangle(cardRect, 6.0f);

        // Accent top border line
        g.setColour(accentCol.withAlpha(0.5f));
        g.drawRoundedRectangle(cardRect, 6.0f, 1.0f);

        // Subtle glowing header bar
        g.setColour(accentCol.withAlpha(0.12f));
        g.fillRoundedRectangle(cardRect.withHeight(18.0f), 6.0f);
    };

    drawModuleCard(0.0f, 1.0f, juce::Colour(0xff6366f1)); // Gate (Indigo)
    drawModuleCard(1.0f, 2.0f, juce::Colour(0xff2563eb)); // Drive (Royal Blue)
    drawModuleCard(3.0f, 2.0f, juce::Colour(0xff3da5ff)); // Comp (Electric Blue)
    drawModuleCard(5.0f, 4.0f, juce::Colour(0xff38e1c6)); // EQ (Cyan / Blue)
    drawModuleCard(9.0f, 1.0f, juce::Colour(0xff8b5cf6)); // Reverb (Radiant Violet)
}

void EffectsPanel::resized()
{
    auto area = getLocalBounds().reduced(8, 4);

    // Row 1: Header + Presets + VST3 quick bar
    auto topRow = area.removeFromTop(24);
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

    area.removeFromTop(6);

    // Row 2: DSP Knobs Rack
    auto knobArea = area;
    int totalUnits = 1 + 2 + 2 + 4 + 1; // 10 units
    int unitW = knobArea.getWidth() / totalUnits;

    // Helper to position a knob and its label inside a column area
    auto layoutKnobWithLabel = [](juce::Rectangle<int> col, juce::Label& lbl, juce::Slider& s)
    {
        int labelH = 14;
        lbl.setBounds(col.removeFromTop(labelH));
        s.setBounds(col);
    };

    // Module 1: Gate (1 unit)
    auto colGate = knobArea.removeFromLeft(unitW).reduced(4, 2);
    gateToggle.setBounds(colGate.removeFromTop(16));
    colGate.removeFromTop(2);
    layoutKnobWithLabel(colGate, gateThreshLabel, gateThreshSlider);

    // Module 2: Drive (2 units)
    auto colDrive = knobArea.removeFromLeft(unitW * 2).reduced(4, 2);
    driveToggle.setBounds(colDrive.removeFromTop(16));
    colDrive.removeFromTop(2);
    int driveW = colDrive.getWidth() / 2;
    layoutKnobWithLabel(colDrive.removeFromLeft(driveW), driveAmountLabel, driveAmountSlider);
    layoutKnobWithLabel(colDrive, driveToneLabel, driveToneSlider);

    // Module 3: Compressor (2 units)
    auto colComp = knobArea.removeFromLeft(unitW * 2).reduced(4, 2);
    compToggle.setBounds(colComp.removeFromTop(16));
    colComp.removeFromTop(2);
    int compW = colComp.getWidth() / 2;
    layoutKnobWithLabel(colComp.removeFromLeft(compW), compThreshLabel, compThreshSlider);
    layoutKnobWithLabel(colComp, compMakeupLabel, compMakeupSlider);

    // Module 4: 4-Band EQ (4 units)
    auto colEq = knobArea.removeFromLeft(unitW * 4).reduced(4, 2);
    eqToggle.setBounds(colEq.removeFromTop(16));
    colEq.removeFromTop(2);
    int eqW = colEq.getWidth() / 4;
    layoutKnobWithLabel(colEq.removeFromLeft(eqW), eqLowLabel, eqLowSlider);
    layoutKnobWithLabel(colEq.removeFromLeft(eqW), eqMid1Label, eqMid1Slider);
    layoutKnobWithLabel(colEq.removeFromLeft(eqW), eqMid2Label, eqMid2Slider);
    layoutKnobWithLabel(colEq, eqHighLabel, eqHighSlider);

    // Module 5: Reverb (1 unit)
    auto colReverb = knobArea.reduced(4, 2);
    reverbToggle.setBounds(colReverb.removeFromTop(16));
    colReverb.removeFromTop(2);
    layoutKnobWithLabel(colReverb, reverbWetLabel, reverbWetSlider);
}

} // namespace paasphoos
