// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2026

#include "ProcessAudioCapture.h"

#if JUCE_WINDOWS

// Ensure we get the Win10 FE (Iron/20H1) APIs for per-process loopback
#ifndef NTDDI_WIN10_FE
#define NTDDI_WIN10_FE 0x0A00000A
#endif
#if !defined(NTDDI_VERSION) || (NTDDI_VERSION < NTDDI_WIN10_FE)
#undef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WIN10_FE
#endif

#include <mmdeviceapi.h>
#include <audioclient.h>
#include <audioclientactivationparams.h>
#include <functiondiscoverykeys_devpkey.h>
#include <audiopolicy.h>
#include <wrl/implements.h>
#include <wrl/client.h>

using namespace Microsoft::WRL;

//==============================================================================
// Completion handler using WRL RuntimeClass with FtmBase for free-threaded marshaling.
// This is REQUIRED by ActivateAudioInterfaceAsync — a bare IUnknown implementation
// will return CO_E_NOT_SUPPORTED (0x8000000E).
class LoopbackActivationHandler :
    public RuntimeClass<RuntimeClassFlags<ClassicCom>, FtmBase, IActivateAudioInterfaceCompletionHandler>
{
public:
    HANDLE completionEvent = nullptr;
    IAudioClient* resultClient = nullptr;
    HRESULT activateResult = E_FAIL;

    LoopbackActivationHandler()
    {
        completionEvent = CreateEvent (nullptr, TRUE, FALSE, nullptr);
    }

    ~LoopbackActivationHandler()
    {
        if (completionEvent != nullptr)
            CloseHandle (completionEvent);
    }

    STDMETHOD(ActivateCompleted) (IActivateAudioInterfaceAsyncOperation* operation) override
    {
        HRESULT hrActivateResult = E_FAIL;
        IUnknown* activatedInterface = nullptr;

        HRESULT hr = operation->GetActivateResult (&hrActivateResult, &activatedInterface);

        if (SUCCEEDED (hr) && SUCCEEDED (hrActivateResult) && activatedInterface != nullptr)
            activatedInterface->QueryInterface (__uuidof (IAudioClient), (void**) &resultClient);

        activateResult = hrActivateResult;
        SetEvent (completionEvent);
        return S_OK;
    }

    bool waitForCompletion (DWORD timeoutMs = 10000)
    {
        return WaitForSingleObject (completionEvent, timeoutMs) == WAIT_OBJECT_0;
    }
};

//==============================================================================
static String getProcessNameFromPid (DWORD pid)
{
    HANDLE snapshot = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return {};

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof (pe);
    String name;

    if (Process32FirstW (snapshot, &pe))
    {
        do
        {
            if (pe.th32ProcessID == pid)
            {
                name = String (pe.szExeFile);
                break;
            }
        } while (Process32NextW (snapshot, &pe));
    }

    CloseHandle (snapshot);
    return name;
}

//==============================================================================
ProcessAudioCapture::~ProcessAudioCapture()
{
    stopCapture();
}

bool ProcessAudioCapture::isSupported()
{
    // Check if ActivateAudioInterfaceAsync is available
    HMODULE mmdevapi = GetModuleHandleW (L"mmdevapi.dll");
    if (mmdevapi == nullptr)
        mmdevapi = LoadLibraryW (L"mmdevapi.dll");

    if (mmdevapi == nullptr)
        return false;

    if (GetProcAddress (mmdevapi, "ActivateAudioInterfaceAsync") == nullptr)
        return false;

    // Check Windows build number using RtlGetVersion (reliable, not affected by manifests)
    using RtlGetVersionFunc = LONG (WINAPI*)(OSVERSIONINFOEXW*);
    auto ntdll = GetModuleHandleW (L"ntdll.dll");
    if (ntdll == nullptr)
        return false;

    auto rtlGetVersion = reinterpret_cast<RtlGetVersionFunc> (GetProcAddress (ntdll, "RtlGetVersion"));
    if (rtlGetVersion == nullptr)
        return false;

    OSVERSIONINFOEXW osvi = {};
    osvi.dwOSVersionInfoSize = sizeof (osvi);
    rtlGetVersion (&osvi);

    return osvi.dwBuildNumber >= 20348;
}

Array<ProcessAudioCapture::ProcessInfo> ProcessAudioCapture::getAudioProcesses()
{
    Array<ProcessInfo> result;

    // Use IAudioSessionManager2 to enumerate only processes with active audio sessions
    // Note: do NOT call CoInitializeEx here — JUCE's message thread already has COM initialized

    IMMDeviceEnumerator* enumerator = nullptr;
    HRESULT hr = CoCreateInstance (__uuidof (MMDeviceEnumerator), nullptr,
                                    CLSCTX_ALL, __uuidof (IMMDeviceEnumerator),
                                    (void**) &enumerator);
    if (FAILED (hr) || enumerator == nullptr)
        return result;

    IMMDevice* device = nullptr;
    hr = enumerator->GetDefaultAudioEndpoint (eRender, eConsole, &device);
    if (FAILED (hr) || device == nullptr)
    {
        enumerator->Release();
        return result;
    }

    IAudioSessionManager2* sessionManager = nullptr;
    hr = device->Activate (__uuidof (IAudioSessionManager2), CLSCTX_ALL,
                           nullptr, (void**) &sessionManager);
    if (FAILED (hr) || sessionManager == nullptr)
    {
        device->Release();
        enumerator->Release();
        return result;
    }

    IAudioSessionEnumerator* sessionEnumerator = nullptr;
    hr = sessionManager->GetSessionEnumerator (&sessionEnumerator);
    if (FAILED (hr) || sessionEnumerator == nullptr)
    {
        sessionManager->Release();
        device->Release();
        enumerator->Release();
        return result;
    }

    int sessionCount = 0;
    sessionEnumerator->GetCount (&sessionCount);

    Array<DWORD> seenPids;

    for (int i = 0; i < sessionCount; ++i)
    {
        IAudioSessionControl* sessionControl = nullptr;
        if (FAILED (sessionEnumerator->GetSession (i, &sessionControl)) || sessionControl == nullptr)
            continue;

        IAudioSessionControl2* sessionControl2 = nullptr;
        hr = sessionControl->QueryInterface (__uuidof (IAudioSessionControl2), (void**) &sessionControl2);
        sessionControl->Release();

        if (FAILED (hr) || sessionControl2 == nullptr)
            continue;

        if (sessionControl2->IsSystemSoundsSession() == S_OK)
        {
            sessionControl2->Release();
            continue;
        }

        DWORD pid = 0;
        hr = sessionControl2->GetProcessId (&pid);
        sessionControl2->Release();

        if (FAILED (hr) || pid == 0 || seenPids.contains (pid))
            continue;

        seenPids.add (pid);

        String name = getProcessNameFromPid (pid);
        if (name.isEmpty())
            continue;

        ProcessInfo info;
        info.pid = pid;
        info.name = name;
        info.displayName = name + " (PID " + String (pid) + ")";
        result.add (info);
    }

    sessionEnumerator->Release();
    sessionManager->Release();
    device->Release();
    enumerator->Release();

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
    activationParams.ActivationType = AUDIOCLIENT_ACTIVATION_TYPE_PROCESS_LOOPBACK;
    activationParams.ProcessLoopbackParams.TargetProcessId = processId;
    activationParams.ProcessLoopbackParams.ProcessLoopbackMode = PROCESS_LOOPBACK_MODE_INCLUDE_TARGET_PROCESS_TREE;

    PROPVARIANT activateParamsPropVariant = {};
    activateParamsPropVariant.vt = VT_BLOB;
    activateParamsPropVariant.blob.cbSize = sizeof (activationParams);
    activateParamsPropVariant.blob.pBlobData = reinterpret_cast<BYTE*> (&activationParams);

    // Create completion handler using WRL (FtmBase required for free-threaded marshaling)
    ComPtr<LoopbackActivationHandler> handler;
    HRESULT hr = MakeAndInitialize<LoopbackActivationHandler> (&handler);
    if (FAILED (hr))
        return false;

    IActivateAudioInterfaceAsyncOperation* asyncOp = nullptr;

    hr = ActivateAudioInterfaceAsync (
        VIRTUAL_AUDIO_DEVICE_PROCESS_LOOPBACK,
        __uuidof (IAudioClient),
        &activateParamsPropVariant,
        handler.Get(),
        &asyncOp);

    if (FAILED (hr))
    {
        if (asyncOp) asyncOp->Release();
        return false;
    }

    if (! handler->waitForCompletion (10000))
    {
        if (asyncOp) asyncOp->Release();
        return false;
    }

    if (FAILED (handler->activateResult) || handler->resultClient == nullptr)
    {
        if (asyncOp) asyncOp->Release();
        return false;
    }

    audioClient = handler->resultClient;
    audioClient->AddRef();

    if (asyncOp) asyncOp->Release();

    // GetMixFormat returns E_NOTIMPL for process loopback — use requested format
    WAVEFORMATEX* mixFormat = nullptr;
    hr = audioClient->GetMixFormat (&mixFormat);

    bool usingDefaultFormat = false;
    static WAVEFORMATEX defaultFormat = {};

    if (FAILED (hr) || mixFormat == nullptr)
    {
        defaultFormat.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
        defaultFormat.nChannels = 2;
        defaultFormat.nSamplesPerSec = (DWORD) sampleRate;
        defaultFormat.wBitsPerSample = 32;
        defaultFormat.nBlockAlign = defaultFormat.nChannels * defaultFormat.wBitsPerSample / 8;
        defaultFormat.nAvgBytesPerSec = defaultFormat.nSamplesPerSec * defaultFormat.nBlockAlign;
        defaultFormat.cbSize = 0;
        mixFormat = &defaultFormat;
        usingDefaultFormat = true;
    }

    captureSampleRate = mixFormat->nSamplesPerSec;
    captureNumChannels = mixFormat->nChannels;

    // Initialize with loopback flag (matches Microsoft's sample)
    REFERENCE_TIME bufferDuration = 10000000LL; // 1 second buffer
    hr = audioClient->Initialize (
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
        bufferDuration,
        0,
        mixFormat,
        nullptr);

    if (! usingDefaultFormat)
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
        audioClient->Stop();

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
            for (int ch = 0; ch < buffer.getNumChannels() && ch < captureNumChannels; ++ch)
                FloatVectorOperations::clear (buffer.getWritePointer (ch, framesRead), framesToCopy);
        }
        else if (data != nullptr)
        {
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
