// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2026

#pragma once

#include "JuceHeader.h"

#if JUCE_WINDOWS

#include <windows.h>
#include <tlhelp32.h>

// Forward declarations — actual Windows audio COM types are only used in .cpp
struct IAudioClient;
struct IAudioCaptureClient;

//==============================================================================
/**
    Per-process audio capture using Windows 11's AUDIOCLIENT_PROCESS_LOOPBACK_PARAMS.

    This captures audio from a specific Windows process (e.g. a game) without needing
    a virtual audio cable or loopback driver.

    Requires Windows 10 Build 20348+ / Windows 11.
*/
class ProcessAudioCapture
{
public:
    ProcessAudioCapture() = default;
    ~ProcessAudioCapture();

    //==============================================================================
    struct ProcessInfo
    {
        DWORD pid;
        String name;        // e.g. "SnowRunner.exe"
        String displayName; // e.g. "SnowRunner.exe (PID 1234)"
    };

    /** Returns a list of currently running processes. */
    static Array<ProcessInfo> getAudioProcesses();

    /** Returns true if the per-process capture API is available on this OS version. */
    static bool isSupported();

    //==============================================================================
    /** Start capturing audio from the given process ID.
        Returns true on success. */
    bool startCapture (DWORD processId, double sampleRate, int numChannels, int bufferSize);

    /** Stop capturing. */
    void stopCapture();

    /** Returns true if currently capturing. */
    bool isCapturing() const { return capturing.load(); }

    /** Read captured samples into the provided buffer.
        Returns the number of frames actually read. */
    int readSamples (AudioBuffer<float>& buffer, int numFrames);

    /** Get the capture format info. */
    double getSampleRate() const { return captureSampleRate; }
    int getNumChannels() const { return captureNumChannels; }

private:
    std::atomic<bool> capturing { false };
    double captureSampleRate = 0;
    int captureNumChannels = 0;

    IAudioClient* audioClient = nullptr;
    IAudioCaptureClient* captureClient = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessAudioCapture)
};

#endif // JUCE_WINDOWS
