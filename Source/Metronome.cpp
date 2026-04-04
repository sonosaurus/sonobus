// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell


#include "Metronome.h"
#include <cmath>
//#include "utility.h"

using namespace SonoAudio;

Metronome::Metronome(double samplerate)
: sampleRate(samplerate), mTempo(0), mBeatsPerBar(1), mGain(1.0f), mPendingGain(1.0f),
 mCurrentBarRemainRatio(0), mCurrentBeatRemainRatio(0), mCurrBeatPos(0)
{
    tempBuffer.setSize(1, 4096);
}

Metronome::~Metronome()
{
}

//void processData (int nframes, const signed short int *indata);

void Metronome::setGain(float val, bool force)
{
    mPendingGain = val;
    if (force) {
        mGain = val;
    }
}



// the timestamp passed in should be relative to time zero for the current tempo
void Metronome::processMix (int nframes, float * inOutDataL, float * inOutDataR, const double beatTime, bool relativeTime)
{
    const ScopedTryLock slock (mSampleLock);

    if (!slock.isLocked() || beatSoundBuffer.getNumSamples() == 0 || mTempo == 0.0) {
	    //cerr << "samples locked, not rendering" << endl;
	    return;
    }
    
    // just in case, shouldn't ever happen
    if (tempBuffer.getNumSamples() < nframes) {
        tempBuffer.setSize(1, nframes);
    }
    
    tempBuffer.clear(0, nframes);
    
    int frames = nframes;
    
    double beatInt;
    double barInt;
    double beatFrac = modf(beatTime, &beatInt);
    double barFrac = modf(beatTime / mBeatsPerBar, &barInt);
        
    long framesInBeat = (long) (sampleRate * 60 / mTempo);
    long framesInBar = mBeatsPerBar * framesInBeat;
    double framesInBeatF = (sampleRate * 60 / mTempo);

    
    long framesUntilBeat; 
    long framesUntilBar;
    
    if (relativeTime) {

        framesUntilBar = (long) lrint(mCurrentBarRemainRatio * framesInBar);
        framesUntilBeat = (long) lrint(mCurrentBeatRemainRatio * framesInBeat);
    }
    else {
        framesUntilBar =  (long) lrint(fmod(1.0 - barFrac, 1.0) * framesInBar);;
        framesUntilBeat = (long) lrint(fmod(1.0 - beatFrac, 1.0) * framesInBeat);

        //DBG("Beattime: " << beatTime << " framesuntilbeat: " << framesUntilBeat << "  beatfrac: " << beatFrac);
        mCurrBeatPos = beatTime;
    }
    
    if (framesUntilBeat != framesUntilBar && abs(framesUntilBeat - framesUntilBar) < framesInBeat / 2) {
        // force the framesuntilbar to be equal to framesuntilbeat if they are close but not equal
        framesUntilBar = framesUntilBeat;
    }
    
    float * metbuf = tempBuffer.getWritePointer(0);
    
    while (frames > 0)
    {
        if (framesUntilBar == 0 )
        {
            framesUntilBar = framesInBar;
            
            if (mBeatsPerBar > 1) {
                // start playback of bar sample
                framesUntilBeat = framesInBeat; // also reset beat so it doesn't play with the bar

                mBarState.sampleRemain = mBarState.sampleLength;
                mBarState.samplePos = 0;
                //printf("Starting bar playout for tempo %g  TS: %ld\n", mTempo, iTimestamp);
            }
        }
        
        if (framesUntilBeat == 0)
        {
            framesUntilBeat = framesInBeat;
            // start playback of beat sample
            
            mBeatState.sampleRemain = mBeatState.sampleLength;
            mBeatState.samplePos = 0;            
            //printf("Starting beat playout  for tempo %g   TS: %ld \n", mTempo, iTimestamp);            
        }
        
        // run enough samples to get to start of next beat/bar or what's left
        long n = std::max((long)1, std::min ((long)frames, std::min(framesUntilBar, framesUntilBeat)));
        
        if (mBarState.sampleRemain > 0)
        {
            // playing bar sound
            long barn = std::min (n, mBarState.sampleRemain);
            for (long i = 0; i < barn; ++i)
            {
                metbuf[i] += mBarState.sampleData[mBarState.samplePos + i];
            }
            
            mBarState.sampleRemain -= barn;
            mBarState.samplePos += barn;
        }
        
        if (mBeatState.sampleRemain > 0) 
        {
            // playing beat sound
            long beatn = std::min (n, mBeatState.sampleRemain);
            for (long i = 0; i < beatn; ++i)
            {
                metbuf[i] +=  mBeatState.sampleData[mBeatState.samplePos + i];
            }
            
            mBeatState.sampleRemain -= beatn;
            mBeatState.samplePos += beatn;
        }
        
                
        framesUntilBar -= n;
        framesUntilBeat -= n;
        frames -= n;
        //pInOutL += n;
        //pInOutR += n;
        metbuf += n;
    }
    
    // apply gain
    if (abs(mPendingGain - mGain) > 0.0001f) {
        tempBuffer.applyGainRamp(0, nframes, mGain, mPendingGain);
        mGain = mPendingGain;
    }
    else if (mGain != 1.0f){
        tempBuffer.applyGain(0, nframes, mGain);
    }
    
    // add to audio data going out
    FloatVectorOperations::add(inOutDataL, tempBuffer.getReadPointer(0), nframes);
    if (inOutDataR != inOutDataL) {
        FloatVectorOperations::add(inOutDataR, tempBuffer.getReadPointer(0), nframes);
    }

    mCurrentBarRemainRatio = framesUntilBar / (double)framesInBar;
    mCurrentBeatRemainRatio = framesUntilBeat / (double)framesInBeat;        
    
    if (relativeTime) {
        mCurrBeatPos += nframes / framesInBeatF;
    }
}

void Metronome::resetRelativeStart(double startBeatPos)
{
    mCurrBeatPos = startBeatPos;
    mCurrentBarRemainRatio = 0.0;
    mCurrentBeatRemainRatio = fmod(1.0 - fmod(mCurrBeatPos, 1.0), 1.0);
    mCurrentBarRemainRatio = fmod(1.0 - fmod(mCurrBeatPos / mBeatsPerBar, 1.0), 1.0);
}

void Metronome::setRemainRatios(double barRemain, double beatRemain)
{
    mCurrentBarRemainRatio = barRemain;
    mCurrentBeatRemainRatio = beatRemain;

    mCurrBeatPos = (1.0 - barRemain) * mBeatsPerBar;
}


void Metronome::setTempo(double bpm)
{
    mTempo = bpm;
}

void Metronome::setBeatsPerBar(int num)
{
    mBeatsPerBar = num;
}


void Metronome::loadBeatSoundFromBinaryData(const void* data, size_t sizeBytes)
{
    const ScopedLock slock (mSampleLock);

    WavAudioFormat wavFormat;
    std::unique_ptr<AudioFormatReader> reader (wavFormat.createReaderFor (new MemoryInputStream (data, sizeBytes, false), true));
    if (reader.get() != nullptr)
    {
        beatSoundBuffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
        reader->read(&beatSoundBuffer, 0, (int)reader->lengthInSamples, 0, true, true);
        DBG("Read beat sound of " << beatSoundBuffer.getNumSamples() << " samples");
        mBeatState.sampleData = beatSoundBuffer.getWritePointer(0);
        mBeatState.sampleLength = beatSoundBuffer.getNumSamples();
        mBeatState.samplePos = 0;
        mBeatState.sampleRemain = 0;
    }
}

void Metronome::loadBarSoundFromBinaryData(const void* data, size_t sizeBytes)
{
    const ScopedLock slock (mSampleLock);

    WavAudioFormat wavFormat;
    std::unique_ptr<AudioFormatReader> reader (wavFormat.createReaderFor (new MemoryInputStream (data, sizeBytes, false), true));
    if (reader.get() != nullptr)
    {
        barSoundBuffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
        reader->read(&barSoundBuffer, 0, (int)reader->lengthInSamples, 0, true, true);
        DBG("Read bar sound of " << barSoundBuffer.getNumSamples() << " samples");
        
        mBarState.sampleData = barSoundBuffer.getWritePointer(0);
        mBarState.sampleLength = barSoundBuffer.getNumSamples();
        mBarState.samplePos = 0;
        mBarState.sampleRemain = 0;
    }
}
