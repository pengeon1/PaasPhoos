#include "JitterBuffer.h"

namespace paasphoos
{

JitterBuffer::JitterBuffer()
{
    prepare(48000.0, 2, DEFAULT_CAPACITY_SAMPLES);
}

void JitterBuffer::prepare(double newSampleRate, int channels, int capacitySamples)
{
    sampleRate = (newSampleRate > 0.0) ? newSampleRate : 48000.0;
    numChannels = std::max(1, channels);
    capacity = std::max(1024, capacitySamples);

    sampleStorage.assign(numChannels * capacity, 0.0f);
    reset();
}

void JitterBuffer::reset()
{
    writeHead.store(0, std::memory_order_relaxed);
    readHeadFractional.store(0.0f, std::memory_order_relaxed);
    samplesAvailable.store(0, std::memory_order_relaxed);
    isPrimed.store(false, std::memory_order_relaxed);

    std::fill(sampleStorage.begin(), sampleStorage.end(), 0.0f);

    lastReceivedSeq = 0;
    lastTransitDelayUs = 0;
    hasReceivedFirstPacket = false;

    smoothedOccupancy = 0.0f;
    currentDriftPpm.store(0.0f, std::memory_order_relaxed);

    lastSampleCh0 = 0.0f;
    lastSampleCh1 = 0.0f;
    concealmentDecay = 1.0f;
    samplesInConcealment = 0;

    packetLossCount.store(0, std::memory_order_relaxed);
    concealmentCount.store(0, std::memory_order_relaxed);
    underrunCount.store(0, std::memory_order_relaxed);
}

void JitterBuffer::setMode(JitterBufferMode newMode) noexcept
{
    mode.store(newMode, std::memory_order_relaxed);
    if (newMode == JitterBufferMode::MANUAL)
    {
        targetDelayMs.store(manualTargetDelayMs.load(std::memory_order_relaxed), std::memory_order_relaxed);
    }
}

void JitterBuffer::setManualTargetDelayMs(float delayMs) noexcept
{
    float clamped = juce::jlimit(2.0f, 60.0f, delayMs);
    manualTargetDelayMs.store(clamped, std::memory_order_relaxed);
    if (mode.load(std::memory_order_relaxed) == JitterBufferMode::MANUAL)
    {
        targetDelayMs.store(clamped, std::memory_order_relaxed);
    }
}

bool JitterBuffer::pushPacket(uint32_t sequenceNumber,
                             uint64_t timestampUs,
                             const float* const* channelData,
                             int inChannels,
                             int numSamples) noexcept
{
    if (channelData == nullptr || numSamples <= 0 || inChannels <= 0)
        return false;

    // 1. Sequence Continuity & Loss Detection
    if (hasReceivedFirstPacket)
    {
        int32_t seqDiff = static_cast<int32_t>(sequenceNumber - lastReceivedSeq);
        if (seqDiff > 1)
        {
            // Dropped packets detected
            packetLossCount.fetch_add(static_cast<uint32_t>(seqDiff - 1), std::memory_order_relaxed);
        }
    }
    lastReceivedSeq = sequenceNumber;

    // 2. RFC 3550 Delay Variation & Adaptive Jitter Calculation
    uint64_t nowUs = getSteadyClockUs();
    if (nowUs >= timestampUs)
    {
        uint64_t transitUs = nowUs - timestampUs;
        if (hasReceivedFirstPacket)
        {
            int64_t delayDiff = std::abs(static_cast<int64_t>(transitUs) - static_cast<int64_t>(lastTransitDelayUs));
            float diffMs = static_cast<float>(delayDiff) / 1000.0f;
            float j = estimatedJitterMs.load(std::memory_order_relaxed);
            // RFC 3550 EMA filter: J = J + (|D| - J) / 16
            j += (diffMs - j) / 16.0f;
            estimatedJitterMs.store(j, std::memory_order_relaxed);

            if (mode.load(std::memory_order_relaxed) == JitterBufferMode::AUTO)
            {
                // LAN playout target: 2x jitter + 2ms headroom, clamped between 2.5ms and 45ms
                float autoTarget = juce::jlimit(2.5f, 45.0f, j * 2.0f + 2.0f);
                targetDelayMs.store(autoTarget, std::memory_order_relaxed);
            }
        }
        lastTransitDelayUs = transitUs;
    }
    hasReceivedFirstPacket = true;

    // 3. Write audio samples into circular storage
    int currentWrite = writeHead.load(std::memory_order_relaxed);
    const int chans = std::min(numChannels, inChannels);

    for (int ch = 0; ch < chans; ++ch)
    {
        const float* src = channelData[ch];
        if (src == nullptr) continue;

        float* destChannelBase = &sampleStorage[ch * capacity];
        int writePos = currentWrite;

        for (int i = 0; i < numSamples; ++i)
        {
            destChannelBase[writePos] = src[i];
            writePos = (writePos + 1) % capacity;
        }
    }

    // Duplicate mono channel if source was 1-channel
    if (chans == 1 && numChannels >= 2)
    {
        float* ch1Base = &sampleStorage[1 * capacity];
        const float* ch0Base = &sampleStorage[0 * capacity];
        int writePos = currentWrite;
        for (int i = 0; i < numSamples; ++i)
        {
            ch1Base[writePos] = ch0Base[writePos];
            writePos = (writePos + 1) % capacity;
        }
    }

    // Advance write head
    int newWrite = (currentWrite + numSamples) % capacity;
    writeHead.store(newWrite, std::memory_order_relaxed);
    samplesAvailable.fetch_add(numSamples, std::memory_order_relaxed);

    // 4. Check Initial Priming Threshold
    if (!isPrimed.load(std::memory_order_relaxed))
    {
        float targetMs = targetDelayMs.load(std::memory_order_relaxed);
        int targetSamples = static_cast<int>(targetMs * sampleRate / 1000.0);
        if (samplesAvailable.load(std::memory_order_relaxed) >= targetSamples)
        {
            // Position read head exactly targetSamples behind write head
            int readPos = (newWrite - targetSamples + capacity) % capacity;
            readHeadFractional.store(static_cast<float>(readPos), std::memory_order_relaxed);
            smoothedOccupancy = static_cast<float>(targetSamples);
            isPrimed.store(true, std::memory_order_relaxed);
        }
    }

    return true;
}

float JitterBuffer::hermiteInterpolate(int channel, float index) const noexcept
{
    float* base = const_cast<float*>(&sampleStorage[channel * capacity]);

    int i0 = static_cast<int>(index);
    float frac = index - static_cast<float>(i0);

    int im1 = (i0 - 1 + capacity) % capacity;
    int ip1 = (i0 + 1) % capacity;
    int ip2 = (i0 + 2) % capacity;
    i0 = (i0 + capacity) % capacity;

    float y_m1 = base[im1];
    float y_0  = base[i0];
    float y_p1 = base[ip1];
    float y_p2 = base[ip2];

    // 4-point Hermite cubic polynomial interpolation
    float c0 = y_0;
    float c1 = 0.5f * (y_p1 - y_m1);
    float c2 = y_m1 - 2.5f * y_0 + 2.0f * y_p1 - 0.5f * y_p2;
    float c3 = 0.5f * (y_p2 - y_m1) + 1.5f * (y_0 - y_p1);

    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}

void JitterBuffer::pullAudio(float* const* destChannels, int destNumChannels, int numSamples) noexcept
{
    if (destChannels == nullptr || numSamples <= 0 || destNumChannels <= 0)
        return;

    // Silence if not yet primed
    if (!isPrimed.load(std::memory_order_relaxed))
    {
        for (int ch = 0; ch < destNumChannels; ++ch)
        {
            if (destChannels[ch] != nullptr)
                std::fill_n(destChannels[ch], numSamples, 0.0f);
        }
        return;
    }

    int available = samplesAvailable.load(std::memory_order_relaxed);

    // 1. Packet Loss Concealment (PLC) / Buffer Starvation
    if (available < numSamples)
    {
        underrunCount.fetch_add(1, std::memory_order_relaxed);
        concealmentCount.fetch_add(1, std::memory_order_relaxed);

        // Smooth exponential decay (0.97 per sample) avoiding digital clicks
        for (int i = 0; i < numSamples; ++i)
        {
            concealmentDecay *= 0.97f;
            if (concealmentDecay < 0.0001f)
                concealmentDecay = 0.0f;

            float out0 = lastSampleCh0 * concealmentDecay;
            float out1 = lastSampleCh1 * concealmentDecay;

            if (destNumChannels > 0 && destChannels[0] != nullptr) destChannels[0][i] = out0;
            if (destNumChannels > 1 && destChannels[1] != nullptr) destChannels[1][i] = out1;
        }

        samplesInConcealment += numSamples;
        if (samplesInConcealment > static_cast<int>(sampleRate * 0.25)) // 250ms dropout -> reset prime
        {
            isPrimed.store(false, std::memory_order_relaxed);
            samplesAvailable.store(0, std::memory_order_relaxed);
        }
        return;
    }

    // 2. Sample Clock Drift Correction Calculation
    float targetMs = targetDelayMs.load(std::memory_order_relaxed);
    float targetSamples = static_cast<float>(targetMs * sampleRate / 1000.0);

    // Exponential moving average filter for buffer occupancy
    smoothedOccupancy = smoothedOccupancy * 0.998f + static_cast<float>(available) * 0.002f;
    float occupancyError = smoothedOccupancy - targetSamples;

    // Subtle drift correction factor: proportional control clamped to +/- 0.08% (+/- 800 ppm)
    // Imperceptible to pitch, avoids buffer overrun or starvation over hours of rehearsal
    float driftRatio = 1.0f;
    if (std::abs(occupancyError) > 12.0f) // Deadband to avoid reacting to normal micro-jitter
    {
        float correction = occupancyError * 0.000015f;
        correction = juce::jlimit(-0.0008f, 0.0008f, correction);
        driftRatio += correction;
    }
    currentDriftPpm.store((driftRatio - 1.0f) * 1000000.0f, std::memory_order_relaxed);

    // 3. Resampled Playout Read
    float readPos = readHeadFractional.load(std::memory_order_relaxed);
    const int chans = std::min(numChannels, destNumChannels);

    for (int i = 0; i < numSamples; ++i)
    {
        for (int ch = 0; ch < chans; ++ch)
        {
            if (destChannels[ch] != nullptr)
            {
                destChannels[ch][i] = hermiteInterpolate(ch, readPos);
            }
        }

        readPos += driftRatio;
        if (readPos >= static_cast<float>(capacity))
            readPos -= static_cast<float>(capacity);
    }

    // 4. Smooth Crossfade from Concealment to Real Audio if recovering
    if (samplesInConcealment > 0)
    {
        int crossfadeLen = std::min(numSamples, CONCEALMENT_CROSSFADE_SAMPLES);
        for (int i = 0; i < crossfadeLen; ++i)
        {
            float t = static_cast<float>(i) / static_cast<float>(crossfadeLen);
            float fadeOld = (1.0f - t) * concealmentDecay;
            if (destChannels[0] != nullptr)
                destChannels[0][i] = destChannels[0][i] * t + (lastSampleCh0 * fadeOld);
            if (destNumChannels > 1 && destChannels[1] != nullptr)
                destChannels[1][i] = destChannels[1][i] * t + (lastSampleCh1 * fadeOld);
        }
        samplesInConcealment = 0;
        concealmentDecay = 1.0f;
    }

    // Store last played samples for future PLC if needed
    if (numSamples > 0)
    {
        lastSampleCh0 = (destChannels[0] != nullptr) ? destChannels[0][numSamples - 1] : 0.0f;
        lastSampleCh1 = (destNumChannels > 1 && destChannels[1] != nullptr) ? destChannels[1][numSamples - 1] : lastSampleCh0;
    }

    readHeadFractional.store(readPos, std::memory_order_relaxed);

    // Update samples available count
    int consumedSamples = static_cast<int>(numSamples * driftRatio);
    int newAvailable = std::max(0, available - consumedSamples);
    samplesAvailable.store(newAvailable, std::memory_order_relaxed);
}

JitterBufferMetrics JitterBuffer::getMetrics() const noexcept
{
    JitterBufferMetrics m;
    int avail = samplesAvailable.load(std::memory_order_relaxed);
    m.currentFillMs = static_cast<float>(avail) * 1000.0f / static_cast<float>(sampleRate);
    m.targetDelayMs = targetDelayMs.load(std::memory_order_relaxed);
    m.estimatedJitterMs = estimatedJitterMs.load(std::memory_order_relaxed);
    m.driftRatioPpm = currentDriftPpm.load(std::memory_order_relaxed);
    m.packetLossCount = packetLossCount.load(std::memory_order_relaxed);
    m.concealmentCount = concealmentCount.load(std::memory_order_relaxed);
    m.underrunCount = underrunCount.load(std::memory_order_relaxed);
    m.isPrimed = isPrimed.load(std::memory_order_relaxed);
    return m;
}

} // namespace paasphoos
