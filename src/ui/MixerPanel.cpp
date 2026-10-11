#include "MixerPanel.h"

namespace paasphoos
{

ChannelStripComponent::ChannelStripComponent(const juce::String& channelName,
                                             ChannelSettings& settings,
                                             bool isMasterSection)
    : name(channelName),
      channelSettings(settings),
      isMaster(isMasterSection)
{
    nameLabel.setText(name, juce::dontSendNotification);
    nameLabel.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    nameLabel.setJustificationType(juce::Justification::centred);
    nameLabel.setColour(juce::Label::textColourId, isMaster ? juce::Colour(0xff38bdf8) : juce::Colour(0xffe2e8f0));
    addAndMakeVisible(nameLabel);

    // Gain Fader (dB style mapping)
    gainFader.setSliderStyle(juce::Slider::LinearVertical);
    gainFader.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 18);
    gainFader.setRange(0.0, 2.0, 0.01);
    gainFader.setValue(1.0, juce::dontSendNotification);
    gainFader.textFromValueFunction = [](double val) -> juce::String
    {
        if (val <= 0.0001) return "-inf dB";
        float db = 20.0f * std::log10(static_cast<float>(val));
        return juce::String::formatted("%+.1f dB", db);
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
        panKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 45, 16);
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
        muteButton.setButtonText("MUTE");
        muteButton.setClickingTogglesState(true);
        muteButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffdc2626));
        muteButton.onClick = [this]()
        {
            channelSettings.mute.store(muteButton.getToggleState(), std::memory_order_relaxed);
        };
        addAndMakeVisible(muteButton);

        // Solo Button
        soloButton.setButtonText("SOLO");
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
        clipResetButton.setButtonText("RST CLIP");
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

void ChannelStripComponent::updateMeters(float peakL, float peakR, bool clipped)
{
    currentPeakL = peakL;
    currentPeakR = peakR;
    isClipped = clipped;
    repaint();
}

void ChannelStripComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff181a20));
    g.setColour(juce::Colour(0xff27272a));
    g.drawRect(getLocalBounds(), 1);

    // Draw LED Level Meter
    auto faderBounds = gainFader.getBounds();
    int meterX = faderBounds.getRight() - 12;
    int meterY = faderBounds.getY() + 4;
    int meterW = isMaster ? 16 : 8;
    int meterH = faderBounds.getHeight() - 26;

    // Background track
    g.setColour(juce::Colour(0xff090a0f));
    g.fillRoundedRectangle((float)meterX, (float)meterY, (float)meterW, (float)meterH, 2.0f);

    auto drawSingleMeter = [&](int x, int w, float peak)
    {
        float filledHeight = juce::jlimit(0.0f, 1.0f, peak) * meterH;
        float fillY = (meterY + meterH) - filledHeight;

        // Gradient: Green -> Yellow -> Red
        juce::ColourGradient grad(juce::Colour(0xff22c55e), (float)x, (float)(meterY + meterH),
                                  juce::Colour(0xffef4444), (float)x, (float)meterY, false);
        grad.addColour(0.7, juce::Colour(0xffeab308));
        g.setGradientFill(grad);
        g.fillRoundedRectangle((float)x, fillY, (float)w, filledHeight, 2.0f);
    };

    if (isMaster)
    {
        drawSingleMeter(meterX, 7, currentPeakL);
        drawSingleMeter(meterX + 8, 7, currentPeakR);
    }
    else
    {
        drawSingleMeter(meterX, meterW, currentPeakL);
    }

    // Clip Indicator LED
    int clipIndicatorY = meterY - 14;
    g.setColour(isClipped ? juce::Colour(0xffef4444) : juce::Colour(0xff3f1010));
    g.fillRoundedRectangle((float)meterX, (float)clipIndicatorY, (float)meterW, 8.0f, 2.0f);
    if (isClipped)
    {
        g.setColour(juce::Colour(0xffff6b6b));
        g.drawRoundedRectangle((float)meterX, (float)clipIndicatorY, (float)meterW, 8.0f, 2.0f, 1.0f);
    }
}

void ChannelStripComponent::resized()
{
    auto area = getLocalBounds().reduced(8);

    nameLabel.setBounds(area.removeFromTop(22));
    area.removeFromTop(4);

    if (!isMaster)
    {
        auto buttonRow = area.removeFromTop(24);
        muteButton.setBounds(buttonRow.removeFromLeft(buttonRow.getWidth() / 2 - 2));
        soloButton.setBounds(buttonRow.removeFromRight(buttonRow.getWidth()));
        area.removeFromTop(6);

        panKnob.setBounds(area.removeFromTop(70));
        area.removeFromTop(6);
    }
    else
    {
        clipResetButton.setBounds(area.removeFromTop(24));
        area.removeFromTop(10);
    }

    // Fader area
    gainFader.setBounds(area.removeFromTop(area.getHeight() - 4));
}

// ================= MixerPanel =================

MixerPanel::MixerPanel(AudioEngine& engine)
    : audioEngine(engine)
{
    titleLabel.setText("LOCAL MONITORING & CHANNEL STRIPS", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff94a3b8));
    addAndMakeVisible(titleLabel);

    channel1Strip = std::make_unique<ChannelStripComponent>("CH 1: GUITAR", audioEngine.getChannel1(), false);
    addAndMakeVisible(*channel1Strip);

    channel2Strip = std::make_unique<ChannelStripComponent>("CH 2: VOCAL", audioEngine.getChannel2(), false);
    addAndMakeVisible(*channel2Strip);

    remotePeerStrip = std::make_unique<ChannelStripComponent>("PEER: REMOTE", audioEngine.getRemotePeer1(), false);
    addAndMakeVisible(*remotePeerStrip);

    masterStrip = std::make_unique<ChannelStripComponent>("MASTER MONITOR", audioEngine.getChannel1(), true);
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

    auto& peer = audioEngine.getRemotePeer1();
    remotePeerStrip->updateMeters(peer.peakLevel.load(std::memory_order_relaxed), 0.0f,
                                 peer.clipDetected.load(std::memory_order_relaxed));

    masterStrip->updateMeters(audioEngine.getMasterPeakLeft(), audioEngine.getMasterPeakRight(),
                              audioEngine.getMasterClipped());
}

void MixerPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff121316));
}

void MixerPanel::resized()
{
    auto area = getLocalBounds().reduced(10, 6);
    titleLabel.setBounds(area.removeFromTop(22));
    area.removeFromTop(6);

    int stripWidth = area.getWidth() / 4;
    channel1Strip->setBounds(area.removeFromLeft(stripWidth).reduced(3));
    channel2Strip->setBounds(area.removeFromLeft(stripWidth).reduced(3));
    remotePeerStrip->setBounds(area.removeFromLeft(stripWidth).reduced(3));
    masterStrip->setBounds(area.reduced(3));
}

} // namespace paasphoos
