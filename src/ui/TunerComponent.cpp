#include "TunerComponent.h"
#include "../common/Typography.h"

namespace paasphoos
{

TunerComponent::TunerComponent(AudioEngine& engine)
    : audioEngine(engine)
{
    headerLabel.setText("CHROMATIC TUNER", juce::dontSendNotification);
    headerLabel.setFont(Typography::h3());
    headerLabel.setColour(juce::Label::textColourId, juce::Colour(0xff94a3b8));
    addAndMakeVisible(headerLabel);

    channelSelectCombo.addItem("Ch 1 (Guitar/Inst)", 1);
    channelSelectCombo.addItem("Ch 2 (Vocal/Aux)", 2);
    channelSelectCombo.setSelectedId(1, juce::dontSendNotification);
    channelSelectCombo.onChange = [this]()
    {
        audioEngine.setTuningInputChannel(channelSelectCombo.getSelectedId() - 1);
    };
    addAndMakeVisible(channelSelectCombo);

    enableToggle.setButtonText("Active");
    enableToggle.setToggleState(true, juce::dontSendNotification);
    enableToggle.onClick = [this]()
    {
        audioEngine.getTuner().setEnabled(enableToggle.getToggleState());
    };
    addAndMakeVisible(enableToggle);

    startTimerHz(30); // 30 FPS updates for pitch display
}

TunerComponent::~TunerComponent()
{
    stopTimer();
}

void TunerComponent::timerCallback()
{
    // Run pitch estimation from GUI thread (lock-free read from circular buffer)
    audioEngine.getTuner().updatePitchEstimation();
    currentResult = audioEngine.getTuner().getCurrentResult();
    repaint();
}

void TunerComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff090a0d));
    g.setColour(juce::Colour(0xff22252e));
    g.drawRect(getLocalBounds(), 1);

    auto bounds = getLocalBounds().reduced(12);
    bounds.removeFromTop(36); // Space for header and combo

    // Background card for tuner gauge
    auto gaugeBounds = bounds.toFloat();
    g.setColour(juce::Colour(0xff12131a));
    g.fillRoundedRectangle(gaugeBounds, 6.0f);
    g.setColour(juce::Colour(0xff2a2d36));
    g.drawRoundedRectangle(gaugeBounds, 6.0f, 1.0f);

    if (!currentResult.isActive)
    {
        g.setColour(juce::Colour(0xff64748b));
        g.setFont(Typography::display(36.0f));
        g.drawText("--", gaugeBounds.removeFromTop(60.0f), juce::Justification::centred);

        g.setFont(Typography::caption());
        g.drawText("Play a note or sing into selected input", gaugeBounds, juce::Justification::centred);
        return;
    }

    auto noteName = Tuner::getNoteName(currentResult.noteNumber);
    float cents = currentResult.centsOffset;
    bool inTune = std::abs(cents) <= 3.0f;

    // Note name display
    auto noteArea = gaugeBounds.removeFromTop(55.0f);
    juce::Colour noteColor = inTune ? juce::Colour(0xff38e1c6) :
                             (std::abs(cents) <= 12.0f ? juce::Colour(0xff3da5ff) : juce::Colour(0xff7c5cff));
    g.setColour(noteColor);
    g.setFont(Typography::display(42.0f));
    g.drawText(noteName, noteArea, juce::Justification::centred);

    // Frequency readout (Tabular figures that never jitter)
    auto freqArea = gaugeBounds.removeFromTop(20.0f);
    g.setColour(juce::Colour(0xff94a3b8));
    g.setFont(Typography::numeric(13.0f));
    g.drawText(juce::String::formatted("%.1f Hz  (%.1f cents)", currentResult.frequency, cents),
               freqArea, juce::Justification::centred);

    // Cents Needle / Meter Bar
    auto meterArea = gaugeBounds.removeFromTop(32.0f).reduced(20.0f, 4.0f);
    g.setColour(juce::Colour(0xff181a22));
    g.fillRoundedRectangle(meterArea, 4.0f);

    float centerX = meterArea.getCentreX();
    float meterY = meterArea.getY();
    float meterHeight = meterArea.getHeight();

    // Center zero line (In Tune target)
    g.setColour(juce::Colour(0xff10b981));
    g.fillRect(centerX - 1.5f, meterY, 3.0f, meterHeight);

    // Needle position (-50 to +50 cents mapped to width)
    float halfWidth = (meterArea.getWidth() * 0.5f) - 4.0f;
    float needleOffset = (juce::jlimit(-50.0f, 50.0f, cents) / 50.0f) * halfWidth;
    float needleX = centerX + needleOffset;

    g.setColour(noteColor);
    g.fillRect(needleX - 2.0f, meterY - 2.0f, 4.0f, meterHeight + 4.0f);

    // Labels
    g.setColour(juce::Colour(0xff64748b));
    g.setFont(Typography::caption(10.0f));
    g.drawText("-50b", meterArea.getX(), meterArea.getBottom() + 2, 35, 14, juce::Justification::left);
    g.drawText("+50#", meterArea.getRight() - 35, meterArea.getBottom() + 2, 35, 14, juce::Justification::right);
    g.drawText("IN TUNE", centerX - 25, meterArea.getBottom() + 2, 50, 14, juce::Justification::centred);
}

void TunerComponent::resized()
{
    auto area = getLocalBounds().reduced(12, 8);
    auto topRow = area.removeFromTop(26);

    headerLabel.setBounds(topRow.removeFromLeft(130));
    enableToggle.setBounds(topRow.removeFromRight(65));
    topRow.removeFromRight(8);
    channelSelectCombo.setBounds(topRow.removeFromRight(140));
}

} // namespace paasphoos
