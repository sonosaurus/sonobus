// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell

#pragma once

#include "JuceHeader.h"


namespace SonoAudio
{

    class Metronome	{

    public:

        Metronome(const void* beatSoundData, size_t beatSoundDataSizeBytes,
                   const void* barSoundData, size_t barSoundDataSizeBytes);
        virtual ~Metronome();
        
        //void processData (int nframes, const signed short int *indata);
        
        void setSampleRate(double rate) { sampleRate = rate; }
        double getSampleRate() const { return sampleRate; }
        
        // the timestamp passed in should be relative to time zero for the current tempo
        // now beat-time instead!
        void processMix (int windowSizeInSamples, float * inOutDataL, float * inOutDataR, const double beatTime, bool relativeTime=false);
        
        void setTempo(double bpm);
        double getTempo() const { return mTempo; }
        
        void setBeatsPerBar(int num);
        int getBeatsPerBar() const { return mBeatsPerBar; }
        
        
        void resetRelativeStart(double startBeatPos = 0.0);
        void setRemainRatios(double barRemain, double beatRemain);
        
        double getCurrBeatPos() const { return mCurrBeatPos; }
        
        void setGain(float val, bool force=false);
        float getGain() const { return mPendingGain; }
        
        // loads the sampleset containing the samples to use for the click.
        // they must have labels corresponding to the interval they will sound
        //  "beat" and "bar" (optional)
        //bool loadSampleSet(const SampleSetInfo & sinfo);
        
        //const SampleSetInfo & getCurrentSamplesetInfo() const { return m_sampleSet.getInfo(); }
        

    protected:
        int  blockSize;
        double sampleRate;

        double mTempo;
        int  mBeatsPerBar;
        float mGain;
        volatile float mPendingGain;

        double mCurrentBeatRemainRatio;
        
        double mCurrBeatPos;
        
        AudioSampleBuffer beatSoundBuffer;
        AudioSampleBuffer barSoundBuffer;

        AudioSampleBuffer tempBuffer;

        //SampleSet  m_sampleSet;
        //NonBlockingLock  mSampleLock;
        CriticalSection mSampleLock;

        int  currentBeatInBar;

        struct SoundTrack {
            SoundTrack() : sampleData(0), sampleLength(0) {}
            ~SoundTrack() { }
            float * sampleData; // not owned by us
            long sampleLength;
        };

        struct SampleState
        {
            SampleState() : samplePos(0) {}
            ~SampleState() { }
            long samplePos;
            SoundTrack soundTrackToPlay;

            void play(float *metbuf, long n) {
                long sampleRemain = soundTrackToPlay.sampleLength - samplePos;
                if (sampleRemain > 0)
                {
                    long sampleToPlay = std::min(n, sampleRemain);
                    for (long i = 0; i < sampleToPlay; ++i)
                    {
                        metbuf[i] += soundTrackToPlay.sampleData[samplePos + i];
                    }

                    samplePos += sampleToPlay;
                }
            }

            void start(SoundTrack soundTrack){
                soundTrackToPlay = soundTrack;
                samplePos = 0;
            }
        };


        SampleState mSampleState;

        SoundTrack beatSoundTrack;
        SoundTrack barSoundTrack;

        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Metronome)

        bool isBarBeatEnabled() const;

        bool isFirstBarBeat() const;

        void loadSoundFromBinaryData(const void *data, unsigned long sizeBytes, AudioSampleBuffer *soundBuffer, SoundTrack *track);
    };
}