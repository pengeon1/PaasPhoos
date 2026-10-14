#include "MainComponent.h"

namespace paasphoos
{

//==============================================================================
// CircularProfileButton Implementation
//==============================================================================
MainComponent::CircularProfileButton::CircularProfileButton()
    : juce::Button("MusicianProfileButton")
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void MainComponent::CircularProfileButton::setProfile(const juce::String& name, const juce::String& role, juce::Colour color)
{
    musicianName = name.trim().isEmpty() ? "Musician" : name.trim();
    instrumentRole = role.trim().isEmpty() ? "Vocals" : role.trim();
    roleColor = color;
    setTooltip("Musician Profile: " + musicianName + " (" + instrumentRole + ")\nClick to change name or instrument");
    repaint();
}

juce::Rectangle<float> MainComponent::CircularProfileButton::getAvatarOrbBoundsInParent() const
{
    auto b = getBounds().toFloat();
    float orbSize = 34.0f;
    float orbX = b.getRight() - orbSize - 1.0f;
    float orbY = b.getY() + (b.getHeight() - orbSize) * 0.5f;
    return juce::Rectangle<float>(orbX, orbY, orbSize, orbSize);
}

void MainComponent::CircularProfileButton::paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = getLocalBounds().toFloat();
    float orbSize = 34.0f;

    // The circular avatar orb is placed at the far right
    auto orbBounds = juce::Rectangle<float>(bounds.getRight() - orbSize - 1.0f,
                                            bounds.getY() + (bounds.getHeight() - orbSize) * 0.5f,
                                            orbSize, orbSize);

    // Text area is to the left of the circle
    auto textArea = juce::Rectangle<float>(6.0f, bounds.getY(),
                                           bounds.getWidth() - orbSize - 14.0f, bounds.getHeight());

    // Sleek glass pill container background
    auto pillBounds = bounds.reduced(1.0f);
    float cornerRadius = pillBounds.getHeight() * 0.5f;

    float bgAlpha = shouldDrawButtonAsDown ? 0.90f : (shouldDrawButtonAsHighlighted ? 0.75f : 0.50f);
    g.setColour(juce::Colour(0xff121520).withAlpha(bgAlpha));
    g.fillRoundedRectangle(pillBounds, cornerRadius);

    float borderAlpha = shouldDrawButtonAsHighlighted ? 0.65f : 0.30f;
    auto borderCol = shouldDrawButtonAsHighlighted ? juce::Colour(ThemeConfig::kAccentVioletHex)
                                                   : juce::Colour(ThemeConfig::kBorderSubtleHex);
    g.setColour(borderCol.withAlpha(borderAlpha));
    g.drawRoundedRectangle(pillBounds, cornerRadius, 1.2f);

    // Draw Musician Name (upper line) & Instrument Role (lower line)
    float midY = textArea.getCentreY();
    auto nameBounds = juce::Rectangle<float>(textArea.getX(), midY - 15.0f, textArea.getWidth(), 16.0f);
    auto roleBounds = juce::Rectangle<float>(textArea.getX(), midY + 1.0f, textArea.getWidth(), 13.0f);

    g.setColour(juce::Colour(ThemeConfig::kTextPrimaryHex));
    g.setFont(Typography::h2().withHeight(12.5f));
    g.drawText(musicianName, nameBounds, juce::Justification::centredRight, true);

    g.setColour(roleColor);
    g.setFont(Typography::labelCaps(9.5f));
    g.drawText(instrumentRole.toUpperCase(), roleBounds, juce::Justification::centredRight, false);

    // Draw Avatar Circle Orb
    drawAvatarCircle(g, orbBounds, musicianName, roleColor, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
}

void MainComponent::CircularProfileButton::drawAvatarCircle(juce::Graphics& g, const juce::Rectangle<float>& orbBounds,
                                                            const juce::String& name, juce::Colour col, bool highlighted, bool isDown)
{
    if (orbBounds.isEmpty()) return;

    // Color hash deterministic variation matching IntroScreenComponent
    uint32_t hash = 5381;
    for (int i = 0; i < name.length(); ++i)
        hash = ((hash << 5) + hash) + (uint32_t)name[i];

    auto c1 = col;
    auto c2 = juce::Colour(ThemeConfig::kAccentVioletHex);
    if ((hash & 1) == 1)
        c2 = juce::Colour(ThemeConfig::kAccentBlueHex);

    // Glowing Aura
    float auraAlpha = isDown ? 0.45f : (highlighted ? 0.35f : 0.20f);
    float auraSpread = highlighted ? 5.5f : 3.0f;
    g.setColour(c1.withAlpha(auraAlpha));
    g.fillEllipse(orbBounds.expanded(auraSpread));

    // Orb Body Gradient
    juce::ColourGradient orbGrad(c1, orbBounds.getX(), orbBounds.getY(),
                                 c2, orbBounds.getRight(), orbBounds.getBottom(), false);
    g.setGradientFill(orbGrad);
    g.fillEllipse(orbBounds);

    // Rim Highlight
    float rimAlpha = highlighted ? 0.60f : 0.35f;
    g.setColour(juce::Colour(0xffffffff).withAlpha(rimAlpha));
    g.drawEllipse(orbBounds, 1.2f);

    // Initial Letter in center
    auto initial = name.trim().substring(0, 1).toUpperCase();
    if (initial.isEmpty()) initial = "P";

    g.setColour(juce::Colour(ThemeConfig::kTextPrimaryHex));
    g.setFont(Typography::display(orbBounds.getHeight() * 0.44f));
    g.drawText(initial, orbBounds, juce::Justification::centred, false);
}

//==============================================================================
// MainComponent Implementation
//==============================================================================
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

    // Load brand logo from Typography (bundled BinaryData with disk fallback)
    logoImage = Typography::getLogoImage();

    // Reusable Brand Wordmark (Sora 700 gradient)
    logoWordmark.setFontSize(22.0f);
    addAndMakeVisible(logoWordmark);

    subtitleLabel.setText("Ultra-Low Latency Band Jamming Console", juce::dontSendNotification);
    subtitleLabel.setFont(Typography::caption());
    subtitleLabel.setColour(juce::Label::textColourId, juce::Colour(ThemeConfig::kTextMutedHex));
    addAndMakeVisible(subtitleLabel);

    modeBadgeLabel.setText("STUDIO JAM ENGINE / ONLINE", juce::dontSendNotification);
    modeBadgeLabel.setFont(Typography::labelCaps(10.0f));
    modeBadgeLabel.setJustificationType(juce::Justification::centred);
    modeBadgeLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff161a24));
    modeBadgeLabel.setColour(juce::Label::textColourId, juce::Colour(ThemeConfig::kAccentCyanHex));
    addAndMakeVisible(modeBadgeLabel);

    cpuBadgeLabel.setText("CPU: 0.0%", juce::dontSendNotification);
    cpuBadgeLabel.setFont(Typography::numeric(10.5f));
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
    statusBarLabel.setFont(Typography::caption());
    statusBarLabel.setColour(juce::Label::textColourId, juce::Colour(ThemeConfig::kTextMutedHex));
    addAndMakeVisible(statusBarLabel);

    mmcssStatusLabel.setText("MMCSS: Pro Audio Active", juce::dontSendNotification);
    mmcssStatusLabel.setFont(Typography::numeric(10.5f));
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
    profileButton.setProfile(name, role, currentMusicianRoleColor);
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
    logoWordmark.setBounds(titleArea.removeFromTop(24));
    subtitleLabel.setBounds(titleArea);

    // Far Top Right: Musician Profile Button (Circular Avatar Orb + Musician Username)
    profileButton.setBounds(headerArea.removeFromRight(175).reduced(0, 3));
    headerArea.removeFromRight(10);

    // Telemetry: CPU badge
    cpuBadgeLabel.setBounds(headerArea.removeFromRight(100).reduced(0, 4));
    headerArea.removeFromRight(6);

    // Status: Studio Jam Engine mode badge
    modeBadgeLabel.setBounds(headerArea.removeFromRight(185).reduced(0, 4));

    // Keep intro screen flying orb landing position in sync with circular avatar
    introScreen.setFlyingOrbTargetBounds(profileButton.getAvatarOrbBoundsInParent());

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

void MainComponent::exportScreenshots(const juce::File& outputDir)
{
    outputDir.createDirectory();
    const int baseW = 1366;
    const int baseH = 768;
    setSize(baseW, baseH);

    isEntranceAnimating = false;
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

    auto saveImage = [](const juce::Image& img, const juce::File& file)
    {
        if (file.existsAsFile()) file.deleteFile();
        juce::FileOutputStream stream(file);
        if (stream.openedOk())
        {
            juce::PNGImageFormat png;
            png.writeImageToStream(img, stream);
        }
    };

    auto captureComponentAtScale = [baseW, baseH, &saveImage](juce::Component& comp, float scale, const juce::File& file)
    {
        int w = juce::roundToInt((float)baseW * scale);
        int h = juce::roundToInt((float)baseH * scale);
        juce::Image img(juce::Image::ARGB, w, h, true);
        {
            juce::Graphics g(img);
            if (scale != 1.0f)
                g.addTransform(juce::AffineTransform::scale(scale));
            comp.paintEntireComponent(g, true);
        }
        saveImage(img, file);
    };

    const float dpiScales[] = { 1.0f, 1.5f, 2.0f };
    const char* dpiNames[] = { "100dpi", "150dpi", "200dpi" };

    // 1. Intro Wordmark & Name Screen
    introScreen.setVisible(true);
    introScreen.setBounds(0, 0, baseW, baseH);
    for (int d = 0; d < 3; ++d)
    {
        float scale = dpiScales[d];
        auto wordmarkImg = introScreen.captureFrameAtTime(1.1f, baseW, baseH);
        if (scale != 1.0f)
            wordmarkImg = wordmarkImg.rescaled(juce::roundToInt(baseW * scale), juce::roundToInt(baseH * scale), juce::Graphics::highResamplingQuality);
        saveImage(wordmarkImg, outputDir.getChildFile(juce::String("01_intro_wordmark_") + dpiNames[d] + ".png"));

        auto nameImg = introScreen.captureFrameAtTime(2.7f, baseW, baseH);
        if (scale != 1.0f)
            nameImg = nameImg.rescaled(juce::roundToInt(baseW * scale), juce::roundToInt(baseH * scale), juce::Graphics::highResamplingQuality);
        saveImage(nameImg, outputDir.getChildFile(juce::String("02_name_screen_") + dpiNames[d] + ".png"));

        // Avatar orb flying toward top right profile position
        auto flightImg = introScreen.captureFrameAtTime(ThemeConfig::kIntroTotalDurationFirstRun + 1.55f, baseW, baseH);
        if (scale != 1.0f)
            flightImg = flightImg.rescaled(juce::roundToInt(baseW * scale), juce::roundToInt(baseH * scale), juce::Graphics::highResamplingQuality);
        saveImage(flightImg, outputDir.getChildFile(juce::String("02b_avatar_flying_to_top_right_") + dpiNames[d] + ".png"));
    }

    introScreen.setVisible(false);

    // Set sample profile with custom name & role for app screenshots
    profileButton.setProfile("Piyush", "Vocals", ThemeConfig::getRoleColor(0));

    // 2. Main Mixer with 6 channels
    mixerPanel.populateMockBandChannels();
    switchView(0);
    resized();
    for (int d = 0; d < 3; ++d)
    {
        captureComponentAtScale(*this, dpiScales[d], outputDir.getChildFile(juce::String("03_mixer_6_channels_") + dpiNames[d] + ".png"));
    }

    // 3. Effects Rack with differentiated, color-coded knobs
    switchView(1);
    resized();
    for (int d = 0; d < 3; ++d)
    {
        captureComponentAtScale(*this, dpiScales[d], outputDir.getChildFile(juce::String("04_effects_rack_differentiated_knobs_") + dpiNames[d] + ".png"));
    }

    // 4. Drum Machine view
    switchView(2);
    resized();
    for (int d = 0; d < 3; ++d)
    {
        captureComponentAtScale(*this, dpiScales[d], outputDir.getChildFile(juce::String("05_drum_machine_") + dpiNames[d] + ".png"));
    }

    // 4b. Rehearsal Tools, Setlist & Chat view
    switchView(3);
    resized();
    for (int d = 0; d < 3; ++d)
    {
        captureComponentAtScale(*this, dpiScales[d], outputDir.getChildFile(juce::String("05b_rehearsal_setlist_chat_") + dpiNames[d] + ".png"));
    }

    // 5. Latency Panel & HUD

    for (int d = 0; d < 3; ++d)
    {
        float scale = dpiScales[d];
        int sw = sessionPanel.getWidth();
        int sh = sessionPanel.getHeight();
        if (sw > 0 && sh > 0)
        {
            int pw = juce::roundToInt(sw * scale);
            int ph = juce::roundToInt(sh * scale);
            juce::Image pImg(juce::Image::ARGB, pw, ph, true);
            {
                juce::Graphics g(pImg);
                if (scale != 1.0f) g.addTransform(juce::AffineTransform::scale(scale));
                sessionPanel.paintEntireComponent(g, true);
            }
            saveImage(pImg, outputDir.getChildFile(juce::String("06_latency_panel_readouts_") + dpiNames[d] + ".png"));
        }
    }

    // 6. Settings Dialog / Panel
    for (int d = 0; d < 3; ++d)
    {
        float scale = dpiScales[d];
        int sw = audioSettingsPanel.getWidth();
        int sh = audioSettingsPanel.getHeight();
        if (sw > 0 && sh > 0)
        {
            int pw = juce::roundToInt(sw * scale);
            int ph = juce::roundToInt(sh * scale);
            juce::Image pImg(juce::Image::ARGB, pw, ph, true);
            {
                juce::Graphics g(pImg);
                if (scale != 1.0f) g.addTransform(juce::AffineTransform::scale(scale));
                audioSettingsPanel.paintEntireComponent(g, true);
            }
            saveImage(pImg, outputDir.getChildFile(juce::String("07_audio_settings_dialog_") + dpiNames[d] + ".png"));
        }
    }
}

} // namespace paasphoos
