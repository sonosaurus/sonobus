// SPDX-License-Identifier: GPLv3-or-later
// Copyright (C) 2021 Jesse Chappell


#include "ChannelGroup.h"

static String nameKey("name");

static String gainKey("gain");
static String channelStartIndexKey("chanstart");
static String numChannelsKey("numchan");
static String reverbSendKey("revsend");
static String monitorLevelKey("monitorlev");
static String peerMonoPanKey("pan");
static String peerPan1Key("span1");
static String peerPan2Key("span2");
static String panDestStartKey("pandeststart");
static String panDestChannelsKey("pandestchans");
static String monDestStartKey("mondeststart");
static String monDestChannelsKey("mondestchans");

static String stereoPanListKey("StereoPanners");

static String panStateKey("Panner");
static String panAttrKey("pan");


static String compressorStateKey("CompressorState");
static String expanderStateKey("ExpanderState");
static String limiterStateKey("LimiterState");
static String eqStateKey("ParametricEqState");

static String inputChannelGroupsStateKey("InputChannelGroups");
static String channelGroupStateKey("ChannelGroup");
static String numChanGroupsKey("numChanGroups");


using namespace SonoAudio;

ChannelGroup::ChannelGroup()
{
}

void ChannelGroup::setToDefaults(bool isplugin)
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
}

void ChannelGroup::init(double sampleRate)
{
    compressor.init(sampleRate);
    compressor.buildUserInterface(&compressorControl);

    //DBG("Compressor Params:");
    //for(int i=0; i < mInputCompressorControl.getParamsCount(); i++){
    //    DBG(mInputCompressorControl.getParamAddress(i));
    //}

    expander.init(sampleRate);
    expander.buildUserInterface(&expanderControl);

    //DBG("Expander Params:");
    //for(int i=0; i < mInputExpanderControl.getParamsCount(); i++){
    //    DBG(mInputExpanderControl.getParamAddress(i));
    //}

    for (int j=0; j < 2; ++j) {
        eq[j].init(sampleRate);
        eq[j].buildUserInterface(&eqControl[j]);
    }

    //DBG("EQ Params:");
    //for(int i=0; i < mInputEqControl[0].getParamsCount(); i++){
    //    DBG(mInputEqControl[0].getParamAddress(i));
    //}

    limiter.init(sampleRate);
    limiter.buildUserInterface(&limiterControl);

    //DBG("Limiter Params:");
    //for(int i=0; i < mInputLimiterControl.getParamsCount(); i++){
    //    DBG(mInputLimiterControl.getParamAddress(i));
    //}

    commitCompressorParams();
    commitExpanderParams();
    commitEqParams();
    commitLimiterParams();
}

void ChannelGroup::processBlock (AudioBuffer<float>& frombuffer, AudioBuffer<float>& tobuffer, int destStartChan, int destNumChans, AudioBuffer<float>& silentBuffer, int numSamples, float gainfactor)
{
    // called from audio thread context



    int chstart = chanStartIndex;
    int numchan = numChannels;
    const int frombufNumChan = frombuffer.getNumChannels();
    const int tobufNumChan = tobuffer.getNumChannels();
    // apply input gain


    float dogain = (muted ? 0.0f : gain) * gainfactor;
    //dogain = 0.0f;

    if (&frombuffer == &tobuffer) {
        // inplace, just apply gain, ignore destchans
        for (int i = chstart; i < chstart+numchan && i < frombufNumChan ; ++i) {
            tobuffer.applyGainRamp(i, 0, numSamples, _lastgain, dogain);
        }
    }
    else {
        for (int i = chstart, desti=destStartChan; i < chstart+numchan && i < frombufNumChan && desti < destStartChan+destNumChans && desti < tobufNumChan; ++i, ++desti) {
            tobuffer.addFromWithRamp(desti, 0, frombuffer.getReadPointer(i), numSamples, _lastgain, dogain);
        }
    }


    _lastgain = dogain;

    // these all operate ONLY when the channel group has 1 or 2 channels
    if (numChannels <= 0 || numChannels > 2) {
        return;
    }

    // apply input expander
    if (expanderParamsChanged) {
        commitExpanderParams();
        expanderParamsChanged = false;
    }
    if (_lastExpanderEnabled || expanderParams.enabled) {
        if (tobufNumChan - destStartChan > 1 && numchan == 2 && destNumChans >= 2) {
            float *bufs[2] = { tobuffer.getWritePointer(destStartChan), tobuffer.getWritePointer(destStartChan+1)};
            expander.compute(numSamples, bufs, bufs);
        } else if (destStartChan < tobufNumChan) {
            float *bufs[2] = { tobuffer.getWritePointer(destStartChan), silentBuffer.getWritePointer(0) }; // just a silent dummy buffer
            expander.compute(numSamples, bufs, bufs);
        }
    }
    _lastExpanderEnabled = expanderParams.enabled;


    // apply input compressor
    if (compressorParamsChanged) {
        commitCompressorParams();
        compressorParamsChanged = false;
    }
    if (_lastCompressorEnabled || compressorParams.enabled) {
        if (tobufNumChan - destStartChan > 1 && numchan == 2 && destNumChans >= 2) {
            float *bufs[2] = { tobuffer.getWritePointer(destStartChan), tobuffer.getWritePointer(destStartChan+1)};
            compressor.compute(numSamples, bufs, bufs);
        } else if (destStartChan < tobufNumChan) {
            float *bufs[2] = { tobuffer.getWritePointer(destStartChan), silentBuffer.getWritePointer(0) }; // just a silent dummy buffer
            compressor.compute(numSamples, bufs, bufs);
        }
    }
    _lastCompressorEnabled = compressorParams.enabled;


    // apply input EQ
    if (eqParamsChanged) {
        commitEqParams();
        eqParamsChanged = false;
    }
    if (_lastEqEnabled || eqParams.enabled) {
        if (tobufNumChan - destStartChan > 1 && numchan == 2 && destNumChans >= 2) {
            // only 2 channels support for now... TODO
            float *bufs[2] = { tobuffer.getWritePointer(destStartChan), tobuffer.getWritePointer(destStartChan+1)};
            eq[0].compute(numSamples, &bufs[0], &bufs[0]);
            eq[1].compute(numSamples, &bufs[1], &bufs[1]);
        } else if (destStartChan < tobufNumChan) {
            float *inbuf = frombuffer.getWritePointer(chstart);
            float *outbuf = tobuffer.getWritePointer(destStartChan);
            eq[0].compute(numSamples, &inbuf, &outbuf);
        }
    }
    _lastEqEnabled = eqParams.enabled;


    // apply input limiter
    if (limiterParamsChanged) {
        commitLimiterParams();
        limiterParamsChanged = false;
    }
    if (_lastLimiterEnabled || limiterParams.enabled) {
        if (tobufNumChan - destStartChan > 1 && numchan == 2 && destNumChans >= 2) {
            float *bufs[2] = { tobuffer.getWritePointer(destStartChan), tobuffer.getWritePointer(destStartChan+1)};
            limiter.compute(numSamples, bufs, bufs);
        } else if (destStartChan < tobufNumChan) {
            float *bufs[2] = { tobuffer.getWritePointer(destStartChan), silentBuffer.getWritePointer(0) }; // just a silent dummy buffer
            limiter.compute(numSamples, bufs, bufs);
        }
    }
    _lastLimiterEnabled = limiterParams.enabled;

}

void ChannelGroup::processPan (AudioBuffer<float>& frombuffer, int fromStartChan, AudioBuffer<float>& tobuffer, int destStartChan, int destNumChans, int numSamples, float gainfactor)
{
    int fromNumChan = frombuffer.getNumChannels();
    int toNumChan = tobuffer.getNumChannels();

    if (fromNumChan == 0) return;

    if (destNumChans == 2) {
        //tobuffer.clear(0, numSamples);

        for (int channel = destStartChan; channel < destStartChan + destNumChans && channel < toNumChan; ++channel) {
            int pani = 0;
            for (int i=fromStartChan; i < fromStartChan + numChannels && i < fromNumChan; ++i, ++pani) {
                const float upan = (numChannels != 2 ? pan[pani] : i==fromStartChan ? panStereo[0] : panStereo[1]);
                const float lastpan = (numChannels != 2 ? _lastpan[pani] : i==fromStartChan ? _laststereopan[0] : _laststereopan[1]);

                // -1 is left, 1 is right
                float pgain = channel == destStartChan ? (upan >= 0.0f ? (1.0f - upan) : 1.0f) : (upan >= 0.0f ? 1.0f : (1.0f+upan)) ;

                // apply pan law
                pgain *= centerPanLaw + (fabsf(upan) * (1.0f - centerPanLaw));
                pgain *= gainfactor;

                if (fabsf(upan - lastpan) > 0.00001f) {
                    float plastgain = channel == destStartChan ? (lastpan >= 0.0f ? (1.0f - lastpan) : 1.0f) : (lastpan >= 0.0f ? 1.0f : (1.0f+lastpan));
                    plastgain *= centerPanLaw + (fabsf(lastpan) * (1.0f - centerPanLaw));
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
        for (int srcchan = fromStartChan; srcchan < fromStartChan + numChannels && srcchan < fromNumChan && channel < toNumChan; ++srcchan) {

            tobuffer.addFrom(channel, 0, frombuffer, srcchan, 0, numSamples, gainfactor);
        }
    }
    else {
        // straight thru to dests - no panning
        int srcchan = fromStartChan;
        for (int channel = destStartChan; srcchan < fromStartChan + numChannels && srcchan < fromNumChan && channel < toNumChan; ++channel, ++srcchan) {
            //int srcchan = channel < mainBusInputChannels ? channel : mainBusInputChannels - 1;
            //int srcchan = chanStartIndex  channel < mainBusInputChannels ? channel : mainBusInputChannels - 1;

            tobuffer.addFrom(channel, 0, frombuffer, srcchan, 0, numSamples, gainfactor);
        }
    }
    

    _laststereopan[0] = panStereo[0];
    _laststereopan[1] = panStereo[1];
    for (int pani=0; pani < numChannels; ++pani) {
        _lastpan[pani] = pan[pani];
    }
}

void ChannelGroup::processMonitor (AudioBuffer<float>& frombuffer, int fromStartChan, AudioBuffer<float>& tobuffer, int destStartChan, int destNumChans, int numSamples, float gainfactor)
{

    // apply monitor level

    float targmon = monitor * gainfactor;

    int fromNumChan = frombuffer.getNumChannels();
    int toNumChan = tobuffer.getNumChannels();

    if (frombuffer.getNumChannels() > 0 && destNumChans == 2) {
        //tobuffer.clear(0, numSamples);

        for (int channel = destStartChan; channel < destStartChan + destNumChans; ++channel) {
            int pani = 0;
            for (int i=fromStartChan; i < fromStartChan + numChannels && i < fromNumChan; ++i, ++pani) {
                const float upan = (numChannels != 2 ? pan[pani] : i==fromStartChan ? panStereo[0] : panStereo[1]);
                const float lastpan = (numChannels != 2 ? _lastmonpan[pani] : i==fromStartChan ? _lastmonstereopan[0] : _lastmonstereopan[1]);

                // -1 is left, 1 is right
                float pgain = channel == destStartChan ? (upan >= 0.0f ? (1.0f - upan) : 1.0f) : (upan >= 0.0f ? 1.0f : (1.0f+upan)) ;

                // apply pan law
                pgain *= centerPanLaw + (fabsf(upan) * (1.0f - centerPanLaw));
                pgain *= targmon;

                if (fabsf(upan - lastpan) > 0.00001f) {
                    float plastgain = channel == destStartChan ? (lastpan >= 0.0f ? (1.0f - lastpan) : 1.0f) : (lastpan >= 0.0f ? 1.0f : (1.0f+lastpan));
                    plastgain *= centerPanLaw + (fabsf(lastpan) * (1.0f - centerPanLaw));
                    plastgain *= targmon;

                    tobuffer.addFromWithRamp(channel, 0, frombuffer.getReadPointer(i), numSamples, plastgain, pgain);
                } else {
                    tobuffer.addFrom (channel, 0, frombuffer, i, 0, numSamples, pgain);
                }
            }
        }
    }
    else if (frombuffer.getNumChannels() > 0 && destNumChans == 1){
        // sum all into destChan
        int channel = destStartChan;
        for (int srcchan = fromStartChan; srcchan < fromStartChan + numChannels && srcchan < fromNumChan && channel < toNumChan; ++srcchan) {

            tobuffer.addFrom(channel, 0, frombuffer, srcchan, 0, numSamples, targmon);
        }
    }
    else if (frombuffer.getNumChannels() > 0){
        // straight thru to dests - no panning
        int srcchan = fromStartChan;
        for (int channel = destStartChan; srcchan < fromStartChan + numChannels && srcchan < fromNumChan && channel < toNumChan; ++channel, ++srcchan) {
//        for (int channel = destStartChan + chanStartIndex; channel < destStartChan + destNumChans && srcchan < chanStartIndex + numChannels && srcchan < fromNumChan && channel < toNumChan; ++channel, ++srcchan) {
            //int srcchan = channel < mainBusInputChannels ? channel : mainBusInputChannels - 1;
            //int srcchan = chanStartIndex  channel < mainBusInputChannels ? channel : mainBusInputChannels - 1;

            tobuffer.addFrom(channel, 0, frombuffer, srcchan, 0, numSamples, targmon);
        }
    }


    _lastmonstereopan[0] = panStereo[0];
    _lastmonstereopan[1] = panStereo[1];
    for (int pani=0; pani < numChannels; ++pani) {
        _lastmonpan[pani] = pan[pani];
    }

    _lastmonitor = targmon;

}

String ChannelGroup::getValueTreeKey() const
{
    return channelGroupStateKey;
}

ValueTree ChannelGroup::getValueTree() const
{
    ValueTree channelGroupTree(channelGroupStateKey);

    channelGroupTree.setProperty(gainKey, gain, nullptr);
    channelGroupTree.setProperty(channelStartIndexKey, chanStartIndex, nullptr);
    channelGroupTree.setProperty(numChannelsKey, numChannels, nullptr);

    channelGroupTree.setProperty(peerPan1Key, panStereo[0], nullptr);
    channelGroupTree.setProperty(peerPan2Key, panStereo[1], nullptr);
    channelGroupTree.setProperty(reverbSendKey, reverbSend, nullptr);
    channelGroupTree.setProperty(monitorLevelKey, monitor, nullptr);

    channelGroupTree.setProperty(monDestStartKey, monDestStartIndex, nullptr);
    channelGroupTree.setProperty(monDestChannelsKey, monDestChannels, nullptr);
    channelGroupTree.setProperty(panDestStartKey, panDestStartIndex, nullptr);
    channelGroupTree.setProperty(panDestChannelsKey, panDestChannels, nullptr);

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

    return channelGroupTree;
}

void ChannelGroup::setFromValueTree(const ValueTree & channelGroupTree)
{
    gain = channelGroupTree.getProperty(gainKey, gain);
    chanStartIndex = channelGroupTree.getProperty(channelStartIndexKey, chanStartIndex);
    numChannels = channelGroupTree.getProperty(numChannelsKey, numChannels);
    //pan = channelGroupTree.getProperty(peerMonoPanKey, pan);
    panStereo[0] = channelGroupTree.getProperty(peerPan1Key, panStereo[0]);
    panStereo[1] = channelGroupTree.getProperty(peerPan2Key, panStereo[1]);
    reverbSend = channelGroupTree.getProperty(reverbSendKey, reverbSend);
    monitor = channelGroupTree.getProperty(monitorLevelKey, monitor);

    panDestStartIndex = channelGroupTree.getProperty(panDestStartKey, panDestStartIndex);
    panDestChannels = channelGroupTree.getProperty(panDestChannelsKey, panDestChannels);

    monDestStartIndex = channelGroupTree.getProperty(monDestStartKey, monDestStartIndex);
    monDestChannels = channelGroupTree.getProperty(monDestChannelsKey, monDestChannels);


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
        commitCompressorParams();
    }
    ValueTree expanderTree = channelGroupTree.getChildWithName(expanderStateKey);
    if (expanderTree.isValid()) {
        expanderParams.setFromValueTree(expanderTree);
        commitExpanderParams();
    }
    ValueTree limiterTree = channelGroupTree.getChildWithName(limiterStateKey);
    if (limiterTree.isValid()) {
        limiterParams.setFromValueTree(limiterTree);
        commitLimiterParams();
    }

    ValueTree eqTree = channelGroupTree.getChildWithName(eqStateKey);
    if (eqTree.isValid()) {
        eqParams.setFromValueTree(eqTree);
        commitEqParams();
    }
}

void ChannelGroup::commitCompressorParams()
{
    compressorControl.setParamValue("/compressor/Bypass", compressorParams.enabled ? 0.0f : 1.0f);
    compressorControl.setParamValue("/compressor/knee", 2.0f);
    compressorControl.setParamValue("/compressor/threshold", compressorParams.thresholdDb);
    compressorControl.setParamValue("/compressor/ratio", compressorParams.ratio);
    compressorControl.setParamValue("/compressor/attack", compressorParams.attackMs * 1e-3);
    compressorControl.setParamValue("/compressor/release", compressorParams.releaseMs * 1e-3);
    compressorControl.setParamValue("/compressor/makeup_gain", compressorParams.makeupGainDb);

    float * tmp = compressorControl.getParamZone("/compressor/outgain");
    if (tmp != compressorOutputLevel) {
        compressorOutputLevel = tmp; // pointer
    }
}


void ChannelGroup::commitExpanderParams()
{
    //mInputCompressorControl.setParamValue("/compressor/Bypass", mInputCompressorParams.enabled ? 0.0f : 1.0f);
    expanderControl.setParamValue("/expander/knee", 3.0f);
    expanderControl.setParamValue("/expander/threshold", expanderParams.thresholdDb);
    expanderControl.setParamValue("/expander/ratio", expanderParams.ratio);
    expanderControl.setParamValue("/expander/attack", expanderParams.attackMs * 1e-3);
    expanderControl.setParamValue("/expander/release", expanderParams.releaseMs * 1e-3);

    float * tmp = expanderControl.getParamZone("/expander/outgain");

    if (tmp != expanderOutputGain) {
        expanderOutputGain = tmp; // pointer
    }
}

void ChannelGroup::commitLimiterParams()
{
    //mInputLimiterControl.setParamValue("/limiter/Bypass", mInputLimiterParams.enabled ? 0.0f : 1.0f);
    //mInputLimiterControl.setParamValue("/limiter/threshold", mInputLimiterParams.thresholdDb);
    //mInputLimiterControl.setParamValue("/limiter/ratio", mInputLimiterParams.ratio);
    //mInputLimiterControl.setParamValue("/limiter/attack", mInputLimiterParams.attackMs * 1e-3);
    //mInputLimiterControl.setParamValue("/limiter/release", mInputLimiterParams.releaseMs * 1e-3);

    limiterControl.setParamValue("/compressor/Bypass", limiterParams.enabled ? 0.0f : 1.0f);
    limiterControl.setParamValue("/compressor/threshold", limiterParams.thresholdDb);
    limiterControl.setParamValue("/compressor/ratio", limiterParams.ratio);
    limiterControl.setParamValue("/compressor/attack", limiterParams.attackMs * 1e-3);
    limiterControl.setParamValue("/compressor/release", limiterParams.releaseMs * 1e-3);
}


void ChannelGroup::commitEqParams()
{
    for (int i=0; i < 2; ++i) {
        eqControl[i].setParamValue("/parametric_eq/low_shelf/gain", eqParams.lowShelfGain);
        eqControl[i].setParamValue("/parametric_eq/low_shelf/transition_freq", eqParams.lowShelfFreq);
        eqControl[i].setParamValue("/parametric_eq/para1/peak_gain", eqParams.para1Gain);
        eqControl[i].setParamValue("/parametric_eq/para1/peak_frequency", eqParams.para1Freq);
        eqControl[i].setParamValue("/parametric_eq/para1/peak_q", eqParams.para1Q);
        eqControl[i].setParamValue("/parametric_eq/para2/peak_gain", eqParams.para2Gain);
        eqControl[i].setParamValue("/parametric_eq/para2/peak_frequency", eqParams.para2Freq);
        eqControl[i].setParamValue("/parametric_eq/para2/peak_q", eqParams.para2Q);
        eqControl[i].setParamValue("/parametric_eq/high_shelf/gain", eqParams.highShelfGain);
        eqControl[i].setParamValue("/parametric_eq/high_shelf/transition_freq", eqParams.highShelfFreq);
    }
}
