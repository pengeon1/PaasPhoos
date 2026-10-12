#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <cmath>
#include <algorithm>

namespace paasphoos
{

enum class FxPreset
{
    CUSTOM,
    CLEAN_ACOUSTIC,
    CRUNCH_RHYTHM,
    HIGH_GAIN_LEAD,
    VOCAL_WARM,
    VOCAL_BRIGHT_HALL
};

class BuiltInFxChain
{
public:
    BuiltInFxChain();
    ~BuiltInFxChain() = default;

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    void process(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);

    // Bypass & Dry/Wet Send
    void setEnabled(bool enabled) noexcept { isEnabled.store(enabled, std::memory_order_relaxed); }
    bool getEnabled() const noexcept { return isEnabled.load(std::memory_order_relaxed); }

    void setSendWetOnly(bool wetOnly) noexcept { sendWetOnly.store(wetOnly, std::memory_order_relaxed); }
    bool getSendWetOnly() const noexcept { return sendWetOnly.load(std::memory_order_relaxed); }

    // Noise Gate
    void setGateEnabled(bool enabled) noexcept { gateEnabled.store(enabled, std::memory_order_relaxed); }
    bool getGateEnabled() const noexcept { return gateEnabled.load(std::memory_order_relaxed); }
    void setGateThreshold(float thresholdDb) noexcept { gateThresholdDb.store(thresholdDb, std::memory_order_relaxed); }
    float getGateThreshold() const noexcept { return gateThresholdDb.load(std::memory_order_relaxed); }

    // Compressor
    void setCompEnabled(bool enabled) noexcept { compEnabled.store(enabled, std::memory_order_relaxed); }
    bool getCompEnabled() const noexcept { return compEnabled.load(std::memory_order_relaxed); }
    void setCompThreshold(float thresholdDb) noexcept { compThresholdDb.store(thresholdDb, std::memory_order_relaxed); }
    float getCompThreshold() const noexcept { return compThresholdDb.load(std::memory_order_relaxed); }
    void setCompRatio(float ratio) noexcept { compRatio.store(ratio, std::memory_order_relaxed); }
    float getCompRatio() const noexcept { return compRatio.load(std::memory_order_relaxed); }
    void setCompMakeup(float makeupDb) noexcept { compMakeupDb.store(makeupDb, std::memory_order_relaxed); }
    float getCompMakeup() const noexcept { return compMakeupDb.load(std::memory_order_relaxed); }

    // Overdrive / Amp Drive
    void setDriveEnabled(bool enabled) noexcept { driveEnabled.store(enabled, std::memory_order_relaxed); }
    bool getDriveEnabled() const noexcept { return driveEnabled.load(std::memory_order_relaxed); }
    void setDriveAmount(float amount) noexcept { driveAmount.store(amount, std::memory_order_relaxed); } // 0.0 to 10.0
    float getDriveAmount() const noexcept { return driveAmount.load(std::memory_order_relaxed); }
    void setDriveTone(float tone) noexcept { driveTone.store(tone, std::memory_order_relaxed); } // 0.0 to 1.0
    float getDriveTone() const noexcept { return driveTone.load(std::memory_order_relaxed); }

    // 4-Band Parametric EQ
    void setEqEnabled(bool enabled) noexcept { eqEnabled.store(enabled, std::memory_order_relaxed); }
    bool getEqEnabled() const noexcept { return eqEnabled.load(std::memory_order_relaxed); }
    void setEqLowGain(float gainDb) noexcept { eqLowDb.store(gainDb, std::memory_order_relaxed); updateFilters(); }
    void setEqMid1Gain(float gainDb) noexcept { eqMid1Db.store(gainDb, std::memory_order_relaxed); updateFilters(); }
    void setEqMid2Gain(float gainDb) noexcept { eqMid2Db.store(gainDb, std::memory_order_relaxed); updateFilters(); }
    void setEqHighGain(float gainDb) noexcept { eqHighDb.store(gainDb, std::memory_order_relaxed); updateFilters(); }
    float getEqLowGain() const noexcept { return eqLowDb.load(std::memory_order_relaxed); }
    float getEqMid1Gain() const noexcept { return eqMid1Db.load(std::memory_order_relaxed); }
    float getEqMid2Gain() const noexcept { return eqMid2Db.load(std::memory_order_relaxed); }
    float getEqHighGain() const noexcept { return eqHighDb.load(std::memory_order_relaxed); }

    // Reverb
    void setReverbEnabled(bool enabled) noexcept { reverbEnabled.store(enabled, std::memory_order_relaxed); }
    bool getReverbEnabled() const noexcept { return reverbEnabled.load(std::memory_order_relaxed); }
    void setReverbRoomSize(float size) noexcept;
    void setReverbDamping(float damping) noexcept;
    void setReverbWet(float wet) noexcept;
    float getReverbWet() const noexcept { return reverbWet.load(std::memory_order_relaxed); }

    // Presets
    void loadPreset(FxPreset preset);

private:
    void updateFilters();

    double currentSampleRate{48000.0};
    int maxBlockSize{512};

    std::atomic<bool> isEnabled{true};
    std::atomic<bool> sendWetOnly{true};

    // Gate parameters & state
    std::atomic<bool> gateEnabled{true};
    std::atomic<float> gateThresholdDb{-48.0f};
    float gateEnvelope{0.0f};
    float gateGain{1.0f};

    // Compressor parameters & state
    std::atomic<bool> compEnabled{true};
    std::atomic<float> compThresholdDb{-18.0f};
    std::atomic<float> compRatio{3.0f};
    std::atomic<float> compMakeupDb{3.0f};
    float compEnvelope{0.0f};

    // Drive parameters
    std::atomic<bool> driveEnabled{false};
    std::atomic<float> driveAmount{3.5f};
    std::atomic<float> driveTone{0.6f};
    float driveFilterState0{0.0f};
    float driveFilterState1{0.0f};

    // EQ parameters & filters
    std::atomic<bool> eqEnabled{true};
    std::atomic<float> eqLowDb{0.0f};
    std::atomic<float> eqMid1Db{0.0f};
    std::atomic<float> eqMid2Db{0.0f};
    std::atomic<float> eqHighDb{0.0f};

    juce::dsp::IIR::Filter<float> lowFilter[2];
    juce::dsp::IIR::Filter<float> mid1Filter[2];
    juce::dsp::IIR::Filter<float> mid2Filter[2];
    juce::dsp::IIR::Filter<float> highFilter[2];

    // Reverb
    std::atomic<bool> reverbEnabled{true};
    std::atomic<float> reverbWet{0.25f};
    juce::dsp::Reverb reverb;
    juce::dsp::Reverb::Parameters reverbParams;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BuiltInFxChain)
};

} // namespace paasphoos
