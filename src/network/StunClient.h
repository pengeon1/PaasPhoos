#pragma once

#include <juce_core/juce_core.h>
#include <atomic>
#include <string>

namespace paasphoos
{

struct StunResult
{
    bool success{false};
    juce::String publicIp;
    int publicPort{0};
    juce::String roomCode;
    juce::String errorMessage;
};

class StunClient
{
public:
    static constexpr const char* DEFAULT_STUN_SERVER = "stun.l.google.com";
    static constexpr int DEFAULT_STUN_PORT = 19302;

    StunClient() = default;
    ~StunClient() = default;

    static StunResult resolvePublicEndpoint(int localBoundPort,
                                            const juce::String& stunServer = DEFAULT_STUN_SERVER,
                                            int stunPort = DEFAULT_STUN_PORT);

    static juce::String generateRoomCode(const juce::String& ip, int port);
    static bool parseRoomCode(const juce::String& roomCode, juce::String& outIp, int& outPort);
};

} // namespace paasphoos
