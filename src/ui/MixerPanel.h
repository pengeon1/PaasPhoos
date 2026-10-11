#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../audio/AudioEngine.h"

namespace paasphoos
{

class ChannelStripComponent : public juce::Component
{
public:
    ChannelStripComponent(const juce::String& channelName,
                          ChannelSettings& settings,
                          bool isMasterSection = false);
    ~ChannelStripComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void updateMeters(float peakL, float peakR, bool clipped);

    void setMasterVolumeControl(AudioEngine* engine);

private:
    juce::String name;
    ChannelSettings& channelSettings;
    bool isMaster;
    AudioEngine* audioEngine = nullptr;

    juce::Label nameLabel;
    juce::Slider gainFader;
    juce::Slider panKnob;
    juce::TextButton muteButton;
    juce::TextButton soloButton;
    juce::TextButton clipResetButton;

    float currentPeakL = 0.0f;
    float currentPeakR = 0.0f;
    bool isClipped = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelStripComponent)
};

class MixerPanel : public juce::Component,
                   public juce::Timer
{
public:
    explicit MixerPanel(AudioEngine& engine);
    ~MixerPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    AudioEngine& audioEngine;

    juce::Label titleLabel;

    std::unique_ptr<ChannelStripComponent> channel1Strip;
    std::unique_ptr<ChannelStripComponent> channel2Strip;
    std::unique_ptr<ChannelStripComponent> remotePeerStrip;
    std::unique_ptr<ChannelStripComponent> masterStrip;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerPanel)
};

} // namespace paasphoos
