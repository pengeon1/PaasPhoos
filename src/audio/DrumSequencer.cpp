#include "DrumSequencer.h"

namespace paasphoos
{

DrumSequencer::DrumSequencer(DrumEngine& engine)
    : drumEngine(engine)
{
    clearPattern();
    generateGroove(GrooveStyle::ROCK_FOUR_ON_FLOOR);
}

void DrumSequencer::prepare(double newSampleRate)
{
    sampleRate = (newSampleRate > 1000.0) ? newSampleRate : 48000.0;
    reset();
}

void DrumSequencer::reset()
{
    currentStep.store(0, std::memory_order_relaxed);
    stepSampleCounter = 0.0;
    barCounter = 0;
    accumulatedMidiTimeSeconds = 0.0;
}

void DrumSequencer::clearPattern()
{
    for (int t = 0; t < NUM_TRACKS; ++t)
    {
        for (int s = 0; s < NUM_STEPS; ++s)
        {
            grid[t][s].store(0.0f, std::memory_order_relaxed);
        }
    }
}

void DrumSequencer::setStep(int track, int step, float velocity)
{
    if (track >= 0 && track < NUM_TRACKS && step >= 0 && step < NUM_STEPS)
    {
        grid[track][step].store(juce::jlimit(0.0f, 1.0f, velocity), std::memory_order_relaxed);
    }
}

float DrumSequencer::getStep(int track, int step) const
{
    if (track >= 0 && track < NUM_TRACKS && step >= 0 && step < NUM_STEPS)
    {
        return grid[track][step].load(std::memory_order_relaxed);
    }
    return 0.0f;
}

void DrumSequencer::toggleStep(int track, int step)
{
    if (track >= 0 && track < NUM_TRACKS && step >= 0 && step < NUM_STEPS)
    {
        float current = grid[track][step].load(std::memory_order_relaxed);
        grid[track][step].store((current > 0.05f) ? 0.0f : 0.85f, std::memory_order_relaxed);
    }
}

void DrumSequencer::generateGroove(GrooveStyle style, float intensity)
{
    clearPattern();
    float vel = juce::jlimit(0.5f, 1.0f, intensity);

    switch (style)
    {
        case GrooveStyle::ROCK_FOUR_ON_FLOOR:
            // Kick on 0, 4, 8, 12
            setStep((int)DrumSound::KICK, 0, vel);
            setStep((int)DrumSound::KICK, 4, vel * 0.9f);
            setStep((int)DrumSound::KICK, 8, vel);
            setStep((int)DrumSound::KICK, 12, vel * 0.9f);
            // Snare on 4, 12 (backbeat)
            setStep((int)DrumSound::SNARE, 4, vel * 1.0f);
            setStep((int)DrumSound::SNARE, 12, vel * 1.0f);
            // Hihat 8th notes on all even steps
            for (int s = 0; s < 16; s += 2)
                setStep((int)DrumSound::HIHAT_CLOSED, s, vel * 0.75f);
            setStep((int)DrumSound::HIHAT_OPEN, 14, vel * 0.8f);
            break;

        case GrooveStyle::POP_BACKBEAT:
            setStep((int)DrumSound::KICK, 0, vel);
            setStep((int)DrumSound::KICK, 6, vel * 0.85f);
            setStep((int)DrumSound::KICK, 10, vel * 0.9f);
            setStep((int)DrumSound::SNARE, 4, vel);
            setStep((int)DrumSound::SNARE, 12, vel);
            setStep((int)DrumSound::CLAP, 4, vel * 0.6f);
            setStep((int)DrumSound::CLAP, 12, vel * 0.6f);
            for (int s = 0; s < 16; s += 2)
                setStep((int)DrumSound::HIHAT_CLOSED, s, vel * 0.7f);
            break;

        case GrooveStyle::METAL_DOUBLE_BASS:
            // Continuous fast double kick
            for (int s = 0; s < 16; ++s)
                setStep((int)DrumSound::KICK, s, (s % 2 == 0) ? vel : vel * 0.85f);
            setStep((int)DrumSound::SNARE, 4, vel * 1.0f);
            setStep((int)DrumSound::SNARE, 12, vel * 1.0f);
            for (int s = 0; s < 16; s += 2)
                setStep((int)DrumSound::HIHAT_OPEN, s, vel * 0.8f);
            break;

        case GrooveStyle::FUNK_GHOST_NOTES:
            setStep((int)DrumSound::KICK, 0, vel);
            setStep((int)DrumSound::KICK, 7, vel * 0.8f);
            setStep((int)DrumSound::KICK, 10, vel * 0.9f);
            setStep((int)DrumSound::SNARE, 4, vel);
            setStep((int)DrumSound::SNARE, 12, vel);
            // Ghost snares
            setStep((int)DrumSound::SNARE, 2, vel * 0.35f);
            setStep((int)DrumSound::SNARE, 7, vel * 0.40f);
            setStep((int)DrumSound::SNARE, 15, vel * 0.35f);
            for (int s = 0; s < 16; ++s)
                setStep((int)DrumSound::HIHAT_CLOSED, s, (s % 2 == 0) ? vel * 0.8f : vel * 0.5f);
            break;

        case GrooveStyle::EDM_HOUSE:
            for (int s = 0; s < 16; s += 4)
                setStep((int)DrumSound::KICK, s, vel);
            setStep((int)DrumSound::CLAP, 4, vel);
            setStep((int)DrumSound::CLAP, 12, vel);
            for (int s = 2; s < 16; s += 4) // Offbeat open hats
                setStep((int)DrumSound::HIHAT_OPEN, s, vel * 0.85f);
            break;

        case GrooveStyle::BOLLYWOOD_TEEN_TAAL:
            // Dholak / Indian hand percussion groove
            setStep((int)DrumSound::PERCUSSION_DHOLAK, 0, vel * 1.0f);
            setStep((int)DrumSound::PERCUSSION_DHOLAK, 2, vel * 0.7f);
            setStep((int)DrumSound::PERCUSSION_DHOLAK, 4, vel * 0.9f);
            setStep((int)DrumSound::PERCUSSION_DHOLAK, 6, vel * 0.6f);
            setStep((int)DrumSound::PERCUSSION_DHOLAK, 8, vel * 1.0f);
            setStep((int)DrumSound::PERCUSSION_DHOLAK, 10, vel * 0.75f);
            setStep((int)DrumSound::PERCUSSION_DHOLAK, 12, vel * 0.9f);
            setStep((int)DrumSound::PERCUSSION_DHOLAK, 14, vel * 0.65f);
            setStep((int)DrumSound::KICK, 0, vel * 0.8f);
            setStep((int)DrumSound::KICK, 8, vel * 0.8f);
            break;
    }
}

void DrumSequencer::triggerFill()
{
    // Fast fill on steps 12..15 across snare and toms
    setStep((int)DrumSound::SNARE, 12, 0.9f);
    setStep((int)DrumSound::SNARE, 13, 0.95f);
    setStep((int)DrumSound::TOM_HIGH, 14, 0.95f);
    setStep((int)DrumSound::TOM_LOW, 15, 1.0f);
}

void DrumSequencer::triggerStep(int stepIndex)
{
    bool doHumanize = humanize.load(std::memory_order_relaxed);

    for (int t = 0; t < NUM_TRACKS; ++t)
    {
        float vel = grid[t][stepIndex].load(std::memory_order_relaxed);
        if (vel > 0.05f)
        {
            if (doHumanize)
            {
                // Randomize velocity +/- 8%
                float rnd = (static_cast<float>(rand() % 100) / 100.0f - 0.5f) * 0.16f;
                vel = juce::jlimit(0.1f, 1.0f, vel + rnd);
            }

            drumEngine.triggerVoice(static_cast<DrumSound>(t), vel);

            // Record MIDI Note for export (Standard GM Drum Map)
            int midiNote = 36; // Kick = 36
            if (t == 1) midiNote = 38; // Snare
            else if (t == 2) midiNote = 42; // Closed HH
            else if (t == 3) midiNote = 46; // Open HH
            else if (t == 4) midiNote = 45; // Low Tom
            else if (t == 5) midiNote = 50; // High Tom
            else if (t == 6) midiNote = 39; // Clap
            else if (t == 7) midiNote = 60; // Percussion

            auto noteOn = juce::MidiMessage::noteOn(10, midiNote, static_cast<uint8_t>(vel * 127.0f));
            noteOn.setTimeStamp(accumulatedMidiTimeSeconds);
            recordedMidi.addEvent(noteOn);

            auto noteOff = juce::MidiMessage::noteOff(10, midiNote);
            noteOff.setTimeStamp(accumulatedMidiTimeSeconds + 0.1);
            recordedMidi.addEvent(noteOff);
        }
    }
}

void DrumSequencer::processBlock(int numSamples)
{
    if (!isPlaying.load(std::memory_order_relaxed) || numSamples <= 0)
        return;

    float bpm = currentBpm.load(std::memory_order_relaxed);
    double samplesPerBeat = (sampleRate * 60.0) / static_cast<double>(bpm);
    double samplesPer16th = samplesPerBeat / 4.0; // 16th note step duration

    stepSampleCounter += numSamples;
    accumulatedMidiTimeSeconds += static_cast<double>(numSamples) / sampleRate;

    // Apply swing offset: on odd steps (1, 3, 5, etc.), delay the trigger
    int step = currentStep.load(std::memory_order_relaxed);
    float sw = swing.load(std::memory_order_relaxed);
    double targetSamples = (step % 2 == 1) ? samplesPer16th * (1.0 + sw * 0.5) : samplesPer16th;

    while (stepSampleCounter >= targetSamples)
    {
        stepSampleCounter -= targetSamples;
        triggerStep(step);

        step = (step + 1) % NUM_STEPS;
        currentStep.store(step, std::memory_order_relaxed);

        if (step == 0)
        {
            barCounter++;
        }

        targetSamples = (step % 2 == 1) ? samplesPer16th * (1.0 + sw * 0.5) : samplesPer16th;
    }
}

bool DrumSequencer::exportMidiFile(const juce::File& destinationFile)
{
    if (recordedMidi.getNumEvents() == 0)
    {
        // If not recorded live yet, populate from current 16-step pattern
        float bpm = currentBpm.load(std::memory_order_relaxed);
        double secondsPer16th = 60.0 / (bpm * 4.0);
        for (int bar = 0; bar < 4; ++bar)
        {
            for (int s = 0; s < NUM_STEPS; ++s)
            {
                double timeSec = (bar * NUM_STEPS + s) * secondsPer16th;
                for (int t = 0; t < NUM_TRACKS; ++t)
                {
                    float vel = grid[t][s].load();
                    if (vel > 0.05f)
                    {
                        int note = 36 + t;
                        auto on = juce::MidiMessage::noteOn(10, note, static_cast<uint8_t>(vel * 127.0f));
                        on.setTimeStamp(timeSec);
                        recordedMidi.addEvent(on);

                        auto off = juce::MidiMessage::noteOff(10, note);
                        off.setTimeStamp(timeSec + 0.08);
                        recordedMidi.addEvent(off);
                    }
                }
            }
        }
    }

    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote(960);
    midiFile.addTrack(recordedMidi);

    destinationFile.deleteFile();
    juce::FileOutputStream output(destinationFile);
    if (output.openedOk())
    {
        midiFile.writeTo(output);
        return true;
    }
    return false;
}

} // namespace paasphoos
