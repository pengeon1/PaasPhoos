#pragma once

#include <juce_core/juce_core.h>
#include <atomic>
#include <vector>
#include <cmath>
#include <array>
#include <string>

namespace paasphoos
{

struct TunerResult
{
    float frequency = 0.0f;
    int noteNumber = -1; // MIDI note number 0-127
    float centsOffset = 0.0f; // -50.0 to +50.0 cents
    float confidence = 0.0f; // 0.0 to 1.0
    bool isActive = false;
};

class Tuner
{
public:
    static constexpr int ANALYSIS_BUFFER_SIZE = 2048;

    Tuner()
        : sampleRate(48000.0),
          writeIndex(0),
          enabled(true)
    {
        analysisBuffer.fill(0.0f);
        diffBuffer.resize(ANALYSIS_BUFFER_SIZE / 2, 0.0f);
        runningSumBuffer.resize(ANALYSIS_BUFFER_SIZE / 2, 0.0f);

        cachedResult.frequency.store(0.0f, std::memory_order_relaxed);
        cachedResult.noteNumber.store(-1, std::memory_order_relaxed);
        cachedResult.centsOffset.store(0.0f, std::memory_order_relaxed);
        cachedResult.confidence.store(0.0f, std::memory_order_relaxed);
        cachedResult.isActive.store(false, std::memory_order_relaxed);
    }

    void prepareToPlay(double newSampleRate)
    {
        sampleRate = (newSampleRate > 1000.0) ? newSampleRate : 48000.0;
        reset();
    }

    void reset()
    {
        writeIndex = 0;
        analysisBuffer.fill(0.0f);
        cachedResult.isActive.store(false, std::memory_order_relaxed);
    }

    void setEnabled(bool isTunerEnabled) noexcept
    {
        enabled.store(isTunerEnabled, std::memory_order_relaxed);
    }

    bool isEnabled() const noexcept
    {
        return enabled.load(std::memory_order_relaxed);
    }

    /**
     * Real-time safe audio push from audio callback.
     * Takes input mono or channel 0 samples.
     */
    void pushSamples(const float* channelData, int numSamples) noexcept
    {
        if (!enabled.load(std::memory_order_relaxed) || channelData == nullptr || numSamples <= 0)
            return;

        for (int i = 0; i < numSamples; ++i)
        {
            analysisBuffer[writeIndex] = channelData[i];
            writeIndex = (writeIndex + 1) % ANALYSIS_BUFFER_SIZE;
        }
    }

    /**
     * Called periodically (e.g. 20-30 times per second from timer/GUI thread) to compute pitch.
     * Keeps heavy autocorrelation off the strict audio callback.
     */
    void updatePitchEstimation()
    {
        if (!enabled.load(std::memory_order_relaxed))
            return;

        // Unroll circular buffer into local linear snapshot
        std::array<float, ANALYSIS_BUFFER_SIZE> snapshot;
        int head = writeIndex;
        for (int i = 0; i < ANALYSIS_BUFFER_SIZE; ++i)
        {
            snapshot[i] = analysisBuffer[(head + i) % ANALYSIS_BUFFER_SIZE];
        }

        // Calculate RMS to ensure signal is above noise threshold
        float sumSquares = 0.0f;
        for (int i = 0; i < ANALYSIS_BUFFER_SIZE; ++i)
            sumSquares += snapshot[i] * snapshot[i];

        float rms = std::sqrt(sumSquares / static_cast<float>(ANALYSIS_BUFFER_SIZE));
        if (rms < 0.005f) // Noise gate: too quiet
        {
            cachedResult.isActive.store(false, std::memory_order_relaxed);
            cachedResult.confidence.store(0.0f, std::memory_order_relaxed);
            return;
        }

        // YIN Pitch Detection Algorithm
        const int halfWindow = ANALYSIS_BUFFER_SIZE / 2;
        const int minLag = static_cast<int>(sampleRate / 1200.0); // max 1200 Hz
        const int maxLag = static_cast<int>(sampleRate / 40.0);   // min 40 Hz (low E string / bass)
        const int clampedMaxLag = std::min(maxLag, halfWindow - 1);

        // Step 1: Difference Function
        diffBuffer[0] = 0.0f;
        for (int tau = 1; tau <= clampedMaxLag; ++tau)
        {
            float diff = 0.0f;
            for (int i = 0; i < halfWindow; ++i)
            {
                float delta = snapshot[i] - snapshot[i + tau];
                diff += delta * delta;
            }
            diffBuffer[tau] = diff;
        }

        // Step 2: Cumulative Mean Normalized Difference
        runningSumBuffer[0] = 1.0f;
        float runningSum = 0.0f;
        for (int tau = 1; tau <= clampedMaxLag; ++tau)
        {
            runningSum += diffBuffer[tau];
            if (runningSum > 0.00001f)
                runningSumBuffer[tau] = diffBuffer[tau] / (runningSum / static_cast<float>(tau));
            else
                runningSumBuffer[tau] = 1.0f;
        }

        // Step 3: Absolute Thresholding
        const float threshold = 0.15f;
        int tauEstimate = -1;
        for (int tau = minLag; tau <= clampedMaxLag; ++tau)
        {
            if (runningSumBuffer[tau] < threshold)
            {
                // Find local minimum
                while (tau + 1 <= clampedMaxLag && runningSumBuffer[tau + 1] < runningSumBuffer[tau])
                {
                    ++tau;
                }
                tauEstimate = tau;
                break;
            }
        }

        // Fallback: Global minimum if no threshold hit
        if (tauEstimate == -1)
        {
            float minVal = 1000.0f;
            for (int tau = minLag; tau <= clampedMaxLag; ++tau)
            {
                if (runningSumBuffer[tau] < minVal)
                {
                    minVal = runningSumBuffer[tau];
                    tauEstimate = tau;
                }
            }
            if (minVal > 0.40f) // Not periodic enough
            {
                cachedResult.isActive.store(false, std::memory_order_relaxed);
                return;
            }
        }

        // Step 4: Parabolic Interpolation for sub-sample accuracy
        float betterTau = static_cast<float>(tauEstimate);
        if (tauEstimate > 0 && tauEstimate < clampedMaxLag)
        {
            float s0 = runningSumBuffer[tauEstimate - 1];
            float s1 = runningSumBuffer[tauEstimate];
            float s2 = runningSumBuffer[tauEstimate + 1];
            float denominator = (s2 - 2.0f * s1 + s0);
            if (std::abs(denominator) > 0.00001f)
            {
                float delta = (s0 - s2) / (2.0f * denominator);
                betterTau += delta;
            }
        }

        if (betterTau <= 0.0f)
        {
            cachedResult.isActive.store(false, std::memory_order_relaxed);
            return;
        }

        float detectedFreq = static_cast<float>(sampleRate) / betterTau;
        if (detectedFreq < 30.0f || detectedFreq > 1400.0f)
        {
            cachedResult.isActive.store(false, std::memory_order_relaxed);
            return;
        }

        // Calculate Musical Note & Cents
        // A4 = 440 Hz = MIDI note 69
        float midiNoteExact = 69.0f + 12.0f * std::log2(detectedFreq / 440.0f);
        int nearestMidiNote = static_cast<int>(std::round(midiNoteExact));
        float cents = (midiNoteExact - static_cast<float>(nearestMidiNote)) * 100.0f;

        float confidence = std::max(0.0f, std::min(1.0f, 1.0f - runningSumBuffer[tauEstimate]));

        cachedResult.frequency.store(detectedFreq, std::memory_order_relaxed);
        cachedResult.noteNumber.store(nearestMidiNote, std::memory_order_relaxed);
        cachedResult.centsOffset.store(cents, std::memory_order_relaxed);
        cachedResult.confidence.store(confidence, std::memory_order_relaxed);
        cachedResult.isActive.store(true, std::memory_order_relaxed);
    }

    TunerResult getCurrentResult() const
    {
        TunerResult res;
        res.frequency = cachedResult.frequency.load(std::memory_order_relaxed);
        res.noteNumber = cachedResult.noteNumber.load(std::memory_order_relaxed);
        res.centsOffset = cachedResult.centsOffset.load(std::memory_order_relaxed);
        res.confidence = cachedResult.confidence.load(std::memory_order_relaxed);
        res.isActive = cachedResult.isActive.load(std::memory_order_relaxed);
        return res;
    }

    static std::string getNoteName(int midiNote)
    {
        if (midiNote < 0 || midiNote > 127)
            return "--";

        static const char* const noteNames[] = {
            "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
        };
        int noteIndex = midiNote % 12;
        int octave = (midiNote / 12) - 1;
        return std::string(noteNames[noteIndex]) + std::to_string(octave);
    }

private:
    double sampleRate;
    std::array<float, ANALYSIS_BUFFER_SIZE> analysisBuffer;
    int writeIndex;
    std::atomic<bool> enabled;

    std::vector<float> diffBuffer;
    std::vector<float> runningSumBuffer;

    struct
    {
        std::atomic<float> frequency;
        std::atomic<int> noteNumber;
        std::atomic<float> centsOffset;
        std::atomic<float> confidence;
        std::atomic<bool> isActive;
    } cachedResult;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Tuner)
};

} // namespace paasphoos
