#include "MetronomeComponent.h"

namespace paasphoos
{

MetronomeComponent::MetronomeComponent(AudioEngine& engine)
    : audioEngine(engine)
{
    headerLabel.setText("MASTER METRONOME / CLICK", juce::dontSendNotification);
    headerLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    headerLabel.setColour(juce::Label::textColourId, juce::Colour(0xff94a3b8));
    addAndMakeVisible(headerLabel);

    // Play / Stop button
    playButton.setButtonText("START CLICK");
    playButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff181a22));
    playButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffff6600));
    playButton.setClickingTogglesState(true);
    playButton.onClick = [this]()
    {
        bool active = playButton.getToggleState();
        audioEngine.getMetronome().setEnabled(active);
        playButton.setButtonText(active ? "STOP CLICK" : "START CLICK");
    };
    addAndMakeVisible(playButton);

    // BPM Display
    bpmDisplayLabel.setText("120 BPM", juce::dontSendNotification);
    bpmDisplayLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    bpmDisplayLabel.setJustificationType(juce::Justification::centred);
    bpmDisplayLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff090a0d));
    bpmDisplayLabel.setColour(juce::Label::textColourId, juce::Colour(0xffffaa33));
    addAndMakeVisible(bpmDisplayLabel);

    // Minus / Plus buttons
    minusBpmButton.setButtonText("-");
    minusBpmButton.onClick = [this]()
    {
        float cur = audioEngine.getMetronome().getBpm();
        bpmSlider.setValue(cur - 1.0f);
    };
    addAndMakeVisible(minusBpmButton);

    plusBpmButton.setButtonText("+");
    plusBpmButton.onClick = [this]()
    {
        float cur = audioEngine.getMetronome().getBpm();
        bpmSlider.setValue(cur + 1.0f);
    };
    addAndMakeVisible(plusBpmButton);

    // BPM Slider
    bpmSlider.setRange(30.0, 280.0, 1.0);
    bpmSlider.setValue(120.0, juce::dontSendNotification);
    bpmSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    bpmSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    bpmSlider.onValueChange = [this]()
    {
        float val = static_cast<float>(bpmSlider.getValue());
        audioEngine.getMetronome().setBpm(val);
        bpmDisplayLabel.setText(juce::String((int)val) + " BPM", juce::dontSendNotification);
    };
    addAndMakeVisible(bpmSlider);

    // Tap Tempo
    tapTempoButton.setButtonText("TAP");
    tapTempoButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff22252e));
    tapTempoButton.onClick = [this]() { handleTapTempo(); };
    addAndMakeVisible(tapTempoButton);

    // Time Signature
    timeSignatureCombo.addItem("4/4", 1);
    timeSignatureCombo.addItem("3/4", 2);
    timeSignatureCombo.addItem("2/4", 3);
    timeSignatureCombo.addItem("6/8", 4);
    timeSignatureCombo.setSelectedId(1, juce::dontSendNotification);
    timeSignatureCombo.onChange = [this]()
    {
        switch (timeSignatureCombo.getSelectedId())
        {
            case 1: audioEngine.getMetronome().setTimeSignature(4, 4); break;
            case 2: audioEngine.getMetronome().setTimeSignature(3, 4); break;
            case 3: audioEngine.getMetronome().setTimeSignature(2, 4); break;
            case 4: audioEngine.getMetronome().setTimeSignature(6, 8); break;
            default: break;
        }
    };
    addAndMakeVisible(timeSignatureCombo);

    // Volume
    volumeLabel.setText("Vol:", juce::dontSendNotification);
    volumeLabel.setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(volumeLabel);

    volumeSlider.setRange(0.0, 1.0, 0.01);
    volumeSlider.setValue(0.7, juce::dontSendNotification);
    volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    volumeSlider.onValueChange = [this]()
    {
        audioEngine.getMetronome().setVolume(static_cast<float>(volumeSlider.getValue()));
    };
    addAndMakeVisible(volumeSlider);

    startTimerHz(30);
}

MetronomeComponent::~MetronomeComponent()
{
    stopTimer();
}

void MetronomeComponent::handleTapTempo()
{
    auto now = juce::Time::currentTimeMillis();

    // Reset if last tap was more than 2 seconds ago
    if (!tapTimes.empty() && (now - tapTimes.back()) > 2000)
        tapTimes.clear();

    tapTimes.push_back(now);
    if (tapTimes.size() > 5)
        tapTimes.erase(tapTimes.begin());

    if (tapTimes.size() >= 2)
    {
        double totalInterval = 0.0;
        for (size_t i = 1; i < tapTimes.size(); ++i)
            totalInterval += static_cast<double>(tapTimes[i] - tapTimes[i - 1]);

        double avgIntervalMs = totalInterval / static_cast<double>(tapTimes.size() - 1);
        if (avgIntervalMs > 0.0)
        {
            double calculatedBpm = 60000.0 / avgIntervalMs;
            bpmSlider.setValue(std::round(calculatedBpm));
        }
    }
}

void MetronomeComponent::timerCallback()
{
    int currentBeat = audioEngine.getMetronome().getCurrentBeat();
    if (currentBeat != lastDrawnBeat || audioEngine.getMetronome().isEnabled())
    {
        lastDrawnBeat = currentBeat;
        repaint();
    }
}

void MetronomeComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff090a0d));
    g.setColour(juce::Colour(0xff22252e));
    g.drawRect(getLocalBounds(), 1);

    // Draw visual beat LEDs
    bool isEnabled = audioEngine.getMetronome().isEnabled();
    int currentBeat = audioEngine.getMetronome().getCurrentBeat();
    int totalBeats = audioEngine.getMetronome().getTimeSignatureNumerator();

    auto bounds = getLocalBounds().reduced(12, 8);
    auto bottomArea = bounds.removeFromBottom(24);

    float dotDiameter = 14.0f;
    float spacing = 22.0f;
    float startX = bottomArea.getX() + 10.0f;
    float centerY = bottomArea.getCentreY();

    for (int i = 0; i < totalBeats; ++i)
    {
        float x = startX + (i * spacing);
        float y = centerY - (dotDiameter * 0.5f);

        bool isActiveBeat = isEnabled && (i == currentBeat);
        bool isDownbeat = (i == 0);

        juce::Colour dotColor;
        if (isActiveBeat)
        {
            dotColor = isDownbeat ? juce::Colour(0xffffaa33) : juce::Colour(0xffff6600);
        }
        else
        {
            dotColor = juce::Colour(0xff1f2129);
        }

        g.setColour(dotColor);
        g.fillEllipse(x, y, dotDiameter, dotDiameter);

        if (isActiveBeat)
        {
            g.setColour(dotColor.withAlpha(0.4f));
            g.drawEllipse(x - 3.0f, y - 3.0f, dotDiameter + 6.0f, dotDiameter + 6.0f, 2.0f);
        }
    }
}

void MetronomeComponent::resized()
{
    auto area = getLocalBounds().reduced(12, 8);

    auto topRow = area.removeFromTop(26);
    headerLabel.setBounds(topRow.removeFromLeft(200));

    auto midRow = area.removeFromTop(32);
    playButton.setBounds(midRow.removeFromLeft(110));
    midRow.removeFromLeft(8);

    minusBpmButton.setBounds(midRow.removeFromLeft(28));
    midRow.removeFromLeft(4);
    bpmDisplayLabel.setBounds(midRow.removeFromLeft(80));
    midRow.removeFromLeft(4);
    plusBpmButton.setBounds(midRow.removeFromLeft(28));
    midRow.removeFromLeft(8);

    bpmSlider.setBounds(midRow.removeFromLeft(140));
    midRow.removeFromLeft(8);

    tapTempoButton.setBounds(midRow.removeFromLeft(50));
    midRow.removeFromLeft(8);

    timeSignatureCombo.setBounds(midRow.removeFromLeft(65));
    midRow.removeFromLeft(12);

    volumeLabel.setBounds(midRow.removeFromLeft(30));
    volumeSlider.setBounds(midRow.removeFromLeft(100));
}

} // namespace paasphoos
