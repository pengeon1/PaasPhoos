# PaasPhoos (पासफूस)
> Ultra-low latency open-source remote rehearsal desktop console for Windows 10/11. Zero budget. Zero paid cloud services. Zero commercial SDK fees.

---

## 1. Overview
**PaasPhoos** is built specifically for a 6-piece band (1 vocalist, 1 keyboardist, 3 guitarists, 1 drummer) to rehearse together in real time with the lowest physical latency possible over local LANs and the internet.

- **Audio Engine**: C++17 powered by JUCE 8 (GPLv3).
- **Driver Architecture**: Native ASIO driver support + Steinberg ASIO SDK integration + low-latency Windows WASAPI fallback.
- **Real-Time Safety**: Zero dynamic allocations (`malloc`/`new`), zero mutexes/locks on audio thread, lock-free ring buffers (`AbstractFifo`), Windows MMCSS Pro Audio thread scheduling.
- **Local Zero-Latency Monitoring**: Direct hardware-to-headphone path for your own instrument and voice.
- **Mixer & Rehearsal Utilities**: Per-channel gain, pan, mute, solo, peak meters with clip detection, sample-accurate click metronome, and real-time chromatic tuner.

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

3. The compiled binary will be placed at:
   ```
   build\PaasPhoos_artefacts\Release\PaasPhoos.exe
   ```

---

## 3. Audio Driver Setup Guide

### Option A: Windows WASAPI (Default - Zero Setup Required)
PaasPhoos works immediately on any Windows 10/11 laptop using Windows Audio Session API (WASAPI Exclusive/Shared).
- Select **WASAPI** from the **Driver Type** dropdown in the app.
- Supports buffer sizes down to 64 or 128 samples (~1.3 to 2.7 ms buffer latency) without installing any drivers.

### Option B: Hardware ASIO (Recommended for Audio Interfaces)
If you are using a dedicated USB audio interface (Focusrite Scarlett, PreSonus, MOTU, Behringer, SSL, etc.) or a multi-effects pedal with USB audio (Zoom G3/G5, Boss GT/Katana, Line 6 Helix/POD):
- Install the official manufacturer ASIO driver from the vendor's free download page.
- Select **ASIO** in PaasPhoos and choose your hardware interface.

### Option C: Free Virtual ASIO Drivers (For Built-in Soundcards)
If you are using a laptop without an external interface and want sub-5ms ASIO performance:
1. **FlexASIO** (Open Source, Free):
   - Download the installer from: [FlexASIO GitHub Releases](https://github.com/dechamps/FlexASIO/releases)
   - Uses WASAPI Exclusive or DirectSound underneath with ASIO application compatibility.
2. **ASIO4ALL** (Free):
   - Download from: [asio4all.org](https://www.asio4all.org/)
   - Provides kernel-streaming low-latency access to Realtek and laptop audio chips.

### Option D: Building with Steinberg ASIO SDK
As of late 2025, the official Steinberg ASIO SDK is dual-licensed under GPLv3.
To compile PaasPhoos with native Steinberg ASIO SDK support:
1. Download the ASIO SDK from the [Steinberg Developer Portal](https://sdk.steinberg.net/).
2. Extract the SDK into `external/asiosdk/` so that `iasiodrv.h` is located at `external/asiosdk/common/iasiodrv.h` or `external/asiosdk/iasiodrv.h`.
3. Re-run `build.bat`. CMake will automatically detect the SDK, define `JUCE_ASIO=1`, and build ASIO driver management into the binary.

---

## 4. Troubleshooting & Latency Optimization Tips

1. **Always Use Headphones**: Never use open laptop speakers while singing or playing into a microphone; the speaker output will leak into the mic and create audio feedback.
2. **Buffer Size Tuning**:
   - Start with **128 samples** (2.67 ms @ 48 kHz).
   - If your audio interface supports it and you hear no clicks, drop to **64 samples** (1.33 ms).
   - If you hear crackling or CPU spikes, increase buffer size to **256 samples**.
3. **Plug into AC Power**: Windows laptops throttle CPU frequencies when on battery power, which can cause audio dropouts (XRuns). Keep laptops plugged in during rehearsals.
4. **Use Ethernet**: Wi-Fi introduces packet jitter and variance that hurts real-time performance. Always use wired Ethernet for multi-player rehearsals.

---

## 5. Development Milestones
- **M1 [COMPLETED]**: Real-time Audio I/O + WASAPI/ASIO device manager + local zero-latency monitor + mixer console + sample-accurate metronome + chromatic tuner.
- **M2 [COMPLETED]**: LAN Mode, 2 users, raw uncompressed PCM over UDP, live latency stats, Wi-Fi detector, headless test-bot.
- **M3 [COMPLETED]**: 6-piece band rehearsal, in-app star relay, per-peer adaptive jitter buffer (RFC 3550), packet loss concealment (PLC), sample clock drift correction (Hermite resampler).
- **M4 [NEXT]**: VST3 plugin hosting + built-in DSP effects chain.
- **M5**: Synthesized drum machine engine + step sequencer + MIDI learn.
- **M6**: Internet Mode with Opus codec + UDP hole punching + network simulator.
- **M7**: Multitrack WAV recording, setlist manager, push-to-talk talkback, portable release.
