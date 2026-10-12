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
    // Logo & Header
    logoLabel.setText("PAASPHOOS", juce::dontSendNotification);
    logoLabel.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    logoLabel.setColour(juce::Label::textColourId, juce::Colour(0xff38bdf8));
    addAndMakeVisible(logoLabel);

    subtitleLabel.setText("Ultra-Low Latency Band Jamming Console", juce::dontSendNotification);
    subtitleLabel.setFont(juce::FontOptions(12.0f));
    subtitleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff94a3b8));
    addAndMakeVisible(subtitleLabel);

    modeBadgeLabel.setText("INTERNET & LAN / M1-M7 COMPLETE", juce::dontSendNotification);
    modeBadgeLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    modeBadgeLabel.setJustificationType(juce::Justification::centred);
    modeBadgeLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff1e293b));
    modeBadgeLabel.setColour(juce::Label::textColourId, juce::Colour(0xff38bdf8));
    addAndMakeVisible(modeBadgeLabel);

    cpuBadgeLabel.setText("CPU: 0.0%", juce::dontSendNotification);
    cpuBadgeLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    cpuBadgeLabel.setJustificationType(juce::Justification::centred);
    cpuBadgeLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff0f172a));
    cpuBadgeLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa5b4fc));
    addAndMakeVisible(cpuBadgeLabel);

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
    statusBarLabel.setColour(juce::Label::textColourId, juce::Colour(0xff94a3b8));
    addAndMakeVisible(statusBarLabel);

    mmcssStatusLabel.setText("MMCSS: Pro Audio Active", juce::dontSendNotification);
    mmcssStatusLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    mmcssStatusLabel.setJustificationType(juce::Justification::centredRight);
    mmcssStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff10b981));
    addAndMakeVisible(mmcssStatusLabel);

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

    auto activeColor = juce::Colour(0xff0284c7);
    auto idleColor = juce::Colour(0xff1e293b);

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

    statusBarLabel.setText(juce::String::formatted("Driver: %s | Device: %s | %.1fkHz | Buffer: %d spl (%.2f ms)",
                                                  typeName.toRawUTF8(), devName.toRawUTF8(), rate / 1000.0, buf, lat),
                           juce::dontSendNotification);
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0f1115));

    // Top Header separator
    g.setColour(juce::Colour(0xff1e222a));
    g.drawLine(0.0f, 54.0f, (float)getWidth(), 54.0f, 1.0f);

    // Bottom Status Bar background
    g.fillRect(0, getHeight() - 26, getWidth(), 26);
    g.setColour(juce::Colour(0xff27272a));
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
    auto titleArea = headerArea.removeFromLeft(360);
    logoLabel.setBounds(titleArea.removeFromTop(24));
    subtitleLabel.setBounds(titleArea);

    cpuBadgeLabel.setBounds(headerArea.removeFromRight(120).reduced(0, 4));
    headerArea.removeFromRight(8);
    modeBadgeLabel.setBounds(headerArea.removeFromRight(220).reduced(0, 4));

    area.reduce(10, 4);

    // Row 1: Session & Network LAN Panel
    sessionPanel.setBounds(area.removeFromTop(110).reduced(2));
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
}

} // namespace paasphoos
