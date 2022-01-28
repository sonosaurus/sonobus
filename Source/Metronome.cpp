// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell


#include "Metronome.h"
#include <cmath>

using namespace SonoAudio;

Metronome::Metronome(const void* beatSoundData, size_t beatSoundDataSizeBytes,
                     const void* barSoundData, size_t barSoundDataSizeBytes)
: sampleRate(44100.0), mTempo(0), mGain(1.0f), mPendingGain(1.0f),
  mCurrentBeatRemainRatio(0), mCurrBeatPos(0)
{
    loadSoundFromBinaryData(beatSoundData, beatSoundDataSizeBytes, &beatSoundTrack);
    loadSoundFromBinaryData(barSoundData, barSoundDataSizeBytes, &barSoundTrack);
    tempBuffer.setSize(1, 4096);
    bar = new Bar(1);
}

Metronome::~Metronome() {}

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

    if (!slock.isLocked() || mTempo == 0.0) {
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
    double beatFrac = modf(beatTime, &beatInt);

    long framesInBeat = (long) (sampleRate * 60 / mTempo);
    double framesInBeatF = (sampleRate * 60 / mTempo);

    
    long framesUntilBeat; 

    if (relativeTime) {
        framesUntilBeat = (long) lrint(mCurrentBeatRemainRatio * framesInBeat);
    }
    else {
        framesUntilBeat = (long) lrint(fmod(1.0 - beatFrac, 1.0) * framesInBeat);
        mCurrBeatPos = beatTime;
    }
    
    float * metbuf = tempBuffer.getWritePointer(0);
    
    while (remainingSampleToPlay > 0)
    {
        if (framesUntilBeat == 0)
        {
            framesUntilBeat = framesInBeat;

            bar->tick();
            auto soundTrack = chooseSoundTrackUsing(bar);
            mSampleState.start(soundTrack);
        }
        
        // run enough samples to get to start of next beat/bar or what's left
        long sampleToPlayInThisIteration = std::max((long)1, std::min ((long)remainingSampleToPlay, framesUntilBeat));

        mSampleState.play(metbuf, sampleToPlayInThisIteration);

        framesUntilBeat -= sampleToPlayInThisIteration;
        remainingSampleToPlay -= sampleToPlayInThisIteration;
        metbuf += sampleToPlayInThisIteration;
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

    mCurrentBeatRemainRatio = framesUntilBeat / (double)framesInBeat;
    
    if (relativeTime) {
        mCurrBeatPos += windowSizeInSamples / framesInBeatF;
    }
}

void Metronome::resetRelativeStart(double startBeatPos)
{
    mCurrBeatPos = startBeatPos;
    mCurrentBeatRemainRatio = fmod(1.0 - fmod(mCurrBeatPos, 1.0), 1.0);
    bar->reset();
}

void Metronome::setRemainRatios(double barRemain, double beatRemain)
{
    mCurrentBeatRemainRatio = beatRemain;
    mCurrBeatPos = (1.0 - barRemain) * bar->getBeatsPerBar();
}

void Metronome::setTempo(double bpm)
{
    mTempo = bpm;
}

void Metronome::setBeatsPerBar(int num)
{
    bar->setBeatsPerBar(num);
}


void Metronome::loadSoundFromBinaryData(const void *data, unsigned long sizeBytes, SoundTrack *track)
{
    const ScopedLock slock (mSampleLock);

    WavAudioFormat wavFormat;
    std::unique_ptr<AudioFormatReader> reader (wavFormat.createReaderFor (new MemoryInputStream (data, sizeBytes, false), true));
    if (reader.get() != nullptr)
    {
        track->soundBuffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
        reader->read(&track->soundBuffer, 0, (int)reader->lengthInSamples, 0, true, true);
        DBG("Read beat sound of " << track->soundBuffer.getNumSamples() << " samples");
    }
}

Metronome::SoundTrack *Metronome::chooseSoundTrackUsing(Bar *bar) {
    if (bar->isBarBeatEnabled() && bar->isFirstBeat()) {
        return &barSoundTrack;
    }
    return &beatSoundTrack;
}
