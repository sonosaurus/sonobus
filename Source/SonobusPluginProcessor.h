

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "aoo/aoo.hpp"
#include "aoo/aoo_net.hpp"

#define MAX_PEERS 32


struct AooServerConnectionInfo
{
    AooServerConnectionInfo() {}
    
    ValueTree getValueTree() const;
    void setFromValueTree(const ValueTree & val);
    
    String userName;
    String userPassword;
    String groupName;
    String groupPassword;
    String serverHost;
    int    serverPort;
    
    int64 timestamp; // milliseconds since 1970
};

inline bool operator==(const AooServerConnectionInfo& lhs, const AooServerConnectionInfo& rhs) {
    // compare all except timestamp
     return (lhs.userName == rhs.userName
             && lhs.userPassword == rhs.userPassword
             && lhs.groupName == rhs.groupName
             && lhs.groupPassword == rhs.groupPassword
             && lhs.serverHost == rhs.serverHost
             && lhs.serverPort == rhs.serverPort
             );
}

inline bool operator!=(const AooServerConnectionInfo& lhs, const AooServerConnectionInfo& rhs){ return !(lhs == rhs); }

inline bool operator<(const AooServerConnectionInfo& lhs, const AooServerConnectionInfo& rhs)
{
    // default sorting most recent first
    return (lhs.timestamp > rhs.timestamp); 
}


//==============================================================================
/**
*/
class SonobusAudioProcessor  : public AudioProcessor, public AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    SonobusAudioProcessor();
    ~SonobusAudioProcessor();

    enum AutoNetBufferMode {
        AutoNetBufferModeOff = 0,
        AutoNetBufferModeAutoIncreaseOnly,
        AutoNetBufferModeAutoFull        
    };
    
    enum AudioCodecFormatCodec { CodecPCM = 0, CodecOpus };

       
    struct AudioCodecFormatInfo {
        AudioCodecFormatInfo() {}
        AudioCodecFormatInfo(int bitdepth_) : codec(CodecPCM), bitdepth(bitdepth_) { computeName(); }
        AudioCodecFormatInfo(int bitrate_, int complexity_, int signaltype, int minblocksize=120) :  codec(CodecOpus), bitrate(bitrate_), complexity(complexity_), signal_type(signaltype), min_preferred_blocksize(minblocksize) { computeName(); }
        void computeName();
        
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
    static String paramDefaultAutoNetbuf;
    static String paramDefaultNetbufMs;
    static String paramDefaultSendQual;
    static String paramMasterSendMute;

    struct EndpointState;
    struct RemoteSink;
    struct RemoteSource;
    struct RemotePeer;
    
    int32_t handleSourceEvents(const aoo_event ** events, int32_t n, int32_t sourceId);
    int32_t handleSinkEvents(const aoo_event ** events, int32_t n, int32_t sinkId);
    int32_t handleServerEvents(const aoo_event ** events, int32_t n);
    int32_t handleClientEvents(const aoo_event ** events, int32_t n);

    // server stuff
    void startAooServer();
    void stopAooServer();
  
    // client stuff
    bool connectToServer(const String & host, int port, const String & username, const String & passwd="");
    bool isConnectedToServer() const;
    bool disconnectFromServer();

    void setAutoconnectToGroupPeers(bool flag);
    bool getAutoconnectToGroupPeers() const { return mAutoconnectGroupPeers; }

    bool joinServerGroup(const String & group, const String & groupsecret = "");
    bool leaveServerGroup(const String & group);
    String getCurrentJoinedGroup() const ;
    
    void addRecentServerConnectionInfo(const AooServerConnectionInfo & cinfo);
    int getRecentServerConnectionInfos(Array<AooServerConnectionInfo> & retarray);
    void clearRecentServerConnectionInfos();
    
    // peer stuff
    
    EndpointState * findOrAddEndpoint(const String & host, int port);
    EndpointState * findOrAddRawEndpoint(void * rawaddr);

    int getUdpLocalPort() const { return mUdpLocalPort; }
    IPAddress getLocalIPAddress() const { return mLocalIPAddress; }
    
  
    int connectRemotePeer(const String & host, int port, const String & username = "", const String & groupname = "",  bool reciprocate=true);
    bool disconnectRemotePeer(const String & host, int port, int32_t sourceId);
    bool disconnectRemotePeer(int index);
    bool removeRemotePeer(int index);

    bool removeAllRemotePeers();
    
    int getNumberRemotePeers() const;

    void setRemotePeerLevelGain(int index, float levelgain);
    float getRemotePeerLevelGain(int index) const;

    void setRemotePeerChannelPan(int index, int chan, float pan);
    float getRemotePeerChannelPan(int index, int chan) const;

    void setRemotePeerUserName(int index, const String & name);
    String getRemotePeerUserName(int index) const;

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
    void    resetRemotePeerPacketStats(int index);
    

    
    float getRemotePeerPingMs(int index) const;
    float getRemotePeerTotalLatencyMs(int index, bool & isreal, bool & estimated) const;

    bool startRemotePeerLatencyTest(int index, float durationsec = 1.0);
    bool stopRemotePeerLatencyTest(int index);
    bool isRemotePeerLatencyTestActive(int index);
    
    
    int getNumberAudioCodecFormats() const {  return mAudioFormats.size(); }

    void setDefaultAudioCodecFormat(int formatIndex);
    int getDefaultAudioCodecFormat() const { return mDefaultAudioFormatIndex; }

    String getAudioCodeFormatName(int formatIndex) const;

    void setDefaultAutoresizeBufferMode(AutoNetBufferMode flag);
    AutoNetBufferMode getDefaultAutoresizeBufferMode() const { return (AutoNetBufferMode) defaultAutoNetbufMode; }
    
    
    void setRemotePeerAudioCodecFormat(int index, int formatIndex);
    int getRemotePeerAudioCodecFormat(int index) const;

    bool getRemotePeerReceiveAudioCodecFormat(int index, AudioCodecFormatInfo & retinfo) const;
    
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
    
    class ClientListener {
    public:
        virtual ~ClientListener() {}
        virtual void aooClientConnected(SonobusAudioProcessor *comp, bool success, const String & errmesg="") {}
        virtual void aooClientDisconnected(SonobusAudioProcessor *comp, bool success, const String & errmesg="") {}
        virtual void aooClientLoginResult(SonobusAudioProcessor *comp, bool success, const String & errmesg="") {}
        virtual void aooClientGroupJoined(SonobusAudioProcessor *comp, bool success, const String & group,  const String & errmesg="") {}
        virtual void aooClientGroupLeft(SonobusAudioProcessor *comp, bool success, const String & group, const String & errmesg="") {}
        virtual void aooClientPeerJoined(SonobusAudioProcessor *comp, const String & group, const String & user) {}
        virtual void aooClientPeerLeft(SonobusAudioProcessor *comp, const String & group, const String & user) {}
        virtual void aooClientError(SonobusAudioProcessor *comp, const String & errmesg) {}
        virtual void aooClientPeerChangedState(SonobusAudioProcessor *comp, const String & mesg) {}
    };
    
    void addClientListener(ClientListener * l) {
        clientListeners.add(l);
    }
    void removeClientListener(ClientListener * l) {
        clientListeners.remove(l);
    }
    
    // recording stuff
    bool startRecordingToFile(const File & file);
    bool stopRecordingToFile();
    bool isRecordingToFile();
    
    
   
    
    
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SonobusAudioProcessor)
    
    
    
    void initializeAoo();
    void cleanupAoo();
    
    void doReceiveData();
    void doSendData();
    void handleEvents();
    
    void setupSourceFormat(RemotePeer * peer, aoo::isource * source, bool latencymode=false);

    
    RemotePeer *  findRemotePeer(EndpointState * endpoint, int32_t ourId);
    RemotePeer *  findRemotePeerByEchoId(EndpointState * endpoint, int32_t echoId);
    RemotePeer *  findRemotePeerByLatencyId(EndpointState * endpoint, int32_t latId);
    RemotePeer *  findRemotePeerByRemoteSourceId(EndpointState * endpoint, int32_t sourceId);
    RemotePeer *  findRemotePeerByRemoteSinkId(EndpointState * endpoint, int32_t sinkId);
    RemotePeer *  doAddRemotePeerIfNecessary(EndpointState * endpoint, int32_t ourId=AOO_ID_NONE);
    bool doRemoveRemotePeerIfNecessary(EndpointState * endpoint, int32_t ourId);
    
    bool removeAllRemotePeersWithEndpoint(EndpointState * endpoint);

    void adjustRemoteSendMatrix(int index, bool removed);

    
    int connectRemotePeerRaw(void * sockaddr, const String & username = "", const String & groupname = "", bool reciprocate=true);

    int findFormatIndex(AudioCodecFormatCodec codec, int bitrate, int bitdepth);

    
    ListenerList<ClientListener> clientListeners;

    
    AudioSampleBuffer tempBuffer;
    AudioSampleBuffer workBuffer;
    AudioSampleBuffer inputBuffer;

    Atomic<float>   mInGain    { 1.0 };
    Atomic<float>   mInMonPan1    {   0.0 };
    Atomic<float>   mInMonPan2    {   0.0 };
    Atomic<float>   mDry    {
#if JUCE_IOS
        0.0 
#else
        1.0
#endif        
    };
    Atomic<float>   mWet    {   1.0 };
    Atomic<double>   mBufferTime     { 0.01 };
    Atomic<double>   mMaxBufferTime     { 1.0 };
    Atomic<bool>   mMasterSendMute    {   false };

    float mLastInputGain    = 0.0f;
    float mLastDry    = 0.0f;
    float mLastWet    = 0.0f;
    float mLastInMonPan1 = 0.0f;
    float mLastInMonPan2 = 0.0f;
    
    int defaultAutoNetbufMode = AutoNetBufferModeAutoFull;
    
    RangedAudioParameter * mDefaultAutoNetbufModeParam;

    
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

    aoo::net::iserver::pointer mAooServer;
    aoo::net::iclient::pointer mAooClient;

    std::unique_ptr<EndpointState> mServerEndpoint;
    
    bool mAutoconnectGroupPeers = true;
    bool mIsConnectedToServer = false;
    String mCurrentJoinedGroup;
    
    // we will add sinks for any peer we invite, as part of a RemoteSource
    
    
    std::unique_ptr<DatagramSocket> mUdpSocket;
    int mUdpLocalPort;
    IPAddress mLocalIPAddress;
    
    class SendThread;
    class RecvThread;
    class EventThread;
    class ServerThread;
    class ClientThread;
    
    CriticalSection  mEndpointsLock;
    ReadWriteLock    mCoreLock;
    CriticalSection  mClientLock;
    
    OwnedArray<EndpointState> mEndpoints;
    
    OwnedArray<RemotePeer> mRemotePeers;


    Array<AooServerConnectionInfo> mRecentConnectionInfos;
    CriticalSection  mRecentsLock;
    
    CriticalSection  mRemotesLock;

    
    
    Array<AudioCodecFormatInfo> mAudioFormats;
    int mDefaultAudioFormatIndex = 5;
    
    RangedAudioParameter * mDefaultAudioFormatParam;

    
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
    std::unique_ptr<ServerThread> mServerThread;
    std::unique_ptr<ClientThread> mClientThread;
    
    // recording stuff
    volatile bool writingPossible = false;
    std::unique_ptr<TimeSliceThread> recordingThread;
    std::unique_ptr<AudioFormatWriter::ThreadedWriter> threadedWriter; // the FIFO used to buffer the incoming data
    CriticalSection writerLock;
    std::atomic<AudioFormatWriter::ThreadedWriter*> activeWriter { nullptr };
    
};
