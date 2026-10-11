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
    receiveRingBuffer.initialize(2, RING_BUFFER_CAPACITY);

    sendPacketBuffer.resize(4096, 0);
    pcmConversionBuffer.resize(2048, 0);
    planarChannelBuffer0.resize(1024, 0.0f);
    planarChannelBuffer1.resize(1024, 0.0f);
}

LanAudioTransport::~LanAudioTransport()
{
    disconnect();
}

bool LanAudioTransport::startAsHost(int port)
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
    connected.store(false);
    targetIp = "";
    targetPort = 0;
    sendSequenceNumber = 0;
    pingSequenceNumber = 0;
    lastPingTimeMs = 0;

    sendRingBuffer.reset();
    receiveRingBuffer.reset();
    stats.reset();

    startThread(juce::Thread::Priority::highest);
    sendChangeMessage();
    return true;
}

bool LanAudioTransport::startAsClient(const juce::String& hostIp, int hostPort)
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
    connected.store(false);
    sendSequenceNumber = 0;
    pingSequenceNumber = 0;
    lastPingTimeMs = 0;

    sendRingBuffer.reset();
    receiveRingBuffer.reset();
    stats.reset();

    // Send Join Request
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
    juce::String("Peer Musician").copyToUTF8(join.clientName, sizeof(join.clientName) - 1);
    juce::String("Guitar 2 / Vocals").copyToUTF8(join.roleOrInstrument, sizeof(join.roleOrInstrument) - 1);
    join.assignedId = 1;
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
    role.store(SessionRole::NONE);
    connected.store(false);

    if (udpSocket != nullptr)
        udpSocket->shutdown();

    stopThread(2000);
    udpSocket.reset();

    targetIp = "";
    targetPort = 0;
    stats.reset();
    sendChangeMessage();
}

juce::String LanAudioTransport::getRemoteAddress() const
{
    return targetIp;
}

int LanAudioTransport::getRemotePort() const
{
    return targetPort;
}

void LanAudioTransport::pushLocalAudio(const float* const* channelData, int numChannels, int numSamples) noexcept
{
    if (channelData != nullptr && numSamples > 0 && isConnected())
    {
        sendRingBuffer.writePlanar(channelData, numChannels, numSamples);
    }
}

void LanAudioTransport::pullRemoteAudio(float* const* destChannels, int numChannels, int numSamples) noexcept
{
    if (destChannels == nullptr || numSamples <= 0)
        return;

    if (!isConnected())
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            if (destChannels[ch] != nullptr)
                std::fill_n(destChannels[ch], numSamples, 0.0f);
        }
        return;
    }

    int ready = receiveRingBuffer.getNumReadyToRead();
    if (ready >= numSamples)
    {
        receiveRingBuffer.readPlanar(destChannels, numChannels, numSamples);
    }
    else
    {
        // Underflow / Starvation: zero fill
        for (int ch = 0; ch < numChannels; ++ch)
        {
            if (destChannels[ch] != nullptr)
                std::fill_n(destChannels[ch], numSamples, 0.0f);
        }
    }
}

void LanAudioTransport::sendPing()
{
    if (!isConnected() || targetIp.isEmpty() || targetPort <= 0 || udpSocket == nullptr)
        return;

    PacketHeader header;
    header.magic = PROTOCOL_MAGIC;
    header.version = PROTOCOL_VERSION;
    header.type = PacketType::PING;
    header.participantId = (role.load() == SessionRole::HOST) ? 0 : 1;
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

    udpSocket->write(targetIp, targetPort, packet.data(), static_cast<int>(packet.size()));
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

    switch (header.type)
    {
        case PacketType::JOIN_REQ:
        {
            if (role.load() == SessionRole::HOST)
            {
                targetIp = senderIp;
                targetPort = senderPort;
                connected.store(true);

                // Send JOIN_ACCEPT
                PacketHeader acceptHeader;
                acceptHeader.magic = PROTOCOL_MAGIC;
                acceptHeader.version = PROTOCOL_VERSION;
                acceptHeader.type = PacketType::JOIN_ACCEPT;
                acceptHeader.participantId = 0;
                acceptHeader.channelCount = 2;
                acceptHeader.sequenceNumber = 0;
                acceptHeader.timestampUs = nowUs;
                acceptHeader.sampleCount = 0;
                acceptHeader.payloadBytes = 0;

                udpSocket->write(targetIp, targetPort, &acceptHeader, sizeof(acceptHeader));
                sendChangeMessage();
            }
            break;
        }

        case PacketType::JOIN_ACCEPT:
        {
            if (role.load() == SessionRole::CLIENT)
            {
                connected.store(true);
                sendChangeMessage();
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
                pongHeader.participantId = (role.load() == SessionRole::HOST) ? 0 : 1;
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
                    stats.recordRtt(rttMs,
                                    hardwareInLatencyMs.load(std::memory_order_relaxed),
                                    hardwareOutLatencyMs.load(std::memory_order_relaxed));
                }
            }
            break;
        }

        case PacketType::AUDIO_PCM16:
        {
            const int channels = header.channelCount;
            const int samples = header.sampleCount;
            const int expectedBytes = channels * samples * static_cast<int>(sizeof(int16_t));

            if (length >= static_cast<int>(sizeof(PacketHeader)) + expectedBytes && samples > 0 && channels <= 2)
            {
                stats.recordReceivedPacket(header.sequenceNumber, header.timestampUs, nowUs);

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

                const float* dest[2] = { planarChannelBuffer0.data(), planarChannelBuffer1.data() };
                receiveRingBuffer.writePlanar(dest, 2, samples);
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
            // 1. Receive incoming datagrams
            juce::String senderIp;
            int senderPort = 0;
            int bytesRead = udpSocket->read(recvBuffer.data(), static_cast<int>(recvBuffer.size()), false, senderIp, senderPort);

            if (bytesRead > 0)
            {
                handleIncomingPacket(recvBuffer.data(), bytesRead, senderIp, senderPort);
            }
        }

        // 2. Transmit outgoing audio if connected
        if (connected.load() && !targetIp.isEmpty() && targetPort > 0 && udpSocket != nullptr)
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
                    // Interleave float into 16-bit PCM
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

                    // Build Audio Packet
                    PacketHeader header;
                    header.magic = PROTOCOL_MAGIC;
                    header.version = PROTOCOL_VERSION;
                    header.type = PacketType::AUDIO_PCM16;
                    header.participantId = (role.load() == SessionRole::HOST) ? 0 : 1;
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

                    udpSocket->write(targetIp, targetPort, sendPacketBuffer.data(), totalPacketBytes);
                    stats.recordSentPacket();
                }
            }

            // 3. Ping probe every 200 ms
            auto nowMs = juce::Time::currentTimeMillis();
            if (nowMs - lastPingTimeMs >= 200)
            {
                lastPingTimeMs = nowMs;
                sendPing();
            }
        }

        // Fast polling loop (~1 ms)
        wait(1);
    }
}

} // namespace paasphoos
