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
void Metronome::processMix (int windowSizeInSamples, float * inOutDataL, float * inOutDataR, const double beatTime, bool relativeTime)
{
    const ScopedTryLock slock (mSampleLock);

    if (!slock.isLocked() || beatSoundBuffer.getNumSamples() == 0 || mTempo == 0.0) {
	    //cerr << "samples locked, not rendering" << endl;
	    return;
    }
    
    // just in case, shouldn't ever happen
    if (tempBuffer.getNumSamples() < windowSizeInSamples) {
        tempBuffer.setSize(1, windowSizeInSamples);
    }
    
    tempBuffer.clear(0, windowSizeInSamples);
    
    int remainingSampleToPlay = windowSizeInSamples;
    
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
    
    while (remainingSampleToPlay > 0)
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
        long n = std::max((long)1, std::min ((long)remainingSampleToPlay, std::min(framesUntilBar, framesUntilBeat)));

        mBarState.play(metbuf, n);
        mBeatState.play(metbuf, n);

        framesUntilBar -= n;
        framesUntilBeat -= n;
        remainingSampleToPlay -= n;
        //pInOutL += n;
        //pInOutR += n;
        metbuf += n;
    }
    
    // apply gain
    if (abs(mPendingGain - mGain) > 0.0001f) {
        tempBuffer.applyGainRamp(0, windowSizeInSamples, mGain, mPendingGain);
        mGain = mPendingGain;
    }
    else if (mGain != 1.0f){
        tempBuffer.applyGain(0, windowSizeInSamples, mGain);
    }
    
    // add to audio data going out
    FloatVectorOperations::add(inOutDataL, tempBuffer.getReadPointer(0), windowSizeInSamples);
    if (inOutDataR != inOutDataL) {
        FloatVectorOperations::add(inOutDataR, tempBuffer.getReadPointer(0), windowSizeInSamples);
    }

    mCurrentBarRemainRatio = framesUntilBar / (double)framesInBar;
    mCurrentBeatRemainRatio = framesUntilBeat / (double)framesInBeat;        
    
    if (relativeTime) {
        mCurrBeatPos += windowSizeInSamples / framesInBeatF;
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
