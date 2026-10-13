# PaasPhoos

Ultra-low latency, open-source real-time rehearsal studio for Windows 10 and 11. Designed for full 6-piece bands (vocalist, keyboardist, three guitarists, drummer) to rehearse simultaneously over local networks (LAN) and the Internet without paid subscriptions, third-party cloud services, or commercial SDK licenses.

---

## Table of Contents

1. [Overview](#1-overview)
2. [Key Architecture & Technical Specifications](#2-key-architecture--technical-specifications)
3. [Installation & Quick Start (Pre-Built Binaries)](#3-installation--quick-start-pre-built-binaries)
4. [Session Workflow: Connecting with Jam Code](#4-session-workflow-connecting-with-jam-code)
5. [Audio Driver Configuration](#5-audio-driver-configuration)
6. [Building from Source](#6-building-from-source)
7. [Running Tests](#7-running-tests)
8. [Recommended Free VST3 Plugins](#8-recommended-free-vst3-plugins)
9. [Troubleshooting & Network Optimization](#9-troubleshooting--network-optimization)
10. [License](#10-license)

---

## 1. Overview

PaasPhoos provides an all-in-one low-latency rehearsal environment engineered to keep acoustic round-trip delay under the critical human perception threshold of 25–30 ms. The software includes:

- **Low-Latency Audio Engine**: Built on C++17 and JUCE 8 (GPLv3), supporting Steinberg ASIO and Windows Audio Session API (WASAPI Exclusive/Shared). Implements lock-free ring buffers and Windows MMCSS (Multimedia Class Scheduler Service) Pro Audio thread priority.
- **Session Management via Jam Code**: Session hosts receive a compact, shareable Jam Code (e.g., `JAM-C0A80169-AC0D`). Band members join with one click using the built-in clipboard paste mechanism, eliminating the need to look up or exchange IP addresses and port numbers.
- **In-App Star Audio Reflector**: Acts as an integrated audio relay supporting up to six active participants simultaneously without requiring a standalone server daemon.
- **Adaptive Jitter Buffer & Clock Drift Resampler**: RFC 3550 delay variation estimation dynamically controls buffer depth. A 4-point Hermite cubic resampler corrects hardware clock drift between participants without inducing audible pitch fluctuations.
- **Real-Time Built-in DSP Effects Rack**: Five-stage in-place processing chain including Noise Gate, Tube Overdrive, Compressor, 4-Band Parametric EQ, and Plate Reverb.
- **VST3 Plugin Hosting**: Native 64-bit VST3 scanner and host with asynchronous background instantiation and floating plugin editor windows.
- **8-Voice Synthesized Drum Machine & Sequencer**: Physical-modeled synthesis engine (Kick, Snare, Closed/Open Hi-Hats, Toms, Clap, Bollywood Dholak percussion), 16-step grid sequencer, groove presets, live keyboard trigger pads, and Standard MIDI File (.mid) export.
- **Lock-Free Multitrack WAV Recording**: Real-time stem recording writing 24-bit WAV files for both the master mix and individual participant stems.
- **Rehearsal Tools**: Setlist manager with automatic tempo/key synchronization, in-app text chat, push-to-talk talkback override, and live component-by-component latency budget diagnostics.

---

## 2. Key Architecture & Technical Specifications

| Component | Technical Detail |
| :--- | :--- |
| **Operating System** | Windows 10 / 11 (64-bit) |
| **Language & Toolchain** | C++17, MSVC 2022 / 2026, CMake 3.22+ |
| **Audio Framework** | JUCE 8 (GPLv3) |
| **Driver Interfaces** | Steinberg ASIO 2.3, Windows WASAPI Exclusive & Shared |
| **Supported Buffer Sizes** | 64, 128, 256, 512 samples (1.33 ms to 10.67 ms @ 48 kHz) |
| **Network Protocols** | UDP unicast audio streaming, UDP broadcast LAN discovery, RFC 5389 STUN NAT traversal |
| **LAN Audio Format** | Uncompressed linear 16-bit PCM @ 48 kHz (0 ms codec delay) |
| **Default Network Port** | UDP 44045 (configurable) |
| **Threading Model** | Strict lock-free, zero-allocation audio callback thread with MMCSS scheduling |

### End-to-End Latency Budget

| Component | Buffer Configuration | Latency Range | Notes |
| :--- | :--- | :--- | :--- |
| **Hardware Input Capture** | 128 samples @ 48 kHz | 2.67 ms | ASIO / WASAPI Exclusive capture |
| **Local DSP & VST3 FX** | Block-aligned | 0.00 – 1.00 ms | In-place sample-accurate processing |
| **Network Propagation** | UDP LAN / Fiber | 1.00 – 12.00 ms | Physical transmission delay |
| **Jitter Buffer & Resampler** | Adaptive (RFC 3550) | 4.00 – 8.00 ms | Dynamic jitter absorption |
| **Hardware Output Render** | 128 samples @ 48 kHz | 2.67 ms | DAC rendering to headphones |
| **Total One-Way Latency** | | **10.34 – 26.34 ms** | **Sub-perceptual for tight ensemble playing** |

---

## 3. Installation & Quick Start (Pre-Built Binaries)

### Step 1: Download
Download the latest portable release archive (`PaasPhoos-v0.1.0-Windows-x64.zip`) from the [GitHub Releases](https://github.com/pengeon1/PaasPhoos/releases) page.

### Step 2: Extract
Extract the contents of the archive to any directory on your computer (e.g., `C:\Apps\PaasPhoos` or your desktop).

### Step 3: Run
Launch the application by running:
- `Start-PaasPhoos.bat` (Recommended launcher), or
- `PaasPhoos.exe`

No installation wizards, background services, or registry modifications are required.

---

## 4. Session Workflow: Connecting with Jam Code

PaasPhoos uses unique **Jam Codes** to simplify connections. You do not need to look up or communicate IP addresses.

### Hosting a Rehearsal
1. Launch **PaasPhoos**.
2. Select your network mode in the top bar:
   - **Internet (STUN)**: For band members in different locations.
   - **Local LAN**: For band members on the same local Wi-Fi or Ethernet network.
3. Click **START HOSTING**.
4. The application will generate a unique Jam Code (e.g., `JAM-C0A80169-AC0D`).
5. Click **COPY CODE** to copy the code to your clipboard.
6. Share this code with your band members via chat or messaging.

### Joining an Existing Rehearsal
1. Launch **PaasPhoos**.
2. Copy the Jam Code provided by your band leader.
3. In PaasPhoos, click **PASTE & JOIN** (or paste the code into the text field and click **JOIN JAM**).
4. The connection handshake will complete automatically and audio streaming will begin.

*(Optional)* Advanced users who prefer direct IP entry can click **DIRECT IP / LAN** in the top navigation bar to access raw IP and port input fields.

---

## 5. Audio Driver Configuration

Access the **Audio Device & Hardware Settings** panel at the top of the interface:

### Option 1: Native Hardware ASIO (Recommended for Audio Interfaces)
If using an external USB audio interface (e.g., Focusrite, PreSonus, MOTU, Behringer, SSL, Universal Audio):
1. Install the official driver provided by the interface manufacturer.
2. In PaasPhoos, set **Driver Type** to **ASIO**.
3. Select your hardware device from the **Device** dropdown.
4. Set the buffer size to **128 samples** (or **64 samples** for ultra-low latency).

### Option 2: Windows WASAPI (Zero Configuration Fallback)
If using a computer's built-in headphone jack or sound card:
1. In PaasPhoos, set **Driver Type** to **WASAPI**.
2. Set **Mode** to **Exclusive** for minimum latency.
3. Select your default audio input and output endpoints.

### Option 3: Virtual ASIO Drivers (For Built-in Sound Cards)
To achieve ASIO-level performance on computers without dedicated hardware drivers:
- **FlexASIO** (Open Source): [github.com/dechamps/FlexASIO/releases](https://github.com/dechamps/FlexASIO/releases)
- **ASIO4ALL**: [asio4all.org](https://www.asio4all.org/)

---

## 6. Building from Source

### Prerequisites
- **Operating System**: Windows 10 or 11 (64-bit)
- **C++ Compiler**: Microsoft Visual C++ (MSVC) from Visual Studio 2022 or Visual Studio Build Tools with the *Desktop development with C++* workload
- **CMake**: Version 3.22 or higher
- **Git**: Installed and available in system PATH

### Build Instructions

1. Clone the repository:
   ```cmd
   git clone https://github.com/pengeon1/PaasPhoos.git
   cd PaasPhoos
   ```

2. Run the automated build script:
   - Command Prompt:
     ```cmd
     build.bat
     ```
   - PowerShell:
     ```powershell
     .\build.ps1
     ```

   Or configure and build directly with CMake:
   ```cmd
   cmake -B build -S .
   cmake --build build --config Release --parallel
   ```

3. Binaries will be generated in:
   - Main Application: `build\PaasPhoos_artefacts\Release\PaasPhoos.exe`
   - Headless Test Client: `build\Release\PaasPhoosBot.exe`
   - Automated Test Suite: `build\Release\PaasPhoosTests.exe`

4. To package a standalone portable ZIP distribution:
   ```powershell
   .\package.ps1
   ```
   The archive will be output to `dist\PaasPhoos-v0.1.0-Windows-x64.zip`.

---

## 7. Running Tests

To execute the automated unit and integration verification test suite:

```cmd
.\build\Release\PaasPhoosTests.exe
```

The test runner validates:
1. Audio ring buffer concurrency and non-blocking safety
2. Sample-accurate metronome calculations
3. Chromatic tuner pitch detection (YIN algorithm)
4. Network packet serialization and header integrity
5. RFC 3550 jitter estimation and round-trip time calculations
6. Adaptive jitter buffer priming and playback
7. Packet loss concealment and Hermite clock drift correction
8. Multi-peer directory management
9. Built-in DSP effects rack filters and presets
10. Drum synthesis voices and physical modeling
11. Drum sequencer pattern generation and Standard MIDI File export
12. STUN Jam Code encoding, decoding, and fallback handling
13. Setlist manager persistence and tempo synchronization

---

## 8. Recommended Free VST3 Plugins

PaasPhoos automatically scans the system VST3 directory (`C:\Program Files\Common Files\VST3`). The following free, open-source VST3 plugins are verified compatible:

- **Guitar Amplification & Cabinet Emulation**:
  - **Neural Amp Modeler (NAM)** (Open Source): Neural network-based tube amplifier emulation.
  - **Ignite Amps Emissary**: High-gain dual-channel tube amplifier.
  - **Ignite Amps NadIR**: Zero-latency dual impulse response (IR) cabinet loader.
- **Dynamics & Mixing**:
  - **Tokyo Dawn Records TDR Nova**: Dynamic parallel equalizer.
  - **Klanghelm DC1A**: Character compressor.
- **Spatial & Time-Based Effects**:
  - **Valhalla Supermassive**: Algorithmic reverb and delay.
  - **Chowdhury DSP Chow Tape**: Physical modeled analog tape saturation.

---

## 9. Troubleshooting & Network Optimization

- **Audio Glitches or Buffer Underruns**:
  - Switch to a dedicated ASIO driver or enable WASAPI Exclusive mode.
  - Increase buffer size from 64 to 128 or 256 samples.
  - Verify that no heavy background processes (e.g., file sync, background updates) are consuming audio thread bandwidth.
- **Connection Diagnostics**:
  - Ensure UDP port **44045** is not blocked by Windows Defender Firewall or local router settings.
  - In restricted corporate or university networks where direct UDP traversal is blocked, running [Tailscale](https://tailscale.com) or [ZeroTier](https://zerotier.com) provides an encrypted peer-to-peer virtual mesh network with zero port forwarding.
- **Network Performance**:
  - For lowest jitter, a wired Ethernet connection is strongly recommended over Wi-Fi.

---

## 10. Typography & Design Tokens

PaasPhoos bundles an offline typography system licensed under the SIL Open Font License (OFL):
- **Sora (600 SemiBold, 700 Bold)**: Wordmark, screen titles, hero headings, and primary CTA buttons.
- **Inter (400 Regular, 500 Medium, 600 SemiBold)**: UI controls, labels, settings, menus, and channel strip names.
- **JetBrains Mono (400 Regular, 500 Medium)**: Live numeric telemetry (BPM, ping, jitter, packet loss, dB meter readouts, sample rate, buffer size) using tabular figures that never shift width.

All fonts are embedded directly into the application executable via JUCE BinaryData (`juce_add_binary_data`), ensuring 100% offline functionality across hostel and university LAN networks without internet access. License texts are located in `assets/fonts/licenses/`.

---

## 11. License

PaasPhoos is open-source software licensed under the **GNU General Public License v3.0 (GPLv3)**.
- JUCE 8 is licensed under the GPLv3 for open-source software.
- The Steinberg ASIO SDK is utilized in compliance with the Steinberg Audio Stream I/O API Agreement.
- Bundled typefaces (Sora, Inter, JetBrains Mono) are licensed under the SIL Open Font License 1.1.
