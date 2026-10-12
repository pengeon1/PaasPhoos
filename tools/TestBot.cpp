#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <thread>
#include <atomic>
#include <csignal>
#include <juce_core/juce_core.h>
#include "../src/network/NetworkPacket.h"
#include "../src/network/NetworkStats.h"

std::atomic<bool> isRunning{true};

void signalHandler(int)
{
    isRunning.store(false);
}

int main(int argc, char* argv[])
{
    std::signal(SIGINT, signalHandler);

    std::string targetIp = (argc > 1) ? argv[1] : "127.0.0.1";
    int targetPort = (argc > 2) ? std::stoi(argv[2]) : 44045;
    float toneFreq = (argc > 3) ? std::stof(argv[3]) : 330.0f; // E4 guitar note
    std::string botName = (argc > 4) ? argv[4] : "Band Bot";
    std::string botInstrument = (argc > 5) ? argv[5] : "Guitar 2";

    std::cout << "========================================================" << std::endl;
    std::cout << "  PaasPhoos Headless Test-Bot Client (M3 Band Simulator)" << std::endl;
    std::cout << "========================================================" << std::endl;
    std::cout << "[*] Name: " << botName << " (" << botInstrument << ")" << std::endl;
    std::cout << "[*] Target Host: " << targetIp << ":" << targetPort << std::endl;
    std::cout << "[*] Synthesizing acoustic test tone: " << toneFreq << " Hz" << std::endl;
    std::cout << "[*] Press Ctrl+C to terminate bot." << std::endl;

    juce::DatagramSocket socket(false);
    if (!socket.bindToPort(0))
    {
        std::cerr << "[ERROR] Could not bind client UDP socket!" << std::endl;
        return 1;
    }

    int localPort = socket.getBoundPort();
    std::cout << "[*] Bot bound to local ephemeral UDP port: " << localPort << std::endl;

    uint8_t myParticipantId = 1;

    // Send JOIN_REQ
    paasphoos::PacketHeader joinHeader;
    joinHeader.magic = paasphoos::PROTOCOL_MAGIC;
    joinHeader.version = paasphoos::PROTOCOL_VERSION;
    joinHeader.type = paasphoos::PacketType::JOIN_REQ;
    joinHeader.participantId = 1;
    joinHeader.channelCount = 2;
    joinHeader.sequenceNumber = 0;
    joinHeader.timestampUs = paasphoos::getSteadyClockUs();
    joinHeader.sampleCount = 0;
    joinHeader.payloadBytes = sizeof(paasphoos::JoinInfo);

    paasphoos::JoinInfo join;
    std::memset(&join, 0, sizeof(join));
    juce::String(botName).copyToUTF8(join.clientName, sizeof(join.clientName) - 1);
    juce::String(botInstrument).copyToUTF8(join.roleOrInstrument, sizeof(join.roleOrInstrument) - 1);
    join.assignedId = 0;
    join.assignedPort = static_cast<uint16_t>(localPort);

    std::vector<uint8_t> joinPacket(sizeof(paasphoos::PacketHeader) + sizeof(paasphoos::JoinInfo));
    std::memcpy(joinPacket.data(), &joinHeader, sizeof(joinHeader));
    std::memcpy(joinPacket.data() + sizeof(joinHeader), &join, sizeof(join));

    socket.write(targetIp, targetPort, joinPacket.data(), static_cast<int>(joinPacket.size()));
    std::cout << "[*] Sent JOIN_REQ to host. Streaming audio..." << std::endl;

    paasphoos::NetworkStats stats;
    uint32_t audioSeq = 0;
    uint32_t pingSeq = 0;
    double phase = 0.0;
    const double sampleRate = 48000.0;
    const int frameSamples = 128; // ~2.67 ms audio block
    const double phaseInc = (2.0 * 3.14159265358979323846 * toneFreq) / sampleRate;

    std::vector<uint8_t> recvBuffer(2048);
    std::vector<int16_t> pcmBuffer(frameSamples * 2);
    std::vector<uint8_t> sendPacket(sizeof(paasphoos::PacketHeader) + frameSamples * 2 * sizeof(int16_t));

    auto lastReportTime = std::chrono::steady_clock::now();
    auto lastPingTime = std::chrono::steady_clock::now();
    uint64_t totalFrames = 0;

    while (isRunning.load())
    {
        auto now = std::chrono::steady_clock::now();
        uint64_t nowUs = paasphoos::getSteadyClockUs();

        // 1. Synthesize audio frame with rhythmic amplitude envelope
        double beatPhase = std::fmod(static_cast<double>(totalFrames * frameSamples) / sampleRate, 0.5);
        float envelope = static_cast<float>(std::exp(-beatPhase * 12.0)); // Plucked decay envelope

        for (int i = 0; i < frameSamples; ++i)
        {
            float s = static_cast<float>(std::sin(phase)) * envelope * 0.6f;
            phase += phaseInc;
            if (phase >= 2.0 * 3.14159265358979323846) phase -= 2.0 * 3.14159265358979323846;

            int16_t pcmVal = static_cast<int16_t>(s * 32767.0f);
            pcmBuffer[i * 2]     = pcmVal;
            pcmBuffer[i * 2 + 1] = pcmVal;
        }

        // 2. Package and send Audio Packet
        paasphoos::PacketHeader header;
        header.magic = paasphoos::PROTOCOL_MAGIC;
        header.version = paasphoos::PROTOCOL_VERSION;
        header.type = paasphoos::PacketType::AUDIO_PCM16;
        header.participantId = myParticipantId;
        header.channelCount = 2;
        header.sequenceNumber = ++audioSeq;
        header.timestampUs = nowUs;
        header.sampleCount = static_cast<uint16_t>(frameSamples);
        header.payloadBytes = static_cast<uint16_t>(pcmBuffer.size() * sizeof(int16_t));

        std::memcpy(sendPacket.data(), &header, sizeof(header));
        std::memcpy(sendPacket.data() + sizeof(header), pcmBuffer.data(), header.payloadBytes);

        socket.write(targetIp, targetPort, sendPacket.data(), static_cast<int>(sendPacket.size()));
        stats.recordSentPacket();
        ++totalFrames;

        // 3. Send Ping periodically (every 250 ms)
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPingTime).count() >= 250)
        {
            lastPingTime = now;
            paasphoos::PacketHeader pingHdr;
            pingHdr.magic = paasphoos::PROTOCOL_MAGIC;
            pingHdr.version = paasphoos::PROTOCOL_VERSION;
            pingHdr.type = paasphoos::PacketType::PING;
            pingHdr.participantId = myParticipantId;
            pingHdr.channelCount = 2;
            pingHdr.sequenceNumber = ++pingSeq;
            pingHdr.timestampUs = nowUs;
            pingHdr.sampleCount = 0;
            pingHdr.payloadBytes = sizeof(paasphoos::PingPayload);

            paasphoos::PingPayload pingPayload;
            pingPayload.pingSeq = pingSeq;
            pingPayload.clientTimestampUs = nowUs;
            pingPayload.serverTimestampUs = 0;

            std::vector<uint8_t> pingPkt(sizeof(paasphoos::PacketHeader) + sizeof(paasphoos::PingPayload));
            std::memcpy(pingPkt.data(), &pingHdr, sizeof(pingHdr));
            std::memcpy(pingPkt.data() + sizeof(pingHdr), &pingPayload, sizeof(pingPayload));

            socket.write(targetIp, targetPort, pingPkt.data(), static_cast<int>(pingPkt.size()));
        }

        // 4. Poll incoming datagrams (audio/pong/join accept from host)
        juce::String senderIp;
        int senderPort = 0;
        int bytesRead = socket.read(recvBuffer.data(), static_cast<int>(recvBuffer.size()), false, senderIp, senderPort);

        if (bytesRead >= static_cast<int>(sizeof(paasphoos::PacketHeader)))
        {
            paasphoos::PacketHeader inHdr;
            std::memcpy(&inHdr, recvBuffer.data(), sizeof(inHdr));

            if (inHdr.magic == paasphoos::PROTOCOL_MAGIC)
            {
                if (inHdr.type == paasphoos::PacketType::JOIN_ACCEPT)
                {
                    myParticipantId = inHdr.participantId;
                    std::cout << "[*] Accepted by host! Assigned Participant Slot: " << (int)myParticipantId << std::endl;
                }
                else if (inHdr.type == paasphoos::PacketType::PONG &&
                         bytesRead >= static_cast<int>(sizeof(paasphoos::PacketHeader) + sizeof(paasphoos::PingPayload)))
                {
                    paasphoos::PingPayload pong;
                    std::memcpy(&pong, recvBuffer.data() + sizeof(paasphoos::PacketHeader), sizeof(pong));
                    float rttMs = static_cast<float>(nowUs - pong.clientTimestampUs) / 1000.0f;
                    stats.recordRtt(rttMs, 2.0f, 2.0f);
                }
                else if (inHdr.type == paasphoos::PacketType::AUDIO_PCM16)
                {
                    stats.recordReceivedPacket(inHdr.sequenceNumber, inHdr.timestampUs, nowUs);
                }
            }
        }

        // 5. Print live diagnostics report every 1 second
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastReportTime).count() >= 1000)
        {
            lastReportTime = now;
            std::cout << "[BOT " << (int)myParticipantId << " HUD] RTT: " << stats.getRttMs() << " ms | Jitter: "
                      << stats.getJitterMs() << " ms | Loss: " << stats.getPacketLossPercent()
                      << "% | Sent: " << stats.getPacketsSent()
                      << " | Recv: " << stats.getPacketsReceived()
                      << " | Total Latency: " << stats.getTotalLatencyMs() << " ms"
                      << std::endl;
        }

        // Accurate frame timing: 128 samples @ 48kHz = 2.667 ms
        std::this_thread::sleep_for(std::chrono::microseconds(2666));
    }

    // Send LEAVE_REQ before exit
    paasphoos::PacketHeader leaveHdr;
    leaveHdr.magic = paasphoos::PROTOCOL_MAGIC;
    leaveHdr.version = paasphoos::PROTOCOL_VERSION;
    leaveHdr.type = paasphoos::PacketType::LEAVE_REQ;
    leaveHdr.participantId = myParticipantId;
    leaveHdr.channelCount = 2;
    leaveHdr.sequenceNumber = 0;
    leaveHdr.timestampUs = paasphoos::getSteadyClockUs();
    leaveHdr.sampleCount = 0;
    leaveHdr.payloadBytes = 0;
    socket.write(targetIp, targetPort, &leaveHdr, sizeof(leaveHdr));

    std::cout << "\n[*] Test-Bot terminated cleanly." << std::endl;
    return 0;
}
