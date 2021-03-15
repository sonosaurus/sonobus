// SPDX-License-Identifier: GPLv3-or-later
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
    float _lastgain = 0.0f;

    // panning (0.0 is centered, -1 is left, 1 is right)
    // used when numChannels != 2
    float pan[MAX_CHANNELS] = { 0.0f };
    // used when numChannels == 2
    float panStereo[2] = {-1.0f, 1.0f }; // only use 2
    float centerPanLaw = 0.596f; // center pan attentuation (default -4.5dB)

    int panDestStartIndex = 0; // destination channel index
    int panDestChannels = 2; // destination number of channels

    bool sendMainMix = true; // used for remote peers

    float _lastpan[MAX_CHANNELS] = { 0.0f };
    float _laststereopan[2] = { 0.0f };
    float _lastmonpan[MAX_CHANNELS] = { 0.0f };
    float _lastmonstereopan[2] = { 0.0f };


    // compressor (only used for 1 or 2 channel groups)
    CompressorParams compressorParams;

    // gate/expander
    CompressorParams expanderParams;

    // EQ (stereo only)
    ParametricEqParams eqParams;

    // limiter
    //faustLimiter mInputLimiter;
    CompressorParams limiterParams;

    // reverb send
    float reverbSend = 0.0f;

    // monitoring level
    float monitor = 1.0f;
    int monDestStartIndex = 0; // destination channel index
    int monDestChannels = 2; // destination number of channels

    float _lastmonitor = 0.0f;

    // monitoring delay
    DelayParams monitorDelayParams;


};


class ChannelGroup
{
public:

    ChannelGroup();

    void init(double sampleRate);

    void processBlock (AudioBuffer<float>& frombuffer, AudioBuffer<float>& tobuffer,  int destStartChan, int destNumChans, AudioBuffer<float>& silentBuffer, int numSamples, float gainfactor);
    void processPan (AudioBuffer<float>& frombuffer, int fromStartChan, AudioBuffer<float>& tobuffer, int destStartChan, int destNumChans, int numSamples, float gainfactor);
    void processMonitor (AudioBuffer<float>& frombuffer, int fromStartChan, AudioBuffer<float>& tobuffer, int destStartChan, int destNumChans, int numSamples, float gainfactor);

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

    float _lastgain = 0.0f;

    float _lastpan[MAX_CHANNELS] = { 0.0f };
    float _laststereopan[2] = { 0.0f };
    float _lastmonpan[MAX_CHANNELS] = { 0.0f };
    float _lastmonstereopan[2] = { 0.0f };


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
    Atomic<bool>  _monitorDelayTimeChanged = false;
    bool  _monitorDelayTimeChanging = false;
    Atomic<bool>  _monitorDelayActive  { false };
    bool   _monitorDelayLastActive = false;
    CriticalSection _monitorDelayLock;

    AudioBuffer<float> delayWorkBuffer;

    float _lastmonitor = 0.0f;

    double sampleRate = 48000.0;
};

}
