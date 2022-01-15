// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell

#pragma once

#include "JuceHeader.h"


namespace SonoAudio
{
    class Metronome	{
    public:
        Metronome(double samplerate=44100.0);
        virtual ~Metronome();
        
        // the timestamp passed in should be relative to time zero for the current tempo
        // now beat-time instead!
        void processMix(AudioSampleBuffer *sampleBuffer, const double beatTime, bool relativeTime = false);

        void turn(bool on);

        void setSampleRate(double rate) { sampleRate = rate; }
        double getSampleRate() const { return sampleRate; }

        void setTempo(double bpm);
        double getTempo() const { return mTempo.get(); }

        void setGain(float val, bool force=false);
        float getGain() const { return mPendingGain; }
        
        void loadBeatSoundFromBinaryData(const void* data, size_t sizeBytes);

    protected:
        struct SampleState
        {
            SampleState() : sampleData(0), samplePos(0), sampleRemain(0), sampleLength(0) {}
            ~SampleState() { }
            float * sampleData; // not owned by us
            long samplePos;
            long sampleRemain;
            long sampleLength;
        };

        int  blockSize;
        double sampleRate;
        Atomic<double>   mTempo;
        Atomic<float>   mGain;
        volatile float mPendingGain;
        double mCurrentBeatRemainRatio;
        double mCurrBeatPos;
        AudioSampleBuffer beatSoundBuffer;
        CriticalSection mSampleLock;
        SampleState mBeatState;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Metronome)
    };
}