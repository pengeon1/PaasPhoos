#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <atomic>
#include <memory>
#include "Metronome.h"
#include "Tuner.h"
#include "RealtimeRingBuffer.h"

namespace paasphoos
{

struct ChannelSettings
{
    std::atomic<float> gain{1.0f};         // 0.0 to 2.0 (linear, 1.0 = 0dB)
    std::atomic<float> pan{0.0f};          // -1.0 (Left) to +1.0 (Right)
    std::atomic<bool> mute{false};
    std::atomic<bool> solo{false};
    std::atomic<float> peakLevel{0.0f};    // 0.0 to 1.0 (with decay)
    std::atomic<bool> clipDetected{false};
};

class AudioEngine : public juce::AudioIODeviceCallback,
                    public juce::ChangeBroadcaster
{
public:
    AudioEngine();
    ~AudioEngine() override;

    // Initialization & Hardware Management
    bool initialize();
    void shutdown();

    juce::AudioDeviceManager& getDeviceManager() { return deviceManager; }

    // Audio device settings helpers
    juce::String getCurrentDeviceName() const;
    juce::String getCurrentDeviceTypeName() const;
    double getCurrentSampleRate() const;
    int getCurrentBufferSize() const;
    double getHardwareLatencyMs() const;

    void setAudioDevice(const juce::String& deviceTypeName,
                        const juce::String& outputDeviceName,
                        const juce::String& inputDeviceName,
                        double sampleRate,
                        int bufferSize);

    // AudioIODeviceCallback overrides (Real-time thread)
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                         int numInputChannels,
                                         float* const* outputChannelData,
                                         int numOutputChannels,
                                         int numSamples,
                                         const juce::AudioIODeviceCallbackContext& context) override;

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

    // Local Mixer Controls
    ChannelSettings& getChannel1() { return channel1; } // Instrument / Guitar
    ChannelSettings& getChannel2() { return channel2; } // Vocal / Secondary
    const ChannelSettings& getChannel1() const { return channel1; }
    const ChannelSettings& getChannel2() const { return channel2; }

    void setMasterVolume(float vol) noexcept { masterVolume.store(juce::jlimit(0.0f, 2.0f, vol), std::memory_order_relaxed); }
    float getMasterVolume() const noexcept { return masterVolume.load(std::memory_order_relaxed); }

    float getMasterPeakLeft() const noexcept { return masterPeakL.load(std::memory_order_relaxed); }
    float getMasterPeakRight() const noexcept { return masterPeakR.load(std::memory_order_relaxed); }
    bool getMasterClipped() const noexcept { return masterClipped.load(std::memory_order_relaxed); }
    void clearMasterClip() noexcept { masterClipped.store(false, std::memory_order_relaxed); }

    // Metronome & Tuner
    Metronome& getMetronome() { return metronome; }
    Tuner& getTuner() { return tuner; }

    // Tuning input channel selection (0 = Channel 1, 1 = Channel 2)
    void setTuningInputChannel(int ch) noexcept { tuningInputChannel.store(ch, std::memory_order_relaxed); }
    int getTuningInputChannel() const noexcept { return tuningInputChannel.load(std::memory_order_relaxed); }

    // Diagnostic CPU Load
    double getCpuUsage() const;

private:
    void enableWindowsMmcss();

    juce::AudioDeviceManager deviceManager;
    double currentSampleRate;
    int currentBufferSize;

    ChannelSettings channel1; // Input 1: Guitar / Instrument
    ChannelSettings channel2; // Input 2: Vocal / Aux

    std::atomic<float> masterVolume{1.0f};
    std::atomic<float> masterPeakL{0.0f};
    std::atomic<float> masterPeakR{0.0f};
    std::atomic<bool> masterClipped{false};

    std::atomic<int> tuningInputChannel{0};

    Metronome metronome;
    Tuner tuner;

    // Temporary real-time buffers for mixing (pre-allocated)
    juce::AudioBuffer<float> localMixBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};

} // namespace paasphoos
