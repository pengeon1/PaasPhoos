#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <atomic>
#include <algorithm>

namespace paasphoos
{

/**
 * Lock-free Single-Producer Single-Consumer (SPSC) Audio Ring Buffer.
 * Uses juce::AbstractFifo internally to manage atomic indices without mutexes or dynamic memory allocations.
 */
template <typename SampleType = float>
class AudioRingBuffer
{
public:
    AudioRingBuffer() : fifo(1024), capacity(1024), numChannels(2)
    {
        buffer.resize(numChannels * capacity, SampleType(0));
    }

    void initialize(int channels, int sizeInSamples)
    {
        numChannels = std::max(1, channels);
        capacity = std::max(64, sizeInSamples);
        fifo.setTotalSize(capacity);
        buffer.assign(numChannels * capacity, SampleType(0));
        fifo.reset();
    }

    int getNumReadyToRead() const noexcept
    {
        return fifo.getNumReady();
    }

    int getNumFreeSpace() const noexcept
    {
        return fifo.getFreeSpace();
    }

    int getCapacity() const noexcept
    {
        return capacity;
    }

    void reset() noexcept
    {
        fifo.reset();
        std::fill(buffer.begin(), buffer.end(), SampleType(0));
    }

    /**
     * Real-time safe write. Multi-channel interleaved or planar.
     * Writes 'numSamples' across all channels.
     */
    bool writePlanar(const SampleType* const* channelData, int channelsToWrite, int numSamples) noexcept
    {
        if (numSamples <= 0 || channelsToWrite <= 0)
            return true;

        const int chans = std::min(numChannels, channelsToWrite);
        auto writer = fifo.write(numSamples);

        if (writer.blockSize1 > 0)
        {
            for (int ch = 0; ch < chans; ++ch)
            {
                if (channelData[ch] != nullptr)
                {
                    SampleType* dest = &buffer[ch * capacity + writer.startIndex1];
                    std::copy_n(channelData[ch], writer.blockSize1, dest);
                }
            }
        }

        if (writer.blockSize2 > 0)
        {
            for (int ch = 0; ch < chans; ++ch)
            {
                if (channelData[ch] != nullptr)
                {
                    SampleType* dest = &buffer[ch * capacity + writer.startIndex2];
                    std::copy_n(channelData[ch] + writer.blockSize1, writer.blockSize2, dest);
                }
            }
        }

        return (writer.blockSize1 + writer.blockSize2) == numSamples;
    }

    /**
     * Real-time safe read. Reads 'numSamples' into channel destination arrays.
     */
    bool readPlanar(SampleType* const* destData, int channelsToRead, int numSamples) noexcept
    {
        if (numSamples <= 0 || channelsToRead <= 0)
            return true;

        const int chans = std::min(numChannels, channelsToRead);
        auto reader = fifo.read(numSamples);

        if (reader.blockSize1 > 0)
        {
            for (int ch = 0; ch < chans; ++ch)
            {
                if (destData[ch] != nullptr)
                {
                    const SampleType* src = &buffer[ch * capacity + reader.startIndex1];
                    std::copy_n(src, reader.blockSize1, destData[ch]);
                }
            }
        }

        if (reader.blockSize2 > 0)
        {
            for (int ch = 0; ch < chans; ++ch)
            {
                if (destData[ch] != nullptr)
                {
                    const SampleType* src = &buffer[ch * capacity + reader.startIndex2];
                    std::copy_n(src, reader.blockSize2, destData[ch] + reader.blockSize1);
                }
            }
        }

        return (reader.blockSize1 + reader.blockSize2) == numSamples;
    }

private:
    juce::AbstractFifo fifo;
    int capacity;
    int numChannels;
    std::vector<SampleType> buffer;
};

} // namespace paasphoos
