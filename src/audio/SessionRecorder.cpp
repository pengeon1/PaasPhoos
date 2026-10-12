#include "SessionRecorder.h"

namespace paasphoos
{

SessionRecorder::SessionRecorder()
    : juce::Thread("PaasPhoos_SessionRecorder")
{
    masterRing.initialize(2, FIFO_CAPACITY);
    for (int i = 0; i < MAX_RECORD_TRACKS; ++i)
    {
        trackRings[i].initialize(2, FIFO_CAPACITY);
    }
    readBufferL.resize(1024, 0.0f);
    readBufferR.resize(1024, 0.0f);
}

SessionRecorder::~SessionRecorder()
{
    stopRecording();
}

bool SessionRecorder::startRecording(const juce::File& folder, double sampleRate)
{
    stopRecording();

    currentSampleRate = (sampleRate > 1000.0) ? sampleRate : 48000.0;
    sessionFolder = folder;
    if (!sessionFolder.exists())
        sessionFolder.createDirectory();

    auto timestampStr = juce::Time::getCurrentTime().formatted("%Y-%m-%d_%H-%M-%S");
    juce::File runFolder = sessionFolder.getChildFile("Rehearsal_" + timestampStr);
    runFolder.createDirectory();

    // 1. Create Master Mix WAV Writer
    juce::File masterFile = runFolder.getChildFile("00_Master_Mix.wav");
    masterFile.deleteFile();
    auto masterStream = masterFile.createOutputStream();
    if (masterStream == nullptr)
        return false;

    masterWriter.reset(wavFormat.createWriterFor(
        masterStream.release(),
        currentSampleRate,
        2, // stereo
        24, // 24-bit PCM
        {},
        0
    ));

    // 2. Create Stems WAV Writers
    const char* trackNames[MAX_RECORD_TRACKS] = {
        "01_Local_Instrument.wav",
        "02_Local_Vocal.wav",
        "03_Peer_1.wav",
        "04_Peer_2.wav",
        "05_Peer_3.wav",
        "06_Peer_4.wav",
        "07_Peer_5.wav",
        "08_Drums.wav"
    };

    for (int i = 0; i < MAX_RECORD_TRACKS; ++i)
    {
        juce::File stemFile = runFolder.getChildFile(trackNames[i]);
        stemFile.deleteFile();
        auto stream = stemFile.createOutputStream();
        if (stream != nullptr)
        {
            trackWriters[i].reset(wavFormat.createWriterFor(
                stream.release(),
                currentSampleRate,
                2,
                24,
                {},
                0
            ));
        }
    }

    masterRing.reset();
    for (int i = 0; i < MAX_RECORD_TRACKS; ++i)
    {
        trackRings[i].reset();
    }

    recordStartTime = juce::Time::getCurrentTime();
    isRecordingActive.store(true, std::memory_order_relaxed);

    startThread(juce::Thread::Priority::normal);
    return true;
}

void SessionRecorder::stopRecording()
{
    isRecordingActive.store(false, std::memory_order_relaxed);
    stopThread(2000);

    // Flush remaining samples and close writers
    masterWriter.reset();
    for (int i = 0; i < MAX_RECORD_TRACKS; ++i)
    {
        trackWriters[i].reset();
    }
}

double SessionRecorder::getRecordingDurationSeconds() const noexcept
{
    if (!isRecordingActive.load(std::memory_order_relaxed))
        return 0.0;
    auto elapsed = juce::Time::getCurrentTime() - recordStartTime;
    return elapsed.inSeconds();
}

void SessionRecorder::pushMasterAudio(const float* const* channelData, int numChannels, int numSamples) noexcept
{
    if (isRecordingActive.load(std::memory_order_relaxed) && channelData != nullptr && numSamples > 0)
    {
        masterRing.writePlanar(channelData, numChannels, numSamples);
    }
}

void SessionRecorder::pushTrackAudio(int trackIndex, const float* const* channelData, int numChannels, int numSamples) noexcept
{
    if (isRecordingActive.load(std::memory_order_relaxed) && trackIndex >= 0 && trackIndex < MAX_RECORD_TRACKS && channelData != nullptr && numSamples > 0)
    {
        trackRings[trackIndex].writePlanar(channelData, numChannels, numSamples);
    }
}

void SessionRecorder::run()
{
    while (!threadShouldExit())
    {
        // 1. Drain Master buffer to file
        int ready = masterRing.getNumReadyToRead();
        if (ready >= 128)
        {
            int toRead = std::min(ready, 512);
            float* dest[2] = { readBufferL.data(), readBufferR.data() };
            if (masterRing.readPlanar(dest, 2, toRead) && masterWriter != nullptr)
            {
                const float* readPtrs[2] = { readBufferL.data(), readBufferR.data() };
                masterWriter->writeFromFloatArrays(readPtrs, 2, toRead);
            }
        }

        // 2. Drain Stems buffers to files
        for (int i = 0; i < MAX_RECORD_TRACKS; ++i)
        {
            int trackReady = trackRings[i].getNumReadyToRead();
            if (trackReady >= 128)
            {
                int toRead = std::min(trackReady, 512);
                float* dest[2] = { readBufferL.data(), readBufferR.data() };
                if (trackRings[i].readPlanar(dest, 2, toRead) && trackWriters[i] != nullptr)
                {
                    const float* readPtrs[2] = { readBufferL.data(), readBufferR.data() };
                    trackWriters[i]->writeFromFloatArrays(readPtrs, 2, toRead);
                }
            }
        }

        wait(5); // Drain every ~5ms
    }
}

} // namespace paasphoos
