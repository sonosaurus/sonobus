// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2021 Jesse Chappell


#include "ChannelGroup.h"

static String nameKey("name");

static String gainKey("gain");
static String channelStartIndexKey("chanstart");
static String numChannelsKey("numchan");
static String monReverbSendKey("monreverbsend");
static String inReverbSendKey("inreverbsend");
static String monitorLevelKey("monitorlev");
static String peerMonoPanKey("pan");
static String peerPan1Key("span1");
static String peerPan2Key("span2");
static String panDestStartKey("pandeststart");
static String panDestChannelsKey("pandestchans");
static String monDestStartKey("mondeststart");
static String monDestChannelsKey("mondestchans");
static String sendMainMixKey("sendmainmix");
static String invertPolarityKey("invertpolarity");


static String stereoPanListKey("StereoPanners");

static String panStateKey("Panner");
static String panAttrKey("pan");


static String compressorStateKey("CompressorState");
static String expanderStateKey("ExpanderState");
static String limiterStateKey("LimiterState");
static String eqStateKey("ParametricEqState");
static String monDelayStateKey("MonitorDelayState");

static String inputChannelGroupsStateKey("InputChannelGroups");
static String channelGroupStateKey("ChannelGroup");
static String numChanGroupsKey("numChanGroups");

static String layoutGroupsKey("Layout");


#define MAX_DELAY_SAMPLES 240000 // 5 seconds at 48k

using namespace SonoAudio;

ChannelGroup::ChannelGroup()
{
}

// copy assignment
void ChannelGroup::copyParametersFrom(const ChannelGroup& other)
{
    params = other.params;

    compressorParamsChanged = true;
    expanderParamsChanged = true;
    eqParamsChanged = true;
    limiterParamsChanged = true;
    monitorDelayParamsChanged = true;

}

void ChannelGroupParams::setToDefaults(bool isplugin)
{
    // default expander params
    expanderParams.thresholdDb = -60.0f;
    expanderParams.releaseMs = 200.0f;
    expanderParams.attackMs = 1.0f;
    expanderParams.ratio = 2.0f;

    if (compressorParams.automakeupGain) {
        // makeupgain = (-thresh -  abs(thresh/ratio))/2
        compressorParams.makeupGainDb = (-compressorParams.thresholdDb - abs(compressorParams.thresholdDb/compressorParams.ratio)) * 0.5f;
    }

    limiterParams.enabled = !isplugin; // input limiter disabled by default in plugin
    limiterParams.thresholdDb = -1.0f;
    limiterParams.ratio = 4.0f;
    limiterParams.attackMs = 0.01;
    limiterParams.releaseMs = 100.0;

    monitorDelayParams.enabled = false;
    monitorDelayParams.delayTimeMs = 0.0;
}

void ChannelGroup::init(double sampRate)
{
    sampleRate = sampRate;
    
    if (!compressor) {
        compressor = std::make_unique<faustCompressor>();
        compressorControl = std::make_unique<MapUI>();
    }
    compressor->init(sampleRate);
    compressor->buildUserInterface(compressorControl.get());

    //DBG("Compressor Params:");
    //for(int i=0; i < mInputCompressorControl.getParamsCount(); i++){
    //    DBG(mInputCompressorControl.getParamAddress(i));
    //}

    if (!expander) {
        expander = std::make_unique<faustExpander>();
        expanderControl = std::make_unique<MapUI>();
    }

    expander->init(sampleRate);
    expander->buildUserInterface(expanderControl.get());

    //DBG("Expander Params:");
    //for(int i=0; i < mInputExpanderControl.getParamsCount(); i++){
    //    DBG(mInputExpanderControl.getParamAddress(i));
    //}

    for (int j=0; j < 2; ++j) {
        if (!eq[j]) {
            eq[j] = std::make_unique<faustParametricEQ>();
            eqControl[j] = std::make_unique<MapUI>();
        }
        eq[j]->init(sampleRate);
        eq[j]->buildUserInterface(eqControl[j].get());
    }

    //DBG("EQ Params:");
    //for(int i=0; i < mInputEqControl[0].getParamsCount(); i++){
    //    DBG(mInputEqControl[0].getParamAddress(i));
    //}

    if (!limiter) {
        limiter = std::make_unique<faustCompressor>();
        limiterControl = std::make_unique<MapUI>();
    }

    limiter->init(sampleRate);
    limiter->buildUserInterface(limiterControl.get());

    //DBG("Limiter Params:");
    //for(int i=0; i < mInputLimiterControl.getParamsCount(); i++){
    //    DBG(mInputLimiterControl.getParamAddress(i));
    //}

    commitCompressorParams();
    commitExpanderParams();
    commitEqParams();
    commitLimiterParams();
    commitMonitorDelayParams();

}

void ChannelGroup::setMonitoringDelayEnabled(bool enabled, int numchans)
{
    if (enabled) {
        if (!monitorDelayLine) {
            const ScopedLock lock(_monitorDelayLock);
            // need to initialize it
            monitorDelayLine = std::make_unique<juce::dsp::DelayLine<float,juce::dsp::DelayLineInterpolationTypes::None> >(MAX_DELAY_SAMPLES);
            monitorDelayLine->setDelay(_monitorDelayTimeSamples);

            juce::dsp::ProcessSpec delayspec = { sampleRate, 4096, (juce::uint32) numchans };
            monitorDelayLine->prepare(delayspec);

            _monitorDelayChans = numchans;
            delayWorkBuffer.setSize(numchans, 4096, false, false, true);
        }
        else if (_monitorDelayChans != numchans) {
            //
            const ScopedLock lock(_monitorDelayLock);

            juce::dsp::ProcessSpec delayspec = { sampleRate, 4096, (juce::uint32) numchans };
            monitorDelayLine->prepare(delayspec);
            monitorDelayLine->reset();
            _monitorDelayChans = numchans;
            delayWorkBuffer.setSize(numchans, 4096, false, false, true);
        }

        


        // if monitorDelayActive is ever true, it means it has already been initialized
        params.monitorDelayParams.enabled = true;
        _monitorDelayActive = true;
    }
    else {
        params.monitorDelayParams.enabled = false;
        _monitorDelayActive = false;
    }
}

void ChannelGroup::setMonitoringDelayTimeMs(double delayms)
{
    params.monitorDelayParams.delayTimeMs = delayms;
    auto newsamps = 1e-3 * delayms * sampleRate;
    if ( fabs(_monitorDelayTimeSamples - newsamps) > 1) {
        _monitorDelayTimeSamples = jmin(newsamps, (double)MAX_DELAY_SAMPLES);
        _monitorDelayTimeChanged = true;
    }
}


void ChannelGroup::processBlock (AudioBuffer<float>& frombuffer,
                                 AudioBuffer<float>& tobuffer, int destStartChan, int destNumChans,
                                 AudioBuffer<float>& silentBuffer,
                                 int numSamples, float gainfactor, ProcessState * oprocstate,
                                 AudioBuffer<float> * reverbbuffer, int revStartChan, int revNumChans, bool revEnabled, float revgainfactor, ProcessState * orevprocstate)
{
    // called from audio thread context

    auto & procstate = oprocstate != nullptr ? *oprocstate : mainProcState;
    auto & revprocstate = orevprocstate != nullptr ? *orevprocstate : inRevProcState;

    int chstart = params.chanStartIndex;
    int numchan = params.numChannels;
    const int frombufNumChan = frombuffer.getNumChannels();
    const int tobufNumChan = tobuffer.getNumChannels();
    // apply input gain


    float dogain = (params.muted ? 0.0f : params.gain) * gainfactor;
    //dogain = 0.0f;

    dogain *= params.invertPolarity ? -1.0f : 1.0f;

    if (&frombuffer == &tobuffer) {
        // inplace, just apply gain, ignore destchans
        for (int i = chstart; i < chstart+numchan && i < frombufNumChan ; ++i) {
            tobuffer.applyGainRamp(i, 0, numSamples, procstate.lastlevel, dogain);
        }
    }
    else {
        for (int i = chstart, desti=destStartChan; i < chstart+numchan && i < frombufNumChan && desti < destStartChan+destNumChans && desti < tobufNumChan; ++i, ++desti) {
            tobuffer.addFromWithRamp(desti, 0, frombuffer.getReadPointer(i), numSamples, procstate.lastlevel, dogain);
        }
    }


    procstate.lastlevel = dogain;

    // these all operate ONLY when the channel group has 1 or 2 channels (and when the effects have been initialized)
    if (params.numChannels > 0 && params.numChannels <= 2 && compressor)
    {
        // apply input expander
        if (expanderParamsChanged) {
            commitExpanderParams();
            expanderParamsChanged = false;
        }
        if (_lastExpanderEnabled || params.expanderParams.enabled) {
            if (tobufNumChan - destStartChan > 1 && numchan == 2 && destNumChans >= 2) {
                float *bufs[2] = { tobuffer.getWritePointer(destStartChan), tobuffer.getWritePointer(destStartChan+1)};
                expander->compute(numSamples, bufs, bufs);
            } else if (destStartChan < tobufNumChan) {
                float *bufs[2] = { tobuffer.getWritePointer(destStartChan), silentBuffer.getWritePointer(0) }; // just a silent dummy buffer
                expander->compute(numSamples, bufs, bufs);
            }
        }
        _lastExpanderEnabled = params.expanderParams.enabled;


        // apply input compressor
        if (compressorParamsChanged) {
            commitCompressorParams();
            compressorParamsChanged = false;
        }
        if (_lastCompressorEnabled || params.compressorParams.enabled) {
            if (tobufNumChan - destStartChan > 1 && numchan == 2 && destNumChans >= 2) {
                float *bufs[2] = { tobuffer.getWritePointer(destStartChan), tobuffer.getWritePointer(destStartChan+1)};
                compressor->compute(numSamples, bufs, bufs);
            } else if (destStartChan < tobufNumChan) {
                float *bufs[2] = { tobuffer.getWritePointer(destStartChan), silentBuffer.getWritePointer(0) }; // just a silent dummy buffer
                compressor->compute(numSamples, bufs, bufs);
            }
        }
        _lastCompressorEnabled = params.compressorParams.enabled;


        // apply input EQ
        if (eqParamsChanged) {
            commitEqParams();
            eqParamsChanged = false;
        }
        if (_lastEqEnabled || params.eqParams.enabled) {
            if (tobufNumChan - destStartChan > 1 && numchan == 2 && destNumChans >= 2) {
                // only 2 channels support for now... TODO
                float *bufs[2] = { tobuffer.getWritePointer(destStartChan), tobuffer.getWritePointer(destStartChan+1)};
                eq[0]->compute(numSamples, &bufs[0], &bufs[0]);
                eq[1]->compute(numSamples, &bufs[1], &bufs[1]);
            } else if (destStartChan < tobufNumChan) {
                float *inbuf = tobuffer.getWritePointer(destStartChan);
                float *outbuf = tobuffer.getWritePointer(destStartChan);
                eq[0]->compute(numSamples, &inbuf, &outbuf);
            }
        }
        _lastEqEnabled = params.eqParams.enabled;


        // apply input limiter
        if (limiterParamsChanged) {
            commitLimiterParams();
            limiterParamsChanged = false;
        }
        if (_lastLimiterEnabled || params.limiterParams.enabled) {
            if (tobufNumChan - destStartChan > 1 && numchan == 2 && destNumChans >= 2) {
                float *bufs[2] = { tobuffer.getWritePointer(destStartChan), tobuffer.getWritePointer(destStartChan+1)};
                limiter->compute(numSamples, bufs, bufs);
            } else if (destStartChan < tobufNumChan) {
                float *bufs[2] = { tobuffer.getWritePointer(destStartChan), silentBuffer.getWritePointer(0) }; // just a silent dummy buffer
                limiter->compute(numSamples, bufs, bufs);
            }
        }
        _lastLimiterEnabled = params.limiterParams.enabled;
    }
    
    // apply to reverb buffer
    if (reverbbuffer) {
        processReverbSend(tobuffer, destStartChan, jmin(params.numChannels, destNumChans), *reverbbuffer, revStartChan, revNumChans, numSamples, revEnabled, true, revgainfactor, &revprocstate);
    }
}

void ChannelGroup::processPan (AudioBuffer<float>& frombuffer, int fromStartChan,
                               AudioBuffer<float>& tobuffer, int destStartChan, int destNumChans,
                               int numSamples, float gainfactor, ProcessState * oprocstate)
{
    int fromNumChan = frombuffer.getNumChannels();
    int toNumChan = tobuffer.getNumChannels();

    if (fromNumChan == 0) return;

    auto & procstate = oprocstate != nullptr ? *oprocstate : mainProcState;


    if (destNumChans == 2) {
        //tobuffer.clear(0, numSamples);

        for (int channel = destStartChan; channel < destStartChan + destNumChans && channel < toNumChan; ++channel) {
            int pani = 0;
            for (int i=fromStartChan; i < fromStartChan + params.numChannels && i < fromNumChan; ++i, ++pani) {
                const float upan = (params.numChannels != 2 ? params.pan[pani] : i==fromStartChan ? params.panStereo[0] : params.panStereo[1]);
                const float lastpan = (params.numChannels != 2 ? procstate.lastpan[pani] : i==fromStartChan ? procstate.laststereopan[0] : procstate.laststereopan[1]);

                // -1 is left, 1 is right
                float pgain = channel == destStartChan ? (upan >= 0.0f ? (1.0f - upan) : 1.0f) : (upan >= 0.0f ? 1.0f : (1.0f+upan)) ;

                // apply pan law
                pgain *= params.centerPanLaw + (fabsf(upan) * (1.0f - params.centerPanLaw));
                pgain *= gainfactor;

                if (fabsf(upan - lastpan) > 0.00001f) {
                    float plastgain = channel == destStartChan ? (lastpan >= 0.0f ? (1.0f - lastpan) : 1.0f) : (lastpan >= 0.0f ? 1.0f : (1.0f+lastpan));
                    plastgain *= params.centerPanLaw + (fabsf(lastpan) * (1.0f - params.centerPanLaw));
                    plastgain *= gainfactor;

                    tobuffer.addFromWithRamp(channel, 0, frombuffer.getReadPointer(i), numSamples, plastgain, pgain);
                } else {
                    tobuffer.addFrom (channel, 0, frombuffer, i, 0, numSamples, pgain);
                }
            }
        }
    }
    else if (destNumChans == 1){
        // sum all into destChan
        int channel = destStartChan;
        for (int srcchan = fromStartChan; srcchan < fromStartChan + params.numChannels && srcchan < fromNumChan && channel < toNumChan; ++srcchan) {

            tobuffer.addFrom(channel, 0, frombuffer, srcchan, 0, numSamples, gainfactor);
        }
    }
    else {
        // straight thru to dests - no panning
        int srcchan = fromStartChan;
        for (int channel = destStartChan; srcchan < fromStartChan + params.numChannels && srcchan < fromNumChan && channel < toNumChan; ++channel, ++srcchan) {
            //int srcchan = channel < mainBusInputChannels ? channel : mainBusInputChannels - 1;
            //int srcchan = chanStartIndex  channel < mainBusInputChannels ? channel : mainBusInputChannels - 1;

            tobuffer.addFrom(channel, 0, frombuffer, srcchan, 0, numSamples, gainfactor);
        }
    }
    

    procstate.laststereopan[0] = params.panStereo[0];
    procstate.laststereopan[1] = params.panStereo[1];
    for (int pani=0; pani < params.numChannels; ++pani) {
        procstate.lastpan[pani] = params.pan[pani];
    }
}

void ChannelGroup::processMonitor (AudioBuffer<float>& frombuffer, int fromStartChan,
                                   AudioBuffer<float>& tobuffer, int destStartChan, int destNumChans,
                                   int numSamples, float gainfactor, ProcessState * oprocstate,
                                   AudioBuffer<float> * reverbbuffer, int revStartChan, int revNumChans, bool revEnabled, float revgainfactor, ProcessState * orevprocstate)
{

    // apply monitor level

    float targmon = params.monitor * gainfactor;

    int fromNumChan = frombuffer.getNumChannels();
    int toNumChan = tobuffer.getNumChannels();

    auto & procstate = oprocstate != nullptr ? *oprocstate : monProcState;
    auto & revprocstate = orevprocstate != nullptr ? *orevprocstate : revProcState;


    if (monitorDelayParamsChanged) {
        commitMonitorDelayParams();
        monitorDelayParamsChanged = false;
    }

    bool domondelay = _monitorDelayActive.load();
    int mondelayfade = 0;

    auto * usefrombuffer = &frombuffer;
    auto useFromStartChan = fromStartChan;
    auto useFromNumChan = fromNumChan;

    if (domondelay || _monitorDelayLastActive != domondelay) {
        const ScopedTryLock lock(_monitorDelayLock);
        if (lock.isLocked()) {

            // transition between delayed and not
            if (_monitorDelayLastActive != domondelay) {
                if (domondelay) {
                    monitorDelayLine->reset();
                    mondelayfade = 1; // fade in
                }
                else {
                    mondelayfade = -1; // fade out
                }
            }

            if (domondelay) {
                if (_monitorDelayTimeChanged.load() && !_monitorDelayTimeChanging) {
                    _monitorDelayTimeChanged = false;
                    mondelayfade = -1.0f; // fade out
                    _monitorDelayTimeChanging = true;
                }
                else if (_monitorDelayTimeChanging) {
                    // already faded out, now apply new delay time
                    _monitorDelayTimeChanging = false;
                    monitorDelayLine->setDelay(_monitorDelayTimeSamples);
                    mondelayfade = 1; // fade in
                }
            }

            if ((domondelay || mondelayfade != 0)) {
                // should match dest num channels, but we need to make sure of it
                if (params.numChannels == _monitorDelayChans) {
                    auto delayinoutBlock = dsp::AudioBlock<float>(delayWorkBuffer).getSubsetChannelBlock(0, (size_t)params.numChannels).getSubBlock(0, numSamples);

                    // apply it going in
                    for (int srcchan = fromStartChan, chan = 0; chan < delayWorkBuffer.getNumChannels() && srcchan < fromStartChan + params.numChannels && srcchan < fromNumChan; ++srcchan, ++chan) {
                        if (mondelayfade != 0) {
                            delayWorkBuffer.copyFromWithRamp(chan, 0, frombuffer.getReadPointer(srcchan), numSamples,
                                mondelayfade > 0 ? 0.0f : 1.0f, mondelayfade > 0 ? 1.0f : 0.0f);
                            //tobuffer.applyGainRamp(0, numSamples, mondelayfade > 0 ? 0.0f : 1.0f, mondelayfade > 0 ? 1.0f : 0.0f);
                        }
                        else {
                            delayWorkBuffer.copyFrom(chan, 0, frombuffer.getReadPointer(srcchan), numSamples);
                        }
                    }

                    monitorDelayLine->process(dsp::ProcessContextReplacing<float>(delayinoutBlock));

                    // apply it coming out
                    for (int chan = 0; chan < params.numChannels && chan < delayWorkBuffer.getNumChannels(); ++chan) {
                        if (mondelayfade != 0) {
                            delayWorkBuffer.applyGainRamp(chan, 0, numSamples, mondelayfade > 0 ? 0.0f : 1.0f, mondelayfade > 0 ? 1.0f : 0.0f);
                        }
                    }

                    usefrombuffer = &delayWorkBuffer;
                    useFromStartChan = 0;
                    useFromNumChan = jmin(params.numChannels, delayWorkBuffer.getNumChannels());
                }
            }

            _monitorDelayLastActive = domondelay;

            
        }
    }

    if (useFromNumChan > 0 && destNumChans == 2) {
        //tobuffer.clear(0, numSamples);

        for (int channel = destStartChan; channel < destStartChan + destNumChans && channel < toNumChan; ++channel) {
            int pani = 0;
            for (int i=useFromStartChan; i < useFromStartChan + params.numChannels && i < useFromNumChan; ++i, ++pani) {
                const float upan = (params.numChannels != 2 ? params.pan[pani] : i==useFromStartChan ? params.panStereo[0] : params.panStereo[1]);
                const float lastpan = (params.numChannels != 2 ? procstate.lastpan[pani] : i==useFromStartChan ? procstate.laststereopan[0] : procstate.laststereopan[1]);

                // -1 is left, 1 is right
                float pgain = channel == destStartChan ? (upan >= 0.0f ? (1.0f - upan) : 1.0f) : (upan >= 0.0f ? 1.0f : (1.0f+upan)) ;

                // apply pan law
                pgain *= params.centerPanLaw + (fabsf(upan) * (1.0f - params.centerPanLaw));
                pgain *= targmon;

                if (fabsf(upan - lastpan) > 0.00001f || fabsf(procstate.lastlevel - targmon) > 0.00001f) {
                    float plastgain = channel == destStartChan ? (lastpan >= 0.0f ? (1.0f - lastpan) : 1.0f) : (lastpan >= 0.0f ? 1.0f : (1.0f+lastpan));
                    plastgain *= params.centerPanLaw + (fabsf(lastpan) * (1.0f - params.centerPanLaw));
                    plastgain *= procstate.lastlevel;

                    tobuffer.addFromWithRamp(channel, 0, usefrombuffer->getReadPointer(i), numSamples, plastgain, pgain);
                } else {
                    tobuffer.addFrom (channel, 0, *usefrombuffer, i, 0, numSamples, pgain);
                }
            }
        }
    }
    else if (useFromNumChan > 0 && destNumChans == 1){
        // sum all into destChan
        int channel = destStartChan;
        for (int srcchan = useFromStartChan; srcchan < useFromStartChan + params.numChannels && srcchan < useFromNumChan && channel < toNumChan; ++srcchan) {

            tobuffer.addFromWithRamp(channel, 0, usefrombuffer->getReadPointer(srcchan), numSamples, procstate.lastlevel, targmon);

        }
    }
    else if (useFromNumChan > 0){
        // straight thru to dests - no panning
        int srcchan = useFromStartChan;
        for (int channel = destStartChan; srcchan < useFromStartChan + params.numChannels && srcchan < useFromNumChan && channel < toNumChan; ++channel, ++srcchan) {
//        for (int channel = destStartChan + chanStartIndex; channel < destStartChan + destNumChans && srcchan < chanStartIndex + numChannels && srcchan < fromNumChan && channel < toNumChan; ++channel, ++srcchan) {
            //int srcchan = channel < mainBusInputChannels ? channel : mainBusInputChannels - 1;
            //int srcchan = chanStartIndex  channel < mainBusInputChannels ? channel : mainBusInputChannels - 1;

            tobuffer.addFromWithRamp(channel, 0, usefrombuffer->getReadPointer(srcchan), numSamples, procstate.lastlevel, targmon);
        }
    }

    // apply to reverb buffer
    if (reverbbuffer) {
        processReverbSend(*usefrombuffer, useFromStartChan, jmin(params.numChannels, useFromNumChan), *reverbbuffer, revStartChan, revNumChans, numSamples, revEnabled, false, targmon * revgainfactor, &revprocstate);
    }

    procstate.laststereopan[0] = params.panStereo[0];
    procstate.laststereopan[1] = params.panStereo[1];
    for (int pani=0; pani < params.numChannels; ++pani) {
        procstate.lastpan[pani] = params.pan[pani];
    }

    procstate.lastlevel = targmon;

}

void ChannelGroup::processReverbSend (AudioBuffer<float>& frombuffer, int fromStartChan, int fromNumChans,
                                      AudioBuffer<float>& tobuffer, int destStartChan, int destNumChans,
                                      int numSamples, bool revEnabled, bool inSend, float gainfactor,  ProcessState * oprocstate)
{
    int fromMaxChans = frombuffer.getNumChannels();
    int destMaxChans = tobuffer.getNumChannels();

    auto & procstate = oprocstate != nullptr ? *oprocstate : revProcState;

    const float targrevgain = gainfactor * (inSend ? params.inReverbSend : params.monReverbSend) * (revEnabled ? 1.0f : 0.0f);
    const float lastrevgain = procstate.lastlevel;

    if (fromNumChans > 0 && destNumChans == 2) {
        //tobuffer.clear(0, numSamples);

        for (int channel = destStartChan; channel < destStartChan + destNumChans && channel < destMaxChans; ++channel) {
            int pani = 0;
            for (int i=fromStartChan; i < fromStartChan + fromNumChans && i < fromMaxChans; ++i, ++pani) {
                const float upan = (fromNumChans != 2 ? params.pan[pani] : i==fromStartChan ? params.panStereo[0] : params.panStereo[1]);
                const float lastpan = (params.numChannels != 2 ? procstate.lastpan[pani] : i==fromStartChan ? procstate.laststereopan[0] : procstate.laststereopan[1]);

                // -1 is left, 1 is right
                float pgain = channel == destStartChan ? (upan >= 0.0f ? (1.0f - upan) : 1.0f) : (upan >= 0.0f ? 1.0f : (1.0f+upan)) ;

                // apply pan law
                pgain *= params.centerPanLaw + (fabsf(upan) * (1.0f - params.centerPanLaw));
                pgain *= targrevgain;

                if (fabsf(upan - lastpan) > 0.00001f || fabsf(lastrevgain - targrevgain) > 0.00001f) {
                    float plastgain = channel == destStartChan ? (lastpan >= 0.0f ? (1.0f - lastpan) : 1.0f) : (lastpan >= 0.0f ? 1.0f : (1.0f+lastpan));
                    plastgain *= params.centerPanLaw + (fabsf(lastpan) * (1.0f - params.centerPanLaw));
                    plastgain *= lastrevgain;

                    tobuffer.addFromWithRamp(channel, 0, frombuffer.getReadPointer(i), numSamples, plastgain, pgain);
                } else {
                    tobuffer.addFrom (channel, 0, frombuffer, i, 0, numSamples, pgain);
                }
            }
        }
    }
    else if (fromNumChans > 0 && destNumChans == 1){
        // sum all into destChan
        int channel = destStartChan;
        for (int srcchan = fromStartChan; srcchan < fromStartChan + fromNumChans && srcchan < fromMaxChans && channel < destMaxChans; ++srcchan) {

            tobuffer.addFromWithRamp(channel, 0, frombuffer.getReadPointer(srcchan), numSamples, lastrevgain, targrevgain);

        }
    }
    else if (fromNumChans > 0){
        // straight thru to dests - no panning
        int srcchan = fromStartChan;
        for (int channel = destStartChan; srcchan < fromStartChan + fromNumChans && srcchan < fromMaxChans && channel < destMaxChans; ++channel, ++srcchan) {
//        for (int channel = destStartChan + chanStartIndex; channel < destStartChan + destNumChans && srcchan < chanStartIndex + numChannels && srcchan < fromNumChan && channel < toNumChan; ++channel, ++srcchan) {
            //int srcchan = channel < mainBusInputChannels ? channel : mainBusInputChannels - 1;
            //int srcchan = chanStartIndex  channel < mainBusInputChannels ? channel : mainBusInputChannels - 1;

            tobuffer.addFromWithRamp(channel, 0, frombuffer.getReadPointer(srcchan), numSamples, lastrevgain, targrevgain);
        }
    }

    procstate.laststereopan[0] = params.panStereo[0];
    procstate.laststereopan[1] = params.panStereo[1];
    for (int pani=0; pani < params.numChannels; ++pani) {
        procstate.lastpan[pani] = params.pan[pani];
    }

    procstate.lastlevel = targrevgain;
}



String ChannelGroupParams::getValueTreeKey() const
{
    return channelGroupStateKey;
}

ValueTree ChannelGroupParams::getValueTree() const
{
    ValueTree channelGroupTree(channelGroupStateKey);

    channelGroupTree.setProperty(gainKey, gain, nullptr);
    channelGroupTree.setProperty(channelStartIndexKey, chanStartIndex, nullptr);
    channelGroupTree.setProperty(numChannelsKey, numChannels, nullptr);

    channelGroupTree.setProperty(peerPan1Key, panStereo[0], nullptr);
    channelGroupTree.setProperty(peerPan2Key, panStereo[1], nullptr);
    channelGroupTree.setProperty(monReverbSendKey, monReverbSend, nullptr);
    channelGroupTree.setProperty(inReverbSendKey, inReverbSend, nullptr);
    channelGroupTree.setProperty(monitorLevelKey, monitor, nullptr);

    channelGroupTree.setProperty(monDestStartKey, monDestStartIndex, nullptr);
    channelGroupTree.setProperty(monDestChannelsKey, monDestChannels, nullptr);
    channelGroupTree.setProperty(panDestStartKey, panDestStartIndex, nullptr);
    channelGroupTree.setProperty(panDestChannelsKey, panDestChannels, nullptr);


    channelGroupTree.setProperty(sendMainMixKey, sendMainMix, nullptr);
    channelGroupTree.setProperty(invertPolarityKey, invertPolarity, nullptr);

    channelGroupTree.setProperty(nameKey, name, nullptr);

    ValueTree panListTree(stereoPanListKey);
    for (int i=0; i < numChannels && i < MAX_CHANNELS; ++i) {
        ValueTree panner(panStateKey);
        panner.setProperty(panAttrKey, pan[i], nullptr);
        panListTree.appendChild(panner, nullptr);
    }

    channelGroupTree.appendChild(panListTree, nullptr);


    channelGroupTree.appendChild(compressorParams.getValueTree(compressorStateKey), nullptr);
    channelGroupTree.appendChild(expanderParams.getValueTree(expanderStateKey), nullptr);
    channelGroupTree.appendChild(limiterParams.getValueTree(limiterStateKey), nullptr);
    channelGroupTree.appendChild(eqParams.getValueTree(), nullptr);
    channelGroupTree.appendChild(monitorDelayParams.getValueTree(monDelayStateKey), nullptr);

    return channelGroupTree;
}

void ChannelGroupParams::setFromValueTree(const ValueTree & channelGroupTree)
{
    gain = channelGroupTree.getProperty(gainKey, gain);
    chanStartIndex = channelGroupTree.getProperty(channelStartIndexKey, chanStartIndex);
    numChannels = channelGroupTree.getProperty(numChannelsKey, numChannels);
    //pan = channelGroupTree.getProperty(peerMonoPanKey, pan);
    panStereo[0] = channelGroupTree.getProperty(peerPan1Key, panStereo[0]);
    panStereo[1] = channelGroupTree.getProperty(peerPan2Key, panStereo[1]);
    monReverbSend = channelGroupTree.getProperty(monReverbSendKey, monReverbSend);
    inReverbSend = channelGroupTree.getProperty(inReverbSendKey, inReverbSend);
    monitor = channelGroupTree.getProperty(monitorLevelKey, monitor);

    panDestStartIndex = channelGroupTree.getProperty(panDestStartKey, panDestStartIndex);
    panDestChannels = channelGroupTree.getProperty(panDestChannelsKey, panDestChannels);

    monDestStartIndex = channelGroupTree.getProperty(monDestStartKey, monDestStartIndex);
    monDestChannels = channelGroupTree.getProperty(monDestChannelsKey, monDestChannels);

    sendMainMix = channelGroupTree.getProperty(sendMainMixKey, sendMainMix);
    invertPolarity = channelGroupTree.getProperty(invertPolarityKey, invertPolarity);

    name = channelGroupTree.getProperty(nameKey, name);

    ValueTree panListTree = channelGroupTree.getChildWithName(stereoPanListKey);
    if (panListTree.isValid()) {

        int i=0;
        for (auto panner : panListTree) {
            if (panner.isValid() && i < MAX_CHANNELS) {
                pan[i] = panner.getProperty(panAttrKey, pan[i]);
            }

            ++i;
        }
    }

    ValueTree compressorTree = channelGroupTree.getChildWithName(compressorStateKey);
    if (compressorTree.isValid()) {
        compressorParams.setFromValueTree(compressorTree);
    }
    ValueTree expanderTree = channelGroupTree.getChildWithName(expanderStateKey);
    if (expanderTree.isValid()) {
        expanderParams.setFromValueTree(expanderTree);
    }
    ValueTree limiterTree = channelGroupTree.getChildWithName(limiterStateKey);
    if (limiterTree.isValid()) {
        limiterParams.setFromValueTree(limiterTree);
    }
    ValueTree eqTree = channelGroupTree.getChildWithName(eqStateKey);
    if (eqTree.isValid()) {
        eqParams.setFromValueTree(eqTree);
    }
    ValueTree mondelayTree = channelGroupTree.getChildWithName(monDelayStateKey);
    if (mondelayTree.isValid()) {
        monitorDelayParams.setFromValueTree(mondelayTree);
    }
}

ValueTree ChannelGroupParams::getChannelLayoutValueTree()
{
    ValueTree channelGroupTree(layoutGroupsKey);

    channelGroupTree.setProperty(channelStartIndexKey, chanStartIndex, nullptr);
    channelGroupTree.setProperty(numChannelsKey, numChannels, nullptr);
    channelGroupTree.setProperty(nameKey, name, nullptr);
    channelGroupTree.setProperty(peerPan1Key, panStereo[0], nullptr);
    channelGroupTree.setProperty(peerPan2Key, panStereo[1], nullptr);

    ValueTree panListTree(stereoPanListKey);
    for (int i=0; i < numChannels && i < MAX_CHANNELS; ++i) {
        ValueTree panner(panStateKey);
        panner.setProperty(panAttrKey, pan[i], nullptr);
        panListTree.appendChild(panner, nullptr);
    }
    channelGroupTree.appendChild(panListTree, nullptr);

    return channelGroupTree;
}

void ChannelGroupParams::setFromChannelLayoutValueTree(const ValueTree & layoutval)
{
    chanStartIndex = layoutval.getProperty(channelStartIndexKey, chanStartIndex);
    numChannels = layoutval.getProperty(numChannelsKey, numChannels);
    name = layoutval.getProperty(nameKey, name);
    panStereo[0] = layoutval.getProperty(peerPan1Key, panStereo[0]);
    panStereo[1] = layoutval.getProperty(peerPan2Key, panStereo[1]);

    ValueTree panListTree = layoutval.getChildWithName(stereoPanListKey);
    if (panListTree.isValid()) {
        int i=0;
        for (auto panner : panListTree) {
            if (panner.isValid() && i < MAX_CHANNELS) {
                pan[i] = panner.getProperty(panAttrKey, pan[i]);
            }
            ++i;
        }
    }
}



void ChannelGroup::commitAllParams()
{
    commitCompressorParams();
    commitLimiterParams();
    commitEqParams();
    commitExpanderParams();
    commitMonitorDelayParams();
}


void ChannelGroup::commitCompressorParams()
{
    if (!compressorControl) return;
    compressorControl->setParamValue("/compressor/Bypass", params.compressorParams.enabled ? 0.0f : 1.0f);
    compressorControl->setParamValue("/compressor/knee", 2.0f);
    compressorControl->setParamValue("/compressor/threshold", params.compressorParams.thresholdDb);
    compressorControl->setParamValue("/compressor/ratio", params.compressorParams.ratio);
    compressorControl->setParamValue("/compressor/attack", params.compressorParams.attackMs * 1e-3);
    compressorControl->setParamValue("/compressor/release", params.compressorParams.releaseMs * 1e-3);
    compressorControl->setParamValue("/compressor/makeup_gain", params.compressorParams.makeupGainDb);

    float * tmp = compressorControl->getParamZone("/compressor/outgain");
    if (tmp != compressorOutputLevel) {
        compressorOutputLevel = tmp; // pointer
    }
}


void ChannelGroup::commitExpanderParams()
{
    if (!expanderControl) return;
    //mInputCompressorControl.setParamValue("/compressor/Bypass", mInputCompressorParams.enabled ? 0.0f : 1.0f);
    expanderControl->setParamValue("/expander/knee", 3.0f);
    expanderControl->setParamValue("/expander/threshold", params.expanderParams.thresholdDb);
    expanderControl->setParamValue("/expander/ratio", params.expanderParams.ratio);
    expanderControl->setParamValue("/expander/attack", params.expanderParams.attackMs * 1e-3);
    expanderControl->setParamValue("/expander/release", params.expanderParams.releaseMs * 1e-3);

    float * tmp = expanderControl->getParamZone("/expander/outgain");

    if (tmp != expanderOutputGain) {
        expanderOutputGain = tmp; // pointer
    }
}

void ChannelGroup::commitLimiterParams()
{
    if (!limiterControl) return;

    limiterControl->setParamValue("/compressor/Bypass", params.limiterParams.enabled ? 0.0f : 1.0f);
    limiterControl->setParamValue("/compressor/threshold", params.limiterParams.thresholdDb);
    limiterControl->setParamValue("/compressor/ratio", params.limiterParams.ratio);
    limiterControl->setParamValue("/compressor/attack", params.limiterParams.attackMs * 1e-3);
    limiterControl->setParamValue("/compressor/release", params.limiterParams.releaseMs * 1e-3);
}


void ChannelGroup::commitEqParams()
{
    if (!eqControl[0]) return;

    for (int i=0; i < 2; ++i) {
        eqControl[i]->setParamValue("/parametric_eq/low_shelf/gain", params.eqParams.lowShelfGain);
        eqControl[i]->setParamValue("/parametric_eq/low_shelf/transition_freq", params.eqParams.lowShelfFreq);
        eqControl[i]->setParamValue("/parametric_eq/para1/peak_gain", params.eqParams.para1Gain);
        eqControl[i]->setParamValue("/parametric_eq/para1/peak_frequency", params.eqParams.para1Freq);
        eqControl[i]->setParamValue("/parametric_eq/para1/peak_q", params.eqParams.para1Q);
        eqControl[i]->setParamValue("/parametric_eq/para2/peak_gain", params.eqParams.para2Gain);
        eqControl[i]->setParamValue("/parametric_eq/para2/peak_frequency", params.eqParams.para2Freq);
        eqControl[i]->setParamValue("/parametric_eq/para2/peak_q", params.eqParams.para2Q);
        eqControl[i]->setParamValue("/parametric_eq/high_shelf/gain", params.eqParams.highShelfGain);
        eqControl[i]->setParamValue("/parametric_eq/high_shelf/transition_freq", params.eqParams.highShelfFreq);
    }
}

void ChannelGroup::commitMonitorDelayParams()
{
    setMonitoringDelayTimeMs(params.monitorDelayParams.delayTimeMs);
    setMonitoringDelayEnabled(params.monitorDelayParams.enabled, params.numChannels);
}


