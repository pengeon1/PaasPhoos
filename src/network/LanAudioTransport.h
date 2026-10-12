#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include "NetworkPacket.h"
#include "NetworkStats.h"
#include "JitterBuffer.h"
#include "../audio/RealtimeRingBuffer.h"
#include <memory>
#include <atomic>
#include <vector>
#include <array>

namespace paasphoos
{

enum class SessionRole
{
    NONE,
    HOST,
    CLIENT
};

struct RemotePeer
{
    uint8_t participantId{0};
    std::atomic<bool> connected{false};
    juce::String name;
    juce::String roleOrInstrument;
    juce::String ipAddress;
    int port{0};
    std::atomic<juce::int64> lastSeenMs{0};

    JitterBuffer jitterBuffer;
    NetworkStats stats;
};

class LanAudioTransport : public juce::Thread,
                          public juce::ChangeBroadcaster
{
public:
    static constexpr int DEFAULT_AUDIO_PORT = 44045;
    static constexpr int RING_BUFFER_CAPACITY = 8192;

    LanAudioTransport();
    ~LanAudioTransport() override;

    // Start / Stop Session
    bool startAsHost(int port = DEFAULT_AUDIO_PORT,
                     const juce::String& hostName = "Host Musician",
                     const juce::String& instrument = "Guitar 1");

    bool startAsClient(const juce::String& hostIp,
                       int hostPort = DEFAULT_AUDIO_PORT,
                       const juce::String& clientName = "Band Member",
                       const juce::String& instrument = "Guitar 2");

    void disconnect();

    SessionRole getRole() const noexcept { return role.load(std::memory_order_relaxed); }
    bool isConnected() const noexcept { return connected.load(std::memory_order_relaxed); }
    uint8_t getMyParticipantId() const noexcept { return myParticipantId.load(std::memory_order_relaxed); }

    juce::String getRemoteAddress() const; // For client: host address. For host: first active client or bound IP
    int getRemotePort() const;

    int getActivePeerCount() const noexcept;
    const RemotePeer* getPeer(int participantIndex) const noexcept;
    RemotePeer* getPeer(int participantIndex) noexcept;

    // Real-time audio callback interface (Lock-free & Allocation-free)
    void pushLocalAudio(const float* const* channelData, int numChannels, int numSamples) noexcept;
    void pullPeerAudio(int participantIndex, float* const* destChannels, int numChannels, int numSamples) noexcept;

    // Jitter buffer configuration
    void setJitterBufferMode(int participantIndex, JitterBufferMode mode) noexcept;
    void setManualTargetDelayMs(int participantIndex, float delayMs) noexcept;

    // Network Simulator
    void setNetworkSimulator(class NetworkSimulator* sim) noexcept { networkSimulator = sim; }

    // Aggregate Network Stats (primary peer or host)
    NetworkStats& getStats() { return primaryStats; }
    const NetworkStats& getStats() const { return primaryStats; }

    void setHardwareLatencies(float inMs, float outMs) noexcept
    {
        hardwareInLatencyMs.store(inMs, std::memory_order_relaxed);
        hardwareOutLatencyMs.store(outMs, std::memory_order_relaxed);
    }

private:
    void run() override;
    void handleIncomingPacket(const uint8_t* data, int length, const juce::String& senderIp, int senderPort);
    void handleJoinRequest(const uint8_t* data, int length, const juce::String& senderIp, int senderPort);
    void handleJoinAccept(const uint8_t* data, int length);
    void handlePeerUpdate(const uint8_t* data, int length);
    void broadcastPeerUpdate();
    void sendPingTo(const juce::String& ip, int port, uint8_t peerId);
    void sendPings();
    void checkTimeouts();

    std::atomic<SessionRole> role{SessionRole::NONE};
    std::atomic<bool> connected{false};
    std::atomic<uint8_t> myParticipantId{0};

    juce::String myName{"Musician"};
    juce::String myInstrument{"Guitar"};

    std::atomic<float> hardwareInLatencyMs{2.67f};
    std::atomic<float> hardwareOutLatencyMs{2.67f};

    juce::String targetIp;
    int targetPort{0};
    int localBoundPort{0};

    std::unique_ptr<juce::DatagramSocket> udpSocket;

    AudioRingBuffer<float> sendRingBuffer;
    NetworkStats primaryStats;

    // 6-Piece Band Peer Slots (0 = Host, 1..5 = Peers)
    std::array<std::unique_ptr<RemotePeer>, MAX_PARTICIPANTS> peers;

    uint32_t sendSequenceNumber{0};
    uint32_t pingSequenceNumber{0};
    juce::int64 lastPingTimeMs{0};
    juce::int64 lastTimeoutCheckMs{0};

    // Pre-allocated packet buffers (zero dynamic allocation on worker loop)
    std::vector<uint8_t> sendPacketBuffer;
    std::vector<int16_t> pcmConversionBuffer;
    std::vector<float> planarChannelBuffer0;
    std::vector<float> planarChannelBuffer1;

    class NetworkSimulator* networkSimulator{nullptr};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LanAudioTransport)
};

} // namespace paasphoos
