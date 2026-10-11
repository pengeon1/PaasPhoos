#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../audio/AudioEngine.h"

namespace paasphoos
{

class AudioSettingsPanel : public juce::Component,
                           public juce::ChangeListener
{
public:
    explicit AudioSettingsPanel(AudioEngine& engine);
    ~AudioSettingsPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    void updateDeviceLists();

private:
    void applyDeviceSettings();
    void updateLatencyBadge();

    AudioEngine& audioEngine;

    juce::Label titleLabel;

    juce::Label driverTypeLabel;
    juce::ComboBox driverTypeCombo;

    juce::Label inputDeviceLabel;
    juce::ComboBox inputDeviceCombo;

    juce::Label outputDeviceLabel;
    juce::ComboBox outputDeviceCombo;

    juce::Label sampleRateLabel;
    juce::ComboBox sampleRateCombo;

    juce::Label bufferSizeLabel;
    juce::ComboBox bufferSizeCombo;

    juce::TextButton asioControlPanelButton;

    juce::Label latencyBadgeLabel;

    bool isUpdatingControls = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioSettingsPanel)
};

} // namespace paasphoos
