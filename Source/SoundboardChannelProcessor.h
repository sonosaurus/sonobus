// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell



#pragma once

#include <list>
#include "JuceHeader.h"
#include "ChannelGroup.h"

class SoundboardChannelProcessor;

class PlaybackPositionListener
{
public:
    virtual ~PlaybackPositionListener() = default;

    /**
     * Called when playback position of the currently playing file is changed.
     *
     * Note: during file playback, this is called repeatedly with a fixed time interval.
     * It may be that the playback position has not actually changed value in this time.
     *
     * @param channelProcessor The channel processor that is playing the audio.
     */
    virtual void onPlaybackPositionChanged(SoundboardChannelProcessor& channelProcessor) {};
};

/**
 * Provides a player for a single audio file.
 */
class SoundboardChannelProcessor : public ChangeListener, private Timer
{
private:
    constexpr static const int READ_AHEAD_BUFFER_SIZE = 65536;
    constexpr static const int TIMER_HZ = 20;

    AudioSampleBuffer buffer;
    foleys::LevelMeterSource meterSource;
    SonoAudio::ChannelGroup channelGroup;
    SonoAudio::ChannelGroup recordChannelGroup;

    AudioTransportSource transportSource;
    std::unique_ptr<AudioFormatReaderSource> currentFileSource;
    AudioFormatManager formatManager;
    TimeSliceThread diskThread { "soundboard audio file reader" };

    float lastGain = 0.0f;

    std::list<PlaybackPositionListener*> listeners;

    /**
     * Unloads the currently loaded file (if any).
     */
    void unloadFile();

    void timerCallback() override;

    void notifyPlaybackPositionListeners();

public:
    SoundboardChannelProcessor();
    ~SoundboardChannelProcessor();

    /**
     * Loads an audio file.
     *
     * @param audioFileUrl URL to the file.
     * @return True whether loading succeeded, false otherwise.
     */
    bool loadFile(const URL& audioFileUrl);

    /**
     * Starts or resumes the playing of the loaded file.
     *
     * Has no effect when no file is loaded.
     */
    void play();

    /**
     * Pauses the playing of the loaded file.
     */
    void pause();

    /**
     * Seeks the currently loaded file to the given position.
     *
     * Has no effect when no file is loaded.
     *
     * @param position The new position, in seconds.
     */
    void seek(double position);

    /**
     * Sets whether the player should loop the current file.
     *
     * When loading a new file, the looping setting is remembered.
     *
     * @param looping Whether the player should loop the file.
     */
    void setLooping(bool looping);

    /**
     * Returns whether a loaded file is currently playing.
     *
     * @return True when a file is currently playing, false otherwise.
     */
    bool isPlaying();

    /**
     * Retrieves the current position of the loaded file playback.
     *
     * @return The current playback position in seconds, or `0.0` when no file is currently loaded.
     */
    double getCurrentPosition();

    /**
     * Retrieves the length of the loaded file playback.
     *
     * @return The length in seconds of the current file playback, or `0.0` when no file is currently loaded.
     */
    double getLength();

    foleys::LevelMeterSource& getMeterSource() { return meterSource; }

    SonoAudio::DelayParams& getMonitorDelayParams();
    void setMonitorDelayParams(SonoAudio::DelayParams& params);

    void getDestStartAndCount(int& start, int& count);
    void setDestStartAndCount(int start, int count);

    float getGain() const;
    void setGain(float gain);

    float getMonitorGain() const;
    void setMonitorGain(float gain);

    int getNumberOfChannels() const;
    int getFileSourceNumberOfChannels() const;

    /**
     * Get a hard copy of the parameters of the channel group.
     * @return Hard copy of the channel group parameters.
     */
    SonoAudio::ChannelGroupParams getChannelGroupParams() const;

    void prepareToPlay(int sampleRate, int meterRmsWindow, const int currentSamplesPerBlock);
    void ensureBuffers(int numSamples, int maxChannels, int meterRmsWindow);
    void processMonitor(AudioBuffer<float>& otherBuffer, int numSamples, int totalOutputChannels, float wet = 1.0, bool recordChannel = false);

    void changeListenerCallback(ChangeBroadcaster* source) override;

    /**
     * Process an incoming audio block.
     *
     * @return true whether an audio block was processed, false otherwise.
     */
    bool processAudioBlock(int numSamples);
    void sendAudioBlock(AudioBuffer<float>& sendWorkBuffer, int numSamples, int sendPanChannels, int startChannel);

    void releaseResources();

    void attach(PlaybackPositionListener& listener) { listeners.emplace_back(&listener); }

    void detach(PlaybackPositionListener& listener) { listeners.remove(&listener); }
};
