#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../audio/AudioEngine.h"
#include "../common/ThemeConfig.h"
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
    void setChannelColor(juce::Colour color);
    void setJitterBadge(const juce::String& text, juce::Colour color);
    void setActive(bool active);

private:
    juce::String name;
    juce::String sub;
    ChannelSettings& channelSettings;
    bool isMaster;
    bool isActiveState{true};
    juce::Colour customAccentColor;
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

    void triggerMeterSweep();

    void setLocalMusicianName(const juce::String& name, const juce::String& role, juce::Colour roleColor = juce::Colour(ThemeConfig::kAccentVioletHex))
    {
        if (channel1Strip != nullptr)
        {
            channel1Strip->setChannelName("YOU: " + name.toUpperCase(), role);
            channel1Strip->setChannelColor(roleColor);
        }
    }

    void populateMockBandChannels()
    {
        if (channel1Strip != nullptr)
        {
            channel1Strip->setChannelName("YOU: ROHAN", "VOCALS");
            channel1Strip->setChannelColor(juce::Colour(ThemeConfig::kRoleVocalsHex));
            channel1Strip->updateMeters(0.72f, 0.68f, false);
        }
        if (channel2Strip != nullptr)
        {
            channel2Strip->setChannelName("IN 2: GUITAR", "CLEAN / LEAD");
            channel2Strip->setChannelColor(juce::Colour(ThemeConfig::kRoleGuitar1Hex));
            channel2Strip->updateMeters(0.55f, 0.58f, false);
        }
        const char* peerNames[AudioEngine::MAX_REMOTE_STRIPS] = { "AARAV", "PRIYA", "KABIR", "VIKRAM", "DEV" };
        const char* peerRoles[AudioEngine::MAX_REMOTE_STRIPS] = { "KEYS / SYNTH", "BASS GUITAR", "BACKING VOX", "DRUMS", "PERCUSSION" };
        const juce::Colour peerCols[AudioEngine::MAX_REMOTE_STRIPS] = {
            juce::Colour(ThemeConfig::kRoleKeysHex),
            juce::Colour(ThemeConfig::kRoleGuitar2Hex),
            juce::Colour(ThemeConfig::kRoleVocalsHex),
            juce::Colour(ThemeConfig::kRoleDrumsHex),
            juce::Colour(ThemeConfig::kRoleOtherHex)
        };
        for (int i = 0; i < AudioEngine::MAX_REMOTE_STRIPS; ++i)
        {
            if (remoteStrips[i] != nullptr)
            {
                remoteStrips[i]->setActive(true);
                remoteStrips[i]->setChannelName(peerNames[i], peerRoles[i]);
                remoteStrips[i]->setChannelColor(peerCols[i]);
                remoteStrips[i]->setJitterBadge(juce::String(14 + i * 5) + " ms", juce::Colour(ThemeConfig::kRoleGuitar2Hex));
                remoteStrips[i]->updateMeters(0.48f + i * 0.08f, 0.45f + i * 0.07f, false);
            }
        }
        if (masterStrip != nullptr)
        {
            masterStrip->updateMeters(0.82f, 0.84f, false);
        }
    }

private:
    AudioEngine& audioEngine;
    juce::Label titleLabel;

    std::unique_ptr<ChannelStripComponent> channel1Strip;
    std::unique_ptr<ChannelStripComponent> channel2Strip;
    std::array<std::unique_ptr<ChannelStripComponent>, AudioEngine::MAX_REMOTE_STRIPS> remoteStrips;
    std::unique_ptr<ChannelStripComponent> masterStrip;

    float meterSweepProgress{0.0f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerPanel)
};

} // namespace paasphoos
