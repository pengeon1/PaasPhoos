#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <cmath>

namespace paasphoos
{

class Metronome
{
public:
    Metronome()
        : sampleRate(48000.0),
          bpm(120.0f),
          volume(0.7f),
          timeSignatureNumerator(4),
          timeSignatureDenominator(4),
          enabled(false),
          currentSampleInBeat(0),
          currentBeat(0),
          clickPhase(0.0),
          clickSampleRemaining(0),
          isAccented(false)
    {
    }

    void prepareToPlay(double newSampleRate)
    {
        sampleRate = (newSampleRate > 1000.0) ? newSampleRate : 48000.0;
        reset();
    }

    void reset()
    {
        currentSampleInBeat = 0;
        currentBeat.store(0, std::memory_order_relaxed);
        clickSampleRemaining = 0;
        clickPhase = 0.0;
    }

    void setBpm(float newBpm) noexcept
    {
        bpm.store(juce::jlimit(20.0f, 320.0f, newBpm), std::memory_order_relaxed);
    }

    float getBpm() const noexcept
    {
        return bpm.load(std::memory_order_relaxed);
    }

    void setVolume(float newVolume) noexcept
    {
        volume.store(juce::jlimit(0.0f, 1.0f, newVolume), std::memory_order_relaxed);
    }

    float getVolume() const noexcept
    {
        return volume.load(std::memory_order_relaxed);
    }

    void setEnabled(bool shouldBeEnabled) noexcept
    {
        if (shouldBeEnabled && !enabled.load(std::memory_order_relaxed))
            reset();
        enabled.store(shouldBeEnabled, std::memory_order_relaxed);
    }

    bool isEnabled() const noexcept
    {
        return enabled.load(std::memory_order_relaxed);
    }

    void setTimeSignature(int numerator, int denominator) noexcept
    {
        timeSignatureNumerator.store(juce::jlimit(1, 16, numerator), std::memory_order_relaxed);
        timeSignatureDenominator.store(juce::jlimit(1, 16, denominator), std::memory_order_relaxed);
    }

    int getTimeSignatureNumerator() const noexcept
    {
        return timeSignatureNumerator.load(std::memory_order_relaxed);
    }

    int getCurrentBeat() const noexcept
    {
        return currentBeat.load(std::memory_order_relaxed);
    }

    /**
     * Render metronome click into output buffer.
     * Guaranteed real-time safe (no allocation, no lock).
     */
    void process(juce::AudioBuffer<float>& buffer, int startSample, int numSamples) noexcept
    {
        if (!enabled.load(std::memory_order_relaxed))
            return;

        const float vol = volume.load(std::memory_order_relaxed);
        if (vol <= 0.0001f)
            return;

        const float currentBpm = bpm.load(std::memory_order_relaxed);
        const int numBeats = timeSignatureNumerator.load(std::memory_order_relaxed);
        const double samplesPerBeat = (sampleRate * 60.0) / static_cast<double>(currentBpm);
        const int samplesPerClick = static_cast<int>(sampleRate * 0.015); // 15 ms click burst

        float* const* channelData = buffer.getArrayOfWritePointers();
        const int numChannels = buffer.getNumChannels();

        for (int i = 0; i < numSamples; ++i)
        {
            const int sampleIndex = startSample + i;

            // Check if we hit the beat boundary
            if (currentSampleInBeat >= static_cast<int64_t>(samplesPerBeat))
            {
                currentSampleInBeat = 0;
                int nextBeat = (currentBeat.load(std::memory_order_relaxed) + 1) % numBeats;
                currentBeat.store(nextBeat, std::memory_order_relaxed);

                isAccented = (nextBeat == 0);
                clickSampleRemaining = samplesPerClick;
                clickPhase = 0.0;
            }

            float clickSample = 0.0f;
            if (clickSampleRemaining > 0)
            {
                const double clickFreq = isAccented ? 1760.0 : 880.0; // A6 for beat 1, A5 for other beats
                const double phaseDelta = (juce::MathConstants<double>::twoPi * clickFreq) / sampleRate;

                // Exponential decay envelope
                const float env = static_cast<float>(clickSampleRemaining) / static_cast<float>(samplesPerClick);
                const float curvedEnv = env * env * env;

                clickSample = static_cast<float>(std::sin(clickPhase)) * curvedEnv * vol;

                clickPhase += phaseDelta;
                if (clickPhase >= juce::MathConstants<double>::twoPi)
                    clickPhase -= juce::MathConstants<double>::twoPi;

                --clickSampleRemaining;
            }

            if (std::abs(clickSample) > 0.00001f)
            {
                for (int ch = 0; ch < numChannels; ++ch)
                {
                    channelData[ch][sampleIndex] += clickSample;
                }
            }

            ++currentSampleInBeat;
        }
    }

private:
    double sampleRate;
    std::atomic<float> bpm;
    std::atomic<float> volume;
    std::atomic<int> timeSignatureNumerator;
    std::atomic<int> timeSignatureDenominator;
    std::atomic<bool> enabled;

    int64_t currentSampleInBeat;
    std::atomic<int> currentBeat;

    double clickPhase;
    int clickSampleRemaining;
    bool isAccented;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Metronome)
};

} // namespace paasphoos
