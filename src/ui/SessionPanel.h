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

    AudioEngine& audioEngine;

    juce::Label headerLabel;
    juce::Label bandCountBadge;

    // Mode Selector
    juce::TextButton lanModeButton;
    juce::TextButton internetModeButton;

    // Host section
    juce::Label hostSectionLabel;
    juce::TextButton hostSessionButton;
    juce::Label hostStatusLabel;

    // Join section
    juce::Label joinSectionLabel;
    juce::Label manualIpLabel;
    juce::TextEditor manualIpEditor;
    juce::Label manualPortLabel;
    juce::TextEditor manualPortEditor;
    juce::TextButton connectManualButton;

    // Discovered sessions
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SessionPanel)
};

} // namespace paasphoos
