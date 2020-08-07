/*
 *  Metronome.h
 *  ThumbJam
 *
 *  Created by Jesse Chappell on 3/27/10.
 *  Copyright 2010 Sonosaurus. All rights reserved.
 *
 */

#pragma once

#include "JuceHeader.h"


namespace SonoAudio
{
    
    class Metronome	{
    public:
        
        Metronome(double samplerate=44100.0);
        virtual ~Metronome();
        
        //void processData (int nframes, const signed short int *indata);
        
        void setSampleRate(double rate) { sampleRate = rate; }
        double getSampleRate() const { return sampleRate; }
        
        // the timestamp passed in should be relative to time zero for the current tempo
        // now beat-time instead!
        void processMix (int nframes, float * inOutDataL, float * inOutDataR, const double beatTime, bool relativeTime=false);
        
        void setTempo(double bpm);
        double getTempo() const { return mTempo; }
        
        void setBeatsPerBar(int num);
        int getBeatsPerBar() const { return mBeatsPerBar; }
        
        
        void resetRelativeStart();
        void setRemainRatios(double barRemain, double beatRemain);
        
        double getCurrBeatPos() const { return mCurrBeatPos; }
        
        void setGain(float val, bool force=false);
        float getGain() const { return mPendingGain; }
        
        // loads the sampleset containing the samples to use for the click.
        // they must have labels corresponding to the interval they will sound
        //  "beat" and "bar" (optional)
        //bool loadSampleSet(const SampleSetInfo & sinfo);
        
        //const SampleSetInfo & getCurrentSamplesetInfo() const { return m_sampleSet.getInfo(); }
        
        void loadBeatSoundFromBinaryData(const void* data, size_t sizeBytes);
        void loadBarSoundFromBinaryData(const void* data, size_t sizeBytes);

        
    protected:
        
        int  blockSize;
        double sampleRate;

        double mTempo;
        int  mBeatsPerBar;
        float mGain;
        volatile float mPendingGain;

        double mCurrentBarRemainRatio;
        double mCurrentBeatRemainRatio;
        
        double mCurrBeatPos;
        
        AudioSampleBuffer beatSoundBuffer;
        AudioSampleBuffer barSoundBuffer;

        AudioSampleBuffer tempBuffer;

        //SampleSet  m_sampleSet;
        //NonBlockingLock  mSampleLock;
        CriticalSection mSampleLock;
        
        struct SampleState
        {
            SampleState() : sampleData(0), samplePos(0), sampleRemain(0), sampleLength(0) {}
            ~SampleState() { }
            float * sampleData; // not owned by us
            long samplePos;
            long sampleRemain;
            long sampleLength;
        };
        
        SampleState mBeatState;
        SampleState mBarState;        

        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Metronome)

    };
    

}

