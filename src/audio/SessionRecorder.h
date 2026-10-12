#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "RealtimeRingBuffer.h"
#include <memory>
#include <atomic>
#include <vector>

namespace paasphoos
{

class SessionRecorder : public juce::Thread
{
public:
    static constexpr int MAX_RECORD_TRACKS = 8;
    static constexpr int FIFO_CAPACITY = 65536; // ~1.3 seconds buffer headroom

    SessionRecorder();
    ~SessionRecorder() override;

    bool startRecording(const juce::File& destinationFolder, double sampleRate);
    void stopRecording();

    bool isRecording() const noexcept { return isRecordingActive.load(std::memory_order_relaxed); }
    double getRecordingDurationSeconds() const noexcept;

    // Real-time audio push from callback thread (Lock-free & Allocation-free)
    void pushMasterAudio(const float* const* channelData, int numChannels, int numSamples) noexcept;
    void pushTrackAudio(int trackIndex, const float* const* channelData, int numChannels, int numSamples) noexcept;

private:
    void run() override;

    std::atomic<bool> isRecordingActive{false};
    double currentSampleRate{48000.0};
    juce::Time recordStartTime;

    juce::File sessionFolder;
    juce::WavAudioFormat wavFormat;

    // Ring buffers between audio thread and file writer thread
    AudioRingBuffer<float> masterRing;
    std::array<AudioRingBuffer<float>, MAX_RECORD_TRACKS> trackRings;

    // File Writers
    std::unique_ptr<juce::AudioFormatWriter> masterWriter;
    std::array<std::unique_ptr<juce::AudioFormatWriter>, MAX_RECORD_TRACKS> trackWriters;

    std::vector<float> readBufferL;
    std::vector<float> readBufferR;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SessionRecorder)
};

} // namespace paasphoos
