#pragma once

#include <atomic>
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace paasphoos
{

enum class ConnectionHealth
{
    DISCONNECTED,
    EXCELLENT, // Green (<15 ms mouth-to-ear)
    GOOD,      // Cyan (<25 ms)
    FAIR,      // Amber (<35 ms)
    POOR       // Red (>=35 ms or high loss)
};

class NetworkStats
{
public:
    NetworkStats()
        : rttMs(0.0f),
          jitterMs(0.0f),
          packetLossPercent(0.0f),
          totalEstimatedLatencyMs(0.0f),
          packetsSent(0),
          packetsReceived(0),
          packetsLost(0),
          lastTransitTimeUs(0),
          hasTransitHistory(false),
          lastReceivedSeq(0)
    {
    }

    void reset()
    {
        rttMs.store(0.0f, std::memory_order_relaxed);
        jitterMs.store(0.0f, std::memory_order_relaxed);
        packetLossPercent.store(0.0f, std::memory_order_relaxed);
        totalEstimatedLatencyMs.store(0.0f, std::memory_order_relaxed);
        packetsSent.store(0, std::memory_order_relaxed);
        packetsReceived.store(0, std::memory_order_relaxed);
        packetsLost.store(0, std::memory_order_relaxed);
        hasTransitHistory = false;
        lastTransitTimeUs = 0;
        lastReceivedSeq = 0;
    }

    void recordSentPacket()
    {
        packetsSent.fetch_add(1, std::memory_order_relaxed);
    }

    /**
     * Record incoming packet for jitter and loss calculation (RFC 3550).
     */
    void recordReceivedPacket(uint32_t sequenceNumber, uint64_t senderTimestampUs, uint64_t receiverTimestampUs)
    {
        packetsReceived.fetch_add(1, std::memory_order_relaxed);

        // Sequence number check for packet loss
        if (lastReceivedSeq > 0)
        {
            if (sequenceNumber > lastReceivedSeq + 1)
            {
                uint32_t gap = sequenceNumber - (lastReceivedSeq + 1);
                packetsLost.fetch_add(gap, std::memory_order_relaxed);
            }
        }
        lastReceivedSeq = sequenceNumber;

        // Update packet loss %
        uint64_t totalExpected = packetsReceived.load(std::memory_order_relaxed) + packetsLost.load(std::memory_order_relaxed);
        if (totalExpected > 50)
        {
            float loss = (static_cast<float>(packetsLost.load(std::memory_order_relaxed)) / static_cast<float>(totalExpected)) * 100.0f;
            packetLossPercent.store(loss, std::memory_order_relaxed);
        }

        // Jitter Calculation: D(i, j) = (R_j - S_j) - (R_i - S_i)
        if (receiverTimestampUs >= senderTimestampUs)
        {
            int64_t transitUs = static_cast<int64_t>(receiverTimestampUs - senderTimestampUs);
            if (hasTransitHistory)
            {
                int64_t deltaUs = std::abs(transitUs - lastTransitTimeUs);
                float curJitter = jitterMs.load(std::memory_order_relaxed);
                float newJitter = curJitter + (static_cast<float>(deltaUs) / 1000.0f - curJitter) / 16.0f;
                jitterMs.store(std::max(0.0f, newJitter), std::memory_order_relaxed);
            }
            lastTransitTimeUs = transitUs;
            hasTransitHistory = true;
        }
    }

    /**
     * Update measured RTT from Pong response.
     */
    void recordRtt(float newRttMs, float hardwareInMs, float hardwareOutMs, float jitterBufferMs = 2.0f)
    {
        float currentRtt = rttMs.load(std::memory_order_relaxed);
        float filteredRtt = (currentRtt <= 0.001f) ? newRttMs : (currentRtt * 0.7f + newRttMs * 0.3f);
        rttMs.store(filteredRtt, std::memory_order_relaxed);

        // Mouth-to-ear latency estimate: InHW + (One-Way Net = RTT/2) + Jitter Buffer + OutHW
        float total = hardwareInMs + (filteredRtt * 0.5f) + jitterBufferMs + hardwareOutMs;
        totalEstimatedLatencyMs.store(total, std::memory_order_relaxed);
    }

    float getRttMs() const noexcept { return rttMs.load(std::memory_order_relaxed); }
    float getJitterMs() const noexcept { return jitterMs.load(std::memory_order_relaxed); }
    float getPacketLossPercent() const noexcept { return packetLossPercent.load(std::memory_order_relaxed); }
    float getTotalLatencyMs() const noexcept { return totalEstimatedLatencyMs.load(std::memory_order_relaxed); }
    uint64_t getPacketsSent() const noexcept { return packetsSent.load(std::memory_order_relaxed); }
    uint64_t getPacketsReceived() const noexcept { return packetsReceived.load(std::memory_order_relaxed); }
    uint64_t getPacketsLost() const noexcept { return packetsLost.load(std::memory_order_relaxed); }

    ConnectionHealth getHealthStatus() const noexcept
    {
        float rtt = getRttMs();
        float jitter = getJitterMs();
        float loss = getPacketLossPercent();
        float total = getTotalLatencyMs();

        if (packetsReceived.load(std::memory_order_relaxed) == 0 && rtt <= 0.001f)
            return ConnectionHealth::DISCONNECTED;

        if (total < 12.0f && jitter < 2.0f && loss < 0.1f)
            return ConnectionHealth::EXCELLENT;
        if (total < 25.0f && jitter < 5.0f && loss < 1.0f)
            return ConnectionHealth::GOOD;
        if (total < 35.0f && jitter < 10.0f && loss < 3.0f)
            return ConnectionHealth::FAIR;

        return ConnectionHealth::POOR;
    }

private:
    std::atomic<float> rttMs;
    std::atomic<float> jitterMs;
    std::atomic<float> packetLossPercent;
    std::atomic<float> totalEstimatedLatencyMs;

    std::atomic<uint64_t> packetsSent;
    std::atomic<uint64_t> packetsReceived;
    std::atomic<uint64_t> packetsLost;

    int64_t lastTransitTimeUs;
    bool hasTransitHistory;
    uint32_t lastReceivedSeq;
};

} // namespace paasphoos
