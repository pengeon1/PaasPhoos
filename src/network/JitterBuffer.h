#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <atomic>
#include <cmath>
#include <algorithm>
#include "NetworkPacket.h"

namespace paasphoos
{

enum class JitterBufferMode
{
    AUTO,
    MANUAL
};

struct JitterBufferMetrics
{
    float currentFillMs{0.0f};
    float targetDelayMs{0.0f};
    float estimatedJitterMs{0.0f};
    float driftRatioPpm{0.0f};
    uint32_t packetLossCount{0};
    uint32_t concealmentCount{0};
    uint32_t underrunCount{0};
    bool isPrimed{false};
};

/**
 * Real-time Adaptive Jitter Buffer with Packet Loss Concealment (PLC)
 * and Sample Clock Drift Correction.
 *
 * Real-time safety guarantees:
 * - pushPacket() and pullAudio() are non-blocking and allocation-free.
 * - Clock drift correction continuously tracks smoothed buffer occupancy
 *   and dynamically resamples audio (via 4-point Hermite cubic interpolation)
 *   by +/-0.05% to +/-0.1% to prevent slow buffer desync over long rehearsal sessions.
 * - Packet Loss Concealment (PLC) gracefully decays previous frames and crossfades
 *   upon packet resumption to eliminate audible digital pops/clicks.
 */
class JitterBuffer
{
public:
    static constexpr int DEFAULT_CAPACITY_SAMPLES = 16384; // ~340 ms at 48kHz
    static constexpr int CONCEALMENT_CROSSFADE_SAMPLES = 32;

    JitterBuffer();
    ~JitterBuffer() = default;

    void prepare(double sampleRate, int numChannels = 2, int capacitySamples = DEFAULT_CAPACITY_SAMPLES);
    void reset();

    // Mode and Target Playout Delay Configuration
    void setMode(JitterBufferMode mode) noexcept;
    JitterBufferMode getMode() const noexcept { return mode.load(std::memory_order_relaxed); }

    void setManualTargetDelayMs(float delayMs) noexcept;
    float getManualTargetDelayMs() const noexcept { return manualTargetDelayMs.load(std::memory_order_relaxed); }

    // Packet ingestion (Called from Network Worker Thread)
    bool pushPacket(uint32_t sequenceNumber,
                    uint64_t timestampUs,
                    const float* const* channelData,
                    int numChannels,
                    int numSamples) noexcept;

    // Real-time audio pull (Called from Audio Engine Callback Thread)
    void pullAudio(float* const* destChannels, int numChannels, int numSamples) noexcept;

    // Live Metrics & Telemetry
    JitterBufferMetrics getMetrics() const noexcept;

private:
    double sampleRate{48000.0};
    int numChannels{2};
    int capacity{DEFAULT_CAPACITY_SAMPLES};

    // Circular sample storage
    std::vector<float> sampleStorage; // Planar: [ch * capacity + sample]
    std::atomic<int> writeHead{0};
    std::atomic<float> readHeadFractional{0.0f};
    std::atomic<int> samplesAvailable{0};

    // Mode & Playout targets
    std::atomic<JitterBufferMode> mode{JitterBufferMode::AUTO};
    std::atomic<float> manualTargetDelayMs{8.0f};
    std::atomic<float> targetDelayMs{8.0f};
    std::atomic<float> estimatedJitterMs{0.5f};

    // Priming state
    std::atomic<bool> isPrimed{false};

    // RFC 3550 Inter-Arrival Jitter Tracking
    uint32_t lastReceivedSeq{0};
    uint64_t lastTransitDelayUs{0};
    bool hasReceivedFirstPacket{false};

    // Drift Correction Tracking
    float smoothedOccupancy{0.0f};
    std::atomic<float> currentDriftPpm{0.0f};

    // Packet Loss Concealment (PLC)
    float lastSampleCh0{0.0f};
    float lastSampleCh1{0.0f};
    float concealmentDecay{1.0f};
    int samplesInConcealment{0};

    // Telemetry Counters
    std::atomic<uint32_t> packetLossCount{0};
    std::atomic<uint32_t> concealmentCount{0};
    std::atomic<uint32_t> underrunCount{0};

    // Helper interpolation functions
    float hermiteInterpolate(int channel, float index) const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JitterBuffer)
};

} // namespace paasphoos
