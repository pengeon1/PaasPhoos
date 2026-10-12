#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../audio/SessionRecorder.h"
#include "../session/SetlistManager.h"
#include "../session/ChatTalkback.h"
#include "../network/NetworkSimulator.h"
#include "../audio/AudioEngine.h"

namespace paasphoos
{

class RehearsalToolsPanel : public juce::Component,
                            public juce::ChangeListener,
                            public juce::Timer
{
public:
    RehearsalToolsPanel(SessionRecorder& recorder,
                        SetlistManager& setlist,
                        ChatTalkback& chat,
                        NetworkSimulator& sim,
                        AudioEngine& engine);
    ~RehearsalToolsPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void timerCallback() override;

private:
    void refreshSongDetails();
    void refreshChatLog();

    SessionRecorder& recorder;
    SetlistManager& setlistManager;
    ChatTalkback& chatTalkback;
    NetworkSimulator& networkSimulator;
    AudioEngine& audioEngine;

    juce::Label headerLabel;

    // 1. Multitrack Recorder Controls
    juce::TextButton recordButton;
    juce::Label recordTimerLabel;

    // 2. Setlist Manager Controls
    juce::Label setlistLabel;
    juce::ComboBox songCombo;
    juce::TextButton prevSongButton;
    juce::TextButton nextSongButton;
    juce::Label songInfoLabel;
    juce::TextEditor songNotesEditor;

    // 3. Chat & Push-to-Talk
    juce::Label chatLabel;
    juce::TextEditor chatLogBox;
    juce::TextEditor chatInputEditor;
    juce::TextButton sendChatButton;
    juce::TextButton pttButton;

    // 4. Latency Budget & Network Sim
    juce::Label latencyBudgetLabel;
    juce::ToggleButton netSimToggle;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RehearsalToolsPanel)
};

} // namespace paasphoos
