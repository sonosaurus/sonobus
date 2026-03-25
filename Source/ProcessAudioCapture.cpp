// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2026

#include "ProcessAudioCapture.h"

#if JUCE_WINDOWS

#include <functiondiscoverykeys_devpkey.h>
#include <audiopolicy.h>

// Forward declarations for Windows 11 per-process loopback API
// These are defined in audioclientactivationparams.h (Windows 11 SDK)
// We define them here to support building with older SDKs

#ifndef AUDIOCLIENT_ACTIVATION_TYPE_PROCESS_LOOPBACK
#define AUDIOCLIENT_ACTIVATION_TYPE_PROCESS_LOOPBACK 1

typedef enum PROCESS_LOOPBACK_MODE
{
    PROCESS_LOOPBACK_MODE_INCLUDE_TARGET_PROCESS_TREE = 0,
    PROCESS_LOOPBACK_MODE_EXCLUDE_TARGET_PROCESS_TREE = 1
} PROCESS_LOOPBACK_MODE;

typedef struct AUDIOCLIENT_PROCESS_LOOPBACK_PARAMS
{
    DWORD TargetProcessId;
    PROCESS_LOOPBACK_MODE ProcessLoopbackMode;
} AUDIOCLIENT_PROCESS_LOOPBACK_PARAMS;

typedef enum AUDIOCLIENT_ACTIVATION_TYPE
{
    AUDIOCLIENT_ACTIVATION_TYPE_DEFAULT = 0,
    AUDIOCLIENT_ACTIVATION_TYPE_PROCESS_LOOPBACK_TYPE = 1
} AUDIOCLIENT_ACTIVATION_TYPE;

typedef struct AUDIOCLIENT_ACTIVATION_PARAMS
{
    AUDIOCLIENT_ACTIVATION_TYPE ActivationType;
    union
    {
        AUDIOCLIENT_PROCESS_LOOPBACK_PARAMS ProcessLoopbackParams;
    };
} AUDIOCLIENT_ACTIVATION_PARAMS;

#endif // AUDIOCLIENT_ACTIVATION_TYPE_PROCESS_LOOPBACK

// Virtual audio device path for process loopback
static const LPCWSTR VIRTUAL_AUDIO_DEVICE_PROCESS_LOOPBACK = L"VAD\\Process_Loopback";

//==============================================================================
// IActivateAudioInterfaceCompletionHandler implementation
class LoopbackActivationHandler : public IActivateAudioInterfaceCompletionHandler
{
public:
    LoopbackActivationHandler()
    {
        completionEvent = CreateEvent (nullptr, TRUE, FALSE, nullptr);
    }

    ~LoopbackActivationHandler()
    {
        if (completionEvent != nullptr)
            CloseHandle (completionEvent);
    }

    // IUnknown
    ULONG STDMETHODCALLTYPE AddRef() override  { return InterlockedIncrement (&refCount); }
    ULONG STDMETHODCALLTYPE Release() override
    {
        auto count = InterlockedDecrement (&refCount);
        if (count == 0)
            delete this;
        return count;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface (REFIID riid, void** ppvObject) override
    {
        if (riid == __uuidof (IUnknown) || riid == __uuidof (IActivateAudioInterfaceCompletionHandler))
        {
            *ppvObject = static_cast<IActivateAudioInterfaceCompletionHandler*> (this);
            AddRef();
            return S_OK;
        }
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    // IActivateAudioInterfaceCompletionHandler
    HRESULT STDMETHODCALLTYPE ActivateCompleted (IActivateAudioInterfaceAsyncOperation* operation) override
    {
        HRESULT hrActivateResult = E_FAIL;
        IUnknown* activatedInterface = nullptr;

        HRESULT hr = operation->GetActivateResult (&hrActivateResult, &activatedInterface);

        if (SUCCEEDED (hr) && SUCCEEDED (hrActivateResult) && activatedInterface != nullptr)
        {
            activatedInterface->QueryInterface (__uuidof (IAudioClient), (void**) &resultClient);
        }

        activateResult = hrActivateResult;
        SetEvent (completionEvent);
        return S_OK;
    }

    bool waitForCompletion (DWORD timeoutMs = 5000)
    {
        return WaitForSingleObject (completionEvent, timeoutMs) == WAIT_OBJECT_0;
    }

    IAudioClient* getClient() { return resultClient; }
    HRESULT getResult() const { return activateResult; }

private:
    LONG refCount = 1;
    HANDLE completionEvent = nullptr;
    IAudioClient* resultClient = nullptr;
    HRESULT activateResult = E_FAIL;
};

//==============================================================================
ProcessAudioCapture::~ProcessAudioCapture()
{
    stopCapture();
}

bool ProcessAudioCapture::isSupported()
{
    // Check if ActivateAudioInterfaceAsync is available (Win8+)
    // and if the process loopback feature works (Win10 20348+ / Win11)
    HMODULE mmdevapi = GetModuleHandleW (L"mmdevapi.dll");
    if (mmdevapi == nullptr)
        mmdevapi = LoadLibraryW (L"mmdevapi.dll");

    if (mmdevapi == nullptr)
        return false;

    auto activateFunc = GetProcAddress (mmdevapi, "ActivateAudioInterfaceAsync");
    if (activateFunc == nullptr)
        return false;

    // Check Windows version - need build 20348+
    OSVERSIONINFOEXW osvi = {};
    osvi.dwOSVersionInfoSize = sizeof (osvi);

    using RtlGetVersionFunc = NTSTATUS (WINAPI*)(PRTL_OSVERSIONINFOW);
    auto ntdll = GetModuleHandleW (L"ntdll.dll");
    if (ntdll != nullptr)
    {
        auto rtlGetVersion = reinterpret_cast<RtlGetVersionFunc> (GetProcAddress (ntdll, "RtlGetVersion"));
        if (rtlGetVersion != nullptr)
        {
            rtlGetVersion (reinterpret_cast<PRTL_OSVERSIONINFOW> (&osvi));
            // Build 20348 is the minimum for process loopback
            return osvi.dwBuildNumber >= 20348;
        }
    }

    return false;
}

Array<ProcessAudioCapture::ProcessInfo> ProcessAudioCapture::getAudioProcesses()
{
    Array<ProcessInfo> result;

    // Get all running processes
    HANDLE snapshot = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return result;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof (pe);

    if (Process32FirstW (snapshot, &pe))
    {
        do
        {
            // Skip system processes
            if (pe.th32ProcessID == 0 || pe.th32ProcessID == 4)
                continue;

            String name = String (pe.szExeFile);

            // Skip known non-audio system processes
            if (name.equalsIgnoreCase ("svchost.exe")
                || name.equalsIgnoreCase ("csrss.exe")
                || name.equalsIgnoreCase ("smss.exe")
                || name.equalsIgnoreCase ("lsass.exe")
                || name.equalsIgnoreCase ("services.exe")
                || name.equalsIgnoreCase ("wininit.exe")
                || name.equalsIgnoreCase ("winlogon.exe")
                || name.equalsIgnoreCase ("dwm.exe")
                || name.equalsIgnoreCase ("System"))
                continue;

            ProcessInfo info;
            info.pid = pe.th32ProcessID;
            info.name = name;
            info.displayName = name + " (PID " + String (pe.th32ProcessID) + ")";
            result.add (info);

        } while (Process32NextW (snapshot, &pe));
    }

    CloseHandle (snapshot);

    // Sort by name using JUCE comparator
    struct ProcessInfoComparator
    {
        int compareElements (const ProcessInfo& a, const ProcessInfo& b) const
        {
            return a.name.compareIgnoreCase (b.name);
        }
    };

    ProcessInfoComparator comparator;
    result.sort (comparator);

    return result;
}

bool ProcessAudioCapture::startCapture (DWORD processId, double sampleRate, int numChannels, int bufferSize)
{
    if (! isSupported())
        return false;

    stopCapture();

    // Set up activation params for process loopback
    AUDIOCLIENT_ACTIVATION_PARAMS activationParams = {};
    activationParams.ActivationType = AUDIOCLIENT_ACTIVATION_TYPE_PROCESS_LOOPBACK_TYPE;
    activationParams.ProcessLoopbackParams.TargetProcessId = processId;
    activationParams.ProcessLoopbackParams.ProcessLoopbackMode = PROCESS_LOOPBACK_MODE_INCLUDE_TARGET_PROCESS_TREE;

    PROPVARIANT activateParamsPropVariant = {};
    activateParamsPropVariant.vt = VT_BLOB;
    activateParamsPropVariant.blob.cbSize = sizeof (activationParams);
    activateParamsPropVariant.blob.pBlobData = reinterpret_cast<BYTE*> (&activationParams);

    // Create completion handler
    auto handler = new LoopbackActivationHandler();
    IActivateAudioInterfaceAsyncOperation* asyncOp = nullptr;

    HRESULT hr = ActivateAudioInterfaceAsync (
        VIRTUAL_AUDIO_DEVICE_PROCESS_LOOPBACK,
        __uuidof (IAudioClient),
        &activateParamsPropVariant,
        handler,
        &asyncOp);

    if (FAILED (hr))
    {
        handler->Release();
        if (asyncOp) asyncOp->Release();
        return false;
    }

    // Wait for async activation to complete
    if (! handler->waitForCompletion (5000))
    {
        handler->Release();
        if (asyncOp) asyncOp->Release();
        return false;
    }

    if (FAILED (handler->getResult()) || handler->getClient() == nullptr)
    {
        handler->Release();
        if (asyncOp) asyncOp->Release();
        return false;
    }

    audioClient = handler->getClient();
    audioClient->AddRef(); // Take ownership

    handler->Release();
    if (asyncOp) asyncOp->Release();

    // Get mix format
    WAVEFORMATEX* mixFormat = nullptr;
    hr = audioClient->GetMixFormat (&mixFormat);
    if (FAILED (hr) || mixFormat == nullptr)
    {
        audioClient->Release();
        audioClient = nullptr;
        return false;
    }

    captureSampleRate = mixFormat->nSamplesPerSec;
    captureNumChannels = mixFormat->nChannels;

    // Initialize in shared mode (required for loopback)
    REFERENCE_TIME bufferDuration = 10000000LL; // 1 second buffer
    hr = audioClient->Initialize (
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
        bufferDuration,
        0,
        mixFormat,
        nullptr);

    CoTaskMemFree (mixFormat);

    if (FAILED (hr))
    {
        audioClient->Release();
        audioClient = nullptr;
        return false;
    }

    // Get capture client
    hr = audioClient->GetService (__uuidof (IAudioCaptureClient), (void**) &captureClient);
    if (FAILED (hr))
    {
        audioClient->Release();
        audioClient = nullptr;
        return false;
    }

    // Start capturing
    hr = audioClient->Start();
    if (FAILED (hr))
    {
        captureClient->Release();
        captureClient = nullptr;
        audioClient->Release();
        audioClient = nullptr;
        return false;
    }

    capturing.store (true);
    return true;
}

void ProcessAudioCapture::stopCapture()
{
    capturing.store (false);

    if (audioClient != nullptr)
    {
        audioClient->Stop();
    }

    if (captureClient != nullptr)
    {
        captureClient->Release();
        captureClient = nullptr;
    }

    if (audioClient != nullptr)
    {
        audioClient->Release();
        audioClient = nullptr;
    }
}

int ProcessAudioCapture::readSamples (AudioBuffer<float>& buffer, int numFrames)
{
    if (! capturing.load() || captureClient == nullptr)
        return 0;

    int framesRead = 0;

    while (framesRead < numFrames)
    {
        UINT32 packetLength = 0;
        HRESULT hr = captureClient->GetNextPacketSize (&packetLength);

        if (FAILED (hr) || packetLength == 0)
            break;

        BYTE* data = nullptr;
        UINT32 numFramesAvailable = 0;
        DWORD flags = 0;

        hr = captureClient->GetBuffer (&data, &numFramesAvailable, &flags, nullptr, nullptr);
        if (FAILED (hr))
            break;

        int framesToCopy = jmin ((int) numFramesAvailable, numFrames - framesRead);

        if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
        {
            // Fill with silence
            for (int ch = 0; ch < buffer.getNumChannels() && ch < captureNumChannels; ++ch)
                FloatVectorOperations::clear (buffer.getWritePointer (ch, framesRead), framesToCopy);
        }
        else if (data != nullptr)
        {
            // Convert interleaved float data to JUCE buffer
            const float* src = reinterpret_cast<const float*> (data);
            for (int frame = 0; frame < framesToCopy; ++frame)
            {
                for (int ch = 0; ch < buffer.getNumChannels() && ch < captureNumChannels; ++ch)
                    buffer.getWritePointer (ch)[framesRead + frame] = src[frame * captureNumChannels + ch];
            }
        }

        framesRead += framesToCopy;
        captureClient->ReleaseBuffer (numFramesAvailable);
    }

    return framesRead;
}

#endif // JUCE_WINDOWS
