#include "MixerPanel.h"

namespace paasphoos
{

ChannelStripComponent::ChannelStripComponent(const juce::String& channelName,
                                             ChannelSettings& settings,
                                             bool isMasterSection,
                                             const juce::String& subTitle)
    : name(channelName),
      sub(subTitle),
      channelSettings(settings),
      isMaster(isMasterSection)
{
    nameLabel.setText(name, juce::dontSendNotification);
    nameLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    nameLabel.setJustificationType(juce::Justification::centred);
    nameLabel.setColour(juce::Label::textColourId, isMaster ? juce::Colour(0xffff7700) : juce::Colour(0xffe2e8f0));
    addAndMakeVisible(nameLabel);

    subLabel.setText(sub, juce::dontSendNotification);
    subLabel.setFont(juce::FontOptions(9.5f));
    subLabel.setJustificationType(juce::Justification::centred);
    subLabel.setColour(juce::Label::textColourId, juce::Colour(0xff94a3b8));
    addAndMakeVisible(subLabel);

    if (!isMaster)
    {
        jitterBadgeLabel.setText("", juce::dontSendNotification);
        jitterBadgeLabel.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        jitterBadgeLabel.setJustificationType(juce::Justification::centred);
        jitterBadgeLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff0f172a));
        jitterBadgeLabel.setColour(juce::Label::textColourId, juce::Colour(0xff64748b));
        addAndMakeVisible(jitterBadgeLabel);
    }

    // Gain Fader (dB style mapping)
    gainFader.setSliderStyle(juce::Slider::LinearVertical);
    gainFader.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 46, 16);
    gainFader.setRange(0.0, 2.0, 0.01);
    gainFader.setValue(1.0, juce::dontSendNotification);
    gainFader.textFromValueFunction = [](double val) -> juce::String
    {
        if (val <= 0.0001) return "-inf";
        float db = 20.0f * std::log10(static_cast<float>(val));
        return juce::String::formatted("%+.1f", db);
    };
    gainFader.onValueChange = [this]()
    {
        float val = static_cast<float>(gainFader.getValue());
        if (isMaster && audioEngine != nullptr)
            audioEngine->setMasterVolume(val);
        else
            channelSettings.gain.store(val, std::memory_order_relaxed);
    };
    addAndMakeVisible(gainFader);

    if (!isMaster)
    {
        // Pan Knob
        panKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        panKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 36, 14);
        panKnob.setRange(-1.0, 1.0, 0.02);
        panKnob.setValue(0.0, juce::dontSendNotification);
        panKnob.textFromValueFunction = [](double val) -> juce::String
        {
            if (std::abs(val) < 0.05) return "C";
            if (val < 0.0) return "L" + juce::String(static_cast<int>(std::abs(val) * 100));
            return "R" + juce::String(static_cast<int>(val * 100));
        };
        panKnob.onValueChange = [this]()
        {
            channelSettings.pan.store(static_cast<float>(panKnob.getValue()), std::memory_order_relaxed);
        };
        addAndMakeVisible(panKnob);

        // Mute Button
        muteButton.setButtonText("M");
        muteButton.setClickingTogglesState(true);
        muteButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffdc2626));
        muteButton.onClick = [this]()
        {
            channelSettings.mute.store(muteButton.getToggleState(), std::memory_order_relaxed);
        };
        addAndMakeVisible(muteButton);

        // Solo Button
        soloButton.setButtonText("S");
        soloButton.setClickingTogglesState(true);
        soloButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xfff59e0b));
        soloButton.onClick = [this]()
        {
            channelSettings.solo.store(soloButton.getToggleState(), std::memory_order_relaxed);
        };
        addAndMakeVisible(soloButton);
    }
    else
    {
        clipResetButton.setButtonText("RST");
        clipResetButton.onClick = [this]()
        {
            if (audioEngine != nullptr)
                audioEngine->clearMasterClip();
        };
        addAndMakeVisible(clipResetButton);
    }
}

void ChannelStripComponent::setMasterVolumeControl(AudioEngine* engine)
{
    audioEngine = engine;
}

void ChannelStripComponent::setChannelName(const juce::String& newName, const juce::String& newSub)
{
    if (name != newName || sub != newSub)
    {
        name = newName;
        sub = newSub;
        nameLabel.setText(name, juce::dontSendNotification);
        subLabel.setText(sub, juce::dontSendNotification);
    }
}

void ChannelStripComponent::setJitterBadge(const juce::String& text, juce::Colour color)
{
    if (!isMaster)
    {
        jitterBadgeLabel.setText(text, juce::dontSendNotification);
        jitterBadgeLabel.setColour(juce::Label::textColourId, color);
    }
}

void ChannelStripComponent::setActive(bool active)
{
    if (isActiveState != active)
    {
        isActiveState = active;
        nameLabel.setColour(juce::Label::textColourId,
                            active ? (isMaster ? juce::Colour(0xffff7700) : juce::Colour(0xfff8fafc))
                                   : juce::Colour(0xff64748b));
        subLabel.setColour(juce::Label::textColourId, active ? juce::Colour(0xffffaa33) : juce::Colour(0xff475569));
        gainFader.setEnabled(active);
        panKnob.setEnabled(active);
        muteButton.setEnabled(active);
        soloButton.setEnabled(active);
        repaint();
    }
}

void ChannelStripComponent::updateMeters(float peakL, float peakR, bool clipped)
{
    currentPeakL = peakL;
    currentPeakR = peakR;
    isClipped = clipped;
    repaint();
}

void ChannelStripComponent::paint(juce::Graphics& g)
{
    g.fillAll(isActiveState ? juce::Colour(0xff14151b) : juce::Colour(0xff090a0d));
    g.setColour(isActiveState ? juce::Colour(0xff2a2d36) : juce::Colour(0xff1a1b22));
    g.drawRect(getLocalBounds(), 1);

    // Level Meter
    auto faderBounds = gainFader.getBounds();
    int meterX = faderBounds.getRight() - 9;
    int meterY = faderBounds.getY() + 2;
    int meterW = isMaster ? 12 : 6;
    int meterH = faderBounds.getHeight() - 20;

    g.setColour(juce::Colour(0xff090a0d));
    g.fillRoundedRectangle((float)meterX, (float)meterY, (float)meterW, (float)meterH, 1.5f);

    auto drawSingleMeter = [&](int x, int w, float peak)
    {
        float filledHeight = juce::jlimit(0.0f, 1.0f, peak) * meterH;
        float fillY = (meterY + meterH) - filledHeight;

        juce::ColourGradient grad(juce::Colour(0xffffaa33), (float)x, (float)(meterY + meterH),
                                  juce::Colour(0xffff2200), (float)x, (float)meterY, false);
        grad.addColour(0.65, juce::Colour(0xffff6600));
        g.setGradientFill(grad);
        g.fillRoundedRectangle((float)x, fillY, (float)w, filledHeight, 1.5f);
    };

    if (isMaster)
    {
        drawSingleMeter(meterX, 5, currentPeakL);
        drawSingleMeter(meterX + 6, 5, currentPeakR);
    }
    else
    {
        drawSingleMeter(meterX, meterW, currentPeakL);
    }

    // Clip LED
    int clipY = meterY - 10;
    g.setColour(isClipped ? juce::Colour(0xffef4444) : juce::Colour(0xff351010));
    g.fillRoundedRectangle((float)meterX, (float)clipY, (float)meterW, 6.0f, 1.5f);
}

void ChannelStripComponent::resized()
{
    auto area = getLocalBounds().reduced(4);

    nameLabel.setBounds(area.removeFromTop(18));
    subLabel.setBounds(area.removeFromTop(14));
    area.removeFromTop(2);

    if (!isMaster)
    {
        jitterBadgeLabel.setBounds(area.removeFromTop(14));
        area.removeFromTop(4);

        auto btnRow = area.removeFromTop(20);
        muteButton.setBounds(btnRow.removeFromLeft(btnRow.getWidth() / 2 - 1));
        soloButton.setBounds(btnRow.removeFromRight(btnRow.getWidth()));
        area.removeFromTop(4);

        panKnob.setBounds(area.removeFromTop(54));
        area.removeFromTop(4);
    }
    else
    {
        clipResetButton.setBounds(area.removeFromTop(20));
        area.removeFromTop(10);
    }

    gainFader.setBounds(area);
}

// ================= MixerPanel =================

MixerPanel::MixerPanel(AudioEngine& engine)
    : audioEngine(engine)
{
    titleLabel.setText("BAND REHEARSAL MIXER CONSOLE (6 PARTICIPANTS)", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff94a3b8));
    addAndMakeVisible(titleLabel);

    channel1Strip = std::make_unique<ChannelStripComponent>("YOU: CH1", audioEngine.getChannel1(), false, "Guitar / Inst");
    addAndMakeVisible(*channel1Strip);

    channel2Strip = std::make_unique<ChannelStripComponent>("YOU: CH2", audioEngine.getChannel2(), false, "Vocal / Aux");
    addAndMakeVisible(*channel2Strip);

    for (int s = 0; s < AudioEngine::MAX_REMOTE_STRIPS; ++s)
    {
        remoteStrips[s] = std::make_unique<ChannelStripComponent>(
            juce::String::formatted("PEER %d", s + 1),
            audioEngine.getRemotePeer(s),
            false,
            "Waiting..."
        );
        remoteStrips[s]->setActive(false);
        addAndMakeVisible(*remoteStrips[s]);
    }

    masterStrip = std::make_unique<ChannelStripComponent>("MASTER", audioEngine.getChannel1(), true, "Phones Out");
    masterStrip->setMasterVolumeControl(&audioEngine);
    addAndMakeVisible(*masterStrip);

    startTimerHz(30);
}

MixerPanel::~MixerPanel()
{
    stopTimer();
}

void MixerPanel::timerCallback()
{
    auto& ch1 = audioEngine.getChannel1();
    channel1Strip->updateMeters(ch1.peakLevel.load(std::memory_order_relaxed), 0.0f,
                                ch1.clipDetected.load(std::memory_order_relaxed));

    auto& ch2 = audioEngine.getChannel2();
    channel2Strip->updateMeters(ch2.peakLevel.load(std::memory_order_relaxed), 0.0f,
                                ch2.clipDetected.load(std::memory_order_relaxed));

    auto& transport = audioEngine.getLanTransport();

    for (int s = 0; s < AudioEngine::MAX_REMOTE_STRIPS; ++s)
    {
        int partId = audioEngine.getParticipantIdForRemoteStrip(s);
        const auto* peer = (partId >= 0) ? transport.getPeer(partId) : nullptr;
        auto& settings = audioEngine.getRemotePeer(s);

        if (peer != nullptr && peer->connected.load(std::memory_order_relaxed))
        {
            remoteStrips[s]->setActive(true);
            remoteStrips[s]->setChannelName(peer->name, peer->roleOrInstrument);

            auto metrics = peer->jitterBuffer.getMetrics();
            juce::Colour badgeCol = (metrics.currentFillMs < 15.0f) ? juce::Colour(0xff34d399) : juce::Colour(0xfffbbf24);
            remoteStrips[s]->setJitterBadge(juce::String::formatted("JB: %.1fms", metrics.currentFillMs), badgeCol);

            remoteStrips[s]->updateMeters(settings.peakLevel.load(std::memory_order_relaxed), 0.0f,
                                         settings.clipDetected.load(std::memory_order_relaxed));
        }
        else
        {
            remoteStrips[s]->setActive(false);
            remoteStrips[s]->setChannelName(juce::String::formatted("SLOT %d", s + 1), "OFFLINE");
            remoteStrips[s]->setJitterBadge("-", juce::Colour(0xff64748b));
            remoteStrips[s]->updateMeters(0.0f, 0.0f, false);
        }
    }

    masterStrip->updateMeters(audioEngine.getMasterPeakLeft(), audioEngine.getMasterPeakRight(),
                              audioEngine.getMasterClipped());
}

void MixerPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff090a0d));
}

void MixerPanel::resized()
{
    auto area = getLocalBounds().reduced(8, 4);
    titleLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(4);

    // 8 total strips: Ch1, Ch2, 5 Remote Peers, Master
    int totalStrips = 2 + AudioEngine::MAX_REMOTE_STRIPS + 1; // 8
    int stripWidth = area.getWidth() / totalStrips;

    channel1Strip->setBounds(area.removeFromLeft(stripWidth).reduced(2));
    channel2Strip->setBounds(area.removeFromLeft(stripWidth).reduced(2));

    for (int s = 0; s < AudioEngine::MAX_REMOTE_STRIPS; ++s)
    {
        remoteStrips[s]->setBounds(area.removeFromLeft(stripWidth).reduced(2));
    }

    masterStrip->setBounds(area.reduced(2));
}

} // namespace paasphoos
