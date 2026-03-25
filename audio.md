# SonoBus Fork — Audio Improvements Plan

## Goal
Fork SonoBus to add WASAPI loopback capture and UX improvements for a direct ethernet audio streaming setup (two PCs, 192.168.1.x, one-way lossless audio → Apple USB-C DAC → amp → speakers).

## Fork
- **Upstream**: https://github.com/sonosaurus/sonobus
- **Fork**: https://github.com/mthwJsmith/sonobus
- **Branch**: `feature/wasapi-loopback-and-improvements`
- **WIP Branch**: `feature/application-audio-wip-broken` (Application Audio — works but audio quality is broken)
- **Base**: JUCE 7 (sono7good) — JUCE 8 had shared mode loopback issues, reverted
- **PR**: https://github.com/sonosaurus/sonobus/pull/280
- **Release**: https://github.com/mthwJsmith/sonobus/releases/tag/v1.7.2-loopback

## Current Status

### SHIPPED (v1.7.2-loopback)
- **WASAPI Loopback Capture** — loopback devices show in input list, audio levels confirmed, sounds great
- **Save Direct Connect Address** — pre-fills last used IP:port
- **Build system** — compiles with VS2022, ASIO SDK, on JUCE 7
- **Direct peer connection** — working after clearing stale %APPDATA% state

### SHELVED — Application Audio (per-process capture)
- Moved to `feature/application-audio-wip-broken` branch
- Win11 API works, shows running audio apps, captures audio
- **Audio quality is broken** — sounds super glitchy/choppy, likely sample rate or buffer mismatch
- Code preserved for future investigation

### Connection Debugging Results (2025-03-25)
Built v0–v3 test versions to isolate a connection issue:
- **v0-clean** (upstream, no changes) — didn't connect
- **v1-loopback** (WASAPI changes only) — worked
- **v2-connect** (+ save address) — didn't connect initially
- **v3-full** (+ Application Audio) — didn't connect initially
- **Root cause**: stale SonoBus state in `%APPDATA%`. After clearing, all versions worked.
- The WASAPI `else if` → `if` change in `createDevices()` may have incidentally helped device initialization.

### Known Issues
- **Loopback + same output device conflict** — in shared mode, can't use same device as both output AND loopback input. Set output to `<< none >>` on sending PC (which is correct for the use case anyway)
- **JUCE 8 shared mode loopback broken** — reverted to JUCE 7
- **Auto-reconnect direct peer not feasible** — ports are ephemeral, can't auto-reconnect (saved address only pre-fills the text field)

## Features Detail

### 1. WASAPI Loopback Capture — DONE
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

### 2. Save Direct Connect Address — DONE
- Saves last used IP:port to processor state (extraState tree)
- Pre-fills the direct connect text field on next launch
- Auto-reconnect removed (ephemeral ports make it impossible)
- Files changed: `SonobusPluginProcessor.h`, `SonobusPluginProcessor.cpp`, `ConnectView.cpp`

### 3. Per-Process Audio Capture (Win11 API) — SHELVED
- On `feature/application-audio-wip-broken` branch
- Files: `Source/ProcessAudioCapture.h`, `Source/ProcessAudioCapture.cpp`, `Source/ApplicationAudioDevice.h`
- Uses `AUDIOCLIENT_PROCESS_LOOPBACK_PARAMS` + `ActivateAudioInterfaceAsync`
- Audio quality broken — glitchy/choppy output, needs investigation

## Architecture Notes

### WASAPI Loopback (how it works)
- `IAudioClient::Initialize` with `AUDCLNT_STREAMFLAGS_LOOPBACK` flag (0x00020000)
- Must use `AUDCLNT_SHAREMODE_SHARED` (exclusive mode not supported)
- Captures post-mix PCM audio digitally before DAC — completely lossless
- Event-driven supported on Win10 1703+
- Loopback device IDs use `JUCE_LOOPBACK::` prefix to distinguish from regular capture devices
- **Cannot use same device as both output AND loopback input in shared mode** — set output to none on sender

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

## How to Use

### WASAPI Loopback (Sending PC)
1. Audio Device Type: **Windows Audio**
2. Input: **Your output device (Loopback)** — e.g. "Speakers (Realtek) (Loopback)"
3. Output: **<< none >>** (IMPORTANT: don't set output to same device as loopback input)
4. Direct Connect to other PC's IP:port

### Receiving PC
1. Audio Device Type: **Windows Audio**
2. Input: **<< none >>**
3. Output: **Apple DAC / speakers**

## Key Files
- `Source/ConnectView.cpp` — direct connect UI, saves last address
- `Source/SonobusPluginProcessor.cpp` — state save/load, connection logic
- `Source/SonoStandaloneFilterWindow.h` — device type registration
- `deps/juce/.../juce_WASAPI_windows.cpp` — WASAPI device enumeration & loopback capture
- `deps/aoo/` — Audio over OSC networking library
- `CMakeLists.txt` — build config

## Bugs Found & Fixed
- **Stale %APPDATA% state breaks connections** — clearing SonoBus appdata fixes direct connect issues between different builds
- **JUCE 8 shared mode loopback** — broken, reverted to JUCE 7

## Resources
- Virtual Audio Driver (for PCs with no real output): https://github.com/VirtualDrivers/Virtual-Audio-Driver
