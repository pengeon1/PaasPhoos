#include "DrumPanel.h"
#include "../common/Typography.h"

namespace paasphoos
{

DrumPanel::DrumPanel(DrumEngine& engine, DrumSequencer& sequencer)
    : drumEngine(engine),
      drumSequencer(sequencer)
{
    setWantsKeyboardFocus(true);

    headerLabel.setText("DRUM MACHINE & SYNTH ENGINE (M5)", juce::dontSendNotification);
    headerLabel.setFont(Typography::h3());
    headerLabel.setColour(juce::Label::textColourId, juce::Colour(0xffffaa33));
    addAndMakeVisible(headerLabel);

    enableToggle.setButtonText("DRUMS ON");
    enableToggle.setToggleState(false, juce::dontSendNotification);
    enableToggle.onStateChange = [this]()
    {
        bool on = enableToggle.getToggleState();
        drumSequencer.setPlaying(on);
        playButton.setButtonText(on ? "PAUSE" : "PLAY");
    };
    addAndMakeVisible(enableToggle);

    playButton.setButtonText("PLAY");
    playButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff6600));
    playButton.onClick = [this]()
    {
        bool playing = !drumSequencer.getPlaying();
        drumSequencer.setPlaying(playing);
        enableToggle.setToggleState(playing, juce::dontSendNotification);
        playButton.setButtonText(playing ? "PAUSE" : "PLAY");
        playButton.setColour(juce::TextButton::buttonColourId, playing ? juce::Colour(0xffcc4400) : juce::Colour(0xffff6600));
    };
    addAndMakeVisible(playButton);

    fillButton.setButtonText("FILL");
    fillButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff7700));
    fillButton.onClick = [this]() { drumSequencer.triggerFill(); };
    addAndMakeVisible(fillButton);

    clearButton.setButtonText("CLR");
    clearButton.onClick = [this]() { drumSequencer.clearPattern(); };
    addAndMakeVisible(clearButton);

    // Groove Presets
    grooveCombo.addItem("Rock 4-on-Floor", 1);
    grooveCombo.addItem("Pop Backbeat", 2);
    grooveCombo.addItem("Metal Double-Bass", 3);
    grooveCombo.addItem("Funk Ghost Notes", 4);
    grooveCombo.addItem("EDM House", 5);
    grooveCombo.addItem("Bollywood Teen-Taal", 6);
    grooveCombo.setSelectedId(1, juce::dontSendNotification);
    grooveCombo.onChange = [this]()
    {
        int id = grooveCombo.getSelectedId();
        drumSequencer.generateGroove(static_cast<GrooveStyle>(id - 1));
    };
    addAndMakeVisible(grooveCombo);

    // Kit Presets
    kitPresetCombo.addItem("Kit: Acoustic Rock", 1);
    kitPresetCombo.addItem("Kit: Heavy Metal", 2);
    kitPresetCombo.addItem("Kit: Vintage 808", 3);
    kitPresetCombo.addItem("Kit: Electronic 909", 4);
    kitPresetCombo.addItem("Kit: Bollywood Dholak", 5);
    kitPresetCombo.setSelectedId(1, juce::dontSendNotification);
    kitPresetCombo.onChange = [this]()
    {
        int id = kitPresetCombo.getSelectedId();
        if (id == 1) drumEngine.loadKitPreset(DrumKitPreset::ACOUSTIC_ROCK);
        else if (id == 2) drumEngine.loadKitPreset(DrumKitPreset::HEAVY_METAL);
        else if (id == 3) drumEngine.loadKitPreset(DrumKitPreset::VINTAGE_808);
        else if (id == 4) drumEngine.loadKitPreset(DrumKitPreset::ELECTRONIC_909);
        else if (id == 5) drumEngine.loadKitPreset(DrumKitPreset::BOLLYWOOD_DHOLAK);
    };
    addAndMakeVisible(kitPresetCombo);

    // Swing
    swingLabel.setText("Swing:", juce::dontSendNotification);
    swingLabel.setFont(Typography::labelCaps(11.0f));
    swingLabel.setColour(juce::Label::textColourId, juce::Colour(0xff94a3b8));
    addAndMakeVisible(swingLabel);

    swingSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    swingSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 36, 14);
    swingSlider.setRange(0.0, 50.0, 1.0);
    swingSlider.setValue(0.0, juce::dontSendNotification);
    swingSlider.textFromValueFunction = [](double v) { return juce::String((int)v) + "%"; };
    swingSlider.onValueChange = [this]()
    {
        drumSequencer.setSwing(static_cast<float>(swingSlider.getValue() / 100.0));
    };
    addAndMakeVisible(swingSlider);

    // Latency Compensation
    latencyCompLabel.setText("Drum Offset:", juce::dontSendNotification);
    latencyCompLabel.setFont(Typography::labelCaps(11.0f));
    latencyCompLabel.setColour(juce::Label::textColourId, juce::Colour(0xff94a3b8));
    addAndMakeVisible(latencyCompLabel);

    latencyCompSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    latencyCompSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 46, 14);
    latencyCompSlider.setRange(-40.0, 40.0, 1.0);
    latencyCompSlider.setValue(0.0, juce::dontSendNotification);
    latencyCompSlider.textFromValueFunction = [](double v) { return juce::String::formatted("%+.0fms", v); };
    latencyCompSlider.onValueChange = [this]()
    {
        drumSequencer.setLatencyCompensationMs(static_cast<float>(latencyCompSlider.getValue()));
    };
    addAndMakeVisible(latencyCompSlider);

    // Export MIDI
    exportMidiButton.setButtonText("EXPORT MIDI");
    exportMidiButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff6600));
    exportMidiButton.onClick = [this]()
    {
        juce::File exportFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile("PaasPhoos_Drums.mid");
        if (drumSequencer.exportMidiFile(exportFile))
        {
            exportMidiButton.setButtonText("EXPORTED!");
        }
    };
    addAndMakeVisible(exportMidiButton);

    // Initialize 8 Live Trigger Pads
    const char* hotkeys[8] = { "KICK [Z]", "SNARE [X]", "HH CLS [C]", "HH OPN [V]",
                               "LO TOM [A]", "HI TOM [S]", "CLAP [D]", "DHOLAK [F]" };
    for (int i = 0; i < DrumEngine::NUM_VOICES; ++i)
    {
        padButtons[i].setButtonText(hotkeys[i]);
        padButtons[i].setColour(juce::TextButton::buttonColourId, juce::Colour(0xff181a22));
        padButtons[i].onClick = [this, i]()
        {
            drumEngine.triggerVoice(static_cast<DrumSound>(i), 0.9f);
        };
        addAndMakeVisible(padButtons[i]);
    }

    // Initialize 8 x 16 Step Matrix
    const char* shortNames[8] = { "KICK", "SNAR", "H-CL", "H-OP", "LTOM", "HTOM", "CLAP", "PERC" };
    stepButtonObjects.reserve(DrumEngine::NUM_VOICES * DrumSequencer::NUM_STEPS);

    for (int t = 0; t < DrumEngine::NUM_VOICES; ++t)
    {
        trackLabels[t].setText(shortNames[t], juce::dontSendNotification);
        trackLabels[t].setFont(Typography::labelCaps(10.0f));
        trackLabels[t].setColour(juce::Label::textColourId, juce::Colour(0xff94a3b8));
        addAndMakeVisible(trackLabels[t]);

        for (int s = 0; s < DrumSequencer::NUM_STEPS; ++s)
        {
            auto btn = std::make_unique<juce::TextButton>();
            btn->setButtonText("");
            btn->setClickingTogglesState(true);
            btn->setColour(juce::TextButton::buttonColourId, (s % 4 == 0) ? juce::Colour(0xff1c1e26) : juce::Colour(0xff101116));
            btn->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffff6600)); // Flame Orange LED
            btn->onClick = [this, t, s]()
            {
                drumSequencer.toggleStep(t, s);
            };
            stepButtons[t][s] = btn.get();
            addAndMakeVisible(*btn);
            stepButtonObjects.push_back(std::move(btn));
        }
    }

    startTimerHz(25); // 25 FPS UI step refresh
}

DrumPanel::~DrumPanel()
{
    stopTimer();
}

bool DrumPanel::keyPressed(const juce::KeyPress& key)
{
    juce::juce_wchar c = std::tolower(key.getTextCharacter());
    if (c == 'z') { drumEngine.triggerVoice(DrumSound::KICK, 0.95f); return true; }
    if (c == 'x') { drumEngine.triggerVoice(DrumSound::SNARE, 0.95f); return true; }
    if (c == 'c') { drumEngine.triggerVoice(DrumSound::HIHAT_CLOSED, 0.85f); return true; }
    if (c == 'v') { drumEngine.triggerVoice(DrumSound::HIHAT_OPEN, 0.85f); return true; }
    if (c == 'a') { drumEngine.triggerVoice(DrumSound::TOM_LOW, 0.90f); return true; }
    if (c == 's') { drumEngine.triggerVoice(DrumSound::TOM_HIGH, 0.90f); return true; }
    if (c == 'd') { drumEngine.triggerVoice(DrumSound::CLAP, 0.90f); return true; }
    if (c == 'f') { drumEngine.triggerVoice(DrumSound::PERCUSSION_DHOLAK, 0.95f); return true; }
    return false;
}

void DrumPanel::timerCallback()
{
    // Update step toggle states from sequencer
    for (int t = 0; t < DrumEngine::NUM_VOICES; ++t)
    {
        for (int s = 0; s < DrumSequencer::NUM_STEPS; ++s)
        {
            float vel = drumSequencer.getStep(t, s);
            stepButtons[t][s]->setToggleState(vel > 0.05f, juce::dontSendNotification);
        }
    }

    int currentStep = drumSequencer.getCurrentStep();
    if (currentStep != lastDrawnStep)
    {
        lastDrawnStep = currentStep;
        repaint();
    }
}

void DrumPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff090a0d));
    g.setColour(juce::Colour(0xff22252e));
    g.drawRect(getLocalBounds(), 1);

    // Draw active step playback cursor
    if (drumSequencer.getPlaying() && lastDrawnStep >= 0 && lastDrawnStep < DrumSequencer::NUM_STEPS)
    {
        auto firstBtn = stepButtons[0][lastDrawnStep]->getBounds();
        auto lastBtn = stepButtons[DrumEngine::NUM_VOICES - 1][lastDrawnStep]->getBounds();

        g.setColour(juce::Colour(0x55ff6600));
        g.fillRoundedRectangle((float)firstBtn.getX() - 1, (float)firstBtn.getY() - 2,
                               (float)firstBtn.getWidth() + 2, (float)(lastBtn.getBottom() - firstBtn.getY() + 4), 2.0f);
    }
}

void DrumPanel::resized()
{
    auto area = getLocalBounds().reduced(8, 6);

    // Top Header & Controls Row
    auto topRow = area.removeFromTop(22);
    headerLabel.setBounds(topRow.removeFromLeft(250));
    enableToggle.setBounds(topRow.removeFromLeft(95));
    playButton.setBounds(topRow.removeFromLeft(55));
    topRow.removeFromLeft(4);
    fillButton.setBounds(topRow.removeFromLeft(45));
    topRow.removeFromLeft(4);
    clearButton.setBounds(topRow.removeFromLeft(45));
    topRow.removeFromLeft(8);

    grooveCombo.setBounds(topRow.removeFromLeft(130));
    topRow.removeFromLeft(6);
    kitPresetCombo.setBounds(topRow.removeFromLeft(130));
    topRow.removeFromLeft(8);

    swingLabel.setBounds(topRow.removeFromLeft(40));
    swingSlider.setBounds(topRow.removeFromLeft(110));
    topRow.removeFromLeft(8);

    latencyCompLabel.setBounds(topRow.removeFromLeft(75));
    latencyCompSlider.setBounds(topRow.removeFromLeft(120));
    topRow.removeFromLeft(8);

    exportMidiButton.setBounds(topRow);

    area.removeFromTop(6);

    // Middle Row: 8 Trigger Pads
    auto padRow = area.removeFromTop(24);
    int padW = padRow.getWidth() / DrumEngine::NUM_VOICES;
    for (int i = 0; i < DrumEngine::NUM_VOICES; ++i)
    {
        padButtons[i].setBounds(padRow.removeFromLeft(padW).reduced(2, 0));
    }

    area.removeFromTop(6);

    // Bottom Area: 8 x 16 Step Sequencer Matrix
    int rowH = area.getHeight() / DrumEngine::NUM_VOICES;
    for (int t = 0; t < DrumEngine::NUM_VOICES; ++t)
    {
        auto row = area.removeFromTop(rowH).reduced(0, 1);
        trackLabels[t].setBounds(row.removeFromLeft(45));

        int stepW = row.getWidth() / DrumSequencer::NUM_STEPS;
        for (int s = 0; s < DrumSequencer::NUM_STEPS; ++s)
        {
            stepButtons[t][s]->setBounds(row.removeFromLeft(stepW).reduced(1, 0));
        }
    }
}

} // namespace paasphoos
