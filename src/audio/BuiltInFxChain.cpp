#include "BuiltInFxChain.h"

namespace paasphoos
{

BuiltInFxChain::BuiltInFxChain()
{
    reverbParams.roomSize = 0.5f;
    reverbParams.damping = 0.4f;
    reverbParams.wetLevel = 0.25f;
    reverbParams.dryLevel = 1.0f;
    reverbParams.width = 1.0f;
    reverbParams.freezeMode = 0.0f;
    reverb.setParameters(reverbParams);
}

void BuiltInFxChain::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = (sampleRate > 1000.0) ? sampleRate : 48000.0;
    maxBlockSize = std::max(64, samplesPerBlock);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = currentSampleRate;
    spec.maximumBlockSize = static_cast<uint32_t>(maxBlockSize);
    spec.numChannels = 2;

    reverb.prepare(spec);
    reverb.setParameters(reverbParams);

    for (int ch = 0; ch < 2; ++ch)
    {
        lowFilter[ch].reset();
        mid1Filter[ch].reset();
        mid2Filter[ch].reset();
        highFilter[ch].reset();
    }

    updateFilters();
    reset();
}

void BuiltInFxChain::reset()
{
    reverb.reset();
    for (int ch = 0; ch < 2; ++ch)
    {
        lowFilter[ch].reset();
        mid1Filter[ch].reset();
        mid2Filter[ch].reset();
        highFilter[ch].reset();
    }
    gateEnvelope = 0.0f;
    gateGain = 1.0f;
    compEnvelope = 0.0f;
    driveFilterState0 = 0.0f;
    driveFilterState1 = 0.0f;
}

void BuiltInFxChain::updateFilters()
{
    float lowGainLinear = std::pow(10.0f, eqLowDb.load(std::memory_order_relaxed) / 20.0f);
    float mid1GainLinear = std::pow(10.0f, eqMid1Db.load(std::memory_order_relaxed) / 20.0f);
    float mid2GainLinear = std::pow(10.0f, eqMid2Db.load(std::memory_order_relaxed) / 20.0f);
    float highGainLinear = std::pow(10.0f, eqHighDb.load(std::memory_order_relaxed) / 20.0f);

    auto lowCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(currentSampleRate, 120.0f, 0.707f, lowGainLinear);
    auto mid1Coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(currentSampleRate, 600.0f, 1.0f, mid1GainLinear);
    auto mid2Coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(currentSampleRate, 2800.0f, 1.2f, mid2GainLinear);
    auto highCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(currentSampleRate, 8000.0f, 0.707f, highGainLinear);

    for (int ch = 0; ch < 2; ++ch)
    {
        *lowFilter[ch].coefficients = *lowCoeffs;
        *mid1Filter[ch].coefficients = *mid1Coeffs;
        *mid2Filter[ch].coefficients = *mid2Coeffs;
        *highFilter[ch].coefficients = *highCoeffs;
    }
}

void BuiltInFxChain::setReverbRoomSize(float size) noexcept
{
    reverbParams.roomSize = juce::jlimit(0.0f, 1.0f, size);
    reverb.setParameters(reverbParams);
}

void BuiltInFxChain::setReverbDamping(float damping) noexcept
{
    reverbParams.damping = juce::jlimit(0.0f, 1.0f, damping);
    reverb.setParameters(reverbParams);
}

void BuiltInFxChain::setReverbWet(float wet) noexcept
{
    float clamped = juce::jlimit(0.0f, 1.0f, wet);
    reverbWet.store(clamped, std::memory_order_relaxed);
    reverbParams.wetLevel = clamped;
    reverb.setParameters(reverbParams);
}

void BuiltInFxChain::loadPreset(FxPreset preset)
{
    switch (preset)
    {
        case FxPreset::CLEAN_ACOUSTIC:
            gateEnabled.store(true); gateThresholdDb.store(-52.0f);
            compEnabled.store(true); compThresholdDb.store(-16.0f); compRatio.store(2.5f); compMakeupDb.store(2.0f);
            driveEnabled.store(false);
            eqEnabled.store(true); eqLowDb.store(1.5f); eqMid1Db.store(-1.5f); eqMid2Db.store(1.0f); eqHighDb.store(3.0f);
            reverbEnabled.store(true); setReverbRoomSize(0.35f); setReverbDamping(0.3f); setReverbWet(0.20f);
            break;

        case FxPreset::CRUNCH_RHYTHM:
            gateEnabled.store(true); gateThresholdDb.store(-46.0f);
            compEnabled.store(true); compThresholdDb.store(-18.0f); compRatio.store(3.5f); compMakeupDb.store(3.0f);
            driveEnabled.store(true); driveAmount.store(3.8f); driveTone.store(0.65f);
            eqEnabled.store(true); eqLowDb.store(2.0f); eqMid1Db.store(1.0f); eqMid2Db.store(2.5f); eqHighDb.store(1.0f);
            reverbEnabled.store(true); setReverbRoomSize(0.4f); setReverbDamping(0.5f); setReverbWet(0.18f);
            break;

        case FxPreset::HIGH_GAIN_LEAD:
            gateEnabled.store(true); gateThresholdDb.store(-40.0f);
            compEnabled.store(true); compThresholdDb.store(-22.0f); compRatio.store(5.0f); compMakeupDb.store(4.5f);
            driveEnabled.store(true); driveAmount.store(7.5f); driveTone.store(0.75f);
            eqEnabled.store(true); eqLowDb.store(1.0f); eqMid1Db.store(3.0f); eqMid2Db.store(2.0f); eqHighDb.store(2.5f);
            reverbEnabled.store(true); setReverbRoomSize(0.65f); setReverbDamping(0.35f); setReverbWet(0.32f);
            break;

        case FxPreset::VOCAL_WARM:
            gateEnabled.store(true); gateThresholdDb.store(-48.0f);
            compEnabled.store(true); compThresholdDb.store(-16.0f); compRatio.store(3.0f); compMakeupDb.store(3.5f);
            driveEnabled.store(false);
            eqEnabled.store(true); eqLowDb.store(-2.0f); eqMid1Db.store(1.5f); eqMid2Db.store(2.0f); eqHighDb.store(2.0f);
            reverbEnabled.store(true); setReverbRoomSize(0.45f); setReverbDamping(0.4f); setReverbWet(0.22f);
            break;

        case FxPreset::VOCAL_BRIGHT_HALL:
            gateEnabled.store(true); gateThresholdDb.store(-45.0f);
            compEnabled.store(true); compThresholdDb.store(-14.0f); compRatio.store(2.8f); compMakeupDb.store(3.0f);
            driveEnabled.store(false);
            eqEnabled.store(true); eqLowDb.store(-3.0f); eqMid1Db.store(0.0f); eqMid2Db.store(2.5f); eqHighDb.store(4.0f);
            reverbEnabled.store(true); setReverbRoomSize(0.75f); setReverbDamping(0.25f); setReverbWet(0.38f);
            break;

        case FxPreset::CUSTOM:
        default:
            break;
    }
    updateFilters();
}

void BuiltInFxChain::process(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    if (!isEnabled.load(std::memory_order_relaxed) || numSamples <= 0)
        return;

    const int channels = std::min(2, buffer.getNumChannels());
    float* ch0 = buffer.getWritePointer(0, startSample);
    float* ch1 = (channels > 1) ? buffer.getWritePointer(1, startSample) : ch0;

    // 1. Noise Gate
    if (gateEnabled.load(std::memory_order_relaxed))
    {
        float threshDb = gateThresholdDb.load(std::memory_order_relaxed);
        float threshLinear = std::pow(10.0f, threshDb / 20.0f);
        const float attackCoeff = static_cast<float>(std::exp(-1.0 / (0.002 * currentSampleRate))); // 2 ms attack
        const float releaseCoeff = static_cast<float>(std::exp(-1.0 / (0.060 * currentSampleRate))); // 60 ms release

        for (int i = 0; i < numSamples; ++i)
        {
            float peak = std::max(std::abs(ch0[i]), std::abs(ch1[i]));
            if (peak > gateEnvelope)
                gateEnvelope = attackCoeff * gateEnvelope + (1.0f - attackCoeff) * peak;
            else
                gateEnvelope = releaseCoeff * gateEnvelope + (1.0f - releaseCoeff) * peak;

            float targetGain = (gateEnvelope >= threshLinear) ? 1.0f : 0.0f;
            gateGain = 0.992f * gateGain + 0.008f * targetGain;

            ch0[i] *= gateGain;
            if (channels > 1) ch1[i] *= gateGain;
        }
    }

    // 2. Overdrive / Saturation (Tube Style Soft-Clipper)
    if (driveEnabled.load(std::memory_order_relaxed))
    {
        float drive = 1.0f + driveAmount.load(std::memory_order_relaxed) * 1.8f;
        float tone = driveTone.load(std::memory_order_relaxed);
        float filterCoeff = juce::jlimit(0.1f, 0.9f, 0.3f + tone * 0.6f);

        for (int i = 0; i < numSamples; ++i)
        {
            // Soft clipping non-linear saturation: x / (1 + |x|)
            float in0 = ch0[i] * drive;
            float sat0 = in0 / (1.0f + std::abs(in0) * 0.7f);
            driveFilterState0 += filterCoeff * (sat0 - driveFilterState0);
            ch0[i] = driveFilterState0 * (1.2f / std::sqrt(drive));

            if (channels > 1)
            {
                float in1 = ch1[i] * drive;
                float sat1 = in1 / (1.0f + std::abs(in1) * 0.7f);
                driveFilterState1 += filterCoeff * (sat1 - driveFilterState1);
                ch1[i] = driveFilterState1 * (1.2f / std::sqrt(drive));
            }
        }
    }

    // 3. Compressor
    if (compEnabled.load(std::memory_order_relaxed))
    {
        float threshDb = compThresholdDb.load(std::memory_order_relaxed);
        float ratio = compRatio.load(std::memory_order_relaxed);
        float makeupLinear = std::pow(10.0f, compMakeupDb.load(std::memory_order_relaxed) / 20.0f);
        const float att = static_cast<float>(std::exp(-1.0 / (0.010 * currentSampleRate))); // 10 ms attack
        const float rel = static_cast<float>(std::exp(-1.0 / (0.120 * currentSampleRate))); // 120 ms release

        for (int i = 0; i < numSamples; ++i)
        {
            float peak = std::max(std::abs(ch0[i]), std::abs(ch1[i]));
            if (peak > compEnvelope)
                compEnvelope = att * compEnvelope + (1.0f - att) * peak;
            else
                compEnvelope = rel * compEnvelope + (1.0f - rel) * peak;

            float envDb = (compEnvelope > 0.00001f) ? 20.0f * std::log10(compEnvelope) : -100.0f;
            float gainReductionDb = 0.0f;
            if (envDb > threshDb)
            {
                gainReductionDb = (envDb - threshDb) * (1.0f - 1.0f / ratio);
            }
            float compGain = std::pow(10.0f, -gainReductionDb / 20.0f) * makeupLinear;

            ch0[i] *= compGain;
            if (channels > 1) ch1[i] *= compGain;
        }
    }

    // 4. 4-Band Parametric EQ
    if (eqEnabled.load(std::memory_order_relaxed))
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float s0 = lowFilter[0].processSample(ch0[i]);
            s0 = mid1Filter[0].processSample(s0);
            s0 = mid2Filter[0].processSample(s0);
            ch0[i] = highFilter[0].processSample(s0);

            if (channels > 1)
            {
                float s1 = lowFilter[1].processSample(ch1[i]);
                s1 = mid1Filter[1].processSample(s1);
                s1 = mid2Filter[1].processSample(s1);
                ch1[i] = highFilter[1].processSample(s1);
            }
        }
    }

    // 5. Reverb
    if (reverbEnabled.load(std::memory_order_relaxed) && reverbWet.load(std::memory_order_relaxed) > 0.01f)
    {
        juce::dsp::AudioBlock<float> block(buffer);
        auto subBlock = block.getSubBlock(static_cast<size_t>(startSample), static_cast<size_t>(numSamples));
        juce::dsp::ProcessContextReplacing<float> context(subBlock);
        reverb.process(context);
    }
}

} // namespace paasphoos
