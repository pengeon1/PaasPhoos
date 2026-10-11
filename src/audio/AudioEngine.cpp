#include "AudioEngine.h"

#if JUCE_WINDOWS
#include <windows.h>
#include <avrt.h>
#pragma comment(lib, "avrt.lib")
#endif

namespace paasphoos
{

AudioEngine::AudioEngine()
    : currentSampleRate(48000.0),
      currentBufferSize(128)
{
}

AudioEngine::~AudioEngine()
{
    shutdown();
}

void AudioEngine::enableWindowsMmcss()
{
#if JUCE_WINDOWS
    DWORD taskIndex = 0;
    HANDLE mmcssHandle = AvSetMmThreadCharacteristicsW(L"Pro Audio", &taskIndex);
    if (mmcssHandle != nullptr)
    {
        AvSetMmThreadPriority(mmcssHandle, AVRT_PRIORITY_CRITICAL);
    }
#endif
}

bool AudioEngine::initialize()
{
    enableWindowsMmcss();

    // Initialize audio device manager with 2 inputs and 2 outputs
    juce::String error = deviceManager.initialiseWithDefaultDevices(2, 2);
    if (error.isNotEmpty())
    {
        DBG("Audio device initialization error: " + error);
    }

    deviceManager.addAudioCallback(this);
    return true;
}

void AudioEngine::shutdown()
{
    deviceManager.removeAudioCallback(this);
    deviceManager.closeAudioDevice();
}

juce::String AudioEngine::getCurrentDeviceName() const
{
    if (auto* currentDevice = deviceManager.getCurrentAudioDevice())
        return currentDevice->getName();
    return "None";
}

juce::String AudioEngine::getCurrentDeviceTypeName() const
{
    if (auto* currentDevice = deviceManager.getCurrentAudioDevice())
        return currentDevice->getTypeName();
    return "None";
}

double AudioEngine::getCurrentSampleRate() const
{
    if (auto* currentDevice = deviceManager.getCurrentAudioDevice())
        return currentDevice->getCurrentSampleRate();
    return currentSampleRate;
}

int AudioEngine::getCurrentBufferSize() const
{
    if (auto* currentDevice = deviceManager.getCurrentAudioDevice())
        return currentDevice->getCurrentBufferSizeSamples();
    return currentBufferSize;
}

double AudioEngine::getHardwareLatencyMs() const
{
    if (auto* currentDevice = deviceManager.getCurrentAudioDevice())
    {
        int inLatency = currentDevice->getInputLatencyInSamples();
        int outLatency = currentDevice->getOutputLatencyInSamples();
        double rate = currentDevice->getCurrentSampleRate();
        if (rate > 0.0)
            return ((inLatency + outLatency + currentDevice->getCurrentBufferSizeSamples()) / rate) * 1000.0;
    }
    if (currentSampleRate > 0.0)
        return (static_cast<double>(currentBufferSize) / currentSampleRate) * 1000.0;
    return 0.0;
}

void AudioEngine::setAudioDevice(const juce::String& deviceTypeName,
                                const juce::String& outputDeviceName,
                                const juce::String& inputDeviceName,
                                double sampleRate,
                                int bufferSize)
{
    if (deviceTypeName.isNotEmpty() && deviceTypeName != deviceManager.getCurrentAudioDeviceType())
    {
        deviceManager.setCurrentAudioDeviceType(deviceTypeName, true);
    }

    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup(setup);

    if (outputDeviceName.isNotEmpty())
        setup.outputDeviceName = outputDeviceName;
    if (inputDeviceName.isNotEmpty())
        setup.inputDeviceName = inputDeviceName;
    if (sampleRate > 1000.0)
        setup.sampleRate = sampleRate;
    if (bufferSize > 0)
        setup.bufferSize = bufferSize;

    setup.useDefaultInputChannels = true;
    setup.useDefaultOutputChannels = true;

    deviceManager.setAudioDeviceSetup(setup, true);
    sendChangeMessage();
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    if (device != nullptr)
    {
        currentSampleRate = device->getCurrentSampleRate();
        currentBufferSize = device->getCurrentBufferSizeSamples();
    }

    metronome.prepareToPlay(currentSampleRate);
    tuner.prepareToPlay(currentSampleRate);

    // Pre-allocate local mixing buffer (real-time safe, no allocations during callback)
    localMixBuffer.setSize(2, std::max(currentBufferSize, 2048), false, false, false);
    localMixBuffer.clear();

    sendChangeMessage();
}

void AudioEngine::audioDeviceStopped()
{
    metronome.reset();
    tuner.reset();
    localMixBuffer.setSize(0, 0);
    sendChangeMessage();
}

double AudioEngine::getCpuUsage() const
{
    return deviceManager.getCpuUsage() * 100.0;
}

void AudioEngine::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                                  int numInputChannels,
                                                  float* const* outputChannelData,
                                                  int numOutputChannels,
                                                  int numSamples,
                                                  const juce::AudioIODeviceCallbackContext& /*context*/)
{
    // Real-time critical section: Zero heap allocations or system calls!

    // Clear output channels first
    for (int ch = 0; ch < numOutputChannels; ++ch)
    {
        if (outputChannelData[ch] != nullptr)
            std::fill_n(outputChannelData[ch], numSamples, 0.0f);
    }

    if (numSamples <= 0)
        return;

    // Check solo and mute logic
    const bool solo1 = channel1.solo.load(std::memory_order_relaxed);
    const bool solo2 = channel2.solo.load(std::memory_order_relaxed);
    const bool anySolo = solo1 || solo2;

    const bool play1 = !channel1.mute.load(std::memory_order_relaxed) && (!anySolo || solo1);
    const bool play2 = !channel2.mute.load(std::memory_order_relaxed) && (!anySolo || solo2);

    const float gain1 = channel1.gain.load(std::memory_order_relaxed);
    const float gain2 = channel2.gain.load(std::memory_order_relaxed);

    const float pan1 = juce::jlimit(-1.0f, 1.0f, channel1.pan.load(std::memory_order_relaxed));
    const float pan2 = juce::jlimit(-1.0f, 1.0f, channel2.pan.load(std::memory_order_relaxed));

    // Pan laws (linear taper)
    const float panL1 = std::sqrt(juce::jlimit(0.0f, 1.0f, (1.0f - pan1) * 0.5f));
    const float panR1 = std::sqrt(juce::jlimit(0.0f, 1.0f, (1.0f + pan1) * 0.5f));

    const float panL2 = std::sqrt(juce::jlimit(0.0f, 1.0f, (1.0f - pan2) * 0.5f));
    const float panR2 = std::sqrt(juce::jlimit(0.0f, 1.0f, (1.0f + pan2) * 0.5f));

    // Prepare local mix buffer
    float* mixL = localMixBuffer.getWritePointer(0);
    float* mixR = localMixBuffer.getWritePointer(1);
    std::fill_n(mixL, numSamples, 0.0f);
    std::fill_n(mixR, numSamples, 0.0f);

    // Process Channel 1 (Instrument / Guitar)
    float ch1Peak = 0.0f;
    const float* in1 = (numInputChannels > 0) ? inputChannelData[0] : nullptr;
    if (in1 != nullptr)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float s = in1[i];
            float absS = std::abs(s);
            if (absS > ch1Peak) ch1Peak = absS;

            if (play1)
            {
                float processed = s * gain1;
                mixL[i] += processed * panL1;
                mixR[i] += processed * panR1;
            }
        }

        if (ch1Peak >= 0.999f)
            channel1.clipDetected.store(true, std::memory_order_relaxed);

        if (tuningInputChannel.load(std::memory_order_relaxed) == 0)
            tuner.pushSamples(in1, numSamples);
    }

    // Decay Ch1 peak for smooth meter visualization
    float prevPeak1 = channel1.peakLevel.load(std::memory_order_relaxed);
    float decayedPeak1 = std::max(ch1Peak, prevPeak1 * 0.92f);
    channel1.peakLevel.store(decayedPeak1, std::memory_order_relaxed);

    // Process Channel 2 (Vocal / Secondary)
    float ch2Peak = 0.0f;
    const float* in2 = (numInputChannels > 1) ? inputChannelData[1] : nullptr;
    if (in2 != nullptr)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float s = in2[i];
            float absS = std::abs(s);
            if (absS > ch2Peak) ch2Peak = absS;

            if (play2)
            {
                float processed = s * gain2;
                mixL[i] += processed * panL2;
                mixR[i] += processed * panR2;
            }
        }

        if (ch2Peak >= 0.999f)
            channel2.clipDetected.store(true, std::memory_order_relaxed);

        if (tuningInputChannel.load(std::memory_order_relaxed) == 1)
            tuner.pushSamples(in2, numSamples);
    }

    // Decay Ch2 peak
    float prevPeak2 = channel2.peakLevel.load(std::memory_order_relaxed);
    float decayedPeak2 = std::max(ch2Peak, prevPeak2 * 0.92f);
    channel2.peakLevel.store(decayedPeak2, std::memory_order_relaxed);

    // Add Metronome click directly to output mix
    metronome.process(localMixBuffer, 0, numSamples);

    // Apply Master Volume and write to hardware output
    const float masterVol = masterVolume.load(std::memory_order_relaxed);
    float outPeakL = 0.0f;
    float outPeakR = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        float outL = mixL[i] * masterVol;
        float outR = mixR[i] * masterVol;

        float absL = std::abs(outL);
        float absR = std::abs(outR);
        if (absL > outPeakL) outPeakL = absL;
        if (absR > outPeakR) outPeakR = absR;

        if (numOutputChannels > 0 && outputChannelData[0] != nullptr)
            outputChannelData[0][i] = outL;
        if (numOutputChannels > 1 && outputChannelData[1] != nullptr)
            outputChannelData[1][i] = outR;
    }

    // Update Master Peaks and Clipping
    if (outPeakL >= 0.999f || outPeakR >= 0.999f)
        masterClipped.store(true, std::memory_order_relaxed);

    float prevMasterL = masterPeakL.load(std::memory_order_relaxed);
    float prevMasterR = masterPeakR.load(std::memory_order_relaxed);
    masterPeakL.store(std::max(outPeakL, prevMasterL * 0.92f), std::memory_order_relaxed);
    masterPeakR.store(std::max(outPeakR, prevMasterR * 0.92f), std::memory_order_relaxed);
}

} // namespace paasphoos
