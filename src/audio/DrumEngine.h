#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <vector>
#include <array>
#include <atomic>
#include <cmath>
#include <random>

namespace paasphoos
{

enum class DrumSound : int
{
    KICK = 0,
    SNARE,
    HIHAT_CLOSED,
    HIHAT_OPEN,
    TOM_LOW,
    TOM_HIGH,
    CLAP,
    PERCUSSION_DHOLAK,
    NUM_DRUM_SOUNDS
};

enum class DrumKitPreset
{
    ACOUSTIC_ROCK,
    HEAVY_METAL,
    POP_STUDIO,
    JAZZ_BRUSH,
    LOFI_CHILL,
    VINTAGE_808,
    ELECTRONIC_909,
    SYNTHWAVE,
    LATIN_PERCUSSION,
    BOLLYWOOD_DHOLAK
};

struct DrumVoiceParams
{
    std::atomic<float> tuning{1.0f};  // 0.5 to 2.0
    std::atomic<float> decay{1.0f};   // 0.2 to 3.0
    std::atomic<float> volume{1.0f};  // 0.0 to 1.5
    std::atomic<float> pan{0.0f};     // -1.0 to 1.0
};

class DrumEngine
{
public:
    static constexpr int NUM_VOICES = static_cast<int>(DrumSound::NUM_DRUM_SOUNDS);

    DrumEngine();
    ~DrumEngine() = default;

    void prepare(double sampleRate);
    void reset();

    void triggerVoice(DrumSound sound, float velocity = 1.0f);
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples);

    void loadKitPreset(DrumKitPreset preset);
    DrumVoiceParams& getVoiceParams(DrumSound sound) { return voiceParams[static_cast<int>(sound)]; }

    // User WAV sample loading
    bool loadCustomSample(DrumSound sound, const juce::File& sampleFile);
    void clearCustomSample(DrumSound sound);

    static juce::String getSoundName(DrumSound sound);

private:
    void renderKick(float* outL, float* outR, int numSamples);
    void renderSnare(float* outL, float* outR, int numSamples);
    void renderHihatClosed(float* outL, float* outR, int numSamples);
    void renderHihatOpen(float* outL, float* outR, int numSamples);
    void renderTomLow(float* outL, float* outR, int numSamples);
    void renderTomHigh(float* outL, float* outR, int numSamples);
    void renderClap(float* outL, float* outR, int numSamples);
    void renderDholak(float* outL, float* outR, int numSamples);

    double currentSampleRate{48000.0};
    std::array<DrumVoiceParams, NUM_VOICES> voiceParams;

    // Active voice state
    struct VoiceState
    {
        std::atomic<bool> active{false};
        float velocity{0.0f};
        float phase{0.0f};
        float envelope{0.0f};
        int sampleIndex{0};
    };
    std::array<VoiceState, NUM_VOICES> voiceStates;

    // Optional user WAV samples
    std::array<juce::AudioBuffer<float>, NUM_VOICES> customSamples;
    std::array<bool, NUM_VOICES> hasCustomSample;

    // Fast noise generator
    std::mt19937 noiseRng{1337};
    std::uniform_real_distribution<float> noiseDist{-1.0f, 1.0f};

    juce::AudioFormatManager formatManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumEngine)
};

} // namespace paasphoos
