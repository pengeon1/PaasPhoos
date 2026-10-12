#pragma once

#include <cstdint>
#include <atomic>
#include <random>
#include <chrono>

namespace paasphoos
{

class NetworkSimulator
{
public:
    NetworkSimulator() = default;
    ~NetworkSimulator() = default;

    void setEnabled(bool enabled) noexcept { isEnabled.store(enabled, std::memory_order_relaxed); }
    bool getEnabled() const noexcept { return isEnabled.load(std::memory_order_relaxed); }

    void setArtificialLatencyMs(float ms) noexcept { artificialLatencyMs.store(ms, std::memory_order_relaxed); }
    float getArtificialLatencyMs() const noexcept { return artificialLatencyMs.load(std::memory_order_relaxed); }

    void setArtificialJitterMs(float ms) noexcept { artificialJitterMs.store(ms, std::memory_order_relaxed); }
    float getArtificialJitterMs() const noexcept { return artificialJitterMs.load(std::memory_order_relaxed); }

    void setArtificialLossPercent(float percent) noexcept { artificialLossPercent.store(percent, std::memory_order_relaxed); }
    float getArtificialLossPercent() const noexcept { return artificialLossPercent.load(std::memory_order_relaxed); }

    // Evaluates whether current packet should be dropped
    bool shouldDropPacket() noexcept
    {
        if (!isEnabled.load(std::memory_order_relaxed))
            return false;

        float lossRate = artificialLossPercent.load(std::memory_order_relaxed);
        if (lossRate <= 0.0f)
            return false;

        float roll = dist(rng) * 100.0f;
        return roll < lossRate;
    }

    // Computes simulated delay for packet
    int calculateDelayMs() noexcept
    {
        if (!isEnabled.load(std::memory_order_relaxed))
            return 0;

        float base = artificialLatencyMs.load(std::memory_order_relaxed);
        float jitter = artificialJitterMs.load(std::memory_order_relaxed);

        if (jitter > 0.0f)
        {
            float variance = (dist(rng) * 2.0f - 1.0f) * jitter;
            base = std::max(0.0f, base + variance);
        }

        return static_cast<int>(base);
    }

private:
    std::atomic<bool> isEnabled{false};
    std::atomic<float> artificialLatencyMs{25.0f}; // Default 25 ms internet delay
    std::atomic<float> artificialJitterMs{5.0f};   // Default 5 ms jitter
    std::atomic<float> artificialLossPercent{1.5f}; // Default 1.5% loss

    std::mt19937 rng{42};
    std::uniform_real_distribution<float> dist{0.0f, 1.0f};
};

} // namespace paasphoos
