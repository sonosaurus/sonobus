//
// Created by Sten on 24-10-2021.
//

#include "SoundboardProcessor.h"

SoundboardProcessor::SoundboardProcessor()
{
    channelGroup.params.name = TRANS("Soundboard");
    channelGroup.params.numChannels = 2;
    recordChannelGroup.params.name = TRANS("Soundboard");
    recordChannelGroup.params.numChannels = 2;

    transportSource.addChangeListener(this);
    formatManager.registerBasicFormats();
}

SoundboardProcessor::~SoundboardProcessor()
{
    transportSource.setSource(nullptr);
    transportSource.removeChangeListener(this);
}

SonoAudio::DelayParams& SoundboardProcessor::getMonitorDelayParams()
{
    return channelGroup.params.monitorDelayParams;
}

void SoundboardProcessor::setMonitorDelayParams(SonoAudio::DelayParams& params)
{
    channelGroup.params.monitorDelayParams = params;
    channelGroup.commitMonitorDelayParams();

    recordChannelGroup.params.monitorDelayParams = params;
    recordChannelGroup.commitMonitorDelayParams();
}

void SoundboardProcessor::getDestStartAndCount(int& start, int& count)
{
    start = channelGroup.params.monDestStartIndex;
    count = channelGroup.params.monDestChannels;
}

void SoundboardProcessor::setDestStartAndCount(int start, int count)
{
    channelGroup.params.monDestStartIndex = start;
    channelGroup.params.monDestChannels = std::max(1, std::min(count, MAX_CHANNELS));
    channelGroup.commitMonitorDelayParams();

    recordChannelGroup.params.monDestStartIndex = start;
    recordChannelGroup.params.monDestChannels = std::max(1, std::min(count, MAX_CHANNELS));
    recordChannelGroup.commitMonitorDelayParams();
}

float SoundboardProcessor::getGain() const
{
    return channelGroup.params.gain;
}

void SoundboardProcessor::setGain(float gain)
{
    channelGroup.params.gain = gain;
    recordChannelGroup.params.gain = gain;
}

float SoundboardProcessor::getMonitorGain() const
{
    return channelGroup.params.monitor;
}

void SoundboardProcessor::setMonitorGain(float gain)
{
    channelGroup.params.monitor = gain;
    recordChannelGroup.params.monitor = gain;
}

int SoundboardProcessor::getNumberOfChannels() const
{
    return channelGroup.params.numChannels;
}

int SoundboardProcessor::getFileSourceNumberOfChannels() const
{
    return currentFileSource != nullptr ? currentFileSource->getAudioFormatReader()->numChannels : 2;
}

SonoAudio::ChannelGroupParams SoundboardProcessor::getChannelGroupParams() const
{
    return channelGroup.params;
}

void SoundboardProcessor::prepareToPlay(const int sampleRate, const int meterRmsWindow, const int currentSamplesPerBlock)
{
    transportSource.prepareToPlay(currentSamplesPerBlock, sampleRate);

    const int numChannels = getFileSourceNumberOfChannels();

    meterSource.resize(numChannels, meterRmsWindow);
    channelGroup.init(sampleRate);
    recordChannelGroup.init(sampleRate);
}

void SoundboardProcessor::ensureBuffers(const int numSamples, const int maxChannels, const int meterRmsWindow)
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

void SoundboardProcessor::processMonitor(AudioBuffer<float>& otherBuffer, int numSamples, int totalOutputChannels, float wet, bool recordChannel)
{
    int dstch = (recordChannel ? recordChannelGroup : channelGroup).params.monDestStartIndex;
    int dstcnt = jmin(totalOutputChannels, (recordChannel ? recordChannelGroup : channelGroup).params.monDestChannels);
    auto fgain = (recordChannel ? recordChannelGroup : channelGroup).params.gain * wet;

    (recordChannel ? recordChannelGroup : channelGroup).processMonitor(buffer, 0, otherBuffer, dstch, dstcnt, numSamples, fgain);
}

void SoundboardProcessor::changeListenerCallback(ChangeBroadcaster* source)
{
    if (!transportSource.isPlaying() && transportSource.getCurrentPosition() >= transportSource.getLengthInSeconds()) {
        // We are at the end, return to start
        transportSource.setPosition(0.0);
    }
}

bool SoundboardProcessor::processAudioBlock(int numSamples)
{
    if (transportSource.getTotalLength() <= 0) {
        return false;
    }

    AudioSourceChannelInfo info(&buffer, 0, numSamples);
    transportSource.getNextAudioBlock(info);

    meterSource.measureBlock(buffer);

    int sourceChannels = getFileSourceNumberOfChannels();
    channelGroup.params.numChannels = sourceChannels;
    channelGroup.commitMonitorDelayParams();
    recordChannelGroup.params.numChannels = sourceChannels;
    recordChannelGroup.commitMonitorDelayParams();

    return true;
}

void SoundboardProcessor::sendAudioBlock(AudioBuffer<float>& sendWorkBuffer, int numSamples, int sendPanChannels, int startChannel)
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

void SoundboardProcessor::releaseResources()
{
    transportSource.releaseResources();
}

void SoundboardProcessor::unloadFile()
{
    pause();
    transportSource.setSource(nullptr);
    currentFileSource.reset();
    currentTransportURL = URL();
}

bool SoundboardProcessor::loadFile(const URL& audioFileUrl)
{
    if (!diskThread.isThreadRunning()) {
        diskThread.startThread(3);
    }

    // Unload the previous file
    unloadFile();

    AudioFormatReader* reader = nullptr;

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

    currentTransportURL = URL(audioFileUrl);
    currentFileSource.reset(new AudioFormatReaderSource(reader, true));

    // TODO: I'm not sure if this is really necessary. It would really complicate separation of concerns though...
//    transportSource.prepareToPlay(currentSamplesPerBlock, currentSampleRate);

    transportSource.setSource(currentFileSource.get(), READ_AHEAD_BUFFER_SIZE, &diskThread, reader->sampleRate, reader->numChannels);

    return true;
}

void SoundboardProcessor::play()
{
    transportSource.start();
}

void SoundboardProcessor::pause()
{
    transportSource.stop();
}

void SoundboardProcessor::seek(double position)
{
    transportSource.setPosition(position);
}

bool SoundboardProcessor::isPlaying()
{
    return transportSource.isPlaying();
}

double SoundboardProcessor::getCurrentPosition()
{
    return transportSource.getCurrentPosition();
}

double SoundboardProcessor::getLength()
{
    return transportSource.getLengthInSeconds();
}
