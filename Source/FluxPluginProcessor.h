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
    static String paramDry;
    static String paramWet;
    static String paramBufferTime;
    static String paramStreamingEnabled;

    struct EndpointState;
    struct RemoteSink;
    struct RemoteSource;
    struct RemotePeer;
    
    int32_t handleSourceEvents(const aoo_event ** events, int32_t n, int32_t sourceId);
    int32_t handleSinkEvents(const aoo_event ** events, int32_t n, int32_t sinkId);

    
    EndpointState * findOrAddEndpoint(const String & host, int port);

    int getUdpLocalPort() const { return mUdpLocalPort; }
    IPAddress getLocalIPAddress() const { return mLocalIPAddress; }
    
    bool addRemoteSink(const String & host, int port, int32_t sinkId);
    bool removeRemoteSink(const String & host, int port, int32_t sinkId);

    bool inviteRemoteSource(const String & host, int port, int32_t sourceId, bool reciprocate=false);
    bool unInviteRemoteSource(const String & host, int port, int32_t sourceId);
    bool unInviteRemoteSource(int index);
    int getNumberRemoteSources() const;
    
    void setRemoteSourceLevelDb(int index, float leveldb);
    float getRemoteSourceLevelDb(int index) const;

    void setRemoteSourceBufferTime(int index, float bufferMs);
    float getRemoteSourceBufferTime(int index) const;

    void setRemoteSourceActive(int index, bool active);
    bool getRemoteSourceActive(int index) const;

    
    int getNumberRemoteSinks() const;

    void setSendToRemoteSinkActive(int index, bool active);
    bool getSendToRemoteSinkActive(int index) const;

    
    bool getRemoteSourceAddressInfo(int index, String & rethost, int & retport) const;

    
    int connectRemotePeer(const String & host, int port, bool reciprocate=true);
    bool disconnectRemotePeer(const String & host, int port, int32_t sourceId);
    bool disconnectRemotePeer(int index);
    bool removeRemotePeer(int index);

    int getNumberRemotePeers() const;

    void setRemotePeerLevelDb(int index, float leveldb);
    float getRemotePeerLevelDb(int index) const;

    void setRemotePeerBufferTime(int index, float bufferMs);
    float getRemotePeerBufferTime(int index) const;

    void setRemotePeerSendActive(int index, bool active);
    bool getRemotePeerSendActive(int index) const;

    void setRemotePeerRecvActive(int index, bool active);
    bool getRemotePeerRecvActive(int index) const;

    int64_t getRemotePeerPacketsReceived(int index) const;

    
    void setRemotePeerConnected(int index, bool active);
    bool getRemotePeerConnected(int index) const;
    
    bool getRemotePeerAddressInfo(int index, String & rethost, int & retport) const;

    bool getPatchMatrixValue(int srcindex, int destindex) const;
    void setPatchMatrixValue(int srcindex, int destindex, bool value);
    
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FluxAoOAudioProcessor)
    
    void initializeAoo();
    void cleanupAoo();
    
    void doReceiveData();
    void doSendData();
    void handleEvents();
    
    void setupSourceFormat(aoo::isource * source);
    
    RemoteSink *  findRemoteSink(EndpointState * endpoint, int32_t sinkId);
    RemoteSink *  doAddRemoteSinkIfNecessary(EndpointState * endpoint, int32_t sinkId);
    bool doRemoveRemoteSinkIfNecessary(EndpointState * endpoint, int32_t sinkId);

    RemoteSource *  findRemoteSource(EndpointState * endpoint, int32_t sourceId);
    RemoteSource *  doAddRemoteSourceIfNecessary(EndpointState * endpoint, int32_t sourceId);
    bool doRemoveRemoteSourceIfNecessary(EndpointState * endpoint, int32_t sourceId);
    
    RemotePeer *  findRemotePeer(EndpointState * endpoint, int32_t ourId);
    RemotePeer *  findRemotePeerByRemoteSourceId(EndpointState * endpoint, int32_t sourceId);
    RemotePeer *  findRemotePeerByRemoteSinkId(EndpointState * endpoint, int32_t sinkId);
    RemotePeer *  doAddRemotePeerIfNecessary(EndpointState * endpoint, int32_t ourId=AOO_ID_NONE);
    bool doRemoveRemotePeerIfNecessary(EndpointState * endpoint, int32_t ourId);
    
    void adjustRemoteSendMatrix(int index, bool removed);

    
    AudioSampleBuffer tempBuffer;
    AudioSampleBuffer workBuffer;

    Atomic<float>   mInGain    {   1.0 };
    Atomic<float>   mDry    {   1.0 };
    Atomic<float>   mWet    {   1.0 };
    Atomic<double>   mBufferTime     { 0.1 };
    Atomic<double>   mMaxBufferTime     { 1.0 };
    Atomic<bool>   mStreamingEnabled {  false };

    float mLastInputGain    = 0.0f;
    float mLastDry    = 0.0f;
    float mLastWet    = 0.0f;

    int maxBlockSize = 4096;

    int mSinkBlocksize = 1024;
    int mSourceBlocksize = 1024;
    
    int currSamplesPerBlock = 256;
    
    AudioProcessorValueTreeState mState;
    UndoManager                  mUndoManager;

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
    
    OwnedArray<RemoteSink> mRemoteSinks;
    OwnedArray<RemoteSource> mRemoteSources;
    OwnedArray<RemotePeer> mRemotePeers;


    CriticalSection  mRemotesLock;
    
    
    bool mRemoteSendMatrix[MAX_PEERS][MAX_PEERS];
    
    
    WaitableEvent  mSendWaitable;
    
    std::unique_ptr<SendThread> mSendThread;
    std::unique_ptr<RecvThread> mRecvThread;
    std::unique_ptr<EventThread> mEventThread;
    
};
