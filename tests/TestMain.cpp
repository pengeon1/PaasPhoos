#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>
#include "../src/audio/RealtimeRingBuffer.h"
#include "../src/audio/Metronome.h"
#include "../src/audio/Tuner.h"

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

int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << "  Running PaasPhoos M1 Audio Test Suite " << std::endl;
    std::cout << "========================================" << std::endl;

    testRingBuffer();
    testMetronome();
    testTuner();

    std::cout << "\nALL M1 TESTS PASSED SUCCESSFULLY!" << std::endl;
    return 0;
}
