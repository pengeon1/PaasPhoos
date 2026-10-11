#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include "NetworkPacket.h"
#include <vector>
#include <mutex>

namespace paasphoos
{

struct DiscoveredSession
{
    juce::String sessionName;
    juce::String hostName;
    juce::String ipAddress;
    int audioPort = 44045;
    int participantCount = 1;
    int maxParticipants = 6;
    juce::Time lastSeen;
};

class LanDiscovery : public juce::Thread,
                     public juce::ChangeBroadcaster
{
public:
    static constexpr int DISCOVERY_PORT = 44044;

    LanDiscovery();
    ~LanDiscovery() override;

    void startHosting(const juce::String& sessionName, const juce::String& hostName, int audioPort);
    void startBrowsing();
    void stop();

    bool isHosting() const noexcept { return hostingActive.load(); }
    bool isBrowsing() const noexcept { return browsingActive.load(); }

    std::vector<DiscoveredSession> getDiscoveredSessions();

    int getBeaconsSent() const noexcept { return beaconsSentCount.load(); }
    int getBeaconsReceived() const noexcept { return beaconsReceivedCount.load(); }

private:
    void run() override;

    std::atomic<bool> hostingActive{false};
    std::atomic<bool> browsingActive{false};

    juce::String currentSessionName;
    juce::String currentHostName;
    int currentAudioPort{44045};

    std::mutex sessionListMutex;
    std::vector<DiscoveredSession> discoveredSessions;

    std::atomic<int> beaconsSentCount{0};
    std::atomic<int> beaconsReceivedCount{0};

    std::unique_ptr<juce::DatagramSocket> socket;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LanDiscovery)
};

} // namespace paasphoos
