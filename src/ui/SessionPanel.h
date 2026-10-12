#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../audio/AudioEngine.h"
#include "../network/NetworkAdapterDetector.h"

namespace paasphoos
{

class SessionPanel : public juce::Component,
                     public juce::ChangeListener,
                     public juce::Timer
{
public:
    explicit SessionPanel(AudioEngine& engine);
    ~SessionPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void timerCallback() override;

private:
    void refreshDiscoveredList();
    void updateNetworkStatsDisplay();
    void updateAdapterWarning();
    void applyJitterBufferSettings();
    void executeJoinWithCode(const juce::String& code);

    AudioEngine& audioEngine;

    // Header & Badges
    juce::Label headerLabel;
    juce::Label bandCountBadge;

    // Connection Method Selector (Jam Code vs Advanced IP)
    juce::TextButton jamCodeModeButton;
    juce::TextButton directIpModeButton;
    bool isDirectIpMode{false};

    // Mode Selector (LAN vs Internet)
    juce::TextButton lanModeButton;
    juce::TextButton internetModeButton;
    bool internetModeSelected{true}; // Default to Internet STUN for remote bands

    // ================= 1. JAM CODE WORKFLOW (PRIMARY) =================
    juce::Label hostSectionLabel;
    juce::TextButton hostSessionButton;
    juce::Label jamCodeDisplayBadge;
    juce::TextButton copyCodeButton;
    juce::Label hostStatusLabel;
    juce::String currentActiveJamCode;

    juce::Label joinSectionLabel;
    juce::TextEditor jamCodeEditor;
    juce::TextButton joinWithCodeButton;
    juce::TextButton pasteAndJoinButton;

    // ================= 2. DIRECT IP / LAN DISCOVERY (ADVANCED) =================
    juce::Label manualIpLabel;
    juce::TextEditor manualIpEditor;
    juce::Label manualPortLabel;
    juce::TextEditor manualPortEditor;
    juce::TextButton connectManualButton;

    juce::ComboBox discoveredSessionsCombo;
    juce::TextButton joinDiscoveredButton;
    juce::TextButton refreshBrowseButton;

    // Active session info
    juce::Label sessionStatusBadge;
    juce::TextButton disconnectButton;

    // Jitter buffer controls
    juce::Label jitterBufferLabel;
    juce::ComboBox jitterBufferCombo;

    // Live Network Diagnostics HUD
    juce::Label hudPingLabel;
    juce::Label hudJitterLabel;
    juce::Label hudLossLabel;
    juce::Label hudTotalLatencyLabel;
    juce::Label healthStatusPill;

    // Adapter & Isolation Warning Banner
    juce::Label adapterWarningLabel;

    NetworkAdapterInfo adapterInfo;
    int copyFeedbackTimer{0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SessionPanel)
};

} // namespace paasphoos
