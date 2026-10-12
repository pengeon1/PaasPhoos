#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "DrumEngine.h"
#include <array>
#include <vector>
#include <atomic>

namespace paasphoos
{

enum class GrooveStyle
{
    ROCK_FOUR_ON_FLOOR,
    POP_BACKBEAT,
    METAL_DOUBLE_BASS,
    FUNK_GHOST_NOTES,
    EDM_HOUSE,
    BOLLYWOOD_TEEN_TAAL
};

class DrumSequencer
{
public:
    static constexpr int NUM_STEPS = 16;
    static constexpr int NUM_TRACKS = DrumEngine::NUM_VOICES;

    DrumSequencer(DrumEngine& engine);
    ~DrumSequencer() = default;

    void prepare(double sampleRate);
    void reset();

    void setPlaying(bool play) noexcept { isPlaying.store(play, std::memory_order_relaxed); }
    bool getPlaying() const noexcept { return isPlaying.load(std::memory_order_relaxed); }

    void setBpm(float bpm) noexcept { currentBpm.store(juce::jlimit(40.0f, 300.0f, bpm), std::memory_order_relaxed); }
    float getBpm() const noexcept { return currentBpm.load(std::memory_order_relaxed); }

    void setSwing(float swingPercent) noexcept { swing.store(juce::jlimit(0.0f, 0.6f, swingPercent), std::memory_order_relaxed); }
    float getSwing() const noexcept { return swing.load(std::memory_order_relaxed); }

    void setHumanize(bool enable) noexcept { humanize.store(enable, std::memory_order_relaxed); }
    bool getHumanize() const noexcept { return humanize.load(std::memory_order_relaxed); }

    void setLatencyCompensationMs(float ms) noexcept { latencyCompMs.store(juce::jlimit(-50.0f, 50.0f, ms), std::memory_order_relaxed); }
    float getLatencyCompensationMs() const noexcept { return latencyCompMs.load(std::memory_order_relaxed); }

    // Step editing
    void setStep(int track, int step, float velocity);
    float getStep(int track, int step) const;
    void toggleStep(int track, int step);
    void clearPattern();

    // Auto-Groove Generation
    void generateGroove(GrooveStyle style, float intensity = 0.8f);
    void triggerFill();

    // Audio callback clock advance
    void processBlock(int numSamples);

    int getCurrentStep() const noexcept { return currentStep.load(std::memory_order_relaxed); }

    // MIDI Export
    bool exportMidiFile(const juce::File& destinationFile);

private:
    void triggerStep(int stepIndex);

    DrumEngine& drumEngine;
    double sampleRate{48000.0};

    std::atomic<bool> isPlaying{false};
    std::atomic<float> currentBpm{120.0f};
    std::atomic<float> swing{0.0f};
    std::atomic<bool> humanize{false};
    std::atomic<float> latencyCompMs{0.0f};

    std::atomic<int> currentStep{0};
    double stepSampleCounter{0.0};
    int barCounter{0};

    // Grid: [track][step] -> velocity (0.0f = off, 0.1 to 1.0f)
    std::array<std::array<std::atomic<float>, NUM_STEPS>, NUM_TRACKS> grid;

    // MIDI Recording buffer for export
    juce::MidiMessageSequence recordedMidi;
    double accumulatedMidiTimeSeconds{0.0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumSequencer)
};

} // namespace paasphoos
