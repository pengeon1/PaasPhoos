# PaasPhoos (पासफूस)
> Ultra-low latency open-source remote rehearsal desktop console for Windows 10/11. Zero budget. Zero paid cloud services. Zero commercial SDK fees.

---

## 1. Overview
**PaasPhoos** is built specifically for a 6-piece band (1 vocalist, 1 keyboardist, 3 guitarists, 1 drummer) to rehearse together in real time with the lowest physical latency possible over local LANs and the internet.

### Complete Feature Set (Milestones 1–7):
- **Audio Engine (M1)**: C++17 powered by JUCE 8 (GPLv3). Low-latency Windows WASAPI and Steinberg ASIO SDK drivers. Zero dynamic allocation on audio callbacks, lock-free ring buffers (`AbstractFifo`), Windows MMCSS Pro Audio thread scheduling.
- **Local Monitoring & Mixer (M1)**: Zero-latency hardware monitor path, per-channel gain, pan, mute, solo, peak meters with clip reset, sample-accurate metronome, and real-time chromatic tuner.
- **LAN Rehearsal (M2)**: Raw uncompressed 16-bit linear PCM over UDP (0 ms codec latency), UDP broadcast session discovery, Wi-Fi vs. Ethernet adapter detector with isolation warning, and headless test-bot client (`PaasPhoosBot.exe`).
- **6-Piece Band Rehearsal (M3)**: In-app Star Topology audio relay supporting 6 band members, per-peer adaptive jitter buffer (RFC 3550 delay variation estimation), Hermite polynomial resampler for sample clock drift correction, and energy-decay packet loss concealment (PLC).
- **DSP Rack & VST3 Hosting (M4)**: Built-in 5-stage real-time DSP effects chain (Noise Gate, Tube Overdrive with tone control, Compressor with auto-makeup, 4-Band Parametric EQ, Plate Reverb) + Full VST3 plugin scanning, hosting, parameter control, and external plugin GUI window display (`C:\Program Files\Common Files\VST3`).
- **Synthesized Drum Machine (M5)**: 8 synthesis voices (Kick, Snare, Closed Hat, Open Hat, Low Tom, High Tom, Clap, Indian Bollywood Dholak percussion) + WAV custom sample loading + 5 kit presets. 16-step sequencer grid, swing (0–60%), humanize timing, fill generator, auto-grooves (Rock, Pop, Metal, Funk, EDM, Bollywood), live trigger pads (hotkeys `Z, X, C, V, A, S, D, F`), and Standard MIDI File (.mid) export.
- **Internet Mode & Network Sim (M6)**: RFC 5389 STUN client querying Google STUN (`stun.l.google.com:19302`) to resolve public IP/port and generate 6-character room codes (`PP-XXXX`) for direct connection across the Internet. Integrated network simulator for artificial latency injection (0–120ms), jitter (0–40ms), and packet loss (0–25%) testing. Virtual LAN (Tailscale / ZeroTier) guide.
- **Multitrack Recording, Setlist & Chat (M7)**: Lock-free background multitrack 24-bit WAV recorder capturing the stereo master mix (`00_Master_Mix.wav`) plus isolated individual stems (`01_Local_Instrument.wav` to `08_Drums.wav`). Setlist manager syncing BPM and key to metronome and drum sequencer. In-app text chat and push-to-talk (PTT) voice talkback. Live end-to-end latency budget breakdown.

---

## 2. Requirements & Building

### Prerequisites
- **Operating System**: Windows 10 or 11 (64-bit).
- **Build Tools**: Visual Studio Build Tools (free) or Visual Studio 2022 / 2026 with C++ Desktop Development workload.
- **CMake**: Version 3.22 or higher (free).
- **Git**: Installed and available in PATH.

### One-Command Build

1. Clone or navigate to the repository directory:
   ```cmd
   cd c:\Users\piyus\OneDrive\Desktop\PaasPhoos
   ```

2. Run the build script:
   - In Command Prompt:
     ```cmd
     build.bat
     ```
   - Or in PowerShell:
     ```powershell
     .\build.ps1
     ```

3. The compiled binaries will be located at:
   - Main App: `build\PaasPhoos_artefacts\Release\PaasPhoos.exe`
   - Headless Bot: `build\Release\PaasPhoosBot.exe`
   - Test Suite: `build\Release\PaasPhoosTests.exe`

4. To package a standalone portable release:
   ```powershell
   .\package.ps1
   ```
   Produces `dist\PaasPhoos-v0.1.0-Windows-x64.zip`.

---

## 3. Audio Driver Setup Guide

### Option A: Windows WASAPI (Default - Zero Setup Required)
PaasPhoos works immediately on any Windows 10/11 laptop using Windows Audio Session API (WASAPI Exclusive/Shared).
- Select **WASAPI** from the **Driver Type** dropdown in the app.
- Supports buffer sizes down to 64 or 128 samples (~1.3 to 2.7 ms buffer latency) without installing any drivers.

### Option B: Hardware ASIO (Recommended for Audio Interfaces)
If you are using a dedicated USB audio interface (Focusrite Scarlett, PreSonus, MOTU, Behringer, SSL, etc.) or a multi-effects pedal with USB audio (Zoom G3/G5, Boss Katana/GT, Line 6 Helix/POD):
- Install the official manufacturer ASIO driver from the vendor's free download page.
- Select **ASIO** in PaasPhoos and choose your hardware interface.

### Option C: Free Virtual ASIO Drivers (For Built-in Soundcards)
If you are using a laptop without an external interface and want sub-5ms ASIO performance:
1. **FlexASIO** (Open Source, Free):
   - Download installer: [FlexASIO GitHub Releases](https://github.com/dechamps/FlexASIO/releases)
   - Uses WASAPI Exclusive underneath with ASIO application compatibility.
2. **ASIO4ALL** (Free):
   - Download from: [asio4all.org](https://www.asio4all.org/)
   - Provides kernel-streaming low-latency access to Realtek and laptop audio chips.

---

## 4. Internet Mode Rehearsal Guide (Zero Cost, No Port Forwarding)

### Method 1: Built-in STUN Room Code (RFC 5389)
1. Host clicks **Internet Mode (STUN)** -> **START HOSTING**.
2. PaasPhoos queries Google STUN (`stun.l.google.com:19302`), discovers the host's public IP and mapped NAT port, and generates a **Room Code** (e.g. `PP-MTkyLjE2OC4x...`).
3. Host sends the Room Code to the band members over chat.
4. Band members paste the code into the **IP / Room Code** box and click **DIRECT CONNECT**.

### Method 2: Virtual Mesh LAN (Tailscale / ZeroTier) - Most Reliable
For band rehearsals where members have Symmetric NATs (e.g. cellular 5G hotspots or strict campus routers) that block direct UDP traversal:
1. Download **Tailscale** (free for up to 100 devices at [tailscale.com](https://tailscale.com)) or **ZeroTier** ([zerotier.com](https://www.zerotier.com)).
2. All 6 band members log into the same private Tailscale tailnet.
3. Tailscale assigns each computer a permanent virtual IP (e.g. `100.x.y.z`).
4. Host clicks **START HOSTING**.
5. Other band members paste the host's `100.x.y.z` address into **IP** and click **DIRECT CONNECT**.
6. Packets travel point-to-point with WireGuard UDP encryption and zero configuration.

---

## 5. Recommended Free VST3 Plugins
PaasPhoos automatically scans `C:\Program Files\Common Files\VST3`. Top recommended free, open-source VST3 plugins:
- **Guitar Amps & Cabs**: 
  - *Neural Amp Modeler (NAM)* (Open Source) – World-class tube amplifier profiles.
  - *LePou Plugins* / *Ignite Amps Emissary* (Free) – High-gain rhythm and clean tube amp emulation.
  - *NadIR* (Ignite Amps) – Zero-latency dual impulse response (IR) cabinet loader.
- **Bass & Vocals**:
  - *TDR Nova* (Tokyo Dawn Records) – Dynamic 4-band parallel equalizer.
  - *Klanghelm DC1A* – Effortless character compressor.
- **Effects**:
  - *Valhalla Supermassive* – Cosmic lush delays and reverbs.
  - *Chow Multi-Tool / Chow Tape* (Chowdhury DSP, Open Source) – Analog tape saturation and flanger.

---

## 6. End-to-End Latency Budget
For remote rehearsals to feel natural and tight, one-way audio latency should remain under 25–30 ms:

| Component | Buffer Size | Time (ms) | Notes |
| :--- | :--- | :--- | :--- |
| **Input Hardware Buffer** | 128 samples @ 48 kHz | 2.67 ms | ASIO / WASAPI input capture |
| **DSP FX & VST3 Processing** | Block-aligned | 0.00 – 1.00 ms | Built-in DSP runs sample-accurate in-place |
| **Network Transmission** | UDP LAN / Metro Fiber | 1.00 – 12.00 ms | Direct physical propagation delay |
| **Jitter Buffer & Resampler** | Adaptive (RFC 3550) | 4.00 – 8.00 ms | Absorbs network jitter, adjusts dynamically |
| **Output Hardware Buffer** | 128 samples @ 48 kHz | 2.67 ms | ASIO / WASAPI output rendering |
| **Total One-Way Acoustic** | | **10.34 – 26.34 ms** | **Ideal for full-band jamming without tempo drag!** |

---

## 7. License
PaasPhoos is open-source software licensed under the **GNU General Public License v3.0 (GPLv3)**.
JUCE 8 is licensed under the GPLv3 for open-source use.
Opus is licensed under the 3-Clause BSD License.
