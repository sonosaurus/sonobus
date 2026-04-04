// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
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

#include "SoundboardChannelProcessor.h"

typedef MVerb<float> MVerbFloat;

namespace SonoAudio {
class Metronome;
}


#define MAX_PEERS 32
#define MAX_CHANGROUPS 64
#define DEFAULT_SERVER_PORT 10998
#define DEFAULT_SERVER_HOST "aoo.sonobus.net"


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


struct SBChatEvent
{
    enum EventType {
        SelfType=0,
        UserType,
        SystemType
    };

    SBChatEvent() {};
    SBChatEvent(EventType type_, const String & group_,  const String & from_, const String &targets_, const String & tags_, const String & mesg_) :
    type(type_), group(group_), from(from_), targets(targets_), tags(tags_), message(mesg_)
    {}

    EventType type = UserType;
    String group;
    String from;
    String targets;
    String tags;
    String message;
};

// chat message storage, thread-safe
typedef Array<SBChatEvent, CriticalSection> SBChatEventList;



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

    enum PeerDisplayMode {
        PeerDisplayModeFull = 0,
        PeerDisplayModeMinimal
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

    struct LatInfo {
        String sourceName;
        String destName;
        float latencyMs = 0.0f; // one way latency from source->dest in ms
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


    void getStateInformationWithOptions(MemoryBlock& destData, bool includecache=true, bool includeInputGroups = true, bool xmlformat=false);
    void setStateInformationWithOptions (const void* data, int sizeInBytes, bool includecache=true, bool includeInputGroups = true, bool xmlformat=false);

    bool saveCurrentAsDefaultPluginSettings();
    void resetDefaultPluginSettings();
    bool loadDefaultPluginSettings();
    
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
    static String paramSendSoundboardAudio;
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
    static String paramSyncMetToHost;
    static String paramSyncMetToFilePlayback;
    static String paramInputReverbLevel;
    static String paramInputReverbSize;
    static String paramInputReverbDamping;
    static String paramInputReverbPreDelay;

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

    bool setCurrentUsername(const String & name);
    String getCurrentUsername() const { return mCurrentUsername; }

    // peer stuff
    
    EndpointState * findOrAddEndpoint(const String & host, int port);
    EndpointState * findOrAddRawEndpoint(void * rawaddr);

    int getUdpLocalPort() const { return mUdpLocalPort; }
    IPAddress getLocalIPAddress() const { return mLocalIPAddress; }
    

    int getSendChannels() const { return mSendChannels.get(); }

    int connectRemotePeer(const String & host, int port, const String & username = "", const String & groupname = "",  bool reciprocate=true);
    bool disconnectRemotePeer(const String & host, int port, int32_t sourceId);
    bool disconnectRemotePeer(int index);
    bool removeRemotePeer(int index, bool sendblock=false);

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

    void setRemotePeerChannelReverbSend(int index, int changroup, float rgain);
    float getRemotePeerChannelReverbSend(int index, int changroup);

    void setRemotePeerPolarityInvert(int index, int changroup, bool invert);
    bool getRemotePeerPolarityInvert(int index, int changroup);


    bool isRemotePeerUserInGroup(const String & name) const;
    
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

    bool getLayoutFormatChangedForRemotePeer(int index) const;
    void restoreLayoutFormatForRemotePeer(int index);


    void setRemotePeerBufferTime(int index, float bufferMs);
    float getRemotePeerBufferTime(int index) const;

    void setRemotePeerAutoresizeBufferMode(int index, AutoNetBufferMode flag);
    AutoNetBufferMode getRemotePeerAutoresizeBufferMode(int index, bool & initCompleted) const;

    // acceptable limit for drop rate in dropinstance/second, above which it will adjust the jitter buffer in Auto modes
    void setAutoresizeBufferDropRateThreshold(float);
    float getAutoresizeBufferDropRateThreshold() const { return mAutoresizeDropRateThresh; }


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
    bool getRemotePeerBlockedUs(int index) const;
    
    struct LatencyInfo
    {
        float pingMs = 0.0f;
        float totalRoundtripMs = 0.0f;
        float outgoingMs = 0.0f;
        float incomingMs = 0.0f;
        float jitterMs = 0.0f;
        bool isreal = false;
        bool estimated = false;
        bool legacy = false;
    };
    
    bool getRemotePeerLatencyInfo(int index, LatencyInfo & retinfo) const;

    bool startRemotePeerLatencyTest(int index, float durationsec = 1.0);
    bool stopRemotePeerLatencyTest(int index);
    bool isRemotePeerLatencyTestActive(int index);
    
    
    bool isAnyRemotePeerRecording() const;
    bool isRemotePeerRecording(int index) const;

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


    // input monitor delay stuff
    void setInputMonitorDelayParams(int changroup, SonoAudio::DelayParams & params);
    bool getInputMonitorDelayParams(int changroup, SonoAudio::DelayParams & retparams);


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


    void setInputReverbSend(int changroup, float rgain, bool input=false);
    float getInputReverbSend(int changroup, bool input=false);

    void setInputPolarityInvert(int changroup, bool invert);
    bool getInputPolarityInvert(int changroup);

    void setInputEqParams(int changroup, SonoAudio::ParametricEqParams & params);
    bool getInputEqParams(int changroup, SonoAudio::ParametricEqParams & retparams);
    
    
    bool getInputEffectsActive(int changroup) const { return mInputChannelGroups[changroup].params.compressorParams.enabled || mInputChannelGroups[changroup].params.expanderParams.enabled || mInputChannelGroups[changroup].params.eqParams.enabled || mInputChannelGroups[changroup].params.invertPolarity || (mInputChannelGroups[changroup].params.inReverbSend > 0.0f); }

    bool getInputMonitorEffectsActive(int changroup) const { return mInputChannelGroups[changroup].params.monitorDelayParams.enabled
        || (mInputChannelGroups[changroup].params.monReverbSend > 0.0f); }

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

    bool getSyncMetToHost() const { return mSyncMetToHost.get(); }

    // misc settings
    bool getSlidersSnapToMousePosition() const { return mSliderSnapToMouse; }
    void setSlidersSnapToMousePosition(bool flag) {  mSliderSnapToMouse = flag; }

    bool getDisableKeyboardShortcuts() const { return mDisableKeyboardShortcuts; }
    void setDisableKeyboardShortcuts(bool flag) {  mDisableKeyboardShortcuts = flag; }


    struct VideoLinkInfo
    {
        ValueTree getValueTree() const;
        void setFromValueTree(const ValueTree & val);

        enum {
            PushAndView = 0,
            PushOnly = 1,
            ViewOnly =2
        };


        bool roomMode = true;
        bool showNames = true;
        bool beDirector = false;
        bool screenShareMode = false;
        bool largeShare = false;
        int pushViewMode = PushAndView;
        String extraParams;
    };

    VideoLinkInfo & getVideoLinkInfo() { return mVideoLinkInfo; }


    // sets and gets the format we send out
    void setRemotePeerAudioCodecFormat(int index, int formatIndex);
    int getRemotePeerAudioCodecFormat(int index) const;

    // gets and requests the format that we are receiving from the remote end
    bool getRemotePeerReceiveAudioCodecFormat(int index, AudioCodecFormatInfo & retinfo) const;
    bool setRequestRemotePeerSendAudioCodecFormat(int index, int formatIndex);
    int getRequestRemotePeerSendAudioCodecFormat(int index) const; // -1 is no preferences
    
    int getRemotePeerSendPacketsize(int index) const;
    void setRemotePeerSendPacketsize(int index, int psize);

    int getRemotePeerOrderPriority(int index) const;
    void setRemotePeerOrderPriority(int index, int priority);

    // select by index or by peer, or don't specify for all
    void updateRemotePeerUserFormat(int index=-1, RemotePeer * onlypeer=nullptr);


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

    foleys::LevelMeterSource & getFilePlaybackMeterSource() { return filePlaybackMeterSource; }
    foleys::LevelMeterSource & getMetronomeMeterSource() { return metMeterSource; }

    bool isAnythingRoutedToPeer(int index) const;
    
    bool isAnythingSoloed() const { return mAnythingSoloed.get(); }
    
    // IP block list
    bool isAddressBlocked(const String & ipaddr) const;
    void addBlockedAddress(const String & ipaddr);
    void removeBlockedAddress(const String & ipaddr);
    StringArray getAllBlockedAddresses() const;
    
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
        virtual void aooClientPeerJoinBlocked(SonobusAudioProcessor *comp, const String & group, const String & user, const String & address, int port) {}
        virtual void aooClientPeerLeft(SonobusAudioProcessor *comp, const String & group, const String & user) {}
        virtual void aooClientError(SonobusAudioProcessor *comp, const String & errmesg) {}
        virtual void aooClientPeerChangedState(SonobusAudioProcessor *comp, const String & mesg) {}
        virtual void sbChatEventReceived(SonobusAudioProcessor *comp, const SBChatEvent & chatevent) {}
        virtual void peerRequestedLatencyMatch(SonobusAudioProcessor *comp, const String & username, float latency) {}
        virtual void peerBlockedInfoChanged(SonobusAudioProcessor *comp, const String & username, bool blocked) {}
        virtual void peerSuggestedNewGroup(SonobusAudioProcessor *comp, const String & username, const String & newgroup, const String & grouppass, bool isPublic, const StringArray & others) {}
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

    void  setInputReverbWetLevel(float level);
    float getInputReverbWetLevel() const { return mInputReverbLevel.get(); }
    void  setInputReverbSize(float value);
    float getInputReverbSize() const { return mInputReverbSize.get(); }
    void  setInputReverbPreDelay(float valuemsec);
    float getInputReverbPreDelay() const { return mInputReverbPreDelay.get(); }


    void setMetronomeMonitorDelayParams(SonoAudio::DelayParams & params);
    bool getMetronomeMonitorDelayParams(SonoAudio::DelayParams & retparams);
    void setMetronomeChannelDestStartAndCount(int start, int count);
    bool getMetronomeChannelDestStartAndCount(int & retstart, int & retcount);
    void setMetronomePan(float pan);
    float getMetronomePan() const;
    void setMetronomeGain(float gain);
    float getMetronomeGain() const;
    void setMetronomeMonitor(float mgain);
    float getMetronomeMonitor() const;


    void setFilePlaybackMonitorDelayParams(SonoAudio::DelayParams & params);
    bool getFilePlaybackMonitorDelayParams(SonoAudio::DelayParams & retparams);
    void setFilePlaybackDestStartAndCount(int start, int count);
    bool getFilePlaybackDestStartAndCount(int & retstart, int & retcount);
    void setFilePlaybackGain(float gain);
    float getFilePlaybackGain() const;
    void setFilePlaybackMonitor(float mgain);
    float getFilePlaybackMonitor() const;


    void setLinkMonitoringDelayTimes(bool flag) { mLinkMonitoringDelayTimes = flag; }
    bool getLinkMonitoringDelayTimes() const { return mLinkMonitoringDelayTimes; }

    double getMonitoringDelayTimeFromAvgPeerLatency(float scalar=1.0f);


    // recording stuff, if record options are RecordMixOnly, or RecordSelf  (only) the file refers to the name of the file
    //   otherwise it refers to a directory where the files will be recorded (and also the prefix for the recorded files)
    bool startRecordingToFile(const URL & recordLocation, const String & filename, URL & mainreturl, uint32 recordOptions=RecordDefaultOptions, RecordFileFormat fileformat=FileFormatDefault);
    bool stopRecordingToFile();
    bool isRecordingToFile();
    double getElapsedRecordTime() const { return mElapsedRecordSamples / getSampleRate(); }
    String getLastErrorMessage() const { return mLastError; }

    void setDefaultRecordingDirectory(const URL & recdir)  { mDefaultRecordDir = recdir; }
    URL getDefaultRecordingDirectory() const { return mDefaultRecordDir; }

    void setLastBrowseDirectory(String recdir)  { mLastBrowseDir = recdir; }
    String getLastBrowseDirectory() const { return mLastBrowseDir; }

    uint32 getDefaultRecordingOptions() const { return mDefaultRecordingOptions; }
    void setDefaultRecordingOptions(uint32 opts) { mDefaultRecordingOptions = opts; }

    RecordFileFormat getDefaultRecordingFormat() const { return mDefaultRecordingFormat; }
    void setDefaultRecordingFormat(RecordFileFormat fmt) { mDefaultRecordingFormat = fmt; }

    int getDefaultRecordingBitsPerSample() const { return mDefaultRecordingBitsPerSample; }
    void setDefaultRecordingBitsPerSample(int fmt) { mDefaultRecordingBitsPerSample = fmt; }

    bool getSelfRecordingPreFX() const { return mRecordInputPreFX; }
    void setSelfRecordingPreFX(bool flag) { mRecordInputPreFX = flag; }

    bool getSelfRecordingSilenceWhenMuted() const { return mRecordInputSilenceWhenMuted; }
    void setSelfRecordingSilenceWhenMuted(bool flag) { mRecordInputSilenceWhenMuted = flag; }

    bool getRecordFinishOpens() const { return mRecordFinishOpens; }
    void setRecordFinishOpens(bool flag) { mRecordFinishOpens = flag; }

    bool getReconnectAfterServerLoss() const { return mReconnectAfterServerLoss.get(); }
    void setReconnectAfterServerLoss(bool flag) { mReconnectAfterServerLoss = flag; }


    PeerDisplayMode getPeerDisplayMode() const { return mPeerDisplayMode; }
    void setPeerDisplayMode(PeerDisplayMode mode) { mPeerDisplayMode = mode; }

    // latency match stuff
    void beginLatencyMatchProcedure();
    bool isLatencyMatchProcedureReady();
    void sendLatencyMatchToAll(float latency);
    void getLatencyInfoList(Array<LatInfo> & retlist);
    void commitLatencyMatch(float latency);

    // invite peers to new group stuff
    void suggestNewGroupToPeers(const String & group, const String & groupPass, const StringArray & peernames, bool ispublic=false);

    void sendBlockedInfoMessage(EndpointState *endpoint, bool blocked);

    // playback stuff
    bool loadURLIntoTransport (const URL& audioURL);
    void clearTransportURL();
    URL getCurrentLoadedTransportURL () const { return mCurrTransportURL; }
    AudioTransportSource & getTransportSource() { return mTransportSource; }
    AudioFormatManager & getFormatManager() { return mFormatManager; }

    // chat
    bool sendChatEvent(const SBChatEvent & event);
    void setLastChatWidth(int width) { mLastChatWidth = width;}
    int getLastChatWidth() const { return mLastChatWidth; }
    void setLastChatShown(bool shown) { mLastChatShown = shown; }
    bool getLastChatShown() const { return mLastChatShown; }
    void setChatUseFixedWidthFont(bool shown) { mChatUseFixedWidthFont = shown; }
    bool getChatUseFixedWidthFont() const { return mChatUseFixedWidthFont; }
    void setChatFontSizeOffset(int offset) { mChatFontSizeOffset = offset;}
    int getChatFontSizeOffset() const { return mChatFontSizeOffset; }
    Array<SBChatEvent, CriticalSection> & getAllChatEvents() { return mAllChatEvents; }

    // soundboard
    void setLastSoundboardWidth(int width) { mLastSoundboardWidth = width; }
    int getLastSoundboardWidth() const { return mLastSoundboardWidth; }
    void setLastSoundboardShown(bool shown) { mLastSoundboardShown = shown; }
    bool getLastSoundboardShown() const { return mLastSoundboardShown; }
    SoundboardChannelProcessor* getSoundboardProcessor() { return soundboardChannelProcessor.get(); }

    void setLastPluginBounds(juce::Rectangle<int> bounds) { mPluginWindowWidth = bounds.getWidth(); mPluginWindowHeight = bounds.getHeight();}
    juce::Rectangle<int> getLastPluginBounds() const { return juce::Rectangle<int>(0,0,mPluginWindowWidth, mPluginWindowHeight); }

    // support dir path
    File getSupportDir() const { return mSupportDir; }
    
    // language
    void setLanguageOverrideCode(const String & code) { mLangOverrideCode = code; }
    String getLanguageOverrideCode() const { return mLangOverrideCode; }

    void setUseUniversalFont(bool flag) { mUseUniversalFont = flag; }
    bool getUseUniversalFont() const { return mUseUniversalFont; }

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

        float mainGain = 1.0f;
        SonoAudio::ChannelGroupParams channelGroupParams[MAX_CHANGROUPS];
        int numChanGroups = 1;
        SonoAudio::ChannelGroupParams channelGroupMultiParams[MAX_CHANGROUPS];
        int numMultiChanGroups = 0;
        bool modifiedChanGroups = false;
        int orderPriority = -1;
    };

    // key is peer name
    typedef std::map<String, PeerStateCache>  PeerStateCacheMap;



    
    void initializeAoo(int udpPort=0);
    void cleanupAoo();
    
    void doReceiveData();
    void doSendData();
    void handleEvents();

    bool handleOtherMessage(EndpointState * endpoint, const char *msg, int32_t n);

    int32_t sendPeerMessage(RemotePeer * peer, const char *msg, int32_t n);

    void handleRemotePeerInfoUpdate(RemotePeer * peer, const juce::var & infodata);
    void sendRemotePeerInfoUpdate(int peerindex = -1, RemotePeer * topeer = nullptr);


    void handlePingEvent(EndpointState * endpoint, uint64_t tt1, uint64_t tt2, uint64_t tt3);

    void sendPingEvent(RemotePeer * peer);

    void updateSafetyMuting(RemotePeer * peer);

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
    void commitInputMonitoringParams(int changroup);

    void updateDynamicResampling();

    void updateRemotePeerSendChannels(int index, RemotePeer * remote);

    void setupSourceFormatsForAll();
    ValueTree getSendUserFormatLayoutTree();

    void applyLayoutFormatToPeer(RemotePeer * remote, const ValueTree & valtree);
    void restoreLayoutFormatForPeer(RemotePeer * remote, bool resetmulti=false);


    int connectRemotePeerRaw(void * sockaddr, const String & username = "", const String & groupname = "", bool reciprocate=true);

    int findFormatIndex(AudioCodecFormatCodec codec, int bitrate, int bitdepth);

    void ensureBuffers(int samples);

    void commitCacheForPeer(RemotePeer * peer);
    bool findAndLoadCacheForPeer(RemotePeer * peer);
    
    void loadPeerCacheFromState();
    void storePeerCacheToState();

    void loadGlobalState();
    bool storeGlobalState();

    void handleLatInfo(const juce::var & obj);
    juce::var getAllLatInfo();
    void sendReqLatInfoToAll();

    void moveOldMisplacedFiles();
    
    bool reconnectToMostRecent();

    
    ListenerList<ClientListener> clientListeners;

    
    AudioSampleBuffer tempBuffer;
    AudioSampleBuffer mixBuffer;
    AudioSampleBuffer workBuffer;
    AudioSampleBuffer inputBuffer;
    AudioSampleBuffer monitorBuffer;
    AudioSampleBuffer sendWorkBuffer;
    AudioSampleBuffer inputPostBuffer;
    AudioSampleBuffer inputPreBuffer;
    AudioSampleBuffer fileBuffer;
    AudioSampleBuffer metBuffer;
    AudioSampleBuffer mainFxBuffer;
    AudioSampleBuffer inputRevBuffer;
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
    Atomic<bool>   mSendSoundboardAudio  { false };
    Atomic<bool>   mHearLatencyTest  { false };
    Atomic<bool>   mMetIsRecorded  { true };
    Atomic<bool>   mMainReverbEnabled  { false };
    Atomic<float>   mMainReverbLevel  { 1.0f };
    Atomic<float>   mMainReverbSize  { 0.15f };
    Atomic<float>   mMainReverbDamping  { 0.5f };
    Atomic<float>   mMainReverbPreDelay  { 20.0f }; // ms
    Atomic<int>   mMainReverbModel  { ReverbModelMVerb };
    Atomic<bool>   mDynamicResampling  { false };
    Atomic<bool>   mAutoReconnectLast  { false };
    Atomic<float>   mDefUserLevel    { 1.0f };
    Atomic<bool>   mSyncMetToHost  { false };
    Atomic<bool>   mSyncMetStartToPlayback  { false };
    Atomic<bool>   mReconnectAfterServerLoss  { true };

    Atomic<float>   mInputReverbLevel  { 1.0f };
    Atomic<float>   mInputReverbSize  { 0.15f };
    Atomic<float>   mInputReverbDamping  { 0.5f };
    Atomic<float>   mInputReverbPreDelay  { 20.0f }; // ms

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
    bool mLastInputReverbEnabled = false;
    bool mFreshInit = true;
    
    Atomic<bool>   mAnythingSoloed  { false };


    int defaultAutoNetbufMode = AutoNetBufferModeAutoFull;
    
    bool mChangingDefaultAudioCodecChangesAll = false;
    bool mChangingDefaultRecvAudioCodecChangesAll = false;

    RangedAudioParameter * mDefaultAutoNetbufModeParam;
    RangedAudioParameter * mTempoParameter;

    int mUseSpecificUdpPort = 0;

    bool mLinkMonitoringDelayTimes = true;

    // acceptable limit for drop rate in dropinstance/second
    // above which it will adjust the jitter buffer in Auto modes
    float mAutoresizeDropRateThresh = 0.5f;

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

    int mLastChatWidth = 250;
    bool mLastChatShown = false;
    bool mChatUseFixedWidthFont = false;
    int mChatFontSizeOffset = 0;
    // chat message storage, thread-safe
    SBChatEventList mAllChatEvents;

    int mLastSoundboardWidth = 250;
    bool mLastSoundboardShown = false;

    int mPluginWindowWidth = 800;
    int mPluginWindowHeight = 600;


    int mActiveSendChannels = 0;

    PeerDisplayMode mPeerDisplayMode = PeerDisplayModeFull;
    
    PeerStateCacheMap mPeerStateCacheMap;
    
    // top level meter sources
    foleys::LevelMeterSource inputMeterSource;
    foleys::LevelMeterSource postinputMeterSource;
    foleys::LevelMeterSource sendMeterSource;
    foleys::LevelMeterSource outputMeterSource;
    foleys::LevelMeterSource filePlaybackMeterSource;
    foleys::LevelMeterSource metMeterSource;

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
    String mCurrentUsername;

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
    
    AooServerConnectionInfo mPendingReconnectInfo;
    bool mPendingReconnect = false;
    bool mRecoveringFromServerLoss = false;
    
    class ServerReconnectTimer : public Timer
    {
    public:
        ServerReconnectTimer(SonobusAudioProcessor & proc) : processor(proc) {
        }
        
        void timerCallback() override;
        
        SonobusAudioProcessor & processor;
    };

    ServerReconnectTimer mReconnectTimer;
    
    CriticalSection  mRemotesLock;

    std::map<String,AooPublicGroupInfo> mPublicGroupInfos;
    CriticalSection  mPublicGroupsLock;

    
    
    Array<AudioCodecFormatInfo> mAudioFormats;
    int mDefaultAudioFormatIndex = 4;
    
    RangedAudioParameter * mDefaultAudioFormatParam;




    CriticalSection  mLatInfoLock;
    Array<LatInfo> mLatInfoList;


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

    // input reverb
    MVerbFloat mInputReverb;


    // met and playback channel groups
    SonoAudio::ChannelGroup  mMetChannelGroup;
    SonoAudio::ChannelGroup  mFilePlaybackChannelGroup;

    float _lastfplaygain = 0.0f;

    // and a replicant one for recording purposes
    SonoAudio::ChannelGroup  mRecMetChannelGroup;
    SonoAudio::ChannelGroup  mRecFilePlaybackChannelGroup;

    
    // recording stuff
    
    uint32 mDefaultRecordingOptions = RecordMix;
    RecordFileFormat mDefaultRecordingFormat = FileFormatFLAC;
    int mDefaultRecordingBitsPerSample = 16;
    bool mRecordInputPreFX = true;
    bool mRecordInputSilenceWhenMuted = true;
    bool mRecordFinishOpens = true;
    URL mDefaultRecordDir;
    String mLastError;
    int mSelfRecordChannels = 2;
    int mActiveInputChannels = 2;
    String mLastBrowseDir;

    std::atomic<bool> writingPossible = { false };
    std::atomic<bool> userWritingPossible = { false };
    int totalRecordingChannels = 2;
    int64 mElapsedRecordSamples = 0;
    std::unique_ptr<TimeSliceThread> recordingThread;
    std::unique_ptr<AudioFormatWriter::ThreadedWriter> threadedMixWriter;
    std::unique_ptr<AudioFormatWriter::ThreadedWriter> threadedMixMinusWriter;
    OwnedArray<AudioFormatWriter::ThreadedWriter> threadedSelfWriters;
    int  mSelfRecordChans[MAX_CHANGROUPS] { 0 };

    CriticalSection writerLock;
    std::atomic<AudioFormatWriter::ThreadedWriter*> activeMixWriter { nullptr };
    std::atomic<AudioFormatWriter::ThreadedWriter*> activeMixMinusWriter { nullptr };
    std::atomic<AudioFormatWriter::ThreadedWriter*> activeSelfWriters[MAX_CHANGROUPS] { nullptr };

    // playing stuff
    AudioTransportSource mTransportSource;
    std::unique_ptr<AudioFormatReaderSource> mCurrentAudioFileSource;
    AudioFormatManager mFormatManager;
    TimeSliceThread mDiskThread  { "audio file reader" };
    URL mCurrTransportURL;
    bool mTransportWasPlaying = false;

    // soundboard
    std::unique_ptr<SoundboardChannelProcessor> soundboardChannelProcessor;

    // metronome
    std::unique_ptr<SonoAudio::Metronome> mMetronome;
   
    // misc
    bool mSliderSnapToMouse = true;
    bool mDisableKeyboardShortcuts = false;
    VideoLinkInfo mVideoLinkInfo;
    
    File mSupportDir;
    
    // global config
    ValueTree   mGlobalState;
    
    String mLangOverrideCode;
    bool mUseUniversalFont = false;

    // main state
    AudioProcessorValueTreeState mState;
    UndoManager                  mUndoManager;

};


inline bool operator==(const SonobusAudioProcessor::LatInfo& lhs, const SonobusAudioProcessor::LatInfo& rhs) {
    // compare all except timestamp
     return (lhs.sourceName == rhs.sourceName
             && lhs.destName == rhs.destName
             );
}

inline bool operator!=(const SonobusAudioProcessor::LatInfo& lhs, const SonobusAudioProcessor::LatInfo& rhs){ return !(lhs == rhs); }

inline bool operator<(const SonobusAudioProcessor::LatInfo& lhs, const SonobusAudioProcessor::LatInfo& rhs)
{
    // default sorting alpha
    return (lhs.sourceName.compareIgnoreCase(rhs.sourceName) < 0);
}
