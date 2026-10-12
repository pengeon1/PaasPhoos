#include "SessionPanel.h"
#include "../network/StunClient.h"

namespace paasphoos
{

SessionPanel::SessionPanel(AudioEngine& engine)
    : audioEngine(engine)
{
    audioEngine.getLanDiscovery().addChangeListener(this);
    audioEngine.getLanTransport().addChangeListener(this);

    // Header & Badges
    headerLabel.setText("BAND SESSION & NETWORK", juce::dontSendNotification);
    headerLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    headerLabel.setColour(juce::Label::textColourId, juce::Colour(0xffff8800));
    addAndMakeVisible(headerLabel);

    bandCountBadge.setText("BAND: 1/6", juce::dontSendNotification);
    bandCountBadge.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    bandCountBadge.setJustificationType(juce::Justification::centred);
    bandCountBadge.setColour(juce::Label::backgroundColourId, juce::Colour(0xff1e130c));
    bandCountBadge.setColour(juce::Label::textColourId, juce::Colour(0xffff9933));
    addAndMakeVisible(bandCountBadge);

    // Method Selector (Jam Code vs Direct IP)
    jamCodeModeButton.setButtonText("JAM CODE");
    jamCodeModeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff6600));
    jamCodeModeButton.onClick = [this]()
    {
        isDirectIpMode = false;
        jamCodeModeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff6600));
        directIpModeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff181920));
        resized();
    };
    addAndMakeVisible(jamCodeModeButton);

    directIpModeButton.setButtonText("DIRECT IP / LAN");
    directIpModeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff181920));
    directIpModeButton.onClick = [this]()
    {
        isDirectIpMode = true;
        directIpModeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff6600));
        jamCodeModeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff181920));
        resized();
    };
    addAndMakeVisible(directIpModeButton);

    // WAN vs LAN Network Mode
    internetModeButton.setButtonText("Internet (STUN)");
    internetModeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff6600));
    internetModeButton.onClick = [this]()
    {
        internetModeSelected = true;
        internetModeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff6600));
        lanModeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff181920));
    };
    addAndMakeVisible(internetModeButton);

    lanModeButton.setButtonText("Local LAN");
    lanModeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff181920));
    lanModeButton.onClick = [this]()
    {
        internetModeSelected = false;
        lanModeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff6600));
        internetModeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff181920));
    };
    addAndMakeVisible(lanModeButton);

    // ================= 1. JAM CODE HOST SECTION =================
    hostSectionLabel.setText("1. HOST A BAND REHEARSAL", juce::dontSendNotification);
    hostSectionLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    hostSectionLabel.setColour(juce::Label::textColourId, juce::Colour(0xffff9933));
    addAndMakeVisible(hostSectionLabel);

    hostSessionButton.setButtonText("START HOSTING");
    hostSessionButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff993d00));
    hostSessionButton.onClick = [this]()
    {
        bool isHosting = audioEngine.getLanDiscovery().isHosting();
        if (isHosting)
        {
            audioEngine.getLanDiscovery().stop();
            audioEngine.getLanTransport().disconnect();
            hostSessionButton.setButtonText("START HOSTING");
            hostSessionButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff993d00));
            jamCodeDisplayBadge.setText("JAM CODE: (Not Hosting)", juce::dontSendNotification);
            hostStatusLabel.setText("Hosting stopped.", juce::dontSendNotification);
            currentActiveJamCode = "";
        }
        else
        {
            adapterInfo = NetworkAdapterDetector::detectPrimaryAdapter();
            int port = LanAudioTransport::DEFAULT_AUDIO_PORT;
            audioEngine.getLanTransport().startAsHost(port, "Host Leader", "Guitar 1");
            audioEngine.getLanDiscovery().startHosting("PaasPhoos Band Jam", "Host Leader", port);
            hostSessionButton.setButtonText("STOP HOSTING");
            hostSessionButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffb91c1c));

            auto localCode = StunClient::generateRoomCode(adapterInfo.ipAddress, port);
            currentActiveJamCode = localCode;
            jamCodeDisplayBadge.setText("JAM CODE: " + localCode, juce::dontSendNotification);

            if (internetModeSelected)
            {
                hostStatusLabel.setText("Resolving WAN public code via STUN...", juce::dontSendNotification);

                juce::Thread::launch([this, port, localIp = adapterInfo.ipAddress, localCode]()
                {
                    auto stunRes = StunClient::resolvePublicEndpoint(port);
                    juce::MessageManager::callAsync([this, stunRes, port, localIp, localCode]()
                    {
                        if (stunRes.success)
                        {
                            currentActiveJamCode = stunRes.roomCode;
                            jamCodeDisplayBadge.setText("JAM CODE: " + stunRes.roomCode, juce::dontSendNotification);
                            hostStatusLabel.setText("Ready! Share code with bandmates (" + stunRes.publicIp + ":" + juce::String(stunRes.publicPort) + ")",
                                                   juce::dontSendNotification);
                        }
                        else
                        {
                            currentActiveJamCode = localCode;
                            jamCodeDisplayBadge.setText("JAM CODE: " + localCode, juce::dontSendNotification);
                            hostStatusLabel.setText("Ready on LAN (" + localIp + ":" + juce::String(port) + ")", juce::dontSendNotification);
                        }
                    });
                });
            }
            else
            {
                hostStatusLabel.setText("Ready on LAN (" + adapterInfo.ipAddress + ":" + juce::String(port) + ")", juce::dontSendNotification);
            }
        }
    };
    addAndMakeVisible(hostSessionButton);

    jamCodeDisplayBadge.setText("JAM CODE: (Click Start Hosting)", juce::dontSendNotification);
    jamCodeDisplayBadge.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    jamCodeDisplayBadge.setJustificationType(juce::Justification::centred);
    jamCodeDisplayBadge.setColour(juce::Label::backgroundColourId, juce::Colour(0xff16171f));
    jamCodeDisplayBadge.setColour(juce::Label::textColourId, juce::Colour(0xffffaa33));
    addAndMakeVisible(jamCodeDisplayBadge);

    copyCodeButton.setButtonText("COPY CODE");
    copyCodeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2c38));
    copyCodeButton.onClick = [this]()
    {
        if (currentActiveJamCode.isNotEmpty())
        {
            juce::SystemClipboard::copyTextToClipboard(currentActiveJamCode);
            copyCodeButton.setButtonText("COPIED! \xE2\x9C\x93");
            copyFeedbackTimer = 20; // 2 seconds at 10Hz
        }
    };
    addAndMakeVisible(copyCodeButton);

    hostStatusLabel.setText("Share the Jam Code with your band to connect.", juce::dontSendNotification);
    hostStatusLabel.setFont(juce::FontOptions(10.5f));
    hostStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff9ca3af));
    addAndMakeVisible(hostStatusLabel);

    // ================= 2. JAM CODE JOIN SECTION =================
    joinSectionLabel.setText("2. JOIN WITH JAM CODE", juce::dontSendNotification);
    joinSectionLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    joinSectionLabel.setColour(juce::Label::textColourId, juce::Colour(0xffff9933));
    addAndMakeVisible(joinSectionLabel);

    jamCodeEditor.setTextToShowWhenEmpty("Paste Band Jam Code (e.g. JAM-C0A8...)", juce::Colour(0xff6b7280));
    jamCodeEditor.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    jamCodeEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff121318));
    jamCodeEditor.setColour(juce::TextEditor::textColourId, juce::Colour(0xffffaa33));
    jamCodeEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff2d2f3d));
    jamCodeEditor.onReturnKey = [this]()
    {
        executeJoinWithCode(jamCodeEditor.getText().trim());
    };
    addAndMakeVisible(jamCodeEditor);

    joinWithCodeButton.setButtonText("JOIN JAM");
    joinWithCodeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff6600));
    joinWithCodeButton.onClick = [this]()
    {
        executeJoinWithCode(jamCodeEditor.getText().trim());
    };
    addAndMakeVisible(joinWithCodeButton);

    pasteAndJoinButton.setButtonText("PASTE & JOIN");
    pasteAndJoinButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff333644));
    pasteAndJoinButton.onClick = [this]()
    {
        juce::String clip = juce::SystemClipboard::getTextFromClipboard().trim();
        if (clip.isNotEmpty())
        {
            jamCodeEditor.setText(clip, juce::dontSendNotification);
            executeJoinWithCode(clip);
        }
    };
    addAndMakeVisible(pasteAndJoinButton);

    // ================= 3. ADVANCED DIRECT IP CONTROLS =================
    manualIpLabel.setText("IP:", juce::dontSendNotification);
    manualIpLabel.setFont(juce::FontOptions(11.0f));
    addChildComponent(manualIpLabel);

    manualIpEditor.setText("127.0.0.1");
    addChildComponent(manualIpEditor);

    manualPortLabel.setText("Port:", juce::dontSendNotification);
    manualPortLabel.setFont(juce::FontOptions(11.0f));
    addChildComponent(manualPortLabel);

    manualPortEditor.setText("44045");
    addChildComponent(manualPortEditor);

    connectManualButton.setButtonText("CONNECT IP");
    connectManualButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff6600));
    connectManualButton.onClick = [this]()
    {
        juce::String ip = manualIpEditor.getText().trim();
        int port = manualPortEditor.getText().getIntValue();
        if (port <= 0) port = 44045;
        audioEngine.getLanTransport().startAsClient(ip, port, "Musician", "Band Member");
    };
    addChildComponent(connectManualButton);

    discoveredSessionsCombo.setTextWhenNoChoicesAvailable("No LAN sessions found yet");
    discoveredSessionsCombo.setTextWhenNothingSelected("Select discovered LAN session...");
    addChildComponent(discoveredSessionsCombo);

    joinDiscoveredButton.setButtonText("JOIN SELECTED");
    joinDiscoveredButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff6600));
    joinDiscoveredButton.onClick = [this]()
    {
        auto sessions = audioEngine.getLanDiscovery().getDiscoveredSessions();
        int idx = discoveredSessionsCombo.getSelectedId() - 1;
        if (idx >= 0 && idx < static_cast<int>(sessions.size()))
        {
            auto& s = sessions[idx];
            audioEngine.getLanTransport().startAsClient(s.ipAddress, s.audioPort, "Musician", "Guitar 2");
        }
    };
    addChildComponent(joinDiscoveredButton);

    refreshBrowseButton.setButtonText("SCAN LAN");
    refreshBrowseButton.onClick = [this]()
    {
        audioEngine.getLanDiscovery().startBrowsing();
        refreshDiscoveredList();
    };
    addChildComponent(refreshBrowseButton);

    // ================= 4. STATUS & CONTROLS =================
    sessionStatusBadge.setText("STATUS: STANDALONE", juce::dontSendNotification);
    sessionStatusBadge.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    sessionStatusBadge.setJustificationType(juce::Justification::centred);
    sessionStatusBadge.setColour(juce::Label::backgroundColourId, juce::Colour(0xff181920));
    sessionStatusBadge.setColour(juce::Label::textColourId, juce::Colour(0xff9ca3af));
    addAndMakeVisible(sessionStatusBadge);

    disconnectButton.setButtonText("DISCONNECT");
    disconnectButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff7f1d1d));
    disconnectButton.onClick = [this]()
    {
        audioEngine.getLanDiscovery().stop();
        audioEngine.getLanTransport().disconnect();
        hostSessionButton.setButtonText("START HOSTING");
        hostSessionButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff993d00));
        hostStatusLabel.setText("Disconnected.", juce::dontSendNotification);
        jamCodeDisplayBadge.setText("JAM CODE: (Not Hosting)", juce::dontSendNotification);
        currentActiveJamCode = "";
    };
    addAndMakeVisible(disconnectButton);

    // Jitter buffer settings
    jitterBufferLabel.setText("Jitter Buffer:", juce::dontSendNotification);
    jitterBufferLabel.setFont(juce::FontOptions(10.5f));
    jitterBufferLabel.setColour(juce::Label::textColourId, juce::Colour(0xff9ca3af));
    addAndMakeVisible(jitterBufferLabel);

    jitterBufferCombo.addItem("Auto (Adaptive RFC 3550)", 1);
    jitterBufferCombo.addItem("Manual: 4 ms (Ultra LAN)", 2);
    jitterBufferCombo.addItem("Manual: 8 ms (Default LAN)", 3);
    jitterBufferCombo.addItem("Manual: 15 ms (Wi-Fi/LAN)", 4);
    jitterBufferCombo.addItem("Manual: 25 ms (Safe)", 5);
    jitterBufferCombo.addItem("Manual: 40 ms (WAN)", 6);
    jitterBufferCombo.setSelectedId(1, juce::dontSendNotification);
    jitterBufferCombo.onChange = [this]() { applyJitterBufferSettings(); };
    addAndMakeVisible(jitterBufferCombo);

    // Live Network Diagnostics HUD
    hudPingLabel.setText("Ping: -- ms", juce::dontSendNotification);
    hudPingLabel.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    hudPingLabel.setColour(juce::Label::textColourId, juce::Colour(0xffffaa33));
    addAndMakeVisible(hudPingLabel);

    hudJitterLabel.setText("Jitter: -- ms", juce::dontSendNotification);
    hudJitterLabel.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    hudJitterLabel.setColour(juce::Label::textColourId, juce::Colour(0xffffaa33));
    addAndMakeVisible(hudJitterLabel);

    hudLossLabel.setText("Loss: 0.0%", juce::dontSendNotification);
    hudLossLabel.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    hudLossLabel.setColour(juce::Label::textColourId, juce::Colour(0xffffaa33));
    addAndMakeVisible(hudLossLabel);

    hudTotalLatencyLabel.setText("Total Ear-to-Ear: -- ms", juce::dontSendNotification);
    hudTotalLatencyLabel.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    hudTotalLatencyLabel.setColour(juce::Label::textColourId, juce::Colour(0xffff8800));
    addAndMakeVisible(hudTotalLatencyLabel);

    healthStatusPill.setText("EXCELLENT", juce::dontSendNotification);
    healthStatusPill.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    healthStatusPill.setJustificationType(juce::Justification::centred);
    healthStatusPill.setColour(juce::Label::backgroundColourId, juce::Colour(0xff14532d));
    healthStatusPill.setColour(juce::Label::textColourId, juce::Colour(0xff4ade80));
    addAndMakeVisible(healthStatusPill);

    // Wi-Fi Warning
    adapterWarningLabel.setText("", juce::dontSendNotification);
    adapterWarningLabel.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    adapterWarningLabel.setJustificationType(juce::Justification::centred);
    adapterWarningLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff451a03));
    adapterWarningLabel.setColour(juce::Label::textColourId, juce::Colour(0xfffbbf24));
    addChildComponent(adapterWarningLabel);

    updateAdapterWarning();
    startTimerHz(10);
}

SessionPanel::~SessionPanel()
{
    stopTimer();
    audioEngine.getLanDiscovery().removeChangeListener(this);
    audioEngine.getLanTransport().removeChangeListener(this);
}

void SessionPanel::executeJoinWithCode(const juce::String& code)
{
    juce::String clean = code.trim();
    if (clean.isEmpty()) return;

    juce::String outIp;
    int outPort = 0;
    if (StunClient::parseRoomCode(clean, outIp, outPort))
    {
        audioEngine.getLanTransport().startAsClient(outIp, outPort, "Musician", "Band Member");
    }
    else
    {
        // Try fallback raw direct host connect
        int defaultPort = 44045;
        audioEngine.getLanTransport().startAsClient(clean, defaultPort, "Musician", "Band Member");
    }
}

void SessionPanel::applyJitterBufferSettings()
{
    int id = jitterBufferCombo.getSelectedId();
    auto& transport = audioEngine.getLanTransport();
    JitterBufferMode mode = (id == 1) ? JitterBufferMode::AUTO : JitterBufferMode::MANUAL;
    float delay = 8.0f;
    switch (id)
    {
        case 2: delay = 4.0f; break;
        case 3: delay = 8.0f; break;
        case 4: delay = 15.0f; break;
        case 5: delay = 25.0f; break;
        case 6: delay = 40.0f; break;
        default: break;
    }
    for (int p = 0; p < AudioEngine::MAX_REMOTE_STRIPS; ++p)
    {
        transport.setJitterBufferMode(p, mode);
        if (mode == JitterBufferMode::MANUAL)
            transport.setManualTargetDelayMs(p, delay);
    }
}

void SessionPanel::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &audioEngine.getLanDiscovery())
    {
        refreshDiscoveredList();
    }
    else if (source == &audioEngine.getLanTransport())
    {
        updateNetworkStatsDisplay();
    }
}

void SessionPanel::timerCallback()
{
    updateNetworkStatsDisplay();

    if (copyFeedbackTimer > 0)
    {
        --copyFeedbackTimer;
        if (copyFeedbackTimer == 0)
            copyCodeButton.setButtonText("COPY CODE");
    }
}

void SessionPanel::refreshDiscoveredList()
{
    discoveredSessionsCombo.clear();
    auto sessions = audioEngine.getLanDiscovery().getDiscoveredSessions();
    for (size_t i = 0; i < sessions.size(); ++i)
    {
        auto& s = sessions[i];
        juce::String text = s.sessionName + " (" + s.ipAddress + ":" + juce::String(s.audioPort) + ") ["
                          + juce::String(s.participantCount) + "/" + juce::String(s.maxParticipants) + "]";
        discoveredSessionsCombo.addItem(text, static_cast<int>(i + 1));
    }
}

void SessionPanel::updateAdapterWarning()
{
    adapterInfo = NetworkAdapterDetector::detectPrimaryAdapter();
    adapterWarningLabel.setVisible(false);
}

void SessionPanel::updateNetworkStatsDisplay()
{
    auto& transport = audioEngine.getLanTransport();
    bool isConn = transport.isConnected();
    auto role = transport.getRole();
    int activeCount = transport.getActivePeerCount();

    bandCountBadge.setText(juce::String::formatted("BAND: %d/6", activeCount), juce::dontSendNotification);

    if (isConn)
    {
        juce::String roleText = (role == SessionRole::HOST) ? "HOSTING" : "CONNECTED";
        sessionStatusBadge.setText(roleText + " (" + transport.getRemoteAddress() + ") [" + juce::String(activeCount) + "/6]", juce::dontSendNotification);
        sessionStatusBadge.setColour(juce::Label::backgroundColourId, juce::Colour(0xff14532d));
        sessionStatusBadge.setColour(juce::Label::textColourId, juce::Colour(0xff4ade80));
    }
    else if (role == SessionRole::HOST)
    {
        sessionStatusBadge.setText("HOSTING: WAITING FOR PEERS", juce::dontSendNotification);
        sessionStatusBadge.setColour(juce::Label::backgroundColourId, juce::Colour(0xff451a03));
        sessionStatusBadge.setColour(juce::Label::textColourId, juce::Colour(0xffffaa33));
    }
    else
    {
        sessionStatusBadge.setText("STATUS: STANDALONE", juce::dontSendNotification);
        sessionStatusBadge.setColour(juce::Label::backgroundColourId, juce::Colour(0xff181920));
        sessionStatusBadge.setColour(juce::Label::textColourId, juce::Colour(0xff9ca3af));
    }

    if (isConn)
    {
        const auto& stats = transport.getStats();
        float rtt = stats.getRttMs();
        float jitter = stats.getJitterMs();
        float loss = stats.getPacketLossPercent();
        float total = stats.getTotalLatencyMs();

        hudPingLabel.setText("Ping: " + juce::String(rtt, 1) + " ms", juce::dontSendNotification);
        hudJitterLabel.setText("Jitter: " + juce::String(jitter, 1) + " ms", juce::dontSendNotification);
        hudLossLabel.setText("Loss: " + juce::String(loss, 1) + "%", juce::dontSendNotification);
        hudTotalLatencyLabel.setText("Total Ear-to-Ear: " + juce::String(total, 1) + " ms", juce::dontSendNotification);

        if (total < 25.0f && loss < 2.0f)
        {
            healthStatusPill.setText("EXCELLENT", juce::dontSendNotification);
            healthStatusPill.setColour(juce::Label::backgroundColourId, juce::Colour(0xff14532d));
            healthStatusPill.setColour(juce::Label::textColourId, juce::Colour(0xff4ade80));
        }
        else if (total < 45.0f && loss < 5.0f)
        {
            healthStatusPill.setText("GOOD", juce::dontSendNotification);
            healthStatusPill.setColour(juce::Label::backgroundColourId, juce::Colour(0xff365314));
            healthStatusPill.setColour(juce::Label::textColourId, juce::Colour(0xffa3e635));
        }
        else
        {
            healthStatusPill.setText("DEGRADED", juce::dontSendNotification);
            healthStatusPill.setColour(juce::Label::backgroundColourId, juce::Colour(0xff7f1d1d));
            healthStatusPill.setColour(juce::Label::textColourId, juce::Colour(0xfff87171));
        }
    }
}

void SessionPanel::paint(juce::Graphics& g)
{
    // Sleek Obsidian Black surface with warm orange trim
    g.fillAll(juce::Colour(0xff0c0d11));

    // Outer subtle border
    g.setColour(juce::Colour(0xff22242e));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 6.0f, 1.0f);

    // Glowing Orange Accent strip along top
    g.setColour(juce::Colour(0xffff6600));
    g.fillRoundedRectangle(2.0f, 2.0f, (float)getWidth() - 4.0f, 2.0f, 1.0f);
}

void SessionPanel::resized()
{
    auto area = getLocalBounds().reduced(8, 6);

    // Top Header & Mode bar
    auto topBar = area.removeFromTop(24);
    headerLabel.setBounds(topBar.removeFromLeft(190));
    bandCountBadge.setBounds(topBar.removeFromLeft(80).reduced(2, 2));

    // Method switch: Jam Code vs Direct IP
    jamCodeModeButton.setBounds(topBar.removeFromLeft(105).reduced(2, 1));
    directIpModeButton.setBounds(topBar.removeFromLeft(145).reduced(2, 1));

    // Right-aligned status badges
    disconnectButton.setBounds(topBar.removeFromRight(95).reduced(2, 1));
    sessionStatusBadge.setBounds(topBar.removeFromRight(190).reduced(2, 1));

    // Network mode selector (Internet / LAN)
    internetModeButton.setBounds(topBar.removeFromRight(110).reduced(2, 1));
    lanModeButton.setBounds(topBar.removeFromRight(90).reduced(2, 1));

    area.removeFromTop(4);

    if (!isDirectIpMode)
    {
        // ================= JAM CODE PRIMARY VIEW =================
        manualIpLabel.setVisible(false);
        manualIpEditor.setVisible(false);
        manualPortLabel.setVisible(false);
        manualPortEditor.setVisible(false);
        connectManualButton.setVisible(false);
        discoveredSessionsCombo.setVisible(false);
        joinDiscoveredButton.setVisible(false);
        refreshBrowseButton.setVisible(false);

        hostSectionLabel.setVisible(true);
        hostSessionButton.setVisible(true);
        jamCodeDisplayBadge.setVisible(true);
        copyCodeButton.setVisible(true);
        hostStatusLabel.setVisible(true);
        joinSectionLabel.setVisible(true);
        jamCodeEditor.setVisible(true);
        joinWithCodeButton.setVisible(true);
        pasteAndJoinButton.setVisible(true);

        auto mainRow = area.removeFromTop(44);

        // Host Column (Left 46%)
        auto hostCol = mainRow.removeFromLeft(static_cast<int>(mainRow.getWidth() * 0.46));
        hostSectionLabel.setBounds(hostCol.removeFromTop(15));
        auto hostActionRow = hostCol;
        hostSessionButton.setBounds(hostActionRow.removeFromLeft(125).reduced(1));
        copyCodeButton.setBounds(hostActionRow.removeFromRight(95).reduced(1));
        jamCodeDisplayBadge.setBounds(hostActionRow.reduced(1));

        mainRow.removeFromLeft(14); // Separator spacing

        // Join Column (Right 54%)
        auto joinCol = mainRow;
        joinSectionLabel.setBounds(joinCol.removeFromTop(15));
        auto joinActionRow = joinCol;
        joinWithCodeButton.setBounds(joinActionRow.removeFromRight(85).reduced(1));
        pasteAndJoinButton.setBounds(joinActionRow.removeFromRight(105).reduced(1));
        jamCodeEditor.setBounds(joinActionRow.reduced(1));

        area.removeFromTop(4);

        // Bottom sub-row: Host status subtext & Live Diagnostics HUD
        auto bottomRow = area.removeFromTop(22);
        hostStatusLabel.setBounds(bottomRow.removeFromLeft(static_cast<int>(bottomRow.getWidth() * 0.46)));
        bottomRow.removeFromLeft(14);

        // Live HUD in bottom right
        healthStatusPill.setBounds(bottomRow.removeFromRight(80).reduced(1));
        hudTotalLatencyLabel.setBounds(bottomRow.removeFromRight(150).reduced(1));
        hudLossLabel.setBounds(bottomRow.removeFromRight(75).reduced(1));
        hudJitterLabel.setBounds(bottomRow.removeFromRight(85).reduced(1));
        hudPingLabel.setBounds(bottomRow.removeFromRight(85).reduced(1));
    }
    else
    {
        // ================= DIRECT IP & LAN DISCOVERY VIEW =================
        hostSectionLabel.setVisible(false);
        jamCodeDisplayBadge.setVisible(false);
        copyCodeButton.setVisible(false);
        hostStatusLabel.setVisible(false);
        joinSectionLabel.setVisible(false);
        jamCodeEditor.setVisible(false);
        joinWithCodeButton.setVisible(false);
        pasteAndJoinButton.setVisible(false);

        manualIpLabel.setVisible(true);
        manualIpEditor.setVisible(true);
        manualPortLabel.setVisible(true);
        manualPortEditor.setVisible(true);
        connectManualButton.setVisible(true);
        discoveredSessionsCombo.setVisible(true);
        joinDiscoveredButton.setVisible(true);
        refreshBrowseButton.setVisible(true);

        auto mainRow = area.removeFromTop(36);
        hostSessionButton.setBounds(mainRow.removeFromLeft(125).reduced(1));

        // Direct IP inputs
        manualIpLabel.setBounds(mainRow.removeFromLeft(24));
        manualIpEditor.setBounds(mainRow.removeFromLeft(120).reduced(1));
        manualPortLabel.setBounds(mainRow.removeFromLeft(34));
        manualPortEditor.setBounds(mainRow.removeFromLeft(55).reduced(1));
        connectManualButton.setBounds(mainRow.removeFromLeft(105).reduced(1));

        mainRow.removeFromLeft(10);
        discoveredSessionsCombo.setBounds(mainRow.removeFromLeft(220).reduced(1));
        joinDiscoveredButton.setBounds(mainRow.removeFromLeft(110).reduced(1));
        refreshBrowseButton.setBounds(mainRow.removeFromLeft(85).reduced(1));

        // Bottom row diagnostics HUD
        auto bottomRow = area.removeFromTop(20);
        healthStatusPill.setBounds(bottomRow.removeFromRight(80).reduced(1));
        hudTotalLatencyLabel.setBounds(bottomRow.removeFromRight(150).reduced(1));
        hudLossLabel.setBounds(bottomRow.removeFromRight(75).reduced(1));
        hudJitterLabel.setBounds(bottomRow.removeFromRight(85).reduced(1));
        hudPingLabel.setBounds(bottomRow.removeFromRight(85).reduced(1));
    }
}

} // namespace paasphoos
