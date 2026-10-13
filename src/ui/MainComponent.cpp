#include "MainComponent.h"

namespace paasphoos
{

MainComponent::MainComponent(AudioEngine& engine, bool introPreviewMode)
    : audioEngine(engine),
      isIntroPreviewMode(introPreviewMode),
      sessionPanel(engine),
      audioSettingsPanel(engine),
      metronomeComponent(engine),
      mixerPanel(engine),
      tunerComponent(engine),
      effectsPanel(engine.getBuiltInFx(), engine.getPluginHostManager()),
      drumPanel(engine.getDrumEngine(), engine.getDrumSequencer()),
      rehearsalToolsPanel(engine.getSessionRecorder(), engine.getSetlistManager(), engine.getChatTalkback(), engine.getNetworkSimulator(), engine)
{
    currentMusicianRoleColor = juce::Colour(ThemeConfig::kAccentVioletHex);

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
    logoLabel.setFont(juce::FontOptions(20.0f, juce::Font::bold));
    logoLabel.setColour(juce::Label::textColourId, juce::Colour(ThemeConfig::kTextPrimaryHex));
    addAndMakeVisible(logoLabel);

    subtitleLabel.setText("Ultra-Low Latency Band Jamming Console", juce::dontSendNotification);
    subtitleLabel.setFont(juce::FontOptions(11.0f));
    subtitleLabel.setColour(juce::Label::textColourId, juce::Colour(ThemeConfig::kTextMutedHex));
    addAndMakeVisible(subtitleLabel);

    modeBadgeLabel.setText("STUDIO JAM ENGINE / ONLINE", juce::dontSendNotification);
    modeBadgeLabel.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    modeBadgeLabel.setJustificationType(juce::Justification::centred);
    modeBadgeLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff161a24));
    modeBadgeLabel.setColour(juce::Label::textColourId, juce::Colour(ThemeConfig::kAccentCyanHex));
    addAndMakeVisible(modeBadgeLabel);

    cpuBadgeLabel.setText("CPU: 0.0%", juce::dontSendNotification);
    cpuBadgeLabel.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    cpuBadgeLabel.setJustificationType(juce::Justification::centred);
    cpuBadgeLabel.setColour(juce::Label::backgroundColourId, juce::Colour(ThemeConfig::kSurfaceHex));
    cpuBadgeLabel.setColour(juce::Label::textColourId, juce::Colour(ThemeConfig::kTextMutedHex));
    addAndMakeVisible(cpuBadgeLabel);

    // Profile badge button in header
    profileButton.onClick = [this]() {
        introScreen.startExperience(false, isIntroPreviewMode);
    };
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
    statusBarLabel.setColour(juce::Label::textColourId, juce::Colour(ThemeConfig::kTextMutedHex));
    addAndMakeVisible(statusBarLabel);

    mmcssStatusLabel.setText("MMCSS: Pro Audio Active", juce::dontSendNotification);
    mmcssStatusLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    mmcssStatusLabel.setJustificationType(juce::Justification::centredRight);
    mmcssStatusLabel.setColour(juce::Label::textColourId, juce::Colour(ThemeConfig::kAccentCyanHex));
    addAndMakeVisible(mmcssStatusLabel);

    // Onboarding Experience Setup
    introScreen.onStudioEntered = [this](const juce::String& name, const juce::String& role, juce::Colour roleColor, juce::Rectangle<float> /*orbRect*/)
    {
        currentMusicianRoleColor = roleColor;
        sessionPanel.setMusicianProfile(name, role);
        mixerPanel.setLocalMusicianName(name, role, roleColor);
        updateProfileBadge();
        statusBarLabel.setText("Active Musician: " + name + " (" + role + ") | Ready to Rehearse", juce::dontSendNotification);

        triggerPanelEntranceAnimation();
    };

    updateProfileBadge();

    addChildComponent(introScreen);

    // Always present full startup intro and name input screen on launch
    introScreen.startExperience(true, isIntroPreviewMode);

    setSize(1280, 840);
    startTimerHz(60); // 60 Hz for smooth entrance animation and meter sweep
}

MainComponent::~MainComponent()
{
    stopTimer();
}

void MainComponent::updateProfileBadge()
{
    auto name = introScreen.getMusicianName();
    auto role = introScreen.getInstrumentRole();
    profileButton.setButtonText(name.toUpperCase() + "  [" + role + "]");
    profileButton.setColour(juce::TextButton::buttonColourId, juce::Colour(ThemeConfig::kSurfaceHex));
    profileButton.setColour(juce::TextButton::textColourOffId, currentMusicianRoleColor);
    repaint();
}

void MainComponent::triggerPanelEntranceAnimation()
{
    isEntranceAnimating = true;
    entranceTimeSeconds = 0.0f;
    mixerPanel.triggerMeterSweep();
}

void MainComponent::switchView(int viewIndex)
{
    currentViewIndex = viewIndex;

    mixerPanel.setVisible(viewIndex == 0);
    tunerComponent.setVisible(viewIndex == 0);
    effectsPanel.setVisible(viewIndex == 1);
    drumPanel.setVisible(viewIndex == 2);
    rehearsalToolsPanel.setVisible(viewIndex == 3);

    auto activeColor = juce::Colour(ThemeConfig::kAccentVioletHex);
    auto idleColor   = juce::Colour(ThemeConfig::kSurfaceHex);

    viewMixerBtn.setColour(juce::TextButton::buttonColourId, (viewIndex == 0) ? activeColor : idleColor);
    viewFxBtn.setColour(juce::TextButton::buttonColourId, (viewIndex == 1) ? activeColor : idleColor);
    viewDrumsBtn.setColour(juce::TextButton::buttonColourId, (viewIndex == 2) ? activeColor : idleColor);
    viewRehearsalBtn.setColour(juce::TextButton::buttonColourId, (viewIndex == 3) ? activeColor : idleColor);

    resized();
}

void MainComponent::timerCallback()
{
    // Update CPU & Hardware info at ~10Hz
    static int frameCounter = 0;
    if (++frameCounter % 6 == 0)
    {
        double cpu = audioEngine.getCpuUsage();
        cpuBadgeLabel.setText(juce::String::formatted("CPU: %.1f%%", cpu), juce::dontSendNotification);

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

    // Handle Staggered Panel Entrance
    if (isEntranceAnimating)
    {
        entranceTimeSeconds += (1.0f / 60.0f);

        // Helper to compute panel alpha & vertical slide
        auto computeEntrance = [this](float delaySec) -> std::pair<float, float>
        {
            if (entranceTimeSeconds < delaySec)
                return { 0.0f, 18.0f }; // fully transparent, shifted down 18px

            float t = TweenEngine::clamp01((entranceTimeSeconds - delaySec) / 0.45f);
            float ease = TweenEngine::easeOutCubic(t);
            float alpha = ease;
            float slideY = 18.0f * (1.0f - ease);
            return { alpha, slideY };
        };

        // Panel 1: Session Panel (delay 0.00s)
        auto [a1, y1] = computeEntrance(0.00f);
        sessionPanel.setAlpha(a1);

        // Panel 2: Audio Settings & Metronome (delay 0.06s)
        auto [a2, y2] = computeEntrance(0.06f);
        audioSettingsPanel.setAlpha(a2);
        metronomeComponent.setAlpha(a2);

        // Panel 3: Nav Buttons (delay 0.12s)
        auto [a3, y3] = computeEntrance(0.12f);
        viewMixerBtn.setAlpha(a3);
        viewFxBtn.setAlpha(a3);
        viewDrumsBtn.setAlpha(a3);
        viewRehearsalBtn.setAlpha(a3);

        // Panel 4: Active View Area (delay 0.18s)
        auto [a4, y4] = computeEntrance(0.18f);
        mixerPanel.setAlpha(a4);
        tunerComponent.setAlpha(a4);
        effectsPanel.setAlpha(a4);
        drumPanel.setAlpha(a4);
        rehearsalToolsPanel.setAlpha(a4);

        if (entranceTimeSeconds >= 0.70f)
        {
            isEntranceAnimating = false;
            // Restore all to 1.0f
            sessionPanel.setAlpha(1.0f);
            audioSettingsPanel.setAlpha(1.0f);
            metronomeComponent.setAlpha(1.0f);
            viewMixerBtn.setAlpha(1.0f);
            viewFxBtn.setAlpha(1.0f);
            viewDrumsBtn.setAlpha(1.0f);
            viewRehearsalBtn.setAlpha(1.0f);
            mixerPanel.setAlpha(1.0f);
            tunerComponent.setAlpha(1.0f);
            effectsPanel.setAlpha(1.0f);
            drumPanel.setAlpha(1.0f);
            rehearsalToolsPanel.setAlpha(1.0f);
        }
    }
}

void MainComponent::paint(juce::Graphics& g)
{
    // Deep obsidian black background #0B0D12
    g.fillAll(juce::Colour(ThemeConfig::kBaseBackgroundHex));

    // Draw Vector Logo in Header (x = 12, y = 8, w = 36, h = 36)
    if (logoImage.isValid())
    {
        g.drawImage(logoImage, 12, 9, 36, 36, 0, 0, logoImage.getWidth(), logoImage.getHeight());
    }
    else
    {
        juce::Rectangle<float> logoBounds(12.0f, 9.0f, 36.0f, 36.0f);
        g.setColour(juce::Colour(ThemeConfig::kSurfaceHex));
        g.fillRoundedRectangle(logoBounds, 8.0f);

        juce::ColourGradient logoGrad(juce::Colour(ThemeConfig::kAccentVioletHex), logoBounds.getX(), logoBounds.getY(),
                                      juce::Colour(ThemeConfig::kAccentBlueHex), logoBounds.getRight(), logoBounds.getBottom(), false);
        g.setGradientFill(logoGrad);
        g.drawRoundedRectangle(logoBounds, 8.0f, 1.5f);

        // Soundwave center
        juce::Path wave;
        wave.startNewSubPath(logoBounds.getCentreX() - 9.0f, logoBounds.getCentreY());
        wave.lineTo(logoBounds.getCentreX() - 4.0f, logoBounds.getCentreY() - 7.0f);
        wave.lineTo(logoBounds.getCentreX(), logoBounds.getCentreY() + 8.0f);
        wave.lineTo(logoBounds.getCentreX() + 4.0f, logoBounds.getCentreY() - 5.0f);
        wave.lineTo(logoBounds.getCentreX() + 9.0f, logoBounds.getCentreY());
        g.setColour(juce::Colour(ThemeConfig::kAccentCyanHex));
        g.strokePath(wave, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Top Header separator (subtle 1px border)
    g.setColour(juce::Colour(ThemeConfig::kBorderSubtleHex));
    g.drawLine(0.0f, 54.0f, (float)getWidth(), 54.0f, 1.0f);

    // Bottom Status Bar background
    g.setColour(juce::Colour(ThemeConfig::kSurfaceHex));
    g.fillRect(0, getHeight() - 26, getWidth(), 26);
    g.setColour(juce::Colour(ThemeConfig::kBorderSubtleHex));
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
    auto headerArea = area.removeFromTop(54).reduced(12, 6);
    headerArea.removeFromLeft(44); // Space for logo icon (x: 12, w: 36)
    auto titleArea = headerArea.removeFromLeft(360);
    logoLabel.setBounds(titleArea.removeFromTop(24));
    subtitleLabel.setBounds(titleArea);

    cpuBadgeLabel.setBounds(headerArea.removeFromRight(110).reduced(0, 4));
    headerArea.removeFromRight(6);
    modeBadgeLabel.setBounds(headerArea.removeFromRight(190).reduced(0, 4));
    headerArea.removeFromRight(6);
    profileButton.setBounds(headerArea.removeFromRight(220).reduced(0, 4));

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
