// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2021 Jesse Chappell


#pragma once

#include "JuceHeader.h"

namespace SonoAudio {

struct CompressorParams
{
    ValueTree getValueTree(const String & stateKey) const;
    void setFromValueTree(const ValueTree & val);

    bool enabled = false;
    float thresholdDb = -16.0f;
    float ratio = 2.0f;
    float attackMs = 10.0f;
    float releaseMs = 80.0f;
    float makeupGainDb = 0.0f;
    bool  automakeupGain = true;
};



struct ParametricEqParams
{
    ValueTree getValueTree() const;
    void setFromValueTree(const ValueTree & val);

    bool enabled = false;
    float lowShelfGain = 0.0f; // db
    float lowShelfFreq = 60.0f; // Hz
    float para1Gain = 0.0f; // db
    float para1Freq = 90.0f; // Hz
    float para1Q = 1.5f;
    float para2Gain = 0.0f; // db
    float para2Freq = 360.0; // Hz
    float para2Q = 4.0f;
    float highShelfGain = 0.0f; // db
    float highShelfFreq = 10000.0f; // Hz
};

struct DelayParams
{
    ValueTree getValueTree(const String & stateKey) const;
    void setFromValueTree(const ValueTree & val);

    bool enabled = false;
    float delayTimeMs = 0.0f;
};


}
