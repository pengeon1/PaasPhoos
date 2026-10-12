#pragma once

#include <cstdint>
#include <cstring>
#include <chrono>

namespace paasphoos
{

#pragma pack(push, 1)

/**
 * Protocol Magic: "PPHS" (PaasPhoos)
 */
constexpr uint32_t PROTOCOL_MAGIC = 0x50504853;
constexpr uint8_t PROTOCOL_VERSION = 1;
constexpr int MAX_PARTICIPANTS = 6; // 1 Host + up to 5 Clients

enum class PacketType : uint8_t
{
    HEARTBEAT       = 0x01,
    PING            = 0x02,
    PONG            = 0x03,
    DISCOVERY_REQ   = 0x04,
    DISCOVERY_RESP  = 0x05,
    JOIN_REQ        = 0x06,
    JOIN_ACCEPT     = 0x07,
    PEER_UPDATE     = 0x08,
    LEAVE_REQ       = 0x09,
    AUDIO_PCM16     = 0x10,
    AUDIO_FLOAT32   = 0x11
};

struct PacketHeader
{
    uint32_t magic;           // PROTOCOL_MAGIC
    uint8_t version;          // PROTOCOL_VERSION
    PacketType type;          // PacketType
    uint8_t participantId;    // 0 = host, 1..5 = peers
    uint8_t channelCount;     // 1 = mono, 2 = stereo
    uint32_t sequenceNumber;  // Monotonically increasing sequence
    uint64_t timestampUs;     // Send timestamp in microseconds
    uint16_t sampleCount;     // Number of audio samples per channel
    uint16_t payloadBytes;    // Size of payload following this header
};

// Ping / Pong Payload
struct PingPayload
{
    uint32_t pingSeq;
    uint64_t clientTimestampUs;
    uint64_t serverTimestampUs;
};

// LAN Discovery Announcement
struct DiscoveryBeacon
{
    char sessionName[32];
    char hostName[32];
    uint16_t audioPort;
    uint8_t participantCount;
    uint8_t maxParticipants;
};

// Join Request / Accept
struct JoinInfo
{
    char clientName[32];
    char roleOrInstrument[32]; // e.g. "Guitar 2", "Vocals", "Drums"
    uint8_t assignedId;
    uint16_t assignedPort;
};

// Multi-peer participant entry
struct ParticipantEntry
{
    uint8_t participantId; // 0 = Host, 1..5 = Peers
    uint8_t isConnected;   // 1 = active, 0 = empty/left
    char name[24];
    char roleOrInstrument[24];
    uint16_t port;
};

// Multi-peer directory update broadcast
struct PeerUpdatePayload
{
    uint8_t totalParticipants;
    ParticipantEntry entries[MAX_PARTICIPANTS];
};

#pragma pack(pop)

inline uint64_t getSteadyClockUs() noexcept
{
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(now).count());
}

} // namespace paasphoos
