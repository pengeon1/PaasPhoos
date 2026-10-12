#include "LanAudioTransport.h"

#if JUCE_WINDOWS
#include <windows.h>
#include <avrt.h>
#endif

namespace paasphoos
{

LanAudioTransport::LanAudioTransport()
    : juce::Thread("PaasPhoos_LanTransport")
{
    sendRingBuffer.initialize(2, RING_BUFFER_CAPACITY);

    for (int i = 0; i < MAX_PARTICIPANTS; ++i)
    {
        peers[i] = std::make_unique<RemotePeer>();
        peers[i]->participantId = static_cast<uint8_t>(i);
        peers[i]->connected.store(false);
        peers[i]->jitterBuffer.prepare(48000.0, 2, JitterBuffer::DEFAULT_CAPACITY_SAMPLES);
        peers[i]->name = (i == 0) ? "Host" : juce::String::formatted("Peer %d", i);
        peers[i]->roleOrInstrument = "Musician";
    }

    sendPacketBuffer.resize(4096, 0);
    pcmConversionBuffer.resize(2048, 0);
    planarChannelBuffer0.resize(1024, 0.0f);
    planarChannelBuffer1.resize(1024, 0.0f);
}

LanAudioTransport::~LanAudioTransport()
{
    disconnect();
}

bool LanAudioTransport::startAsHost(int port, const juce::String& hostName, const juce::String& instrument)
{
    disconnect();

    udpSocket = std::make_unique<juce::DatagramSocket>(false);
    if (!udpSocket->bindToPort(port))
    {
        udpSocket.reset();
        return false;
    }

    localBoundPort = port;
    role.store(SessionRole::HOST);
    myParticipantId.store(0);
    myName = hostName.isNotEmpty() ? hostName : "Host Musician";
    myInstrument = instrument.isNotEmpty() ? instrument : "Guitar 1";

    connected.store(true);
    targetIp = "";
    targetPort = 0;
    sendSequenceNumber = 0;
    pingSequenceNumber = 0;
    lastPingTimeMs = 0;
    lastTimeoutCheckMs = 0;

    sendRingBuffer.reset();
    primaryStats.reset();

    // Slot 0 is local host
    peers[0]->name = myName;
    peers[0]->roleOrInstrument = myInstrument;
    peers[0]->connected.store(true);
    peers[0]->ipAddress = "127.0.0.1";
    peers[0]->port = port;

    // Reset client slots 1..5
    for (int i = 1; i < MAX_PARTICIPANTS; ++i)
    {
        peers[i]->connected.store(false);
        peers[i]->jitterBuffer.reset();
        peers[i]->stats.reset();
        peers[i]->name = juce::String::formatted("Peer %d", i);
        peers[i]->roleOrInstrument = "Musician";
    }

    startThread(juce::Thread::Priority::highest);
    sendChangeMessage();
    return true;
}

bool LanAudioTransport::startAsClient(const juce::String& hostIp,
                                      int hostPort,
                                      const juce::String& clientName,
                                      const juce::String& instrument)
{
    disconnect();

    udpSocket = std::make_unique<juce::DatagramSocket>(false);
    if (!udpSocket->bindToPort(0))
    {
        udpSocket.reset();
        return false;
    }

    localBoundPort = udpSocket->getBoundPort();
    role.store(SessionRole::CLIENT);
    targetIp = hostIp;
    targetPort = hostPort;
    myName = clientName.isNotEmpty() ? clientName : "Band Member";
    myInstrument = instrument.isNotEmpty() ? instrument : "Guitar 2";
    myParticipantId.store(1); // Default until assigned by host

    connected.store(false);
    sendSequenceNumber = 0;
    pingSequenceNumber = 0;
    lastPingTimeMs = 0;
    lastTimeoutCheckMs = 0;

    sendRingBuffer.reset();
    primaryStats.reset();

    for (int i = 0; i < MAX_PARTICIPANTS; ++i)
    {
        peers[i]->connected.store(false);
        peers[i]->jitterBuffer.reset();
        peers[i]->stats.reset();
    }

    // Host slot
    peers[0]->name = "Host";
    peers[0]->roleOrInstrument = "Band Leader";
    peers[0]->ipAddress = hostIp;
    peers[0]->port = hostPort;

    // Send Join Request to Host
    PacketHeader header;
    header.magic = PROTOCOL_MAGIC;
    header.version = PROTOCOL_VERSION;
    header.type = PacketType::JOIN_REQ;
    header.participantId = 1;
    header.channelCount = 2;
    header.sequenceNumber = 0;
    header.timestampUs = getSteadyClockUs();
    header.sampleCount = 0;
    header.payloadBytes = sizeof(JoinInfo);

    JoinInfo join;
    std::memset(&join, 0, sizeof(join));
    myName.copyToUTF8(join.clientName, sizeof(join.clientName) - 1);
    myInstrument.copyToUTF8(join.roleOrInstrument, sizeof(join.roleOrInstrument) - 1);
    join.assignedId = 0;
    join.assignedPort = static_cast<uint16_t>(localBoundPort);

    std::vector<uint8_t> packet(sizeof(PacketHeader) + sizeof(JoinInfo));
    std::memcpy(packet.data(), &header, sizeof(header));
    std::memcpy(packet.data() + sizeof(header), &join, sizeof(join));

    udpSocket->write(targetIp, targetPort, packet.data(), static_cast<int>(packet.size()));

    startThread(juce::Thread::Priority::highest);
    sendChangeMessage();
    return true;
}

void LanAudioTransport::disconnect()
{
    // If client, send LEAVE_REQ before tearing down
    if (role.load(std::memory_order_relaxed) == SessionRole::CLIENT && udpSocket != nullptr && !targetIp.isEmpty())
    {
        PacketHeader leaveHeader;
        leaveHeader.magic = PROTOCOL_MAGIC;
        leaveHeader.version = PROTOCOL_VERSION;
        leaveHeader.type = PacketType::LEAVE_REQ;
        leaveHeader.participantId = myParticipantId.load(std::memory_order_relaxed);
        leaveHeader.channelCount = 2;
        leaveHeader.sequenceNumber = 0;
        leaveHeader.timestampUs = getSteadyClockUs();
        leaveHeader.sampleCount = 0;
        leaveHeader.payloadBytes = 0;

        udpSocket->write(targetIp, targetPort, &leaveHeader, sizeof(leaveHeader));
    }

    role.store(SessionRole::NONE);
    connected.store(false);

    if (udpSocket != nullptr)
        udpSocket->shutdown();

    stopThread(2000);
    udpSocket.reset();

    targetIp = "";
    targetPort = 0;
    primaryStats.reset();

    for (int i = 0; i < MAX_PARTICIPANTS; ++i)
    {
        peers[i]->connected.store(false);
        peers[i]->jitterBuffer.reset();
        peers[i]->stats.reset();
    }

    sendChangeMessage();
}

juce::String LanAudioTransport::getRemoteAddress() const
{
    if (role.load() == SessionRole::CLIENT)
        return targetIp;

    // For host: return IP of first connected client or bound status
    for (int i = 1; i < MAX_PARTICIPANTS; ++i)
    {
        if (peers[i]->connected.load())
            return peers[i]->ipAddress;
    }
    return "0.0.0.0";
}

int LanAudioTransport::getRemotePort() const
{
    if (role.load() == SessionRole::CLIENT)
        return targetPort;

    for (int i = 1; i < MAX_PARTICIPANTS; ++i)
    {
        if (peers[i]->connected.load())
            return peers[i]->port;
    }
    return localBoundPort;
}

int LanAudioTransport::getActivePeerCount() const noexcept
{
    int count = 0;
    for (int i = 0; i < MAX_PARTICIPANTS; ++i)
    {
        if (peers[i]->connected.load(std::memory_order_relaxed))
            count++;
    }
    return count;
}

const RemotePeer* LanAudioTransport::getPeer(int participantIndex) const noexcept
{
    if (participantIndex >= 0 && participantIndex < MAX_PARTICIPANTS)
        return peers[participantIndex].get();
    return nullptr;
}

RemotePeer* LanAudioTransport::getPeer(int participantIndex) noexcept
{
    if (participantIndex >= 0 && participantIndex < MAX_PARTICIPANTS)
        return peers[participantIndex].get();
    return nullptr;
}

void LanAudioTransport::setJitterBufferMode(int participantIndex, JitterBufferMode bufferMode) noexcept
{
    if (participantIndex >= 0 && participantIndex < MAX_PARTICIPANTS)
    {
        peers[participantIndex]->jitterBuffer.setMode(bufferMode);
    }
}

void LanAudioTransport::setManualTargetDelayMs(int participantIndex, float delayMs) noexcept
{
    if (participantIndex >= 0 && participantIndex < MAX_PARTICIPANTS)
    {
        peers[participantIndex]->jitterBuffer.setManualTargetDelayMs(delayMs);
    }
}

void LanAudioTransport::pushLocalAudio(const float* const* channelData, int numChannels, int numSamples) noexcept
{
    if (channelData != nullptr && numSamples > 0 && isConnected())
    {
        sendRingBuffer.writePlanar(channelData, numChannels, numSamples);
    }
}

void LanAudioTransport::pullPeerAudio(int participantIndex, float* const* destChannels, int numChannels, int numSamples) noexcept
{
    if (destChannels == nullptr || numSamples <= 0)
        return;

    if (participantIndex < 0 || participantIndex >= MAX_PARTICIPANTS || !peers[participantIndex]->connected.load(std::memory_order_relaxed))
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            if (destChannels[ch] != nullptr)
                std::fill_n(destChannels[ch], numSamples, 0.0f);
        }
        return;
    }

    peers[participantIndex]->jitterBuffer.pullAudio(destChannels, numChannels, numSamples);
}

void LanAudioTransport::handleJoinRequest(const uint8_t* data, int length, const juce::String& senderIp, int senderPort)
{
    if (role.load() != SessionRole::HOST || length < static_cast<int>(sizeof(PacketHeader) + sizeof(JoinInfo)))
        return;

    JoinInfo join;
    std::memcpy(&join, data + sizeof(PacketHeader), sizeof(join));

    // Check if client is already known or find first free slot (1..5)
    int assignedSlot = -1;
    for (int i = 1; i < MAX_PARTICIPANTS; ++i)
    {
        if (peers[i]->connected.load() && peers[i]->ipAddress == senderIp && peers[i]->port == senderPort)
        {
            assignedSlot = i;
            break;
        }
    }

    if (assignedSlot == -1)
    {
        for (int i = 1; i < MAX_PARTICIPANTS; ++i)
        {
            if (!peers[i]->connected.load())
            {
                assignedSlot = i;
                break;
            }
        }
    }

    if (assignedSlot == -1)
    {
        // Band session full (6/6 participants)
        return;
    }

    auto& peer = peers[assignedSlot];
    peer->participantId = static_cast<uint8_t>(assignedSlot);
    peer->name = juce::String::fromUTF8(join.clientName);
    peer->roleOrInstrument = juce::String::fromUTF8(join.roleOrInstrument);
    peer->ipAddress = senderIp;
    peer->port = senderPort;
    peer->lastSeenMs.store(juce::Time::currentTimeMillis());
    peer->jitterBuffer.reset();
    peer->stats.reset();
    peer->connected.store(true);

    // Send JOIN_ACCEPT with PeerUpdatePayload
    std::vector<uint8_t> packet(sizeof(PacketHeader) + sizeof(PeerUpdatePayload));
    PacketHeader* header = reinterpret_cast<PacketHeader*>(packet.data());
    header->magic = PROTOCOL_MAGIC;
    header->version = PROTOCOL_VERSION;
    header->type = PacketType::JOIN_ACCEPT;
    header->participantId = static_cast<uint8_t>(assignedSlot);
    header->channelCount = 2;
    header->sequenceNumber = 0;
    header->timestampUs = getSteadyClockUs();
    header->sampleCount = 0;
    header->payloadBytes = sizeof(PeerUpdatePayload);

    PeerUpdatePayload* payload = reinterpret_cast<PeerUpdatePayload*>(packet.data() + sizeof(PacketHeader));
    std::memset(payload, 0, sizeof(PeerUpdatePayload));
    payload->totalParticipants = static_cast<uint8_t>(getActivePeerCount());

    for (int i = 0; i < MAX_PARTICIPANTS; ++i)
    {
        payload->entries[i].participantId = static_cast<uint8_t>(i);
        payload->entries[i].isConnected = peers[i]->connected.load() ? 1 : 0;
        payload->entries[i].port = static_cast<uint16_t>(peers[i]->port);
        peers[i]->name.copyToUTF8(payload->entries[i].name, sizeof(payload->entries[i].name) - 1);
        peers[i]->roleOrInstrument.copyToUTF8(payload->entries[i].roleOrInstrument, sizeof(payload->entries[i].roleOrInstrument) - 1);
    }

    udpSocket->write(senderIp, senderPort, packet.data(), static_cast<int>(packet.size()));

    // Broadcast updated directory to all other connected clients
    broadcastPeerUpdate();
    sendChangeMessage();
}

void LanAudioTransport::handleJoinAccept(const uint8_t* data, int length)
{
    if (role.load() != SessionRole::CLIENT || length < static_cast<int>(sizeof(PacketHeader)))
        return;

    const PacketHeader* header = reinterpret_cast<const PacketHeader*>(data);
    myParticipantId.store(header->participantId);

    peers[0]->connected.store(true);
    connected.store(true);

    if (length >= static_cast<int>(sizeof(PacketHeader) + sizeof(PeerUpdatePayload)))
    {
        handlePeerUpdate(data, length);
    }

    sendChangeMessage();
}

void LanAudioTransport::handlePeerUpdate(const uint8_t* data, int length)
{
    if (length < static_cast<int>(sizeof(PacketHeader) + sizeof(PeerUpdatePayload)))
        return;

    const PeerUpdatePayload* payload = reinterpret_cast<const PeerUpdatePayload*>(data + sizeof(PacketHeader));

    for (int i = 0; i < MAX_PARTICIPANTS; ++i)
    {
        if (i == myParticipantId.load())
            continue; // Don't overwrite local settings

        const auto& entry = payload->entries[i];
        peers[i]->participantId = entry.participantId;
        peers[i]->connected.store(entry.isConnected == 1);
        peers[i]->name = juce::String::fromUTF8(entry.name);
        peers[i]->roleOrInstrument = juce::String::fromUTF8(entry.roleOrInstrument);
        peers[i]->port = entry.port;
    }

    sendChangeMessage();
}

void LanAudioTransport::broadcastPeerUpdate()
{
    if (role.load() != SessionRole::HOST || udpSocket == nullptr)
        return;

    std::vector<uint8_t> packet(sizeof(PacketHeader) + sizeof(PeerUpdatePayload));
    PacketHeader* header = reinterpret_cast<PacketHeader*>(packet.data());
    header->magic = PROTOCOL_MAGIC;
    header->version = PROTOCOL_VERSION;
    header->type = PacketType::PEER_UPDATE;
    header->participantId = 0;
    header->channelCount = 2;
    header->sequenceNumber = 0;
    header->timestampUs = getSteadyClockUs();
    header->sampleCount = 0;
    header->payloadBytes = sizeof(PeerUpdatePayload);

    PeerUpdatePayload* payload = reinterpret_cast<PeerUpdatePayload*>(packet.data() + sizeof(PacketHeader));
    std::memset(payload, 0, sizeof(PeerUpdatePayload));
    payload->totalParticipants = static_cast<uint8_t>(getActivePeerCount());

    for (int i = 0; i < MAX_PARTICIPANTS; ++i)
    {
        payload->entries[i].participantId = static_cast<uint8_t>(i);
        payload->entries[i].isConnected = peers[i]->connected.load() ? 1 : 0;
        payload->entries[i].port = static_cast<uint16_t>(peers[i]->port);
        peers[i]->name.copyToUTF8(payload->entries[i].name, sizeof(payload->entries[i].name) - 1);
        peers[i]->roleOrInstrument.copyToUTF8(payload->entries[i].roleOrInstrument, sizeof(payload->entries[i].roleOrInstrument) - 1);
    }

    for (int i = 1; i < MAX_PARTICIPANTS; ++i)
    {
        if (peers[i]->connected.load() && !peers[i]->ipAddress.isEmpty() && peers[i]->port > 0)
        {
            udpSocket->write(peers[i]->ipAddress, peers[i]->port, packet.data(), static_cast<int>(packet.size()));
        }
    }
}

void LanAudioTransport::sendPingTo(const juce::String& ip, int port, uint8_t peerId)
{
    if (ip.isEmpty() || port <= 0 || udpSocket == nullptr)
        return;

    PacketHeader header;
    header.magic = PROTOCOL_MAGIC;
    header.version = PROTOCOL_VERSION;
    header.type = PacketType::PING;
    header.participantId = myParticipantId.load();
    header.channelCount = 2;
    header.sequenceNumber = ++pingSequenceNumber;
    header.timestampUs = getSteadyClockUs();
    header.sampleCount = 0;
    header.payloadBytes = sizeof(PingPayload);

    PingPayload payload;
    payload.pingSeq = pingSequenceNumber;
    payload.clientTimestampUs = header.timestampUs;
    payload.serverTimestampUs = 0;

    std::vector<uint8_t> packet(sizeof(PacketHeader) + sizeof(PingPayload));
    std::memcpy(packet.data(), &header, sizeof(header));
    std::memcpy(packet.data() + sizeof(header), &payload, sizeof(payload));

    udpSocket->write(ip, port, packet.data(), static_cast<int>(packet.size()));
}

void LanAudioTransport::sendPings()
{
    if (role.load() == SessionRole::HOST)
    {
        for (int i = 1; i < MAX_PARTICIPANTS; ++i)
        {
            if (peers[i]->connected.load())
            {
                sendPingTo(peers[i]->ipAddress, peers[i]->port, static_cast<uint8_t>(i));
            }
        }
    }
    else if (role.load() == SessionRole::CLIENT && connected.load())
    {
        sendPingTo(targetIp, targetPort, 0);
    }
}

void LanAudioTransport::checkTimeouts()
{
    auto nowMs = juce::Time::currentTimeMillis();

    if (role.load() == SessionRole::HOST)
    {
        bool changed = false;
        for (int i = 1; i < MAX_PARTICIPANTS; ++i)
        {
            if (peers[i]->connected.load())
            {
                if (nowMs - peers[i]->lastSeenMs.load() > 5000) // 5 second timeout
                {
                    peers[i]->connected.store(false);
                    peers[i]->jitterBuffer.reset();
                    peers[i]->stats.reset();
                    changed = true;
                }
            }
        }

        if (changed)
        {
            broadcastPeerUpdate();
            sendChangeMessage();
        }
    }
    else if (role.load() == SessionRole::CLIENT && connected.load())
    {
        if (nowMs - peers[0]->lastSeenMs.load() > 5000)
        {
            connected.store(false);
            peers[0]->connected.store(false);
            sendChangeMessage();
        }
    }
}

void LanAudioTransport::handleIncomingPacket(const uint8_t* data, int length, const juce::String& senderIp, int senderPort)
{
    if (length < static_cast<int>(sizeof(PacketHeader)))
        return;

    PacketHeader header;
    std::memcpy(&header, data, sizeof(header));

    if (header.magic != PROTOCOL_MAGIC || header.version != PROTOCOL_VERSION)
        return;

    uint64_t nowUs = getSteadyClockUs();
    auto nowMs = juce::Time::currentTimeMillis();

    switch (header.type)
    {
        case PacketType::JOIN_REQ:
            handleJoinRequest(data, length, senderIp, senderPort);
            break;

        case PacketType::JOIN_ACCEPT:
            handleJoinAccept(data, length);
            break;

        case PacketType::PEER_UPDATE:
            handlePeerUpdate(data, length);
            break;

        case PacketType::LEAVE_REQ:
        {
            if (role.load() == SessionRole::HOST)
            {
                for (int i = 1; i < MAX_PARTICIPANTS; ++i)
                {
                    if (peers[i]->connected.load() && peers[i]->ipAddress == senderIp && peers[i]->port == senderPort)
                    {
                        peers[i]->connected.store(false);
                        peers[i]->jitterBuffer.reset();
                        peers[i]->stats.reset();
                        broadcastPeerUpdate();
                        sendChangeMessage();
                        break;
                    }
                }
            }
            break;
        }

        case PacketType::PING:
        {
            if (length >= static_cast<int>(sizeof(PacketHeader) + sizeof(PingPayload)))
            {
                PingPayload inPing;
                std::memcpy(&inPing, data + sizeof(PacketHeader), sizeof(inPing));

                PacketHeader pongHeader;
                pongHeader.magic = PROTOCOL_MAGIC;
                pongHeader.version = PROTOCOL_VERSION;
                pongHeader.type = PacketType::PONG;
                pongHeader.participantId = myParticipantId.load();
                pongHeader.channelCount = 2;
                pongHeader.sequenceNumber = header.sequenceNumber;
                pongHeader.timestampUs = inPing.clientTimestampUs;
                pongHeader.sampleCount = 0;
                pongHeader.payloadBytes = sizeof(PingPayload);

                PingPayload pongPayload;
                pongPayload.pingSeq = inPing.pingSeq;
                pongPayload.clientTimestampUs = inPing.clientTimestampUs;
                pongPayload.serverTimestampUs = nowUs;

                std::vector<uint8_t> pongPacket(sizeof(PacketHeader) + sizeof(PingPayload));
                std::memcpy(pongPacket.data(), &pongHeader, sizeof(pongHeader));
                std::memcpy(pongPacket.data() + sizeof(pongHeader), &pongPayload, sizeof(pongPayload));

                udpSocket->write(senderIp, senderPort, pongPacket.data(), static_cast<int>(pongPacket.size()));
            }
            break;
        }

        case PacketType::PONG:
        {
            if (length >= static_cast<int>(sizeof(PacketHeader) + sizeof(PingPayload)))
            {
                PingPayload pong;
                std::memcpy(&pong, data + sizeof(PacketHeader), sizeof(pong));

                if (nowUs >= pong.clientTimestampUs)
                {
                    float rttMs = static_cast<float>(nowUs - pong.clientTimestampUs) / 1000.0f;
                    float inLat = hardwareInLatencyMs.load(std::memory_order_relaxed);
                    float outLat = hardwareOutLatencyMs.load(std::memory_order_relaxed);

                    primaryStats.recordRtt(rttMs, inLat, outLat);

                    // If sender is a peer, update their stats
                    int senderId = header.participantId;
                    if (senderId >= 0 && senderId < MAX_PARTICIPANTS)
                    {
                        peers[senderId]->stats.recordRtt(rttMs, inLat, outLat);
                        peers[senderId]->lastSeenMs.store(nowMs);
                    }
                }
            }
            break;
        }

        case PacketType::AUDIO_PCM16:
        {
            const int channels = header.channelCount;
            const int samples = header.sampleCount;
            const int expectedBytes = channels * samples * static_cast<int>(sizeof(int16_t));
            const int senderId = header.participantId;

            if (length >= static_cast<int>(sizeof(PacketHeader)) + expectedBytes &&
                samples > 0 && channels <= 2 &&
                senderId >= 0 && senderId < MAX_PARTICIPANTS)
            {
                auto& peer = peers[senderId];
                peer->lastSeenMs.store(nowMs);
                peer->stats.recordReceivedPacket(header.sequenceNumber, header.timestampUs, nowUs);
                primaryStats.recordReceivedPacket(header.sequenceNumber, header.timestampUs, nowUs);

                const int16_t* pcmData = reinterpret_cast<const int16_t*>(data + sizeof(PacketHeader));

                if (static_cast<int>(planarChannelBuffer0.size()) < samples)
                {
                    planarChannelBuffer0.resize(samples);
                    planarChannelBuffer1.resize(samples);
                }

                // De-interleave and convert 16-bit PCM to float [-1.0, 1.0]
                if (channels == 2)
                {
                    for (int i = 0; i < samples; ++i)
                    {
                        planarChannelBuffer0[i] = static_cast<float>(pcmData[i * 2]) / 32768.0f;
                        planarChannelBuffer1[i] = static_cast<float>(pcmData[i * 2 + 1]) / 32768.0f;
                    }
                }
                else
                {
                    for (int i = 0; i < samples; ++i)
                    {
                        float val = static_cast<float>(pcmData[i]) / 32768.0f;
                        planarChannelBuffer0[i] = val;
                        planarChannelBuffer1[i] = val;
                    }
                }

                const float* planes[2] = { planarChannelBuffer0.data(), planarChannelBuffer1.data() };
                peer->jitterBuffer.pushPacket(header.sequenceNumber, header.timestampUs, planes, 2, samples);

                // --- HOST RELAY ENGINE ---
                // If Host received audio from client i, immediately relay it to all other active clients j != i!
                if (role.load() == SessionRole::HOST && senderId >= 1)
                {
                    for (int j = 1; j < MAX_PARTICIPANTS; ++j)
                    {
                        if (j != senderId && peers[j]->connected.load() && !peers[j]->ipAddress.isEmpty() && peers[j]->port > 0)
                        {
                            udpSocket->write(peers[j]->ipAddress, peers[j]->port, data, length);
                        }
                    }
                }
            }
            break;
        }

        default:
            break;
    }
}

void LanAudioTransport::run()
{
#if JUCE_WINDOWS
    DWORD taskIndex = 0;
    AvSetMmThreadCharacteristicsW(L"Pro Audio", &taskIndex);
#endif

    std::vector<uint8_t> recvBuffer(2048);

    while (!threadShouldExit())
    {
        if (udpSocket != nullptr)
        {
            // 1. Drain incoming UDP datagrams
            while (true)
            {
                juce::String senderIp;
                int senderPort = 0;
                int bytesRead = udpSocket->read(recvBuffer.data(), static_cast<int>(recvBuffer.size()), false, senderIp, senderPort);

                if (bytesRead <= 0)
                    break;

                handleIncomingPacket(recvBuffer.data(), bytesRead, senderIp, senderPort);
            }
        }

        // 2. Transmit outgoing local audio
        if (isConnected() && udpSocket != nullptr)
        {
            int readySamples = sendRingBuffer.getNumReadyToRead();
            if (readySamples >= 64)
            {
                int samplesToSend = std::min(readySamples, 128);

                if (static_cast<int>(planarChannelBuffer0.size()) < samplesToSend)
                {
                    planarChannelBuffer0.resize(samplesToSend);
                    planarChannelBuffer1.resize(samplesToSend);
                }

                float* destPlanes[2] = { planarChannelBuffer0.data(), planarChannelBuffer1.data() };
                if (sendRingBuffer.readPlanar(destPlanes, 2, samplesToSend))
                {
                    int totalPcmSamples = samplesToSend * 2;
                    if (static_cast<int>(pcmConversionBuffer.size()) < totalPcmSamples)
                        pcmConversionBuffer.resize(totalPcmSamples);

                    for (int i = 0; i < samplesToSend; ++i)
                    {
                        float s0 = juce::jlimit(-1.0f, 1.0f, planarChannelBuffer0[i]);
                        float s1 = juce::jlimit(-1.0f, 1.0f, planarChannelBuffer1[i]);
                        pcmConversionBuffer[i * 2]     = static_cast<int16_t>(s0 * 32767.0f);
                        pcmConversionBuffer[i * 2 + 1] = static_cast<int16_t>(s1 * 32767.0f);
                    }

                    PacketHeader header;
                    header.magic = PROTOCOL_MAGIC;
                    header.version = PROTOCOL_VERSION;
                    header.type = PacketType::AUDIO_PCM16;
                    header.participantId = myParticipantId.load();
                    header.channelCount = 2;
                    header.sequenceNumber = ++sendSequenceNumber;
                    header.timestampUs = getSteadyClockUs();
                    header.sampleCount = static_cast<uint16_t>(samplesToSend);
                    header.payloadBytes = static_cast<uint16_t>(totalPcmSamples * sizeof(int16_t));

                    int totalPacketBytes = static_cast<int>(sizeof(PacketHeader)) + header.payloadBytes;
                    if (static_cast<int>(sendPacketBuffer.size()) < totalPacketBytes)
                        sendPacketBuffer.resize(totalPacketBytes);

                    std::memcpy(sendPacketBuffer.data(), &header, sizeof(header));
                    std::memcpy(sendPacketBuffer.data() + sizeof(header), pcmConversionBuffer.data(), header.payloadBytes);

                    // Send to targets
                    if (role.load() == SessionRole::HOST)
                    {
                        // Send Host's audio to all connected clients
                        for (int i = 1; i < MAX_PARTICIPANTS; ++i)
                        {
                            if (peers[i]->connected.load() && !peers[i]->ipAddress.isEmpty() && peers[i]->port > 0)
                            {
                                udpSocket->write(peers[i]->ipAddress, peers[i]->port, sendPacketBuffer.data(), totalPacketBytes);
                            }
                        }
                    }
                    else if (role.load() == SessionRole::CLIENT)
                    {
                        // Send client's audio to Host
                        if (!targetIp.isEmpty() && targetPort > 0)
                        {
                            udpSocket->write(targetIp, targetPort, sendPacketBuffer.data(), totalPacketBytes);
                        }
                    }

                    primaryStats.recordSentPacket();
                }
            }

            // 3. Periodic Ping probes (200 ms)
            auto nowMs = juce::Time::currentTimeMillis();
            if (nowMs - lastPingTimeMs >= 200)
            {
                lastPingTimeMs = nowMs;
                sendPings();
            }

            // 4. Timeout check (every 1000 ms)
            if (nowMs - lastTimeoutCheckMs >= 1000)
            {
                lastTimeoutCheckMs = nowMs;
                checkTimeouts();
            }
        }

        wait(1);
    }
}

} // namespace paasphoos
