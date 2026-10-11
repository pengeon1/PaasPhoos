#include "AudioSettingsPanel.h"

namespace paasphoos
{

AudioSettingsPanel::AudioSettingsPanel(AudioEngine& engine)
    : audioEngine(engine)
{
    audioEngine.addChangeListener(this);

    titleLabel.setText("AUDIO HARDWARE & DRIVER", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff94a3b8));
    addAndMakeVisible(titleLabel);

    // Driver Type (WASAPI / ASIO)
    driverTypeLabel.setText("Driver Type:", juce::dontSendNotification);
    driverTypeLabel.setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(driverTypeLabel);

    driverTypeCombo.onChange = [this]()
    {
        if (isUpdatingControls) return;
        auto selectedType = driverTypeCombo.getText();
        if (selectedType.isNotEmpty())
        {
            audioEngine.getDeviceManager().setCurrentAudioDeviceType(selectedType, true);
            updateDeviceLists();
            updateLatencyBadge();
        }
    };
    addAndMakeVisible(driverTypeCombo);

    // Input Device
    inputDeviceLabel.setText("Input:", juce::dontSendNotification);
    inputDeviceLabel.setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(inputDeviceLabel);

    inputDeviceCombo.onChange = [this]() { if (!isUpdatingControls) applyDeviceSettings(); };
    addAndMakeVisible(inputDeviceCombo);

    // Output Device
    outputDeviceLabel.setText("Output:", juce::dontSendNotification);
    outputDeviceLabel.setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(outputDeviceLabel);

    outputDeviceCombo.onChange = [this]() { if (!isUpdatingControls) applyDeviceSettings(); };
    addAndMakeVisible(outputDeviceCombo);

    // Sample Rate
    sampleRateLabel.setText("Sample Rate:", juce::dontSendNotification);
    sampleRateLabel.setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(sampleRateLabel);

    sampleRateCombo.addItem("44100 Hz", 1);
    sampleRateCombo.addItem("48000 Hz", 2);
    sampleRateCombo.onChange = [this]() { if (!isUpdatingControls) applyDeviceSettings(); };
    addAndMakeVisible(sampleRateCombo);

    // Buffer Size
    bufferSizeLabel.setText("Buffer Size:", juce::dontSendNotification);
    bufferSizeLabel.setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(bufferSizeLabel);

    bufferSizeCombo.addItem("32 samples (ultra-low)", 1);
    bufferSizeCombo.addItem("64 samples (1.3 ms)", 2);
    bufferSizeCombo.addItem("128 samples (2.7 ms)", 3);
    bufferSizeCombo.addItem("256 samples (5.3 ms)", 4);
    bufferSizeCombo.addItem("512 samples (10.7 ms)", 5);
    bufferSizeCombo.onChange = [this]() { if (!isUpdatingControls) applyDeviceSettings(); };
    addAndMakeVisible(bufferSizeCombo);

    // ASIO Control Panel
    asioControlPanelButton.setButtonText("ASIO Panel");
    asioControlPanelButton.onClick = [this]()
    {
        if (auto* dev = audioEngine.getDeviceManager().getCurrentAudioDevice())
        {
            if (dev->hasControlPanel())
                dev->showControlPanel();
            else
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Audio Driver",
                                                       "The current driver does not provide a hardware control panel.");
        }
    };
    addAndMakeVisible(asioControlPanelButton);

    // Latency Badge
    latencyBadgeLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    latencyBadgeLabel.setJustificationType(juce::Justification::centred);
    latencyBadgeLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff064e3b));
    latencyBadgeLabel.setColour(juce::Label::textColourId, juce::Colour(0xff34d399));
    addAndMakeVisible(latencyBadgeLabel);

    updateDeviceLists();
    updateLatencyBadge();
}

AudioSettingsPanel::~AudioSettingsPanel()
{
    audioEngine.removeChangeListener(this);
}

void AudioSettingsPanel::changeListenerCallback(juce::ChangeBroadcaster* /*source*/)
{
    updateDeviceLists();
    updateLatencyBadge();
}

void AudioSettingsPanel::updateDeviceLists()
{
    isUpdatingControls = true;

    auto& dm = audioEngine.getDeviceManager();

    // Populate driver types
    driverTypeCombo.clear();
    const auto& types = dm.getAvailableDeviceTypes();
    int selectedTypeIdx = 1;
    for (int i = 0; i < types.size(); ++i)
    {
        auto typeName = types[i]->getTypeName();
        driverTypeCombo.addItem(typeName, i + 1);
        if (typeName == dm.getCurrentAudioDeviceType())
            selectedTypeIdx = i + 1;
    }
    driverTypeCombo.setSelectedId(selectedTypeIdx, juce::dontSendNotification);

    // Populate input & output devices
    inputDeviceCombo.clear();
    outputDeviceCombo.clear();

    if (auto* currentType = dm.getCurrentDeviceTypeObject())
    {
        currentType->scanForDevices();
        auto inNames = currentType->getDeviceNames(true);
        auto outNames = currentType->getDeviceNames(false);

        for (int i = 0; i < inNames.size(); ++i)
            inputDeviceCombo.addItem(inNames[i], i + 1);

        for (int i = 0; i < outNames.size(); ++i)
            outputDeviceCombo.addItem(outNames[i], i + 1);
    }

    juce::AudioDeviceManager::AudioDeviceSetup setup;
    dm.getAudioDeviceSetup(setup);

    inputDeviceCombo.setText(setup.inputDeviceName, juce::dontSendNotification);
    outputDeviceCombo.setText(setup.outputDeviceName, juce::dontSendNotification);

    if (std::abs(setup.sampleRate - 44100.0) < 100.0)
        sampleRateCombo.setSelectedId(1, juce::dontSendNotification);
    else
        sampleRateCombo.setSelectedId(2, juce::dontSendNotification);

    switch (setup.bufferSize)
    {
        case 32:  bufferSizeCombo.setSelectedId(1, juce::dontSendNotification); break;
        case 64:  bufferSizeCombo.setSelectedId(2, juce::dontSendNotification); break;
        case 128: bufferSizeCombo.setSelectedId(3, juce::dontSendNotification); break;
        case 256: bufferSizeCombo.setSelectedId(4, juce::dontSendNotification); break;
        case 512: bufferSizeCombo.setSelectedId(5, juce::dontSendNotification); break;
        default:  bufferSizeCombo.setText(juce::String(setup.bufferSize) + " samples", juce::dontSendNotification); break;
    }

    auto* dev = dm.getCurrentAudioDevice();
    asioControlPanelButton.setEnabled(dev != nullptr && dev->hasControlPanel());

    isUpdatingControls = false;
}

void AudioSettingsPanel::applyDeviceSettings()
{
    double rate = (sampleRateCombo.getSelectedId() == 1) ? 44100.0 : 48000.0;
    int bufferSize = 128;
    switch (bufferSizeCombo.getSelectedId())
    {
        case 1: bufferSize = 32; break;
        case 2: bufferSize = 64; break;
        case 3: bufferSize = 128; break;
        case 4: bufferSize = 256; break;
        case 5: bufferSize = 512; break;
        default: break;
    }

    audioEngine.setAudioDevice(driverTypeCombo.getText(),
                               outputDeviceCombo.getText(),
                               inputDeviceCombo.getText(),
                               rate,
                               bufferSize);

    updateLatencyBadge();
}

void AudioSettingsPanel::updateLatencyBadge()
{
    double latencyMs = audioEngine.getHardwareLatencyMs();
    int buffer = audioEngine.getCurrentBufferSize();
    double rate = audioEngine.getCurrentSampleRate();

    juce::String text = juce::String::formatted("Buffer: %.2f ms (%d @ %.1fkHz)",
                                                latencyMs, buffer, rate / 1000.0);
    latencyBadgeLabel.setText(text, juce::dontSendNotification);

    if (latencyMs <= 6.0)
    {
        latencyBadgeLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff064e3b));
        latencyBadgeLabel.setColour(juce::Label::textColourId, juce::Colour(0xff34d399));
    }
    else if (latencyMs <= 12.0)
    {
        latencyBadgeLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff78350f));
        latencyBadgeLabel.setColour(juce::Label::textColourId, juce::Colour(0xfffbbf24));
    }
    else
    {
        latencyBadgeLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff7f1d1d));
        latencyBadgeLabel.setColour(juce::Label::textColourId, juce::Colour(0xfff87171));
    }
}

void AudioSettingsPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff181a20));
    g.setColour(juce::Colour(0xff27272a));
    g.drawRect(getLocalBounds(), 1);
}

void AudioSettingsPanel::resized()
{
    auto area = getLocalBounds().reduced(10, 6);

    titleLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(4);

    auto row1 = area.removeFromTop(28);
    driverTypeLabel.setBounds(row1.removeFromLeft(75));
    driverTypeCombo.setBounds(row1.removeFromLeft(130));
    row1.removeFromLeft(12);

    inputDeviceLabel.setBounds(row1.removeFromLeft(45));
    inputDeviceCombo.setBounds(row1.removeFromLeft(200));
    row1.removeFromLeft(12);

    outputDeviceLabel.setBounds(row1.removeFromLeft(50));
    outputDeviceCombo.setBounds(row1.removeFromLeft(200));

    area.removeFromTop(6);
    auto row2 = area.removeFromTop(28);

    sampleRateLabel.setBounds(row2.removeFromLeft(80));
    sampleRateCombo.setBounds(row2.removeFromLeft(110));
    row2.removeFromLeft(12);

    bufferSizeLabel.setBounds(row2.removeFromLeft(75));
    bufferSizeCombo.setBounds(row2.removeFromLeft(160));
    row2.removeFromLeft(12);

    asioControlPanelButton.setBounds(row2.removeFromLeft(100));
    row2.removeFromLeft(12);

    latencyBadgeLabel.setBounds(row2.removeFromLeft(230));
}

} // namespace paasphoos
