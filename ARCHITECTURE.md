# PaasPhoos Architecture & Technical Design

## 1. Overview
**PaasPhoos** is a zero-budget, high-performance, open-source Windows 10/11 desktop application tailored for real-time remote music rehearsals. It accommodates a standard 6-piece band (1 vocalist, 1 keyboardist, 3 guitarists, 1 drummer) over Local Area Networks (LAN) and the public Internet with minimum acoustic and network latency.

---

## 2. Technology Stack & Justification

| Layer | Technology | Justification |
| :--- | :--- | :--- |
| **Framework & Audio Engine** | **C++17 + JUCE 8 (GPLv3)** | Standard in professional audio software. Native support for Windows WASAPI & ASIO, VST3 plugin hosting, DSP filters, MIDI processing, high-DPI rendering, and real-time safe threading abstractions (`AbstractFifo`). |
| **Driver Layer** | **Steinberg ASIO SDK + Windows WASAPI Fallback** | Sub-5ms audio hardware buffers requires ASIO. WASAPI Exclusive mode acts as an out-of-the-box fallback when no dedicated ASIO hardware driver is installed. Free virtual drivers (ASIO4ALL, FlexASIO) are supported seamlessly. |
| **Network Audio Codec** | **Uncompressed PCM (LAN) & libopus (Internet)** | LAN uses raw uncompressed linear 16-bit or 24-bit float PCM (0 ms codec delay). Internet uses Opus with 2.5 ms or 5 ms frame sizes (64–256 kbps), offering low latency, built-in FEC, and robust packet loss concealment (PLC). |
| **Networking Architecture** | **In-App Peer-Host Hybrid (UDP/STUN)** | One band member hosts the session inside the same desktop application (no dedicated server daemon or cloud VM costs). UDP sockets are used for audio streaming with custom sequence-numbered headers. LAN discovery uses UDP broadcast; Internet traversal uses UDP hole punching (STUN) or virtual mesh LANs (Tailscale / ZeroTier). |
| **Concurrency & Real-Time Safety** | **Lock-Free SPSC/MPMC Ring Buffers + MMCSS** | Zero locks (`std::mutex`), zero dynamic allocations (`malloc`/`new`), and zero I/O on the high-priority audio callback thread. MMCSS (`AvSetMmThreadCharacteristicsW`) ensures Windows gives maximum priority to audio and network queues. |

---

## 3. High-Level Architectural Diagram

```
+----------------------------------------------------------------------------------------------------+
|                                         GUI THREAD (JUCE)                                          |
|   - Per-Channel Mixer (Gain, Pan, Mute, Solo)   - Latency Budget Panel   - Chromatic Tuner          |
|   - Drum Machine (Pads, Step Sequencer, Engine) - VST3 Plugin Editors   - Network Diagnostics      |
+--------------------------------------------------+-------------------------------------------------+
                                                   | (Lock-free UI queue & atomics)
                                                   v
+--------------------------------------------------+-------------------------------------------------+
|                                        AUDIO ENGINE THREAD                                         |
|  [Hardware Input] (ASIO / WASAPI)                                                                  |
|         |                                                                                          |
|         +---> [Local FX Chain / VST3 Host] ---> [Local Zero-Latency Monitor] ---> [Hardware Out]   |
|         |                                                                             ^            |
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
|  [Send Queue] ---> [Encoder (PCM / Opus)] ---> [UDP Socket Transmitter] ---> [Host / Peers]        |
|                                                                                                    |
|  [UDP Socket Receiver] ---> [Jitter Buffer + Drift Correction + PLC] ---> [Decoder] ---> Recv Ring |
+----------------------------------------------------------------------------------------------------+
```

---

## 4. Subsystem Details

### 4.1 Real-Time Audio Engine
1. **Audio Callback (`juce::AudioIODeviceCallback`)**:
   - Reads hardware input samples (Channel 1: Instrument/Guitar, Channel 2: Vocal/Mic or Stereo).
   - Feeds local DSP / VST3 effects chain (Noise Gate -> Compressor -> EQ -> Overdrive -> Reverb).
   - Routes wet/dry signal directly to local monitoring mixer (0 network delay).
   - Pushes timestamped audio frames into lock-free SPSC ring buffers for network transmission.
   - Pulls remote peer streams from jitter buffers, mixes with local monitor, and writes to audio output buffer.
2. **Real-Time Safety Guarantees**:
   - Memory for all audio buffers is pre-allocated during engine initialization.
   - Communication between audio callback and UI/Network is performed strictly via lock-free atomic circular buffers (`juce::AbstractFifo` and atomic floats).

### 4.2 Network & Session Subsystem
1. **LAN Mode**:
   - UDP broadcast (`255.255.255.255:44044`) for zero-configuration host discovery.
   - Raw linear PCM transmission with minimal header (Packet ID, Timestamp, Channel ID).
   - Target network transit time: 1–3 ms on wired gigabit ethernet.
2. **Internet Mode**:
   - Integrated room code / session rendezvous using free public STUN servers (e.g. Google STUN `stun.l.google.com:19302`) for UDP hole punching.
   - Opus audio encoding with 2.5 ms or 5 ms packet duration.
   - Fallback configuration guide for Tailscale/ZeroTier virtual zero-config LAN.
3. **Adaptive Jitter Buffer & Drift Correction**:
   - Dynamic buffer sizing tracking packet arrival variance.
   - Picosecond-level sample-rate drift compensation using cubic spline or linear audio interpolation (preventing buffer underrun/overflow over long rehearsal sessions).
4. **Network Health Diagnostics**:
   - Continuous round-trip time (RTT), packet loss %, and jitter calculations.
   - Detection and warning for Wi-Fi vs. Ethernet adapters and local client isolation.

### 4.3 Guitar & Vocal Processor (VST3 + Built-in DSP)
- Supports hardware multi-effects / amp modelers (Zoom, Boss, Line 6, Helix, etc.) via low-latency ASIO.
- Built-in JUCE DSP effects chain (Gate, Compressor, Parametric EQ, Drive, Plate Reverb) with zero latency overhead.
- VST3 hosting module scanning standard Windows VST3 folders (`C:\Program Files\Common Files\VST3`), instantiating plugins, and rendering custom plugin GUI windows.

### 4.4 Drum Machine Subsystem
- Synthesized algorithmic drum voice engine (Kick, Snare, Hi-hat closed/open, Toms, Clap, Percussion / Dholak).
- Step sequencer (16/32 steps) with swing, velocity modulation, and pattern switching.
- Real-time keyboard pads and MIDI input support with MIDI Learn.
- Drum latency compensation slider to adjust drum feel against network delay.
- MIDI export and audio multitrack routing.

### 4.5 Latency Budget Breakdown
For remote jamming to feel natural, total mouth-to-ear latency should ideally stay under **25–35 ms**:

| Stage | LAN (Optimal) | Internet (Typical Fiber) |
| :--- | :--- | :--- |
| Audio Input Buffer (64 samples @ 48kHz) | 1.33 ms | 1.33 ms |
| DSP / FX Processing | 0.00 – 0.50 ms | 0.00 – 0.50 ms |
| Network Transmission & Serialization | 1.00 – 2.50 ms | 8.00 – 18.00 ms |
| Jitter Buffer Window | 1.00 – 2.00 ms | 5.00 – 10.00 ms |
| Audio Output Buffer (64 samples @ 48kHz) | 1.33 ms | 1.33 ms |
| **Total Estimated Latency** | **~4.7 – 7.7 ms** | **~15.7 – 31.2 ms** |

---

## 5. Potential Risks & Mitigation Strategies

1. **Risk: Audio Glitches / XRuns on Low-End Laptops**
   - *Mitigation:* Employ Windows Multimedia Class Scheduler Service (MMCSS) to bump audio and worker thread priorities. Enforce zero allocations in the audio loop. Provide WASAPI Exclusive mode when native ASIO drivers are absent.
2. **Risk: Wi-Fi Packet Jitter and Packet Loss**
   - *Mitigation:* Built-in network adapter detector that flags Wi-Fi adapters and displays clear prompts to plug in an Ethernet cable. Use Opus FEC (Forward Error Correction) and Packet Loss Concealment (PLC) on Internet connections.
3. **Risk: Router NAT / Firewall Blocking UDP Packets**
   - *Mitigation:* Three-tier connectivity strategy: (1) Free STUN-based UDP hole punching, (2) Detailed UPnP / port forwarding guidance, (3) Free zero-configuration virtual LAN overlay (Tailscale / ZeroTier) with step-by-step setup guide.
4. **Risk: Clock Drift Over Extended Rehearsals**
   - *Mitigation:* Fractional sample rate converter (resampler) in the jitter buffer playback stage to subtly pull or push the read pointer by fractions of a sample without audible pitch warble.
