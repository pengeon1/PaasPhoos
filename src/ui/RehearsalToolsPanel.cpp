#include "RehearsalToolsPanel.h"
#include "../common/Typography.h"
#include "../common/ThemeConfig.h"

namespace paasphoos
{

RehearsalToolsPanel::RehearsalToolsPanel(SessionRecorder& rec,
                                         SetlistManager& setlist,
                                         ChatTalkback& chat,
                                         NetworkSimulator& sim,
                                         AudioEngine& engine)
    : recorder(rec),
      setlistManager(setlist),
      chatTalkback(chat),
      networkSimulator(sim),
      audioEngine(engine)
{
    setlistManager.addChangeListener(this);
    chatTalkback.addChangeListener(this);

    headerLabel.setText("REHEARSAL TOOLS, SETLIST & CHAT (M7)", juce::dontSendNotification);
    headerLabel.setFont(Typography::h3());
    headerLabel.setColour(juce::Label::textColourId, juce::Colour(0xff3da5ff));
    addAndMakeVisible(headerLabel);

    // 1. Multitrack Recorder
    recordButton.setButtonText("START REC (WAV)");
    recordButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff6366f1));
    recordButton.onClick = [this]()
    {
        if (recorder.isRecording())
        {
            recorder.stopRecording();
            recordButton.setButtonText("START REC (WAV)");
            recordButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff6366f1));
            recordTimerLabel.setText("Saved to Desktop", juce::dontSendNotification);
        }
        else
        {
            auto dest = juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile("PaasPhoos_Recordings");
            if (recorder.startRecording(dest, audioEngine.getCurrentSampleRate()))
            {
                recordButton.setButtonText("STOP REC");
                recordButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff7c5cff));
            }
        }
    };
    addAndMakeVisible(recordButton);

    recordTimerLabel.setText("00:00", juce::dontSendNotification);
    recordTimerLabel.setFont(Typography::numeric(12.0f));
    recordTimerLabel.setColour(juce::Label::textColourId, juce::Colour(0xfff8fafc));
    addAndMakeVisible(recordTimerLabel);

    // 2. Setlist
    setlistLabel.setText("Setlist:", juce::dontSendNotification);
    setlistLabel.setFont(Typography::labelCaps(11.0f));
    setlistLabel.setColour(juce::Label::textColourId, juce::Colour(0xff7c5cff));
    addAndMakeVisible(setlistLabel);

    songCombo.onChange = [this]()
    {
        setlistManager.selectSong(songCombo.getSelectedId() - 1);
    };
    addAndMakeVisible(songCombo);

    prevSongButton.setButtonText("<");
    prevSongButton.onClick = [this]()
    {
        int cur = setlistManager.getCurrentSongIndex();
        if (cur > 0) setlistManager.selectSong(cur - 1);
    };
    addAndMakeVisible(prevSongButton);

    nextSongButton.setButtonText(">");
    nextSongButton.onClick = [this]()
    {
        int cur = setlistManager.getCurrentSongIndex();
        if (cur < setlistManager.getNumSongs() - 1) setlistManager.selectSong(cur + 1);
    };
    addAndMakeVisible(nextSongButton);

    songInfoLabel.setFont(Typography::numeric(11.0f));
    songInfoLabel.setColour(juce::Label::textColourId, juce::Colour(0xff38e1c6));
    addAndMakeVisible(songInfoLabel);

    songNotesEditor.setMultiLine(true);
    songNotesEditor.setReadOnly(true);
    songNotesEditor.setBorder(juce::BorderSize<int>(0));
    songNotesEditor.setIndents(8, 6);
    songNotesEditor.setFont(Typography::caption(11.0f));
    songNotesEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff0e1118));
    songNotesEditor.setColour(juce::TextEditor::textColourId, juce::Colour(ThemeConfig::kTextMutedHex));
    songNotesEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(ThemeConfig::kBorderSubtleHex));
    songNotesEditor.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(ThemeConfig::kAccentBlueHex));
    addAndMakeVisible(songNotesEditor);

    // 3. Chat & PTT
    chatLabel.setText("Band Chat:", juce::dontSendNotification);
    chatLabel.setFont(Typography::labelCaps(11.0f));
    chatLabel.setColour(juce::Label::textColourId, juce::Colour(0xff7c5cff));
    addAndMakeVisible(chatLabel);

    chatLogBox.setMultiLine(true);
    chatLogBox.setReadOnly(true);
    chatLogBox.setBorder(juce::BorderSize<int>(0));
    chatLogBox.setIndents(8, 6);
    chatLogBox.setFont(Typography::caption());
    chatLogBox.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff0e1118));
    chatLogBox.setColour(juce::TextEditor::textColourId, juce::Colour(ThemeConfig::kTextPrimaryHex));
    chatLogBox.setColour(juce::TextEditor::outlineColourId, juce::Colour(ThemeConfig::kBorderSubtleHex));
    chatLogBox.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(ThemeConfig::kAccentBlueHex));
    addAndMakeVisible(chatLogBox);

    chatInputEditor.setMultiLine(false);
    chatInputEditor.setReturnKeyStartsNewLine(false);
    chatInputEditor.setBorder(juce::BorderSize<int>(0));
    chatInputEditor.setIndents(10, 0);
    chatInputEditor.setFont(Typography::body().withHeight(13.0f));
    chatInputEditor.setTextToShowWhenEmpty("Type message...", juce::Colour(ThemeConfig::kTextDimHex));
    chatInputEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff0e1118));
    chatInputEditor.setColour(juce::TextEditor::textColourId, juce::Colour(ThemeConfig::kTextPrimaryHex));
    chatInputEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(ThemeConfig::kBorderSubtleHex));
    chatInputEditor.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(ThemeConfig::kAccentBlueHex));
    chatInputEditor.setColour(juce::CaretComponent::caretColourId, juce::Colour(ThemeConfig::kAccentBlueHex));
    chatInputEditor.setColour(juce::TextEditor::highlightColourId, juce::Colour(ThemeConfig::kAccentBlueHex).withAlpha(0.35f));
    chatInputEditor.onReturnKey = [this]()
    {
        sendChatButton.triggerClick();
    };
    addAndMakeVisible(chatInputEditor);

    sendChatButton.setButtonText("SEND");
    sendChatButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff7c5cff));
    sendChatButton.onClick = [this]()
    {
        auto txt = chatInputEditor.getText().trim();
        if (txt.isNotEmpty())
        {
            chatTalkback.addMessage("You", txt);
            chatInputEditor.clear();
        }
    };
    addAndMakeVisible(sendChatButton);

    pttButton.setButtonText("TALK (PTT)");
    pttButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff22252e));
    pttButton.onClick = [this]()
    {
        bool ptt = !chatTalkback.isPushToTalkActive();
        chatTalkback.setPushToTalkActive(ptt);
        pttButton.setColour(juce::TextButton::buttonColourId, ptt ? juce::Colour(0xff3da5ff) : juce::Colour(0xff22252e));
    };
    addAndMakeVisible(pttButton);

    // 4. Latency Budget & Sim
    latencyBudgetLabel.setFont(Typography::numeric(11.0f));
    latencyBudgetLabel.setColour(juce::Label::textColourId, juce::Colour(0xff38e1c6));
    addAndMakeVisible(latencyBudgetLabel);

    netSimToggle.setButtonText("Net Sim (+25ms / 2% Loss)");
    netSimToggle.setToggleState(false, juce::dontSendNotification);
    netSimToggle.onStateChange = [this]()
    {
        networkSimulator.setEnabled(netSimToggle.getToggleState());
    };
    addAndMakeVisible(netSimToggle);

    refreshSongDetails();
    refreshChatLog();
    startTimerHz(4); // 4 FPS timer for recorder clock
}

RehearsalToolsPanel::~RehearsalToolsPanel()
{
    stopTimer();
    setlistManager.removeChangeListener(this);
    chatTalkback.removeChangeListener(this);
}

void RehearsalToolsPanel::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &setlistManager)
        refreshSongDetails();
    else if (source == &chatTalkback)
        refreshChatLog();
}

void RehearsalToolsPanel::refreshSongDetails()
{
    songCombo.clear();
    for (int i = 0; i < setlistManager.getNumSongs(); ++i)
    {
        auto* s = setlistManager.getSong(i);
        songCombo.addItem(s->title, i + 1);
    }
    int cur = setlistManager.getCurrentSongIndex();
    songCombo.setSelectedId(cur + 1, juce::dontSendNotification);

    auto* song = setlistManager.getSong(cur);
    if (song != nullptr)
    {
        songInfoLabel.setText("Key: " + song->key + " | Tempo: " + juce::String(static_cast<int>(song->bpm)) + " BPM", juce::dontSendNotification);
        songNotesEditor.setText(song->notes, false);
        audioEngine.getMetronome().setBpm(song->bpm);
    }
}

void RehearsalToolsPanel::refreshChatLog()
{
    juce::String fullText;
    for (int i = 0; i < chatTalkback.getNumMessages(); ++i)
    {
        auto* m = chatTalkback.getMessage(i);
        fullText += "[" + m->timeString + "] " + m->sender + ": " + m->text + "\n";
    }
    chatLogBox.setText(fullText, false);
    chatLogBox.moveCaretToEnd();
}

void RehearsalToolsPanel::timerCallback()
{
    if (recorder.isRecording())
    {
        double sec = recorder.getRecordingDurationSeconds();
        int mins = static_cast<int>(sec) / 60;
        int secs = static_cast<int>(sec) % 60;
        recordTimerLabel.setText(juce::String::formatted("REC %02d:%02d", mins, secs), juce::dontSendNotification);
    }

    // Update latency budget text
    double hwIn = audioEngine.getHardwareLatencyMs() * 0.5;
    double hwOut = hwIn;
    float net = audioEngine.getLanTransport().getStats().getRttMs() * 0.5f;
    float jb = 4.0f;
    float total = static_cast<float>(hwIn + 0.3 + net + jb + hwOut);

    latencyBudgetLabel.setText(
        juce::String::formatted("Budget: In %.1fms + FX 0.3ms + Net %.1fms + JB %.1fms + Out %.1fms = Total %.1f ms",
                                hwIn, net, jb, hwOut, total),
        juce::dontSendNotification
    );
}

void RehearsalToolsPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff090a0d));
    g.setColour(juce::Colour(0xff22252e));
    g.drawRect(getLocalBounds(), 1);
}

void RehearsalToolsPanel::resized()
{
    auto area = getLocalBounds().reduced(8, 6);

    // Header & Budget
    auto topRow = area.removeFromTop(22);
    headerLabel.setBounds(topRow.removeFromLeft(280));
    latencyBudgetLabel.setBounds(topRow.removeFromLeft(380));
    netSimToggle.setBounds(topRow);

    area.removeFromTop(4);

    // Three Columns:
    // Left (25%): Multitrack Recorder
    // Mid (35%): Setlist & Song Notes
    // Right (40%): Chat & Talkback
    auto body = area;
    int col1W = static_cast<int>(body.getWidth() * 0.22);
    int col2W = static_cast<int>(body.getWidth() * 0.38);

    auto colLeft = body.removeFromLeft(col1W).reduced(2);
    auto colMid = body.removeFromLeft(col2W).reduced(2);
    auto colRight = body.reduced(2);

    // Col 1: Recorder
    colLeft.removeFromTop(4);
    recordButton.setBounds(colLeft.removeFromTop(32));
    colLeft.removeFromTop(6);
    recordTimerLabel.setBounds(colLeft.removeFromTop(20));

    // Col 2: Setlist
    auto songSelRow = colMid.removeFromTop(22);
    setlistLabel.setBounds(songSelRow.removeFromLeft(50));
    prevSongButton.setBounds(songSelRow.removeFromLeft(24));
    songSelRow.removeFromLeft(2);
    songCombo.setBounds(songSelRow.removeFromLeft(130));
    songSelRow.removeFromLeft(2);
    nextSongButton.setBounds(songSelRow.removeFromLeft(24));
    songSelRow.removeFromLeft(6);
    songInfoLabel.setBounds(songSelRow);

    colMid.removeFromTop(4);
    songNotesEditor.setBounds(colMid);

    // Col 3: Chat
    auto chatHeaderRow = colRight.removeFromTop(20);
    chatLabel.setBounds(chatHeaderRow.removeFromLeft(70));
    pttButton.setBounds(chatHeaderRow.removeFromRight(90));

    auto chatInputRow = colRight.removeFromBottom(28);
    chatInputEditor.setBounds(chatInputRow.removeFromLeft(chatInputRow.getWidth() - 60));
    chatInputRow.removeFromLeft(4);
    sendChatButton.setBounds(chatInputRow);

    colRight.removeFromBottom(4);
    chatLogBox.setBounds(colRight);
}

} // namespace paasphoos
