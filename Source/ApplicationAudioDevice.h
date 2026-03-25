// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2026

#pragma once

#include "JuceHeader.h"

#if JUCE_WINDOWS

#include "ProcessAudioCapture.h"

//==============================================================================
/**
    An AudioIODevice that captures audio from a specific Windows process
    using the Windows 11 per-process loopback API.
*/
class ApplicationAudioDevice final : public AudioIODevice,
                                      private Thread
{
public:
    ApplicationAudioDevice (const String& processName, DWORD processId)
        : AudioIODevice ("Application Audio", "Application Audio"),
          Thread ("AppAudioCapture"),
          targetProcessName (processName),
          targetProcessId (processId)
    {
    }

    ~ApplicationAudioDevice() override
    {
        close();
    }

    //==============================================================================
    StringArray getOutputChannelNames() override    { return {}; }
    StringArray getInputChannelNames() override     { return { "Left", "Right" }; }

    Array<double> getAvailableSampleRates() override    { return { 44100.0, 48000.0, 96000.0 }; }
    Array<int> getAvailableBufferSizes() override       { return { 256, 512, 1024, 2048, 4096 }; }
    int getDefaultBufferSize() override                 { return 1024; }

    String open (const BigInteger& inputChannels,
                 const BigInteger& outputChannels,
                 double sampleRate,
                 int bufferSizeSamples) override
    {
        currentSampleRate = sampleRate;
        currentBufferSize = bufferSizeSamples;
        activeInputChannels = inputChannels;

        if (! capture.startCapture (targetProcessId, sampleRate, 2, bufferSizeSamples))
            return "Failed to start process audio capture. Requires Windows 10 Build 20348+ and the target process must be running.";

        isOpen_ = true;
        return {};
    }

    void close() override
    {
        stop();
        capture.stopCapture();
        isOpen_ = false;
    }

    bool isOpen() override                          { return isOpen_; }
    bool isPlaying() override                       { return isStarted; }

    int getCurrentBufferSizeSamples() override      { return currentBufferSize; }
    double getCurrentSampleRate() override          { return currentSampleRate; }
    int getCurrentBitDepth() override               { return 32; }

    BigInteger getActiveOutputChannels() const override    { return {}; }
    BigInteger getActiveInputChannels() const override     { return activeInputChannels; }

    int getOutputLatencyInSamples() override        { return 0; }
    int getInputLatencyInSamples() override         { return currentBufferSize; }

    bool hasControlPanel() const override           { return false; }
    bool showControlPanel() override                { return false; }
    bool setAudioPreprocessingEnabled (bool) override { return false; }

    String getLastError() override                  { return lastError; }

    void start (AudioIODeviceCallback* newCallback) override
    {
        if (newCallback != nullptr && isOpen_ && ! isStarted)
        {
            callback = newCallback;
            callback->audioDeviceAboutToStart (this);
            isStarted = true;
            startThread (Thread::Priority::high);
        }
    }

    void stop() override
    {
        if (isStarted)
        {
            isStarted = false;
            stopThread (2000);

            if (callback != nullptr)
            {
                callback->audioDeviceStopped();
                callback = nullptr;
            }
        }
    }

private:
    void run() override
    {
        AudioBuffer<float> inputBuffer (2, currentBufferSize);

        while (! threadShouldExit() && isStarted)
        {
            inputBuffer.clear();
            capture.readSamples (inputBuffer, currentBufferSize);

            const float* inputChannelData[2] = {
                inputBuffer.getReadPointer (0),
                inputBuffer.getReadPointer (1)
            };

            if (callback != nullptr)
            {
                callback->audioDeviceIOCallbackWithContext (inputChannelData, 2,
                                                            nullptr, 0,
                                                            currentBufferSize, {});
            }

            // Sleep based on buffer size to match expected callback rate
            auto sleepMs = (int) (1000.0 * currentBufferSize / currentSampleRate);
            Thread::sleep (jmax (1, sleepMs - 1));
        }
    }

    ProcessAudioCapture capture;
    String targetProcessName;
    DWORD targetProcessId;

    bool isOpen_ = false;
    bool isStarted = false;
    double currentSampleRate = 48000.0;
    int currentBufferSize = 1024;
    BigInteger activeInputChannels;
    String lastError;
    AudioIODeviceCallback* callback = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ApplicationAudioDevice)
};


//==============================================================================
/**
    AudioIODeviceType that enumerates running processes as input devices.
    Shows up as "Application Audio" in the Audio Device Type dropdown.
*/
class ApplicationAudioDeviceType final : public AudioIODeviceType
{
public:
    ApplicationAudioDeviceType()
        : AudioIODeviceType ("Application Audio")
    {
    }

    void scanForDevices() override
    {
        hasScanned = true;
        inputNames.clear();
        inputPids.clear();

        if (! ProcessAudioCapture::isSupported())
            return;

        auto processes = ProcessAudioCapture::getAudioProcesses();
        for (auto& proc : processes)
        {
            inputNames.add (proc.name + " (PID " + String (proc.pid) + ")");
            inputPids.add (proc.pid);
        }
    }

    StringArray getDeviceNames (bool wantInputNames) const override
    {
        return wantInputNames ? inputNames : StringArray();
    }

    int getDefaultDeviceIndex (bool) const override
    {
        return 0;
    }

    int getIndexOfDevice (AudioIODevice* device, bool asInput) const override
    {
        if (! asInput)
            return -1;

        for (int i = 0; i < inputNames.size(); ++i)
            if (inputNames[i] == device->getName())
                return i;

        return -1;
    }

    bool hasSeparateInputsAndOutputs() const override    { return true; }

    AudioIODevice* createDevice (const String& outputDeviceName,
                                 const String& inputDeviceName) override
    {
        auto index = inputNames.indexOf (inputDeviceName);

        if (index >= 0)
            return new ApplicationAudioDevice (inputDeviceName, inputPids[index]);

        return nullptr;
    }

private:
    bool hasScanned = false;
    StringArray inputNames;
    Array<DWORD> inputPids;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ApplicationAudioDeviceType)
};

#endif // JUCE_WINDOWS
