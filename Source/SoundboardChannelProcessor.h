// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell



#pragma once

#include "JuceHeader.h"
#include "ChannelGroup.h"

/**
 * Provides a player for a single audio file.
 */
class SoundboardChannelProcessor : public ChangeListener
{
private:
    constexpr static const int READ_AHEAD_BUFFER_SIZE = 65536;

    AudioSampleBuffer buffer;
    foleys::LevelMeterSource meterSource;
    SonoAudio::ChannelGroup channelGroup;
    SonoAudio::ChannelGroup recordChannelGroup;

    AudioTransportSource transportSource;
    std::unique_ptr<AudioFormatReaderSource> currentFileSource;
    AudioFormatManager formatManager;
    TimeSliceThread diskThread { "soundboard audio file reader" };
    URL currentTransportURL;

    float lastGain = 0.0f;

    /**
     * Unloads the currently loaded file (if any).
     */
    void unloadFile();

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
};
