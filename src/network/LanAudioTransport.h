#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include "NetworkPacket.h"
#include "NetworkStats.h"
#include "../audio/RealtimeRingBuffer.h"
#include <memory>
#include <atomic>
#include <vector>

namespace paasphoos
{

enum class SessionRole
{
    NONE,
    HOST,
    CLIENT
};

class LanAudioTransport : public juce::Thread,
                          public juce::ChangeBroadcaster
{
public:
    static constexpr int DEFAULT_AUDIO_PORT = 44045;
    static constexpr int RING_BUFFER_CAPACITY = 8192;

    LanAudioTransport();
    ~LanAudioTransport() override;

    // Start / Stop
    bool startAsHost(int port = DEFAULT_AUDIO_PORT);
    bool startAsClient(const juce::String& hostIp, int hostPort = DEFAULT_AUDIO_PORT);
    void disconnect();

    SessionRole getRole() const noexcept { return role.load(); }
    bool isConnected() const noexcept { return connected.load(); }

    juce::String getRemoteAddress() const;
    int getRemotePort() const;

    // Real-time audio callback interface (Lock-free & Allocation-free)
    void pushLocalAudio(const float* const* channelData, int numChannels, int numSamples) noexcept;
    void pullRemoteAudio(float* const* destChannels, int numChannels, int numSamples) noexcept;

    // Network stats
    NetworkStats& getStats() { return stats; }
    const NetworkStats& getStats() const { return stats; }

    void setHardwareLatencies(float inMs, float outMs) noexcept
    {
        hardwareInLatencyMs.store(inMs, std::memory_order_relaxed);
        hardwareOutLatencyMs.store(outMs, std::memory_order_relaxed);
    }

private:
    void run() override;
    void handleIncomingPacket(const uint8_t* data, int length, const juce::String& senderIp, int senderPort);
    void sendPing();

    std::atomic<SessionRole> role{SessionRole::NONE};
    std::atomic<bool> connected{false};

    std::atomic<float> hardwareInLatencyMs{2.67f};
    std::atomic<float> hardwareOutLatencyMs{2.67f};

    juce::String targetIp;
    int targetPort{0};
    int localBoundPort{0};

    std::unique_ptr<juce::DatagramSocket> udpSocket;

    AudioRingBuffer<float> sendRingBuffer;
    AudioRingBuffer<float> receiveRingBuffer;

    NetworkStats stats;

    uint32_t sendSequenceNumber{0};
    uint32_t pingSequenceNumber{0};
    juce::int64 lastPingTimeMs{0};

    // Pre-allocated packet buffers (zero dynamic allocation on worker loop)
    std::vector<uint8_t> sendPacketBuffer;
    std::vector<int16_t> pcmConversionBuffer;
    std::vector<float> planarChannelBuffer0;
    std::vector<float> planarChannelBuffer1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LanAudioTransport)
};

} // namespace paasphoos
