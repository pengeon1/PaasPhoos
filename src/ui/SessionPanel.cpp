#include "SessionPanel.h"

namespace paasphoos
{

SessionPanel::SessionPanel(AudioEngine& engine)
    : audioEngine(engine)
{
    audioEngine.getLanDiscovery().addChangeListener(this);
    audioEngine.getLanTransport().addChangeListener(this);

    headerLabel.setText("SESSION & NETWORK (LAN MODE - 2 PEERS)", juce::dontSendNotification);
    headerLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    headerLabel.setColour(juce::Label::textColourId, juce::Colour(0xff94a3b8));
    addAndMakeVisible(headerLabel);

    // Mode Buttons
    lanModeButton.setButtonText("LAN Mode (Active)");
    lanModeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff0284c7));
    addAndMakeVisible(lanModeButton);

    internetModeButton.setButtonText("Internet Mode (M6)");
    internetModeButton.setEnabled(false);
    addAndMakeVisible(internetModeButton);

    // Host Controls
    hostSectionLabel.setText("1. HOST A REHEARSAL", juce::dontSendNotification);
    hostSectionLabel.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    hostSectionLabel.setColour(juce::Label::textColourId, juce::Colour(0xff38bdf8));
    addAndMakeVisible(hostSectionLabel);

    hostSessionButton.setButtonText("START HOSTING");
    hostSessionButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff047857));
    hostSessionButton.onClick = [this]()
    {
        bool isHosting = audioEngine.getLanDiscovery().isHosting();
        if (isHosting)
        {
            audioEngine.getLanDiscovery().stop();
            audioEngine.getLanTransport().disconnect();
            hostSessionButton.setButtonText("START HOSTING");
            hostSessionButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff047857));
            hostStatusLabel.setText("Host stopped.", juce::dontSendNotification);
        }
        else
        {
            adapterInfo = NetworkAdapterDetector::detectPrimaryAdapter();
            int port = LanAudioTransport::DEFAULT_AUDIO_PORT;
            audioEngine.getLanTransport().startAsHost(port);
            audioEngine.getLanDiscovery().startHosting("PaasPhoos Band Jam", "Host", port);
            hostSessionButton.setButtonText("STOP HOSTING");
            hostSessionButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffb91c1c));
            hostStatusLabel.setText(juce::String::formatted("Hosting on %s:%d", adapterInfo.ipAddress.toRawUTF8(), port), juce::dontSendNotification);
        }
    };
    addAndMakeVisible(hostSessionButton);

    hostStatusLabel.setText("Not hosting.", juce::dontSendNotification);
    hostStatusLabel.setFont(juce::FontOptions(11.0f));
    hostStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff94a3b8));
    addAndMakeVisible(hostStatusLabel);

    // Join Section
    joinSectionLabel.setText("2. JOIN A REHEARSAL", juce::dontSendNotification);
    joinSectionLabel.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    joinSectionLabel.setColour(juce::Label::textColourId, juce::Colour(0xff38bdf8));
    addAndMakeVisible(joinSectionLabel);

    // Discovered Sessions Combo
    discoveredSessionsCombo.setTextWhenNoChoicesAvailable("No LAN sessions found yet");
    discoveredSessionsCombo.setTextWhenNothingSelected("Select discovered LAN session...");
    addAndMakeVisible(discoveredSessionsCombo);

    joinDiscoveredButton.setButtonText("JOIN SELECTED");
    joinDiscoveredButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff0284c7));
    joinDiscoveredButton.onClick = [this]()
    {
        auto sessions = audioEngine.getLanDiscovery().getDiscoveredSessions();
        int idx = discoveredSessionsCombo.getSelectedId() - 1;
        if (idx >= 0 && idx < static_cast<int>(sessions.size()))
        {
            auto& s = sessions[idx];
            audioEngine.getLanTransport().startAsClient(s.ipAddress, s.audioPort);
        }
    };
    addAndMakeVisible(joinDiscoveredButton);

    refreshBrowseButton.setButtonText("SCAN LAN");
    refreshBrowseButton.onClick = [this]()
    {
        audioEngine.getLanDiscovery().startBrowsing();
        refreshDiscoveredList();
    };
    addAndMakeVisible(refreshBrowseButton);

    // Manual Direct Connect
    manualIpLabel.setText("IP:", juce::dontSendNotification);
    manualIpLabel.setFont(juce::FontOptions(11.0f));
    addAndMakeVisible(manualIpLabel);

    manualIpEditor.setText("127.0.0.1");
    addAndMakeVisible(manualIpEditor);

    manualPortLabel.setText("Port:", juce::dontSendNotification);
    manualPortLabel.setFont(juce::FontOptions(11.0f));
    addAndMakeVisible(manualPortLabel);

    manualPortEditor.setText("44045");
    addAndMakeVisible(manualPortEditor);

    connectManualButton.setButtonText("DIRECT CONNECT");
    connectManualButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2563eb));
    connectManualButton.onClick = [this]()
    {
        juce::String ip = manualIpEditor.getText().trim();
        int port = manualPortEditor.getText().getIntValue();
        if (port <= 0) port = 44045;
        audioEngine.getLanTransport().startAsClient(ip, port);
    };
    addAndMakeVisible(connectManualButton);

    // Active session status & Disconnect
    sessionStatusBadge.setText("STATUS: STANDALONE", juce::dontSendNotification);
    sessionStatusBadge.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    sessionStatusBadge.setJustificationType(juce::Justification::centred);
    sessionStatusBadge.setColour(juce::Label::backgroundColourId, juce::Colour(0xff1e293b));
    sessionStatusBadge.setColour(juce::Label::textColourId, juce::Colour(0xff94a3b8));
    addAndMakeVisible(sessionStatusBadge);

    disconnectButton.setButtonText("DISCONNECT");
    disconnectButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff7f1d1d));
    disconnectButton.onClick = [this]()
    {
        audioEngine.getLanDiscovery().stop();
        audioEngine.getLanTransport().disconnect();
        hostSessionButton.setButtonText("START HOSTING");
        hostSessionButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff047857));
        hostStatusLabel.setText("Disconnected.", juce::dontSendNotification);
    };
    addAndMakeVisible(disconnectButton);

    // Network Diagnostics HUD
    hudPingLabel.setText("Ping: -- ms", juce::dontSendNotification);
    hudPingLabel.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    hudPingLabel.setColour(juce::Label::textColourId, juce::Colour(0xfff8fafc));
    addAndMakeVisible(hudPingLabel);

    hudJitterLabel.setText("Jitter: -- ms", juce::dontSendNotification);
    hudJitterLabel.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    hudJitterLabel.setColour(juce::Label::textColourId, juce::Colour(0xfff8fafc));
    addAndMakeVisible(hudJitterLabel);

    hudLossLabel.setText("Loss: 0.0%", juce::dontSendNotification);
    hudLossLabel.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    hudLossLabel.setColour(juce::Label::textColourId, juce::Colour(0xfff8fafc));
    addAndMakeVisible(hudLossLabel);

    hudTotalLatencyLabel.setText("Total Ear-to-Ear: -- ms", juce::dontSendNotification);
    hudTotalLatencyLabel.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    hudTotalLatencyLabel.setColour(juce::Label::textColourId, juce::Colour(0xff38bdf8));
    addAndMakeVisible(hudTotalLatencyLabel);

    healthStatusPill.setText("IDLE", juce::dontSendNotification);
    healthStatusPill.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    healthStatusPill.setJustificationType(juce::Justification::centred);
    healthStatusPill.setColour(juce::Label::backgroundColourId, juce::Colour(0xff334155));
    healthStatusPill.setColour(juce::Label::textColourId, juce::Colour(0xfff8fafc));
    addAndMakeVisible(healthStatusPill);

    // Adapter Warning Label
    adapterWarningLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    adapterWarningLabel.setJustificationType(juce::Justification::centred);
    adapterWarningLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff78350f));
    adapterWarningLabel.setColour(juce::Label::textColourId, juce::Colour(0xfffef08a));
    addChildComponent(adapterWarningLabel);

    updateAdapterWarning();
    startTimerHz(10); // 10 FPS stats update
}

SessionPanel::~SessionPanel()
{
    stopTimer();
    audioEngine.getLanDiscovery().removeChangeListener(this);
    audioEngine.getLanTransport().removeChangeListener(this);
}

void SessionPanel::changeListenerCallback(juce::ChangeBroadcaster* /*source*/)
{
    refreshDiscoveredList();
}

void SessionPanel::refreshDiscoveredList()
{
    auto sessions = audioEngine.getLanDiscovery().getDiscoveredSessions();
    discoveredSessionsCombo.clear();

    for (size_t i = 0; i < sessions.size(); ++i)
    {
        auto& s = sessions[i];
        juce::String text = juce::String::formatted("%s (%s:%d) [%d/%d]",
                                                    s.sessionName.toRawUTF8(),
                                                    s.ipAddress.toRawUTF8(),
                                                    s.audioPort,
                                                    s.participantCount,
                                                    s.maxParticipants);
        discoveredSessionsCombo.addItem(text, static_cast<int>(i + 1));
    }
}

void SessionPanel::updateAdapterWarning()
{
    adapterInfo = NetworkAdapterDetector::detectPrimaryAdapter();
    if (adapterInfo.isWiFi)
    {
        adapterWarningLabel.setText(adapterInfo.warningMessage, juce::dontSendNotification);
        adapterWarningLabel.setVisible(true);
    }
    else
    {
        adapterWarningLabel.setVisible(false);
    }
}

void SessionPanel::updateNetworkStatsDisplay()
{
    auto& transport = audioEngine.getLanTransport();
    bool isConn = transport.isConnected();
    auto role = transport.getRole();

    if (isConn)
    {
        juce::String roleText = (role == SessionRole::HOST) ? "HOSTING" : "PEER CONNECTED";
        sessionStatusBadge.setText(roleText + " (" + transport.getRemoteAddress() + ")", juce::dontSendNotification);
        sessionStatusBadge.setColour(juce::Label::backgroundColourId, juce::Colour(0xff064e3b));
        sessionStatusBadge.setColour(juce::Label::textColourId, juce::Colour(0xff34d399));
    }
    else if (role == SessionRole::HOST)
    {
        sessionStatusBadge.setText("HOSTING: WAITING FOR PEER", juce::dontSendNotification);
        sessionStatusBadge.setColour(juce::Label::backgroundColourId, juce::Colour(0xff78350f));
        sessionStatusBadge.setColour(juce::Label::textColourId, juce::Colour(0xfffbbf24));
    }
    else if (role == SessionRole::CLIENT)
    {
        sessionStatusBadge.setText("CONNECTING...", juce::dontSendNotification);
        sessionStatusBadge.setColour(juce::Label::backgroundColourId, juce::Colour(0xff1e3a8a));
        sessionStatusBadge.setColour(juce::Label::textColourId, juce::Colour(0xff60a5fa));
    }
    else
    {
        sessionStatusBadge.setText("STANDALONE / OFFLINE", juce::dontSendNotification);
        sessionStatusBadge.setColour(juce::Label::backgroundColourId, juce::Colour(0xff1e293b));
        sessionStatusBadge.setColour(juce::Label::textColourId, juce::Colour(0xff94a3b8));
    }

    auto& stats = transport.getStats();
    float rtt = stats.getRttMs();
    float jitter = stats.getJitterMs();
    float loss = stats.getPacketLossPercent();
    float total = stats.getTotalLatencyMs();
    auto health = stats.getHealthStatus();

    if (isConn)
    {
        hudPingLabel.setText(juce::String::formatted("Ping: %.1f ms", rtt), juce::dontSendNotification);
        hudJitterLabel.setText(juce::String::formatted("Jitter: %.1f ms", jitter), juce::dontSendNotification);
        hudLossLabel.setText(juce::String::formatted("Loss: %.1f%%", loss), juce::dontSendNotification);
        hudTotalLatencyLabel.setText(juce::String::formatted("Total Ear-to-Ear: %.1f ms", total), juce::dontSendNotification);

        switch (health)
        {
            case ConnectionHealth::EXCELLENT:
                healthStatusPill.setText("EXCELLENT (<15ms)", juce::dontSendNotification);
                healthStatusPill.setColour(juce::Label::backgroundColourId, juce::Colour(0xff064e3b));
                healthStatusPill.setColour(juce::Label::textColourId, juce::Colour(0xff34d399));
                break;
            case ConnectionHealth::GOOD:
                healthStatusPill.setText("GOOD (<25ms)", juce::dontSendNotification);
                healthStatusPill.setColour(juce::Label::backgroundColourId, juce::Colour(0xff0c4a6e));
                healthStatusPill.setColour(juce::Label::textColourId, juce::Colour(0xff38bdf8));
                break;
            case ConnectionHealth::FAIR:
                healthStatusPill.setText("FAIR (<35ms)", juce::dontSendNotification);
                healthStatusPill.setColour(juce::Label::backgroundColourId, juce::Colour(0xff78350f));
                healthStatusPill.setColour(juce::Label::textColourId, juce::Colour(0xfffbbf24));
                break;
            case ConnectionHealth::POOR:
                healthStatusPill.setText("POOR (HIGH LATENCY)", juce::dontSendNotification);
                healthStatusPill.setColour(juce::Label::backgroundColourId, juce::Colour(0xff7f1d1d));
                healthStatusPill.setColour(juce::Label::textColourId, juce::Colour(0xfff87171));
                break;
            default:
                break;
        }
    }
    else
    {
        hudPingLabel.setText("Ping: -- ms", juce::dontSendNotification);
        hudJitterLabel.setText("Jitter: -- ms", juce::dontSendNotification);
        hudLossLabel.setText("Loss: 0.0%", juce::dontSendNotification);
        hudTotalLatencyLabel.setText("Total Ear-to-Ear: -- ms", juce::dontSendNotification);
        healthStatusPill.setText("OFFLINE", juce::dontSendNotification);
        healthStatusPill.setColour(juce::Label::backgroundColourId, juce::Colour(0xff334155));
        healthStatusPill.setColour(juce::Label::textColourId, juce::Colour(0xffcbd5e1));
    }
}

void SessionPanel::timerCallback()
{
    updateNetworkStatsDisplay();
}

void SessionPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff14171d));
    g.setColour(juce::Colour(0xff27272a));
    g.drawRect(getLocalBounds(), 1);
}

void SessionPanel::resized()
{
    auto area = getLocalBounds().reduced(10, 6);

    // Top Header & Mode
    auto topRow = area.removeFromTop(24);
    headerLabel.setBounds(topRow.removeFromLeft(330));
    internetModeButton.setBounds(topRow.removeFromRight(130));
    topRow.removeFromRight(6);
    lanModeButton.setBounds(topRow.removeFromRight(130));

    area.removeFromTop(4);

    // Warning Banner if visible
    if (adapterWarningLabel.isVisible())
    {
        adapterWarningLabel.setBounds(area.removeFromTop(20));
        area.removeFromTop(4);
    }

    // Two Columns: Host (Left 35%) & Join (Right 65%)
    auto body = area.removeFromTop(62);
    auto leftCol = body.removeFromLeft(body.getWidth() * 0.36);
    body.removeFromLeft(12);
    auto rightCol = body;

    // Host Column
    hostSectionLabel.setBounds(leftCol.removeFromTop(18));
    auto hostBtnRow = leftCol.removeFromTop(26);
    hostSessionButton.setBounds(hostBtnRow.removeFromLeft(110));
    hostBtnRow.removeFromLeft(8);
    hostStatusLabel.setBounds(hostBtnRow);

    // Join Column
    joinSectionLabel.setBounds(rightCol.removeFromTop(18));
    auto joinRow1 = rightCol.removeFromTop(26);
    refreshBrowseButton.setBounds(joinRow1.removeFromLeft(75));
    joinRow1.removeFromLeft(6);
    discoveredSessionsCombo.setBounds(joinRow1.removeFromLeft(190));
    joinRow1.removeFromLeft(6);
    joinDiscoveredButton.setBounds(joinRow1.removeFromLeft(105));
    joinRow1.removeFromLeft(10);

    // Direct manual connect
    manualIpLabel.setBounds(joinRow1.removeFromLeft(20));
    manualIpEditor.setBounds(joinRow1.removeFromLeft(80));
    joinRow1.removeFromLeft(4);
    manualPortLabel.setBounds(joinRow1.removeFromLeft(30));
    manualPortEditor.setBounds(joinRow1.removeFromLeft(45));
    joinRow1.removeFromLeft(6);
    connectManualButton.setBounds(joinRow1);

    area.removeFromTop(4);

    // Bottom HUD Row: Diagnostics + Status Badge
    auto hudRow = area.removeFromTop(26);
    sessionStatusBadge.setBounds(hudRow.removeFromLeft(190));
    hudRow.removeFromLeft(6);
    disconnectButton.setBounds(hudRow.removeFromLeft(95));
    hudRow.removeFromLeft(12);

    hudPingLabel.setBounds(hudRow.removeFromLeft(90));
    hudRow.removeFromLeft(8);
    hudJitterLabel.setBounds(hudRow.removeFromLeft(90));
    hudRow.removeFromLeft(8);
    hudLossLabel.setBounds(hudRow.removeFromLeft(80));
    hudRow.removeFromLeft(8);
    hudTotalLatencyLabel.setBounds(hudRow.removeFromLeft(150));
    hudRow.removeFromLeft(8);
    healthStatusPill.setBounds(hudRow);
}

} // namespace paasphoos
