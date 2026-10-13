#include "MainComponent.h"

namespace paasphoos
{

MainComponent::MainComponent(AudioEngine& engine)
    : audioEngine(engine),
      sessionPanel(engine),
      audioSettingsPanel(engine),
      metronomeComponent(engine),
      mixerPanel(engine),
      tunerComponent(engine),
      effectsPanel(engine.getBuiltInFx(), engine.getPluginHostManager()),
      drumPanel(engine.getDrumEngine(), engine.getDrumSequencer()),
      rehearsalToolsPanel(engine.getSessionRecorder(), engine.getSetlistManager(), engine.getChatTalkback(), engine.getNetworkSimulator(), engine)
{
    // Try to load generated brand logo from resources
    auto logoFile = juce::File::getCurrentWorkingDirectory().getChildFile("resources/logo.jpg");
    if (!logoFile.existsAsFile())
    {
        logoFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory().getChildFile("resources/logo.jpg");
    }
    if (logoFile.existsAsFile())
    {
        logoImage = juce::ImageFileFormat::loadFrom(logoFile);
    }

    // Logo & Header
    logoLabel.setText("PAASPHOOS", juce::dontSendNotification);
    logoLabel.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    logoLabel.setColour(juce::Label::textColourId, juce::Colour(0xffff6600)); // Glowing Flame Orange
    addAndMakeVisible(logoLabel);

    subtitleLabel.setText("Ultra-Low Latency Band Jamming Console", juce::dontSendNotification);
    subtitleLabel.setFont(juce::FontOptions(11.5f));
    subtitleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff9ca3af));
    addAndMakeVisible(subtitleLabel);

    modeBadgeLabel.setText("STUDIO JAM ENGINE / ONLINE", juce::dontSendNotification);
    modeBadgeLabel.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    modeBadgeLabel.setJustificationType(juce::Justification::centred);
    modeBadgeLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff1f130b));
    modeBadgeLabel.setColour(juce::Label::textColourId, juce::Colour(0xffff9933));
    addAndMakeVisible(modeBadgeLabel);

    cpuBadgeLabel.setText("CPU: 0.0%", juce::dontSendNotification);
    cpuBadgeLabel.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    cpuBadgeLabel.setJustificationType(juce::Justification::centred);
    cpuBadgeLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff15161c));
    cpuBadgeLabel.setColour(juce::Label::textColourId, juce::Colour(0xffffaa33));
    addAndMakeVisible(cpuBadgeLabel);

    profileButton.setButtonText("MUSICIAN: " + introScreen.getMusicianName() + " (" + introScreen.getInstrumentRole() + ")");
    profileButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff181a24));
    profileButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffffaa33));
    profileButton.onClick = [this]() { introScreen.show(true); };
    addAndMakeVisible(profileButton);

    // View Navigation Buttons
    viewMixerBtn.setButtonText("MIXER & TUNER");
    viewMixerBtn.onClick = [this]() { switchView(0); };
    addAndMakeVisible(viewMixerBtn);

    viewFxBtn.setButtonText("FX RACK & VST3");
    viewFxBtn.onClick = [this]() { switchView(1); };
    addAndMakeVisible(viewFxBtn);

    viewDrumsBtn.setButtonText("DRUM MACHINE");
    viewDrumsBtn.onClick = [this]() { switchView(2); };
    addAndMakeVisible(viewDrumsBtn);

    viewRehearsalBtn.setButtonText("SETLIST, REC & CHAT");
    viewRehearsalBtn.onClick = [this]() { switchView(3); };
    addAndMakeVisible(viewRehearsalBtn);

    // Always-visible Global Panels
    addAndMakeVisible(sessionPanel);
    addAndMakeVisible(audioSettingsPanel);
    addAndMakeVisible(metronomeComponent);

    // Dynamic Workspace Views
    addChildComponent(mixerPanel);
    addChildComponent(tunerComponent);
    addChildComponent(effectsPanel);
    addChildComponent(drumPanel);
    addChildComponent(rehearsalToolsPanel);

    // Default to Mixer & Tuner
    switchView(0);

    // Status Bar
    statusBarLabel.setText("Ready: Audio Engine Initialized", juce::dontSendNotification);
    statusBarLabel.setFont(juce::FontOptions(11.0f));
    statusBarLabel.setColour(juce::Label::textColourId, juce::Colour(0xff9ca3af));
    addAndMakeVisible(statusBarLabel);

    mmcssStatusLabel.setText("MMCSS: Pro Audio Active", juce::dontSendNotification);
    mmcssStatusLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    mmcssStatusLabel.setJustificationType(juce::Justification::centredRight);
    mmcssStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xffff8800));
    addAndMakeVisible(mmcssStatusLabel);

    // Setup Intro Screen Animation and Musician Name Entry
    introScreen.onStudioEntered = [this](const juce::String& name, const juce::String& role)
    {
        sessionPanel.setMusicianProfile(name, role);
        mixerPanel.setLocalMusicianName(name, role);
        profileButton.setButtonText("MUSICIAN: " + name + " (" + role + ")");
        statusBarLabel.setText("Active Musician: " + name + " (" + role + ") | Ready to Rehearse", juce::dontSendNotification);
    };

    // Apply initial profile
    sessionPanel.setMusicianProfile(introScreen.getMusicianName(), introScreen.getInstrumentRole());
    mixerPanel.setLocalMusicianName(introScreen.getMusicianName(), introScreen.getInstrumentRole());

    addChildComponent(introScreen);
    introScreen.show(false);

    setSize(1280, 840);
    startTimerHz(10); // 10 Hz refresh for status and CPU
}

MainComponent::~MainComponent()
{
    stopTimer();
}

void MainComponent::switchView(int viewIndex)
{
    currentViewIndex = viewIndex;

    mixerPanel.setVisible(viewIndex == 0);
    tunerComponent.setVisible(viewIndex == 0);
    effectsPanel.setVisible(viewIndex == 1);
    drumPanel.setVisible(viewIndex == 2);
    rehearsalToolsPanel.setVisible(viewIndex == 3);

    auto activeColor = juce::Colour(0xffff6600); // Electric flame orange
    auto idleColor = juce::Colour(0xff16171f);   // Obsidian carbon dark

    viewMixerBtn.setColour(juce::TextButton::buttonColourId, (viewIndex == 0) ? activeColor : idleColor);
    viewFxBtn.setColour(juce::TextButton::buttonColourId, (viewIndex == 1) ? activeColor : idleColor);
    viewDrumsBtn.setColour(juce::TextButton::buttonColourId, (viewIndex == 2) ? activeColor : idleColor);
    viewRehearsalBtn.setColour(juce::TextButton::buttonColourId, (viewIndex == 3) ? activeColor : idleColor);

    resized();
}

void MainComponent::timerCallback()
{
    double cpu = audioEngine.getCpuUsage();
    cpuBadgeLabel.setText(juce::String::formatted("AUDIO CPU: %.1f%%", cpu), juce::dontSendNotification);

    juce::String devName = audioEngine.getCurrentDeviceName();
    juce::String typeName = audioEngine.getCurrentDeviceTypeName();
    int buf = audioEngine.getCurrentBufferSize();
    double rate = audioEngine.getCurrentSampleRate();
    double lat = audioEngine.getHardwareLatencyMs();

    juce::String rateStr = juce::String(rate / 1000.0, 1) + "kHz";
    juce::String latStr = juce::String(lat, 2) + " ms";
    statusBarLabel.setText("Driver: " + typeName + " | Device: " + devName + " | " + rateStr + " | Buffer: " + juce::String(buf) + " spl (" + latStr + ")",
                           juce::dontSendNotification);
}

void MainComponent::paint(juce::Graphics& g)
{
    // Deep obsidian black background
    g.fillAll(juce::Colour(0xff090a0d));

    // Draw Brand Logo Icon in header (x = 10, y = 8, w = 38, h = 38)
    if (logoImage.isValid())
    {
        g.drawImage(logoImage, 10, 8, 38, 38, 0, 0, logoImage.getWidth(), logoImage.getHeight());
    }
    else
    {
        // Custom Glowing Vector Logo: Neon Orange Headphone + Audio Wave Pick
        juce::Rectangle<float> logoBounds(10.0f, 8.0f, 38.0f, 38.0f);
        g.setColour(juce::Colour(0xff1a1410));
        g.fillRoundedRectangle(logoBounds, 8.0f);
        g.setColour(juce::Colour(0xffff6600));
        g.drawRoundedRectangle(logoBounds, 8.0f, 1.5f);

        // Headphone Arc
        juce::Path arc;
        arc.addCentredArc(logoBounds.getCentreX(), logoBounds.getCentreY() + 1.0f, 12.0f, 12.0f, 0.0f, -juce::MathConstants<float>::pi * 0.75f, juce::MathConstants<float>::pi * 0.75f, true);
        g.setColour(juce::Colour(0xffffaa33));
        g.strokePath(arc, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Soundwave center
        juce::Path wave;
        wave.startNewSubPath(logoBounds.getCentreX() - 9.0f, logoBounds.getCentreY() + 1.0f);
        wave.lineTo(logoBounds.getCentreX() - 4.0f, logoBounds.getCentreY() - 7.0f);
        wave.lineTo(logoBounds.getCentreX(), logoBounds.getCentreY() + 8.0f);
        wave.lineTo(logoBounds.getCentreX() + 4.0f, logoBounds.getCentreY() - 5.0f);
        wave.lineTo(logoBounds.getCentreX() + 9.0f, logoBounds.getCentreY() + 1.0f);
        g.setColour(juce::Colour(0xffff6600));
        g.strokePath(wave, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Top Header separator (charcoal with subtle warm amber tint)
    g.setColour(juce::Colour(0xff22242e));
    g.drawLine(0.0f, 54.0f, (float)getWidth(), 54.0f, 1.0f);

    // Bottom Status Bar background (obsidian carbon)
    g.fillRect(0, getHeight() - 26, getWidth(), 26);
    g.setColour(juce::Colour(0xff22242e));
    g.drawLine(0.0f, (float)getHeight() - 26.0f, (float)getWidth(), (float)getHeight() - 26.0f, 1.0f);
}

void MainComponent::resized()
{
    auto area = getLocalBounds();

    // Bottom status bar
    auto statusArea = area.removeFromBottom(26).reduced(10, 2);
    mmcssStatusLabel.setBounds(statusArea.removeFromRight(180));
    statusBarLabel.setBounds(statusArea);

    // Header Bar
    auto headerArea = area.removeFromTop(54).reduced(10, 6);
    headerArea.removeFromLeft(44); // Space for logo icon (x: 10, w: 38)
    auto titleArea = headerArea.removeFromLeft(360);
    logoLabel.setBounds(titleArea.removeFromTop(24));
    subtitleLabel.setBounds(titleArea);

    cpuBadgeLabel.setBounds(headerArea.removeFromRight(110).reduced(0, 4));
    headerArea.removeFromRight(6);
    modeBadgeLabel.setBounds(headerArea.removeFromRight(190).reduced(0, 4));
    headerArea.removeFromRight(6);
    profileButton.setBounds(headerArea.removeFromRight(200).reduced(0, 4));

    area.reduce(10, 4);

    // Row 1: Session & Jam Code Network Panel
    sessionPanel.setBounds(area.removeFromTop(120).reduced(2));
    area.removeFromTop(4);

    // Row 2: Audio Settings Panel + Metronome
    auto row2 = area.removeFromTop(110);
    audioSettingsPanel.setBounds(row2.removeFromLeft(row2.getWidth() * 0.54).reduced(2));
    metronomeComponent.setBounds(row2.reduced(2));
    area.removeFromTop(6);

    // View Navigation Toolbar (Tab bar)
    auto navBar = area.removeFromTop(32).reduced(2, 0);
    int btnWidth = navBar.getWidth() / 4;
    viewMixerBtn.setBounds(navBar.removeFromLeft(btnWidth).reduced(2, 0));
    viewFxBtn.setBounds(navBar.removeFromLeft(btnWidth).reduced(2, 0));
    viewDrumsBtn.setBounds(navBar.removeFromLeft(btnWidth).reduced(2, 0));
    viewRehearsalBtn.setBounds(navBar.reduced(2, 0));
    area.removeFromTop(6);

    // Dynamic View Area
    if (currentViewIndex == 0)
    {
        int mixerWidth = static_cast<int>(area.getWidth() * 0.73);
        mixerPanel.setBounds(area.removeFromLeft(mixerWidth).reduced(2));
        tunerComponent.setBounds(area.reduced(2));
    }
    else if (currentViewIndex == 1)
    {
        effectsPanel.setBounds(area.reduced(2));
    }
    else if (currentViewIndex == 2)
    {
        drumPanel.setBounds(area.reduced(2));
    }
    else if (currentViewIndex == 3)
    {
        rehearsalToolsPanel.setBounds(area.reduced(2));
    }

    // Intro Screen & Musician Profile setup covers window when active
    introScreen.setBounds(getLocalBounds());
    introScreen.toFront(true);
}

} // namespace paasphoos
