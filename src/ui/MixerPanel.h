#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../audio/AudioEngine.h"
#include <array>

namespace paasphoos
{

class ChannelStripComponent : public juce::Component
{
public:
    ChannelStripComponent(const juce::String& channelName,
                          ChannelSettings& settings,
                          bool isMasterSection = false,
                          const juce::String& subLabel = {});
    ~ChannelStripComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void updateMeters(float peakL, float peakR, bool clipped);

    void setMasterVolumeControl(AudioEngine* engine);
    void setChannelName(const juce::String& newName, const juce::String& newSub = {});
    void setJitterBadge(const juce::String& text, juce::Colour color);
    void setActive(bool active);

private:
    juce::String name;
    juce::String sub;
    ChannelSettings& channelSettings;
    bool isMaster;
    bool isActiveState{true};
    AudioEngine* audioEngine = nullptr;

    juce::Label nameLabel;
    juce::Label subLabel;
    juce::Label jitterBadgeLabel;
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

    void setLocalMusicianName(const juce::String& name, const juce::String& role)
    {
        if (channel1Strip != nullptr)
            channel1Strip->setChannelName("YOU: " + name.toUpperCase(), role);
    }

private:
    AudioEngine& audioEngine;

    juce::Label titleLabel;

    std::unique_ptr<ChannelStripComponent> channel1Strip;
    std::unique_ptr<ChannelStripComponent> channel2Strip;
    std::array<std::unique_ptr<ChannelStripComponent>, AudioEngine::MAX_REMOTE_STRIPS> remoteStrips;
    std::unique_ptr<ChannelStripComponent> masterStrip;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerPanel)
};

} // namespace paasphoos
