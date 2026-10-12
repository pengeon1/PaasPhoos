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
      currentBufferSize(128),
      drumSequencer(drumEngine)
{
    localMixBuffer.setSize(2, 2048);
    fxProcessBuffer.setSize(2, 2048);
    drumMixBuffer.setSize(2, 2048);
    for (int i = 0; i < MAX_REMOTE_STRIPS; ++i)
    {
        remoteAudioBuffers[i].setSize(2, 2048);
    }
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
    lanTransport.setNetworkSimulator(&networkSimulator);
    return true;
}

void AudioEngine::shutdown()
{
    sessionRecorder.stopRecording();
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
    builtInFx.prepare(currentSampleRate, currentBufferSize);
    pluginHostManager.initialize(currentSampleRate, currentBufferSize);
    drumEngine.prepare(currentSampleRate);
    drumSequencer.prepare(currentSampleRate);

    int allocSize = std::max(currentBufferSize, 2048);
    localMixBuffer.setSize(2, allocSize, false, false, false);
    localMixBuffer.clear();

    fxProcessBuffer.setSize(2, allocSize, false, false, false);
    fxProcessBuffer.clear();

    drumMixBuffer.setSize(2, allocSize, false, false, false);
    drumMixBuffer.clear();

    for (int i = 0; i < MAX_REMOTE_STRIPS; ++i)
    {
        remoteAudioBuffers[i].setSize(2, allocSize, false, false, false);
        remoteAudioBuffers[i].clear();
    }

    float halfHwLatency = static_cast<float>(getHardwareLatencyMs() * 0.5);
    lanTransport.setHardwareLatencies(halfHwLatency, halfHwLatency);

    sendChangeMessage();
}

void AudioEngine::audioDeviceStopped()
{
    metronome.reset();
    tuner.reset();
    builtInFx.reset();
    drumEngine.reset();
    drumSequencer.reset();

    localMixBuffer.setSize(0, 0);
    fxProcessBuffer.setSize(0, 0);
    drumMixBuffer.setSize(0, 0);

    for (int i = 0; i < MAX_REMOTE_STRIPS; ++i)
    {
        remoteAudioBuffers[i].setSize(0, 0);
    }
    sendChangeMessage();
}

double AudioEngine::getCpuUsage() const
{
    return deviceManager.getCpuUsage() * 100.0;
}

int AudioEngine::getParticipantIdForRemoteStrip(int stripIndex) const noexcept
{
    uint8_t myId = lanTransport.getMyParticipantId();
    int count = 0;
    for (int p = 0; p < MAX_PARTICIPANTS; ++p)
    {
        if (p == myId) continue;
        if (count == stripIndex) return p;
        count++;
    }
    return -1;
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

    // Check solo states across local channels and all remote peer channels
    const bool solo1 = channel1.solo.load(std::memory_order_relaxed);
    const bool solo2 = channel2.solo.load(std::memory_order_relaxed);
    bool anySolo = solo1 || solo2;

    for (int s = 0; s < MAX_REMOTE_STRIPS; ++s)
    {
        if (remotePeers[s].solo.load(std::memory_order_relaxed))
        {
            anySolo = true;
            break;
        }
    }

    const bool play1 = !channel1.mute.load(std::memory_order_relaxed) && (!anySolo || solo1);
    const bool play2 = !channel2.mute.load(std::memory_order_relaxed) && (!anySolo || solo2);

    const float gain1 = channel1.gain.load(std::memory_order_relaxed);
    const float gain2 = channel2.gain.load(std::memory_order_relaxed);

    const float pan1 = juce::jlimit(-1.0f, 1.0f, channel1.pan.load(std::memory_order_relaxed));
    const float pan2 = juce::jlimit(-1.0f, 1.0f, channel2.pan.load(std::memory_order_relaxed));

    const float panL1 = std::sqrt(juce::jlimit(0.0f, 1.0f, (1.0f - pan1) * 0.5f));
    const float panR1 = std::sqrt(juce::jlimit(0.0f, 1.0f, (1.0f + pan1) * 0.5f));

    const float panL2 = std::sqrt(juce::jlimit(0.0f, 1.0f, (1.0f - pan2) * 0.5f));
    const float panR2 = std::sqrt(juce::jlimit(0.0f, 1.0f, (1.0f + pan2) * 0.5f));

    // Prepare local mix buffer
    float* mixL = localMixBuffer.getWritePointer(0);
    float* mixR = localMixBuffer.getWritePointer(1);
    std::fill_n(mixL, numSamples, 0.0f);
    std::fill_n(mixR, numSamples, 0.0f);

    // 1. Process Channel 1 (Instrument / Guitar) through Built-in FX Chain & VST3 Host
    float ch1Peak = 0.0f;
    const float* in1 = (numInputChannels > 0) ? inputChannelData[0] : nullptr;

    if (in1 != nullptr)
    {
        // Copy to FX process buffer
        float* fx0 = fxProcessBuffer.getWritePointer(0);
        float* fx1 = fxProcessBuffer.getWritePointer(1);
        std::copy_n(in1, numSamples, fx0);
        std::copy_n(in1, numSamples, fx1);

        // Run DSP FX rack (Gate -> Drive -> Comp -> EQ -> Reverb)
        builtInFx.process(fxProcessBuffer, 0, numSamples);

        // Run VST3 Plugin Host if an external plugin is loaded
        emptyMidiBuffer.clear();
        pluginHostManager.process(fxProcessBuffer, emptyMidiBuffer);

        for (int i = 0; i < numSamples; ++i)
        {
            float s = fx0[i];
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

    float prevPeak1 = channel1.peakLevel.load(std::memory_order_relaxed);
    channel1.peakLevel.store(std::max(ch1Peak, prevPeak1 * 0.92f), std::memory_order_relaxed);

    // 2. Process Channel 2 (Vocal / Secondary)
    float ch2Peak = 0.0f;
    const float* in2 = (numInputChannels > 1) ? inputChannelData[1] : nullptr;
    if (in2 != nullptr)
    {
        // Talkback ducking/boost if PTT active
        float pttBoost = chatTalkback.isPushToTalkActive() ? 1.4f : 1.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            float s = in2[i] * pttBoost;
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

    float prevPeak2 = channel2.peakLevel.load(std::memory_order_relaxed);
    channel2.peakLevel.store(std::max(ch2Peak, prevPeak2 * 0.92f), std::memory_order_relaxed);

    // 3. Process Drum Machine Subsystem (Synthesizer + Step Sequencer)
    drumMixBuffer.clear();
    drumSequencer.processBlock(numSamples);
    drumEngine.renderNextBlock(drumMixBuffer, 0, numSamples);

    const float* drumL = drumMixBuffer.getReadPointer(0);
    const float* drumR = drumMixBuffer.getReadPointer(1);

    for (int i = 0; i < numSamples; ++i)
    {
        mixL[i] += drumL[i];
        mixR[i] += drumR[i];
    }

    // 4. Send local processed audio to band over network (local monitoring remains zero-latency)
    lanTransport.pushLocalAudio(localMixBuffer.getArrayOfReadPointers(), 2, numSamples);

    // 5. Pull and mix audio for all 5 remote band member channels
    for (int s = 0; s < MAX_REMOTE_STRIPS; ++s)
    {
        int partId = getParticipantIdForRemoteStrip(s);
        auto* peer = (partId >= 0) ? lanTransport.getPeer(partId) : nullptr;
        bool isPeerActive = (peer != nullptr && peer->connected.load(std::memory_order_relaxed));

        if (!isPeerActive)
        {
            float prev = remotePeers[s].peakLevel.load(std::memory_order_relaxed);
            remotePeers[s].peakLevel.store(prev * 0.90f, std::memory_order_relaxed);
            continue;
        }

        lanTransport.pullPeerAudio(partId, remoteAudioBuffers[s].getArrayOfWritePointers(), 2, numSamples);
        const float* rL = remoteAudioBuffers[s].getReadPointer(0);
        const float* rR = remoteAudioBuffers[s].getReadPointer(1);

        const bool soloPeer = remotePeers[s].solo.load(std::memory_order_relaxed);
        const bool playPeer = !remotePeers[s].mute.load(std::memory_order_relaxed) && (!anySolo || soloPeer);
        const float gainPeer = remotePeers[s].gain.load(std::memory_order_relaxed);
        const float panPeer = juce::jlimit(-1.0f, 1.0f, remotePeers[s].pan.load(std::memory_order_relaxed));
        const float panLPeer = std::sqrt(juce::jlimit(0.0f, 1.0f, (1.0f - panPeer) * 0.5f));
        const float panRPeer = std::sqrt(juce::jlimit(0.0f, 1.0f, (1.0f + panPeer) * 0.5f));

        float stripPeak = 0.0f;
        for (int i = 0; i < numSamples; ++i)
        {
            float s0 = rL[i];
            float s1 = rR[i];
            float absMax = std::max(std::abs(s0), std::abs(s1));
            if (absMax > stripPeak) stripPeak = absMax;

            if (playPeer)
            {
                float procL = s0 * gainPeer;
                float procR = s1 * gainPeer;
                mixL[i] += (procL * panLPeer);
                mixR[i] += (procR * panRPeer);
            }
        }

        if (stripPeak >= 0.999f)
            remotePeers[s].clipDetected.store(true, std::memory_order_relaxed);

        float prevPeerPeak = remotePeers[s].peakLevel.load(std::memory_order_relaxed);
        remotePeers[s].peakLevel.store(std::max(stripPeak, prevPeerPeak * 0.92f), std::memory_order_relaxed);

        // Multitrack recorder feed for remote peer
        if (sessionRecorder.isRecording())
        {
            sessionRecorder.pushTrackAudio(2 + s, remoteAudioBuffers[s].getArrayOfReadPointers(), 2, numSamples);
        }
    }

    // Feed local stems to multitrack recorder
    if (sessionRecorder.isRecording())
    {
        if (in1 != nullptr)
        {
            const float* in1Planes[2] = { in1, in1 };
            sessionRecorder.pushTrackAudio(0, in1Planes, 2, numSamples);
        }
        if (in2 != nullptr)
        {
            const float* in2Planes[2] = { in2, in2 };
            sessionRecorder.pushTrackAudio(1, in2Planes, 2, numSamples);
        }
        sessionRecorder.pushTrackAudio(7, drumMixBuffer.getArrayOfReadPointers(), 2, numSamples);
        sessionRecorder.pushMasterAudio(localMixBuffer.getArrayOfReadPointers(), 2, numSamples);
    }

    // 6. Add Metronome click to local monitor mix
    metronome.process(localMixBuffer, 0, numSamples);

    // 7. Apply Master Volume and write to hardware output
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
