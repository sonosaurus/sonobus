/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "aoo/aoo.hpp"

#define MAX_PEERS 32

//==============================================================================
/**
*/
class FluxAoOAudioProcessor  : public AudioProcessor, public AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    FluxAoOAudioProcessor();
    ~FluxAoOAudioProcessor();

    enum AutoNetBufferMode {
        AutoNetBufferModeOff = 0,
        AutoNetBufferModeAutoIncreaseOnly,
        AutoNetBufferModeAutoFull        
    };
    
    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void parameterChanged (const String &parameterID, float newValue) override;

    AudioProcessorValueTreeState& getValueTreeState();

    static String paramInGain;
    static String paramInMonitorPan1;
    static String paramInMonitorPan2;
    static String paramDry;
    static String paramWet;
    static String paramBufferTime;

    struct EndpointState;
    struct RemoteSink;
    struct RemoteSource;
    struct RemotePeer;
    
    int32_t handleSourceEvents(const aoo_event ** events, int32_t n, int32_t sourceId);
    int32_t handleSinkEvents(const aoo_event ** events, int32_t n, int32_t sinkId);

    
    EndpointState * findOrAddEndpoint(const String & host, int port);

    int getUdpLocalPort() const { return mUdpLocalPort; }
    IPAddress getLocalIPAddress() const { return mLocalIPAddress; }
    
  
    int connectRemotePeer(const String & host, int port, bool reciprocate=true);
    bool disconnectRemotePeer(const String & host, int port, int32_t sourceId);
    bool disconnectRemotePeer(int index);
    bool removeRemotePeer(int index);

    int getNumberRemotePeers() const;

    void setRemotePeerLevelGain(int index, float levelgain);
    float getRemotePeerLevelGain(int index) const;

    void setRemotePeerChannelPan(int index, int chan, float pan);
    float getRemotePeerChannelPan(int index, int chan) const;

    int getRemotePeerChannelCount(int index) const;
    
    void setRemotePeerBufferTime(int index, float bufferMs);
    float getRemotePeerBufferTime(int index) const;

    void setRemotePeerAutoresizeBufferMode(int index, AutoNetBufferMode flag);
    AutoNetBufferMode getRemotePeerAutoresizeBufferMode(int index) const;

    void setRemotePeerSendActive(int index, bool active);
    bool getRemotePeerSendActive(int index) const;

    void setRemotePeerRecvActive(int index, bool active);
    bool getRemotePeerRecvActive(int index) const;

    void setRemotePeerSendAllow(int index, bool allow);
    bool getRemotePeerSendAllow(int index) const;

    void setRemotePeerRecvAllow(int index, bool allow);
    bool getRemotePeerRecvAllow(int index) const;

    
    int64_t getRemotePeerPacketsReceived(int index) const;
    int64_t getRemotePeerPacketsSent(int index) const;

    int64_t getRemotePeerBytesReceived(int index) const;
    int64_t getRemotePeerBytesSent(int index) const;

    int64_t getRemotePeerPacketsDropped(int index) const;
    int64_t getRemotePeerPacketsResent(int index) const;


    
    float getRemotePeerPingMs(int index) const;
    float getRemotePeerTotalLatencyMs(int index) const;


    int getNumberAudioCodecFormats() const {  return mAudioFormats.size(); }

    void setDefaultAudioCodecFormat(int formatIndex) { if (formatIndex < mAudioFormats.size()) mDefaultAudioFormatIndex = formatIndex; }
    int getDefaultAudioCodecFormat() const { return mDefaultAudioFormatIndex; }

    String getAudioCodeFormatName(int formatIndex) const;

    
    void setRemotePeerAudioCodecFormat(int index, int formatIndex);
    int getRemotePeerAudioCodecFormat(int index) const;

    int getRemotePeerSendPacketsize(int index) const;
    void setRemotePeerSendPacketsize(int index, int psize);

    // danger
    foleys::LevelMeterSource * getRemotePeerRecvMeterSource(int index);
    foleys::LevelMeterSource * getRemotePeerSendMeterSource(int index);
    
    void setRemotePeerConnected(int index, bool active);
    bool getRemotePeerConnected(int index) const;
    
    bool getRemotePeerAddressInfo(int index, String & rethost, int & retport) const;

    bool getPatchMatrixValue(int srcindex, int destindex) const;
    void setPatchMatrixValue(int srcindex, int destindex, bool value);
    
    foleys::LevelMeterSource & getInputMeterSource() { return inputMeterSource; }
    foleys::LevelMeterSource & getOutputMeterSource() { return outputMeterSource; }
    
    bool isAnythingRoutedToPeer(int index) const;
    
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FluxAoOAudioProcessor)
    
    void initializeAoo();
    void cleanupAoo();
    
    void doReceiveData();
    void doSendData();
    void handleEvents();
    
    void setupSourceFormat(RemotePeer * peer, aoo::isource * source);

    
    RemotePeer *  findRemotePeer(EndpointState * endpoint, int32_t ourId);
    RemotePeer *  findRemotePeerByRemoteSourceId(EndpointState * endpoint, int32_t sourceId);
    RemotePeer *  findRemotePeerByRemoteSinkId(EndpointState * endpoint, int32_t sinkId);
    RemotePeer *  doAddRemotePeerIfNecessary(EndpointState * endpoint, int32_t ourId=AOO_ID_NONE);
    bool doRemoveRemotePeerIfNecessary(EndpointState * endpoint, int32_t ourId);
    
    void adjustRemoteSendMatrix(int index, bool removed);

    
    AudioSampleBuffer tempBuffer;
    AudioSampleBuffer workBuffer;

    Atomic<float>   mInGain    {   1.0 };
    Atomic<float>   mInMonPan1    {   0.0 };
    Atomic<float>   mInMonPan2    {   0.0 };
    Atomic<float>   mDry    {   1.0 };
    Atomic<float>   mWet    {   1.0 };
    Atomic<double>   mBufferTime     { 0.1 };
    Atomic<double>   mMaxBufferTime     { 1.0 };

    float mLastInputGain    = 0.0f;
    float mLastDry    = 0.0f;
    float mLastWet    = 0.0f;
    float mLastInMonPan1 = 0.0f;
    float mLastInMonPan2 = 0.0f;
    
    bool hasInitializedInMonPanners = false;
    
    int maxBlockSize = 4096;

    int mSinkBlocksize = 1024;
    int mSourceBlocksize = 1024;
    
    int currSamplesPerBlock = 256;
    int lastSamplesPerBlock = 256;
    
    float meterRmsWindow = 0.0f;
    
    int lastInputChannels = 0;
    int lastOutputChannels = 0;
    
    AudioProcessorValueTreeState mState;
    UndoManager                  mUndoManager;

    // top level meter sources
    foleys::LevelMeterSource inputMeterSource;
    foleys::LevelMeterSource outputMeterSource;
    
    // AOO stuff
    aoo::isource::pointer mAooDummySource;

    // we will add sinks for any peer we invite, as part of a RemoteSource
    
    
    std::unique_ptr<DatagramSocket> mUdpSocket;
    int mUdpLocalPort;
    IPAddress mLocalIPAddress;
    
    class SendThread;
    class RecvThread;
    class EventThread;
    
    CriticalSection  mEndpointsLock;
    ReadWriteLock    mCoreLock;
    
    OwnedArray<EndpointState> mEndpoints;
    
    OwnedArray<RemotePeer> mRemotePeers;


    CriticalSection  mRemotesLock;

    enum AudioCodecFormatCodec { CodecPCM = 0, CodecOpus };

    
    struct AudioCodecFormatInfo {
        AudioCodecFormatInfo() {}
        AudioCodecFormatInfo(String name_, int bitdepth_) : name(name_), codec(CodecPCM), bitdepth(bitdepth_) {}
        AudioCodecFormatInfo(String name_, int bitrate_, int complexity_, int signaltype, int minblocksize=120) : name(name_), codec(CodecOpus), bitrate(bitrate_), complexity(complexity_), signal_type(signaltype), min_preferred_blocksize(minblocksize) {}
        String name;
        AudioCodecFormatCodec codec;
        // PCM options
        int bitdepth = 2; // bytes
        // opus options
        int bitrate = 0;
        int complexity = 0;
        int signal_type = 0;
        int min_preferred_blocksize = 120;
    };
    
    Array<AudioCodecFormatInfo> mAudioFormats;
    int mDefaultAudioFormatIndex;
    
    void initFormats();
    
    bool mRemoteSendMatrix[MAX_PEERS][MAX_PEERS];
    
    
    void notifySendThread() {
        mHasStuffToSend = true;
        mSendWaitable.signal();
    }
    
    WaitableEvent  mSendWaitable;
    volatile bool mHasStuffToSend = false;
    
    std::unique_ptr<SendThread> mSendThread;
    std::unique_ptr<RecvThread> mRecvThread;
    std::unique_ptr<EventThread> mEventThread;
    
};
