// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell



#pragma once

#include <list>
#include <optional>
#include "JuceHeader.h"
#include "ChannelGroup.h"
#include "Soundboard.h"

class SoundboardChannelProcessor;
class SamplePlaybackManager;

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
     * @param playbackManager The playback manager that is playing the audio.
     */
    virtual void onPlaybackPositionChanged(const SamplePlaybackManager& playbackManager) {};
};

class SamplePlaybackManager : private Timer, private ChangeListener
{
public:
    SamplePlaybackManager(const SoundSample* sample_, SoundboardChannelProcessor* channelProcessor_);
    ~SamplePlaybackManager() override;

    /**
     * Loads the file for playback.
     *
     * When the file is already loaded, this does nothing.
     *
     * @param fileReadThread thread to use for reading the file. The thread must be running.
     *
     * @return True when succeeded, or false when the file could not be loaded.
     */
    bool loadFileFromSample(TimeSliceThread& fileReadThread);

    /**
     * Applies playback settings from the sample to the player.
     *
     * Must be called when the playback options of the sample changed, in order to apply them to the current playback.
     */
    void reloadPlaybackSettingsFromSample();

    /**
     * Stop and unload current playback. Removes this playback manager from the channel processor.
     */
    void unload();

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
     * Returns whether a loaded file is currently playing.
     *
     * @return True when a file is currently playing, false otherwise.
     */
    bool isPlaying() const;

    /**
     * Retrieves the current position of the loaded file playback.
     *
     * @return The current playback position in seconds, or `0.0` when no file is currently loaded.
     */
    double getCurrentPosition() const;

    /**
     * Retrieves the length of the loaded file playback.
     *
     * @return The length in seconds of the current file playback, or `0.0` when no file is currently loaded.
     */
    double getLength() const;

    const SoundSample* getSample() const { return sample; };

    AudioSource* getAudioSource() { return &transportSource; };

    void attach(PlaybackPositionListener& listener) { listeners.emplace_back(&listener); }
    void detach(PlaybackPositionListener& listener) { listeners.remove(&listener); }

private:
    constexpr static const int READ_AHEAD_BUFFER_SIZE = 65536;
    constexpr static const int TIMER_HZ = 20;

    const SoundSample* sample;
    SoundboardChannelProcessor* channelProcessor;
    bool loaded;

    // The order in which these two members are defined is important!
    // The current file source should be cleaned up AFTER transport source performs its destructing operations,
    // as the transport source attempts to clean up some things in the current file source.
    // As Juce refuses to take responsibility of cleaning up the file source itself,
    // we must manage this explicitly ourselves and therefore make sure the order of the following lines does not change.
    std::unique_ptr<AudioFormatReaderSource> currentFileSource;
    AudioTransportSource transportSource;

    AudioFormatManager formatManager;

    std::list<PlaybackPositionListener*> listeners;

    void notifyPlaybackPositionListeners() const;

    void timerCallback() override;

    void changeListenerCallback(ChangeBroadcaster* source) override;
};

/**
 * Provides a player for a audio files on a soundboard.
 *
 * The channel processor is able to mix sound from multiple audio files.
 */
class SoundboardChannelProcessor
{
public:
    SoundboardChannelProcessor();
    ~SoundboardChannelProcessor();

    /**
     * Loads the playback of the given sample.
     *
     * Loads the audio file such that it can be played. The sample playback options are also configured.
     *
     * @param sample The sample to load.
     * @return The playback manager for this sample, or none when loading failed.
     */
    std::optional<std::shared_ptr<SamplePlaybackManager>> loadSample(const SoundSample& sample);

    /**
     * Finds the playback manager for the given sample, or none when the sample is not playing.
     */
    std::optional<std::shared_ptr<SamplePlaybackManager>> findPlaybackManager(const SoundSample& sample);

    foleys::LevelMeterSource& getMeterSource() { return meterSource; }

    SonoAudio::DelayParams& getMonitorDelayParams();
    void setMonitorDelayParams(SonoAudio::DelayParams& params);

    void getDestStartAndCount(int& start, int& count) const;
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

    void prepareToPlay(int sampleRate, int meterRmsWindow, int currentSamplesPerBlock);
    void ensureBuffers(int numSamples, int maxChannels, int meterRmsWindow);
    void processMonitor(AudioBuffer<float>& otherBuffer, int numSamples, int totalOutputChannels, float wet = 1.0, bool recordChannel = false);

    /**
     * Process an incoming audio block.
     *
     * @return true whether an audio block was processed, false otherwise.
     */
    bool processAudioBlock(int numSamples);
    void sendAudioBlock(AudioBuffer<float>& sendWorkBuffer, int numSamples, int sendPanChannels, int startChannel);

    void releaseResources();

    void notifyStopped(SamplePlaybackManager* samplePlaybackManager);

    std::unordered_map<const SoundSample*, std::shared_ptr<SamplePlaybackManager>>& getActiveSamples() { return activeSamples; }

private:
    MixerAudioSource mixer;
    std::unordered_map<const SoundSample*, std::shared_ptr<SamplePlaybackManager>> activeSamples;

    AudioSampleBuffer buffer;
    foleys::LevelMeterSource meterSource;
    SonoAudio::ChannelGroup channelGroup;
    SonoAudio::ChannelGroup recordChannelGroup;

    TimeSliceThread diskThread { "soundboard audio file reader" };

    float lastGain = 0.0f;
};
