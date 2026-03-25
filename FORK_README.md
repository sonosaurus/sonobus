# SonoBus Fork — WASAPI Loopback & Application Audio Capture

A fork of [SonoBus](https://github.com/sonosaurus/sonobus) that adds native Windows audio capture features, eliminating the need for Voicemeeter, VB-Cable, or any virtual audio cable software.

## New Features

### WASAPI Loopback Capture
Every output device (speakers, headphones, monitors) now appears as a **(Loopback)** input device in SonoBus. Select one to capture all audio playing through that device — lossless, digital, no extra software.

### Application Audio Capture (Windows 11)
A new **"Application Audio"** device type that lists running processes. Select a specific app (e.g. SnowRunner.exe) to capture only its audio. No output device needed at all. Requires Windows 10 Build 20348+ / Windows 11.

### Save Direct Connect Address
The last used direct connect IP:port is saved and pre-filled on next launch.

### Auto-Reconnect Direct Peer
The "Reconnect Last" option now also reconnects to the last direct peer connection on startup.

## Direct Ethernet Setup Guide

You can connect two PCs with a single ethernet cable for ultra-low-latency audio streaming. No router or internet needed.

### 1. Physical Connection
Plug an ethernet cable directly between the two PCs.

### 2. Set Static IPs
On each PC, configure the ethernet adapter with a static IP:

**PC 1 (e.g. receiver with speakers):**
1. Open **Settings > Network & Internet > Ethernet** (or the direct connection adapter)
2. Click **Edit** next to IP assignment, switch to **Manual**
3. Enable **IPv4** and set:
   - IP address: `192.168.1.1`
   - Subnet mask: `255.255.255.0`
   - Gateway: leave blank
   - DNS: leave blank
4. Save

**PC 2 (e.g. sender with the game):**
- Same steps, but set IP to `192.168.1.2`

### 3. Connect in SonoBus
1. Open SonoBus on both PCs
2. On either PC, note the **Local Address** shown in the Direct Connect dialog (e.g. `192.168.1.1:58893`)
3. On the other PC, click **Direct Connect** and enter the address (e.g. `192.168.1.2:58893`)
4. The port changes each time SonoBus starts — always check the Local Address shown

### 4. Audio Settings

**Sending PC (e.g. running SnowRunner):**
- Audio Device Type: **Windows Audio**
- Input: **Your output device (Loopback)** — e.g. "Speakers (Realtek) (Loopback)"
- Output: **<< none >>**
- Set Windows default output to any device, mute it in Windows if you don't want local sound

Or use **Application Audio** device type to capture a specific game directly (Win11 only).

**Receiving PC (with DAC/amp/speakers):**
- Audio Device Type: **Windows Audio**
- Input: **<< none >>**
- Output: **Your DAC/speakers** (e.g. USB-C audio adapter)

### 5. Audio Format
In SonoBus, set the send quality to **PCM 32-bit** for completely lossless audio over the direct ethernet link. Bandwidth is not an issue on a local cable.

## Sending All System Audio (No Real Output Device)

Three options, from easiest to most involved:

1. **Mute a real device** — Set any real output device (e.g. Realtek onboard) as Windows default, mute it in Windows. WASAPI loopback still captures audio even when muted. Easiest option.

2. **Application Audio** — Use the "Application Audio" device type to capture a specific app directly. No output device needed. Windows 11 only.

3. **Virtual Audio Driver** — If your PC genuinely has no audio output at all, install [Virtual-Audio-Driver](https://github.com/VirtualDrivers/Virtual-Audio-Driver) (open source, MIT) to create a virtual output, then use its loopback. Requires test signing mode.

## Download

Grab `SonoBus.exe` from the [Releases](https://github.com/mthwJsmith/sonobus/releases) page, or build from source.

## Building from Source

### Requirements
- CMake 3.15+
- Visual Studio 2022
- Windows SDK
- [ASIO SDK](https://github.com/audiosdk/asio) — clone to `../asiosdk` (sibling directory)

### Steps

```bash
# Clone
git clone --recursive https://github.com/mthwJsmith/sonobus.git
cd sonobus

# Clone ASIO SDK
git clone https://github.com/audiosdk/asio.git ../asiosdk

# Configure (unset Android toolchain if you have Android SDK installed)
CMAKE_TOOLCHAIN_FILE="" cmake -B build -G "Visual Studio 17 2022" -A x64

# Build
CMAKE_TOOLCHAIN_FILE="" cmake --build build --config Release --target SonoBus_Standalone

# Output: build/SonoBus_artefacts/Release/Standalone/SonoBus.exe
```

## Technical Details

### WASAPI Loopback
- `AUDCLNT_STREAMFLAGS_LOOPBACK` on `IAudioClient::Initialize`
- Lossless PCM capture, digitally before the DAC
- Forces shared mode (required by Windows)
- Changes in `deps/juce/modules/juce_audio_devices/native/juce_WASAPI_windows.cpp`

### Per-Process Audio Capture
- `ActivateAudioInterfaceAsync` with `AUDIOCLIENT_PROCESS_LOOPBACK_PARAMS`
- Captures from a specific process tree
- No output device, virtual cable, or driver needed
- Windows 10 Build 20348+ (checked at runtime)
- `Source/ProcessAudioCapture.cpp` and `Source/ApplicationAudioDevice.h`

## Upstream Issues Addressed
- [#241 — WASAPI Loopback as Input Device](https://github.com/sonosaurus/sonobus/issues/241)
- [#53 — Save previous direct connection IP](https://github.com/sonosaurus/sonobus/issues/53)

## License
GPLv3 — same as upstream SonoBus.
