#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>
#include "../src/audio/RealtimeRingBuffer.h"
#include "../src/audio/Metronome.h"
#include "../src/audio/Tuner.h"
#include "../src/network/NetworkPacket.h"
#include "../src/network/NetworkStats.h"

void testRingBuffer()
{
    std::cout << "[TEST] Running AudioRingBuffer tests..." << std::endl;
    paasphoos::AudioRingBuffer<float> ring;
    ring.initialize(2, 256);

    assert(ring.getCapacity() == 256);
    assert(ring.getNumReadyToRead() == 0);
    assert(ring.getNumFreeSpace() == 255); // AbstractFifo reserves 1 slot to distinguish full/empty

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

    juce::AudioBuffer<float> buffer(2, 48000); // 1 second of audio = exactly 2 beats at 120 BPM
    buffer.clear();
    metro.process(buffer, 0, 48000);

    // Audio should contain click bursts
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

    // Synthesize 440 Hz test sine wave (Concert A4)
    std::vector<float> sine440(paasphoos::Tuner::ANALYSIS_BUFFER_SIZE);
    for (size_t i = 0; i < sine440.size(); ++i)
    {
        sine440[i] = 0.5f * std::sin(2.0 * 3.14159265358979323846 * 440.0 * (double)i / 48000.0);
    }

    tuner.pushSamples(sine440.data(), (int)sine440.size());
    tuner.updatePitchEstimation();

    auto result = tuner.getCurrentResult();
    assert(result.isActive);
    assert(result.noteNumber == 69); // MIDI note 69 is A4
    assert(std::abs(result.frequency - 440.0f) < 2.0f); // Within 2 Hz accuracy
    assert(std::abs(result.centsOffset) < 5.0f); // Within 5 cents

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
    // Total = In(1.5) + (RTT/2 = 2.0) + JitterBuf(2.0) + Out(1.5) = 7.0 ms
    assert(std::abs(stats.getTotalLatencyMs() - 7.0f) < 0.1f);

    // Simulate packet stream arrivals for jitter
    uint64_t baseSendUs = 1000000;
    uint64_t baseRecvUs = 1005000; // 5ms transit
    stats.recordReceivedPacket(1, baseSendUs, baseRecvUs);
    stats.recordReceivedPacket(2, baseSendUs + 2666, baseRecvUs + 2666 + 500); // 5.5ms transit (+0.5ms jitter)
    stats.recordReceivedPacket(3, baseSendUs + 5333, baseRecvUs + 5333 - 300); // 4.7ms transit

    assert(stats.getPacketsReceived() == 3);
    assert(stats.getJitterMs() >= 0.0f);

    std::cout << "  NetworkStats & Jitter passed! Total Latency: " << stats.getTotalLatencyMs() << " ms" << std::endl;
}

int main()
{
    std::cout << "===========================================" << std::endl;
    std::cout << "  Running PaasPhoos M1 + M2 Audio/Net Tests" << std::endl;
    std::cout << "===========================================" << std::endl;

    testRingBuffer();
    testMetronome();
    testTuner();
    testPacketSerialization();
    testNetworkStats();

    std::cout << "\nALL AUDIO AND NETWORK TESTS PASSED SUCCESSFULLY!" << std::endl;
    return 0;
}
