# PaasPhoos Verification & Testing Guide

This document details the test procedures, automated test suite verification, and manual acceptance testing steps for **PaasPhoos**.

---

## 1. Automated Test Suite Execution

To execute the automated regression test suite covering audio DSP, networking, jitter buffer management, packet loss concealment, drum synthesis, and Jam Code encoding:

```cmd
.\build\Release\PaasPhoosTests.exe
```

### Expected Output

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
  STUN Room Code ('JAM-C0A80169-AC0D') generated & decoded successfully!
[TEST] Running SetlistManager song library tests...
  SetlistManager songs, indexing & navigation passed!

===========================================
ALL 13 AUDIO, NETWORK, DSP, DRUMS & REHEARSAL TESTS PASSED!
===========================================
```

---

## 2. Audio Engine, Mixer & Monitoring Verification

1. **Application Launch**:
   - Start `PaasPhoos.exe`.
   - Verify the MMCSS indicator in the lower status bar displays `MMCSS: Pro Audio Active`.
2. **Audio Hardware Selection**:
   - In the **Audio Device & Hardware Settings** panel, switch between available buffer sizes (64, 128, 256 samples).
   - Verify the calculated round-trip buffer latency indicator updates accordingly.
3. **Local Monitoring**:
   - Connect an input signal (microphone or instrument) to Channel 1.
   - Verify audio level displays on the Channel 1 peak meter.
   - Confirm zero-latency local monitoring through the connected headphones.
4. **Mixer Controls**:
   - Verify that gain faders, pan sliders, mute (`M`), and solo (`S`) buttons operate deterministically.
   - Confirm that the clip indicator illuminates when the signal exceeds 0 dBFS and resets upon clicking `RST`.
5. **Metronome & Tuner**:
   - Enable the master metronome at 120 BPM; observe beat indicators.
   - Provide an instrument pitch to the input; confirm that the chromatic tuner identifies note and cents offset.

---

## 3. Real-Time Network & Headless Bot Testing

1. **Headless Bot Execution**:
   - Launch the test bot client to simulate a remote participant:
     ```cmd
     .\build\Release\PaasPhoosBot.exe --client 127.0.0.1 44045
     ```
2. **Session Hosting**:
   - In `PaasPhoos.exe`, click **START HOSTING**.
3. **Handshake Verification**:
   - Confirm the headless bot connects over UDP.
   - Verify that Channel Strip 3 (Peer 1) registers active audio metrics.
   - Verify that live network diagnostics (Ping, Jitter, Packet Loss, Ear-to-Ear Latency) display on the session HUD.

---

## 4. Multi-Peer Rehearsal & Jitter Buffer Stress Testing

1. **Multi-Peer Directory**:
   - Connect multiple client instances or test bots.
   - Verify that participant slots 1 through 5 reflect active names, instruments, and jitter buffer metrics.
2. **Clock Drift Correction & Packet Loss Concealment (PLC)**:
   - Introduce varying transmission latency.
   - Confirm that the Hermite cubic resampler corrects clock drift without inducing audible clicks, and that PLC conceals dropped packets.

---

## 5. DSP Rack & VST3 Plugin Hosting Verification

1. Open the **FX RACK & VST3** tab in PaasPhoos.
2. **Built-In DSP Chain**:
   - **Noise Gate**: Adjust threshold; confirm signal attenuation when input falls below the threshold.
   - **Tube Overdrive**: Increase drive amount; verify hyperbolic tangent harmonic saturation.
   - **Compressor**: Verify dynamic range reduction and auto-makeup gain.
   - **4-Band EQ**: Adjust frequency bands; confirm tonal modification.
   - **Plate Reverb**: Adjust wet mix; confirm spatial reverb tail.
   - Select presets (Clean Acoustic, Crunch Rhythm, High Gain Lead, Vocal Warm, Bright Hall); verify parameter updates.
3. **VST3 Hosting**:
   - Click **SCAN VST3**. Confirm installed 64-bit plugins in `C:\Program Files\Common Files\VST3` appear in the selector.
   - Select a plugin and click **LOAD**.
   - Click **OPEN GUI** to display the plugin's native editor window.

---

## 6. Synthesized Drum Machine & Sequencer Verification

1. Open the **DRUM MACHINE** tab.
2. **Trigger Pads**:
   - Click trigger pads or press keyboard hotkeys (`Z, X, C, V, A, S, D, F`).
   - Confirm immediate synthesis of Kick, Snare, Hi-Hats, Toms, Clap, and Dholak voices.
3. **Step Sequencer**:
   - Toggle step buttons on the 8x16 grid matrix.
   - Select groove styles (Rock 4-on-Floor, Pop Backbeat, Metal Double-Bass, Funk Ghost Notes, EDM House, Bollywood Teen-Taal).
   - Click **PLAY** to start sequenced playback synchronized with the master tempo.
   - Adjust **Swing** and verify natural rhythmic displacement.
   - Click **FILL** to trigger immediate bar-end fill patterns.
4. **MIDI Export**:
   - Click **EXPORT MIDI**.
   - Verify that a valid Standard MIDI File (`PaasPhoos_Drums.mid`) is written to the Desktop.

---

## 7. Internet Traversal via Jam Code & Network Simulation

1. **Jam Code Connection**:
   - Select **Internet (STUN)** mode and click **START HOSTING**.
   - Confirm the application resolves external mappings and generates a Jam Code (e.g., `JAM-XXXXXXXX-XXXX`).
   - On the client machine, paste the code and click **JOIN JAM**.
   - Confirm bidirectional audio streaming without requiring manual router port forwarding.
2. **Network Simulator**:
   - In the **SETLIST, REC & CHAT** tab, enable the network simulator.
   - Inject simulated latency (+25 ms) and packet loss (2%).
   - Verify that the jitter buffer adapts smoothly and the HUD displays updated metrics.

---

## 8. Multitrack WAV Recording, Setlist & Chat Verification

1. Open the **SETLIST, REC & CHAT** tab.
2. **Multitrack Stem Recording**:
   - Click **START REC (WAV)**; verify timer begins counting up.
   - Stream audio through local and remote channels.
   - Click **STOP REC**.
   - Navigate to the `PaasPhoos_Recordings` folder on the Desktop.
   - Confirm the presence of 24-bit WAV files for both the stereo master mix and individual stems:
     - `00_Master_Mix.wav`
     - `01_Local_Instrument.wav`
     - `02_Local_Vocal.wav`
     - `03_Remote_Peer1.wav` through `07_Remote_Peer5.wav`
     - `08_Drums.wav`
3. **Setlist Manager**:
   - Select songs in the setlist dropdown (Rock Classic, Bollywood Groove, Metal Jam, Funk Workout).
   - Verify that tempo and key parameters update the master metronome and drum sequencer.
4. **Chat & Push-to-Talk (PTT)**:
   - Enter text into the chat input and click **SEND**; confirm message appears in the chat log.
   - Toggle **TALK (PTT)**; confirm talkback voice override functionality.
