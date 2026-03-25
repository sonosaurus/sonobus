# SonoBus Fork — Audio Improvements Plan

## Goal
Fork SonoBus to add WASAPI loopback capture, per-process audio capture, and UX improvements for a direct ethernet audio streaming setup (two PCs, 192.168.1.x, one-way lossless audio → Apple USB-C DAC → amp → speakers).

## Fork
- **Upstream**: https://github.com/sonosaurus/sonobus
- **Fork**: https://github.com/mthwJsmith/sonobus
- **Branch**: `feature/wasapi-loopback-and-improvements`
- **Base**: JUCE 7 (sono7good) — JUCE 8 had shared mode loopback issues, reverted

## Current Status

### WORKING
- **WASAPI Loopback Capture** — loopback devices show in input list, audio levels confirmed
- **Application Audio (per-process capture)** — Win11 API working, shows running audio apps
- **Save Direct Connect Address** — pre-fills last used IP:port
- **Build system** — compiles with VS2022, ASIO SDK, on JUCE 7

### BROKEN — Direct peer connection not working
- Our custom build cannot establish direct peer connections (raw connect)
- The old installed SonoBus 1.7.2 (`C:\Program Files\SonoBus\SonoBus.exe`) works perfectly
- Same version (1.7.2), same network, same firewall rules — so something in our code changes broke it
- **Not a network issue** — ping 192.168.1.2 works, <1ms
- **Not a firewall issue** — firewall rules exist for our exe paths

### Known Issues
- **Loopback + same output device conflict** — in shared mode, can't use same device as both output AND loopback input. Set output to `<< none >>` on sending PC (which is correct for the use case anyway)
- **JUCE 8 shared mode loopback broken** — reverted to JUCE 7
- **Auto-reconnect direct peer removed** — ports are ephemeral, can't auto-reconnect (saved address only pre-fills the text field)

## Debugging Plan — Connection Issue

Need to isolate which change broke direct connect. Plan: build multiple versions with incremental changes, test each on USB drive.

### Test Builds (put all on D:\ USB drive)

1. **`SonoBus-v0-clean.exe`** — Fresh clone of upstream, zero changes. Should work like installed version. If this doesn't work either, the issue is build config not code changes.

2. **`SonoBus-v1-loopback.exe`** — Only WASAPI loopback changes to `juce_WASAPI_windows.cpp`. No processor/connect/Application Audio changes. Tests if JUCE WASAPI modifications broke networking.

3. **`SonoBus-v2-connect.exe`** — Loopback + save direct connect address (ConnectView.cpp + SonobusPluginProcessor changes). Tests if the processor state changes broke networking.

4. **`SonoBus-v3-full.exe`** — Everything including Application Audio. Current state.

### How to test each
- Run on both PCs
- Try raw connect with 192.168.1.2:port
- If connection works → that version is fine, bug is in the next version's additions

### How to build each version
```bash
# From sonobus directory
CMAKE_TOOLCHAIN_FILE="" cmake -B build -G "Visual Studio 17 2022" -A x64
CMAKE_TOOLCHAIN_FILE="" cmake --build build --config Release --target SonoBus_Standalone
# Output: build/SonoBus_artefacts/Release/Standalone/SonoBus.exe
```

### How to get clean source
```bash
# Clean clone (already done at ../sonobus-clean)
cd /c/Users/mthwj/documents/workspace/audio
git clone https://github.com/sonosaurus/sonobus.git sonobus-clean

# Or in existing repo, check out specific commits:
# git stash / git checkout <commit> / build / git checkout - / git stash pop
```

## Features Detail

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
- Completion handler MUST use WRL `RuntimeClass<ClassicCom, FtmBase, ...>` for free-threaded marshaling
- `GetMixFormat()` returns E_NOTIMPL for process loopback — use caller's requested sample rate
- Do NOT call `CoInitializeEx` in `getAudioProcesses()` — JUCE's message thread already has COM
- `getIndexOfDevice()` must null-check the device pointer
- Dummy output device name required so JUCE's AudioDeviceSelectorComponent doesn't crash
- Application Audio device type added AFTER `deviceManager.initialise()` so WASAPI is default

### 4. Save Direct Connect Address — DONE
- Pre-fills last used address in Direct Connect dialog
- Auto-reconnect removed (ephemeral ports make it impossible)

## Architecture Notes

### WASAPI Loopback (how it works)
- `IAudioClient::Initialize` with `AUDCLNT_STREAMFLAGS_LOOPBACK` flag (0x00020000)
- Must use `AUDCLNT_SHAREMODE_SHARED` (exclusive mode not supported)
- Captures post-mix PCM audio digitally before DAC — completely lossless
- Event-driven supported on Win10 1703+
- Loopback device IDs use `JUCE_LOOPBACK::` prefix to distinguish from regular capture devices
- **Cannot use same device as both output AND loopback input in shared mode** — set output to none on sender

### Per-Process Capture (how it works)
- `ActivateAudioInterfaceAsync` with `AUDIOCLIENT_ACTIVATION_TYPE_PROCESS_LOOPBACK`
- `AUDIOCLIENT_PROCESS_LOOPBACK_PARAMS` specifies target PID
- Requires WRL RuntimeClass with FtmBase (bare IUnknown returns CO_E_NOT_SUPPORTED 0x8000000E)
- `GetMixFormat()` returns E_NOTIMPL — must hardcode format (32-bit float stereo at requested rate)
- AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM handles any resampling
- Requires Windows 10 Build 20348+ (checked at runtime via RtlGetVersion)

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

### Application Audio (Sending PC, Win11 only)
1. Audio Device Type: **Application Audio**
2. Input: Pick the process (e.g. "brave.exe (PID 1234)")
3. Output shows "(No output - capture only)" — this is normal
4. Direct Connect to other PC's IP:port

### Receiving PC
1. Audio Device Type: **Windows Audio**
2. Input: **<< none >>**
3. Output: **Apple DAC / speakers**

## Key Files
- `Source/ConnectView.cpp` — direct connect UI, saves last address
- `Source/SonobusPluginProcessor.cpp` — state save/load, connection logic
- `Source/ProcessAudioCapture.cpp/.h` — per-process audio capture (Win11 API)
- `Source/ApplicationAudioDevice.h` — Application Audio device type for JUCE
- `Source/SonoStandaloneFilterWindow.h` — where Application Audio type is registered
- `deps/juce/.../juce_WASAPI_windows.cpp` — WASAPI device enumeration & loopback capture
- `deps/aoo/` — Audio over OSC networking library
- `CMakeLists.txt` — build config

## Bugs Found & Fixed
- **JUCE crash: null device in getIndexOfDevice** — JUCE passes nullptr when no device open, our code called `device->getName()` on it
- **JUCE crash: empty output device list** — AudioDeviceSelectorComponent crashes if getDeviceNames(false) returns empty, need dummy output
- **WRL FtmBase required** — ActivateAudioInterfaceAsync returns 0x8000000E without free-threaded marshaling
- **GetMixFormat E_NOTIMPL** — process loopback doesn't support GetMixFormat, must use hardcoded format
- **COM apartment conflict** — don't call CoInitializeEx in getAudioProcesses, JUCE message thread is already STA
- **JUCE 8 shared mode loopback** — broken, reverted to JUCE 7
