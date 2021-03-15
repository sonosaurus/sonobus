// SPDX-License-Identifier: GPLv3-or-later
// Copyright (C) 2021 Jesse Chappell


#include "EffectParams.h"

using namespace SonoAudio;

static String compressorEnabledKey("enabled");
static String compressorThresholdKey("threshold");
static String compressorRatioKey("ratio");
static String compressorAttackKey("attack");
static String compressorReleaseKey("release");
static String compressorMakeupGainKey("makeupgain");
static String compressorAutoMakeupGainKey("automakeup");

static String eqStateKey("ParametricEqState");
static String eqEnabledKey("enabled");
static String eqLowShelfGainKey("lsgain");
static String eqLowShelfFreqKey("lsfreq");
static String eqHighShelfGainKey("hsgain");
static String eqHighShelfFreqKey("hsfreq");
static String eqPara1GainKey("p1gain");
static String eqPara1FreqKey("p1freq");
static String eqPara1QKey("p1q");
static String eqPara2GainKey("p2gain");
static String eqPara2FreqKey("p2freq");
static String eqPara2QKey("p1q");

static String delayEnabledKey("enabled");
static String delayTimeMsKey("delaytimems");



ValueTree CompressorParams::getValueTree(const String & stateKey) const
{
    ValueTree item(stateKey);

    item.setProperty(compressorEnabledKey, enabled, nullptr);
    item.setProperty(compressorThresholdKey, thresholdDb, nullptr);
    item.setProperty(compressorRatioKey, ratio, nullptr);
    item.setProperty(compressorAttackKey, attackMs, nullptr);
    item.setProperty(compressorReleaseKey, releaseMs, nullptr);
    item.setProperty(compressorMakeupGainKey, makeupGainDb, nullptr);
    item.setProperty(compressorAutoMakeupGainKey, automakeupGain, nullptr);

    return item;
}

void CompressorParams::setFromValueTree(const ValueTree & item)
{
    enabled = item.getProperty(compressorEnabledKey, enabled);
    thresholdDb = item.getProperty(compressorThresholdKey, thresholdDb);
    ratio = item.getProperty(compressorRatioKey, ratio);
    attackMs = item.getProperty(compressorAttackKey, attackMs);
    releaseMs = item.getProperty(compressorReleaseKey, releaseMs);
    makeupGainDb = item.getProperty(compressorMakeupGainKey, makeupGainDb);
    automakeupGain = item.getProperty(compressorAutoMakeupGainKey, automakeupGain);
}

ValueTree ParametricEqParams::getValueTree() const
{
    ValueTree item(eqStateKey);

    item.setProperty(eqEnabledKey, enabled, nullptr);
    item.setProperty(eqLowShelfFreqKey, lowShelfFreq, nullptr);
    item.setProperty(eqLowShelfGainKey, lowShelfGain, nullptr);
    item.setProperty(eqHighShelfFreqKey, highShelfFreq, nullptr);
    item.setProperty(eqHighShelfGainKey, highShelfGain, nullptr);
    item.setProperty(eqPara1GainKey, para1Gain, nullptr);
    item.setProperty(eqPara1FreqKey, para1Freq, nullptr);
    item.setProperty(eqPara1QKey, para1Q, nullptr);
    item.setProperty(eqPara2GainKey, para2Gain, nullptr);
    item.setProperty(eqPara2FreqKey, para2Freq, nullptr);
    item.setProperty(eqPara2QKey, para2Q, nullptr);

    return item;
}

void ParametricEqParams::setFromValueTree(const ValueTree & item)
{
    enabled = item.getProperty(eqEnabledKey, enabled);
    lowShelfFreq = item.getProperty(eqLowShelfFreqKey, lowShelfFreq);
    lowShelfGain = item.getProperty(eqLowShelfGainKey, lowShelfGain);
    highShelfFreq = item.getProperty(eqHighShelfFreqKey, highShelfFreq);
    highShelfGain = item.getProperty(eqHighShelfGainKey, highShelfGain);
    para1Gain = item.getProperty(eqPara1GainKey, para1Gain);
    para1Freq = item.getProperty(eqPara1FreqKey, para1Freq);
    para1Q = item.getProperty(eqPara1QKey, para1Q);
    para2Gain = item.getProperty(eqPara2GainKey, para2Gain);
    para2Freq = item.getProperty(eqPara2FreqKey, para2Freq);
    para2Q = item.getProperty(eqPara2QKey, para2Q);

}

ValueTree DelayParams::getValueTree(const String & stateKey) const
{
    ValueTree item(stateKey);

    item.setProperty(delayEnabledKey, enabled, nullptr);
    item.setProperty(delayTimeMsKey, delayTimeMs, nullptr);

    return item;
}

void DelayParams::setFromValueTree(const ValueTree & item)
{
    enabled = item.getProperty(delayEnabledKey, enabled);
    delayTimeMs = item.getProperty(delayTimeMsKey, delayTimeMs);
}
