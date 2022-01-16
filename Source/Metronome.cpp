// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell


#include "Metronome.h"
#include <cmath>

using namespace SonoAudio;

Metronome::Metronome(double samplerate)
: sampleRate(samplerate), mTempo(100), mGain(1.0f), mPendingGain(1.0f),
  mCurrentBeatRemainRatio(0), mCurrBeatPos(0)
{
    loadBeatSoundFromBinaryData(BinaryData::beat_click_wav, BinaryData::beat_click_wavSize);
}

Metronome::~Metronome()
{
}

void Metronome::setGain(float val, bool force)
{
    mPendingGain = val;
    if (force) {
        mGain = val;
    }
}

void Metronome::turn(bool on) {
    if(on){
        setGain(getGain(), true);
        mCurrentBeatRemainRatio = 0.0;
        mCurrBeatPos = 0.0;
    }
}

// the timestamp passed in should be relative to time zero for the current tempo
void Metronome::processMix(AudioSampleBuffer *sampleBuffer, const double beatTime, bool relativeTime)
{
    const ScopedTryLock slock (mSampleLock);

    if (!slock.isLocked()) {
	    return;
    }
    
    sampleBuffer->clear();

    auto insideFrames = sampleBuffer->getNumSamples();

    long framesInBeat = (long) (sampleRate * 60 / mTempo.get());
    double framesInBeatF = (sampleRate * 60 / mTempo.get());

    long framesUntilBeat; 

    if (relativeTime) {
        framesUntilBeat = (long) lrint(mCurrentBeatRemainRatio * framesInBeat);
    }
    else {
        double beatInt;
        double beatFrac = modf(beatTime, &beatInt);
        framesUntilBeat = (long) lrint((1.0 - beatFrac) * framesInBeat);
        mCurrBeatPos = beatTime;
    }
    
    float * metbuf = sampleBuffer->getWritePointer(0);

    int remainingFramesToGenerate = insideFrames;

    while (remainingFramesToGenerate > 0)
    {
        if (framesUntilBeat == 0)
        {
            framesUntilBeat = framesInBeat;
            // start playback of beat sample
            
            mBeatState.sampleRemain = mBeatState.sampleLength;
            mBeatState.samplePos = 0;            
            //printf("Starting beat playout  for tempo %g   TS: %ld \n", mTempo, iTimestamp);            
        }
        
        // run enough samples to get to start of next beat or what's left
        long n = std::max((long)1, std::min ((long)remainingFramesToGenerate, framesUntilBeat));
        
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
        
                
        framesUntilBeat -= n;
        remainingFramesToGenerate -= n;
        metbuf += n;
    }
    
    // apply gain
    if (abs(mPendingGain - mGain.get()) > 0.0001f) {
        sampleBuffer->applyGainRamp(0, insideFrames, mGain.get(), mPendingGain);
        mGain = mPendingGain;
    }
    else if (mGain.get() != 1.0f){
        sampleBuffer->applyGain(0, insideFrames, mGain.get());
    }

    mCurrentBeatRemainRatio = framesUntilBeat / (double)framesInBeat;
    
    if (relativeTime) {
        mCurrBeatPos += insideFrames / framesInBeatF;
    }
}

void Metronome::loadBeatSoundFromBinaryData(const void* data, size_t sizeBytes)
{
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

void Metronome::setTempo(double bpm)
{
    mTempo = bpm;
}
