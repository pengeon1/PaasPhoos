# PaasPhoos Rehearsal Engine - Testing Guide & Verification

## Overview
This document contains the step-by-step verification procedures for each milestone (M1 through M7) of **PaasPhoos**. 

---

## 1. Quick Automated Test Suite
To verify all audio DSP, networking, jitter buffer, packet loss concealment, drum synthesis, MIDI export, and STUN encoding algorithms in one command:
```powershell
.\build\Release\PaasPhoosTests.exe
```
Expected output:
```
===========================================
  Running PaasPhoos Full M1-M7 Test Suite
===========================================
[TEST] Running AudioRingBuffer tests...
  AudioRingBuffer passed!
[TEST] Running Metronome tests...
  Metronome passed!
[TEST] Running Tuner pitch detection tests...
  Tuner detected: A4 (440.017 Hz, 0.0679016 cents) - PASSED!
[TEST] Running Packet Serialization tests...
  Packet serialization passed!
[TEST] Running NetworkStats & Jitter calculation tests...
  NetworkStats & Jitter passed! Total Latency: 7 ms
[TEST] Running JitterBuffer Adaptive & Playout tests...
  JitterBuffer adaptive priming and playout passed!
[TEST] Running JitterBuffer PLC & Clock Drift Correction tests...
  JitterBuffer PLC & Drift tests passed! Concealments recorded: 1
[TEST] Running Multi-Peer Directory Packet tests...
  Multi-Peer Directory Packet serialization passed!
[TEST] Running BuiltInFxChain tests...
  BuiltInFxChain DSP & Presets passed!
[TEST] Running DrumEngine synthesis tests...
  DrumEngine synthesized voices & kits passed!
[TEST] Running DrumSequencer pattern & MIDI export tests...
  DrumSequencer grooves & Standard MIDI File export passed!
[TEST] Running STUN Room Code encoding & decoding tests...
  STUN Room Code ('PP-MTkyLjE2OC4xLjEwNTo0NDA0NQ==') generated & decoded successfully!
[TEST] Running SetlistManager song library tests...
  SetlistManager songs, indexing & navigation passed!

===========================================
ALL 13 AUDIO, NETWORK, DSP, DRUMS & REHEARSAL TESTS PASSED!
===========================================
```

---

## 2. Milestone 1 (M1) Test Checklist: Audio I/O + Mixer + Monitoring + Metronome
1. **Launch PaasPhoos**: Start `PaasPhoos.exe`. Confirm the MMCSS status badge displays `MMCSS: Pro Audio Active`.
2. **Audio Device Selection**: Switch buffer sizes (64, 128, 256 samples). Verify latency calculation badge updates immediately.
3. **Local Monitoring**: Speak into mic or play guitar. Observe channel peak meters and confirm zero-latency headphone monitoring.
4. **Mixer Controls**: Verify volume faders, mute, solo, and pan controls. Verify clip indicator lights up if input exceeds 0 dBFS.
5. **Metronome & Tuner**: Start metronome click at 120 BPM; observe accurate beats and LED flash. Pluck a guitar string and verify accurate chromatic pitch detection.

---

## 3. Milestone 2 (M2) Test Checklist: LAN Mode & Headless Bot
1. **Run Headless Bot**:
   ```cmd
   .\build\Release\PaasPhoosBot.exe --client 127.0.0.1 44045
   ```
2. **Host Session**: In `PaasPhoos.exe`, click **START HOSTING**.
3. **Verify Connection**:
   - The bot connects over UDP.
   - Channel Strip 3 (Remote Peer 1) begins lighting up with test tone audio.
   - Observe live RTT, Jitter, and Packet Loss stats on the HUD.
4. **Wi-Fi Warning**: If connected over Wi-Fi, verify the yellow Wi-Fi warning banner appears.

---

## 4. Milestone 3 (M3) Test Checklist: 6-Piece Band Rehearsal & Adaptive Jitter Buffer
1. **Multi-Peer Directory**: Host session and connect multiple instances or bots. Verify participant slots 1 through 5 populate with names and instruments.
2. **Clock Drift & PLC**: Test under varying network load. Verify that the Hermite resampler keeps audio smooth without pops or clicks, and PLC smoothly conceals occasional dropped packets.

---

## 5. Milestone 4 (M4) Test Checklist: Built-in DSP Rack & VST3 Plugin Hosting
1. Click the **FX RACK & VST3** tab in PaasPhoos.
2. **Built-in DSP Chain**:
   - Toggle **Noise Gate**: Adjust threshold; confirm ambient room noise is cut when not playing.
   - Toggle **Tube Overdrive**: Turn up drive amount; confirm warm tube harmonic saturation.
   - Toggle **Compressor**: Confirm punchy dynamic control and makeup gain.
   - Toggle **4-Band EQ**: Boost Highs and Bass; confirm tonal shaping.
   - Toggle **Plate Reverb**: Turn up wet slider; confirm lush spatial reverb.
   - Switch presets (Clean Acoustic, High Gain Lead, Vocal Hall) and confirm parameters update.
3. **VST3 Hosting**:
   - Click **SCAN VST3 PLUGINS**. Confirm installed plugins in `C:\Program Files\Common Files\VST3` populate the dropdown.
   - Select a plugin and click **LOAD VST3**.
   - Click **OPEN GUI** to display the plugin's native editor window.

---

## 6. Milestone 5 (M5) Test Checklist: Synthesized Drum Machine & Step Sequencer
1. Click the **DRUM MACHINE** tab in PaasPhoos.
2. **Live Drum Trigger Pads**: Click pads (or press keyboard hotkeys `Z, X, C, V, A, S, D, F`) to trigger Kick, Snare, HiHats, Toms, Clap, and Bollywood Dholak percussion.
3. **16-Step Sequencer Grid**:
   - Click grid cells to create custom beats.
   - Switch grooves (Rock Four on Floor, Pop Backbeat, Metal Double Bass, Funk, Bollywood Teen Taal).
   - Click **PLAY / STOP** to start playback synchronized with the master tempo.
   - Adjust **Swing** slider (0–60%) and **Humanize** toggle to hear natural groove feel.
   - Click **FILL NOW** to trigger an automated fill at bar boundaries.
4. **MIDI File Export**: Click **EXPORT MIDI PATTERN**. Select a file destination and verify a valid Standard MIDI File is written.

---

## 7. Milestone 6 (M6) Test Checklist: Internet Mode & Network Simulator
1. **STUN Room Code**:
   - Select **Internet Mode (STUN)** -> click **START HOSTING**.
   - Confirm PaasPhoos queries Google STUN (`stun.l.google.com:19302`) and displays a Room Code (e.g. `PP-MTky...`).
   - On the client, paste the Room Code into the direct connect box and click **DIRECT CONNECT**.
2. **Network Simulator**:
   - In **SETLIST, REC & CHAT** tab, inspect the **Network Simulator** controls.
   - Enable simulator, set **Artificial Latency** to 40 ms, **Jitter** to 10 ms, and **Loss** to 5%.
   - Verify that the adaptive jitter buffer absorbs the jitter without underruns, and the HUD reflects the simulated metrics.

---

## 8. Milestone 7 (M7) Test Checklist: Multitrack WAV Recording, Setlist & Chat
1. Click the **SETLIST, REC & CHAT** tab.
2. **Multitrack Stem Recording**:
   - Click **START RECORDING**. The recording timer begins counting up.
   - Play audio into Channel 1 and run the drum sequencer.
   - Click **STOP RECORDING**.
   - Navigate to `C:\Users\<user>\Music\PaasPhoos_Recordings\Session_<timestamp>\`.
   - Confirm the following 24-bit WAV files are present:
     - `00_Master_Mix.wav`
     - `01_Local_Instrument.wav`
     - `02_Local_Vocal.wav`
     - `03_Remote_Peer1.wav` ... `07_Remote_Peer5.wav`
     - `08_Drums.wav`
3. **Setlist Manager**:
   - Select songs in the setlist dropdown (Rock Classic, Bollywood Groove, Metal Jam, Funk Workout).
   - Confirm metronome and drum machine automatically update their BPM and key.
   - Edit song notes/chords and confirm changes persist.
4. **Chat & Push-to-Talk (PTT)**:
   - Type a message and click **SEND**; confirm message appears in chat log.
   - Press and hold **HOLD TO TALK (PTT)** to enable voice talkback override.
5. **End-to-End Latency Budget**:
   - Inspect the latency breakdown table at the bottom of the panel showing hardware input, DSP processing, network transit, jitter buffer depth, and hardware output summing to under 25 ms.
