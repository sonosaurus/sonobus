// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2021 Jesse Chappell

#pragma once

#include "JuceHeader.h"

#include "faustCompressor.h"
#include "faustExpander.h"
#include "faustParametricEQ.h"
#include "faustLimiter.h"

#include "EffectParams.h"

namespace SonoAudio {

#ifndef MAX_CHANNELS
#define MAX_CHANNELS 64
#endif

// used to encapsulate audio processing for grouped set of audio
// is used for both local or remote audio input

struct ChannelGroupParams
{

    void setToDefaults(bool isplugin);

    ValueTree getValueTree() const;
    void setFromValueTree(const ValueTree & item);

    String getValueTreeKey() const;

    ValueTree getChannelLayoutValueTree();
    void setFromChannelLayoutValueTree(const ValueTree & layoutval);

    // group info
    String name;
    int chanStartIndex = 0; // source channel
    int numChannels = 1;

    bool muted = false;
    bool soloed = false;

    // input gain
    float gain = 1.0f;

    // panning (0.0 is centered, -1 is left, 1 is right)
    // used when numChannels != 2
    float pan[MAX_CHANNELS] = { 0.0f };
    // used when numChannels == 2
    float panStereo[2] = {-1.0f, 1.0f }; // only use 2
    float centerPanLaw = 0.596f; // center pan attenuation (default -4.5dB)

    int panDestStartIndex = 0; // destination channel index
    int panDestChannels = 2; // destination number of channels

    bool sendMainMix = true; // used for remote peers

    // compressor (only used for 1 or 2 channel groups)
    CompressorParams compressorParams;

    // gate/expander
    CompressorParams expanderParams;

    // EQ (stereo only)
    ParametricEqParams eqParams;

    // limiter
    //faustLimiter mInputLimiter;
    CompressorParams limiterParams;

    // polarity invert
    bool invertPolarity = false;

    // reverb send
    float inReverbSend = 0.0f;
    float monReverbSend = 0.0f;

    // monitoring level
    float monitor = 1.0f;
    int monDestStartIndex = 0; // destination channel index
    int monDestChannels = 2; // destination number of channels

    // monitoring delay
    DelayParams monitorDelayParams;


};


class ChannelGroup
{
public:

    ChannelGroup();


    void init(double sampleRate);

    struct ProcessState
    {
        float lastlevel = 0.0f;
        float lastpan[MAX_CHANNELS] = { 0.0f };
        float laststereopan[2] = { 0.0f };
    };


    void processBlock (AudioBuffer<float>& frombuffer, AudioBuffer<float>& tobuffer,  int destStartChan, int destNumChans, AudioBuffer<float>& silentBuffer, int numSamples, float gainfactor, ProcessState * procstate=nullptr, AudioBuffer<float> * reverbbuffer=nullptr, int revStartChan=0, int revNumChans=2, bool revEnabled=false, float revgainfactor=1.0f, ProcessState * revprocstate=nullptr);

    void processPan (AudioBuffer<float>& frombuffer, int fromStartChan, AudioBuffer<float>& tobuffer, int destStartChan, int destNumChans, int numSamples, float gainfactor, ProcessState * procstate=nullptr);


    void processMonitor (AudioBuffer<float>& frombuffer, int fromStartChan, AudioBuffer<float>& tobuffer, int destStartChan, int destNumChans, int numSamples, float gainfactor, ProcessState * procstate = nullptr, AudioBuffer<float> * reverbbuffer=nullptr, int revStartChan=0, int revNumChans=2, bool revEnabled=false, float revgainfactor=1.0f, ProcessState * revprocstate=nullptr);

    void processReverbSend (AudioBuffer<float>& frombuffer, int fromStartChan, int fromNumChans, AudioBuffer<float>& tobuffer, int destStartChan, int destNumChans, int numSamples, bool revEnabled, bool inSend, float gainfactor=1.0f, ProcessState * procstate = nullptr);

    // shallow copy of parameters and state
    void copyParametersFrom(const ChannelGroup& other);

    void commitAllParams();
    
    void commitCompressorParams();
    void commitExpanderParams();
    void commitLimiterParams();
    void commitEqParams();
    void commitMonitorDelayParams();

    void setMonitoringDelayEnabled(bool enabled, int numchans);
    void setMonitoringDelayTimeMs(double delayms);

    ChannelGroupParams params;

    ProcessState mainProcState;
    ProcessState monProcState;
    ProcessState inRevProcState;
    ProcessState revProcState;

    // compressor (only used for 1 or 2 channel groups)
    std::unique_ptr<faustCompressor> compressor;
    std::unique_ptr<MapUI> compressorControl;
    float * compressorOutputLevel = nullptr;
    bool compressorParamsChanged = false;
    bool _lastCompressorEnabled = false;

    // gate/expander
    std::unique_ptr<faustExpander> expander;
    std::unique_ptr<MapUI>  expanderControl;
    bool expanderParamsChanged = false;
    bool _lastExpanderEnabled = false;
    float * expanderOutputGain = nullptr;

    // EQ (stereo only)
    std::unique_ptr<faustParametricEQ> eq[2];
    std::unique_ptr<MapUI>  eqControl[2];
    bool eqParamsChanged = false;
    bool _lastEqEnabled = false;

    // limiter
    //faustLimiter mInputLimiter;
    std::unique_ptr<faustCompressor> limiter;
    std::unique_ptr<MapUI>  limiterControl;
    bool limiterParamsChanged = false;
    bool _lastLimiterEnabled = false;

    // monitoring delay
    std::unique_ptr<juce::dsp::DelayLine<float,juce::dsp::DelayLineInterpolationTypes::None> > monitorDelayLine;
    bool monitorDelayParamsChanged = false;
    double _monitorDelayTimeSamples = 0.0;
    int _monitorDelayChans = 0;
    std::atomic<bool>  _monitorDelayTimeChanged { false };
    bool  _monitorDelayTimeChanging = false;
    std::atomic<bool>  _monitorDelayActive  { false };
    bool   _monitorDelayLastActive = false;
    CriticalSection _monitorDelayLock;

    AudioBuffer<float> delayWorkBuffer;


    double sampleRate = 48000.0;
};

}
