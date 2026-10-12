#include "MainComponent.h"

namespace paasphoos
{

MainComponent::MainComponent(AudioEngine& engine)
    : audioEngine(engine),
      sessionPanel(engine),
      audioSettingsPanel(engine),
      metronomeComponent(engine),
      mixerPanel(engine),
      tunerComponent(engine)
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

    modeBadgeLabel.setText("LAN MODE / M3 ENGINE", juce::dontSendNotification);
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

    addAndMakeVisible(sessionPanel);
    addAndMakeVisible(audioSettingsPanel);
    addAndMakeVisible(metronomeComponent);
    addAndMakeVisible(mixerPanel);
    addAndMakeVisible(tunerComponent);

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

    setSize(1240, 780);
    startTimerHz(10); // 10 Hz refresh for status and CPU
}

MainComponent::~MainComponent()
{
    stopTimer();
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
    modeBadgeLabel.setBounds(headerArea.removeFromRight(170).reduced(0, 4));

    area.reduce(10, 6);

    // Row 1: Session & Network LAN Panel
    sessionPanel.setBounds(area.removeFromTop(114).reduced(2));
    area.removeFromTop(6);

    // Row 2: Audio Settings Panel + Metronome
    auto row2 = area.removeFromTop(116);
    audioSettingsPanel.setBounds(row2.removeFromLeft(row2.getWidth() * 0.54).reduced(3));
    metronomeComponent.setBounds(row2.reduced(3));
    area.removeFromTop(6);

    // Row 3: Band Mixer Console (Left 73%) + Tuner (Right 27%)
    int mixerWidth = static_cast<int>(area.getWidth() * 0.73);
    mixerPanel.setBounds(area.removeFromLeft(mixerWidth).reduced(3));
    tunerComponent.setBounds(area.reduced(3));
}

} // namespace paasphoos
