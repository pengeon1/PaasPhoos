# PaasPhoos Rehearsal Engine - Testing Guide & Verification

## Overview
This document contains the step-by-step verification procedures for each milestone of **PaasPhoos**. 

---

## Milestone 1 (M1) Test Checklist: Audio I/O + Mixer + Monitoring + Metronome

### Prerequisites
- Windows 10/11 PC.
- Audio input device (microphone, guitar plugged into audio interface, or laptop built-in mic).
- Audio output device (headphones or speakers - **headphones strongly recommended** to prevent feedback loops).
- (Optional) Hardware USB guitar pedal (Zoom, Boss, Line 6) or audio interface with ASIO or WASAPI support.

### Test Procedure

#### Test 1: Build & Launch
1. Open PowerShell or Command Prompt in the project folder.
2. Run `.\build.bat` or `.\build.ps1`.
3. Confirm build succeeds and produces `build\PaasPhoos_artefacts\Release\PaasPhoos.exe` (or `build\Release\PaasPhoos.exe`).
4. Launch `PaasPhoos.exe`.
5. **Expected result:** The dark studio console window appears with:
   - Header with Logo, "STANDALONE / M1 ENGINE", and live CPU % meter.
   - "AUDIO HARDWARE & DRIVER" panel with device selectors.
   - "MASTER METRONOME / CLICK" panel.
   - "LOCAL MONITORING & CHANNEL STRIPS" console (Ch 1 Guitar, Ch 2 Vocal, Master Monitor).
   - "CHROMATIC TUNER" widget with pitch gauge.
   - Bottom status bar showing MMCSS "Pro Audio Active" and active driver/buffer specs.

#### Test 2: Audio Device Selection & Buffer Latency Verification
1. In the **Audio Hardware & Driver** panel, observe the **Driver Type** dropdown (WASAPI / ASIO).
2. Select your desired Input device and Output device.
3. Switch **Buffer Size** between 64, 128, 256, and 512 samples.
4. **Expected result:**
   - The green **Buffer Latency Badge** dynamically recalculates:
     - 64 samples @ 48 kHz displays `~1.33 ms`.
     - 128 samples @ 48 kHz displays `~2.67 ms`.
     - 256 samples @ 48 kHz displays `~5.33 ms`.
   - Audio continues running smoothly without stutter or crashes.

#### Test 3: Zero-Latency Local Monitoring & Mixer Console
1. Connect headphones.
2. Strum an instrument on Channel 1 or speak into Channel 2 microphone.
3. Observe the vertical LED peak meters on the corresponding channel strip.
4. Adjust the **Fader** on Channel 1 / Channel 2:
   - Lower to `-inf dB`: Signal is silenced.
   - Raise to `0.0 dB`: Signal is heard at unity gain directly in headphones with zero perceptible latency.
5. Turn the **PAN** knob left (`L100`) and right (`R100`):
   - Sound pans accurately between left and right ears.
6. Press the red **MUTE** button:
   - Channel instantly mutes.
7. Press the amber **SOLO** button:
   - The selected channel plays, and the other channel is silenced.
8. Observe the **Master Monitor** meters:
   - Master stereo meters track the combined output signal.
   - If input exceeds 0 dBFS, the red **Clip LED** illuminates. Clicking **RST CLIP** clears the clip indicator.

#### Test 4: Sample-Accurate Metronome & Beat Visualization
1. Click **START CLICK** in the metronome bar.
2. **Expected result:**
   - Clear, rhythmic audio clicks play through the output mix.
   - Beat 1 (downbeat) produces an accented high-pitch click (1760 Hz / A6) with an amber/red flashing LED.
   - Beats 2, 3, 4 produce standard pitch clicks (880 Hz / A5) with cyan flashing LEDs.
3. Change BPM slider from 120 to 140 using the slider or `+` / `-` buttons.
   - Click tempo accelerates seamlessly.
4. Tap the **TAP** button 4 times in rhythm:
   - Metronome automatically syncs to your tapped tempo.
5. Switch **Time Signature** to 3/4 or 6/8:
   - Beat visualizer updates the number of dots (3 or 6 dots), and downbeat accent repeats accordingly.
6. Adjust the **Vol** slider: Click volume fades smoothly. Click **STOP CLICK** to stop.

#### Test 5: Chromatic Guitar & Vocal Tuner
1. In the **Chromatic Tuner** panel, select **Ch 1 (Guitar/Inst)** or **Ch 2 (Vocal/Aux)**.
2. Play open strings on guitar or sing a steady sustained note (e.g. A 440 Hz or open E2 string ~82.4 Hz).
3. **Expected result:**
   - The tuner immediately detects the fundamental pitch (e.g. `E2` or `A4`).
   - The center needle moves smoothly along the `-50b` to `+50#` scale.
   - When within ±3 cents of pitch, the note name glows vibrant emerald green and indicates "IN TUNE".
   - Exact frequency (Hz) and cent deviation are displayed with sub-Hz precision.
