// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell

#include "SoundboardChannelProcessor.h"

SamplePlaybackManager::SamplePlaybackManager(SoundSample* sample_, SoundboardChannelProcessor* channelProcessor_)
    : sample(sample_), channelProcessor(channelProcessor_), loaded(false)
{
    formatManager.registerBasicFormats();
    transportSource.addChangeListener(this);
}

SamplePlaybackManager::~SamplePlaybackManager()
{
    stopTimer();
    transportSource.removeChangeListener(this);
}

bool SamplePlaybackManager::loadFileFromSample(TimeSliceThread &fileReadThread)
{
    if (loaded) return true;
    if (!fileReadThread.isThreadRunning()) return false;

    AudioFormatReader* reader = nullptr;
    auto audioFileUrl = URL(File(sample->getFilePath()));

#if !JUCE_IOS
    if (audioFileUrl.isLocalFile()) {
        reader = formatManager.createReaderFor(audioFileUrl.getLocalFile());
    }
    else
#endif
    {
        reader = formatManager.createReaderFor(audioFileUrl.createInputStream(false));
    }

    if (reader == nullptr) {
        return false;
    }

    currentFileSource = std::make_unique<AudioFormatReaderSource>(reader, true);
    transportSource.setSource(currentFileSource.get(), READ_AHEAD_BUFFER_SIZE, &fileReadThread, reader->sampleRate, 2);

    reloadPlaybackSettingsFromSample();

    loaded = true;
    return true;
}

void SamplePlaybackManager::reloadPlaybackSettingsFromSample()
{
    transportSource.setLooping(sample->isLoop());
    transportSource.setGain(sample->getGain());
}

void SamplePlaybackManager::unload()
{
    stopTimer();
    transportSource.stop();
    sample->setLastPlaybackPosition(transportSource.getCurrentPosition());
}

void SamplePlaybackManager::play()
{
    transportSource.start();
    startTimerHz(TIMER_HZ);
}

void SamplePlaybackManager::pause()
{
    unload();
}

void SamplePlaybackManager::seek(double position)
{
    transportSource.setPosition(position);
    notifyPlaybackPositionListeners();
}

void SamplePlaybackManager::setGain(float gain)
{
    transportSource.setGain(gain);
}

bool SamplePlaybackManager::isPlaying() const
{
    return transportSource.isPlaying();
}

double SamplePlaybackManager::getCurrentPosition() const
{
    return transportSource.getCurrentPosition();
}

double SamplePlaybackManager::getLength() const
{
    return transportSource.getLengthInSeconds();
}

void SamplePlaybackManager::notifyPlaybackPositionListeners() const
{
    for (auto& listener : listeners) {
        listener->onPlaybackPositionChanged(*this);
    }
}

void SamplePlaybackManager::timerCallback()
{
    notifyPlaybackPositionListeners();
}

void SamplePlaybackManager::changeListenerCallback(ChangeBroadcaster* source)
{
    if (!transportSource.isPlaying() && transportSource.getCurrentPosition() >= transportSource.getLengthInSeconds()) {
        // We are at the end, return to start
        transportSource.setPosition(0.0);
        sample->setLastPlaybackPosition(0.0);
        notifyPlaybackPositionListeners();
    }

    if (!transportSource.isPlaying()) {
        if (sample->getReplayBehaviour() == SoundSample::ReplayBehaviour::REPLAY_FROM_START) {
            transportSource.setPosition(0.0);
            notifyPlaybackPositionListeners();
        }

        // Notify the channel processor that this instance can be removed from the mixer
        // THIS MIGHT CAUSE THAT THIS OBJECT IS REMOVED!
        // Therefore, make sure this is the last statement executed within this class
        channelProcessor->notifyStopped(this);
    }

}

SoundboardChannelProcessor::SoundboardChannelProcessor()
{
    channelGroup.params.name = TRANS("Soundboard");
    channelGroup.params.numChannels = 2;
    recordChannelGroup.params.name = TRANS("Soundboard");
    recordChannelGroup.params.numChannels = 2;
}

SoundboardChannelProcessor::~SoundboardChannelProcessor()
{
    mixer.removeAllInputs();
}

std::optional<std::shared_ptr<SamplePlaybackManager>> SoundboardChannelProcessor::loadSample(SoundSample& sample)
{
    // Reset playback position to beginning when sample is already playing.
    auto existingManager = findPlaybackManager(sample);
    if (existingManager.has_value()) {
        auto manager = *existingManager;
        manager->seek(0.0);
        manager->setGain(sample.getGain());
        return manager;
    }

    auto manager = std::make_shared<SamplePlaybackManager>(&sample, this);

    if (!diskThread.isThreadRunning()) {
        diskThread.startThread(3);
    }

    auto loaded = manager->loadFileFromSample(diskThread);
    if (!loaded) {
        return {};
    }

    // Check playback behaviour
    switch (sample.getPlaybackBehaviour()) {
        case SoundSample::SIMULTANEOUS:
        case SoundSample::BACKGROUND:
            break;
        case SoundSample::BACK_TO_BACK:
            unloadAllNonBackground();
            break;
    }

    manager->setGain(sample.getGain());

    // Check replay behaviour
    switch (sample.getReplayBehaviour()) {
        case SoundSample::ReplayBehaviour::REPLAY_FROM_START:
            break;
        case SoundSample::ReplayBehaviour::CONTINUE_FROM_LAST_POSITION:
            manager->seek(sample.getLastPlaybackPosition());
            break;
    }

    activeSamples[&sample] = manager;
    mixer.addInputSource(manager->getAudioSource(), false);

    return manager;
}

std::optional<std::shared_ptr<SamplePlaybackManager>> SoundboardChannelProcessor::findPlaybackManager(const SoundSample& sample)
{
    auto existingManager = activeSamples.find(&sample);
    if (existingManager != activeSamples.end()) {
        return existingManager->second;
    }

    return {};
}

SonoAudio::DelayParams& SoundboardChannelProcessor::getMonitorDelayParams()
{
    return channelGroup.params.monitorDelayParams;
}

void SoundboardChannelProcessor::setMonitorDelayParams(SonoAudio::DelayParams& params)
{
    channelGroup.params.monitorDelayParams = params;
    channelGroup.commitMonitorDelayParams();

    recordChannelGroup.params.monitorDelayParams = params;
    recordChannelGroup.commitMonitorDelayParams();
}

void SoundboardChannelProcessor::getDestStartAndCount(int& start, int& count) const
{
    start = channelGroup.params.monDestStartIndex;
    count = channelGroup.params.monDestChannels;
}

void SoundboardChannelProcessor::setDestStartAndCount(int start, int count)
{
    channelGroup.params.monDestStartIndex = start;
    channelGroup.params.monDestChannels = std::max(1, std::min(count, MAX_CHANNELS));
    channelGroup.commitMonitorDelayParams();

    recordChannelGroup.params.monDestStartIndex = start;
    recordChannelGroup.params.monDestChannels = std::max(1, std::min(count, MAX_CHANNELS));
    recordChannelGroup.commitMonitorDelayParams();
}

float SoundboardChannelProcessor::getGain() const
{
    return channelGroup.params.gain;
}

void SoundboardChannelProcessor::setGain(float gain)
{
    channelGroup.params.gain = gain;
    recordChannelGroup.params.gain = gain;
}

float SoundboardChannelProcessor::getMonitorGain() const
{
    return channelGroup.params.monitor;
}

void SoundboardChannelProcessor::setMonitorGain(float gain)
{
    channelGroup.params.monitor = gain;
    recordChannelGroup.params.monitor = gain;
}

int SoundboardChannelProcessor::getNumberOfChannels() const
{
    return channelGroup.params.numChannels;
}

int SoundboardChannelProcessor::getFileSourceNumberOfChannels() const
{
    // Always process in stereo, to prevent mixing issues when multiple sources use a different number of channels
    return 2;
}

SonoAudio::ChannelGroupParams SoundboardChannelProcessor::getChannelGroupParams() const
{
    return channelGroup.params;
}

void SoundboardChannelProcessor::prepareToPlay(const int sampleRate, const int meterRmsWindow, const int currentSamplesPerBlock)
{
    mixer.prepareToPlay(currentSamplesPerBlock, sampleRate);

    const int numChannels = getFileSourceNumberOfChannels();

    meterSource.resize(numChannels, meterRmsWindow);
    channelGroup.init(sampleRate);
    recordChannelGroup.init(sampleRate);
}

void SoundboardChannelProcessor::ensureBuffers(const int numSamples, const int maxChannels, const int meterRmsWindow)
{
    const int numChannels = getFileSourceNumberOfChannels();
    const int realMaxChannels = jmax(maxChannels, numChannels);

    if (meterSource.getNumChannels() < numChannels) {
        meterSource.resize(numChannels, meterRmsWindow);
    }

    if (buffer.getNumSamples() < numSamples || buffer.getNumChannels() != realMaxChannels) {
        buffer.setSize(realMaxChannels, numSamples, false, false, true);
    }
}

void SoundboardChannelProcessor::processMonitor(AudioBuffer<float>& otherBuffer, int numSamples, int totalOutputChannels, float wet, bool recordChannel)
{
    int dstch = (recordChannel ? recordChannelGroup : channelGroup).params.monDestStartIndex;
    int dstcnt = jmin(totalOutputChannels, (recordChannel ? recordChannelGroup : channelGroup).params.monDestChannels);
    auto fgain = (recordChannel ? recordChannelGroup : channelGroup).params.gain * wet;

    (recordChannel ? recordChannelGroup : channelGroup).processMonitor(buffer, 0, otherBuffer, dstch, dstcnt, numSamples, fgain);
}

bool SoundboardChannelProcessor::processAudioBlock(int numSamples)
{
    AudioSourceChannelInfo info(&buffer, 0, numSamples);
    mixer.getNextAudioBlock(info);

    if (buffer.hasBeenCleared()) {
        return false;
    }

    meterSource.measureBlock(buffer);

    int sourceChannels = getFileSourceNumberOfChannels();
    channelGroup.params.numChannels = sourceChannels;
    channelGroup.commitMonitorDelayParams();
    recordChannelGroup.params.numChannels = sourceChannels;
    recordChannelGroup.commitMonitorDelayParams();

    return true;
}

void SoundboardChannelProcessor::sendAudioBlock(AudioBuffer<float>& sendWorkBuffer, int numSamples, int sendPanChannels, int startChannel)
{
    int sourceChannels = getFileSourceNumberOfChannels();
    float gain = channelGroup.params.gain;

    if (sendPanChannels == 1) {
        // add to main buffer for going out, mix as appropriate depending on how many channels being sent
        if (sourceChannels > 0) {
            gain *= (1.0f/std::max(1.0f, (float)(sourceChannels)));
        }

        for (int channel = 0; channel < sourceChannels; ++channel) {
            sendWorkBuffer.addFromWithRamp(0, 0, buffer.getReadPointer(channel), numSamples, gain, lastGain);
        }
    }
    else if (sendPanChannels > 2) {
        // straight-tru
        int destinationChannel = startChannel;
        for( int channel = 0; channel < sourceChannels && destinationChannel < sendWorkBuffer.getNumChannels(); ++channel) {
            sendWorkBuffer.addFromWithRamp(destinationChannel, 0, buffer.getReadPointer(channel), numSamples, gain, lastGain);
            ++destinationChannel;
        }
    }
    else if (sendPanChannels == 2) {
        // change dest ch target
        int dstch = channelGroup.params.panDestStartIndex;  // todo change dest ch target
        int dstcnt = jmin(sendPanChannels, channelGroup.params.panDestChannels);

        channelGroup.processPan(buffer, 0, sendWorkBuffer, dstch, dstcnt, numSamples, gain);
    }

    lastGain = gain;
}

void SoundboardChannelProcessor::releaseResources()
{
    mixer.releaseResources();
}

void SoundboardChannelProcessor::unloadAll()
{
    for (const auto &item : activeSamples) {
        item.second->unload();
    }
}

void SoundboardChannelProcessor::unloadAllNonBackground()
{
    for (const auto &item : activeSamples) {
        if (item.second->getSample()->getPlaybackBehaviour() != SoundSample::BACKGROUND) {
            item.second->unload();
        }
    }
}

void SoundboardChannelProcessor::notifyStopped(SamplePlaybackManager* samplePlaybackManager)
{
    mixer.removeInputSource(samplePlaybackManager->getAudioSource());
    activeSamples.erase(samplePlaybackManager->getSample());
}
