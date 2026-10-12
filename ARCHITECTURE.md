# PaasPhoos Architecture & Technical Design

## 1. Overview
**PaasPhoos** is a zero-budget, high-performance, open-source Windows 10/11 desktop application tailored for real-time remote music rehearsals. It accommodates a standard 6-piece band (1 vocalist, 1 keyboardist, 3 guitarists, 1 drummer) over Local Area Networks (LAN) and the public Internet with minimum acoustic and network latency.

---

## 2. Technology Stack & Justification

| Layer | Technology | Justification |
| :--- | :--- | :--- |
| **Framework & Audio Engine** | **C++17 + JUCE 8 (GPLv3)** | Standard in professional audio software. Native support for Windows WASAPI & ASIO, VST3 plugin hosting, DSP filters, MIDI processing, high-DPI rendering, and real-time safe threading abstractions (`AbstractFifo`). |
| **Driver Layer** | **Steinberg ASIO SDK + Windows WASAPI Fallback** | Sub-5ms audio hardware buffers require ASIO. WASAPI Exclusive mode acts as an out-of-the-box fallback when no dedicated ASIO hardware driver is installed. Free virtual drivers (ASIO4ALL, FlexASIO) are supported seamlessly. |
| **Network Audio Codec** | **Uncompressed PCM (LAN) & libopus (Internet)** | LAN uses raw uncompressed linear 16-bit or 24-bit float PCM (0 ms codec delay). Internet uses Opus with 2.5 ms or 5 ms frame sizes (64–256 kbps), offering low latency, built-in FEC, and robust packet loss concealment (PLC). |
| **Networking Architecture** | **In-App Peer-Host Hybrid (UDP/STUN)** | One band member hosts the session inside the same desktop application (no dedicated server daemon or cloud VM costs). UDP sockets are used for audio streaming with custom sequence-numbered headers. LAN discovery uses UDP broadcast; Internet traversal uses RFC 5389 UDP hole punching (STUN) or virtual mesh LANs (Tailscale / ZeroTier). |
| **Built-in DSP Effects** | **JUCE DSP Module (`juce_dsp`)** | 5-stage real-time DSP chain (Noise Gate, Tube Overdrive, Compressor, 4-Band Parametric EQ, Plate Reverb) with lock-free atomic parameter modulation and pre-allocated delay buffers. |
| **Plugin Hosting** | **JUCE VST3 AudioProcessorGraph** | Dynamic scanning of `C:\Program Files\Common Files\VST3`, non-blocking plugin instantiation, and native HWND/JUCE plugin editor window hosting. |
| **Drum Synthesis & Sequencer** | **Direct DSP Oscillator/Noise + JUCE Midi** | 8 real-time synthesized voices (including physical modeled Indian Bollywood Dholak), 16-step grid sequencer with swing, humanize, fill generator, live trigger pads, and Standard MIDI File (.mid) export. |
| **Multitrack Recording** | **Lock-Free Background Disk Writer** | High-priority audio callback copies blocks into a lock-free ring buffer; dedicated background worker writes 24-bit WAV stems (Master + 8 individual tracks) without stalling the audio thread. |
| **Concurrency & Real-Time Safety** | **Lock-Free Ring Buffers + MMCSS** | Zero locks (`std::mutex`), zero dynamic allocations (`malloc`/`new`), and zero file/network I/O on the high-priority audio callback thread. MMCSS (`AvSetMmThreadCharacteristicsW`) ensures Windows assigns maximum priority to audio and network queues. |

---

## 3. High-Level Architectural Diagram

```
+----------------------------------------------------------------------------------------------------+
|                                         GUI THREAD (JUCE)                                          |
|   - Multi-View Studio: [Mixer & Tuner] [FX Rack & VST3] [Drum Machine] [Setlist, Rec & Chat]       |
|   - Live Peak Meters & Clip Reset, Chromatic Tuner Needle, 8x16 Step Sequencer Matrix LED          |
+--------------------------------------------------+-------------------------------------------------+
                                                   | (Lock-free UI queue & atomics)
                                                   v
+--------------------------------------------------+-------------------------------------------------+
|                                        AUDIO ENGINE THREAD                                         |
|  [Hardware Input] (ASIO / WASAPI)                                                                  |
|         |                                                                                          |
|         +---> [Built-in DSP FX / VST3 Host] ---> [Local Zero-Latency Monitor] ---> [Hardware Out]  |
|         |                                                                             ^            |
|         +---> [Drum Engine Synthesis] ------------------------------------------------+            |
|         |                                                                             |            |
|         +---> [Multitrack WAV Recorder Ring Buffer]                                    |            |
|         |                                                                             |            |
|         +---> [Network Send Ring Buffer]                                              |            |
|                                                                                       |            |
|  [Hardware Out] <--- [Local Master Mix Summer] <--- [Lock-Free Playback Ring Buffer] -+            |
+--------------------------------------------------+-------------------------------------------------+
                          |                                                ^
                          v                                                |
+-------------------------+------------------------------------------------+-------------------------+
|                                    NETWORK WORKER THREAD(S)                                        |
|                                                                                                    |
|  [Send Queue] ---> [PCM / Opus Encoder] ---> [UDP Socket Transmitter] ---> [Host / Peers]          |
|                                                                                                    |
|  [UDP Receiver] ---> [Network Simulator] ---> [Jitter Buffer + Hermite Resampler] ---> Recv Ring   |
+----------------------------------------------------------------------------------------------------+
```

---

## 4. Subsystem Details

### 4.1 Real-Time Audio Engine
1. **Audio Callback (`juce::AudioIODeviceCallback`)**:
   - Reads hardware input samples (Channel 1: Instrument/Guitar, Channel 2: Vocal/Mic).
   - Feeds local DSP / VST3 effects chain (Noise Gate -> Compressor -> EQ -> Overdrive -> Reverb).
   - Routes wet/dry signal directly to local monitoring mixer (0 network delay).
   - Sums synthesized drum engine output if playing.
   - Pushes processed channels into the network send ring buffer and multitrack recording buffer.
   - Reads remote participants' audio from lock-free jitter buffers, applies per-channel gain/pan, sums into master mix, and outputs to headphones.

### 4.2 Network & Transport Layer
1. **Uncompressed PCM Mode (LAN)**:
   - 16-bit linear PCM @ 48 kHz. 128 samples per packet (2.67 ms).
   - 0 ms codec delay, minimal CPU overhead.
2. **In-App Star Relay (Host-Client)**:
   - Band Host listens on UDP 44045.
   - Host receives audio from all 5 remote peers and forwards to the others, acting as a lightweight zero-latency software reflector.
3. **Adaptive Jitter Buffer & Clock Drift Resampler**:
   - Continuous RFC 3550 jitter calculation (`D = (R_j - S_j) - (R_i - S_i)`).
   - Dynamically adjusts priming depth between minimum buffer size and safety threshold.
   - Cubic Hermite polynomial interpolation resamples remote audio to match local hardware clock, eliminating clicks without pitch artifacts.
   - Packet Loss Concealment (PLC) repeats recent pitch periods with progressive 3 dB exponential decay on dropped packets.

### 4.3 VST3 Hosting & Built-In DSP
- **Built-In DSP Rack**: Zero-dependency C++17 implementations of Noise Gate, Tube Overdrive (hyperbolic tangent waveshaping), Feed-forward Compressor with soft knee, 4-Band Parametric EQ (JUCE IIR biquad filters), and Plate Reverb.
- **VST3 Host**: Scans `C:\Program Files\Common Files\VST3`, instantiates plugins asynchronously, and manages parameter serialization.

### 4.4 Drum Machine
- **8 Synthesis Voices**: Kick (frequency drop sine + punch noise), Snare (tone + shaped noise), Closed Hat, Open Hat, Low/High Toms (pitched resonators), Clap (multi-burst envelope), and Indian Bollywood Dholak percussion (low bayan bass pitch bend + high dayan slap).
- **Sequencer**: 16-step grid, swing calculation (0–60%), humanized velocity/timing offsets, fill generator, and standard MIDI file generation.

### 4.5 Multitrack WAV Recorder & Setlist Manager
- **Multitrack Stem Recording**: Lock-free ring buffer streams 9 separate channels (`00_Master_Mix.wav` plus individual stems for each band member and drums) to a dedicated disk I/O thread in 24-bit WAV format.
- **Setlist Manager**: Manages song titles, keys, BPMs, and chord charts/lyrics; automatically configures the metronome and drum sequencer when a song is selected.
