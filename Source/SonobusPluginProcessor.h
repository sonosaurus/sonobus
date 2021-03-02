// SPDX-License-Identifier: GPLv3-or-later
// Copyright (C) 2020 Jesse Chappell



#pragma once

#include "JuceHeader.h"

#include "aoo/aoo.hpp"
#include "aoo/aoo_net.hpp"

#include <map>
#include <string>

#include "MVerb.h"

#include "EffectParams.h"
#include "ChannelGroup.h"

#include "zitaRev.h"

typedef MVerb<float> MVerbFloat;

namespace SonoAudio {
class Metronome;
}


#define MAX_PEERS 32
#define MAX_CHANGROUPS 64
#define DEFAULT_SERVER_PORT 10998


struct AooServerConnectionInfo
{
    AooServerConnectionInfo() {}
    
    ValueTree getValueTree() const;
    void setFromValueTree(const ValueTree & val);
    
    String userName;
    String userPassword;
    String groupName;
    String groupPassword;
    bool   groupIsPublic = false;
    String serverHost;
    int    serverPort;
    
    int64 timestamp; // milliseconds since 1970
};

struct AooPublicGroupInfo
{
    AooPublicGroupInfo() {}

    String groupName;
    int    activeCount = 0;

    int64 timestamp = 0; // milliseconds since 1970
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
class SonobusAudioProcessor  : public AudioProcessor, public AudioProcessorValueTreeState::Listener, public ChangeListener
{
public:
    //==============================================================================
    SonobusAudioProcessor();
    ~SonobusAudioProcessor();

    enum AutoNetBufferMode {
        AutoNetBufferModeOff = 0,
        AutoNetBufferModeAutoIncreaseOnly,
        AutoNetBufferModeAutoFull,        
        AutoNetBufferModeInitAuto
    };
    
    enum AudioCodecFormatCodec { CodecPCM = 0, CodecOpus };

    enum ReverbModel {
        ReverbModelFreeverb = 0,
        ReverbModelMVerb,
        ReverbModelZita
    };
    
    // treated as bitmask options
    enum RecordFileOptions {
        RecordDefaultOptions = 0,
        RecordMix = 1,
        RecordSelf = 2,
        RecordMixMinusSelf = 4,
        RecordIndividualUsers = 8
    };
    
    enum RecordFileFormat {
        FileFormatDefault = 0,
        FileFormatAuto,
        FileFormatFLAC,
        FileFormatWAV,
        FileFormatOGG
    };
    
    struct AudioCodecFormatInfo {
        AudioCodecFormatInfo() {}
        AudioCodecFormatInfo(int bitdepth_) : codec(CodecPCM), bitdepth(bitdepth_), min_preferred_blocksize(16)  { computeName(); }
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
    
    int32 getCurrSamplesPerBlock() const { return currSamplesPerBlock; }
    
    void changeListenerCallback (ChangeBroadcaster* source) override;

    static BusesProperties getDefaultLayout();

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

    //bool canAddBus    (bool isInput) const override   { return (! isInput && getBusCount (false) < 10); }
    //bool canRemoveBus (bool isInput) const override   { return (! isInput && getBusCount (false) > 1); }
    
    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void parameterChanged (const String &parameterID, float newValue) override;

    AudioProcessorValueTreeState& getValueTreeState();

    static String paramInGain;
    static String paramInMonitorMonoPan;
    static String paramInMonitorPan1;
    static String paramInMonitorPan2;
    static String paramDry;
    static String paramWet;
    static String paramSendChannels;
    static String paramBufferTime;
    static String paramDefaultAutoNetbuf;
    static String paramDefaultNetbufMs;
    static String paramDefaultSendQual;
    static String paramMainSendMute;
    static String paramMainRecvMute;
    static String paramMetEnabled;
    static String paramMetGain;
    static String paramMetTempo;
    static String paramSendMetAudio;
    static String paramSendFileAudio;
    static String paramHearLatencyTest;
    static String paramMetIsRecorded;
    static String paramMainReverbEnabled;
    static String paramMainReverbLevel;
    static String paramMainReverbSize;
    static String paramMainReverbDamping;
    static String paramMainReverbPreDelay;
    static String paramMainReverbModel;
    static String paramDynamicResampling;
    static String paramMainInMute;
    static String paramMainMonitorSolo;
    static String paramAutoReconnectLast;
    static String paramDefaultPeerLevel;

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
    
    // if value is 0, the system will choose any available UDP port (default)
    void setUseSpecificUdpPort(int port);
    int getUseSpecificUdpPort() const { return mUseSpecificUdpPort; }
    
    bool connectToServer(const String & host, int port, const String & username, const String & passwd="");
    bool isConnectedToServer() const;
    bool disconnectFromServer();
    double getElapsedConnectedTime() const { return mSessionConnectionStamp > 0 ? (Time::getMillisecondCounterHiRes() - mSessionConnectionStamp) * 1e-3 : 0.0; }

    void setAutoconnectToGroupPeers(bool flag);
    bool getAutoconnectToGroupPeers() const { return mAutoconnectGroupPeers; }

    bool joinServerGroup(const String & group, const String & groupsecret = "", bool isPublic=false);
    bool leaveServerGroup(const String & group);
    String getCurrentJoinedGroup() const ;
    bool setWatchPublicGroups(bool flag);
    bool getWatchPublicGroups() const { return mWatchPublicGroups; }

    int getPublicGroupInfos(Array<AooPublicGroupInfo> & retarray);

    void addRecentServerConnectionInfo(const AooServerConnectionInfo & cinfo);
    void removeRecentServerConnectionInfo(int index);
    int getRecentServerConnectionInfos(Array<AooServerConnectionInfo> & retarray);
    void clearRecentServerConnectionInfos();
    
    // peer stuff
    
    EndpointState * findOrAddEndpoint(const String & host, int port);
    EndpointState * findOrAddRawEndpoint(void * rawaddr);

    int getUdpLocalPort() const { return mUdpLocalPort; }
    IPAddress getLocalIPAddress() const { return mLocalIPAddress; }
    

    int getSendChannels() const { return mSendChannels.get(); }

    int connectRemotePeer(const String & host, int port, const String & username = "", const String & groupname = "",  bool reciprocate=true);
    bool disconnectRemotePeer(const String & host, int port, int32_t sourceId);
    bool disconnectRemotePeer(int index);
    bool removeRemotePeer(int index);

    bool removeAllRemotePeers();
    
    int getNumberRemotePeers() const;

    void setRemotePeerLevelGain(int index, float levelgain);
    float getRemotePeerLevelGain(int index) const;

    void setRemotePeerChannelGain(int index, int changroup, float levelgain);
    float getRemotePeerChannelGain(int index, int changroup) const;

    void setRemotePeerChannelPan(int index, int changroup, int chan, float pan);
    float getRemotePeerChannelPan(int index, int changroup, int chan) const;

    void setRemotePeerChannelMuted(int index, int changroup, bool muted);
    bool getRemotePeerChannelMuted(int index, int changroup) const;

    void setRemotePeerChannelSoloed(int index, int changroup, bool soloed);
    bool getRemotePeerChannelSoloed(int index, int changroup) const;


    void setRemotePeerUserName(int index, const String & name);
    String getRemotePeerUserName(int index) const;


    int getRemotePeerChannelGroupCount(int index) const;
    void setRemotePeerChannelGroupCount(int index, int count);

    int getRemotePeerRecvChannelCount(int index) const;

    //int getRemotePeerChannelGroupChannelCount(int index, int changroup) const;
    //void setRemotePeerChannelGroupChannelCount(int index, int changroup, int count);

    void setRemotePeerChannelGroupStartAndCount(int index, int changroup, int start, int count);
    bool getRemotePeerChannelGroupStartAndCount(int index, int changroup, int & retstart, int & retcount);

    void setRemotePeerChannelGroupDestStartAndCount(int index, int changroup, int start, int count);
    bool getRemotePeerChannelGroupDestStartAndCount(int index, int changroup, int & retstart, int & retcount);

    void setRemotePeerChannelGroupSendMainMix(int index, int changroup, bool mainmix);
    bool getRemotePeerChannelGroupSendMainMix(int index, int changroup);

    bool insertRemotePeerChannelGroup(int index, int atgroup, int chstart, int chcount);
    bool removeRemotePeerChannelGroup(int index, int atgroup);

    bool copyRemotePeerChannelGroup(int index, int fromgroup, int togroup);

    String getRemotePeerChannelGroupName(int index, int changroup) const;
    void setRemotePeerChannelGroupName(int index, int changroup, const String & name);


    int getRemotePeerNominalSendChannelCount(int index) const;
    void setRemotePeerNominalSendChannelCount(int index, int numchans);

    int getRemotePeerOverrideSendChannelCount(int index) const;
    void setRemotePeerOverrideSendChannelCount(int index, int numchans);

    int getRemotePeerActualSendChannelCount(int index) const;

    bool getRemotePeerViewExpanded(int index) const;
    void setRemotePeerViewExpanded(int index, bool expanded);


    void setRemotePeerBufferTime(int index, float bufferMs);
    float getRemotePeerBufferTime(int index) const;

    void setRemotePeerAutoresizeBufferMode(int index, AutoNetBufferMode flag);
    AutoNetBufferMode getRemotePeerAutoresizeBufferMode(int index, bool & initCompleted) const;

    bool getRemotePeerReceiveBufferFillRatio(int index, float & retratio, float & retstddev) const;

    
    void setRemotePeerSendActive(int index, bool active);
    bool getRemotePeerSendActive(int index) const;

    void setRemotePeerRecvActive(int index, bool active);
    bool getRemotePeerRecvActive(int index) const;

    void setRemotePeerSendAllow(int index, bool allow, bool cached=false);
    bool getRemotePeerSendAllow(int index, bool cached=false) const;

    void setRemotePeerRecvAllow(int index, bool allow, bool cached=false);
    bool getRemotePeerRecvAllow(int index, bool cached=false) const;

    void setRemotePeerSoloed(int index, bool soloed);
    bool getRemotePeerSoloed(int index) const;

    
    int64_t getRemotePeerPacketsReceived(int index) const;
    int64_t getRemotePeerPacketsSent(int index) const;

    int64_t getRemotePeerBytesReceived(int index) const;
    int64_t getRemotePeerBytesSent(int index) const;

    int64_t getRemotePeerPacketsDropped(int index) const;
    int64_t getRemotePeerPacketsResent(int index) const;
    void    resetRemotePeerPacketStats(int index);

    bool getRemotePeerSafetyMuted(int index) const;


    struct LatencyInfo
    {
        float pingMs = 0.0f;
        float totalRoundtripMs = 0.0f;
        float outgoingMs = 0.0f;
        float incomingMs = 0.0f;
        float jitterMs = 0.0f;
        bool isreal = false;
        bool estimated = false;
    };
    
    bool getRemotePeerLatencyInfo(int index, LatencyInfo & retinfo) const;

    bool startRemotePeerLatencyTest(int index, float durationsec = 1.0);
    bool stopRemotePeerLatencyTest(int index);
    bool isRemotePeerLatencyTestActive(int index);
    

    
    void setRemotePeerCompressorParams(int index, int changroup, SonoAudio::CompressorParams & params);
    bool getRemotePeerCompressorParams(int index, int changroup, SonoAudio::CompressorParams & retparams);

    void setRemotePeerExpanderParams(int index, int changroup, SonoAudio::CompressorParams & params);
    bool getRemotePeerExpanderParams(int index, int changroup, SonoAudio::CompressorParams & retparams);

    void setRemotePeerEqParams(int index, int changroup, SonoAudio::ParametricEqParams & params);
    bool getRemotePeerEqParams(int index, int changroup, SonoAudio::ParametricEqParams & retparams);

    bool getRemotePeerEffectsActive(int index, int changroup);



    void setInputCompressorParams(int changroup, SonoAudio::CompressorParams & params);
    bool getInputCompressorParams(int changroup, SonoAudio::CompressorParams & retparams);

    void setInputExpanderParams(int changroup, SonoAudio::CompressorParams & params);
    bool getInputExpanderParams(int changroup, SonoAudio::CompressorParams & retparams);

    void setInputLimiterParams(int changroup, SonoAudio::CompressorParams & params);
    bool getInputLimiterParams(int changroup, SonoAudio::CompressorParams & retparams);

    
    // input channel group stuff

    void setInputGroupCount(int count);
    int getInputGroupCount() const { return mInputChannelGroupCount; }

    void setInputGroupChannelStartAndCount(int changroup, int start, int count);
    bool getInputGroupChannelStartAndCount(int changroup, int & retstart, int & retcount);

    void setInputGroupChannelDestStartAndCount(int changroup, int start, int count);
    bool getInputGroupChannelDestStartAndCount(int changroup, int & retstart, int & retcount);


    
    // pushes existing groups around
    bool insertInputChannelGroup(int atgroup, int chstart, int chcount);
    bool removeInputChannelGroup(int atgroup);
    bool moveInputChannelGroupTo(int atgroup, int togroup);

    void setInputGroupName(int changroup, const String & name);
    String getInputGroupName(int changroup);

    void setInputChannelPan(int changroup, int chan, float pan);
    float getInputChannelPan(int changroup, int chan);

    void setInputGroupGain(int changroup, float gain);
    float getInputGroupGain(int changroup);

    void setInputMonitor(int changroup, float mgain);
    float getInputMonitor(int changroup);

    void setInputGroupMuted(int changroup, bool muted);
    bool getInputGroupMuted(int changroup);

    void setInputGroupSoloed(int changroup, bool muted);
    bool getInputGroupSoloed(int changroup);


    void setInputReverbSend(int changroup, float rgain);
    float getInputReverbSend(int changroup);


    void setInputEqParams(int changroup, SonoAudio::ParametricEqParams & params);
    bool getInputEqParams(int changroup, SonoAudio::ParametricEqParams & retparams);
    
    
    bool getInputEffectsActive(int changroup) const { return mInputChannelGroups[changroup].compressorParams.enabled || mInputChannelGroups[changroup].expanderParams.enabled || mInputChannelGroups[changroup].eqParams.enabled; }
    
    int getNumberAudioCodecFormats() const {  return mAudioFormats.size(); }

    void setDefaultAudioCodecFormat(int formatIndex);
    int getDefaultAudioCodecFormat() const { return mDefaultAudioFormatIndex; }

    void setChangingDefaultAudioCodecSetsExisting(bool flag) { mChangingDefaultAudioCodecChangesAll = flag; }
    bool getChangingDefaultAudioCodecSetsExisting() const { return mChangingDefaultAudioCodecChangesAll;}

    void setChangingDefaultRecvAudioCodecSetsExisting(bool flag) { mChangingDefaultRecvAudioCodecChangesAll = flag; }
    bool getChangingDefaultRecvAudioCodecSetsExisting() const { return mChangingDefaultRecvAudioCodecChangesAll;}

    
    String getAudioCodeFormatName(int formatIndex) const;
    bool getAudioCodeFormatInfo(int formatIndex, AudioCodecFormatInfo & retinfo) const;

    void setDefaultAutoresizeBufferMode(AutoNetBufferMode flag);
    AutoNetBufferMode getDefaultAutoresizeBufferMode() const { return (AutoNetBufferMode) defaultAutoNetbufMode; }
    
    bool getSendingFilePlaybackAudio() const { return mSendPlaybackAudio.get(); }

    bool getAutoReconnectToLast() const { return mAutoReconnectLast.get(); }

    // misc settings
    bool getSlidersSnapToMousePosition() const { return mSliderSnapToMouse; }
    void setSlidersSnapToMousePosition(bool flag) {  mSliderSnapToMouse = flag; }




    // sets and gets the format we send out
    void setRemotePeerAudioCodecFormat(int index, int formatIndex);
    int getRemotePeerAudioCodecFormat(int index) const;

    // gets and requests the format that we are receiving from the remote end
    bool getRemotePeerReceiveAudioCodecFormat(int index, AudioCodecFormatInfo & retinfo) const;
    bool setRequestRemotePeerSendAudioCodecFormat(int index, int formatIndex);
    int getRequestRemotePeerSendAudioCodecFormat(int index) const; // -1 is no preferences
    
    int getRemotePeerSendPacketsize(int index) const;
    void setRemotePeerSendPacketsize(int index, int psize);

    void updateAllRemotePeerUserFormats();


    // danger
    foleys::LevelMeterSource * getRemotePeerRecvMeterSource(int index);
    foleys::LevelMeterSource * getRemotePeerSendMeterSource(int index);
    
    void setRemotePeerConnected(int index, bool active);
    bool getRemotePeerConnected(int index) const;
    
    bool getRemotePeerAddressInfo(int index, String & rethost, int & retport) const;

    bool getPatchMatrixValue(int srcindex, int destindex) const;
    void setPatchMatrixValue(int srcindex, int destindex, bool value);

    int getActiveSendChannelCount() const { return mActiveSendChannels; }

    foleys::LevelMeterSource & getInputMeterSource() { return inputMeterSource; }
    foleys::LevelMeterSource & getPostInputMeterSource() { return postinputMeterSource; }
    foleys::LevelMeterSource & getSendMeterSource() { return sendMeterSource; }
    foleys::LevelMeterSource & getOutputMeterSource() { return outputMeterSource; }
    
    bool isAnythingRoutedToPeer(int index) const;
    
    bool isAnythingSoloed() const { return mAnythingSoloed.get(); }
    
    class ClientListener {
    public:
        virtual ~ClientListener() {}
        virtual void aooClientConnected(SonobusAudioProcessor *comp, bool success, const String & errmesg="") {}
        virtual void aooClientDisconnected(SonobusAudioProcessor *comp, bool success, const String & errmesg="") {}
        virtual void aooClientLoginResult(SonobusAudioProcessor *comp, bool success, const String & errmesg="") {}
        virtual void aooClientGroupJoined(SonobusAudioProcessor *comp, bool success, const String & group,  const String & errmesg="") {}
        virtual void aooClientGroupLeft(SonobusAudioProcessor *comp, bool success, const String & group, const String & errmesg="") {}
        virtual void aooClientPublicGroupModified(SonobusAudioProcessor *comp, const String & group, int count, const String & errmesg="") {}
        virtual void aooClientPublicGroupDeleted(SonobusAudioProcessor *comp, const String & group,  const String & errmesg="") {}
        virtual void aooClientPeerPendingJoin(SonobusAudioProcessor *comp, const String & group, const String & user) {}
        virtual void aooClientPeerJoined(SonobusAudioProcessor *comp, const String & group, const String & user) {}
        virtual void aooClientPeerJoinFailed(SonobusAudioProcessor *comp, const String & group, const String & user) {}
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
    
    // effects
    void setMainReverbEnabled(bool flag);
    bool getMainReverbEnabled() const { return mMainReverbEnabled.get(); }
    void  setMainReverbDryLevel(float level);
    float getMainReverbDryLevel() const;
    void  setMainReverbWetLevel(float level);
    float getMainReverbWetLevel() const { return mMainReverbLevel.get(); }
    void  setMainReverbSize(float value);
    float getMainReverbSize() const { return mMainReverbSize.get(); }
    void  setMainReverbPreDelay(float valuemsec);
    float getMainReverbPreDelay() const { return mMainReverbPreDelay.get(); }
    void setMainReverbModel(ReverbModel flag);
    ReverbModel getMainReverbModel() const { return (ReverbModel) mMainReverbModel.get(); }
    
    
    // recording stuff, if record options are RecordMixOnly, or RecordSelf  (only) the file refers to the name of the file
    //   otherwise it refers to a directory where the files will be recorded (and also the prefix for the recorded files)
    bool startRecordingToFile(File & file, uint32 recordOptions=RecordDefaultOptions, RecordFileFormat fileformat=FileFormatDefault);
    bool stopRecordingToFile();
    bool isRecordingToFile();
    double getElapsedRecordTime() const { return mElapsedRecordSamples / getSampleRate(); }
    String getLastErrorMessage() const { return mLastError; }

    void setDefaultRecordingDirectory(String recdir)  { mDefaultRecordDir = recdir; }
    String getDefaultRecordingDirectory() const { return mDefaultRecordDir; }

    uint32 getDefaultRecordingOptions() const { return mDefaultRecordingOptions; }
    void setDefaultRecordingOptions(uint32 opts) { mDefaultRecordingOptions = opts; }

    RecordFileFormat getDefaultRecordingFormat() const { return mDefaultRecordingFormat; }
    void setDefaultRecordingFormat(RecordFileFormat fmt) { mDefaultRecordingFormat = fmt; }

    int getDefaultRecordingBitsPerSample() const { return mDefaultRecordingBitsPerSample; }
    void setDefaultRecordingBitsPerSample(int fmt) { mDefaultRecordingBitsPerSample = fmt; }

    // playback stuff
    bool loadURLIntoTransport (const URL& audioURL);
    AudioTransportSource & getTransportSource() { return mTransportSource; }
    AudioFormatManager & getFormatManager() { return mFormatManager; }

    
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SonobusAudioProcessor)
    
    struct PeerStateCache
    {
        PeerStateCache();
        ValueTree getValueTree() const;
        void setFromValueTree(const ValueTree & val);

        String name;
        float netbuf = 10.0f;
        int   netbufauto;
        int   sendFormat = 4;

        SonoAudio::ChannelGroup channelGroups[MAX_CHANGROUPS];
        int numChanGroups = 1;
    };

    // key is peer name
    typedef std::map<String, PeerStateCache>  PeerStateCacheMap;



    
    void initializeAoo(int udpPort=0);
    void cleanupAoo();
    
    void doReceiveData();
    void doSendData();
    void handleEvents();
    
    void setupSourceFormat(RemotePeer * peer, aoo::isource * source, bool latencymode=false);
    bool formatInfoToAooFormat(const AudioCodecFormatInfo & info, int channels, aoo_format_storage & retformat);

    void setupSourceUserFormat(RemotePeer * peer, aoo::isource * source);

    
    RemotePeer *  findRemotePeer(EndpointState * endpoint, int32_t ourId);
    RemotePeer *  findRemotePeerByEchoId(EndpointState * endpoint, int32_t echoId);
    RemotePeer *  findRemotePeerByLatencyId(EndpointState * endpoint, int32_t latId);
    RemotePeer *  findRemotePeerByRemoteSourceId(EndpointState * endpoint, int32_t sourceId);
    RemotePeer *  findRemotePeerByRemoteSinkId(EndpointState * endpoint, int32_t sinkId);
    RemotePeer *  doAddRemotePeerIfNecessary(EndpointState * endpoint, int32_t ourId=AOO_ID_NONE, const String & username={}, const String & groupname={});
    bool doRemoveRemotePeerIfNecessary(EndpointState * endpoint, int32_t ourId);
    
    bool removeAllRemotePeersWithEndpoint(EndpointState * endpoint);

    void adjustRemoteSendMatrix(int index, bool removed);

    void commitCompressorParams(RemotePeer * peer, int changroup);
    void commitInputCompressorParams(int changroup);

    //void commitExpanderParams(RemotePeer * peer);
    void commitInputExpanderParams(int changroup);
    void commitInputLimiterParams(int changroup);

    void commitInputEqParams(int changroup);

    void updateDynamicResampling();

    void updateRemotePeerSendChannels(int index, RemotePeer * remote);

    void setupSourceFormatsForAll();
    ValueTree getSendUserFormatLayoutTree();

    void applyLayoutFormatToPeer(RemotePeer * remote, const ValueTree & valtree);


    int connectRemotePeerRaw(void * sockaddr, const String & username = "", const String & groupname = "", bool reciprocate=true);

    int findFormatIndex(AudioCodecFormatCodec codec, int bitrate, int bitdepth);

    void ensureBuffers(int samples);

    void commitCacheForPeer(RemotePeer * peer);
    bool findAndLoadCacheForPeer(RemotePeer * peer);
    
    void loadPeerCacheFromState();
    void storePeerCacheToState();
    
    ListenerList<ClientListener> clientListeners;

    
    AudioSampleBuffer tempBuffer;
    AudioSampleBuffer mixBuffer;
    AudioSampleBuffer workBuffer;
    AudioSampleBuffer inputBuffer;
    AudioSampleBuffer monitorBuffer;
    AudioSampleBuffer inputWorkBuffer;
    AudioSampleBuffer inputPostBuffer;
    AudioSampleBuffer fileBuffer;
    AudioSampleBuffer metBuffer;
    AudioSampleBuffer mainFxBuffer;
    AudioSampleBuffer silentBuffer; // only ever has one channel
    int mTempBufferSamples = 0;
    int mTempBufferChannels = 0;
    
    Atomic<float>   mInGain    { 1.0 };
    Atomic<float>   mInMonMonoPan    {   0.0 };
    Atomic<float>   mInMonPan1    {   -1.0 };
    Atomic<float>   mInMonPan2    {   1.0 };
    Atomic<float>   mDry    { 0.0 };
    Atomic<float>   mWet    {   1.0 };
    Atomic<double>   mBufferTime     { 0.001 };
    Atomic<double>   mMaxBufferTime     { 1.0 };
    Atomic<bool>   mMainSendMute    {   false };
    Atomic<bool>   mMainRecvMute    {   false };
    Atomic<bool>   mMainInMute    {   false };
    Atomic<bool>   mMainMonitorSolo    {   false };
    Atomic<bool>   mMetEnabled  { false };
    Atomic<bool>   mSendMet  { false };
    Atomic<int>   mSendChannels  { 1 }; // 0 is match inputs, 1 is 1, etc
    Atomic<float>   mMetGain    { 0.5f };
    Atomic<double>   mMetTempo    { 100.0f };
    Atomic<bool>   mSendPlaybackAudio  { false };
    Atomic<bool>   mHearLatencyTest  { false };
    Atomic<bool>   mMetIsRecorded  { true };
    Atomic<bool>   mMainReverbEnabled  { false };
    Atomic<float>   mMainReverbLevel  { 0.0626f };
    Atomic<float>   mMainReverbSize  { 0.15f };
    Atomic<float>   mMainReverbDamping  { 0.5f };
    Atomic<float>   mMainReverbPreDelay  { 20.0f }; // ms
    Atomic<int>   mMainReverbModel  { ReverbModelMVerb };
    Atomic<bool>   mDynamicResampling  { false };
    Atomic<bool>   mAutoReconnectLast  { false };
    Atomic<float>   mDefUserLevel    { 1.0f };

    float mLastInputGain    = 0.0f;
    float mLastDry    = 0.0f;
    float mLastWet    = 0.0f;
    float mLastInMonMonoPan = 0.0f;
    float mLastInMonPan1 = -1.0f;
    float mLastInMonPan2 = 1.0f;
    bool mLastMetEnabled = false;
    bool mLastMainReverbEnabled = false;
    bool mReverbParamsChanged = false;
    bool mLastHasMainFx = false;
    
    Atomic<bool>   mAnythingSoloed  { false };


    int defaultAutoNetbufMode = AutoNetBufferModeAutoFull;
    
    bool mChangingDefaultAudioCodecChangesAll = false;
    bool mChangingDefaultRecvAudioCodecChangesAll = false;

    RangedAudioParameter * mDefaultAutoNetbufModeParam;
    RangedAudioParameter * mTempoParameter;

    int mUseSpecificUdpPort = 0;
    
    bool hasInitializedInMonPanners = false;
    
    int maxBlockSize = 4096;

    int mSinkBlocksize = 1024;
    int mSourceBlocksize = 1024;
    
    int currSamplesPerBlock = 256;
    int lastSamplesPerBlock = 256;

    int blocksizeCounter = -1;
    Atomic<bool> mNeedsSampleSetup  { false };

    float meterRmsWindow = 0.0f;
    
    int lastInputChannels = 0;
    int lastOutputChannels = 0;

    int mActiveSendChannels = 0;

    PeerStateCacheMap mPeerStateCacheMap;
    
    // top level meter sources
    foleys::LevelMeterSource inputMeterSource;
    foleys::LevelMeterSource postinputMeterSource;
    foleys::LevelMeterSource sendMeterSource;
    foleys::LevelMeterSource outputMeterSource;
    
    // AOO stuff
    aoo::isource::pointer mAooDummySource;

    aoo::net::iserver::pointer mAooServer;
    aoo::net::iclient::pointer mAooClient;

    std::unique_ptr<EndpointState> mServerEndpoint;
    
    bool mAutoconnectGroupPeers = true;
    bool mIsConnectedToServer = false;
    String mCurrentJoinedGroup;
    double mSessionConnectionStamp = 0.0;
    bool mWatchPublicGroups = false;

    double mPrevSampleRate = 0.0;
    Atomic<bool> mPendingUnmute {false}; // jlc
    uint32 mPendingUnmuteAtStamp = 0;

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
    CriticalSection  mSourceFormatLock;

    OwnedArray<EndpointState> mEndpoints;
    
    OwnedArray<RemotePeer> mRemotePeers;


    Array<AooServerConnectionInfo> mRecentConnectionInfos;
    CriticalSection  mRecentsLock;
    
    CriticalSection  mRemotesLock;

    std::map<String,AooPublicGroupInfo> mPublicGroupInfos;
    CriticalSection  mPublicGroupsLock;

    
    
    Array<AudioCodecFormatInfo> mAudioFormats;
    int mDefaultAudioFormatIndex = 4;
    
    RangedAudioParameter * mDefaultAudioFormatParam;

    
    void initFormats();
    
    bool mRemoteSendMatrix[MAX_PEERS][MAX_PEERS];
    
    
    void notifySendThread() {
        mNeedSendSentinel += 1;
        mSendWaitable.signal();
    }
    
    WaitableEvent  mSendWaitable;
    Atomic<int>   mNeedSendSentinel  { 0 };


    std::unique_ptr<SendThread> mSendThread;
    std::unique_ptr<RecvThread> mRecvThread;
    std::unique_ptr<EventThread> mEventThread;
    std::unique_ptr<ServerThread> mServerThread;
    std::unique_ptr<ClientThread> mClientThread;


    // Input channelgroups
    SonoAudio::ChannelGroup mInputChannelGroups[MAX_CHANGROUPS];
    int mInputChannelGroupCount = 0;

    // Effects
    std::unique_ptr<Reverb> mMainReverb;
    Reverb::Parameters mMainReverbParams;
    MVerbFloat mMReverb;
    zitaRev mZitaReverb;
    MapUI  mZitaControl;
    

    ReverbModel mLastReverbModel = ReverbModelMVerb;
    
    
    // recording stuff
    
    uint32 mDefaultRecordingOptions = RecordMix;
    RecordFileFormat mDefaultRecordingFormat = FileFormatFLAC;
    int mDefaultRecordingBitsPerSample = 16;
    String mDefaultRecordDir;
    String mLastError;

    std::atomic<bool> writingPossible = { false };
    std::atomic<bool> userWritingPossible = { false };
    int totalRecordingChannels = 2;
    int64 mElapsedRecordSamples = 0;
    std::unique_ptr<TimeSliceThread> recordingThread;
    std::unique_ptr<AudioFormatWriter::ThreadedWriter> threadedMixWriter; // the FIFO used to buffer the incoming data
    std::unique_ptr<AudioFormatWriter::ThreadedWriter> threadedMixMinusWriter; // the FIFO used to buffer the incoming data
    std::unique_ptr<AudioFormatWriter::ThreadedWriter> threadedSelfWriter; // the FIFO used to buffer the incoming data

    CriticalSection writerLock;
    std::atomic<AudioFormatWriter::ThreadedWriter*> activeMixWriter { nullptr };
    std::atomic<AudioFormatWriter::ThreadedWriter*> activeMixMinusWriter { nullptr };
    std::atomic<AudioFormatWriter::ThreadedWriter*> activeSelfWriter { nullptr };
  
    // playing stuff
    AudioTransportSource mTransportSource;
    std::unique_ptr<AudioFormatReaderSource> mCurrentAudioFileSource; // the FIFO used to buffer the incoming data
    AudioFormatManager mFormatManager;
    TimeSliceThread mDiskThread  { "audio file reader" };
    
    // metronome
    std::unique_ptr<SonoAudio::Metronome> mMetronome;
   
    // misc
    bool mSliderSnapToMouse = true;

    // main state
    AudioProcessorValueTreeState mState;
    UndoManager                  mUndoManager;

};
