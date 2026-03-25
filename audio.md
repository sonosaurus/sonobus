# SonoBus Fork — Audio Improvements Plan

## Goal
Fork SonoBus to add WASAPI loopback capture, per-process audio capture, and UX improvements for a direct ethernet audio streaming setup (two PCs, 192.168.1.x, one-way lossless audio → Apple USB-C DAC → amp → speakers).

## Fork
- **Upstream**: https://github.com/sonosaurus/sonobus
- **Fork**: https://github.com/mthwJsmith/sonobus
- **Branch**: `feature/wasapi-loopback-and-improvements`

## Features

### 1. Save Direct Connect Address — DONE
- Saves last used IP:port to processor state (extraState tree)
- Pre-fills the direct connect text field on next launch
- Files changed: `SonobusPluginProcessor.h`, `SonobusPluginProcessor.cpp`, `ConnectView.cpp`

### 2. WASAPI Loopback Capture — DONE
- Eliminates need for Voicemeeter/VB-Cable entirely
- Adds render devices as "(Loopback)" input devices in JUCE's WASAPI backend
- SonoBus can directly capture desktop/game audio (lossless PCM)
- File: `deps/juce/modules/juce_audio_devices/native/juce_WASAPI_windows.cpp`
- Changes:
  - Added loopback device ID helpers (`JUCE_LOOPBACK::` prefix scheme)
  - Added `isLoopbackDevice` flag to `WASAPIDeviceBase`
  - Modified `getStreamFlags()` to add `AUDCLNT_STREAMFLAGS_LOOPBACK` (0x00020000)
  - Modified `scan()` to enumerate render devices as loopback inputs with "(Loopback)" suffix
  - Modified `createDevices()` to detect loopback IDs and open render endpoint as capture
  - Modified `initialiseStandardClient()` to force shared mode for loopback
  - Modified `tryFormat()`, `findSupportedFormat()`, `querySupportedSampleRates()` to force shared mode for loopback
  - `WASAPIInputDevice` constructor accepts loopback flag

### 3. Per-Process Audio Capture (Win11 API) — DONE
- New files: `Source/ProcessAudioCapture.h`, `Source/ProcessAudioCapture.cpp`, `Source/ApplicationAudioDevice.h`
- Uses `AUDIOCLIENT_PROCESS_LOOPBACK_PARAMS` + `ActivateAudioInterfaceAsync`
- Captures audio from a specific app (e.g. SnowRunner.exe) instead of all system audio
- No special permissions needed, no drivers
- Windows 10 Build 20348+ / Windows 11
- Shows as **"Application Audio"** in Audio Device Type dropdown
- Lists running processes as input devices
- Fully wired into SonoBus UI

### 4. Auto-Reconnect Direct Peer — DONE
- Extended existing `reconnectToMostRecent()` to also try last direct peer connection
- If saved direct connect address exists, reconnects to it on startup
- Falls back to server/group reconnect if no direct address saved
- Uses the existing "Reconnect Last" checkbox in options

### 5. Update JUCE to sono8good — PENDING
- Current: essej/JUCE `sono7good` branch
- Target: essej/JUCE `sono8good` branch (JUCE 8, updated Jan 2026)
- Should be done carefully — reapply loopback changes on top of new base

### 6. Push & Build Verification — PENDING
- Push to fork, verify CMake + VS2022 build on Windows

## Architecture Notes

### WASAPI Loopback (how it works)
- `IAudioClient::Initialize` with `AUDCLNT_STREAMFLAGS_LOOPBACK` flag (0x00020000)
- Must use `AUDCLNT_SHAREMODE_SHARED` (exclusive mode not supported)
- Captures post-mix PCM audio digitally before DAC — completely lossless
- Event-driven supported on Win10 1703+
- Device enumeration: render endpoints show up as capture sources with "(Loopback)" suffix
- Loopback device IDs use `JUCE_LOOPBACK::` prefix to distinguish from regular capture devices

### Per-Process Capture (how it works)
- `ActivateAudioInterfaceAsync` with `AUDIOCLIENT_ACTIVATION_TYPE_PROCESS_LOOPBACK`
- `AUDIOCLIENT_PROCESS_LOOPBACK_PARAMS` specifies target PID
- Include mode: capture only that process's audio
- Exclude mode: capture everything except that process
- Async initialization (callback-based), different from normal WASAPI flow
- Not tied to specific audio endpoint — captures from all endpoints where process renders
- Requires Windows 10 Build 20348+ (checked at runtime)

### SonoBus Network Protocol
- Audio over OSC (AoO) — UDP peer-to-peer
- Supports PCM 16/24/32-bit and Opus codec
- Direct connect: `processor.connectRemotePeer(host, port, ...)`

## Build

**Important:** If you have Android SDK/NDK installed, you MUST unset the CMAKE_TOOLCHAIN_FILE
env var or CMake will try to use the Android NDK Clang compiler instead of MSVC.

**ASIO SDK:** Clone https://github.com/audiosdk/asio.git to `../asiosdk` (sibling of sonobus dir).

```bash
# Clone ASIO SDK (one-time)
git clone https://github.com/audiosdk/asio.git ../asiosdk

# Unset Android toolchain and configure
CMAKE_TOOLCHAIN_FILE="" cmake -B build -G "Visual Studio 17 2022" -A x64

# Build standalone app
CMAKE_TOOLCHAIN_FILE="" cmake --build build --config Release --target SonoBus_Standalone

# Output: build/SonoBus_artefacts/Release/Standalone/SonoBus.exe (25MB)
```
Requires: CMake 3.15+, Visual Studio 2022, Windows SDK, ASIO SDK

## How to Use WASAPI Loopback

1. Open SonoBus (the built exe, not the installed one)
2. Go to the gear/settings icon
3. Set **Audio Device Type** to **"Windows Audio"** (this is WASAPI shared mode)
4. In the **Input** dropdown, you should now see your output devices listed with **(Loopback)** suffix, e.g.:
   - `Speakers (Realtek High Definition Audio) (Loopback)`
   - `DELL S2722QC (NVIDIA High Definition Audio) (Loopback)`
5. Select the loopback device matching where your game/desktop audio plays
6. The **Output** dropdown stays as your normal playback device (or leave empty if you're only sending)
7. Connect to your other PC via Direct Connect as usual

This captures your desktop/game audio digitally (lossless PCM) and sends it over SonoBus — no Voicemeeter, no virtual cables, no restart issues.

## Key Files
- `Source/ConnectView.cpp` — direct connect UI, saves last address
- `Source/SonobusPluginProcessor.cpp` — state save/load, connection logic, auto-reconnect
- `Source/ProcessAudioCapture.cpp/.h` — per-process audio capture (Win11 API)
- `deps/juce/.../juce_WASAPI_windows.cpp` — WASAPI device enumeration & loopback capture
- `deps/aoo/` — Audio over OSC networking library
- `CMakeLists.txt` — build config (ProcessAudioCapture added)

## Files Changed Summary
```
Modified:
  CMakeLists.txt                                    — added ProcessAudioCapture to build
  Source/ConnectView.cpp                             — save/load direct connect address
  Source/SonobusPluginProcessor.cpp                  — persist address, auto-reconnect direct peer
  Source/SonobusPluginProcessor.h                    — lastDirectConnectAddress member + accessors
  deps/juce/.../juce_WASAPI_windows.cpp              — WASAPI loopback capture support

New:
  Source/ProcessAudioCapture.cpp                     — Win11 per-process audio capture
  Source/ProcessAudioCapture.h                       — header for above
  audio.md                                           — this file
```
