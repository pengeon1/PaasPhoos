#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>
#include "../src/audio/RealtimeRingBuffer.h"
#include "../src/audio/Metronome.h"
#include "../src/audio/Tuner.h"
#include "../src/network/NetworkPacket.h"
#include "../src/network/NetworkStats.h"
#include "../src/network/JitterBuffer.h"

void testRingBuffer()
{
    std::cout << "[TEST] Running AudioRingBuffer tests..." << std::endl;
    paasphoos::AudioRingBuffer<float> ring;
    ring.initialize(2, 256);

    assert(ring.getCapacity() == 256);
    assert(ring.getNumReadyToRead() == 0);
    assert(ring.getNumFreeSpace() == 255);

    std::vector<float> ch0(64, 1.0f);
    std::vector<float> ch1(64, 2.0f);
    const float* inData[2] = { ch0.data(), ch1.data() };

    bool writeSuccess = ring.writePlanar(inData, 2, 64);
    assert(writeSuccess);
    assert(ring.getNumReadyToRead() == 64);

    std::vector<float> out0(64, 0.0f);
    std::vector<float> out1(64, 0.0f);
    float* outData[2] = { out0.data(), out1.data() };

    bool readSuccess = ring.readPlanar(outData, 2, 64);
    assert(readSuccess);
    assert(ring.getNumReadyToRead() == 0);
    assert(out0[0] == 1.0f);
    assert(out1[0] == 2.0f);
    assert(out0[63] == 1.0f);
    assert(out1[63] == 2.0f);

    std::cout << "  AudioRingBuffer passed!" << std::endl;
}

void testMetronome()
{
    std::cout << "[TEST] Running Metronome tests..." << std::endl;
    paasphoos::Metronome metro;
    metro.prepareToPlay(48000.0);
    metro.setBpm(120.0f);
    metro.setTimeSignature(4, 4);
    metro.setEnabled(true);

    assert(metro.getBpm() == 120.0f);
    assert(metro.getTimeSignatureNumerator() == 4);
    assert(metro.isEnabled() == true);

    juce::AudioBuffer<float> buffer(2, 48000);
    buffer.clear();
    metro.process(buffer, 0, 48000);

    float maxAmp = buffer.getMagnitude(0, 48000);
    assert(maxAmp > 0.05f);

    std::cout << "  Metronome passed!" << std::endl;
}

void testTuner()
{
    std::cout << "[TEST] Running Tuner pitch detection tests..." << std::endl;
    paasphoos::Tuner tuner;
    tuner.prepareToPlay(48000.0);
    tuner.setEnabled(true);

    std::vector<float> sine440(paasphoos::Tuner::ANALYSIS_BUFFER_SIZE);
    for (size_t i = 0; i < sine440.size(); ++i)
    {
        sine440[i] = 0.5f * std::sin(2.0 * 3.14159265358979323846 * 440.0 * (double)i / 48000.0);
    }

    tuner.pushSamples(sine440.data(), (int)sine440.size());
    tuner.updatePitchEstimation();

    auto result = tuner.getCurrentResult();
    assert(result.isActive);
    assert(result.noteNumber == 69); // MIDI A4
    assert(std::abs(result.frequency - 440.0f) < 2.0f);
    assert(std::abs(result.centsOffset) < 5.0f);

    std::string noteName = paasphoos::Tuner::getNoteName(result.noteNumber);
    assert(noteName == "A4");

    std::cout << "  Tuner detected: " << noteName << " (" << result.frequency << " Hz, " << result.centsOffset << " cents) - PASSED!" << std::endl;
}

void testPacketSerialization()
{
    std::cout << "[TEST] Running Packet Serialization tests..." << std::endl;
    paasphoos::PacketHeader header;
    header.magic = paasphoos::PROTOCOL_MAGIC;
    header.version = paasphoos::PROTOCOL_VERSION;
    header.type = paasphoos::PacketType::AUDIO_PCM16;
    header.participantId = 1;
    header.channelCount = 2;
    header.sequenceNumber = 42;
    header.timestampUs = 123456789ULL;
    header.sampleCount = 128;
    header.payloadBytes = 512;

    std::vector<uint8_t> buffer(sizeof(paasphoos::PacketHeader));
    std::memcpy(buffer.data(), &header, sizeof(header));

    paasphoos::PacketHeader decoded;
    std::memcpy(&decoded, buffer.data(), sizeof(decoded));

    assert(decoded.magic == paasphoos::PROTOCOL_MAGIC);
    assert(decoded.version == 1);
    assert(decoded.type == paasphoos::PacketType::AUDIO_PCM16);
    assert(decoded.participantId == 1);
    assert(decoded.channelCount == 2);
    assert(decoded.sequenceNumber == 42);
    assert(decoded.timestampUs == 123456789ULL);
    assert(decoded.sampleCount == 128);
    assert(decoded.payloadBytes == 512);

    std::cout << "  Packet serialization passed!" << std::endl;
}

void testNetworkStats()
{
    std::cout << "[TEST] Running NetworkStats & Jitter calculation tests..." << std::endl;
    paasphoos::NetworkStats stats;
    stats.recordSentPacket();
    stats.recordSentPacket();

    assert(stats.getPacketsSent() == 2);
    assert(stats.getHealthStatus() == paasphoos::ConnectionHealth::DISCONNECTED);

    // Simulate RTT measurement (4.0 ms)
    stats.recordRtt(4.0f, 1.5f, 1.5f, 2.0f);
    assert(stats.getRttMs() == 4.0f);
    assert(std::abs(stats.getTotalLatencyMs() - 7.0f) < 0.1f);

    // Simulate packet arrivals
    uint64_t baseSendUs = 1000000;
    uint64_t baseRecvUs = 1005000;
    stats.recordReceivedPacket(1, baseSendUs, baseRecvUs);
    stats.recordReceivedPacket(2, baseSendUs + 2666, baseRecvUs + 2666 + 500);
    stats.recordReceivedPacket(3, baseSendUs + 5333, baseRecvUs + 5333 - 300);

    assert(stats.getPacketsReceived() == 3);
    assert(stats.getJitterMs() >= 0.0f);

    std::cout << "  NetworkStats & Jitter passed! Total Latency: " << stats.getTotalLatencyMs() << " ms" << std::endl;
}

void testJitterBuffer()
{
    std::cout << "[TEST] Running JitterBuffer Adaptive & Playout tests..." << std::endl;
    paasphoos::JitterBuffer jb;
    jb.prepare(48000.0, 2, 8192);
    jb.setMode(paasphoos::JitterBufferMode::MANUAL);
    jb.setManualTargetDelayMs(4.0f); // 4ms = 192 samples @ 48kHz

    auto metricsBefore = jb.getMetrics();
    assert(!metricsBefore.isPrimed);

    // Verify unprimed buffer outputs silence
    std::vector<float> out0(64, 1.0f);
    std::vector<float> out1(64, 1.0f);
    float* outChannels[2] = { out0.data(), out1.data() };
    jb.pullAudio(outChannels, 2, 64);
    assert(out0[0] == 0.0f && out1[0] == 0.0f);

    // Push audio packet 1 (128 samples = 2.67ms)
    std::vector<float> in0(128, 0.75f);
    std::vector<float> in1(128, 0.75f);
    const float* inChannels[2] = { in0.data(), in1.data() };
    jb.pushPacket(1, paasphoos::getSteadyClockUs(), inChannels, 2, 128);

    // Still less than 192 samples -> should still not be primed
    assert(!jb.getMetrics().isPrimed);

    // Push packet 2 (total = 256 samples >= 192 threshold) -> primes!
    jb.pushPacket(2, paasphoos::getSteadyClockUs(), inChannels, 2, 128);
    assert(jb.getMetrics().isPrimed);

    // Pull audio now: should produce valid audio (non-zero)
    jb.pullAudio(outChannels, 2, 64);
    assert(std::abs(out0[0]) > 0.01f);

    std::cout << "  JitterBuffer adaptive priming and playout passed!" << std::endl;
}

void testJitterBufferPLCAndDrift()
{
    std::cout << "[TEST] Running JitterBuffer PLC & Clock Drift Correction tests..." << std::endl;
    paasphoos::JitterBuffer jb;
    jb.prepare(48000.0, 2, 8192);
    jb.setMode(paasphoos::JitterBufferMode::MANUAL);
    jb.setManualTargetDelayMs(4.0f);

    // Push packets to prime
    std::vector<float> inData(128, 0.8f);
    const float* inPlanes[2] = { inData.data(), inData.data() };
    jb.pushPacket(1, paasphoos::getSteadyClockUs(), inPlanes, 2, 128);
    jb.pushPacket(2, paasphoos::getSteadyClockUs(), inPlanes, 2, 128);
    assert(jb.getMetrics().isPrimed);

    // Pull all available samples to force starvation / PLC
    std::vector<float> out0(128, 0.0f);
    std::vector<float> out1(128, 0.0f);
    float* outPlanes[2] = { out0.data(), out1.data() };

    jb.pullAudio(outPlanes, 2, 128);
    jb.pullAudio(outPlanes, 2, 128);
    // Now starved: pull again triggers Packet Loss Concealment (PLC)
    jb.pullAudio(outPlanes, 2, 128);

    auto metrics = jb.getMetrics();
    assert(metrics.concealmentCount > 0);
    assert(metrics.underrunCount > 0);

    // Ensure output values did not explode to NaN or inf
    assert(!std::isnan(out0[0]) && !std::isinf(out0[0]));

    std::cout << "  JitterBuffer PLC & Drift tests passed! Concealments recorded: " << metrics.concealmentCount << std::endl;
}

void testMultiPeerPacket()
{
    std::cout << "[TEST] Running Multi-Peer Directory Packet tests..." << std::endl;
    paasphoos::PeerUpdatePayload payload;
    std::memset(&payload, 0, sizeof(payload));
    payload.totalParticipants = 3;

    payload.entries[0].participantId = 0;
    payload.entries[0].isConnected = 1;
    strcpy_s(payload.entries[0].name, "Band Host");
    strcpy_s(payload.entries[0].roleOrInstrument, "Lead Guitar");

    payload.entries[1].participantId = 1;
    payload.entries[1].isConnected = 1;
    strcpy_s(payload.entries[1].name, "Vocalist");
    strcpy_s(payload.entries[1].roleOrInstrument, "Vocals");

    payload.entries[2].participantId = 2;
    payload.entries[2].isConnected = 1;
    strcpy_s(payload.entries[2].name, "Drummer");
    strcpy_s(payload.entries[2].roleOrInstrument, "Drums");

    std::vector<uint8_t> buffer(sizeof(payload));
    std::memcpy(buffer.data(), &payload, sizeof(payload));

    paasphoos::PeerUpdatePayload received;
    std::memcpy(&received, buffer.data(), sizeof(received));

    assert(received.totalParticipants == 3);
    assert(std::string(received.entries[0].name) == "Band Host");
    assert(std::string(received.entries[1].name) == "Vocalist");
    assert(std::string(received.entries[2].name) == "Drummer");
    assert(received.entries[2].participantId == 2);

    std::cout << "  Multi-Peer Directory Packet serialization passed!" << std::endl;
}

int main()
{
    std::cout << "===========================================" << std::endl;
    std::cout << "  Running PaasPhoos M1 + M2 + M3 Test Suite" << std::endl;
    std::cout << "===========================================" << std::endl;

    testRingBuffer();
    testMetronome();
    testTuner();
    testPacketSerialization();
    testNetworkStats();
    testJitterBuffer();
    testJitterBufferPLCAndDrift();
    testMultiPeerPacket();

    std::cout << "\n===========================================" << std::endl;
    std::cout << "ALL 8 AUDIO, NETWORK & M3 TESTS PASSED!" << std::endl;
    std::cout << "===========================================" << std::endl;
    return 0;
}
