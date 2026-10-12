#include "DrumEngine.h"

namespace paasphoos
{

DrumEngine::DrumEngine()
{
    formatManager.registerBasicFormats();
    for (int i = 0; i < NUM_VOICES; ++i)
    {
        hasCustomSample[i] = false;
        voiceStates[i].active.store(false);
    }
    loadKitPreset(DrumKitPreset::ACOUSTIC_ROCK);
}

void DrumEngine::prepare(double sampleRate)
{
    currentSampleRate = (sampleRate > 1000.0) ? sampleRate : 48000.0;
    reset();
}

void DrumEngine::reset()
{
    for (int i = 0; i < NUM_VOICES; ++i)
    {
        voiceStates[i].active.store(false);
        voiceStates[i].phase = 0.0f;
        voiceStates[i].envelope = 0.0f;
        voiceStates[i].sampleIndex = 0;
    }
}

juce::String DrumEngine::getSoundName(DrumSound sound)
{
    switch (sound)
    {
        case DrumSound::KICK: return "KICK";
        case DrumSound::SNARE: return "SNARE";
        case DrumSound::HIHAT_CLOSED: return "HH CLOSED";
        case DrumSound::HIHAT_OPEN: return "HH OPEN";
        case DrumSound::TOM_LOW: return "LOW TOM";
        case DrumSound::TOM_HIGH: return "HIGH TOM";
        case DrumSound::CLAP: return "CLAP";
        case DrumSound::PERCUSSION_DHOLAK: return "DHOLAK/PERC";
        default: return "DRUM";
    }
}

void DrumEngine::triggerVoice(DrumSound sound, float velocity)
{
    int idx = static_cast<int>(sound);
    if (idx >= 0 && idx < NUM_VOICES)
    {
        voiceStates[idx].velocity = juce::jlimit(0.05f, 1.0f, velocity);
        voiceStates[idx].phase = 0.0f;
        voiceStates[idx].envelope = 1.0f;
        voiceStates[idx].sampleIndex = 0;
        voiceStates[idx].active.store(true, std::memory_order_relaxed);
    }
}

bool DrumEngine::loadCustomSample(DrumSound sound, const juce::File& sampleFile)
{
    int idx = static_cast<int>(sound);
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(sampleFile));
    if (reader != nullptr)
    {
        customSamples[idx].setSize(2, static_cast<int>(reader->lengthInSamples));
        reader->read(&customSamples[idx], 0, static_cast<int>(reader->lengthInSamples), 0, true, true);
        hasCustomSample[idx] = true;
        return true;
    }
    return false;
}

void DrumEngine::clearCustomSample(DrumSound sound)
{
    int idx = static_cast<int>(sound);
    hasCustomSample[idx] = false;
    customSamples[idx].setSize(0, 0);
}

void DrumEngine::loadKitPreset(DrumKitPreset preset)
{
    for (int i = 0; i < NUM_VOICES; ++i)
    {
        voiceParams[i].tuning.store(1.0f);
        voiceParams[i].decay.store(1.0f);
        voiceParams[i].volume.store(1.0f);
        voiceParams[i].pan.store(0.0f);
    }

    switch (preset)
    {
        case DrumKitPreset::ACOUSTIC_ROCK:
            voiceParams[(int)DrumSound::KICK].tuning.store(0.95f);
            voiceParams[(int)DrumSound::SNARE].tuning.store(1.05f);
            voiceParams[(int)DrumSound::TOM_LOW].pan.store(-0.35f);
            voiceParams[(int)DrumSound::TOM_HIGH].pan.store(0.35f);
            voiceParams[(int)DrumSound::HIHAT_CLOSED].pan.store(-0.25f);
            voiceParams[(int)DrumSound::HIHAT_OPEN].pan.store(-0.30f);
            break;

        case DrumKitPreset::HEAVY_METAL:
            voiceParams[(int)DrumSound::KICK].tuning.store(0.85f);
            voiceParams[(int)DrumSound::KICK].volume.store(1.25f);
            voiceParams[(int)DrumSound::SNARE].tuning.store(1.20f);
            voiceParams[(int)DrumSound::SNARE].decay.store(0.9f);
            break;

        case DrumKitPreset::VINTAGE_808:
            voiceParams[(int)DrumSound::KICK].tuning.store(0.80f);
            voiceParams[(int)DrumSound::KICK].decay.store(2.40f); // Long 808 sub bass boom
            voiceParams[(int)DrumSound::SNARE].tuning.store(0.95f);
            voiceParams[(int)DrumSound::CLAP].volume.store(1.20f);
            break;

        case DrumKitPreset::ELECTRONIC_909:
            voiceParams[(int)DrumSound::KICK].tuning.store(1.05f);
            voiceParams[(int)DrumSound::KICK].decay.store(1.2f);
            voiceParams[(int)DrumSound::SNARE].tuning.store(1.10f);
            voiceParams[(int)DrumSound::HIHAT_CLOSED].decay.store(0.7f);
            break;

        case DrumKitPreset::BOLLYWOOD_DHOLAK:
            voiceParams[(int)DrumSound::PERCUSSION_DHOLAK].volume.store(1.35f);
            voiceParams[(int)DrumSound::PERCUSSION_DHOLAK].tuning.store(1.15f);
            voiceParams[(int)DrumSound::KICK].decay.store(1.3f);
            break;

        default:
            break;
    }
}

void DrumEngine::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (numSamples <= 0) return;

    float* outL = outputBuffer.getWritePointer(0, startSample);
    float* outR = (outputBuffer.getNumChannels() > 1) ? outputBuffer.getWritePointer(1, startSample) : outL;

    renderKick(outL, outR, numSamples);
    renderSnare(outL, outR, numSamples);
    renderHihatClosed(outL, outR, numSamples);
    renderHihatOpen(outL, outR, numSamples);
    renderTomLow(outL, outR, numSamples);
    renderTomHigh(outL, outR, numSamples);
    renderClap(outL, outR, numSamples);
    renderDholak(outL, outR, numSamples);
}

void DrumEngine::renderKick(float* outL, float* outR, int numSamples)
{
    auto& state = voiceStates[(int)DrumSound::KICK];
    if (!state.active.load(std::memory_order_relaxed)) return;

    auto& p = voiceParams[(int)DrumSound::KICK];
    float tuning = p.tuning.load(std::memory_order_relaxed);
    float decay = p.decay.load(std::memory_order_relaxed);
    float vol = p.volume.load(std::memory_order_relaxed) * state.velocity;
    float pan = p.pan.load(std::memory_order_relaxed);

    float panL = std::sqrt(juce::jlimit(0.0f, 1.0f, (1.0f - pan) * 0.5f));
    float panR = std::sqrt(juce::jlimit(0.0f, 1.0f, (1.0f + pan) * 0.5f));

    float decayCoeff = static_cast<float>(std::exp(-1.0 / (0.160 * decay * currentSampleRate)));

    for (int i = 0; i < numSamples; ++i)
    {
        // Pitch envelope: drops rapidly from 160 Hz to 48 Hz
        float freq = (48.0f + 115.0f * state.envelope * state.envelope) * tuning;
        state.phase += static_cast<float>(2.0 * 3.14159265358979323846 * freq / currentSampleRate);
        if (state.phase >= 6.2831853f) state.phase -= 6.2831853f;

        // Sine body + transient click
        float click = (state.sampleIndex < 80) ? (0.6f * (1.0f - state.sampleIndex / 80.0f)) : 0.0f;
        float sample = (std::sin(state.phase) + click) * state.envelope * vol;

        outL[i] += sample * panL;
        outR[i] += sample * panR;

        state.envelope *= decayCoeff;
        state.sampleIndex++;

        if (state.envelope < 0.001f)
        {
            state.active.store(false, std::memory_order_relaxed);
            break;
        }
    }
}

void DrumEngine::renderSnare(float* outL, float* outR, int numSamples)
{
    auto& state = voiceStates[(int)DrumSound::SNARE];
    if (!state.active.load(std::memory_order_relaxed)) return;

    auto& p = voiceParams[(int)DrumSound::SNARE];
    float tuning = p.tuning.load(std::memory_order_relaxed);
    float decay = p.decay.load(std::memory_order_relaxed);
    float vol = p.volume.load(std::memory_order_relaxed) * state.velocity;
    float pan = p.pan.load(std::memory_order_relaxed);

    float panL = std::sqrt(juce::jlimit(0.0f, 1.0f, (1.0f - pan) * 0.5f));
    float panR = std::sqrt(juce::jlimit(0.0f, 1.0f, (1.0f + pan) * 0.5f));

    float decayCoeff = static_cast<float>(std::exp(-1.0 / (0.140 * decay * currentSampleRate)));

    for (int i = 0; i < numSamples; ++i)
    {
        float f1 = 185.0f * tuning;
        float f2 = 330.0f * tuning;
        state.phase += static_cast<float>(2.0 * 3.14159265358979323846 * f1 / currentSampleRate);
        if (state.phase >= 6.2831853f) state.phase -= 6.2831853f;

        float body = (std::sin(state.phase) + 0.4f * std::sin(state.phase * 1.78f)) * 0.5f;
        float noise = noiseDist(noiseRng) * 0.85f;
        float sample = (body * 0.45f + noise * 0.55f) * state.envelope * vol;

        outL[i] += sample * panL;
        outR[i] += sample * panR;

        state.envelope *= decayCoeff;
        state.sampleIndex++;

        if (state.envelope < 0.001f)
        {
            state.active.store(false, std::memory_order_relaxed);
            break;
        }
    }
}

void DrumEngine::renderHihatClosed(float* outL, float* outR, int numSamples)
{
    auto& state = voiceStates[(int)DrumSound::HIHAT_CLOSED];
    if (!state.active.load(std::memory_order_relaxed)) return;

    auto& p = voiceParams[(int)DrumSound::HIHAT_CLOSED];
    float decay = p.decay.load(std::memory_order_relaxed);
    float vol = p.volume.load(std::memory_order_relaxed) * state.velocity * 0.7f;
    float pan = p.pan.load(std::memory_order_relaxed);

    float panL = std::sqrt(juce::jlimit(0.0f, 1.0f, (1.0f - pan) * 0.5f));
    float panR = std::sqrt(juce::jlimit(0.0f, 1.0f, (1.0f + pan) * 0.5f));

    float decayCoeff = static_cast<float>(std::exp(-1.0 / (0.045 * decay * currentSampleRate)));

    for (int i = 0; i < numSamples; ++i)
    {
        float noise = noiseDist(noiseRng);
        // High frequency metallic pulse
        state.phase += 0.85f;
        float metallic = std::sin(state.phase) * std::sin(state.phase * 2.37f);
        float sample = (noise * 0.7f + metallic * 0.3f) * state.envelope * vol;

        outL[i] += sample * panL;
        outR[i] += sample * panR;

        state.envelope *= decayCoeff;
        state.sampleIndex++;

        if (state.envelope < 0.001f)
        {
            state.active.store(false, std::memory_order_relaxed);
            break;
        }
    }
}

void DrumEngine::renderHihatOpen(float* outL, float* outR, int numSamples)
{
    auto& state = voiceStates[(int)DrumSound::HIHAT_OPEN];
    if (!state.active.load(std::memory_order_relaxed)) return;

    auto& p = voiceParams[(int)DrumSound::HIHAT_OPEN];
    float decay = p.decay.load(std::memory_order_relaxed);
    float vol = p.volume.load(std::memory_order_relaxed) * state.velocity * 0.65f;
    float pan = p.pan.load(std::memory_order_relaxed);

    float panL = std::sqrt(juce::jlimit(0.0f, 1.0f, (1.0f - pan) * 0.5f));
    float panR = std::sqrt(juce::jlimit(0.0f, 1.0f, (1.0f + pan) * 0.5f));

    float decayCoeff = static_cast<float>(std::exp(-1.0 / (0.320 * decay * currentSampleRate)));

    for (int i = 0; i < numSamples; ++i)
    {
        float noise = noiseDist(noiseRng);
        state.phase += 0.88f;
        float metallic = std::sin(state.phase) * std::sin(state.phase * 2.19f);
        float sample = (noise * 0.65f + metallic * 0.35f) * state.envelope * vol;

        outL[i] += sample * panL;
        outR[i] += sample * panR;

        state.envelope *= decayCoeff;
        state.sampleIndex++;

        if (state.envelope < 0.001f)
        {
            state.active.store(false, std::memory_order_relaxed);
            break;
        }
    }
}

void DrumEngine::renderTomLow(float* outL, float* outR, int numSamples)
{
    auto& state = voiceStates[(int)DrumSound::TOM_LOW];
    if (!state.active.load(std::memory_order_relaxed)) return;

    auto& p = voiceParams[(int)DrumSound::TOM_LOW];
    float tuning = p.tuning.load(std::memory_order_relaxed);
    float decay = p.decay.load(std::memory_order_relaxed);
    float vol = p.volume.load(std::memory_order_relaxed) * state.velocity;
    float pan = p.pan.load(std::memory_order_relaxed);

    float panL = std::sqrt(juce::jlimit(0.0f, 1.0f, (1.0f - pan) * 0.5f));
    float panR = std::sqrt(juce::jlimit(0.0f, 1.0f, (1.0f + pan) * 0.5f));

    float decayCoeff = static_cast<float>(std::exp(-1.0 / (0.240 * decay * currentSampleRate)));

    for (int i = 0; i < numSamples; ++i)
    {
        float freq = (85.0f + 65.0f * state.envelope) * tuning;
        state.phase += static_cast<float>(2.0 * 3.14159265358979323846 * freq / currentSampleRate);
        if (state.phase >= 6.2831853f) state.phase -= 6.2831853f;

        float sample = std::sin(state.phase) * state.envelope * vol;
        outL[i] += sample * panL;
        outR[i] += sample * panR;

        state.envelope *= decayCoeff;
        state.sampleIndex++;

        if (state.envelope < 0.001f)
        {
            state.active.store(false, std::memory_order_relaxed);
            break;
        }
    }
}

void DrumEngine::renderTomHigh(float* outL, float* outR, int numSamples)
{
    auto& state = voiceStates[(int)DrumSound::TOM_HIGH];
    if (!state.active.load(std::memory_order_relaxed)) return;

    auto& p = voiceParams[(int)DrumSound::TOM_HIGH];
    float tuning = p.tuning.load(std::memory_order_relaxed);
    float decay = p.decay.load(std::memory_order_relaxed);
    float vol = p.volume.load(std::memory_order_relaxed) * state.velocity;
    float pan = p.pan.load(std::memory_order_relaxed);

    float panL = std::sqrt(juce::jlimit(0.0f, 1.0f, (1.0f - pan) * 0.5f));
    float panR = std::sqrt(juce::jlimit(0.0f, 1.0f, (1.0f + pan) * 0.5f));

    float decayCoeff = static_cast<float>(std::exp(-1.0 / (0.200 * decay * currentSampleRate)));

    for (int i = 0; i < numSamples; ++i)
    {
        float freq = (140.0f + 95.0f * state.envelope) * tuning;
        state.phase += static_cast<float>(2.0 * 3.14159265358979323846 * freq / currentSampleRate);
        if (state.phase >= 6.2831853f) state.phase -= 6.2831853f;

        float sample = std::sin(state.phase) * state.envelope * vol;
        outL[i] += sample * panL;
        outR[i] += sample * panR;

        state.envelope *= decayCoeff;
        state.sampleIndex++;

        if (state.envelope < 0.001f)
        {
            state.active.store(false, std::memory_order_relaxed);
            break;
        }
    }
}

void DrumEngine::renderClap(float* outL, float* outR, int numSamples)
{
    auto& state = voiceStates[(int)DrumSound::CLAP];
    if (!state.active.load(std::memory_order_relaxed)) return;

    auto& p = voiceParams[(int)DrumSound::CLAP];
    float decay = p.decay.load(std::memory_order_relaxed);
    float vol = p.volume.load(std::memory_order_relaxed) * state.velocity * 0.85f;
    float pan = p.pan.load(std::memory_order_relaxed);

    float panL = std::sqrt(juce::jlimit(0.0f, 1.0f, (1.0f - pan) * 0.5f));
    float panR = std::sqrt(juce::jlimit(0.0f, 1.0f, (1.0f + pan) * 0.5f));

    float decayCoeff = static_cast<float>(std::exp(-1.0 / (0.180 * decay * currentSampleRate)));

    for (int i = 0; i < numSamples; ++i)
    {
        float burst = 0.0f;
        // Staggered micro-claps at t=0, t=11ms, t=22ms
        int s = state.sampleIndex;
        int p1 = static_cast<int>(0.011 * currentSampleRate);
        int p2 = static_cast<int>(0.022 * currentSampleRate);

        if (s < p1 || (s >= p1 && s < p1 + 200) || (s >= p2 && s < p2 + 200) || s >= p2)
        {
            burst = noiseDist(noiseRng);
        }

        float sample = burst * state.envelope * vol;
        outL[i] += sample * panL;
        outR[i] += sample * panR;

        state.envelope *= decayCoeff;
        state.sampleIndex++;

        if (state.envelope < 0.001f)
        {
            state.active.store(false, std::memory_order_relaxed);
            break;
        }
    }
}

void DrumEngine::renderDholak(float* outL, float* outR, int numSamples)
{
    auto& state = voiceStates[(int)DrumSound::PERCUSSION_DHOLAK];
    if (!state.active.load(std::memory_order_relaxed)) return;

    auto& p = voiceParams[(int)DrumSound::PERCUSSION_DHOLAK];
    float tuning = p.tuning.load(std::memory_order_relaxed);
    float decay = p.decay.load(std::memory_order_relaxed);
    float vol = p.volume.load(std::memory_order_relaxed) * state.velocity;
    float pan = p.pan.load(std::memory_order_relaxed);

    float panL = std::sqrt(juce::jlimit(0.0f, 1.0f, (1.0f - pan) * 0.5f));
    float panR = std::sqrt(juce::jlimit(0.0f, 1.0f, (1.0f + pan) * 0.5f));

    float decayCoeff = static_cast<float>(std::exp(-1.0 / (0.280 * decay * currentSampleRate)));

    for (int i = 0; i < numSamples; ++i)
    {
        // Dual timbre: deep Bayan pitch slide + resonant Dayan slap
        float freqBayan = (55.0f + 40.0f * state.envelope) * tuning;
        float freqDayan = 380.0f * tuning;

        state.phase += static_cast<float>(2.0 * 3.14159265358979323846 * freqBayan / currentSampleRate);
        if (state.phase >= 6.2831853f) state.phase -= 6.2831853f;

        float bayan = std::sin(state.phase) * 0.7f;
        float slapSnap = (state.sampleIndex < 120) ? std::sin(state.sampleIndex * 0.35f * freqDayan / 200.0f) * 0.5f : 0.0f;
        float sample = (bayan + slapSnap) * state.envelope * vol;

        outL[i] += sample * panL;
        outR[i] += sample * panR;

        state.envelope *= decayCoeff;
        state.sampleIndex++;

        if (state.envelope < 0.001f)
        {
            state.active.store(false, std::memory_order_relaxed);
            break;
        }
    }
}

} // namespace paasphoos
