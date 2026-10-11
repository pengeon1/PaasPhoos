#include "LanDiscovery.h"

namespace paasphoos
{

LanDiscovery::LanDiscovery()
    : juce::Thread("PaasPhoos_LanDiscovery")
{
}

LanDiscovery::~LanDiscovery()
{
    stop();
}

void LanDiscovery::startHosting(const juce::String& sessionName, const juce::String& hostName, int audioPort)
{
    stop();

    currentSessionName = sessionName.isNotEmpty() ? sessionName : "Rehearsal Session";
    currentHostName = hostName.isNotEmpty() ? hostName : "Band Host";
    currentAudioPort = audioPort;

    hostingActive.store(true);
    browsingActive.store(false);
    beaconsSentCount.store(0);
    beaconsReceivedCount.store(0);

    // Create broadcast socket (bound to ephemeral port, enabling broadcast)
    socket = std::make_unique<juce::DatagramSocket>(false);
    socket->bindToPort(0);

    startThread(juce::Thread::Priority::normal);
}

void LanDiscovery::startBrowsing()
{
    stop();

    hostingActive.store(false);
    browsingActive.store(true);
    beaconsSentCount.store(0);
    beaconsReceivedCount.store(0);

    {
        std::lock_guard<std::mutex> lock(sessionListMutex);
        discoveredSessions.clear();
    }

    // Listen on DISCOVERY_PORT
    socket = std::make_unique<juce::DatagramSocket>(true);
    socket->bindToPort(DISCOVERY_PORT);

    startThread(juce::Thread::Priority::normal);
}

void LanDiscovery::stop()
{
    hostingActive.store(false);
    browsingActive.store(false);

    if (socket != nullptr)
    {
        socket->shutdown();
    }

    stopThread(2000);
    socket.reset();
}

std::vector<DiscoveredSession> LanDiscovery::getDiscoveredSessions()
{
    std::lock_guard<std::mutex> lock(sessionListMutex);

    // Filter out expired sessions (older than 6 seconds)
    auto now = juce::Time::getCurrentTime();
    discoveredSessions.erase(
        std::remove_if(discoveredSessions.begin(), discoveredSessions.end(),
                       [&](const DiscoveredSession& s) {
                           return (now - s.lastSeen).inMilliseconds() > 6000;
                       }),
        discoveredSessions.end()
    );

    return discoveredSessions;
}

void LanDiscovery::run()
{
    std::vector<uint8_t> buffer(1024);

    while (!threadShouldExit())
    {
        if (hostingActive.load())
        {
            // Build Discovery Beacon packet
            PacketHeader header;
            header.magic = PROTOCOL_MAGIC;
            header.version = PROTOCOL_VERSION;
            header.type = PacketType::DISCOVERY_RESP;
            header.participantId = 0;
            header.channelCount = 2;
            header.sequenceNumber = static_cast<uint32_t>(beaconsSentCount.load());
            header.timestampUs = getSteadyClockUs();
            header.sampleCount = 0;
            header.payloadBytes = sizeof(DiscoveryBeacon);

            DiscoveryBeacon beacon;
            std::memset(&beacon, 0, sizeof(beacon));
            currentSessionName.copyToUTF8(beacon.sessionName, sizeof(beacon.sessionName) - 1);
            currentHostName.copyToUTF8(beacon.hostName, sizeof(beacon.hostName) - 1);
            beacon.audioPort = static_cast<uint16_t>(currentAudioPort);
            beacon.participantCount = 1;
            beacon.maxParticipants = 6;

            std::vector<uint8_t> packet(sizeof(PacketHeader) + sizeof(DiscoveryBeacon));
            std::memcpy(packet.data(), &header, sizeof(header));
            std::memcpy(packet.data() + sizeof(header), &beacon, sizeof(beacon));

            if (socket != nullptr)
            {
                // Send UDP broadcast on LAN
                socket->write("255.255.255.255", DISCOVERY_PORT, packet.data(), static_cast<int>(packet.size()));
                beaconsSentCount.fetch_add(1);
            }

            // Beacon interval: 1000 ms
            wait(1000);
        }
        else if (browsingActive.load())
        {
            if (socket != nullptr)
            {
                juce::String senderIp;
                int senderPort = 0;
                int bytesRead = socket->read(buffer.data(), static_cast<int>(buffer.size()), false, senderIp, senderPort);

                if (bytesRead >= static_cast<int>(sizeof(PacketHeader) + sizeof(DiscoveryBeacon)))
                {
                    PacketHeader header;
                    std::memcpy(&header, buffer.data(), sizeof(header));

                    if (header.magic == PROTOCOL_MAGIC && header.type == PacketType::DISCOVERY_RESP)
                    {
                        DiscoveryBeacon beacon;
                        std::memcpy(&beacon, buffer.data() + sizeof(header), sizeof(beacon));

                        DiscoveredSession session;
                        session.sessionName = juce::String::fromUTF8(beacon.sessionName);
                        session.hostName = juce::String::fromUTF8(beacon.hostName);
                        session.ipAddress = senderIp;
                        session.audioPort = beacon.audioPort;
                        session.participantCount = beacon.participantCount;
                        session.maxParticipants = beacon.maxParticipants;
                        session.lastSeen = juce::Time::getCurrentTime();

                        beaconsReceivedCount.fetch_add(1);

                        // Add or update discovered session
                        {
                            std::lock_guard<std::mutex> lock(sessionListMutex);
                            bool found = false;
                            for (auto& s : discoveredSessions)
                            {
                                if (s.ipAddress == session.ipAddress && s.audioPort == session.audioPort)
                                {
                                    s = session;
                                    found = true;
                                    break;
                                }
                            }
                            if (!found)
                            {
                                discoveredSessions.push_back(session);
                            }
                        }

                        sendChangeMessage();
                    }
                }
            }

            // Wait small interval or until packet arrives
            wait(50);
        }
        else
        {
            wait(100);
        }
    }
}

} // namespace paasphoos
