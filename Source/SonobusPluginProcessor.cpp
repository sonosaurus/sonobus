// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell



#include "SonobusPluginProcessor.h"
#include "SonobusPluginEditor.h"

#include "RunCumulantor.h"

#include "aoo/aoo.h"
#include "aoo/codec/aoo_pcm.h"
#include "aoo/codec/aoo_opus.h"

//#include "aoo/aoo_net.hpp"
#include "common/net_utils.hpp"
#include "common/time.hpp"

#include "oscpack/osc/OscOutboundPacketStream.h"
#include "oscpack/osc/OscReceivedElements.h"

#include "mtdm.h"

#include <algorithm>
#include <thread>

#include "LatencyMeasurer.h"
#include "Metronome.h"

using namespace SonoAudio;

#if JUCE_WINDOWS
#define NOMINMAX
#include <WinSock2.h>
#include <WS2tcpip.h>
typedef int socklen_t;
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#define MAX_DELAY_SAMPLES 192000
#define SENDBUFSIZE_SCALAR 2.0f
#define PEER_PING_INTERVAL_MS 2000.0

#define LOCAL_SERVER_PORT 10999

String SonobusAudioProcessor::paramInGain     ("ingain");
String SonobusAudioProcessor::paramDry     ("dry");
String SonobusAudioProcessor::paramInMonitorMonoPan     ("inmonmonopan");
String SonobusAudioProcessor::paramInMonitorPan1     ("inmonpan1");
String SonobusAudioProcessor::paramInMonitorPan2     ("inmonpan2");
String SonobusAudioProcessor::paramWet     ("wet");
String SonobusAudioProcessor::paramBufferTime  ("buffertime");
String SonobusAudioProcessor::paramDefaultNetbufMs ("defnetbuf");
String SonobusAudioProcessor::paramDefaultAutoNetbuf ("defnetauto");
String SonobusAudioProcessor::paramDefaultSendQual ("defsendqual");
String SonobusAudioProcessor::paramMainSendMute ("mastsendmute");
String SonobusAudioProcessor::paramMainRecvMute ("mastrecvmute");
String SonobusAudioProcessor::paramMainInMute ("mastinmute");
String SonobusAudioProcessor::paramMainMonitorSolo ("mastmonsolo");
String SonobusAudioProcessor::paramMetEnabled ("metenabled");
String SonobusAudioProcessor::paramMetGain     ("metgain");
String SonobusAudioProcessor::paramMetTempo     ("mettempo");
String SonobusAudioProcessor::paramSendChannels    ("sendchannels");
String SonobusAudioProcessor::paramSendMetAudio    ("sendmetaudio");
String SonobusAudioProcessor::paramSendFileAudio    ("sendfileaudio");
String SonobusAudioProcessor::paramSendSoundboardAudio    ("sendsoundboardaudio");
String SonobusAudioProcessor::paramHearLatencyTest   ("hearlatencytest");
String SonobusAudioProcessor::paramMetIsRecorded   ("metisrecorded");
String SonobusAudioProcessor::paramMainReverbEnabled  ("mainreverbenabled");
String SonobusAudioProcessor::paramMainReverbLevel  ("nmainreverblevel");
String SonobusAudioProcessor::paramMainReverbSize  ("mainreverbsize");
String SonobusAudioProcessor::paramMainReverbDamping  ("mainreverbdamp");
String SonobusAudioProcessor::paramMainReverbPreDelay  ("mainreverbpredelay");
String SonobusAudioProcessor::paramMainReverbModel  ("mainreverbmodel");
String SonobusAudioProcessor::paramDynamicResampling  ("dynamicresampling");
String SonobusAudioProcessor::paramAutoReconnectLast  ("reconnectlast");
String SonobusAudioProcessor::paramDefaultPeerLevel  ("defPeerLevel");
String SonobusAudioProcessor::paramSyncMetToHost  ("syncMetHost");
String SonobusAudioProcessor::paramSyncMetToFilePlayback  ("syncMetFile");
String SonobusAudioProcessor::paramInputReverbLevel  ("inreverblevel");
String SonobusAudioProcessor::paramInputReverbSize  ("inreverbsize");
String SonobusAudioProcessor::paramInputReverbDamping  ("inreverbdamp");
String SonobusAudioProcessor::paramInputReverbPreDelay  ("inreverbpredelay");

static String recentsCollectionKey("RecentConnections");
static String recentsItemKey("ServerConnectionInfo");

static String extraStateCollectionKey("ExtraState");
static String useSpecificUdpPortKey("UseUdpPort");
static String changeQualForAllKey("ChangeQualForAll");
static String changeRecvQualForAllKey("ChangeRecvQualForAll");
static String defRecordOptionsKey("DefaultRecordingOptions");
static String defRecordFormatKey("DefaultRecordingFormat");
static String defRecordBitsKey("DefaultRecordingBitsPerSample");
static String recordSelfPreFxKey("RecordSelfPreFx");
static String recordFinishOpenKey("RecordFinishOpen");
static String defRecordDirKey("DefaultRecordDir");
static String defRecordDirURLKey("DefaultRecordDirURL");
static String lastBrowseDirKey("LastBrowseDir");
static String sliderSnapKey("SliderSnapToMouse");
static String disableShortcutsKey("DisableKeyShortcuts");
static String peerDisplayModeKey("PeerDisplayMode");
static String lastChatWidthKey("lastChatWidth");
static String lastChatShownKey("lastChatShown");
static String lastSoundboardWidthKey("lastSoundboardWidth");
static String lastSoundboardShownKey("lastSoundboardShown");
static String chatUseFixedWidthFontKey("chatFixedWidthFont");
static String chatFontSizeOffsetKey("chatFontSizeOffset");
static String linkMonitoringDelayTimesKey("linkMonDelayTimes");
static String langOverrideCodeKey("langOverrideCode");
static String lastWindowWidthKey("lastWindowWidth");
static String lastWindowHeightKey("lastWindowHeight");
static String autoresizeDropRateThreshKey("autoDropRateThreshNew");
static String reconnectServerLossKey("reconnServLoss");

static String compressorStateKey("CompressorState");
static String expanderStateKey("ExpanderState");
static String limiterStateKey("LimiterState");
static String eqStateKey("ParametricEqState");

static String lastUsernameKey("lastUsername");

static String blockedAddressesKey("BlockedAddresses");
static String addressKey("Address");
static String addressValueKey("value");


//static String inputEffectsStateKey("InputEffects");

static String inputChannelGroupsStateKey("InputChannelGroups");
static String extraChannelGroupsStateKey("ExtraChannelGroups");
static String channelGroupsStateKey("ChannelGroups");
static String channelGroupsMultiStateKey("MultiChannelGroups");
static String channelGroupStateKey("ChannelGroup");
static String numChanGroupsKey("numChanGroups");
static String numMultiChanGroupsKey("numMultiChanGroups");
static String modifiedChanGroupsKey("modifiedChanGroups");

static String channelLayoutsKey("ChannelLayouts");


static String peerStateCacheMapKey("PeerStateCacheMap");
static String peerStateCacheKey("PeerStateCache");
static String peerNameKey("name");
static String peerLevelKey("level");
static String peerMonoPanKey("pan");
static String peerPan1Key("span1");
static String peerPan2Key("span2");
static String peerNetbufKey("netbuf");
static String peerNetbufAutoKey("netbufauto");
static String peerSendFormatKey("sendformat");
static String peerOrderPriorityKey("orderpriority");


#define METER_RMS_SEC 0.03
#define MAX_PANNERS 64

#define ECHO_SINKSOURCE_ID 12321
#define DEFAULT_UDP_PORT 11000

#define UDP_OVERHEAD_BYTES 0 // 28

// get sockaddr, IPv4 or IPv6:
static void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return &(((struct sockaddr_in*)sa)->sin_addr);
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

static int get_in_port(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return (((struct sockaddr_in*)sa)->sin_port);

    return (((struct sockaddr_in6*)sa)->sin6_port);
}

static addrinfo* getAddressInfo (bool isDatagram, const String& hostName, int portNumber)
{
    struct addrinfo hints;
    zerostruct (hints);

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = isDatagram ? SOCK_DGRAM : SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;

    struct addrinfo* info = nullptr;

    if (getaddrinfo (hostName.toRawUTF8(), String (portNumber).toRawUTF8(), &hints, &info) == 0)
        return info;

    return nullptr;
}

#if 0
struct SonobusAudioProcessor::EndpointState {
    EndpointState(String ipaddr_="", int port_=0) : ipaddr(ipaddr_), port(port_) {
        rawaddr.sa_family = AF_UNSPEC;
    }
    

    DatagramSocket *owner;
    //struct sockaddr_storage addr;
    //socklen_t addrlen;
    std::unique_ptr<DatagramSocket::RemoteAddrInfo> peer;
    String ipaddr;
    int port = 0;
    
    
    struct sockaddr * getRawAddr() {
        if (rawaddr.sa_family == AF_UNSPEC) {
            struct addrinfo * info = getAddressInfo(true, ipaddr, port);
            if (info) {
                memcpy(&rawaddr, info->ai_addr, info->ai_addrlen);

                freeaddrinfo(info);
            }
        }
        return &rawaddr;
    }
    
    
    // runtime state
    int64_t sentBytes = 0;
    int64_t recvBytes = 0;
    
private:
    struct sockaddr rawaddr;
    
};
#else
struct SonobusAudioProcessor::EndpointState {
    EndpointState(aoo::ip_address addr) : address(addr) {

        ipaddr = address.name_unmapped();
        port = address.port();
    }

    int ownerSocket = -1;

    String ipaddr;
    int port = 0;

    AooId groupid = kAooIdInvalid;
    AooId userid = kAooIdInvalid;

    struct sockaddr * getRawAddr() {
        return address.address_ptr();
    }


    // runtime state
    int64_t sentBytes = 0;
    int64_t recvBytes = 0;

    aoo::ip_address address;
};

#endif



#define LATENCY_ID_OFFSET 20000
#define ECHO_ID_OFFSET    40000

enum {
    RemoteNetTypeUnknown = 0,
    RemoteNetTypeEthernet = 1,
    RemoteNetTypeWifi = 2,
    RemoteNetTypeMobileData = 3
};

struct ProcessorIdPair
{
    ProcessorIdPair(SonobusAudioProcessor *proc=nullptr, AooId id_=-1) : processor(proc), id(id_) {}
    SonobusAudioProcessor * processor;
    AooId id;
};


struct SonobusAudioProcessor::RemotePeer {
    RemotePeer(EndpointState * ep = 0, int id_=0, AooSink::Ptr oursink_ = 0, AooSource::Ptr oursource_ = 0) : endpoint(ep),
        ourId(id_), 
        oursink(std::move(oursink_)), oursource(std::move(oursource_))
    {
        for (int i=0; i < MAX_PANNERS; ++i) {
            recvPan[i] = recvPanLast[i] = 0.0f;
            if ((i % 2) == 0) {
                recvStereoPan[i] = -1.0f;
            } else {
                recvStereoPan[i] = 1.0f;
            }
        }
            
        oursink = AooSink::create(ourId, nullptr);
        oursource = AooSource::create(ourId, nullptr);
    }

    EndpointState * endpoint = 0;
    int32_t ourId = kAooIdInvalid;
    int32_t remoteSinkId = kAooIdInvalid;
    int32_t remoteSourceId = kAooIdInvalid;
    AooSink::Ptr oursink;
    AooSource::Ptr oursource;

    ProcessorIdPair oursinkpp;
    ProcessorIdPair oursourcepp;

    float gain = 1.0f;

    float buffertimeMs = 0.0f;
    float padBufferTimeMs = 0.0f;
    AutoNetBufferMode  autosizeBufferMode = AutoNetBufferModeAutoFull;
    bool sendActive = false;
    bool sendCommonActive = false;
    bool recvActive = false;
    bool sendAllow = true;
    bool recvAllow = true;
    bool recvAllowCache = false; // used for recvmute all state
    bool sendAllowCache = false; // used for sendmute all state
    bool soloed = false;
    bool invitedPeer = false;
    int  formatIndex = -1; // default
    AudioCodecFormatInfo recvFormat;
    int reqRemoteSendFormatIndex = -1; // no pref
    int packetsize = 600;
    int sendChannels = 1; // actual current send channel count
    int nominalSendChannels = 1; // 0 matches input, 1 is 1, 2 is 2
    int sendChannelsOverride = -1; // -1 don't override
    int recvChannels = 0;
    float recvPan[MAX_PANNERS];
    float recvStereoPan[MAX_PANNERS]; // only use 2
    // runtime state
    float _lastgain = 0.0f;
    bool connected = false;
    String userName;
    String groupName;
    AooId userId = kAooIdInvalid;
    AooId groupId = kAooIdInvalid;
    int64_t dataPacketsReceived = 0;
    int64_t dataPacketsSent = 0;
    int64_t dataPacketsDropped = 0;
    int64_t dataPacketsResent = 0;
    double lastDroptime = 0;
    double resetDroptime = 0;
    int64_t lastDropCount = 0;
    double lastNetBufDecrTime = 0;
    float netBufAutoBaseline = 0.0f;
    bool autoNetbufInitCompleted = false;
    bool latencyMatched = false;
    bool resetSafetyMuted = true;
    float pingTime = 0.0f; // ms
    double lastSendPingTimeMs = -1;
    bool   gotNewStylePing = false;
    bool   haveSentFirstPeerInfo = false;
    stats::RunCumulantor1D  smoothPingTime; // ms
    stats::RunCumulantor1D  fillRatio;
    stats::RunCumulantor1D  fillRatioSlow;
    stats::RunCumulantor1D  fastDropRate;
    float totalEstLatency = 0.0f; // ms
    float totalLatency = 0.0f; // ms
    float bufferTimeAtRealLatency = 0.0f; // ms
    bool hasRealLatency = false;
    bool latencyDirty = false;
    AudioSampleBuffer workBuffer;
    float recvPanLast[MAX_PANNERS];
    // metering
    foleys::LevelMeterSource sendMeterSource;
    foleys::LevelMeterSource recvMeterSource;
    bool viewExpanded = false;
    int orderPriority = -1;

    // channel groups
    SonoAudio::ChannelGroup chanGroups[MAX_CHANGROUPS];
    int numChanGroups = 1;
    bool modifiedChanGroups = false;
    bool modifiedMultiChanGroups = false;
    bool recvdChanLayout = false;
    SonoAudio::ChannelGroupParams lastMultiChanParams[MAX_CHANGROUPS];
    int lastMultiNumChanGroups = 0;
    // runtime state
    SonoAudio::ChannelGroupParams origChanParams[MAX_CHANGROUPS];
    int origNumChanGroups = 1;

    // remote info
    float remoteJitterBufMs = 0.0f;
    float remoteInLatMs = 0.0f;
    float remoteOutLatMs = 0.0f;
    int remoteNetType = RemoteNetTypeUnknown;
    bool remoteIsRecording = false;
    bool hasRemoteInfo = false;
    bool blockedUs = false;

    struct PeerInfo {
        int32_t flags = 0;
    };
    PeerInfo aooPeerInfo;

    std::unique_ptr<AudioFormatWriter::ThreadedWriter> fileWriter;


    ReadWriteLock    sinkLock;
};



static int32_t endpoint_send(void *e, const AooByte *data, int32_t size)
{
    SonobusAudioProcessor::EndpointState * endpoint = static_cast<SonobusAudioProcessor::EndpointState*>(e);
    int result = -1;
    //if (endpoint->peer) {
    //    result = endpoint->owner->write(*(endpoint->peer), data, size);
    //} else {
    //    result = endpoint->owner->write(endpoint->ipaddr, endpoint->port, data, size);
    //}

    result = aoo::socket_sendto(endpoint->ownerSocket, data, size, endpoint->address);

    if (result > 0) {
        // include UDP overhead
        endpoint->sentBytes += result + UDP_OVERHEAD_BYTES;
    }
    else if (result < 0) {
        DBG("Error sending bytes to endpoint " << endpoint->ipaddr);
    }
    return result;
}

int32_t SonobusAudioProcessor::udpsend(void *user, const AooByte *msg, AooInt32 size,
                      const void *addr, AooAddrSize addrlen, AooFlag flags)
{
    auto x = (SonobusAudioProcessor *)user;
    aoo::ip_address address((const sockaddr *)addr, addrlen);

    // lookup endpoint
    if (auto * es = x->findEndpoint(address)) {
        return endpoint_send(es, msg, size);
    }
    else {
        return aoo::socket_sendto(x->mUdpSocketHandle, msg, size, address);
    }
}


class SonobusAudioProcessor::SendThread : public juce::Thread
{
public:
    SendThread(SonobusAudioProcessor & processor) : Thread("SonoBusSendThread") , _processor(processor) 
    {}
    
    void run() override {
        
        setPriority(Thread::Priority::highest);

        bool shouldwait = false;

        while (!threadShouldExit()) {
            // don't overcall it, but make sure it runs consistently
            // if we are notified to send, the wait will return sooner than the timeout

            if (shouldwait) {
                _processor.mSendWaitable.wait(20);
            }

            auto sentinel = _processor.mNeedSendSentinel.get();

            _processor.doSendData();

            shouldwait = (sentinel == _processor.mNeedSendSentinel.get());

        }
        DBG("Send thread finishing");
    }
    
    SonobusAudioProcessor & _processor;
    
};

class SonobusAudioProcessor::RecvThread : public juce::Thread
{
public:
    RecvThread(SonobusAudioProcessor & processor) : Thread("SonoBusRecvThread") , _processor(processor) 
    {}
    
    void run() override {

        setPriority(Thread::Priority::highest);

        while (!threadShouldExit()) {

            _processor.doReceiveData();
        }

        DBG("Recv thread finishing");        
    }
    
    SonobusAudioProcessor & _processor;
    
};

class SonobusAudioProcessor::EventThread : public juce::Thread
{
public:
    EventThread(SonobusAudioProcessor & processor) : Thread("SonoBusEventThread") , _processor(processor) 
    {}
    
    void run() override {

        while (!threadShouldExit()) {
         
            Thread::sleep(20);
            
            _processor.handleEvents();                       
        }
        
        DBG("Event thread finishing");
    }
    
    SonobusAudioProcessor & _processor;
    
};

class SonobusAudioProcessor::ServerThread : public juce::Thread
{
public:
    ServerThread(SonobusAudioProcessor & processor) : Thread("SonoBusServerThread") , _processor(processor) 
    {}
    
    void run() override {

        //if (_processor.mAooServer) {
        //    _processor.mAooServer->run();
        //}
        
        DBG("Server thread finishing");        
    }
    
    SonobusAudioProcessor & _processor;
    
};

class SonobusAudioProcessor::ClientThread : public juce::Thread
{
public:
    ClientThread(SonobusAudioProcessor & processor) : Thread("SonoBusClientThread") , _processor(processor) 
    {}
    
    void run() override {

        if (_processor.mAooClient) {
            _processor.mAooClient->run(false);
        }
        
        DBG("Client thread finishing");        
    }
    
    SonobusAudioProcessor & _processor;
    
};


enum {
    OutMixBusIndex = 0,
    OutSelfBusIndex,
    OutUserBaseBusIndex,
    OutUserLastBusIndex = 9
};

#if JUCE_IOS
#define ALTBUS_ACTIVE true
#else
#define ALTBUS_ACTIVE false
#endif


SonobusAudioProcessor::BusesProperties SonobusAudioProcessor::getDefaultLayout()
{
    auto props = SonobusAudioProcessor::BusesProperties();
    auto plugtype = PluginHostType::getPluginLoadedAs();

    // common to all
    props = props.withInput  ("Main In",  AudioChannelSet::stereo(), true)
    .withOutput ("Mix Out", AudioChannelSet::stereo(), true);


    // extra inputs
    if (plugtype == AudioProcessor::wrapperType_AAX) {
        // only one sidechain mono allowed, doesn't even work anyway
        props = props.withInput ("Aux 1 In", AudioChannelSet::mono(), ALTBUS_ACTIVE);
    }
    else if (plugtype == AudioProcessor::wrapperType_VST) {
        // no multi-bus outputs for now for VST2, so it works in OBS
    }
    else {
        // throw in some input sidechains
        props = props.withInput  ("Aux 1 In",  AudioChannelSet::stereo(), ALTBUS_ACTIVE)
        .withInput  ("Aux 2 In",  AudioChannelSet::stereo(), ALTBUS_ACTIVE)
        .withInput  ("Aux 3 In",  AudioChannelSet::stereo(), ALTBUS_ACTIVE)
        .withInput  ("Aux 4 In",  AudioChannelSet::stereo(), ALTBUS_ACTIVE)
        .withInput  ("Aux 5 In",  AudioChannelSet::stereo(), ALTBUS_ACTIVE)
        .withInput  ("Aux 6 In",  AudioChannelSet::stereo(), ALTBUS_ACTIVE)
        .withInput  ("Aux 7 In",  AudioChannelSet::stereo(), ALTBUS_ACTIVE)
        .withInput  ("Aux 8 In",  AudioChannelSet::stereo(), ALTBUS_ACTIVE);
    }

    // outputs
    if (plugtype == AudioProcessor::wrapperType_VST) {
        // no multi-bus outputs for now for VST2, so it works in OBS
    }
    else {
        props = props.withOutput ("Aux 1 Out", AudioChannelSet::stereo(), ALTBUS_ACTIVE)
        .withOutput ("Aux 2 Out", AudioChannelSet::stereo(), ALTBUS_ACTIVE)
        .withOutput ("Aux 3 Out", AudioChannelSet::stereo(), ALTBUS_ACTIVE)
        .withOutput ("Aux 4 Out", AudioChannelSet::stereo(), ALTBUS_ACTIVE)
        .withOutput ("Aux 5 Out", AudioChannelSet::stereo(), ALTBUS_ACTIVE)
        .withOutput ("Aux 6 Out", AudioChannelSet::stereo(), ALTBUS_ACTIVE)
        .withOutput ("Aux 7 Out", AudioChannelSet::stereo(), ALTBUS_ACTIVE)
        .withOutput ("Aux 8 Out", AudioChannelSet::stereo(), ALTBUS_ACTIVE);
    }


    return props;
}


//==============================================================================
SonobusAudioProcessor::SonobusAudioProcessor()
: AudioProcessor ( getDefaultLayout() ),
mReconnectTimer(*this),
soundboardChannelProcessor(std::make_unique<SoundboardChannelProcessor>()),
mGlobalState("SonobusGlobalState"),
mState (*this, &mUndoManager, "SonoBusAoO",
{
           std::make_unique<AudioParameterFloat>(ParameterID(paramInGain, 1),     TRANS ("In Gain"),    NormalisableRange<float>(0.0, 4.0, 0.0, 0.33), mInGain.get(), "", AudioProcessorParameter::genericParameter,
                                          [](float v, int maxlen) -> String { return Decibels::toString(Decibels::gainToDecibels(v), 1); }, 
                                          [](const String& s) -> float { return Decibels::decibelsToGain(s.getFloatValue()); }),

    std::make_unique<AudioParameterFloat>(ParameterID(paramInMonitorMonoPan, 1),     TRANS ("In Pan"),    NormalisableRange<float>(-1.0, 1.0, 0.0), mInMonMonoPan.get(), "", AudioProcessorParameter::genericParameter,
                                          [](float v, int maxlen) -> String { if (fabs(v) < 0.01) return TRANS("C"); return String((int)rint(abs(v*100.0f))) + ((v > 0 ? "% R" : "% L")) ; },
                                          [](const String& s) -> float { return s.getFloatValue()*1e-2f; }),

    std::make_unique<AudioParameterFloat>(ParameterID(paramInMonitorPan1, 1),     TRANS ("In Pan 1"),    NormalisableRange<float>(-1.0, 1.0, 0.0), mInMonPan1.get(), "", AudioProcessorParameter::genericParameter,
                                          [](float v, int maxlen) -> String { if (fabs(v) < 0.01) return TRANS("C"); return String((int)rint(abs(v*100.0f))) + ((v > 0 ? "% R" : "% L")) ; },
                                          [](const String& s) -> float { return s.getFloatValue()*1e-2f; }),

    std::make_unique<AudioParameterFloat>(ParameterID(paramInMonitorPan2, 1),     TRANS ("In Pan 2"),    NormalisableRange<float>(-1.0, 1.0, 0.0), mInMonPan2.get(), "", AudioProcessorParameter::genericParameter,
                                          [](float v, int maxlen) -> String { if (fabs(v) < 0.01) return TRANS("C"); return String((int)rint(abs(v*100.0f))) + ((v > 0 ? "% R" : "% L")) ; },
                                          [](const String& s) -> float { return s.getFloatValue()*1e-2f; }),

    std::make_unique<AudioParameterFloat>(ParameterID(paramDry, 1),     TRANS ("Dry Level"),    NormalisableRange<float>(0.0,    1.0, 0.0, 0.5), JUCEApplicationBase::isStandaloneApp() ? mDry.get() : 1.0f, "", AudioProcessorParameter::genericParameter,
                                          [](float v, int maxlen) -> String { return Decibels::toString(Decibels::gainToDecibels(v), 1); }, 
                                          [](const String& s) -> float { return Decibels::decibelsToGain(s.getFloatValue()); }),

    std::make_unique<AudioParameterFloat>(ParameterID(paramWet, 1),     TRANS ("Output Level"),    NormalisableRange<float>(0.0, 2.0, 0.0, 0.5), mWet.get(), "", AudioProcessorParameter::genericParameter,
                                          [](float v, int maxlen) -> String { return Decibels::toString(Decibels::gainToDecibels(v), 1); }, 
                                          [](const String& s) -> float { return Decibels::decibelsToGain(s.getFloatValue()); }),

    std::make_unique<AudioParameterFloat>(ParameterID(paramDefaultNetbufMs, 1),     TRANS ("Default Jitter Buffer Time"),    NormalisableRange<float>(0.0, mMaxBufferTime.get(), 0.001, 0.5), mBufferTime.get(), "", AudioProcessorParameter::genericParameter,
                                          [](float v, int maxlen) -> String { return String(v*1000.0) + " ms"; }, 
                                          [](const String& s) -> float { return s.getFloatValue()*1e-3f; }),
    std::make_unique<AudioParameterChoice>(ParameterID(paramSendChannels, 1), TRANS ("Send Channels"), StringArray({ "Match # Inputs", "Send Mono", "Send Stereo"}), JUCEApplicationBase::isStandaloneApp() ? mSendChannels.get() : 0),

    std::make_unique<AudioParameterBool>(ParameterID(paramMetEnabled, 1), TRANS ("Metronome Enabled"), mMetEnabled.get()),
    std::make_unique<AudioParameterBool>(ParameterID(paramSendMetAudio, 1), TRANS ("Send Metronome Audio"), mSendMet.get()),
    std::make_unique<AudioParameterFloat>(ParameterID(paramMetGain, 1),     TRANS ("Metronome Gain"),    NormalisableRange<float>(0.0, 1.0, 0.0, 0.5), mMetGain.get(), "", AudioProcessorParameter::genericParameter,
                                          [](float v, int maxlen) -> String { return Decibels::toString(Decibels::gainToDecibels(v), 1); },
                                          [](const String& s) -> float { return Decibels::decibelsToGain(s.getFloatValue()); }),

    std::make_unique<AudioParameterFloat>(ParameterID(paramMetTempo, 1),     TRANS ("Metronome Tempo"),    NormalisableRange<float>(10.0, 400.0, 1, 0.5), mMetTempo.get(), "", AudioProcessorParameter::genericParameter,
                                          [](float v, int maxlen) -> String { return String(v) + " bpm"; },
                                          [](const String& s) -> float { return s.getFloatValue(); }),

    std::make_unique<AudioParameterBool>(ParameterID(paramSendFileAudio, 1), TRANS ("Send Playback Audio"), mSendPlaybackAudio.get()),
    std::make_unique<AudioParameterBool>(ParameterID(paramSendSoundboardAudio, 1), TRANS ("Send Soundboard Audio"), mSendSoundboardAudio.get()),
    std::make_unique<AudioParameterBool>(ParameterID(paramHearLatencyTest, 1), TRANS ("Hear Latency Test"), mHearLatencyTest.get()),
    std::make_unique<AudioParameterBool>(ParameterID(paramMetIsRecorded, 1), TRANS ("Record Metronome to File"), mMetIsRecorded.get()),
    std::make_unique<AudioParameterBool>(ParameterID(paramMainReverbEnabled, 1), TRANS ("Main Reverb Enabled"), mMainReverbEnabled.get()),
    std::make_unique<AudioParameterFloat>(ParameterID(paramMainReverbLevel, 1),     TRANS ("Main Reverb Level"),    NormalisableRange<float>(0.0,    1.0, 0.0, 0.4), mMainReverbLevel.get(), "", AudioProcessorParameter::genericParameter,
                                          [](float v, int maxlen) -> String { return Decibels::toString(Decibels::gainToDecibels(v), 1); }, 
                                          [](const String& s) -> float { return Decibels::decibelsToGain(s.getFloatValue()); }),
    std::make_unique<AudioParameterFloat>(ParameterID(paramMainReverbSize, 1),     TRANS ("Main Reverb Size"),    NormalisableRange<float>(0.0, 1.0, 0.0), mMainReverbSize.get(), "", AudioProcessorParameter::genericParameter,
                                          [](float v, int maxlen) -> String { return String((int)(v*100)) + " %"; },
                                          [](const String& s) -> float { return s.getFloatValue()*0.01f; }),
    std::make_unique<AudioParameterFloat>(ParameterID(paramMainReverbDamping, 1),     TRANS ("Main Reverb Damping"),    NormalisableRange<float>(0.0, 1.0, 0.0), mMainReverbDamping.get(), "", AudioProcessorParameter::genericParameter,
                                          [](float v, int maxlen) -> String { return String((int)(v*100)) + " %"; },
                                          [](const String& s) -> float { return s.getFloatValue()*0.01f; }),
    std::make_unique<AudioParameterFloat>(ParameterID(paramMainReverbPreDelay, 1),     TRANS ("Pre-Delay Time"),    NormalisableRange<float>(0.0, 100.0, 1.0, 1.0), mMainReverbPreDelay.get(), "", AudioProcessorParameter::genericParameter,
                                          [](float v, int maxlen) -> String { return String(v, 0) + " ms"; }, 
                                          [](const String& s) -> float { return s.getFloatValue(); }),

    std::make_unique<AudioParameterChoice>(ParameterID(paramMainReverbModel, 1), TRANS ("Main Reverb Model"), StringArray({ "Freeverb", "MVerb", "Zita"}), mMainReverbModel.get()),

    std::make_unique<AudioParameterBool>(ParameterID(paramMainSendMute, 1), TRANS ("Main Send Mute"), mMainSendMute.get()),
    std::make_unique<AudioParameterBool>(ParameterID(paramMainRecvMute,1 ), TRANS ("Main Receive Mute"), mMainRecvMute.get()),
    std::make_unique<AudioParameterBool>(ParameterID(paramMainInMute, 1), TRANS ("Main In Mute"), mMainInMute.get()),
    std::make_unique<AudioParameterBool>(ParameterID(paramMainMonitorSolo, 1), TRANS ("Main Monitor Solo"), mMainMonitorSolo.get()),

    std::make_unique<AudioParameterChoice>(ParameterID(paramDefaultAutoNetbuf, 1), TRANS ("Def Auto Net Buffer Mode"), StringArray({ "Off", "Auto-Increase", "Auto-Full", "Initial-Auto"}), defaultAutoNetbufMode),

    std::make_unique<AudioParameterInt>(ParameterID(paramDefaultSendQual, 1), TRANS ("Def Send Format"), 0, 14, mDefaultAudioFormatIndex),
    std::make_unique<AudioParameterBool>(ParameterID(paramDynamicResampling, 1), TRANS ("Dynamic Resampling"), mDynamicResampling.get()),
    std::make_unique<AudioParameterBool>(ParameterID(paramAutoReconnectLast, 1), TRANS ("Reconnect Last"), mAutoReconnectLast.get()),
    std::make_unique<AudioParameterFloat>(ParameterID(paramDefaultPeerLevel, 1),     TRANS ("Default User Level"),    NormalisableRange<float>(0.0,    1.0, 0.0, 0.5), mDefUserLevel.get(), "", AudioProcessorParameter::genericParameter,
                                          [](float v, int maxlen) -> String { return Decibels::toString(Decibels::gainToDecibels(v), 1); },
                                          [](const String& s) -> float { return Decibels::decibelsToGain(s.getFloatValue()); }),
    std::make_unique<AudioParameterBool>(ParameterID(paramSyncMetToHost, 1), TRANS ("Sync to Host"), JUCEApplicationBase::isStandaloneApp() ? false : true),
    std::make_unique<AudioParameterFloat>(ParameterID(paramInputReverbLevel, 1),     TRANS ("Input Reverb Level"),    NormalisableRange<float>(0.0,    1.0, 0.0, 0.4), mInputReverbLevel.get(), "", AudioProcessorParameter::genericParameter,
                                          [](float v, int maxlen) -> String { return Decibels::toString(Decibels::gainToDecibels(v), 1); },
                                          [](const String& s) -> float { return Decibels::decibelsToGain(s.getFloatValue()); }),
    std::make_unique<AudioParameterFloat>(ParameterID(paramInputReverbSize, 1),     TRANS ("Input Reverb Size"),    NormalisableRange<float>(0.0, 1.0, 0.0), mInputReverbSize.get(), "", AudioProcessorParameter::genericParameter,
                                          [](float v, int maxlen) -> String { return String((int)(v*100)) + " %"; },
                                          [](const String& s) -> float { return s.getFloatValue()*0.01f; }),
    std::make_unique<AudioParameterFloat>(ParameterID(paramInputReverbDamping, 1),     TRANS ("Input Reverb Damping"),    NormalisableRange<float>(0.0, 1.0, 0.0), mInputReverbDamping.get(), "", AudioProcessorParameter::genericParameter,
                                          [](float v, int maxlen) -> String { return String((int)(v*100)) + " %"; },
                                          [](const String& s) -> float { return s.getFloatValue()*0.01f; }),
    std::make_unique<AudioParameterFloat>(ParameterID(paramInputReverbPreDelay, 1),     TRANS ("Input Reverb Pre-Delay Time"),    NormalisableRange<float>(0.0, 100.0, 1.0, 1.0), mInputReverbPreDelay.get(), "", AudioProcessorParameter::genericParameter,
                                          [](float v, int maxlen) -> String { return String(v, 0) + " ms"; },
                                          [](const String& s) -> float { return s.getFloatValue(); }),
    std::make_unique<AudioParameterBool>(ParameterID(paramSyncMetToFilePlayback, 1), TRANS ("Sync Met to File Playback"), false),

})
{
    mState.addParameterListener (paramInGain, this);
    mState.addParameterListener (paramDry, this);
    mState.addParameterListener (paramWet, this);
    mState.addParameterListener (paramInMonitorMonoPan, this);
    mState.addParameterListener (paramInMonitorPan1, this);
    mState.addParameterListener (paramInMonitorPan2, this);
    mState.addParameterListener (paramDefaultAutoNetbuf, this);
    mState.addParameterListener (paramDefaultNetbufMs, this);
    mState.addParameterListener (paramDefaultSendQual, this);
    mState.addParameterListener (paramMainSendMute, this);
    mState.addParameterListener (paramMainRecvMute, this);
    mState.addParameterListener (paramMetEnabled, this);
    mState.addParameterListener (paramMetGain, this);
    mState.addParameterListener (paramMetTempo, this);
    mState.addParameterListener (paramSendMetAudio, this);
    mState.addParameterListener (paramSendFileAudio, this);
    mState.addParameterListener (paramSendSoundboardAudio, this);
    mState.addParameterListener (paramHearLatencyTest, this);
    mState.addParameterListener (paramMetIsRecorded, this);
    mState.addParameterListener (paramMainReverbEnabled, this);
    mState.addParameterListener (paramMainReverbSize, this);
    mState.addParameterListener (paramMainReverbLevel, this);
    mState.addParameterListener (paramMainReverbDamping, this);
    mState.addParameterListener (paramMainReverbPreDelay, this);
    mState.addParameterListener (paramMainReverbModel, this);
    mState.addParameterListener (paramSendChannels, this);
    mState.addParameterListener (paramDynamicResampling, this);
    mState.addParameterListener (paramAutoReconnectLast, this);
    mState.addParameterListener (paramMainInMute, this);
    mState.addParameterListener (paramMainMonitorSolo, this);
    mState.addParameterListener (paramDefaultPeerLevel, this);
    mState.addParameterListener (paramSyncMetToHost, this);
    mState.addParameterListener (paramSyncMetToFilePlayback, this);
    mState.addParameterListener (paramInputReverbSize, this);
    mState.addParameterListener (paramInputReverbLevel, this);
    mState.addParameterListener (paramInputReverbDamping, this);
    mState.addParameterListener (paramInputReverbPreDelay, this);

    for (int i=0; i < MAX_PEERS; ++i) {
        for (int j=0; j < MAX_PEERS; ++j) {
            mRemoteSendMatrix[i][j] = false;
        }
    }

    // use this to match our main app support dir
    PropertiesFile::Options options;
    options.applicationName     = "SonoBus";
    options.filenameSuffix      = ".xml";
    options.osxLibrarySubFolder = "Application Support/SonoBus";
   #if JUCE_LINUX
    options.folderName          = "~/.config/sonobus";
   #else
    options.folderName          = "";
   #endif

    mSupportDir = options.getDefaultFile().getParentDirectory();

#if (JUCE_IOS)
    mDefaultRecordDir = URL(File::getSpecialLocation (File::userDocumentsDirectory));
    if (mDefaultRecordDir.isLocalFile()) {
        mLastBrowseDir = mDefaultRecordDir.getLocalFile().getFullPathName();
    }
    DBG("Default record dir: " << mDefaultRecordDir.toString(false));
#elif (JUCE_ANDROID)
    //auto parentDir = File::getSpecialLocation (File::userApplicationDataDirectory);
    //parentDir = parentDir.getChildFile("Recordings");
    //mDefaultRecordDir = URL(parentDir);
    //mLastBrowseDir = mDefaultRecordDir.getLocalFile().getFullPathName();
    // LEAVE EMPTY by default
#else
    auto parentDir = File::getSpecialLocation (File::userMusicDirectory);
    parentDir = parentDir.getChildFile("SonoBus");
    mDefaultRecordDir = URL(parentDir);
    mLastBrowseDir = mDefaultRecordDir.getLocalFile().getFullPathName();
#endif


    initFormats();
    
    mDefaultAutoNetbufModeParam = mState.getParameter(paramDefaultAutoNetbuf);
    mDefaultAudioFormatParam = mState.getParameter(paramDefaultSendQual);

    const bool isplugin = !JUCEApplicationBase::isStandaloneApp();
    if (isplugin) {
        // default dry to 1.0 if plugin
        mDry = 1.0;
        mSendChannels = 0; // match inputs default for plugin
        mSyncMetToHost = true;
    } else {
        mDry = 0.0;
        mSyncMetToHost = false;
    }

    mState.getParameter(paramDry)->setValue(mDry.get());
    mState.getParameter(paramSendChannels)->setValue(mState.getParameter(paramSendChannels)->convertTo0to1(mSendChannels.get()));

    mTempoParameter = mState.getParameter(paramMetTempo);
    
    mMetronome = std::make_unique<SonoAudio::Metronome>();
    
    mMetronome->loadBarSoundFromBinaryData(BinaryData::bar_click_wav, BinaryData::bar_click_wavSize);
    mMetronome->loadBeatSoundFromBinaryData(BinaryData::beat_click_wav, BinaryData::beat_click_wavSize);
    mMetronome->setTempo(100.0);
    
    mMainReverb = std::make_unique<Reverb>();
    mMainReverbParams.dryLevel = 0.0f;
    mMainReverbParams.wetLevel = mMainReverbLevel.get() * 0.5f;
    mMainReverbParams.damping = mMainReverbDamping.get();
    mMainReverbParams.roomSize = jmap(mMainReverbSize.get(), 0.55f, 1.0f);
    mMainReverb->setParameters(mMainReverbParams);


    for (int i=0; i < MAX_CHANGROUPS; ++i) {

        // default to all independent
        mInputChannelGroups[i].params.chanStartIndex = i;
        mInputChannelGroups[i].params.numChannels = 1;

        mInputChannelGroups[i].params.setToDefaults(isplugin);
    }

    mMetChannelGroup.params.name = TRANS("Metronome");
    mMetChannelGroup.params.numChannels = 1;

    mRecMetChannelGroup.params.name = TRANS("Metronome");
    mRecMetChannelGroup.params.numChannels = 1;

    mFilePlaybackChannelGroup.params.name = TRANS("File Playback");
    mFilePlaybackChannelGroup.params.numChannels = 2;

    mRecFilePlaybackChannelGroup.params.name = TRANS("File Playback");
    mRecFilePlaybackChannelGroup.params.numChannels = 2;


    mTransportSource.addChangeListener(this);
    
    // audio setup
    mFormatManager.registerBasicFormats();    
    
    initializeAoo();

    if (isplugin) {
        loadDefaultPluginSettings();
    }
    
    loadGlobalState();
}



SonobusAudioProcessor::~SonobusAudioProcessor()
{
    mTransportSource.setSource(nullptr);
    mTransportSource.removeChangeListener(this);

    cleanupAoo();
}

void SonobusAudioProcessor::setUseSpecificUdpPort(int port)
{
    mUseSpecificUdpPort = port;
    
    if (port != 0) {
        if (port != mUdpLocalPort) {
            cleanupAoo();
            initializeAoo(port);
        }
    }
}

void SonobusAudioProcessor::initializeAoo(int udpPort)
{
    int udpport = udpPort; // defaults to letting system choose it  // DEFAULT_UDP_PORT;
    
    // we have both an AOO source and sink
        
    aoo_initialize(nullptr);
    

    const ScopedWriteLock sl (mCoreLock);        

    //mAooSink.reset(aoo::isink::create(1));

    mAooCommonSource = AooSource::create(0, nullptr);


    
    //mAooSink->set_buffersize(mBufferTime.get()); // ms
    //mAooSource->set_buffersize(mBufferTime.get()); // ms
    
    //mAooSink->set_packetsize(1024);
    //mAooSource->set_packetsize(1024);
    

    
    //mUdpSocket = std::make_unique<DatagramSocket>();
    //mUdpSocket->setSendBufferSize(1048576);
    //mUdpSocket->setReceiveBufferSize(1048576);

    



    /*
    int tos_local = 0x38; // QOS realtime DSCP
    int opterr = setsockopt(mUdpSocket->getRawSocketHandle(), IPPROTO_IP, IP_TOS,  &tos_local, sizeof(tos_local));
    if (opterr != 0) {
        DBG("Error setting QOS on socket: " << opterr);
    }
     */

    if (udpport > 0) {
        int attempts = 100;
        while (attempts > 0) {

            int sock = aoo::socket_udp(udpport);

            //if (mUdpSocket->bindToPort(udpport)) {
            //    udpport = mUdpSocket->getBoundPort();
            if (sock >= 0) {
                DBG("Bound udp port to " << udpport);
                mUdpSocketHandle = sock;
                break;
            }
            ++udpport;
            --attempts;
        }

        if (attempts <= 0) {
            DBG("COULD NOT BIND TO PORT!");
            udpport = 0;
        }
    }
    else {
        // system assigned
        int sock = aoo::socket_udp(0);

        //if (!mUdpSocket->bindToPort(0)) {
        if (sock < 0) {
            DBG("Error binding to any udp port!");
        }
        else {
            //udpport = mUdpSocket->getBoundPort();
            mUdpSocketHandle = sock;
        }
    }
    
    if (mUdpSocketHandle >= 0) {
        // increase socket buffers
        const int sendbufsize = 1 << 19; // 512 Kb
        const int recvbufsize = 1 << 20; // 1 MB
        aoo::socket_set_sendbufsize(mUdpSocketHandle, sendbufsize);
        aoo::socket_set_recvbufsize(mUdpSocketHandle, recvbufsize);

        aoo::ip_address addr;
        if (aoo::socket_address(mUdpSocketHandle, addr) != 0){
            DBG("AooNode: couldn't get socket address");
            aoo::socket_close(mUdpSocketHandle);
            mUdpSocketHandle = -1;
            udpport = 0;
        }
        else {
            udpport = addr.port();
            mLocalClientAddress = addr;
        }

    }

    mUdpLocalPort = udpport;

    DBG("Bound udp port to " << mUdpLocalPort);


    //mLocalIPAddress = IPAddress::getLocalAddress();

#if JUCE_IOS    
    auto addresses = IPAddress::getAllInterfaceAddresses (false);
    for (auto& a : addresses) {
        // look for local wifi interface address first
        if (a.first != IPAddress::local(false) && ( a.second == "en0" || a.second == "en1")) {
            mLocalIPAddress = a.first;
            break;
        }        
    }
    if (mLocalIPAddress.isNull()) {
        // now accept anything
        for (auto& a : addresses) {
            // look for local wifi interface address first
            if (a.first != IPAddress::local(false)) {
                mLocalIPAddress = a.first;
                break;
            }
        }        
    }
#else
    auto addresses = IPAddress::getAllAddresses (false);
    for (auto& a : addresses) {
        if (a != IPAddress::local(false)) {
            mLocalIPAddress = a;
            break;
        }
    }
#endif
    
    //mServerEndpoint = std::make_unique<EndpointState>();
    //mServerEndpoint->owner = mUdpSocket.get();
    
    if (mUdpSocketHandle >= 0) {

        mAooClient = AooClient::create(nullptr);
        auto flags = aoo::socket_family(mUdpSocketHandle) == aoo::ip_address::IPv6 ?
                kAooSocketDualStack : kAooSocketIPv4;
        mAooClient->setup(mUdpLocalPort, flags);
        
        mAooClient->setEventHandler(
                                     [](void *user, const AooEvent *event, int32_t level) {
            static_cast<SonobusAudioProcessor*>(user)->handleAooClientEvent(event, level);
        }, this, kAooEventModeCallback);
    }

    
    mSendThread = std::make_unique<SendThread>(*this);
    mRecvThread = std::make_unique<RecvThread>(*this);
    mEventThread = std::make_unique<EventThread>(*this);

    if (mAooClient) {
        mClientThread = std::make_unique<ClientThread>(*this);

#if 0
        mAooClient->addSource(mAooCommonSource.get(), mCurrentUserId);

        mAooCommonSource->setEventHandler(
                                           [](void *user, const AooEvent *event, int32_t level){
            auto * pp = static_cast<SonobusAudioProcessor *>(user);
            pp->handleAooSourceEvent(event, level, mCurrentUserId);
        }, this, kAooEventModeCallback);
#endif
    }
    
    uint32_t estWorkDurationMs = 10; // just a guess
    int rtprio = 1; // all that is necessary

#if JUCE_WINDOWS
    // do not use startRealtimeThread() call because it triggers the whole process to be realtime, which we don't want
    mSendThread->startThread(Thread::Priority::highest);
    mRecvThread->startThread(Thread::Priority::highest);
#else
    if (!mSendThread->startRealtimeThread({ rtprio, estWorkDurationMs }))
    {
        DBG("Send thread failed to start realtime: trying regular");
        mSendThread->startThread(Thread::Priority::highest);
    }

    if (!mRecvThread->startRealtimeThread({ rtprio, estWorkDurationMs }))
    {
        DBG("Recv thread failed to start realtime: trying regular");
        mRecvThread->startThread(Thread::Priority::highest);
    }
#endif

    mEventThread->startThread(Thread::Priority::normal);

    if (mAooClient) {
        mClientThread->startThread();
    }
    
}

void SonobusAudioProcessor::cleanupAoo()
{
    disconnectFromServer();
    
    DBG("waiting on recv thread to die");
    mRecvThread->stopThread(400);
    DBG("waiting on send thread to die");
    mSendThread->stopThread(400);
    DBG("waiting on event thread to die");
    mEventThread->stopThread(400);

    if (mAooClient) {
        auto cb = [](void* x, const AooRequest *request, AooError result,
                     const AooResponse *response) {
            auto obj = (SonobusAudioProcessor *)x;
            if (result == kAooOk){
                DBG("Disconnected");
            } else {
                auto reply = reinterpret_cast<const AooResponseError *>(response);
                if (reply) {
                    DBG("Error disconnecting: " << reply->errorCode << "  msg: " << reply->errorMessage);
                }
            }

        };

        mAooClient->disconnect(cb, this);

        mAooClient->quit();
        mClientThread->stopThread(400);        
    }

    
    {
        const ScopedWriteLock sl (mCoreLock);        

        mAooClient.reset();

        if (mUdpSocketHandle >= 0) {
            aoo::socket_close(mUdpSocketHandle);
        }
        //mUdpSocket.reset();
        
        mAooCommonSource.reset();
        
        mRemotePeers.clear();
        
        mEndpoints.clear();
    }

    stopAooServer();    
}

void SonobusAudioProcessor::startAooServer()
{
    stopAooServer();
    
    {
        const ScopedWriteLock sl (mCoreLock);
        AooError err;

        mAooServerWrapper = std::make_unique<AooServerWrapper>(*this, LOCAL_SERVER_PORT, String(""));
#if 0
        mAooServer = AooServer::create(LOCAL_SERVER_PORT, 0, &err);

        if (mAooServer) {
            mAooServer->setEventHandler(
                                         [](void *user, const AooEvent *event, int32_t level) {
                static_cast<SonobusAudioProcessor*>(user)->handleAooServerEvent(event, level);
            }, this, kAooEventModeCallback);

        }
        else {
            DBG("Error creating Aoo Server: " << err);
        }
#endif
    }

#if 0
    if (mAooServer) {
        mServerThread = std::make_unique<ServerThread>(*this);    
        mServerThread->startThread();
    }
#endif
}
    
void SonobusAudioProcessor::stopAooServer()
{
    if (mAooServerWrapper) {
        mAooServerWrapper.reset();
    }

#if 0
    if (mAooServer) {
        DBG("waiting on recv thread to die");
        mAooServer->quit();
        if (mServerThread) {
            mServerThread->stopThread(400);
        }

        const ScopedWriteLock sl (mCoreLock);
        mAooServer.reset();
    }
#endif
}

bool SonobusAudioProcessor::setCurrentUsername(const String & name)
{
    if (mIsConnectedToServer) return false;

    mCurrentUsername = name;
    return true;
}

bool SonobusAudioProcessor::connectToServer(const String & host, int port, const String & username, const String & passwd)
{
    if (!mAooClient) return false;
    
    // disconnect from everything else, unless we are recovering from server loss
    if (!mRecoveringFromServerLoss) {
        removeAllRemotePeers();
    }
    
    
    //mServerEndpoint->ipaddr = host;
    //mServerEndpoint->port = port;
    //mServerEndpoint->add.reset();

    int token = 0;

    auto cb = [](void* x, const AooRequest *request, AooError result,
                 const AooResponse *response) {
        auto obj = (SonobusAudioProcessor *)x;

        if (result == kAooOk)
        {
            auto resp = reinterpret_cast<const AooResponseConnect *>(response);
            
            auto client_id = resp->clientId;

            obj->mIsConnectedToServer = true;
            obj->mSessionConnectionStamp = Time::getMillisecondCounterHiRes();
            obj->mCurrentClientId = client_id;

            obj->clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientConnected, obj, response->type != kAooRequestError, "");

        } else {
            auto reply = reinterpret_cast<const AooResponseError *>(response);

            obj->mIsConnectedToServer = false;
            obj->mSessionConnectionStamp = 0.0;
            obj->mCurrentClientId = kAooIdInvalid;

            DBG("Error connecting to server: " << reply->errorCode << " msg: " << reply->errorMessage);

            obj->clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientConnected, obj, response->type != kAooRequestError, reply->errorMessage);
        }
    };

    auto retval = mAooClient->connect(host.toRawUTF8(), port, passwd.toRawUTF8(), nullptr, cb, this);

#if 0
    auto cb = [](void *x, AooError result, const void *data){
        auto obj = (SonobusAudioProcessor *)x;
        //auto obj = request->obj;
        //auto group = request->group;
        //auto pwd = request->pwd;

        if (result == kAooOk){
            auto reply = (const AooNetReplyConnect *)data;
            auto user_id = reply->userId;

            obj->mIsConnectedToServer = true;
            obj->mSessionConnectionStamp = Time::getMillisecondCounterHiRes();
            obj->mCurrentUserId = user_id;

            obj->clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientConnected, obj, result == kAooOk, "");
        } else {
            auto reply = (const AooNetReplyError *)data;

            obj->mIsConnectedToServer = false;
            obj->mSessionConnectionStamp = 0.0;
            obj->mCurrentUserId = kAooIdInvalid;

            DBG("Error connecting to server: " << reply->errorCode << " msg: " << reply->errorMessage);

            obj->clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientConnected, obj, result == kAooOk, reply->errorMessage);
        }

    };

    int32_t retval = mAooClient->connect(host.toRawUTF8(), port, username.toRawUTF8(), passwd.toRawUTF8(), cb, this);
#endif

    mCurrentUsername = username;

    if (retval != kAooOk) {
        DBG("Error connecting to server: " << retval);
    }

    return retval == kAooOk;
}

bool SonobusAudioProcessor::isConnectedToServer() const
{
    if (!mAooClient) return false;

    return mIsConnectedToServer;    
}

bool SonobusAudioProcessor::disconnectFromServer()
{
    if (!mAooClient) return false;

    auto cb = [](void* x, const AooRequest *request, AooError result,
                 const AooResponse *response) {
        auto obj = (SonobusAudioProcessor *)x;
        if (result == kAooOk)
        {

        } else {
            auto reply = reinterpret_cast<const AooResponseError *>(response);
            DBG("Error disconnecting to server: " << reply->errorCode << " msg: " << reply->errorMessage);
        }

        // disconnect from everything else!
        obj->removeAllRemotePeers();

        {
            const ScopedLock sl (obj->mClientLock);

               obj->mIsConnectedToServer = false;
               obj->mSessionConnectionStamp = 0.0;

               obj->mCurrentJoinedGroup.clear();
        }

        {
            const ScopedLock sl (obj->mPublicGroupsLock);

               obj->mPublicGroupInfos.clear();
        }
    };

    mAooClient->disconnect(cb, this);



    return true;
}

void SonobusAudioProcessor::addRecentServerConnectionInfo(const AooServerConnectionInfo & cinfo)
{
    const ScopedLock sl (mRecentsLock);

    // look for existing match, and update only timestamp if found, otherwise add to end
    int index = mRecentConnectionInfos.indexOf(cinfo);
    if (index >= 0) {
        // already there
        mRecentConnectionInfos.getReference(index).timestamp = cinfo.timestamp;
    } else {
        // add it
        mRecentConnectionInfos.add(cinfo);
    }
    // resort
    mRecentConnectionInfos.sort();
    
    // limit to 10
    if (mRecentConnectionInfos.size() > 10) {
        mRecentConnectionInfos.removeRange(10, mRecentConnectionInfos.size() - 10);
    }
}

int SonobusAudioProcessor::getRecentServerConnectionInfos(Array<AooServerConnectionInfo> & retarray)
{
    const ScopedLock sl (mRecentsLock);
    retarray = mRecentConnectionInfos;
    return retarray.size();
}

void SonobusAudioProcessor::clearRecentServerConnectionInfos()
{
    const ScopedLock sl (mRecentsLock);
    mRecentConnectionInfos.clear();
}

void SonobusAudioProcessor::removeRecentServerConnectionInfo(int index)
{
    const ScopedLock sl (mRecentsLock);
    if (index < mRecentConnectionInfos.size()) {
        mRecentConnectionInfos.remove(index);
    }
}

int SonobusAudioProcessor::getPublicGroupInfos(Array<AooPublicGroupInfo> & retarray)
{
    retarray.clearQuick();

    const ScopedLock sl (mPublicGroupsLock);
    for (auto & item : mPublicGroupInfos) {
        retarray.add(item.second);
    }
    return retarray.size();
}



void SonobusAudioProcessor::setAutoconnectToGroupPeers(bool flag)
{
    mAutoconnectGroupPeers = flag;
}


bool SonobusAudioProcessor::setWatchPublicGroups(bool flag)
{
    if (!mAooClient) return false;

    mWatchPublicGroups = flag;

    JUCE_COMPILER_WARNING ("IMPLEMENT PUBLIC GROUPS");

    int32_t retval = 0; // mAooClient->group_watch_public(flag);
    const ScopedLock sl (mPublicGroupsLock);

    mPublicGroupInfos.clear();


    if (retval < 0) {
        DBG("Error watching public groups: " << retval);
    }

    return retval >= 0;

}

struct GroupRequest {
    SonobusAudioProcessor * obj;
    String group;
    bool   ispublic;
};

bool SonobusAudioProcessor::setupCommonAooSource()
{
    const int32_t userid = mCurrentUserId;
    
    mAooCommonSource->setId(userid);
    mAooClient->addSource(mAooCommonSource.get(), userid);
    
    mAooCommonSource->setEventHandler(
                                      [](void *user, const AooEvent *event, int32_t level){
                                          auto * pp = static_cast<SonobusAudioProcessor *>(user);
                                          AooId aid(kAooIdInvalid);
                                          pp->mAooCommonSource->getId(aid);
                                          pp->handleAooSourceEvent(event, level, aid);
                                      }, this, kAooEventModeCallback);
    
}

bool SonobusAudioProcessor::joinServerGroup(const String & group, const String & groupsecret, const String & username, const String & userpass, bool isPublic)
{
    if (!mAooClient) return false;

    auto cb = [](void* x, const AooRequest *request, AooError result,
                 const AooResponse* response) {

        auto grreq = (GroupRequest *)x;
        auto obj = grreq->obj;
        auto group = grreq->group;
        std::string errmsg;

        if (result == kAooOk) {
            DBG("Joined group - " << group);
            auto r = (const AooResponseGroupJoin *)response;
            const ScopedLock sl (obj->mClientLock);
            obj->mCurrentJoinedGroup = group; //CharPointer_UTF8 (e->name);
            obj->mCurrentJoinedGroupId = r->groupId;
            // TODO grab more of the group info
            obj->mCurrentUserId = r->userId;

            obj->setupCommonAooSource();
            
            obj->mSessionConnectionStamp = Time::getMillisecondCounterHiRes();
        } else {
            //t_error_reply error { reply->error_code, reply->error_message };
            auto reply = reinterpret_cast<const AooResponseError *>(response);
            errmsg = reply ? reply->errorMessage : "";
            DBG("Error joining group " << group << " : " << errmsg);

        }

        obj->clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientGroupJoined, obj, response->type != kAooRequestError, group, errmsg);

        delete grreq;
    };

    // need to add PUBLIC
    auto retval = mAooClient->joinGroup(group.toRawUTF8(), groupsecret.toRawUTF8(), nullptr,
                                            username.toRawUTF8(), userpass.toRawUTF8(), nullptr, nullptr,
                                            cb, new GroupRequest { this, group, isPublic });



    if (retval < 0) {
        DBG("Error joining group " << group << " : " << retval);
    }
    
    return retval >= 0;
}

bool SonobusAudioProcessor::leaveServerGroup(const String & group)
{
    if (!mAooClient) return false;

    auto cb = [](void* x, const AooRequest *request, AooError result,
                 const AooResponse* response) {
        auto grreq = (GroupRequest *)x;
        auto obj = grreq->obj;
        auto group = grreq->group;
        std::string errmsg;

        if (result == kAooOk) {
            DBG("Group leave - " << group);

            const ScopedLock sl (obj->mClientLock);
            obj->mCurrentJoinedGroup.clear();
            obj->mCurrentJoinedGroupId = kAooIdInvalid;
            obj->mCurrentUserId = kAooIdInvalid;

            obj->mAooClient->removeSource(obj->mAooCommonSource.get());

            // assume they are all part of the group, XXX
            obj->removeAllRemotePeers();

        } else {
            auto reply = reinterpret_cast<const AooResponseError *>(response);
            errmsg = (reply ? reply->errorMessage : "");
            //t_error_reply error { reply->error_code, reply->error_message };
            DBG("Error leaving group " << group << " : " << errmsg);

        }

        obj->clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientGroupLeft, obj, response->type != kAooRequestError, group, errmsg);

        delete grreq;
    };

    // for now we only connect to one group at a time, so just leave the current one

    int32_t retval = mAooClient->leaveGroup(mCurrentJoinedGroupId, cb, new GroupRequest { this, group, false });

    if (retval < 0) {
        DBG("Error leaving group " << group << " : " << retval);
    }

    const ScopedLock sl (mClientLock);        
    
    if (mCurrentJoinedGroup == group) {
        mCurrentJoinedGroup.clear();
    }
    
    return retval >= 0;
}

String SonobusAudioProcessor::getCurrentJoinedGroup() const { 
    const ScopedLock sl (mClientLock);        
    return mCurrentJoinedGroup;     
}

void SonobusAudioProcessor::AudioCodecFormatInfo::computeName()
{
    if (codec == SonobusAudioProcessor::CodecOpus) {
        name = String::formatted("%d kbps/ch", bitrate/1000);
    }
    else {
        if (bitdepth == 2) {
            name = "PCM 16 bit";
        }
        else if (bitdepth == 3) {
            name = "PCM 24 bit";
        }
        else if (bitdepth == 4) {
            name = "PCM 32 bit float";
        }
        else if (bitdepth == 8) {
            name = "PCM 64 bit float";
        }
    }
}

void SonobusAudioProcessor::initFormats()
{
    mAudioFormats.clear();
    
    // low bandwidth to high
    //mAudioFormats.add(AudioCodecFormatInfo(8000, 10, OPUS_SIGNAL_MUSIC, 960));
    mAudioFormats.add(AudioCodecFormatInfo(16000, 10, OPUS_SIGNAL_MUSIC, 960));
    mAudioFormats.add(AudioCodecFormatInfo(24000, 10, OPUS_SIGNAL_MUSIC, 480));
    mAudioFormats.add(AudioCodecFormatInfo(48000, 10, OPUS_SIGNAL_MUSIC, 240));
    mAudioFormats.add(AudioCodecFormatInfo(64000, 10, OPUS_SIGNAL_MUSIC, 240));
    mAudioFormats.add(AudioCodecFormatInfo(96000, 10, OPUS_SIGNAL_MUSIC, 120));
    mAudioFormats.add(AudioCodecFormatInfo(128000, 10, OPUS_SIGNAL_MUSIC, 120));
    mAudioFormats.add(AudioCodecFormatInfo(160000, 10, OPUS_SIGNAL_MUSIC, 120));
    mAudioFormats.add(AudioCodecFormatInfo(256000, 10, OPUS_SIGNAL_MUSIC, 120));
    
    mAudioFormats.add(AudioCodecFormatInfo(2));
    mAudioFormats.add(AudioCodecFormatInfo(3));
    mAudioFormats.add(AudioCodecFormatInfo(4));
    //mAudioFormats.add(AudioCodecFormatInfo(CodecPCM, 8)); // insanity!

    mDefaultAudioFormatIndex = 4; // 96kpbs/ch Opus
}

int SonobusAudioProcessor::findFormatIndex(SonobusAudioProcessor::AudioCodecFormatCodec codec, int bitrate, int bitdepth)
{
    for (int i=0; i < mAudioFormats.size(); ++i) {
        const auto & format = mAudioFormats.getReference(i);
        if (format.codec == codec) {
            if (format.codec == CodecOpus) {
                if (format.bitrate == bitrate) {
                    return i;
                }
            }
            else if (bitdepth == format.bitdepth){
                return i;
            }
        }
    }
    
    return -1;
}


String SonobusAudioProcessor::getAudioCodeFormatName(int formatIndex) const
{
    if (formatIndex >= mAudioFormats.size() || formatIndex < 0) return "";
    
    const AudioCodecFormatInfo & info = mAudioFormats.getReference(formatIndex);
    return info.name;    
}

bool SonobusAudioProcessor::getAudioCodeFormatInfo(int formatIndex, AudioCodecFormatInfo & retinfo) const
{
    if (formatIndex >= mAudioFormats.size() || formatIndex < 0) return false;
    retinfo = mAudioFormats.getReference(formatIndex);
    return true;    
}


void SonobusAudioProcessor::setDefaultAudioCodecFormat(int formatIndex)
{
    if (formatIndex < mAudioFormats.size() && formatIndex >= 0) {
        mDefaultAudioFormatIndex = formatIndex;
        mDefaultAudioFormatParam->setValueNotifyingHost(mDefaultAudioFormatParam->convertTo0to1(mDefaultAudioFormatIndex));
    }
    
}


void SonobusAudioProcessor::setDefaultAutoresizeBufferMode(AutoNetBufferMode flag)
{
    defaultAutoNetbufMode = flag;
    mDefaultAutoNetbufModeParam->setValueNotifyingHost(mDefaultAutoNetbufModeParam->convertTo0to1(defaultAutoNetbufMode));

}


void SonobusAudioProcessor::setRemotePeerAudioCodecFormat(int index, int formatIndex)
{
    if (formatIndex >= mAudioFormats.size() || index >= mRemotePeers.size()) return;
    
    const AudioCodecFormatInfo & info = mAudioFormats.getReference(formatIndex);

    const ScopedReadLock sl (mCoreLock);        
 
    auto remote = mRemotePeers.getUnchecked(index);
    remote->formatIndex = formatIndex;
    
    if (remote->oursource) {
        setupSourceFormat(remote, remote->oursource.get());
        remote->oursource->setup(remote->sendChannels, getSampleRate(), currSamplesPerBlock, 0);
        //remote->oursource->setup(getSampleRate(), remote->packetsize    , getTotalNumOutputChannels());        
        
        remote->latencyDirty = true;
    }
}

int SonobusAudioProcessor::getRemotePeerAudioCodecFormat(int index) const
{
    if (index >= mRemotePeers.size()) return -1;
    
    const ScopedReadLock sl (mCoreLock);        
    auto remote = mRemotePeers.getUnchecked(index);
    return remote->formatIndex;
}

bool SonobusAudioProcessor::getRemotePeerReceiveAudioCodecFormat(int index, AudioCodecFormatInfo & retinfo) const
{
    if (index >= mRemotePeers.size()) return false;
    
    const ScopedReadLock sl (mCoreLock);
    auto remote = mRemotePeers.getUnchecked(index);
    retinfo = remote->recvFormat;
    return true;
}

bool SonobusAudioProcessor::setRequestRemotePeerSendAudioCodecFormat(int index, int formatIndex)
{
    if (formatIndex >= mAudioFormats.size() || index >= mRemotePeers.size()) return false;
    

    const ScopedReadLock sl (mCoreLock);
    auto remote = mRemotePeers.getUnchecked(index);

    AooFormatStorage fmt;
    
    
    if (formatIndex >= 0) {
        const AudioCodecFormatInfo & info = mAudioFormats.getReference(formatIndex);

        if (formatInfoToAooFormat(info, remote->recvChannels, fmt)) {
            JUCE_COMPILER_WARNING ("IMPLEMENT SOURCE CODEC CHANGE");
            //remote->oursink->request_source_codec_change(remote->endpoint, remote->remoteSourceId, fmt.header);

            remote->reqRemoteSendFormatIndex = formatIndex; 
            return true;
        }
        else {
            return false;
        }
    } else {
        remote->reqRemoteSendFormatIndex = -1; // no preference
        return true;
    }
}

int SonobusAudioProcessor::getRequestRemotePeerSendAudioCodecFormat(int index) const
{
    if (index >= mRemotePeers.size()) return -1;
    
    const ScopedReadLock sl (mCoreLock);
    auto remote = mRemotePeers.getUnchecked(index);

    return remote->reqRemoteSendFormatIndex;    
}

int SonobusAudioProcessor::getRemotePeerOrderPriority(int index) const
{
    if (index >= mRemotePeers.size()) return -1;
    const ScopedReadLock sl (mCoreLock);
    auto remote = mRemotePeers.getUnchecked(index);
    return remote->orderPriority;
}

void SonobusAudioProcessor::setRemotePeerOrderPriority(int index, int priority)
{
    if (index >= mRemotePeers.size()) return;

    const ScopedReadLock sl (mCoreLock);

    auto remote = mRemotePeers.getUnchecked(index);
    remote->orderPriority = priority;
}


int SonobusAudioProcessor::getRemotePeerSendPacketsize(int index) const
{
    if (index >= mRemotePeers.size()) return -1;
    const ScopedReadLock sl (mCoreLock);        
    auto remote = mRemotePeers.getUnchecked(index);
    return remote->packetsize;
}

void SonobusAudioProcessor::setRemotePeerSendPacketsize(int index, int psize)
{
    if (index >= mRemotePeers.size()) return;
    
    const ScopedReadLock sl (mCoreLock);        

    DBG("Setting packet size to " << psize);
    
    auto remote = mRemotePeers.getUnchecked(index);
    remote->packetsize = psize;
    
    if (remote->oursource) {
        //setupSourceFormat(remote, remote->oursource.get());

        //remote->oursource->setup(getSampleRate(), remote->packetsize, getTotalNumInputChannels());

        remote->oursource->setPacketSize(remote->packetsize);
    }
    
}

void SonobusAudioProcessor::setRemotePeerCompressorParams(int index, int changroup, CompressorParams & params)
{
    if (index >= mRemotePeers.size()) return;
    
    const ScopedReadLock sl (mCoreLock);        

    auto remote = mRemotePeers.getUnchecked(index);

    // sanity check and compute automakeupgain if necessary    
    params.ratio = jlimit(1.0f, 120.0f, params.ratio);
    if (params.automakeupGain) {
        // makeupgain = (-thresh -  abs(thresh/ratio))/2
        params.makeupGainDb = (-params.thresholdDb - abs(params.thresholdDb/params.ratio)) * 0.5f;
    }

    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        remote->chanGroups[changroup].params.compressorParams = params;
        remote->chanGroups[changroup].compressorParamsChanged = true;
    }
}

bool SonobusAudioProcessor::getRemotePeerCompressorParams(int index, int changroup, CompressorParams & retparams)
{
    if (index >= mRemotePeers.size()) return false;
    const ScopedReadLock sl (mCoreLock);        
    auto remote = mRemotePeers.getUnchecked(index);
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        retparams = remote->chanGroups[changroup].params.compressorParams;
        return true;
    }
    return false;

}

void SonobusAudioProcessor::setRemotePeerExpanderParams(int index, int changroup, SonoAudio::CompressorParams & params)
{
    if (index >= mRemotePeers.size()) return;

    const ScopedReadLock sl (mCoreLock);

    auto remote = mRemotePeers.getUnchecked(index);

    // sanity check and compute automakeupgain if necessary
    params.ratio = jlimit(1.0f, 120.0f, params.ratio);

    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        remote->chanGroups[changroup].params.expanderParams = params;
        remote->chanGroups[changroup].expanderParamsChanged = true;
    }
}

bool SonobusAudioProcessor::getRemotePeerExpanderParams(int index, int changroup, SonoAudio::CompressorParams & retparams)\
{
    if (index >= mRemotePeers.size()) return false;
    const ScopedReadLock sl (mCoreLock);
    auto remote = mRemotePeers.getUnchecked(index);
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        retparams = remote->chanGroups[changroup].params.expanderParams;
        return true;
    }
    return false;
}

void SonobusAudioProcessor::setRemotePeerEqParams(int index, int changroup, SonoAudio::ParametricEqParams & params)
{
    if (index >= mRemotePeers.size()) return;

    const ScopedReadLock sl (mCoreLock);

    auto remote = mRemotePeers.getUnchecked(index);

    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        remote->chanGroups[changroup].params.eqParams = params;
        remote->chanGroups[changroup].eqParamsChanged = true;
    }
}

bool SonobusAudioProcessor::getRemotePeerEqParams(int index, int changroup, SonoAudio::ParametricEqParams & retparams)
{
    if (index >= mRemotePeers.size()) return false;
    const ScopedReadLock sl (mCoreLock);
    auto remote = mRemotePeers.getUnchecked(index);
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        retparams = remote->chanGroups[changroup].params.eqParams;
        return true;
    }
    return false;
}

bool SonobusAudioProcessor::getRemotePeerEffectsActive(int index, int changroup)
{
    if (index >= mRemotePeers.size()) return false;
    const ScopedReadLock sl (mCoreLock);
    auto remote = mRemotePeers.getUnchecked(index);
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        return remote->chanGroups[changroup].params.compressorParams.enabled || remote->chanGroups[changroup].params.expanderParams.enabled || remote->chanGroups[changroup].params.eqParams.enabled || remote->chanGroups[changroup].params.invertPolarity || (remote->chanGroups[changroup].params.monReverbSend > 0.0f);
    }
    return false;


}


void SonobusAudioProcessor::setMetronomeMonitorDelayParams(SonoAudio::DelayParams & params)
{
    mMetChannelGroup.params.monitorDelayParams = params;
    //mInputChannelGroups[changroup].monitorDelayParamsChanged = true;
    // commit them now
    mMetChannelGroup.commitMonitorDelayParams();

    mRecMetChannelGroup.params.monitorDelayParams = params;
    mRecMetChannelGroup.commitMonitorDelayParams();
}

bool SonobusAudioProcessor::getMetronomeMonitorDelayParams(SonoAudio::DelayParams & retparams)
{
    retparams = mMetChannelGroup.params.monitorDelayParams;
    return true;
}

void SonobusAudioProcessor::setMetronomeChannelDestStartAndCount(int start, int count)
{
    mMetChannelGroup.params.monDestStartIndex = start;
    mMetChannelGroup.params.monDestChannels = std::max(1, std::min(count, MAX_CHANNELS));
    mMetChannelGroup.commitMonitorDelayParams(); // need to do this too

    mRecMetChannelGroup.params.monDestStartIndex = start;
    mRecMetChannelGroup.params.monDestChannels = std::max(1, std::min(count, MAX_CHANNELS));
    mRecMetChannelGroup.commitMonitorDelayParams(); // need to do this too
}

bool SonobusAudioProcessor::getMetronomeChannelDestStartAndCount(int & retstart, int & retcount)
{
    retstart = mMetChannelGroup.params.monDestStartIndex;
    retcount = mMetChannelGroup.params.monDestChannels;
    return true;
}


void SonobusAudioProcessor::setMetronomePan(float pan)
{
    mMetChannelGroup.params.pan[0] = pan;
    mRecMetChannelGroup.params.pan[0] = pan;
}

float SonobusAudioProcessor::getMetronomePan() const
{
    return mMetChannelGroup.params.pan[0];
}

void SonobusAudioProcessor::setMetronomeGain(float gain)
{
    mState.getParameter(paramMetGain)->setValueNotifyingHost(mState.getParameter(paramMetGain)->convertTo0to1(gain));
}

float SonobusAudioProcessor::getMetronomeGain() const
{
    return mMetGain.get();
}

void SonobusAudioProcessor::setMetronomeMonitor(float mgain)
{
    mMetChannelGroup.params.monitor = mgain;
}

float SonobusAudioProcessor::getMetronomeMonitor() const
{
    return mMetChannelGroup.params.monitor;
}



void SonobusAudioProcessor::setFilePlaybackMonitorDelayParams(SonoAudio::DelayParams & params)
{
    mFilePlaybackChannelGroup.params.monitorDelayParams = params;
    //mInputChannelGroups[changroup].monitorDelayParamsChanged = true;
    // commit them now
    mFilePlaybackChannelGroup.commitMonitorDelayParams();

    mRecFilePlaybackChannelGroup.params.monitorDelayParams = params;
    mRecFilePlaybackChannelGroup.commitMonitorDelayParams();

}

bool SonobusAudioProcessor::getFilePlaybackMonitorDelayParams(SonoAudio::DelayParams & retparams)
{
    retparams = mFilePlaybackChannelGroup.params.monitorDelayParams;
    return true;
}

void SonobusAudioProcessor::setFilePlaybackDestStartAndCount(int start, int count)
{
    mFilePlaybackChannelGroup.params.monDestStartIndex = start;
    mFilePlaybackChannelGroup.params.monDestChannels = std::max(1, std::min(count, MAX_CHANNELS));
    mFilePlaybackChannelGroup.commitMonitorDelayParams(); // need to do this too

    mRecFilePlaybackChannelGroup.params.monDestStartIndex = start;
    mRecFilePlaybackChannelGroup.params.monDestChannels = std::max(1, std::min(count, MAX_CHANNELS));
    mRecFilePlaybackChannelGroup.commitMonitorDelayParams(); // need to do this too
}

bool SonobusAudioProcessor::getFilePlaybackDestStartAndCount(int & retstart, int & retcount)
{
    retstart = mFilePlaybackChannelGroup.params.monDestStartIndex;
    retcount = mFilePlaybackChannelGroup.params.monDestChannels;
    return true;
}

void SonobusAudioProcessor::setFilePlaybackGain(float gain)
{
    mFilePlaybackChannelGroup.params.gain = gain;
    mRecFilePlaybackChannelGroup.params.gain = gain;
    //mTransportSource.setGain(gain);
}

float SonobusAudioProcessor::getFilePlaybackGain() const
{
    return mFilePlaybackChannelGroup.params.gain;
    //return mTransportSource.getGain();
}

void SonobusAudioProcessor::setFilePlaybackMonitor(float mgain)
{
    mFilePlaybackChannelGroup.params.monitor = mgain;
    mRecFilePlaybackChannelGroup.params.monitor = mgain;
}

float SonobusAudioProcessor::getFilePlaybackMonitor() const
{
    return mFilePlaybackChannelGroup.params.monitor;
}


void SonobusAudioProcessor::setInputCompressorParams(int changroup, CompressorParams & params)
{
    // sanity check and compute automakeupgain if necessary    
    params.ratio = jlimit(1.0f, 120.0f, params.ratio);
    if (params.automakeupGain) {
        // makeupgain = (-thresh -  abs(thresh/ratio))/2
        params.makeupGainDb = (-params.thresholdDb - abs(params.thresholdDb/params.ratio)) * 0.5f;
    }

    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        mInputChannelGroups[changroup].params.compressorParams = params;
        mInputChannelGroups[changroup].compressorParamsChanged = true;
    }
}

bool SonobusAudioProcessor::getInputCompressorParams(int changroup, CompressorParams & retparams)
{
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        retparams = mInputChannelGroups[changroup].params.compressorParams;
        return true;
    }
    return false;
}

void SonobusAudioProcessor::setInputLimiterParams(int changroup, CompressorParams & params)
{
    // sanity check and compute automakeupgain if necessary    
    params.ratio = jlimit(1.0f, 120.0f, params.ratio);

    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        mInputChannelGroups[changroup].params.limiterParams = params;
        mInputChannelGroups[changroup].limiterParamsChanged = true;
    }
}

bool SonobusAudioProcessor::getInputLimiterParams(int changroup, CompressorParams & retparams)
{
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        retparams = mInputChannelGroups[changroup].params.limiterParams;
        return true;
    }
    return false;
}

void SonobusAudioProcessor::setInputMonitorDelayParams(int changroup, SonoAudio::DelayParams & params)
{
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        mInputChannelGroups[changroup].params.monitorDelayParams = params;
        //mInputChannelGroups[changroup].monitorDelayParamsChanged = true;
        // commit them now
        mInputChannelGroups[changroup].commitMonitorDelayParams();
    }
}

bool SonobusAudioProcessor::getInputMonitorDelayParams(int changroup, SonoAudio::DelayParams & retparams)
{
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        retparams = mInputChannelGroups[changroup].params.monitorDelayParams;
        return true;
    }
    return false;
}


void SonobusAudioProcessor::setInputExpanderParams(int changroup, CompressorParams & params)
{
    // sanity check and compute automakeupgain if necessary    
    params.ratio = jlimit(1.0f, 120.0f, params.ratio);

    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        mInputChannelGroups[changroup].params.expanderParams = params;
        mInputChannelGroups[changroup].expanderParamsChanged = true;
    }
}

bool SonobusAudioProcessor::getInputExpanderParams(int changroup, CompressorParams & retparams)
{
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        retparams = mInputChannelGroups[changroup].params.expanderParams;
        return true;
    }
    return false;
}

void SonobusAudioProcessor::setInputEqParams(int changroup, ParametricEqParams & params)
{
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        mInputChannelGroups[changroup].params.eqParams = params;
        mInputChannelGroups[changroup].eqParamsChanged = true;
    }
}

bool SonobusAudioProcessor::getInputEqParams(int changroup, ParametricEqParams & retparams)
{
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        retparams = mInputChannelGroups[changroup].params.eqParams;
        return true;
    }
    return false;
}


void SonobusAudioProcessor::setInputGroupCount(int count)
{
    int newcnt = std::max(0, std::min(count, MAX_CHANGROUPS-1));

    mInputChannelGroupCount = newcnt;
}

void SonobusAudioProcessor::setInputGroupChannelStartAndCount(int changroup, int start, int count)
{
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        mInputChannelGroups[changroup].params.chanStartIndex = start;
        mInputChannelGroups[changroup].params.numChannels = std::max(1, std::min(count, MAX_CHANNELS));
        mInputChannelGroups[changroup].commitMonitorDelayParams();
    }
}

bool SonobusAudioProcessor::getInputGroupChannelStartAndCount(int changroup, int & retstart, int & retcount)
{
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        retstart = mInputChannelGroups[changroup].params.chanStartIndex;
        retcount = mInputChannelGroups[changroup].params.numChannels;
        return true;
    }
    return false;
}

void SonobusAudioProcessor::setInputGroupChannelDestStartAndCount(int changroup, int start, int count)
{
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        mInputChannelGroups[changroup].params.monDestStartIndex = start;
        mInputChannelGroups[changroup].params.monDestChannels = std::max(1, std::min(count, MAX_CHANNELS));
    }
}

bool SonobusAudioProcessor::getInputGroupChannelDestStartAndCount(int changroup, int & retstart, int & retcount)
{
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        retstart = mInputChannelGroups[changroup].params.monDestStartIndex;
        retcount = mInputChannelGroups[changroup].params.monDestChannels;
        return true;
    }
    return false;
}


bool SonobusAudioProcessor::insertInputChannelGroup(int atgroup, int chstart, int chcount)
{
    if (atgroup >= 0 && atgroup < MAX_CHANGROUPS) {
        // push any existing ones down

        for (auto i = MAX_CHANGROUPS-1; i > atgroup; --i) {
            mInputChannelGroups[i].copyParametersFrom(mInputChannelGroups[i-1]);
            //mInputChannelGroups[i].chanStartIndex += chcount;
            //mInputChannelGroups[i-1].numChannels = std::max(1, std::min(count, MAX_CHANNELS));
        }
        mInputChannelGroups[atgroup].params.chanStartIndex = chstart;
        mInputChannelGroups[atgroup].params.numChannels = std::max(1, std::min(chcount, MAX_CHANNELS));
        mInputChannelGroups[atgroup].params.monDestStartIndex = jmax(0, jmin(2 * (chstart / 2), getTotalNumOutputChannels()-1));
        mInputChannelGroups[atgroup].params.monDestChannels = std::max(1, std::min(2, getTotalNumOutputChannels() - mInputChannelGroups[atgroup].params.monDestStartIndex));

        // todo some defaults
        mInputChannelGroups[atgroup].commitMonitorDelayParams(); // need to do this too

        return true;
    }
    return false;
}

bool SonobusAudioProcessor::removeInputChannelGroup(int atgroup)
{
    if (atgroup >= 0 && atgroup < MAX_CHANGROUPS) {
        // move any existing ones after up

        for (auto i = atgroup + 1; i < MAX_CHANGROUPS; ++i) {
            mInputChannelGroups[i-1].copyParametersFrom(mInputChannelGroups[i]);
            //mInputChannelGroups[i].chanStartIndex += chcount;
            //mInputChannelGroups[i-1].numChannels = std::max(1, std::min(count, MAX_CHANNELS));
        }
        return true;
    }
    return false;
}

bool SonobusAudioProcessor::moveInputChannelGroupTo(int atgroup, int togroup)
{
    if (atgroup == togroup || atgroup < 0 || atgroup >= MAX_CHANGROUPS || togroup < 0 || togroup >= MAX_CHANGROUPS) {
        return false;
    }

    // takes group atgroup and rearranges settings
    insertInputChannelGroup(togroup, mInputChannelGroups[atgroup].params.chanStartIndex, mInputChannelGroups[atgroup].params.numChannels);
    int origgroup = atgroup < togroup ? atgroup : atgroup+1;
    // copy from origgroup to new atgroup
    mInputChannelGroups[togroup].copyParametersFrom(mInputChannelGroups[origgroup]);

    // remove origgroup
    removeInputChannelGroup(origgroup);

    return true;
}



void SonobusAudioProcessor::setInputGroupName(int changroup, const String & name)
{
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        mInputChannelGroups[changroup].params.name = name;
    }
}

String SonobusAudioProcessor::getInputGroupName(int changroup)
{
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        return mInputChannelGroups[changroup].params.name;
    }
    return "";
}

void SonobusAudioProcessor::setInputChannelPan(int changroup, int chan, float pan)
{
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        if (chan >= 0 && chan < MAX_CHANNELS) {
            if (mInputChannelGroups[changroup].params.numChannels == 2 && chan < 2) {
                mInputChannelGroups[changroup].params.panStereo[chan] = pan;
            } else {
                mInputChannelGroups[changroup].params.pan[chan] = pan;
            }
        }
    }
}

float SonobusAudioProcessor::getInputChannelPan(int changroup, int chan)
{
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        if (chan >= 0 && chan < MAX_CHANNELS) {
            if (mInputChannelGroups[changroup].params.numChannels == 2 && chan < 2) {
                return mInputChannelGroups[changroup].params.panStereo[chan];
            } else {
                return mInputChannelGroups[changroup].params.pan[chan];
            }
        }
    }

    return 0.0f;
}

void SonobusAudioProcessor::setInputGroupGain(int changroup, float gain)
{
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        mInputChannelGroups[changroup].params.gain = gain;
    }
}

float SonobusAudioProcessor::getInputGroupGain(int changroup)
{
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        return mInputChannelGroups[changroup].params.gain;
    }
    return 0.0f;
}

void SonobusAudioProcessor::setInputMonitor(int changroup, float mgain)
{
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        mInputChannelGroups[changroup].params.monitor = mgain;
    }
}

float SonobusAudioProcessor::getInputMonitor(int changroup)
{
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        return mInputChannelGroups[changroup].params.monitor;
    }
    return 0.0f;
}

void SonobusAudioProcessor::setInputGroupMuted(int changroup, bool muted)
{
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        mInputChannelGroups[changroup].params.muted = muted;
    }
}

bool SonobusAudioProcessor::getInputGroupMuted(int changroup)
{
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        return mInputChannelGroups[changroup].params.muted;
    }
    return false;
}

void SonobusAudioProcessor::setInputGroupSoloed(int changroup, bool soloed)
{
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        mInputChannelGroups[changroup].params.soloed = soloed;
    }
}

bool SonobusAudioProcessor::getInputGroupSoloed(int changroup)
{
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        return mInputChannelGroups[changroup].params.soloed;
    }
    return false;
}



void SonobusAudioProcessor::setInputReverbSend(int changroup, float rgain, bool input)
{
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        if (input) {
            mInputChannelGroups[changroup].params.inReverbSend = rgain;
        } else {
            mInputChannelGroups[changroup].params.monReverbSend = rgain;
        }
    }
}

float SonobusAudioProcessor::getInputReverbSend(int changroup, bool input)
{
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        if (input) {
            return mInputChannelGroups[changroup].params.inReverbSend;
        } else {
            return mInputChannelGroups[changroup].params.monReverbSend;
        }
    }
    return 0.0f;
}

void SonobusAudioProcessor::setInputPolarityInvert(int changroup, bool invert)
{
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        mInputChannelGroups[changroup].params.invertPolarity = invert;
    }
}

bool SonobusAudioProcessor::getInputPolarityInvert(int changroup)
{
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        return mInputChannelGroups[changroup].params.invertPolarity;
    }
    return false;
}



void SonobusAudioProcessor::commitCompressorParams(RemotePeer * peer, int changroup)
{   
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        peer->chanGroups[changroup].commitCompressorParams();
    }
}

void SonobusAudioProcessor::commitInputCompressorParams(int changroup)
{   
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        mInputChannelGroups[changroup].commitCompressorParams();
    }
}

void SonobusAudioProcessor::commitInputExpanderParams(int changroup)
{
   
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        //mInputCompressorControl.setParamValue("/compressor/Bypass", mInputCompressorParams.enabled ? 0.0f : 1.0f);
        mInputChannelGroups[changroup].commitExpanderParams();
    }

}

void SonobusAudioProcessor::commitInputLimiterParams(int changroup)
{   
    //mInputLimiterControl.setParamValue("/limiter/Bypass", mInputLimiterParams.enabled ? 0.0f : 1.0f);
    //mInputLimiterControl.setParamValue("/limiter/threshold", mInputLimiterParams.thresholdDb);
    //mInputLimiterControl.setParamValue("/limiter/ratio", mInputLimiterParams.ratio);
    //mInputLimiterControl.setParamValue("/limiter/attack", mInputLimiterParams.attackMs * 1e-3);
    //mInputLimiterControl.setParamValue("/limiter/release", mInputLimiterParams.releaseMs * 1e-3);

    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        mInputChannelGroups[changroup].commitLimiterParams();
    }

}


void SonobusAudioProcessor::commitInputEqParams(int changroup)
{
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        mInputChannelGroups[changroup].commitEqParams();
    }
}

void SonobusAudioProcessor::commitInputMonitoringParams(int changroup)
{
    if (changroup >= 0 && changroup < MAX_CHANGROUPS) {
        mInputChannelGroups[changroup].commitMonitorDelayParams();
    }
}



// should use shared pointer
foleys::LevelMeterSource * SonobusAudioProcessor::getRemotePeerRecvMeterSource(int index)
{
    if (index >= mRemotePeers.size()) return nullptr;
    const ScopedReadLock sl (mCoreLock);        
    auto remote = mRemotePeers.getUnchecked(index);
    return &(remote->recvMeterSource);
}

foleys::LevelMeterSource * SonobusAudioProcessor::getRemotePeerSendMeterSource(int index)
{
    if (index >= mRemotePeers.size()) return nullptr;
    const ScopedReadLock sl (mCoreLock);        
    auto remote = mRemotePeers.getUnchecked(index);
    return &(remote->sendMeterSource);
}




SonobusAudioProcessor::EndpointState * SonobusAudioProcessor::findOrAddRawEndpoint(const void * rawaddr, int addrlen)
{
    aoo::ip_address addr((const struct sockaddr *)rawaddr, addrlen);

    return findOrAddEndpoint(addr);
}


SonobusAudioProcessor::EndpointState * SonobusAudioProcessor::findOrAddEndpoint(const String & host, int port)
{
    aoo::ip_address addr(host.toStdString(), port, aoo::ip_address::Unspec);

    return findOrAddEndpoint(addr);
}

SonobusAudioProcessor::EndpointState * SonobusAudioProcessor::findOrAddEndpoint(const aoo::ip_address & ipaddr)
{
    EndpointState * endpoint = findEndpoint(ipaddr);

    if (!endpoint) {
        // add it as new
        const ScopedLock sl (mEndpointsLock);
        endpoint = mEndpoints.add(new EndpointState(ipaddr));
        endpoint->ownerSocket = mUdpSocketHandle;
        DBG("Added new endpoint for " << ipaddr.name_unmapped() << ":" << ipaddr.port());
    }
    return endpoint;
}

SonobusAudioProcessor::EndpointState * SonobusAudioProcessor::findEndpoint(const aoo::ip_address & ipaddr)
{
    const ScopedLock sl (mEndpointsLock);

    EndpointState * endpoint = nullptr;

    for (auto ep : mEndpoints) {
        if (ep->address == ipaddr) {
            endpoint = ep;
            break;
        }
    }

    return endpoint;
}

SonobusAudioProcessor::EndpointState * SonobusAudioProcessor::findOrAddEndpoint(AooId groupid, AooId userid)
{
    const ScopedLock sl (mEndpointsLock);

    EndpointState * endpoint = nullptr;
    if (groupid == kAooIdInvalid || userid == kAooIdInvalid) return nullptr;

    for (auto ep : mEndpoints) {
        if (ep->groupid == groupid && ep->userid == userid) {
            endpoint = ep;
            break;
        }
    }

    return endpoint;
}


void SonobusAudioProcessor::updateSafetyMuting(RemotePeer * peer)
{
    // assumed corelock already held
    //const float droprate =  (peer->dataPacketsDropped) / ((nowtime - peer->resetDroptime)*1e-3);
    const float droprate = peer->fastDropRate.xbar;
    const float safetyunmutethreshrate = 2.0f;
    const float safetyunmutethreshmintime = 0.5f;
    const float safetyunmutethreshtime = 0.75f;
    const float jitterbufthresh = 15.0f;

    double nowtime = Time::getMillisecondCounterHiRes();
    double timesincereset = (nowtime - peer->resetDroptime) * 1e-3;
    double deltadroptime = peer->lastDroptime > 0 ? (nowtime - peer->lastDroptime) * 1e-3 : timesincereset;


    if ((timesincereset > safetyunmutethreshmintime)
        && ((droprate > 0.0f && droprate < safetyunmutethreshrate)
            || (droprate == 0.0f && deltadroptime > safetyunmutethreshtime)
            || (peer->buffertimeMs > jitterbufthresh))) {
        DBG("Droprate: " << droprate << "  deltatime: " << deltadroptime << " buftimems: " << peer->buffertimeMs);
        DBG("Unmuting after reset drop");
        peer->resetSafetyMuted = false;
    }

    peer->fastDropRate.Z *= 0.965;

    float realdroprate =  (peer->dataPacketsDropped - peer->lastDropCount) / deltadroptime;
    peer->fastDropRate.push(realdroprate);

}

void SonobusAudioProcessor::doReceiveData()
{
    // receive from udp port, and parse packet
    AooByte buf[AOO_MAX_PACKET_SIZE];

    //int nbytes = mUdpSocket->read(buf, AOO_MAXPACKETSIZE, false, senderIP, senderPort);

    aoo::ip_address addr;
    int32_t addrlen = aoo::ip_address::max_length;
    double timeoutsec = 0.02; // 20 ms
    //int nbytes = mUdpSocket->read(buf, AOO_MAXPACKETSIZE, false, addr.address_ptr(), addrlen);
    int nbytes = socket_receive(mUdpSocketHandle, buf, AOO_MAX_PACKET_SIZE, &addr, timeoutsec);

    if (nbytes == 0) return; // timeout
    else if (nbytes < 0) {
        DBG("Error receiving UDP");
        return;
    }

    //*addr.length_ptr() = addrlen;

    // find endpoint from sender info
    EndpointState * endpoint = findOrAddEndpoint(addr);
    
    endpoint->recvBytes += nbytes + UDP_OVERHEAD_BYTES;

    // TODO - handle possible OSC bundles
    bool aoohandled = false;

    if (mAooClient) {
        // AoO message
        const ScopedReadLock sl (mCoreLock);

        aoohandled = mAooClient->handleMessage(buf, nbytes, addr.address(), addr.length()) == kAooOk;

        if (aoohandled) {

            if (auto * remote = findRemotePeer(endpoint, -1)) {
                // todo - remote->dataPacketsReceived += 1;
                //if (remote->recvAllow && !remote->recvActive) {
                //    remote->recvActive = true;
                //}
                if (remote->resetSafetyMuted) {
                    updateSafetyMuting(remote);
                }
            }

            // notify send thread
            notifySendThread();
        }
    }

    if (!aoohandled) {
        if (handleOtherMessage(endpoint, buf, nbytes)) {

        }
        else {
            // not a valid AoO OSC message
            DBG("SonoBus: not a valid AOO message! : " << buf[0] << buf[1] << buf[2] << buf[3]);
        }
    }

}

// XXX
// all of this should be refactored into its own class

#define SONOBUS_MSG_DOMAIN "/sb"
#define SONOBUS_MSG_DOMAIN_LEN 3

#define SONOBUS_MSG_PEERINFO "/pinfo"
#define SONOBUS_MSG_PEERINFO_LEN 6
#define SONOBUS_FULLMSG_PEERINFO SONOBUS_MSG_DOMAIN SONOBUS_MSG_PEERINFO

#define SONOBUS_MSG_LAYOUTINFO "/clayinfo"
#define SONOBUS_MSG_LAYOUTINFO_LEN 9
#define SONOBUS_FULLMSG_LAYOUTINFO SONOBUS_MSG_DOMAIN SONOBUS_MSG_LAYOUTINFO

#define SONOBUS_MSG_CHAT "/chat"
#define SONOBUS_MSG_CHAT_LEN 5
#define SONOBUS_FULLMSG_CHAT SONOBUS_MSG_DOMAIN SONOBUS_MSG_CHAT

#define SONOBUS_MSG_PING "/ping"
#define SONOBUS_MSG_PING_LEN 5
#define SONOBUS_FULLMSG_PING SONOBUS_MSG_DOMAIN SONOBUS_MSG_PING

#define SONOBUS_MSG_PINGACK "/pngack"
#define SONOBUS_MSG_PINGACK_LEN 7
#define SONOBUS_FULLMSG_PINGACK SONOBUS_MSG_DOMAIN SONOBUS_MSG_PINGACK

#define SONOBUS_MSG_REQLATINFO "/reqlatinfo"
#define SONOBUS_MSG_REQLATINFO_LEN 12
#define SONOBUS_FULLMSG_REQLATINFO SONOBUS_MSG_DOMAIN SONOBUS_MSG_REQLATINFO

#define SONOBUS_MSG_LATINFO "/latinfo"
#define SONOBUS_MSG_LATINFO_LEN 8
#define SONOBUS_FULLMSG_LATINFO SONOBUS_MSG_DOMAIN SONOBUS_MSG_LATINFO

#define SONOBUS_MSG_SUGGESTLAT "/suggestlat"
#define SONOBUS_MSG_SUGGESTLAT_LEN 11
#define SONOBUS_FULLMSG_SUGGESTLAT SONOBUS_MSG_DOMAIN SONOBUS_MSG_SUGGESTLAT

#define SONOBUS_MSG_BLOCKEDINFO "/blockedinfo"
#define SONOBUS_MSG_BLOCKEDINFO_LEN 13
#define SONOBUS_FULLMSG_BLOCKEDINFO SONOBUS_MSG_DOMAIN SONOBUS_MSG_BLOCKEDINFO


enum {
    SONOBUS_MSGTYPE_UNKNOWN = 0,
    SONOBUS_MSGTYPE_PEERINFO,
    SONOBUS_MSGTYPE_LAYOUTINFO,
    SONOBUS_MSGTYPE_CHAT,
    SONOBUS_MSGTYPE_PING,
    SONOBUS_MSGTYPE_PINGACK,
    SONOBUS_MSGTYPE_REQLATINFO,
    SONOBUS_MSGTYPE_LATINFO,
    SONOBUS_MSGTYPE_SUGGESTLAT,
    SONOBUS_MSGTYPE_BLOCKEDINFO
};

static int32_t sonobusOscParsePattern(const AooByte *msg, int32_t n, int32_t & rettype)
{
    int32_t offset = 0;
    if (n >= SONOBUS_MSG_DOMAIN_LEN
        && !memcmp(msg, SONOBUS_MSG_DOMAIN, SONOBUS_MSG_DOMAIN_LEN))
    {
        offset += SONOBUS_MSG_DOMAIN_LEN;

        if (n >= (offset + SONOBUS_MSG_PING_LEN)
            && !memcmp(msg + offset, SONOBUS_MSG_PING, SONOBUS_MSG_PING_LEN))
        {
            rettype = SONOBUS_MSGTYPE_PING;
            offset += SONOBUS_MSG_PING_LEN;
            return offset;
        }
        else if (n >= (offset + SONOBUS_MSG_PINGACK_LEN)
            && !memcmp(msg + offset, SONOBUS_MSG_PINGACK, SONOBUS_MSG_PINGACK_LEN))
        {
            rettype = SONOBUS_MSGTYPE_PINGACK;
            offset += SONOBUS_MSG_PINGACK_LEN;
            return offset;
        }
        else if (n >= (offset + SONOBUS_MSG_PEERINFO_LEN)
            && !memcmp(msg + offset, SONOBUS_MSG_PEERINFO, SONOBUS_MSG_PEERINFO_LEN))
        {
            rettype = SONOBUS_MSGTYPE_PEERINFO;
            offset += SONOBUS_MSG_PEERINFO_LEN;
            return offset;
        }
        else if (n >= (offset + SONOBUS_MSG_LAYOUTINFO_LEN)
            && !memcmp(msg + offset, SONOBUS_MSG_LAYOUTINFO, SONOBUS_MSG_LAYOUTINFO_LEN))
        {
            rettype = SONOBUS_MSGTYPE_LAYOUTINFO;
            offset += SONOBUS_MSG_LAYOUTINFO_LEN;
            return offset;
        }
        else if (n >= (offset + SONOBUS_MSG_CHAT_LEN)
            && !memcmp(msg + offset, SONOBUS_MSG_CHAT, SONOBUS_MSG_CHAT_LEN))
        {
            rettype = SONOBUS_MSGTYPE_CHAT;
            offset += SONOBUS_MSG_CHAT_LEN;
            return offset;
        }
        else if (n >= (offset + SONOBUS_MSG_REQLATINFO_LEN)
            && !memcmp(msg + offset, SONOBUS_MSG_REQLATINFO, SONOBUS_MSG_REQLATINFO_LEN))
        {
            rettype = SONOBUS_MSGTYPE_REQLATINFO;
            offset += SONOBUS_MSG_REQLATINFO_LEN;
            return offset;
        }
        else if (n >= (offset + SONOBUS_MSG_LATINFO_LEN)
            && !memcmp(msg + offset, SONOBUS_MSG_LATINFO, SONOBUS_MSG_LATINFO_LEN))
        {
            rettype = SONOBUS_MSGTYPE_LATINFO;
            offset += SONOBUS_MSG_LATINFO_LEN;
            return offset;
        }
        else if (n >= (offset + SONOBUS_MSG_SUGGESTLAT_LEN)
            && !memcmp(msg + offset, SONOBUS_MSG_SUGGESTLAT, SONOBUS_MSG_SUGGESTLAT_LEN))
        {
            rettype = SONOBUS_MSGTYPE_SUGGESTLAT;
            offset += SONOBUS_MSG_SUGGESTLAT_LEN;
            return offset;
        }
        else if (n >= (offset + SONOBUS_MSG_BLOCKEDINFO_LEN)
            && !memcmp(msg + offset, SONOBUS_MSG_BLOCKEDINFO, SONOBUS_MSG_BLOCKEDINFO_LEN))
        {
            rettype = SONOBUS_MSGTYPE_BLOCKEDINFO;
            offset += SONOBUS_MSG_BLOCKEDINFO_LEN;
            return offset;
        }
        else {
            return 0;
        }
    }
    return 0;
}

bool SonobusAudioProcessor::handleOtherMessage(EndpointState * endpoint, const AooByte *msg, int32_t n)
{
    // try to parse it as an OSC /sb  message
    int32_t type = SONOBUS_MSGTYPE_UNKNOWN;
    int32_t onset = 0;

    if (! (onset = sonobusOscParsePattern(msg, n, type))) {
        return false;
    }

    try {
        osc::ReceivedPacket packet((const char*)msg, n);
        osc::ReceivedMessage message(packet);

        if (type == SONOBUS_MSGTYPE_PING) {
            // received from the other side
            // args: t:origtime

            auto it = message.ArgumentsBegin();
            auto tt = (it++)->AsTimeTag();

            // now prepare and send ack immediately
            auto tt2 = aoo::time_tag::now(); // use real system time

            char buf[AOO_MAX_PACKET_SIZE];
            osc::OutboundPacketStream outmsg(buf, sizeof(buf));

            try {
                outmsg << osc::BeginMessage(SONOBUS_FULLMSG_PINGACK)
                << osc::TimeTag(tt) << osc::TimeTag(tt2)
                << osc::EndMessage;
            }
            catch (const osc::Exception& e){
                DBG("exception in pingack message constructions: " << e.what());
                return false;
            }

            if (mAooClient) {

                RemotePeer * peer = findRemotePeer(endpoint, -1);
                if (!peer) {
                    DBG("Peerinfo: Could not find peer for endpoint: " << endpoint->ipaddr << " port: " << endpoint->port);
                    return false;
                }

                // have to use group/user?
                AooData msg { kAooDataOSC, (AooByte*)outmsg.Data(), (AooSize) outmsg.Size() };
                
                mAooClient->sendMessage(peer->groupId, peer->userId, msg, 0, 0);

                //mAooClient->sendPeerMessage( {kAooDataTypeOSC, (AooByte*)outmsg.Data(), (AooInt32) outmsg.Size() }, endpoint->address.address(), endpoint->address.length(), 0);
            } else {
                endpoint_send(endpoint, (AooByte*)outmsg.Data(), (int) outmsg.Size());
            }

            DBG("Received ping from " << endpoint->ipaddr << ":" << endpoint->port << "  stamp: " << tt);

        }
        else if (type == SONOBUS_MSGTYPE_PINGACK) {
            // received from the other side
            // args: t:origtime t:theirtime

            auto it = message.ArgumentsBegin();
            auto tt = (it++)->AsTimeTag();
            auto tt2 = (it++)->AsTimeTag();
            auto tt3 = aoo::time_tag::now(); // use real system time

            handlePingEvent(endpoint, tt, tt2, tt3); // jlc

        }
        else if (type == SONOBUS_MSGTYPE_PEERINFO) {
            // peerinfo message arguments:
            // blob containing JSON
            auto it = message.ArgumentsBegin();

            const void *infojson;
            osc::osc_bundle_element_size_t size;

            (it++)->AsBlob(infojson, size);

            String jsonstr = String::createStringFromData(infojson, size);

            juce::var infodata;
            auto result = juce::JSON::parse(jsonstr, infodata);
            if (result.failed()) {
                DBG("Peerinfo Json parsing failed: " << result.getErrorMessage());
                return false;
            }

            {
                const ScopedReadLock sl (mCoreLock);

                // find remote peer
                RemotePeer * peer = findRemotePeer(endpoint, -1);
                if (!peer) {
                    DBG("Peerinfo: Could not find peer for endpoint: " << endpoint->ipaddr << " port: " << endpoint->port);
                    return false;
                }

                handleRemotePeerInfoUpdate(peer, infodata);
            }

        }
        else if (type == SONOBUS_MSGTYPE_LAYOUTINFO) {
            // layout info message arguments:
            // i:sourceid  b:<blob containing valuetree in binary form>
            auto it = message.ArgumentsBegin();
            auto sourceid = (it++)->AsInt32();

            const void *info;
            osc::osc_bundle_element_size_t size;

            (it++)->AsBlob(info, size);
            // jlc

            ValueTree tree = ValueTree::readFromData (info, size);

            if (!tree.isValid()) {
                DBG("layoutinfo parsing failed ");
                return false;
            }
            else {
                DBG("Got layoutinfo");
            }

            bool changed = false;

            {
                const ScopedReadLock sl (mCoreLock);

                // find remote peer
                RemotePeer * peer = findRemotePeer(endpoint, sourceid);
                if (!peer) {
                    DBG("layoutinfo: Could not find peer for endpoint: " << endpoint->ipaddr << " src: " <<  sourceid);
                }
                else {
                    peer->recvdChanLayout = true;
                    applyLayoutFormatToPeer(peer, tree);
                    changed = true;
                }
            }

            if (changed) {
                clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientPeerChangedState, this, "format");
            }


        }
        else if (type == SONOBUS_MSGTYPE_CHAT) {
            // layout info message arguments:
            // s:groupname s:from s:targets s:tags s:message
            auto it = message.ArgumentsBegin();

            String group (CharPointer_UTF8((it++)->AsString()));
            String from (CharPointer_UTF8((it++)->AsString()));
            String targets (CharPointer_UTF8((it++)->AsString()));
            String tags (CharPointer_UTF8((it++)->AsString()));
            String message (CharPointer_UTF8((it++)->AsString()));

            SBChatEvent chatevent(SBChatEvent::UserType, group, from, targets, tags, message);
            
            if (!isAddressBlocked(endpoint->ipaddr)) {
                
                mAllChatEvents.add(chatevent);
                clientListeners.call(&SonobusAudioProcessor::ClientListener::sbChatEventReceived, this, chatevent);
            }

        }
        else if (type == SONOBUS_MSGTYPE_REQLATINFO) {
            // received from the other side
            // args: none

            if (!isAddressBlocked(endpoint->ipaddr)) {

                
                auto latinfo = getAllLatInfo();
                
                char buf[AOO_MAX_PACKET_SIZE];
                osc::OutboundPacketStream outmsg(buf, sizeof(buf));
                
                String jsonstr = JSON::toString(latinfo, true, 6);
                
                if (jsonstr.getNumBytesAsUTF8() > AOO_MAX_PACKET_SIZE - 100) {
                    DBG("Info too big for packet!");
                    return false;
                }
                
                
                try {
                    outmsg << osc::BeginMessage(SONOBUS_FULLMSG_LATINFO)
                    << osc::Blob(jsonstr.toRawUTF8(), (int) jsonstr.getNumBytesAsUTF8())
                    << osc::EndMessage;
                }
                catch (const osc::Exception& e){
                    DBG("exception in reqlat message constructions: " << e.what());
                    return false;
                }
                
                endpoint_send(endpoint, (AooByte*)outmsg.Data(), (int) outmsg.Size());
                
                DBG("Received REQLAT from " << endpoint->ipaddr << ":" << endpoint->port);
            }

        }
        else if (type == SONOBUS_MSGTYPE_LATINFO) {
            // peerinfo message arguments:
            // blob containing JSON
            auto it = message.ArgumentsBegin();

            const void *infojson;
            osc::osc_bundle_element_size_t size;

            (it++)->AsBlob(infojson, size);

            String jsonstr = String::createStringFromData(infojson, size);

            juce::var infodata;
            auto result = juce::JSON::parse(jsonstr, infodata);
            if (result.failed()) {
                DBG("Peerinfo Json parsing failed: " << result.getErrorMessage());
                return false;
            }

            handleLatInfo(infodata);

        }
        else if (type == SONOBUS_MSGTYPE_SUGGESTLAT) {
            // received from the other side
            // args: s:username  f:latency

            auto it = message.ArgumentsBegin();
            auto username = (it++)->AsString();
            auto latency = (it++)->AsFloat();

            if (!isAddressBlocked(endpoint->ipaddr)) {
                clientListeners.call(&SonobusAudioProcessor::ClientListener::peerRequestedLatencyMatch, this, username, latency);
            }
        }
        else if (type == SONOBUS_MSGTYPE_BLOCKEDINFO) {
            // received from the other side when they have blocked/unblocked us
            // args: s:username  b:blocked

            auto it = message.ArgumentsBegin();
            auto username = (it++)->AsString();
            auto blocked = (it++)->AsBool();

            {
                const ScopedReadLock sl (mCoreLock);

                // find remote peer
                RemotePeer * peer = findRemotePeer(endpoint, -1);
                if (!peer) {
                    DBG("Could not find peer recv blockinfo for endpoint: " << endpoint->ipaddr);
                }
                else {
                    peer->blockedUs = blocked;
                    DBG("Remote peer " << username << " set blocked = " << (int) blocked);
                    
                    int ind = 0;
                    int retind = -1;
                    for (auto s : mRemotePeers) {
                        if (s->endpoint == endpoint) {
                            retind = ind;
                            break;
                        }
                        ++ind;
                    }
                    
                    if (retind >= 0) {
                        setRemotePeerSendActive(retind, false);
                    }
                }
            }
            
            clientListeners.call(&SonobusAudioProcessor::ClientListener::peerBlockedInfoChanged, this, username, blocked);
        }
        return true;
    } catch (const osc::Exception& e){
        DBG("exception in handleOtherMessage: " << e.what());
    }
    return false;
}


bool SonobusAudioProcessor::sendChatEvent(const SBChatEvent & event)
{
    // /sb/chat s:groupname s:from s:targets s:tags s:message

    // if peerindex < 0 - send to all peers
    char buf[AOO_MAX_PACKET_SIZE];
    osc::OutboundPacketStream msg(buf, sizeof(buf));

    try {

        msg << osc::BeginMessage(SONOBUS_FULLMSG_CHAT)
        << event.group.toRawUTF8()
        << event.from.toRawUTF8()
        << event.targets.toRawUTF8()
        << event.tags.toRawUTF8()
        << event.message.toRawUTF8()
        << osc::EndMessage;

    }
    catch (const osc::Exception& e){
        DBG("exception in chat message constructions: " << e.what());
        return false;
    }

    StringArray targetnames = StringArray::fromTokens(event.targets, "|", "");


    const ScopedReadLock sl (mCoreLock);
    for (int i=0;  i < mRemotePeers.size(); ++i) {
        auto * peer = mRemotePeers.getUnchecked(i);

        if (!targetnames.isEmpty() && !targetnames.contains(peer->userName))
            continue;

        DBG("Sending chat message to " << i);
        // TODO have to use group/user!?
        //mAooClient->sendMessage(group, user, {kAooDataTypeOSC, (AooByte*)outmsg.Data(), (AooInt32) outmsg.Size() }, 0, 0);

        this->sendPeerMessage(peer, (AooByte*) msg.Data(), (int32_t) msg.Size());

    }

    return true;
}

void SonobusAudioProcessor::handleLatInfo(const juce::var & infolist)
{
    const ScopedLock sl (mLatInfoLock);

    // received a latinfo list from elsewhere, add it to our list
    // todo remove any duplicates

    if (!infolist.isArray()) return;

    for (int i=0; i < infolist.size(); ++i) {
        auto infodata = infolist[i];

        LatInfo latinfo;
        latinfo.sourceName = infodata.getProperty("srcname", "");
        latinfo.destName = infodata.getProperty("destname", "");
        latinfo.latencyMs = infodata.getProperty("latms", 0.0f);

        if (latinfo.sourceName.isNotEmpty() && latinfo.destName.isNotEmpty()) {
            mLatInfoList.add(latinfo);
        }

        DBG("latinfo: srcname: " << latinfo.sourceName << "  dest: " << latinfo.destName << "  latms: " << latinfo.latencyMs);
    }
}

juce::var SonobusAudioProcessor::getAllLatInfo()
{
    juce::var infolist;

    const ScopedReadLock sl (mCoreLock);

    for (int i=0;  i < mRemotePeers.size(); ++i) {
        auto * peer = mRemotePeers.getUnchecked(i);
        if (!peer) continue;
        DynamicObject::Ptr item = new DynamicObject(); // this will delete itself
        LatencyInfo latinfo;
        getRemotePeerLatencyInfo(i, latinfo);

        item->setProperty("srcname", peer->userName);
        item->setProperty("destname", mCurrentUsername);
        item->setProperty("latms", latinfo.incomingMs);

        infolist.append(item.get());
    }

    return infolist;
}

void SonobusAudioProcessor::sendBlockedInfoMessage(EndpointState *endpoint, bool blocked)
{
    char buf[AOO_MAX_PACKET_SIZE];
    osc::OutboundPacketStream outmsg(buf, sizeof(buf));

    try {

        outmsg << osc::BeginMessage(SONOBUS_FULLMSG_BLOCKEDINFO)
        << mCurrentUsername.toRawUTF8()
        << blocked
        << osc::EndMessage;

    }
    catch (const osc::Exception& e){
        DBG("exception in blockedinfo message constructions: " << e.what());
        return;
    }
    
    endpoint_send(endpoint, (AooByte*) outmsg.Data(), (int) outmsg.Size());
}


void SonobusAudioProcessor::sendReqLatInfoToAll()
{
    char buf[AOO_MAX_PACKET_SIZE];
    osc::OutboundPacketStream msg(buf, sizeof(buf));

    try {

        msg << osc::BeginMessage(SONOBUS_FULLMSG_REQLATINFO)
        << osc::EndMessage;

    }
    catch (const osc::Exception& e){
        DBG("exception in reqlat message constructions: " << e.what());
        return;
    }

    const ScopedReadLock sl (mCoreLock);
    for (int i=0;  i < mRemotePeers.size(); ++i) {
        auto * peer = mRemotePeers.getUnchecked(i);

        DBG("Sending reqlat message to " << i);
        this->sendPeerMessage(peer, (AooByte*) msg.Data(), (int32_t) msg.Size());
    }
}

void SonobusAudioProcessor::sendLatencyMatchToAll(float latency)
{
    // suggest to all that they should adjust all receiving latencies to be this value

    char buf[AOO_MAX_PACKET_SIZE];
    osc::OutboundPacketStream msg(buf, sizeof(buf));

    try {

        msg << osc::BeginMessage(SONOBUS_FULLMSG_SUGGESTLAT)
        << mCurrentUsername.toRawUTF8()
        << latency
        << osc::EndMessage;

    }
    catch (const osc::Exception& e){
        DBG("exception in suggestlat message constructions: " << e.what());
        return;
    }

    const ScopedReadLock sl (mCoreLock);
    for (int i=0;  i < mRemotePeers.size(); ++i) {
        auto * peer = mRemotePeers.getUnchecked(i);

        DBG("Sending suggestlat: " << latency << " message to " << i);
        this->sendPeerMessage(peer, (AooByte*)msg.Data(), (int32_t) msg.Size());
    }

}


void SonobusAudioProcessor::beginLatencyMatchProcedure()
{
    {
        const ScopedLock sl (mLatInfoLock);
        mLatInfoList.clearQuick();

        // add our own info
        handleLatInfo(getAllLatInfo());
    }

    sendReqLatInfoToAll();
}

bool SonobusAudioProcessor::isLatencyMatchProcedureReady()
{
    // we expect an entry for every directional path in the mesh
    const ScopedLock sl (mLatInfoLock);
    int totpeers = mRemotePeers.size() + 1;
    int predictedSize =  (totpeers*(totpeers - 1));

    return mLatInfoList.size() >= predictedSize;
}

void SonobusAudioProcessor::getLatencyInfoList(Array<LatInfo> & retlist)
{
    const ScopedLock sl (mLatInfoLock);
    retlist.addArray(mLatInfoList);
}

void SonobusAudioProcessor::commitLatencyMatch(float latency)
{
    // Adjust the jitter buffer for each peer with extra values to pad up to the requested latency

    const ScopedReadLock sl (mCoreLock);
    for (int i=0;  i < mRemotePeers.size(); ++i) {
        auto * peer = mRemotePeers.getUnchecked(i);

        float pingms= peer->smoothPingTime.xbar;
        const auto absizeMs = 1e3*currSamplesPerBlock/getSampleRate();
        float basebuftimeMs = jmax((double) (peer->netBufAutoBaseline > 0.0 ? peer->netBufAutoBaseline : peer->buffertimeMs), absizeMs);
        auto halfping = pingms*0.5f;
        auto recvcodecLat = peer->recvFormat.codec == CodecOpus ? 2.5f : 0.0f; // Opus adds codec latency

        auto baseline = /*absizeMs + */ recvcodecLat +  peer->remoteInLatMs + halfping + basebuftimeMs;

        if (baseline < latency) {
            // we can add some padding
            auto padms =  (latency - baseline);
            auto newbuftime = basebuftimeMs + padms;
            DBG("Adding " << padms << " ms of padding for peer: " << i);
            setRemotePeerBufferTime(i, newbuftime);
            // now set the auto baseline
        }
        peer->latencyMatched = true;

    }
}



void SonobusAudioProcessor::handleRemotePeerInfoUpdate(RemotePeer * peer, const juce::var & infodata)
{
    // core read lock already held

    // infodata should contain useful info

    DBG("peerinfo: Handle remote peerinfo update ");

    if (infodata.hasProperty("jitbuf")) {
        float jitbufms = infodata.getProperty("jitbuf", 0.0f);
        DBG("peerinfo: Got remote jitter buffer: " << jitbufms);
        peer->remoteJitterBufMs = jitbufms;
    }
    if (infodata.hasProperty("inlat")) {
        float latms = infodata.getProperty("inlat", 0.0f);
        DBG("peerinfo: Got remote input latency: " << latms);
        peer->remoteInLatMs = latms;
    }
    if (infodata.hasProperty("outlat")) {
        float latms = infodata.getProperty("outlat", 0.0f);
        DBG("peerinfo: Got remote input latency: " << latms);
        peer->remoteOutLatMs = latms;
    }
    if (infodata.hasProperty("nettype")) {
        int nettype = infodata.getProperty("nettype", (int) RemoteNetTypeUnknown);
        DBG("peerinfo: Got remote net type: " << nettype);
        peer->remoteNetType = nettype;
    }
    if (infodata.hasProperty("rec")) {
        bool isrec = infodata.getProperty("rec", false);
        DBG("peerinfo: Got remote recording: " << (int)isrec);
        peer->remoteIsRecording = isrec;
    }

    peer->hasRemoteInfo = true;

}

void SonobusAudioProcessor::sendRemotePeerInfoUpdate(int index, RemotePeer * topeer)
{
    // send our info to this remote peer
    DynamicObject::Ptr info = new DynamicObject(); // this will delete itself

    // not great, better than nothing - TODO make this accurate
    info->setProperty("inlat", 1e3 * currSamplesPerBlock / getSampleRate());
    info->setProperty("outlat", 1e3 * currSamplesPerBlock / getSampleRate());
    info->setProperty("rec", isRecordingToFile());

    // nettype TODO

    char buf[AOO_MAX_PACKET_SIZE];

    const ScopedReadLock sl (mCoreLock);
    for (int i=0;  i < mRemotePeers.size(); ++i) {
        auto * peer = mRemotePeers.getUnchecked(i);
        if (topeer && topeer != peer) continue;
        if (index >= 0 && index != i) continue;

        osc::OutboundPacketStream msg(buf, sizeof(buf));

        auto buftimeMs = jmax((double)peer->buffertimeMs, 1e3 * currSamplesPerBlock / getSampleRate());
        info->setProperty("jitbuf", buftimeMs);

        String jsonstr = JSON::toString(info.get(), true, 6);

        if (jsonstr.getNumBytesAsUTF8() > AOO_MAX_PACKET_SIZE - 100) {
            DBG("Info too big for packet!");
            return;
        }

        try {
            msg << osc::BeginMessage(SONOBUS_FULLMSG_PEERINFO)
            << osc::Blob(jsonstr.toRawUTF8(), (int) jsonstr.getNumBytesAsUTF8())
            << osc::EndMessage;
        }
        catch (const osc::Exception& e){
            DBG("exception in PEERINFO message constructions: " << e.what());
            continue;
        }

        DBG("Sending peerinfo message to peer id " << peer->userId);
        this->sendPeerMessage(peer, (AooByte*)msg.Data(), (int32_t) msg.Size());

        if (index == i || topeer == peer) break;
    }

}


int32_t SonobusAudioProcessor::sendPeerMessage(RemotePeer * peer, const AooByte *msg, int32_t n)
{
    if (mAooClient) {
        mAooClient->sendMessage(peer->groupId, peer->userId, {kAooDataOSC, msg, (AooSize)n }, 0, 0);

        //mAooClient->sendPeerMessage(msg, n, peer->endpoint->address.address(), peer->endpoint->address.length(), 0);
    } else {
        return endpoint_send(peer->endpoint, msg, n);
    }
}




void SonobusAudioProcessor::doSendData()
{
    // just try to send for everybody
    const ScopedReadLock sl (mCoreLock);        

    // send stuff until there is nothing left to send
    
    int32_t didsomething = 1;

    auto nowtimems = Time::getMillisecondCounterHiRes();


    if (mAooClient) {
        mAooClient->send(udpsend, this);
    }


    for (auto & remote : mRemotePeers) {
        if ( nowtimems > (remote->lastSendPingTimeMs + PEER_PING_INTERVAL_MS) ) {
            sendPingEvent(remote);
            remote->lastSendPingTimeMs = nowtimems;
            if (!remote->haveSentFirstPeerInfo) {
                sendRemotePeerInfoUpdate(-1, remote);
                remote->haveSentFirstPeerInfo = true;
            }
        }
    }

#if 0
    while (didsomething) {
        //mAooSource->send();
        didsomething = 0;
        
        didsomething |= mAooDummySource->send();
        
        if (mAooClient) {
            mAooClient->send();
        }
        
        for (auto & remote : mRemotePeers) {
            if (remote->oursource) {
                didsomething |= remote->oursource->send();
                
                if (didsomething) {
                    remote->dataPacketsSent += 1;
                }
            }
            if (remote->oursink) {
                didsomething |= remote->oursink->send();
            }

            if ( nowtimems > (remote->lastSendPingTimeMs + PEER_PING_INTERVAL_MS) ) {
                sendPingEvent(remote);
                remote->lastSendPingTimeMs = nowtimems;
                if (!remote->haveSentFirstPeerInfo) {
                    sendRemotePeerInfoUpdate(-1, remote);
                    remote->haveSentFirstPeerInfo = true;
                }
            }
        }
    }
#endif


    if (mPendingUnmute.get() && mPendingUnmuteAtStamp < Time::getMillisecondCounter() ) {
        DBG("UNMUTING ALL");
        mState.getParameter(paramMainRecvMute)->setValueNotifyingHost(0.0f);

        mPendingUnmute = false;
    }

    if (mNeedsSampleSetup.get()) {
        DBG("Doing sample setup for all");
        setupSourceFormatsForAll();
        mNeedsSampleSetup = false;

        // reset all incoming by toggling muting
        if (!mMainRecvMute.get()) {
            DBG("Toggling main recv mute");
            mState.getParameter(paramMainRecvMute)->setValueNotifyingHost(1.0f);
            mPendingUnmuteAtStamp = Time::getMillisecondCounter() + 250;
            mPendingUnmute = true;
        }

        sendRemotePeerInfoUpdate(-1); // send to all

    }


}


#if 0
static int32_t gHandleSourceEvents(void * user, const aoo_event ** events, int32_t n)
{
    ProcessorIdPair * pp = static_cast<ProcessorIdPair*> (user);
    return pp->processor->handleSourceEvents(events, n, pp->id);    
}

static int32_t gHandleSinkEvents(void * user, const aoo_event ** events, int32_t n)
{
    ProcessorIdPair * pp = static_cast<ProcessorIdPair*> (user);
    return pp->processor->handleSinkEvents(events, n, pp->id);
}

static int32_t gHandleServerEvents(void * user, const aoo_event ** events, int32_t n)
{
    ProcessorIdPair * pp = static_cast<ProcessorIdPair*> (user);
    return pp->processor->handleServerEvents(events, n);
}

static int32_t gHandleClientEvents(void * user, const aoo_event ** events, int32_t n)
{
    ProcessorIdPair * pp = static_cast<ProcessorIdPair*> (user);
    return pp->processor->handleClientEvents(events, n);
}
#endif


int32_t SonobusAudioProcessor::handleAooServerEvent(const AooEvent *event, int32_t level)
{
    switch (event->type){
        case kAooEventClientLogin:
        {
            auto e = (const AooEventClientLogin *)event;

            DBG("Server - Client login: " << e->id);

            break;
        }
        case kAooRequestGroupJoin:
        {
            auto e = (const AooRequestGroupJoin *)event;

            DBG("Server - Group Joined: " << e->groupName << "  by user: " << e->userName << "id:" << e->userId);

            break;
        }
        case kAooRequestGroupLeave:
        {
            auto e = (const AooRequestGroupLeave *)event;

            DBG("Server - Group Left: " << e->group );

            break;
        }
        case kAooEventGroupAdd:
        {
            auto e = (const AooEventGroupAdd *)event;
            DBG("Server - Group Added: " << e->id << " : " << e->name);
            break;
        }
        case kAooEventGroupRemove:
        {
            auto e = (const AooEventGroupRemove *)event;
            DBG("Server - Group Remove: " << e->id);
            break;
        }
        case kAooEventError:
        {
            auto e = (const AooEventError *)event;
            DBG("Server error: " << e->errorMessage);
            break;
        }
        default:
            DBG("Got unknown server event: " << event->type);
            break;
    }

    return kAooOk;
}

int32_t SonobusAudioProcessor::handleAooClientEvent(const AooEvent *event, int32_t level)
{
    switch (event->type){
        case kAooEventPeerMessage:
        {
            auto e = (const AooEventPeerMessage *)event;
            // lookup by group/user id
            
            //aoo::ip_address address((const sockaddr *)e->address, e->addrlen);
            
            EndpointState * es = (EndpointState *) findOrAddEndpoint(e->groupId, e->userId);
            if (es) {
                handleOtherMessage(es, e->data.data, e->data.size);
            } else {
                DBG("Error finding endpoint for " << e->groupId << " : " << e->userId);
            }
            
            break;
        }
            
            
        case kAooRequestDisconnect:
        {
            // don't remove all peers?
            //removeAllRemotePeers();
            
            mIsConnectedToServer = false;
            mSessionConnectionStamp = 0.0;
            
            clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientDisconnected, this, true, "");
            
            break;
        }
#if 0
        case AOONET_CLIENT_GROUP_JOIN_EVENT:
        {
            aoonet_client_group_event *e = (aoonet_client_group_event *)event;
            if (e->result > 0){
                DBG("Joined group - " << e->name);
                const ScopedLock sl (mClientLock);
                mCurrentJoinedGroup = CharPointer_UTF8 (e->name);
                
                mSessionConnectionStamp = Time::getMillisecondCounterHiRes();
                
                
            } else {
                DBG("Couldn't join group " << e->name << " - " << e->errormsg);
            }
            clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientGroupJoined, this, e->result > 0, CharPointer_UTF8 (e->name), e->errormsg);
            break;
        }
        case AOONET_CLIENT_GROUP_LEAVE_EVENT:
        {
            aoonet_client_group_event *e = (aoonet_client_group_event *)event;
            if (e->result > 0){
                
                DBG("Group leave - " << e->name);
                
                const ScopedLock sl (mClientLock);
                mCurrentJoinedGroup.clear();
                
                // assume they are all part of the group, XXX
                removeAllRemotePeers();
                
                //aoo_node_remove_group(x->x_node, gensym(e->name));
                
                
            } else {
                DBG("Couldn't leave group " << e->name << " - " << e->errormsg);
            }
            
            clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientGroupLeft, this, e->result > 0, CharPointer_UTF8 (e->name), e->errormsg);
            
            break;
        }
        case AOONET_CLIENT_GROUP_PUBLIC_ADD_EVENT:
        {
            aoonet_client_group_event *e = (aoonet_client_group_event *)event;
            DBG("Public group add/changed - " << e->name << " count: " << e->result);
            {
                const ScopedLock sl (mPublicGroupsLock);
                String group = CharPointer_UTF8 (e->name);
                AooPublicGroupInfo & ginfo = mPublicGroupInfos[group];
                ginfo.groupName = group;
                ginfo.activeCount = e->result;
                ginfo.timestamp = Time::getCurrentTime().toMilliseconds();
            }
            
            clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientPublicGroupModified, this, CharPointer_UTF8 (e->name), e->result,  e->errormsg);
            break;
        }
        case AOONET_CLIENT_GROUP_PUBLIC_DEL_EVENT:
        {
            aoonet_client_group_event *e = (aoonet_client_group_event *)event;
            DBG("Public group deleted - " << e->name);
            {
                const ScopedLock sl (mPublicGroupsLock);
                String group = CharPointer_UTF8 (e->name);
                mPublicGroupInfos.erase(group);
            }
            
            clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientPublicGroupDeleted, this, CharPointer_UTF8 (e->name), e->errormsg);
            break;
        }
#endif
            
            
        case kAooEventPeerHandshake:
        {
            auto e = (const AooEventPeer *)event;
            
            DBG("Peer attempting to join group " <<  e->groupName << " - user " << e->userName);
            
            clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientPeerPendingJoin, this, CharPointer_UTF8 (e->groupName), CharPointer_UTF8 (e->userName), e->groupId, e->userId);
            
            break;
        }
            
        case kAooEventPeerJoin:
        {
            auto e = (const AooEventPeerJoin *)event;
            
            DBG("Peer joined group " <<  e->groupName << " - user " << e->userName << " userId: " << e->userId);
            if (mAutoconnectGroupPeers) {
                connectRemotePeerRaw(e->address.data, e->address.size, e->userId, CharPointer_UTF8 (e->userName), CharPointer_UTF8 (e->groupName), e->groupId, !mMainRecvMute.get());
            }
            
            //aoo_node_add_peer(x->x_node, gensym(e->group), gensym(e->user),
            //                  (const struct sockaddr *)e->address, e->length);
            
            clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientPeerJoined, this, CharPointer_UTF8 (e->groupName), CharPointer_UTF8 (e->userName), e->groupId, e->userId);
            
            
            break;
        }
        case kAooEventPeerTimeout:
        {
            auto e = (const AooEventPeerTimeout *)event;
            
            DBG("Peer failed to join group " <<  e->groupName << " - user " << e->userName);
            
            clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientPeerJoinFailed, this, CharPointer_UTF8 (e->groupName), CharPointer_UTF8 (e->userName), e->groupId, e->userId);
            
            break;
        }
        case kAooEventPeerLeave:
        {
            auto e = (const AooEventPeerLeave *)event;
            
            DBG("Peer leave group " <<  e->groupName << " - user " << e->userName);
            
            EndpointState * endpoint = findOrAddRawEndpoint(e->address.data, e->address.size);
            if (endpoint) {
                
                removeAllRemotePeersWithEndpoint(endpoint);
            }
            
            //aoo_node_remove_peer(x->x_node, gensym(e->group), gensym(e->user));
            clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientPeerLeft, this, CharPointer_UTF8 (e->groupName), CharPointer_UTF8 (e->userName), e->groupId, e->userId);
            
            break;
        }
        case kAooEventError:
        {
            auto e = (const AooEventError *)event;
            DBG("client error: " << e->errorMessage);
            clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientError, this, e->errorMessage);
            break;
        }
        case kAooEventPeerPing:
        {
            auto e = (const AooEventPeerPing *)event;
            DBG("Peer ping " <<  e->group << " - user " << e->user);
            break;
        }
            
        default:
            DBG("Got unknown client event: " << event->type);
            break;
    }
    
    return kAooOk;
}


int32_t SonobusAudioProcessor::handleAooSinkEvent(const AooEvent *event, int32_t level, int32 sinkId)
{
    switch (event->type){
        case kAooEventSourceAdd:
        {
            auto e = (AooEventEndpoint *)event;
            EndpointState * es = (EndpointState *) findOrAddRawEndpoint(e->endpoint.address, e->endpoint.addrlen);
            
            RemotePeer * peer = findRemotePeer(es, sinkId);
            if (peer) {
                // someone has added us, thus accepting our invitation
                int32_t dummyid;
                
#if 0
                if (mAooDummySource->getId(dummyid) == kAooOk && dummyid == e->endpoint.id ) {
                    // ignoring dummy add
                    DBG("Got dummy handshake add from " << es->ipaddr << ":" << es->port);
                }
                else
#endif
                {
                    DBG("Added source " << es->ipaddr << ":" << es->port << "  " <<  e->endpoint.id  << " to our " << sinkId);
                    peer->remoteSourceId = e->endpoint.id;
                    
                    //AooEndpoint bogusep = { e->endpoint.address, e->endpoint.addrlen, 0 };
                    //peer->oursink->uninviteSource(bogusep); // get rid of existing bogus one
                    
                    if (peer->recvAllow) {
                        peer->oursink->inviteSource(e->endpoint, nullptr);
                        //peer->recvActive = true;
                    } else {
                        DBG("we aren't accepting recv right now, politely decline it");
                        peer->oursink->uninviteSource(e->endpoint);
                        peer->recvActive = false;
                    }
                    
                    peer->connected = true;
                }
                
                // do invite here?
                
            }
            else {
                DBG("Added source to unknown " << e->endpoint.id);
            }
            // add remote source
            //doAddRemoteSourceIfNecessary(es, e->id);
            
            
            break;
        }
        case kAooEventFormatChange:
        {
            auto e = (const AooEventFormatChange *)event;
            EndpointState * es = (EndpointState *) findOrAddRawEndpoint(e->endpoint.address, e->endpoint.addrlen);
            
            const ScopedReadLock sl (mCoreLock);
            
            RemotePeer * peer = findRemotePeer(es, sinkId);
            if (peer) {
                AooFormatStorage f;
                if (peer->oursink->getSourceFormat(e->endpoint, f) == kAooOk) {
                    DBG("Got source format event from " << es->ipaddr << ":" << es->port << "  " <<  e->endpoint.id  << "  channels: " << f.header.numChannels);
                    peer->recvMeterSource.resize(f.header.numChannels, meterRmsWindow);
                    
                    // check for layout
                    bool gotuserformat = false;
                    char userfmtdata[1024];
                    JUCE_COMPILER_WARNING ("TODO: USERFORMAT");
                    /*
                     int32_t retsize = peer->oursink->get_sourceoption(e->endpoint, e->id, aoo_opt_userformat, userfmtdata, sizeof(userfmtdata));
                     if (retsize > 0) {
                     ValueTree tree = ValueTree::readFromData (userfmtdata, retsize);
                     
                     if (tree.isValid()) {
                     applyLayoutFormatToPeer(peer, tree);
                     gotuserformat = true;
                     }
                     else {
                     DBG("Error parsing userformat");
                     }
                     }
                     else {
                     DBG("No userformat: " << retsize);
                     }
                     */
                    
                    if (peer->recvChannels != f.header.numChannels) {
                        
                        {
                            const ScopedWriteLock sl (peer->sinkLock);
                            
                            peer->recvChannels = std::min(MAX_PANNERS, f.header.numChannels);
                            
                            // set up this sink with new channel count
                            
                            int sinkchan = std::max(getMainBusNumOutputChannels(), peer->recvChannels);
                            
                            peer->oursink->setup(sinkchan, getSampleRate(), currSamplesPerBlock, 0);
                        }
                        peer->recvMeterSource.resize (peer->recvChannels, meterRmsWindow);
                        
                        // for now if > 2, all on own changroup (by default)
                        
                        if (!gotuserformat && !peer->recvdChanLayout) {
                            if (peer->recvChannels > 2) {
                                if (!peer->modifiedChanGroups) {
                                    for (int cgi=0; cgi < peer->recvChannels; ++cgi) {
                                        peer->chanGroups[cgi].params.chanStartIndex = cgi;
                                        peer->chanGroups[cgi].params.numChannels = 1;
                                    }
                                    peer->numChanGroups = peer->recvChannels;
                                }
                            }
                            else {
                                peer->chanGroups[0].params.numChannels = peer->recvChannels;
                                peer->numChanGroups = 1;
                                
                                if (peer->recvChannels == 1) {
                                    peer->viewExpanded = false;
                                }
                            }
                        }
                        
                        if (peer->recvChannels == 1) {
                            peer->viewExpanded = false;
                        }
                        
                        /*
                         if (peer->recvChannels == 1) {
                         // center pan
                         peer->recvPan[0] = 0.0f;
                         } else if (peer->recvChannels == 2) {
                         // Left/Right
                         peer->recvStereoPan[0] = -1.0f;
                         peer->recvStereoPan[1] = 1.0f;
                         } else if (peer->recvChannels > 2) {
                         peer->recvStereoPan[0] = -1.0f;
                         peer->recvStereoPan[1] = 1.0f;
                         for (int i=2; i < peer->recvChannels; ++i) {
                         peer->recvPan[i] = 0.0f;
                         }
                         }
                         */
                    }
                    
                    
                    
                    AudioCodecFormatCodec codec = String(f.header.codecName) == kAooCodecOpus ? CodecOpus : CodecPCM;
                    if (codec == CodecOpus) {
                        AooFormatOpus *fmt = (AooFormatOpus *)&f;
                        // unknown parts,
                        //peer->recvFormat = AudioCodecFormatInfo(fmt->bitrate/fmt->header.nchannels, fmt->complexity, fmt->signalType);
                        //peer->recvFormatIndex = findFormatIndex(codec, fmt->bitrate / fmt->header.nchannels, 0);
                    } else {
                        AooFormatPcm *fmt = (AooFormatPcm *)&f;
                        int bitdepth = fmt->bitDepth == kAooPcmInt16 ? 2 : fmt->bitDepth == kAooPcmInt24  ? 3  : fmt->bitDepth == kAooPcmFloat32 ? 4 : fmt->bitDepth == kAooPcmFloat64  ? 8 : 2;
                        peer->recvFormat = AudioCodecFormatInfo(bitdepth);
                        //peer->recvFormatIndex = findFormatIndex(codec, 0, fmt->bitdepth);
                    }
                    
                    clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientPeerChangedState, this, "format");
                }
            }
            else {
                DBG("format event to unknown " << e->endpoint.id);
                
            }
            
            break;
        }
        case kAooEventStreamState:
        {
            auto e = (AooEventStreamState *)event;
            EndpointState * es = (EndpointState *) findOrAddRawEndpoint(e->endpoint.address, e->endpoint.addrlen);
            
            DBG("Got source state event from " << es->ipaddr << ":" << es->port << " -- " << e->state);
            
            const ScopedReadLock sl (mCoreLock);
            
            RemotePeer * peer = findRemotePeer(es, sinkId);
            if (peer) {
                peer->recvActive = peer->recvAllow && e->state == kAooStreamStateActive;
                if (!peer->recvActive && !peer->sendActive) {
                    peer->connected = false;
                } else {
                    peer->connected = true;
                }
            }
            
            //clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientPeerChangedState, this, "state");
            
            break;
        }
        case kAooEventBufferUnderrun:
            //case AOO_BLOCK_LOST_EVENT:
        {
            auto *e = (AooEventBufferUnderrun *)event;
            
            EndpointState * es = (EndpointState *) findOrAddRawEndpoint(e->endpoint.address, e->endpoint.addrlen);
            
            DBG("Got source underrun event from " << es->ipaddr << ":" << es->port << "   " << e->endpoint.id);
            
            const ScopedReadLock sl (mCoreLock);
            RemotePeer * peer = findRemotePeer(es, sinkId);
            if (peer) {
                peer->dataPacketsDropped += 1; // e->count;
                
                if (peer->autosizeBufferMode != AutoNetBufferModeOff) {
                    // see if our drop rate exceeds threshold, and increase buffersize if so
                    double nowtime = Time::getMillisecondCounterHiRes();
                    const float dropratethresh = peer->autosizeBufferMode == AutoNetBufferModeInitAuto ? 1.0f : mAutoresizeDropRateThresh;
                    const float adjustlimit = 0.5f; // don't adjust more often than once every 0.5 seconds
                    
                    bool autoinitdone = peer->autosizeBufferMode == AutoNetBufferModeInitAuto && peer->autoNetbufInitCompleted;
                    
                    if (peer->lastDroptime > 0 && !autoinitdone) {
                        double deltatime = (nowtime - peer->lastDroptime) * 1e-3;
                        if (deltatime > adjustlimit) {
                            //float droprate =  (peer->dataPacketsDropped - peer->lastDropCount) / deltatime;
                            float droprate =  1.0f / deltatime; // treat any drops as one instance
                            if (droprate > dropratethresh) {
                                float adjms = 1000.0f * currSamplesPerBlock / getSampleRate();
                                peer->buffertimeMs += adjms;
                                peer->totalEstLatency = peer->smoothPingTime.xbar + 2*peer->buffertimeMs + (1e3*currSamplesPerBlock/getSampleRate());
                                peer->oursink->setLatency(peer->buffertimeMs * 1e-3);
                                peer->latencyDirty = true;
                                peer->fillRatioSlow.reset();
                                peer->fillRatio.reset();
                                
                                DBG("AUTO-Increasing buffer time by " << adjms << " ms to " << (int)peer->buffertimeMs << " droprate: " << droprate);
                                
                                if (peer->hasRealLatency) {
                                    peer->totalEstLatency = peer->totalLatency + (peer->buffertimeMs - peer->bufferTimeAtRealLatency);
                                }
                                
                                if (peer->autosizeBufferMode == AutoNetBufferModeAutoFull) {
                                    
                                    const float timesincedecrthresh = 2.0;
                                    if (peer->lastNetBufDecrTime > 0 && (nowtime - peer->lastNetBufDecrTime)*1e-3 < timesincedecrthresh ) {
                                        peer->netBufAutoBaseline = peer->buffertimeMs;
                                        DBG("Got drop within short time thresh, setting minimum baseline for future decr to " << peer->netBufAutoBaseline);
                                    }
                                }
                                
                                sendRemotePeerInfoUpdate(-1, peer); // send to this peer
                                
                            }
                            
                            float realdroprate =  (peer->dataPacketsDropped - peer->lastDropCount) / deltatime;
                            peer->fastDropRate.push(realdroprate);
                            
                            peer->lastDroptime = nowtime;
                            peer->lastDropCount = peer->dataPacketsDropped;
                        }
                    }
                    else {
                        if (peer->lastDroptime > 0) {
                            double deltatime = (nowtime - peer->lastDroptime) * 1e-3;
                            float droprate =  (peer->dataPacketsDropped - peer->lastDropCount) / deltatime;
                            peer->fastDropRate.push(droprate);
                        }
                        
                        peer->lastDroptime = nowtime;
                        peer->lastDropCount = peer->dataPacketsDropped;
                    }
                    
                    
                    
                    //peer->lastNetBufDecrTime = 0; // reset auto-decr
                }
            }
            
            break;
        }
        case kAooEventBlockDrop:
        {
            auto *e = (AooEventBlockDrop *)event;
            
            EndpointState * es = (EndpointState *) findOrAddRawEndpoint(e->endpoint.address, e->endpoint.addrlen);
            
            DBG("Got source block dropped event from " << es->ipaddr << ":" << es->port << "  " << e->endpoint.id << " -- " << e->count);
            
            break;
        }
        case kAooEventBlockResend:
        {
            auto *e = (AooEventBlockResend *)event;
            EndpointState * es = (EndpointState *) findOrAddRawEndpoint(e->endpoint.address, e->endpoint.addrlen);
            
            DBG("Got source block resent event from " << es->ipaddr << ":" << es->port << "  " << e->endpoint.id << " -- " << e->count);
            const ScopedReadLock sl (mCoreLock);
            RemotePeer * peer = findRemotePeer(es, sinkId);
            if (peer) {
                peer->dataPacketsResent += e->count;
            }
            
            break;
        }
        case kAooEventSinkPing:
        {
            auto *e = (AooEventSinkPing *)event;
            EndpointState * es = (EndpointState *) findOrAddRawEndpoint(e->endpoint.address, e->endpoint.addrlen);
            
            
            double diff = aoo::time_tag::duration(e->t1, e->t2) * 1000.0;
            DBG("Got source block ping event from " << es->ipaddr << ":" << es->port << "  " << e->endpoint.id << " -- " << diff);
            
            
            RemotePeer * peer = findRemotePeer(es, sinkId);
            if (peer) {
                const ScopedReadLock sl (mCoreLock);
                
                double nowtime = Time::getMillisecondCounterHiRes();
                
                double deltadroptime = peer->lastDroptime > 0 ? (nowtime - peer->lastDroptime) * 1e-3 : (nowtime - peer->resetDroptime) * 1e-3;
                
                if (peer->autosizeBufferMode != AutoNetBufferModeOff) {
                    if (!peer->autoNetbufInitCompleted) {
                        const float nodropsthresh = 7.0;
                        
                        if (deltadroptime > nodropsthresh) {
                            peer->autoNetbufInitCompleted = true;
                            peer->resetSafetyMuted = false;
                            DBG("Netbuf Initial auto time is done after no drops in " << nodropsthresh);
                            
                            // clear drop count
                            peer->dataPacketsResent = 0;
                            peer->dataPacketsDropped = 0;
                            peer->lastDropCount = 0;
                            peer->resetDroptime = nowtime;
                            peer->fastDropRate.resetInitVal(0.0f);
                            //peer->lastDroptime = 0;
                        }
                        
                    }
                    
                }
                else {
                    // manual mode
                    peer->resetSafetyMuted = false;
                }
                
                
                if (peer->autosizeBufferMode == AutoNetBufferModeAutoFull) {
                    // possibly adjust net buffer down, if it has been longer than threshold since last drop
                    double nowtime = Time::getMillisecondCounterHiRes();
                    const float nodropsthresh = 10.0; // no drops in 10 seconds
                    const float adjustlimit = 10; // don't adjust more often than once every 10 seconds
                    
                    if (peer->lastNetBufDecrTime > 0 && peer->buffertimeMs > peer->netBufAutoBaseline && !peer->latencyMatched) {
                        double deltatime = (nowtime - peer->lastNetBufDecrTime) * 1e-3;
                        double deltadroptime = (nowtime - peer->lastDroptime) * 1e-3;
                        if (deltatime > adjustlimit) {
                            //float droprate =  (peer->dataPacketsDropped - peer->lastDropCount) / deltatime;
                            //if (droprate < dropratethresh) {
                            if (deltadroptime > nodropsthresh) {
                                float adjms = 1000.0f * currSamplesPerBlock / getSampleRate();
                                peer->buffertimeMs -= adjms;
                                
                                peer->buffertimeMs = std::max(peer->buffertimeMs, peer->netBufAutoBaseline);
                                
                                peer->totalEstLatency = peer->smoothPingTime.xbar + 2*peer->buffertimeMs + (1e3*currSamplesPerBlock/getSampleRate());
                                peer->oursink->setLatency(peer->buffertimeMs * 1e-3);
                                peer->latencyDirty = true;
                                
                                peer->fillRatioSlow.reset();
                                peer->fillRatio.reset();
                                
                                if (peer->hasRealLatency) {
                                    peer->totalEstLatency = peer->totalLatency + (peer->buffertimeMs - peer->bufferTimeAtRealLatency);
                                }
                                
                                DBG("AUTO-Decreasing buffer time by " << adjms << " ms to " << (int) peer->buffertimeMs);
                                
                                peer->lastNetBufDecrTime = nowtime;
                                
                                sendRemotePeerInfoUpdate(-1, peer); // send to this peer
                                
                            }
                            
                            //peer->lastNetBufDropCount = peer->dataPacketsDropped;
                        }
                    }
                    else {
                        peer->lastNetBufDecrTime = nowtime;
                    }
                    
                }
                
                if (peer->resetSafetyMuted) {
                    updateSafetyMuting(peer);
                }
            }
            
            
            break;
        }
        default:
            break;
    }
    
    return kAooOk;
}

int32_t SonobusAudioProcessor::handleAooSourceEvent(const AooEvent *event, int32_t level, int32_t sourceId)
{
    switch (event->type){
        case kAooEventSourcePing:
        {
            auto & e = * ((AooEventSourcePing*) event);
            double diff1 = aoo::time_tag::duration(e.t1, e.t2) * 1000.0;
            double diff2 = aoo::time_tag::duration(e.t2, e.t3) * 1000.0;
            double rtt = aoo::time_tag::duration(e.t1, e.t3) * 1000.0;

            EndpointState * es = (EndpointState *) findOrAddRawEndpoint(e.endpoint.address, e.endpoint.addrlen);

            RemotePeer * peer = findRemotePeer(es, sourceId);
            if (peer && !peer->gotNewStylePing) {
                const ScopedReadLock sl (mCoreLock);

                // smooth it
                peer->pingTime = rtt; // * 0.5;
                if (rtt < 600.0 ) {
                    peer->smoothPingTime.Z *= 0.5f;
                    peer->smoothPingTime.push(peer->pingTime);
                }

                DBG("ping to source " << sourceId << " recvd from " <<  es->ipaddr << ":" << es->port << " -- " << diff1 << " " << diff2 << " " <<  rtt << " smooth: " << peer->smoothPingTime.xbar << " stdev: " <<peer->smoothPingTime.s2xx);


                if (!peer->hasRealLatency) {
                    peer->totalEstLatency =  peer->smoothPingTime.xbar + 2*peer->buffertimeMs + (1e3*currSamplesPerBlock/getSampleRate());
                }
            }
            break;
        }
        case kAooEventInvite:
        {
            auto *e = (AooEventInvite *)event;

            // accepts invites
            if (true){
                EndpointState * es = (EndpointState *) findOrAddRawEndpoint(e->endpoint.address, e->endpoint.addrlen);
                // handle dummy source specially

                {
                    // invited
                    aoo::ip_address epaddr((const struct sockaddr *)e->endpoint.address, e->endpoint.addrlen);

                    char tmpbuf[64];
                    std::string addrhex;
                    for (int i=0; i < e->endpoint.addrlen; ++i) {
                        snprintf(tmpbuf, sizeof(tmpbuf), "%x", ((uint8_t *)e->endpoint.address)[i] );
                        addrhex += tmpbuf;
                        addrhex += " ";
                    }
                    DBG("raw addr: " << addrhex);

                    DBG("Invite received to our source: " << sourceId << " from " << epaddr.name_unmapped() <<  " esaddr: " << es->ipaddr << ":" << es->port << "  " << e->endpoint.id);

                    if (sourceId == mCurrentUserId) {

                        if (auto * peer = findRemotePeer(es, -1)) {

                            peer->remoteSinkId = e->endpoint.id;
                            peer->connected = true; // ??

                            if (peer->sendAllow && !mMainSendMute.get()) {
                                DBG("Accepted invitation to send from our common to remote sink: " << peer->remoteSinkId);
                                mAooCommonSource->handleInvite(e->endpoint, e->token, true);
                                peer->sendCommonActive = true;
                            }
                            else {
                                mAooCommonSource->handleInvite(e->endpoint, e->token, false);
                                peer->sendCommonActive = false;
                                DBG("Not sending from our common to remote sink: " << peer->remoteSinkId);
                            }
                            
                            sendRemotePeerInfoUpdate(-1, peer);
                        }
                        else {
                            DBG("Could not find peer from endpoint alone");
                        }
                    }
                    else if (auto * peer = findRemotePeer(es, sourceId)) {
                        
                        peer->remoteSinkId = e->endpoint.id;
                        
                        //peer->oursource->addSink(e->endpoint, 0);
                        
                        //peer->oursource->set_sinkoption(es, peer->remoteSinkId, aoo_opt_protocol_flags, &e->flags, sizeof(int32_t));
                        
                        if (peer->sendAllow) {
                            peer->oursource->handleInvite(e->endpoint, e->token, true);
                            peer->sendActive = true;
                            DBG("Accepted invitation to send to remote sink: " << peer->remoteSinkId);
                        } else {
                            // don't accept invitation to send
                            peer->oursource->handleInvite(e->endpoint, e->token, false);
                            DBG("Not sending , Not Accepting invitation to send to remote sink");
                        }
                        
                        peer->connected = true;
                        
                        updateRemotePeerUserFormat(-1, peer);
                        sendRemotePeerInfoUpdate(-1, peer);
                        
                        DBG("Finishing peer connection for " << es->ipaddr << ":" << es->port  << "  " << peer->remoteSinkId);
                        
                    }
                    else {
                        // not one of our sources
                        DBG("No source " << sourceId << " invited, how is this possible?");
                    }
                    
                }

            } else {
                DBG("Invite received");
            }

            break;
        }
        case kAooEventUninvite:
        {
            auto *e = (AooEventUninvite *)event;

            // accepts uninvites
            if (true){
                EndpointState * es = (EndpointState *) findOrAddRawEndpoint(e->endpoint.address, e->endpoint.addrlen);
                int32_t dummyid;


                RemotePeer * peer = findRemotePeerByRemoteSinkId(es, e->endpoint.id);

                if (peer) {
                    if (sourceId == mCurrentUserId) {
                        mAooCommonSource->handleUninvite(e->endpoint, e->token, true);
                        peer->sendCommonActive = false;
                        peer->dataPacketsSent = 0;
                        if (!peer->recvActive) {
                            peer->connected = false;
                        }
                        DBG("Uninvited from our common source " << es->ipaddr << ":" << es->port);
                    }
                    else
                    {
                        const ScopedReadLock sl (mCoreLock);
                        peer->oursource->handleUninvite(e->endpoint, e->token, true);
                        peer->oursource->removeAll();

                        peer->sendActive = false;
                        peer->dataPacketsSent = 0;

                        if (!peer->recvActive) {
                            peer->connected = false;
                        }
                        DBG("Uninvited from our remote peer source " << es->ipaddr << ":" << es->port);
                    }


                }
                else {
                    DBG("Uninvite received to unknown");
                }

            } else {
                DBG("Uninvite received");
            }

            break;
        }
#if 0
        case AOO_CHANGECODEC_EVENT:
        {
            aoo_source_event *e = (aoo_source_event *)event;
            DBG("Change codec received from sink " << e->id);

            EndpointState * es = (EndpointState *) findOrAddRawEndpoint(e->ep.address, e->ep.addrlen);

            RemotePeer * peer = findRemotePeerByRemoteSinkId(es, e->id);

            if (peer) {
                // now we need to set our latency and echo source to match our main source's format
                aoo_format_storage fmt;
                if (peer->oursource->get_format(fmt) > 0) {

                    AudioCodecFormatCodec codec = String(fmt.header.codec) == AOO_CODEC_OPUS ? CodecOpus : CodecPCM;
                    if (codec == CodecOpus) {
                        aoo_format_opus *ofmt = (aoo_format_opus *)&fmt;
                        int retindex = findFormatIndex(codec, ofmt->bitrate / ofmt->header.nchannels, 0);
                        if (retindex >= 0) {
                            peer->formatIndex = retindex; // new sending format index
                        }
                    }
                    else if (codec == CodecPCM) {
                        aoo_format_pcm *pfmt = (aoo_format_pcm *)&fmt;
                        int bdepth = pfmt->bitdepth == AOO_PCM_FLOAT32 ? 4 : pfmt->bitdepth == AOO_PCM_INT24 ? 3 : pfmt->bitdepth == AOO_PCM_FLOAT64 ? 8 : 2;
                        int retindex = findFormatIndex(codec, 0, bdepth);
                        if (retindex >= 0) {
                            peer->formatIndex = retindex; // new sending format index
                        }
                    }
                }
            }
        }
#endif
        default:
            break;
    }

    return kAooOk;
}

void SonobusAudioProcessor::handleEvents()
{
    const ScopedReadLock sl (mCoreLock);        
    int32_t dummy = 0;

#if 1
    if (mAooCommonSource->eventsAvailable() > 0) {
        mAooCommonSource->pollEvents();
    }
#endif

    for (auto & remote : mRemotePeers) {
        if (remote->oursource) {
            remote->oursource->pollEvents();
        }
        if (remote->oursink) {
            remote->oursink->pollEvents();
        }

    }


#if 0
    if (mAooServer /*&& mAooServer->events_available()*/) {
        ProcessorIdPair pp(this, dummy);
        mAooServer->handle_events(gHandleServerEvents, &pp);
    }

    if (mAooClient && mAooClient->events_available()) {
        ProcessorIdPair pp(this, dummy);
        mAooClient->handle_events(gHandleClientEvents, &pp);
    }

    
    if (mAooDummySource->events_available() > 0) {
        mAooDummySource->get_id(dummy);
        ProcessorIdPair pp(this, dummy);
        mAooDummySource->handle_events(gHandleSourceEvents, &pp);
    }

    for (auto & remote : mRemotePeers) {
        if (remote->oursource) {
            remote->oursource->get_id(dummy);
            ProcessorIdPair pp(this, dummy);
            remote->oursource->handle_events(gHandleSourceEvents, &pp);
        }
        if (remote->oursink) {
            remote->oursink->get_id(dummy);
            ProcessorIdPair pp(this, dummy);
            remote->oursink->handle_events(gHandleSinkEvents, &pp);

        }

    }
#endif
}

void SonobusAudioProcessor::sendPingEvent(RemotePeer * peer)
{

    auto tt = aoo::time_tag::now();

    char buf[AOO_MAX_PACKET_SIZE];
    osc::OutboundPacketStream outmsg(buf, sizeof(buf));

    try {
        outmsg << osc::BeginMessage(SONOBUS_FULLMSG_PING)
        << osc::TimeTag(tt)
        << osc::EndMessage;
    }
    catch (const osc::Exception& e){
        DBG("exception in ping message construction: " << e.what());
        return;
    }

    sendPeerMessage(peer, (AooByte*)outmsg.Data(), (int32_t) outmsg.Size());

    DBG("Sent ping to peer: " << peer->endpoint->ipaddr);
}


void SonobusAudioProcessor::handlePingEvent(EndpointState * endpoint, uint64_t tt1, uint64_t tt2, uint64_t tt3)
{
    double diff1 = aoo::time_tag::duration(tt1, tt2) * 1000.0;
    double diff2 = aoo::time_tag::duration(tt2, tt3) * 1000.0;
    double rtt = aoo::time_tag::duration(tt1, tt3) * 1000.0;

    const ScopedReadLock sl (mCoreLock);

    auto * peer = findRemotePeer(endpoint, -1);
    if (!peer) return;

    // smooth it
    peer->pingTime = rtt; // * 0.5;
    if (rtt < 600.0 ) {
        peer->smoothPingTime.Z *= 0.5f;
        peer->smoothPingTime.push(peer->pingTime);
    }

    DBG("ping recvd from " << peer->endpoint->ipaddr << ":" << peer->endpoint->port << " -- " << diff1 << " " << diff2 << " " <<  rtt << " smooth: " << peer->smoothPingTime.xbar << " stdev: " <<peer->smoothPingTime.s2xx);


    if (!peer->hasRealLatency) {
        peer->totalEstLatency =  peer->smoothPingTime.xbar + 2*peer->buffertimeMs + (1e3*currSamplesPerBlock/getSampleRate());
    }

    peer->gotNewStylePing = true;
}



bool SonobusAudioProcessor::connectRemotePeerRaw(const void * sockaddr, int addrlen, AooId userid, const String & username, const String & groupname, AooId groupid, bool reciprocate)
{
    EndpointState * endpoint = findOrAddRawEndpoint(sockaddr, addrlen);
    
    if (!endpoint) {
        DBG("Error getting endpoint from raw address");
        return false;
    }
    
    return connectRemotePeerInternal(endpoint, userid, username, groupname, groupid, reciprocate);
}

bool SonobusAudioProcessor::connectRemotePeer(const String & host, int port, AooId userid, const String & username, const String & groupname, AooId groupid, bool reciprocate)
{
    EndpointState * endpoint = findOrAddEndpoint(host, port);

    return connectRemotePeerInternal(endpoint, userid, username, groupname, groupid, reciprocate);
}

bool SonobusAudioProcessor::connectRemotePeerInternal(EndpointState * endpoint, AooId userid, const String & username, const String & groupname, AooId groupid, bool reciprocate)
{
    if (!endpoint) {
        DBG("No endpoint passed to connectRemotePeerInternal");
        return false;
    }
    
    RemotePeer * remote = doAddRemotePeerIfNecessary(endpoint, userid, userid, username, groupname, groupid); // get new one

    endpoint->groupid = groupid;
    endpoint->userid = userid;

    remote->recvAllow = !mMainRecvMute.get();

    // if we use their userid as the source id to get their common source
    
    // or - use our userid as source id to get their source specific to us
    
    AooId sourceid = userid;
    
    AooEndpoint aep = { endpoint->address.address_ptr(), (AooAddrSize) endpoint->address.length(), sourceid };
    remote->remoteSourceId = sourceid;

    bool ret = remote->oursink->inviteSource(aep, nullptr) == kAooOk;
    
    if (ret) {
        DBG("Successfully invited remote peer at " <<  endpoint->address.name_unmapped() << ":" << endpoint->address.port() << " - ourId " << remote->ourId);
        remote->connected = true;
        remote->invitedPeer = reciprocate;
        //remote->recvActive = reciprocate;
        if (!mMainSendMute.get()) {
            remote->sendActive = true;
            remote->oursource->startStream(nullptr);
            updateRemotePeerUserFormat(-1, remote);
        }

        sendRemotePeerInfoUpdate(-1, remote);
        sendBlockedInfoMessage(remote->endpoint, false);
        
    } else {
        DBG("Error inviting remote peer at " << endpoint->address.name_unmapped() << ":" <<  endpoint->address.port() << " - ourId " << remote->ourId);
    }

    return ret;
}


bool SonobusAudioProcessor::disconnectRemotePeer(const String & host, int port, int32_t ourId)
{
    EndpointState * endpoint = findOrAddEndpoint(host, port);
    bool ret = false;

    {
        const ScopedReadLock sl (mCoreLock);        
        
        RemotePeer * remote = findRemotePeer(endpoint, ourId);
        
        if (remote) {
            if (remote->oursink) {
                DBG("uninviting all remote source " << remote->remoteSourceId);
                ret = remote->oursink->uninviteAll();
                //ret = remote->oursink->uninvite_source(endpoint, remote->remoteSourceId, endpoint_send) == 1;
            }
         
            // if we auto-invited the other end's remote source, remove that as a dest sink
            if (remote->oursource) {
                DBG("removing all remote sink " << remote->remoteSinkId);
                remote->oursource->removeAll();
                //ret |= remote->oursource->remove_sink(endpoint, remote->remoteSinkId) == 1;
            }
            
            remote->connected = false;
            remote->recvActive = false;
            remote->sendActive = false;

            endpoint->groupid = kAooIdInvalid;
            endpoint->userid = kAooIdInvalid;
        }
    }

    //doRemoveRemotePeerIfNecessary(endpoint, ourId);        

    
    if (ret) {
        DBG("Successfully disconnected remote peer " << ourId << " at " << host << ":" << port);
    } else {
        DBG("Error disconnecting remote peer " << ourId << " at " << host << ":" << port);
    }

    return ret;
}

bool SonobusAudioProcessor::disconnectRemotePeer(int index)
{
    RemotePeer * remote = 0;
    bool ret = false;
    {
        const ScopedReadLock sl (mCoreLock);
        
        if (index < mRemotePeers.size()) {
            remote = mRemotePeers.getUnchecked(index);
            if (remote->oursink) {
                DBG("uninviting all remote source " << remote->remoteSourceId);
                ret = remote->oursink->uninviteAll();
            }

            // if we auto-invited the other end's remote source, remove that as a dest sink
            if (remote->oursource && remote->remoteSinkId >= 0) {
                DBG("removing all remote sink " << remote->remoteSinkId);
                remote->oursource->removeAll();
                //ret |= remote->oursource->remove_sink(remote->endpoint, remote->remoteSinkId) == 1;
            }
            
            remote->connected = false;
            remote->recvActive = false;
            remote->sendActive = false;
            
            //mRemotePeers.remove(index);
        }
    }
    
    return ret;
}

bool SonobusAudioProcessor::getPatchMatrixValue(int srcindex, int destindex) const
{
    if (srcindex < MAX_PEERS && destindex < MAX_PEERS) {
        return mRemoteSendMatrix[srcindex][destindex];
    }
    return false;
}

void SonobusAudioProcessor::setPatchMatrixValue(int srcindex, int destindex, bool value)
{

    if (srcindex < MAX_PEERS && destindex < MAX_PEERS) {
        mRemoteSendMatrix[srcindex][destindex] = value;

        const ScopedReadLock sl (mCoreLock);        
        if (destindex < mRemotePeers.size() && destindex >= 0) {
            auto * peer = mRemotePeers.getUnchecked(destindex);

            updateRemotePeerSendChannels(destindex, peer);
            
        }
    }
}


void SonobusAudioProcessor::adjustRemoteSendMatrix(int index, bool removed)
{
    if (removed) {
        // adjust mRemoteSendMatrix
        // shift rows
        for (int i=index+1; i < mRemotePeers.size(); ++i) {
            // shift values up 1 from index+1 on
            for (int j=0; j < mRemotePeers.size(); ++j) {
                mRemoteSendMatrix[i-1][j] = mRemoteSendMatrix[i][j];         
            }                
        }
        // shift cols (one less row
        for (int i=0; i < mRemotePeers.size() - 1; ++i) {
            // shift values back 1 from index+1 on
            for (int j=index+1; j < mRemotePeers.size(); ++j) {
                mRemoteSendMatrix[i][j-1] = mRemoteSendMatrix[i][j];                    
            }                
        }
    }
    else {
        // adjust mRemoteSendMatrix to insert a new row
        // shift rows
        for (int i=mRemotePeers.size(); i > index; --i) {
            // shift values down 1 from index on
            for (int j=0; j < mRemotePeers.size(); ++j) {
                mRemoteSendMatrix[i][j] = mRemoteSendMatrix[i-1][j];         
            }                
        }
        // shift cols (one more row)
        for (int i=0; i < mRemotePeers.size() + 1; ++i) {
            // shift values forward 1 from index+1 on
            for (int j=mRemotePeers.size(); j > index; --j) {
                mRemoteSendMatrix[i][j] = mRemoteSendMatrix[i][j-1];                    
            }            
        }                
        // now put in default crossroutings
        for (int i=0; i < mRemotePeers.size() + 1; ++i) {
            mRemoteSendMatrix[i][index] = false;
            mRemoteSendMatrix[index][i] = false;
        }                

    }
}

bool SonobusAudioProcessor::removeAllRemotePeers()
{
    const ScopedReadLock sl (mCoreLock);

    OwnedArray<RemotePeer> removed;

    for (int index = 0; index < mRemotePeers.size(); ++index) {  
        auto remote = mRemotePeers.getUnchecked(index);
        
        commitCacheForPeer(remote);

        if (remote->connected) {
            disconnectRemotePeer(index);
        }

        removed.add(remote);
    }


    {
        const ScopedWriteLock slw (mCoreLock);
        mRemotePeers.clearQuick(false); // not deleting objects here

        for (int index = 0; index < removed.size(); ++index) {
            auto remote = removed.getUnchecked(index);

            mAooClient->removeSink(remote->oursink.get());

            mAooClient->removeSource(remote->oursource.get());
        }
    }
    
    // reset matrix
    for (int i=0; i < MAX_PEERS; ++i) {
        for (int j=0; j < MAX_PEERS; ++j) {
            mRemoteSendMatrix[i][j] = false;
        }
    }

    // they will be cleaned up when removed list goes out of scope

    return true;
}


bool SonobusAudioProcessor::removeRemotePeer(int index, bool sendblock)
{
    RemotePeer * remote = 0;
    bool ret = false;
    {
        const ScopedReadLock sl (mCoreLock);

        if (index < mRemotePeers.size()) {            
            remote = mRemotePeers.getUnchecked(index);

            commitCacheForPeer(remote);
            
            if (remote->connected) {
                disconnectRemotePeer(index);
            }
                        
            if (sendblock) {
                sendBlockedInfoMessage(remote->endpoint, true);
            }
            
            adjustRemoteSendMatrix(index, true);
            
            std::unique_ptr<RemotePeer> removed(remote);

            {
                const ScopedWriteLock slw (mCoreLock);
                mAooClient->removeSink(remote->oursink.get());

                mAooClient->removeSource(remote->oursource.get());

                mRemotePeers.remove(index, false); // not deleting in scoped write lock
            }

        }
    }
    
    return ret;
}


int SonobusAudioProcessor::getNumberRemotePeers() const
{
    return mRemotePeers.size();
}

void SonobusAudioProcessor::setRemotePeerLevelGain(int index, float levelgain)
{
    const ScopedReadLock sl (mCoreLock);
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        remote->gain = levelgain;
    }
}

float SonobusAudioProcessor::getRemotePeerLevelGain(int index) const
{
    float levelgain = 0.0f;

    const ScopedReadLock sl (mCoreLock);
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        levelgain = remote->gain;
    }
    return levelgain;
}


void SonobusAudioProcessor::setRemotePeerChannelGain(int index, int changroup, float levelgain)
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size() && changroup < MAX_CHANGROUPS) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        remote->chanGroups[changroup].params.gain = levelgain;
    }
}

float SonobusAudioProcessor::getRemotePeerChannelGain(int index, int changroup) const
{
    float levelgain = 0.0f;
    
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size() && changroup < MAX_CHANGROUPS) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        levelgain = remote->chanGroups[changroup].params.gain;
    }
    return levelgain;
}

void SonobusAudioProcessor::setRemotePeerChannelReverbSend(int index, int changroup, float rgain)
{
    const ScopedReadLock sl (mCoreLock);
    if (index < mRemotePeers.size() && changroup < MAX_CHANGROUPS) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        remote->chanGroups[changroup].params.monReverbSend = rgain;
    }
}

float SonobusAudioProcessor::getRemotePeerChannelReverbSend(int index, int changroup)
{
    float revsend = 0.0f;

    const ScopedReadLock sl (mCoreLock);
    if (index < mRemotePeers.size() && changroup < MAX_CHANGROUPS) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        revsend = remote->chanGroups[changroup].params.monReverbSend;
    }
    return revsend;
}

void SonobusAudioProcessor::setRemotePeerPolarityInvert(int index, int changroup, bool invert)
{
    const ScopedReadLock sl (mCoreLock);
    if (index < mRemotePeers.size() && changroup < MAX_CHANGROUPS) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        remote->chanGroups[changroup].params.invertPolarity = invert;
    }
}

bool SonobusAudioProcessor::getRemotePeerPolarityInvert(int index, int changroup)
{
    const ScopedReadLock sl (mCoreLock);
    if (index < mRemotePeers.size() && changroup < MAX_CHANGROUPS) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return remote->chanGroups[changroup].params.invertPolarity;
    }
    return false;
}

void SonobusAudioProcessor::setRemotePeerChannelMuted(int index, int changroup, bool muted)
{
    const ScopedReadLock sl (mCoreLock);
    if (index < mRemotePeers.size() && changroup < MAX_CHANGROUPS) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        remote->chanGroups[changroup].params.muted = muted;
    }
}

bool SonobusAudioProcessor::getRemotePeerChannelMuted(int index, int changroup) const
{
    const ScopedReadLock sl (mCoreLock);
    if (index < mRemotePeers.size() && changroup < MAX_CHANGROUPS) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return remote->chanGroups[changroup].params.muted;
    }
    return false;
}

void SonobusAudioProcessor::setRemotePeerChannelSoloed(int index, int changroup, bool soloed)
{
    const ScopedReadLock sl (mCoreLock);
    if (index < mRemotePeers.size() && changroup < MAX_CHANGROUPS) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        remote->chanGroups[changroup].params.soloed = soloed;
    }
}

bool SonobusAudioProcessor::getRemotePeerChannelSoloed(int index, int changroup) const
{
    const ScopedReadLock sl (mCoreLock);
    if (index >= 0 && index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        if (changroup < 0) {
            // check all groups for any soloed
            bool anysoloed = false;
            for (int i=0; i < remote->numChanGroups && i < MAX_CHANGROUPS; ++i) {
                if (remote->chanGroups[i].params.soloed) {
                    anysoloed = true;
                    break;
                }
            }
            return anysoloed;
        }
        else if (changroup < MAX_CHANGROUPS) {
            return remote->chanGroups[changroup].params.soloed;
        }
    }
    return false;
}


String SonobusAudioProcessor::getRemotePeerChannelGroupName(int index, int changroup) const
{
    const ScopedReadLock sl (mCoreLock);
    if (index < mRemotePeers.size() && changroup < MAX_CHANGROUPS) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return remote->chanGroups[changroup].params.name;
    }
    return "";
}

void SonobusAudioProcessor::setRemotePeerChannelGroupName(int index, int changroup, const String & name)
{
    const ScopedReadLock sl (mCoreLock);
    if (index < mRemotePeers.size() && changroup < MAX_CHANGROUPS) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        remote->chanGroups[changroup].params.name = name;
        remote->modifiedChanGroups = true;
        remote->modifiedMultiChanGroups = true;
    }
}

void SonobusAudioProcessor::setRemotePeerChannelGroupCount(int index, int count)
{
    const ScopedReadLock sl (mCoreLock);
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        int newcnt = std::max(0, std::min(count, MAX_CHANGROUPS-1));
        remote->numChanGroups = newcnt;
        remote->modifiedChanGroups = true;
        remote->modifiedMultiChanGroups = true;
    }
}


void SonobusAudioProcessor::setRemotePeerUserName(int index, const String & name)
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        auto * remote = mRemotePeers.getUnchecked(index);
        remote->userName = name;
    }
}

String SonobusAudioProcessor::getRemotePeerUserName(int index) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        auto * remote = mRemotePeers.getUnchecked(index);
        return remote->userName;
    }
    return "";
}

bool SonobusAudioProcessor::isRemotePeerUserInGroup(const String & name) const
{
    const ScopedReadLock sl (mCoreLock);
    for (int index = 0; index < mRemotePeers.size(); ++index) {
        auto * remote = mRemotePeers.getUnchecked(index);
        if (remote->userName == name)
            return true;
    }
    return false;
}


void SonobusAudioProcessor::setRemotePeerChannelPan(int index, int changroup, int chan, float pan)
{
    const ScopedReadLock sl (mCoreLock);
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        if (changroup < MAX_CHANGROUPS) {
            if (remote->chanGroups[changroup].params.numChannels == 2 && chan < 2) {
                remote->chanGroups[changroup].params.panStereo[chan] = pan;
            }
            else if (chan < MAX_CHANNELS){
                remote->chanGroups[changroup].params.pan[chan] = pan;
            }
            remote->modifiedChanGroups = true;
            remote->modifiedMultiChanGroups = true;
        }
    }
}

float SonobusAudioProcessor::getRemotePeerChannelPan(int index, int changroup, int chan) const
{
    float pan = 0.0f;
    
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size() && changroup < MAX_CHANGROUPS) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);

        if (chan >= 0 && chan < MAX_CHANNELS) {
            if (remote->chanGroups[changroup].params.numChannels == 2 && chan < 2) {
                pan = remote->chanGroups[changroup].params.panStereo[chan];
            } else {
                pan = remote->chanGroups[changroup].params.pan[chan];
            }
        }
    }
    return pan;
}

void SonobusAudioProcessor::setRemotePeerChannelGroupStartAndCount(int index, int changroup, int start, int count)
{
    const ScopedReadLock sl (mCoreLock);
    if (index < mRemotePeers.size() && changroup < MAX_CHANGROUPS) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        remote->chanGroups[changroup].params.chanStartIndex = start;
        remote->chanGroups[changroup].params.numChannels = std::max(1, std::min(count, MAX_CHANNELS));
        remote->modifiedChanGroups = true;
        remote->modifiedMultiChanGroups = true;
    }
}

bool SonobusAudioProcessor::getRemotePeerChannelGroupStartAndCount(int index, int changroup, int & retstart, int & retcount)
{
    const ScopedReadLock sl (mCoreLock);
    if (index < mRemotePeers.size() && changroup < MAX_CHANGROUPS) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);

        retstart = remote->chanGroups[changroup].params.chanStartIndex;
        retcount = remote->chanGroups[changroup].params.numChannels;
        return true;
    }
    return false;
}

void SonobusAudioProcessor::setRemotePeerChannelGroupDestStartAndCount(int index, int changroup, int start, int count)
{
    const ScopedReadLock sl (mCoreLock);
    if (index < mRemotePeers.size() && changroup < MAX_CHANGROUPS) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        remote->chanGroups[changroup].params.panDestStartIndex = start;
        remote->chanGroups[changroup].params.panDestChannels = std::max(1, std::min(count, MAX_CHANNELS));
        remote->modifiedChanGroups = true;
        remote->modifiedMultiChanGroups = true;
    }
}

bool SonobusAudioProcessor::getRemotePeerChannelGroupDestStartAndCount(int index, int changroup, int & retstart, int & retcount)
{
    const ScopedReadLock sl (mCoreLock);
    if (index < mRemotePeers.size() && changroup < MAX_CHANGROUPS) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);

        retstart = remote->chanGroups[changroup].params.panDestStartIndex;
        retcount = remote->chanGroups[changroup].params.panDestChannels;
        return true;
    }
    return false;
}

void SonobusAudioProcessor::setRemotePeerChannelGroupSendMainMix(int index, int changroup, bool mainmix)
{
    const ScopedReadLock sl (mCoreLock);
    if (index < mRemotePeers.size() && changroup < MAX_CHANGROUPS) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        remote->chanGroups[changroup].params.sendMainMix = mainmix;
    }
}

bool SonobusAudioProcessor::getRemotePeerChannelGroupSendMainMix(int index, int changroup)
{
    const ScopedReadLock sl (mCoreLock);
    if (index < mRemotePeers.size() && changroup < MAX_CHANGROUPS) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);

        return remote->chanGroups[changroup].params.sendMainMix;
    }
    return 0;
}


bool SonobusAudioProcessor::insertRemotePeerChannelGroup(int index, int atgroup, int chstart, int chcount)
{
    const ScopedReadLock sl (mCoreLock);
    if (index < mRemotePeers.size() && atgroup >= 0 && atgroup < MAX_CHANGROUPS) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        // push any existing ones down

        for (auto i = MAX_CHANGROUPS-1; i > atgroup; --i) {
            remote->chanGroups[i].copyParametersFrom(remote->chanGroups[i-1]);
            //mInputChannelGroups[i].chanStartIndex += chcount;
            //mInputChannelGroups[i-1].numChannels = std::max(1, std::min(count, MAX_CHANNELS));
        }
        remote->chanGroups[atgroup].params.chanStartIndex = chstart;
        remote->chanGroups[atgroup].params.numChannels = std::max(1, std::min(chcount, MAX_CHANNELS));
        remote->chanGroups[atgroup].params.panDestStartIndex = 0;
        remote->chanGroups[atgroup].params.panDestChannels = std::max(1, std::min(2, getTotalNumOutputChannels()));

        remote->modifiedChanGroups = true;
        remote->modifiedMultiChanGroups = true;
        // todo some defaults
    }

    return false;
}

bool SonobusAudioProcessor::removeRemotePeerChannelGroup(int index, int atgroup)
{
    const ScopedReadLock sl (mCoreLock);
    if (index < mRemotePeers.size() && atgroup >= 0 && atgroup < MAX_CHANGROUPS) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        // move any existing ones after up

        for (auto i = atgroup + 1; i < MAX_CHANGROUPS; ++i) {
            remote->chanGroups[i-1].copyParametersFrom(remote->chanGroups[i]);
            //mInputChannelGroups[i].chanStartIndex += chcount;
            //mInputChannelGroups[i-1].numChannels = std::max(1, std::min(count, MAX_CHANNELS));
        }
        remote->modifiedChanGroups = true;
        remote->modifiedMultiChanGroups = true;
    }

    return false;
}

bool SonobusAudioProcessor::copyRemotePeerChannelGroup(int index, int fromgroup, int togroup)
{
    const ScopedReadLock sl (mCoreLock);
    if (index < mRemotePeers.size() && fromgroup < MAX_CHANGROUPS && togroup < MAX_CHANGROUPS) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);

        remote->chanGroups[togroup].copyParametersFrom(remote->chanGroups[fromgroup]);
        remote->modifiedChanGroups = true;
        remote->modifiedMultiChanGroups = true;
    }
    return false;
}


int SonobusAudioProcessor::getRemotePeerRecvChannelCount(int index) const
{
    int ret = 0;
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        ret = remote->recvChannels;
    }
    return ret;
}

int SonobusAudioProcessor::getRemotePeerChannelGroupCount(int index) const
{
    int ret = 0;
    const ScopedReadLock sl (mCoreLock);
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        ret = remote->numChanGroups;
    }
    return ret;
}

bool SonobusAudioProcessor::getRemotePeerViewExpanded(int index) const
{
    bool ret = false;
    const ScopedReadLock sl (mCoreLock);
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        ret = remote->viewExpanded;
    }
    return ret;
}

void SonobusAudioProcessor::setRemotePeerViewExpanded(int index, bool expanded)
{
    const ScopedReadLock sl (mCoreLock);
    // could be -1 as index meaning all remote peers
    for (int i=0; i < mRemotePeers.size(); ++i) {
        if (index < 0 || index == i) {
            RemotePeer * remote = mRemotePeers.getUnchecked(i);
            remote->viewExpanded = expanded;
        }
    }
}

bool SonobusAudioProcessor::getLayoutFormatChangedForRemotePeer(int index) const
{
    bool ret = false;
    const ScopedReadLock sl (mCoreLock);
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        ret = remote->modifiedChanGroups;
    }
    return ret;
}

void SonobusAudioProcessor::restoreLayoutFormatForRemotePeer(int index)
{
    const ScopedReadLock sl (mCoreLock);
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        restoreLayoutFormatForPeer(remote, true);
    }
}


int SonobusAudioProcessor::getRemotePeerNominalSendChannelCount(int index) const
{
    int ret = 0;
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        ret = remote->nominalSendChannels;
    }
    return ret;    
}

void SonobusAudioProcessor::setRemotePeerNominalSendChannelCount(int index, int numchans)
{
    const ScopedReadLock sl (mCoreLock);        
    // could be -1 as index meaning all remote peers
    for (int i=0; i < mRemotePeers.size(); ++i) {
        if (index < 0 || index == i) {
            RemotePeer * remote = mRemotePeers.getUnchecked(i);
            remote->nominalSendChannels = numchans;
            updateRemotePeerSendChannels(i, remote);
        }
    }
    
    if (index < 0) {
        setupSourceFormat(nullptr, mAooCommonSource.get());
        int mainsendchans = mSendChannels.get() <= 0 ?  mActiveSendChannels : mSendChannels.get();
        mAooCommonSource->setup(mainsendchans, getSampleRate(), currSamplesPerBlock, 0);
    }
}

int SonobusAudioProcessor::getRemotePeerActualSendChannelCount(int index) const
{
    int ret = 0;
    const ScopedReadLock sl (mCoreLock);
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        ret = remote->sendChannels;
    }
    return ret;
}


void SonobusAudioProcessor::updateDynamicResampling()
{
    const ScopedReadLock sl (mCoreLock);
    bool newval = mDynamicResampling.get();
    for (int i=0; i < mRemotePeers.size(); ++i) {
        RemotePeer * remote = mRemotePeers.getUnchecked(i);
        remote->oursink->setDynamicResampling(newval ? 1 : 0);
        remote->oursource->setDynamicResampling(newval ? 1 : 0);
    }
}




int SonobusAudioProcessor::getRemotePeerOverrideSendChannelCount(int index) const
{
    int ret = 0;
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        ret = remote->sendChannelsOverride;
    }
    return ret;    
}



void SonobusAudioProcessor::setRemotePeerOverrideSendChannelCount(int index, int numchans)
{
    const ScopedReadLock sl (mCoreLock);        
    // could be -1 as index meaning all remote peers
    for (int i=0; i < mRemotePeers.size(); ++i) {
        if (index < 0 || index == i) {
            RemotePeer * remote = mRemotePeers.getUnchecked(i);
            remote->sendChannelsOverride = numchans;
            
            updateRemotePeerSendChannels(i, remote);
            
        }
    }
}

void SonobusAudioProcessor::updateRemotePeerSendChannels(int index, RemotePeer * remote) {
    // this is called while the corereadlock is already held
    int newchancnt = remote->sendChannels;

    if (remote->sendChannelsOverride < 0) {
        int totinchans = 0;
        for (int cgi=0; cgi < mInputChannelGroupCount && cgi < MAX_CHANGROUPS ; ++cgi) {
            totinchans += mInputChannelGroups[cgi].params.numChannels;
        }
        // met and file and soundboard
        if (mSendMet.get()) {
            totinchans += 1;
        }
        if (mSendPlaybackAudio.get()) {
            totinchans += mFilePlaybackChannelGroup.params.numChannels;
        }
        if (mSendSoundboardAudio.get()) {
            totinchans += soundboardChannelProcessor->getNumberOfChannels();
        }

        newchancnt = isAnythingRoutedToPeer(index) ? getMainBusNumOutputChannels() :  remote->nominalSendChannels <= 0 ? totinchans : remote->nominalSendChannels;
    }
    else {
        newchancnt = jmin(getMainBusNumOutputChannels(), remote->sendChannelsOverride);
    }
    
    if (remote->sendChannels != newchancnt) {
        remote->sendChannels = newchancnt;
        DBG("Peer " << index << "  has new sendChannel count: " << remote->sendChannels);
        if (remote->oursource) {
            setupSourceFormat(remote, remote->oursource.get());
            //setupSourceUserFormat(remote, remote->oursource.get());

            remote->oursource->setup(remote->sendChannels, getSampleRate(), currSamplesPerBlock, 0);

            updateRemotePeerUserFormat(index);
        }
    }
}

void SonobusAudioProcessor::changeListenerCallback (ChangeBroadcaster* source)
{
    if (source == &mTransportSource) {
        if (!mTransportSource.isPlaying() && mTransportSource.getCurrentPosition() >= mTransportSource.getLengthInSeconds()) {
            // at end, return to start
            mTransportSource.setPosition(0.0);
        }

#if 0
        if (mSendChannels.get() == 0) {
            if (mTransportSource.isPlaying() && mSendPlaybackAudio.get()) {
                // override sending
                int srcchans = mCurrentAudioFileSource ? mCurrentAudioFileSource->getAudioFormatReader()->numChannels : 2;
                setRemotePeerOverrideSendChannelCount(-1, jmax(getMainBusNumInputChannels(), srcchans));
            }
            else if (!mTransportSource.isPlaying()) {
                // remove override
                setRemotePeerOverrideSendChannelCount(-1, -1);
            }
        }
#endif
    }
}

void SonobusAudioProcessor::setRemotePeerBufferTime(int index, float bufferMs)
{
    const ScopedReadLock sl (mCoreLock);        
    if (index >= 0 && index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        remote->buffertimeMs = bufferMs;
        remote->totalEstLatency = remote->smoothPingTime.xbar + 2*remote->buffertimeMs + (1e3*currSamplesPerBlock/getSampleRate());
        remote->oursink->setLatency(remote->buffertimeMs * 1e-3);
        remote->fillRatioSlow.reset();
        remote->fillRatio.reset();
        remote->netBufAutoBaseline = (1e3*currSamplesPerBlock/getSampleRate()); // at least a process block
        remote->latencyDirty = true;
        remote->autoNetbufInitCompleted = false;
        remote->lastDroptime = Time::getMillisecondCounterHiRes();
        remote->resetSafetyMuted = bufferMs == 0.0f;
        remote->latencyMatched = false;

        //if (remote->autosizeBufferMode == AutoNetBufferModeOff) {
        // clear drop count
        remote->dataPacketsResent = 0;
        remote->dataPacketsDropped = 0;
        remote->resetDroptime = remote->lastDroptime;
        remote->fastDropRate.resetInitVal(0.0f);
        remote->lastDropCount = 0;
        //}

        if (remote->hasRealLatency) {
            remote->totalEstLatency = remote->totalLatency + (remote->buffertimeMs - remote->bufferTimeAtRealLatency);
        }

        sendRemotePeerInfoUpdate(index);
    }
}

float SonobusAudioProcessor::getRemotePeerBufferTime(int index) const
{
    float buftimeMs = 30.0f;
    
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        buftimeMs = remote->buffertimeMs;
        // reflect actual truth
        buftimeMs = jmax((double)buftimeMs, 1000.0f * currSamplesPerBlock / getSampleRate());
    }
    return buftimeMs;    
}

void SonobusAudioProcessor::setRemotePeerAutoresizeBufferMode(int index, SonobusAudioProcessor::AutoNetBufferMode flag)
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        remote->autosizeBufferMode = flag;
        
        if (flag == AutoNetBufferModeAutoFull) {
            remote->netBufAutoBaseline = (1e3*currSamplesPerBlock/getSampleRate()); // at least a process block
        } else if (flag == AutoNetBufferModeInitAuto) {
            setRemotePeerBufferTime(index, 0.0f); // reset to zero and start it over
        }

        // clear drop count
        remote->dataPacketsResent = 0;
        remote->dataPacketsDropped = 0;
        remote->lastDropCount = 0;
        remote->resetDroptime = Time::getMillisecondCounterHiRes();
        remote->fastDropRate.resetInitVal(0.0f);

        //remote->lastDroptime = 0;
    }
}

SonobusAudioProcessor::AutoNetBufferMode SonobusAudioProcessor::getRemotePeerAutoresizeBufferMode(int index, bool & initCompleted) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        initCompleted = remote->autoNetbufInitCompleted;
        return remote->autosizeBufferMode;
    }
    return AutoNetBufferModeOff;    
}

// acceptable limit for drop rate in dropinstance/second, above which it will adjust the jitter buffer in Auto modes
void SonobusAudioProcessor::setAutoresizeBufferDropRateThreshold(float thresh)
{
    mAutoresizeDropRateThresh = thresh;
}



bool SonobusAudioProcessor::getRemotePeerReceiveBufferFillRatio(int index, float & retratio, float & retstddev) const
{
    retratio = 0.0f;
    retstddev = 0.0f;
    const ScopedReadLock sl (mCoreLock);
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        retratio = remote->fillRatio.xbar;
        retstddev = remote->fillRatioSlow.s2xx;
        return true;
    }
    return false;
}

void SonobusAudioProcessor::setRemotePeerRecvActive(int index, bool active)
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);

        //remote->recvActive = active;

        if (active) {
            remote->recvAllow = true;
            remote->recvAllowCache = true;            
        }
        
        // TODO
#if 1
        if (active) {
            DBG("inviting peer " <<  remote->ourId << " source " << remote->remoteSourceId);
            AooEndpoint aep = { remote->endpoint->address.address_ptr(), (AooAddrSize) remote->endpoint->address.length(), remote->remoteSourceId };
            remote->oursink->inviteSource(aep, nullptr);
        } else {
            DBG("uninviting peer " <<  remote->ourId << " source " << remote->remoteSourceId);
            AooEndpoint aep = { remote->endpoint->address.address_ptr(), (AooAddrSize) remote->endpoint->address.length(), remote->remoteSourceId };
            remote->oursink->uninviteSource(aep);
        }
#endif
    }
}

bool SonobusAudioProcessor::getRemotePeerRecvActive(int index) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return remote->recvActive;
    }
    return false;        
}

void SonobusAudioProcessor::setRemotePeerSendAllow(int index, bool allow, bool cached)
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size() && index >= 0) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        if (cached) {
            remote->sendAllowCache = allow;
        } else {
            remote->sendAllow = allow;
            
            
            if (!allow) {
                //remote->oursource->remove_all();
                setRemotePeerSendActive(index, allow);
            }
        }
    }
}

bool SonobusAudioProcessor::getRemotePeerSendAllow(int index, bool cached) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return cached ? remote->sendAllowCache : remote->sendAllow;
    }
    return false;        
}

void SonobusAudioProcessor::setRemotePeerRecvAllow(int index, bool allow, bool cached)
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size() && index >= 0) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);

        if (cached) {
            remote->recvAllowCache = allow;            
        }
        else {
            remote->recvAllow = allow;
            
            if (!allow) {
                setRemotePeerRecvActive(index, allow);
            }
        }
    }
}

bool SonobusAudioProcessor::getRemotePeerRecvAllow(int index, bool cached) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return cached ? remote->recvAllowCache : remote->recvAllow;
    }
    return false;        
}

void SonobusAudioProcessor::setRemotePeerSoloed(int index, bool soloed)
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        
        remote->soloed = soloed;            
    }
    
    bool anysoloed = mMainMonitorSolo.get();
    for (auto & remote : mRemotePeers) 
    {
        if (remote->soloed) {
            anysoloed = true;
            break;
        }
    }
    mAnythingSoloed = anysoloed;
}

bool SonobusAudioProcessor::getRemotePeerSoloed(int index) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return  remote->soloed;
    }
    return false;            
}


int64_t SonobusAudioProcessor::getRemotePeerPacketsReceived(int index) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return remote->dataPacketsReceived;
    }
    return 0;      
}

int64_t SonobusAudioProcessor::getRemotePeerPacketsSent(int index) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return remote->dataPacketsSent;
    }
    return 0;      
}

int64_t SonobusAudioProcessor::getRemotePeerBytesSent(int index) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return remote->endpoint->sentBytes;
    }
    return 0;          
}

int64_t SonobusAudioProcessor::getRemotePeerBytesReceived(int index) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return remote->endpoint->recvBytes;
    }
    return 0;      
}

int64_t  SonobusAudioProcessor::getRemotePeerPacketsDropped(int index) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return remote->dataPacketsDropped;
    }
    return 0;      
}

int64_t  SonobusAudioProcessor::getRemotePeerPacketsResent(int index) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return remote->dataPacketsResent;
    }
    return 0;      
}

bool SonobusAudioProcessor::getRemotePeerSafetyMuted(int index) const
{
    const ScopedReadLock sl (mCoreLock);
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return remote->resetSafetyMuted;
    }
    return false;
}

bool SonobusAudioProcessor::getRemotePeerBlockedUs(int index) const
{
    const ScopedReadLock sl (mCoreLock);
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return remote->blockedUs;
    }
    return false;
}

void  SonobusAudioProcessor::resetRemotePeerPacketStats(int index)
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        remote->dataPacketsResent = 0;
        remote->dataPacketsDropped = 0;
        remote->resetDroptime = Time::getMillisecondCounterHiRes();
        remote->fastDropRate.resetInitVal(0.0f);
    }
}


bool SonobusAudioProcessor::getRemotePeerLatencyInfo(int index, LatencyInfo & retinfo) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);

        retinfo.pingMs = remote->smoothPingTime.xbar;

        if (remote->hasRemoteInfo) {
            float buftimeMs = jmax((double)remote->buffertimeMs, 1000.0f * currSamplesPerBlock / getSampleRate());
            auto halfping = retinfo.pingMs*0.5f;
            auto absizeMs = 1e3*currSamplesPerBlock/getSampleRate();
            int sendformatIndex = remote->formatIndex;
            if (sendformatIndex < 0 || sendformatIndex >= mAudioFormats.size()) sendformatIndex = 4; //emergency default
            const AudioCodecFormatInfo & sendformatinfo =  mAudioFormats.getReference(sendformatIndex);
            auto sendcodecLat = sendformatinfo.codec == CodecOpus ? 2.5f : 0.0f; // Opus adds codec latency
            auto recvcodecLat = remote->recvFormat.codec == CodecOpus ? 2.5f : 0.0f; // Opus adds codec latency

            // new style
            retinfo.incomingMs = /*absizeMs + */ recvcodecLat +  remote->remoteInLatMs + halfping + buftimeMs;
            retinfo.outgoingMs = /*absizeMs + */ sendcodecLat +  remote->remoteOutLatMs  +  halfping  + remote->remoteJitterBufMs;
            retinfo.jitterMs =  2 * remote->fillRatioSlow.s2xx * buftimeMs; // can't find a good estimate for this yet

            retinfo.isreal = true;
            retinfo.estimated = false;
            retinfo.legacy = false;
            retinfo.totalRoundtripMs =  retinfo.incomingMs + retinfo.outgoingMs;
        }
        else {
            // OLD LEGACY TEST MODE
            if (remote->hasRealLatency) {
                retinfo.estimated = remote->latencyDirty;
                retinfo.isreal = true;
                if (remote->latencyDirty) {
                    // is a good estimate
                    retinfo.totalRoundtripMs = remote->totalEstLatency;
                }
                else {
                    retinfo.totalRoundtripMs = remote->totalLatency;
                }

            }
            else {
                retinfo.isreal = false;
                retinfo.estimated = true;
                retinfo.totalRoundtripMs = remote->totalEstLatency ;
            }

            // given a roundtrip, a ping time, and our known internal buffering latency
            // we can get decent estimates for the each-direction latencies
            // outgoing = internal_input_latency + 0.5*pingtime + ??? remote buffering + ??? remote_output_latency
            // incoming = internal_output_latency + 0.5*pingtime + receivejitterbuffer + ??? remote_input_latency
            // roundtrip = incoming + outgoing
            // reflect actual truth
            float buftimeMs = jmax((double)remote->buffertimeMs, 1000.0f * currSamplesPerBlock / getSampleRate());

            retinfo.incomingMs = 2e3*currSamplesPerBlock/getSampleRate() + retinfo.pingMs*0.5f + buftimeMs;
            retinfo.outgoingMs = /* unknwon_remote_output_latency + */  retinfo.totalRoundtripMs - retinfo.incomingMs;
            retinfo.jitterMs =  2 * remote->fillRatioSlow.s2xx * buftimeMs; // can't find a good estimate for this yet
            retinfo.legacy = true;
        }

        return true;
    }
    return false;          
}

bool SonobusAudioProcessor::isAnyRemotePeerRecording() const
{
    const ScopedReadLock sl (mCoreLock);
    for (int index=0; index < mRemotePeers.size(); ++index) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        if (remote->remoteIsRecording) return true;
    }
    return false;
}

bool  SonobusAudioProcessor::isRemotePeerRecording(int index) const
{
    const ScopedReadLock sl (mCoreLock);
    if (index > 0 && index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return remote->remoteIsRecording;
    }
    return false;
}



void SonobusAudioProcessor::setRemotePeerSendActive(int index, bool active)
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        AooEndpoint aend = {remote->endpoint->address.address(), (AooAddrSize)remote->endpoint->address.length(), remote->remoteSinkId};
        remote->sendActive = active;
        if (active) {
            remote->sendAllow = true; // implied
            remote->sendAllowCache = true; // implied
            remote->oursource->startStream(nullptr);
            mAooCommonSource->activate(aend, true); // maybe remove/add instead? XXX
        } else {
            remote->oursource->stopStream();
            mAooCommonSource->activate(aend, false);
        }
    }
}

bool SonobusAudioProcessor::getRemotePeerSendActive(int index) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return remote->sendActive;
    }
    return false;        
}

void SonobusAudioProcessor::setRemotePeerConnected(int index, bool active)
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        remote->connected = active;
    }
}

bool SonobusAudioProcessor::getRemotePeerConnected(int index) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return remote->connected;
    }
    return false;        
}


bool SonobusAudioProcessor::getRemotePeerAddressInfo(int index, String & rethost, int & retport) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        rethost = remote->endpoint->ipaddr;
        retport = remote->endpoint->port;
        return true;
    }    
    return false;
}


SonobusAudioProcessor::RemotePeer *  SonobusAudioProcessor::findRemotePeer(EndpointState * endpoint, int32_t ourId)
{
    const ScopedReadLock sl (mCoreLock);        

    RemotePeer * retpeer = 0;

    for (auto s : mRemotePeers) {
        if (s->endpoint == endpoint && (ourId < 0 || s->ourId == ourId)) {
            retpeer = s;
            break;
        }
    }
        
    return retpeer;        
}

SonobusAudioProcessor::RemotePeer *  SonobusAudioProcessor::findRemotePeerByRemoteSourceId(EndpointState * endpoint, int32_t sourceId)
{
    const ScopedReadLock sl (mCoreLock);        

    RemotePeer * retpeer = 0;

    for (auto s : mRemotePeers) {
        if (s->endpoint == endpoint && s->remoteSourceId == sourceId) {
            retpeer = s;
            break;
        }
    }
        
    return retpeer;        
}

SonobusAudioProcessor::RemotePeer *  SonobusAudioProcessor::findRemotePeerByRemoteSinkId(EndpointState * endpoint, int32_t sinkId)
{
    const ScopedReadLock sl (mCoreLock);        

    RemotePeer * retpeer = 0;

    for (auto s : mRemotePeers) {
        if (s->endpoint == endpoint && s->remoteSinkId == sinkId) {
            retpeer = s;
            break;
        }
    }
        
    return retpeer;        
}



SonobusAudioProcessor::RemotePeer * SonobusAudioProcessor::doAddRemotePeerIfNecessary(EndpointState * endpoint, int32_t ourId, AooId userid, const String & username, const String & groupname, AooId groupid)
{
    const ScopedReadLock sl (mCoreLock);

    RemotePeer * retpeer = nullptr;
    bool doadd = true;

    for (auto s : mRemotePeers) {
        if (s->endpoint == endpoint /*&& s->ourId == ourId */) {
            doadd = false;
            retpeer = s;
            break;
        }
    }

    
    if (doadd) {
        // find free id
        int32_t newid = userid >= 0 ? userid : 1;
        bool hasit = false;
        while (!hasit) {
            bool safe = true;
            for (auto s : mRemotePeers) {
                if (s->ourId == newid) {
                    safe = false;
                    ++newid;
                    break;
                }
            }
            if (safe) hasit = true;
        }
        
        adjustRemoteSendMatrix(mRemotePeers.size(), false);

        retpeer = new RemotePeer(endpoint, newid);


        retpeer->userName = username;
        retpeer->userId = userid;
        retpeer->groupName = groupname;
        retpeer->groupId = groupid;

        retpeer->buffertimeMs = mBufferTime.get() * 1000.0f;
        retpeer->formatIndex = mDefaultAudioFormatIndex;
        retpeer->autosizeBufferMode = (AutoNetBufferMode) defaultAutoNetbufMode;

        retpeer->resetDroptime = Time::getMillisecondCounterHiRes();
        retpeer->fastDropRate.resetInitVal(0.0f);

        // default
        retpeer->numChanGroups = 1;
        retpeer->chanGroups[0].params.numChannels = 0;
        retpeer->chanGroups[0].params.gain = 1.0f;

        retpeer->gain = mDefUserLevel.get();

        findAndLoadCacheForPeer(retpeer);

        if (retpeer->autosizeBufferMode == AutoNetBufferModeInitAuto) {
            retpeer->buffertimeMs = 0;
        }

        retpeer->resetSafetyMuted = retpeer->buffertimeMs < 3.0f;

        retpeer->oursinkpp.processor = this;
        retpeer->oursink->getId(retpeer->oursinkpp.id);
        retpeer->oursink->setEventHandler(
                                           [](void *user, const AooEvent *event, int32_t level){
            auto * pp = static_cast<ProcessorIdPair *>(user);
            pp->processor->handleAooSinkEvent(event, level, pp->id);
        }, &retpeer->oursinkpp, kAooEventModePoll);


        retpeer->oursourcepp.processor = this;
        retpeer->oursource->getId(retpeer->oursourcepp.id);
        retpeer->oursource->setEventHandler(
                                           [](void *user, const AooEvent *event, int32_t level){
            auto * pp = static_cast<ProcessorIdPair *>(user);
            pp->processor->handleAooSourceEvent(event, level, pp->id);
        }, &retpeer->oursourcepp, kAooEventModePoll);


        retpeer->blockedUs = false;

        retpeer->oursink->setup(getMainBusNumOutputChannels(), getSampleRate(), currSamplesPerBlock, 0);
        //retpeer->oursink->setBufferSize(retpeer->buffertimeMs * 1e-3);
        retpeer->oursink->setLatency(retpeer->buffertimeMs * 1e-3);

        //int32_t flags = AOO_PROTOCOL_FLAG_COMPACT_DATA;
        //retpeer->oursink->set_option(aoo_opt_protocol_flags, &flags, sizeof(int32_t));

        retpeer->nominalSendChannels = mSendChannels.get();
        retpeer->sendChannels =  mSendChannels.get() <= 0 ?  mActiveSendChannels : mSendChannels.get();

        setupSourceFormat(retpeer, retpeer->oursource.get());
        float sendbufsize = jmax(10.0, SENDBUFSIZE_SCALAR * 1000.0f * currSamplesPerBlock / getSampleRate());
        retpeer->oursource->setup(retpeer->sendChannels, getSampleRate(), currSamplesPerBlock, 0);
        retpeer->oursource->setBufferSize(sendbufsize * 1e-3);
        retpeer->oursource->setPacketSize(retpeer->packetsize);
        //setupSourceUserFormat(retpeer, retpeer->oursource.get());
        
        retpeer->oursource->setPingInterval(2);

        //retpeer->oursource->set_respect_codec_change_requests(1);
        
        
        retpeer->oursink->setDynamicResampling(mDynamicResampling.get() ? 1 : 0);
        retpeer->oursource->setDynamicResampling(mDynamicResampling.get() ? 1 : 0);

        
        retpeer->workBuffer.setSize(2, currSamplesPerBlock, false, false, true);

        int outchannels = getMainBusNumOutputChannels();
        
        retpeer->recvMeterSource.resize (outchannels, meterRmsWindow);
        retpeer->sendMeterSource.resize (retpeer->sendChannels, meterRmsWindow);

        retpeer->sendAllow = !mMainSendMute.get();
        retpeer->sendAllowCache = true; // cache is allowed for new ones, so when it is unmuted it actually does
        
        retpeer->recvAllow = !mMainRecvMute.get();
        retpeer->recvAllowCache = true;
        
        retpeer->lastSendPingTimeMs = Time::getMillisecondCounterHiRes() - PEER_PING_INTERVAL_MS/2; // so that first ping doesn't happen immediately
        retpeer->haveSentFirstPeerInfo = false;

        //retpeer->chanGroups[0].init(getSampleRate());
        for (auto chgrpi = 0; /*chgrpi < s->numChanGroups && */ chgrpi < MAX_CHANGROUPS; ++chgrpi) {
            retpeer->chanGroups[chgrpi].init(getSampleRate());
        };


        // now add it, once initialized
        {
            const ScopedWriteLock slw (mCoreLock);

            mAooClient->addSink(retpeer->oursink.get(), retpeer->oursinkpp.id);

            mAooClient->addSource(retpeer->oursource.get(), retpeer->oursourcepp.id);

            mRemotePeers.add(retpeer);
        }


        if (retpeer->sendAllow) {
            retpeer->oursource->startStream(nullptr);

            retpeer->sendActive = true;
        } else {
            retpeer->sendActive = false;
        }

        //updateRemotePeerUserFormat(mRemotePeers.size()-1);

    }
    else if (retpeer) {
        DBG("Remote peer already exists, setting name and group");
        if (username.isNotEmpty() && retpeer->userName.isEmpty()) {
            // but set it's name and group
            retpeer->userName = username;
            retpeer->groupName = groupname;
            
            if (findAndLoadCacheForPeer(retpeer)) {
                
                setupSourceFormat(retpeer, retpeer->oursource.get());

                retpeer->oursink->setLatency(retpeer->buffertimeMs * 1e-3);
                
                for (auto i=0; i < retpeer->numChanGroups && i < MAX_CHANGROUPS; ++i) {
                    retpeer->chanGroups[i].commitCompressorParams();
                    retpeer->chanGroups[i].commitExpanderParams();
                    retpeer->chanGroups[i].commitEqParams();
                }
            }

        }
    }
    
    return retpeer;    
}

void SonobusAudioProcessor::commitCacheForPeer(RemotePeer * retpeer)
{
    if (retpeer->userName.isEmpty()) {
        DBG("username empty, can't commit");
        return;
    }

    PeerStateCache newcache;
    newcache.netbuf = retpeer->buffertimeMs;
    newcache.netbufauto = retpeer->autosizeBufferMode;
    newcache.name = retpeer->userName;
    newcache.sendFormat = retpeer->formatIndex;
    newcache.numChanGroups = retpeer->numChanGroups;
    newcache.mainGain = retpeer->gain;
    newcache.numMultiChanGroups = retpeer->lastMultiNumChanGroups;
    newcache.modifiedChanGroups = retpeer->modifiedMultiChanGroups;
    newcache.orderPriority = retpeer->orderPriority;

    for (int i=0; i < retpeer->numChanGroups && i < MAX_CHANGROUPS; ++i) {
        newcache.channelGroupParams[i] = retpeer->chanGroups[i].params;
    }
    for (int i=0; i < retpeer->lastMultiNumChanGroups && i < MAX_CHANGROUPS; ++i) {
        newcache.channelGroupMultiParams[i] = retpeer->lastMultiChanParams[i];
    }

    PeerStateCacheMap::iterator found  = mPeerStateCacheMap.find(retpeer->userName);
    
    if (found != mPeerStateCacheMap.end()) {
        found->second = newcache;
    }
    else {
        mPeerStateCacheMap.insert(PeerStateCacheMap::value_type(retpeer->userName, newcache));
    }
}

bool SonobusAudioProcessor::findAndLoadCacheForPeer(RemotePeer * retpeer)
{
    if (retpeer->userName.isEmpty()) {
        DBG("username empty, can't match");
        return false;
    }
    
    // look for current peer by user name in peer cache and apply settings
    PeerStateCacheMap::iterator found  = mPeerStateCacheMap.find(retpeer->userName);
    if (found == mPeerStateCacheMap.end()) {
        // no exact match, look for ones starting with the same beginning
        String namebase = retpeer->userName;
        StringArray nametoks = StringArray::fromTokens(retpeer->userName, false);
        if (nametoks.size() > 1) {
            nametoks.remove(nametoks.size()-1);
            namebase = nametoks.joinIntoString(" ").trim();
        }
        
        for (PeerStateCacheMap::iterator iter = mPeerStateCacheMap.begin(); iter != mPeerStateCacheMap.end(); ++iter) {
            if (iter->first.startsWith(namebase)) {
                // close match
                DBG("Found close peer match: " << namebase << "  with cachename: " << iter->first);
                found = iter;
                break;
            }
        }
    }
    else {
        DBG("Found exact peer match: " << retpeer->userName);
    }
    
    if (found != mPeerStateCacheMap.end()) {
        const PeerStateCache & cache = found->second;
        retpeer->autosizeBufferMode = (AutoNetBufferMode) cache.netbufauto;
        retpeer->buffertimeMs = cache.netbuf;
        retpeer->formatIndex = cache.sendFormat;
        retpeer->numChanGroups = cache.numChanGroups; // restore this?
        retpeer->gain = cache.mainGain;
        retpeer->lastMultiNumChanGroups = cache.numMultiChanGroups;
        retpeer->modifiedChanGroups = retpeer->modifiedMultiChanGroups = cache.modifiedChanGroups;
        retpeer->orderPriority  = cache.orderPriority;


        for (int i=0; i < retpeer->numChanGroups  && i < MAX_CHANGROUPS; ++i) {
            retpeer->chanGroups[i].params = cache.channelGroupParams[i];
        }

        for (int i=0; i < retpeer->lastMultiNumChanGroups  && i < MAX_CHANGROUPS; ++i) {
            retpeer->lastMultiChanParams[i] = cache.channelGroupMultiParams[i];
        }

        sendRemotePeerInfoUpdate(-1, retpeer); // send to this peer

        return true;
    }
    return false;
}


bool SonobusAudioProcessor::removeAllRemotePeersWithEndpoint(EndpointState * endpoint)
{
    const ScopedReadLock sl (mCoreLock);

    bool didremove = false;

    OwnedArray<RemotePeer> removed;

    // go from end, so deletions don't mess it up
    for (int i = mRemotePeers.size()-1; i >= 0;  --i) {
        auto * s = mRemotePeers.getUnchecked(i);
        if (s->endpoint == endpoint) {
            
            if (s->connected) {
                disconnectRemotePeer(i);
            }
            
            adjustRemoteSendMatrix(i, true);

            commitCacheForPeer(s);

            didremove = true;

            {
                const ScopedWriteLock slw (mCoreLock);


                mAooClient->removeSink(s->oursink.get());

                mAooClient->removeSource(s->oursource.get());

                removed.add(mRemotePeers.removeAndReturn(i));
            }
        }
    }

    // remote peers will be deleted when removed list goes out of scope

    return didremove;
}


bool SonobusAudioProcessor::doRemoveRemotePeerIfNecessary(EndpointState * endpoint, int32_t ourId)
{
    const ScopedReadLock sl (mCoreLock);

    bool didremove = false;
    OwnedArray<RemotePeer> removed;

    int i=0;
    for (auto s : mRemotePeers) {
        if (s->endpoint == endpoint && s->ourId == ourId) {
            didremove = true;
            commitCacheForPeer(s);

            {
                const ScopedWriteLock slw (mCoreLock);
                mAooClient->removeSink(s->oursink.get());

                mAooClient->removeSource(s->oursource.get());

                removed.add(mRemotePeers.removeAndReturn(i));
            }
            break;
        }
        ++i;
    }
    
    return didremove;
    
}

bool SonobusAudioProcessor::isAnythingRoutedToPeer(int index) const
{
    bool ret = false;

    for (int i=0; i < mRemotePeers.size(); ++i) {
        if (mRemoteSendMatrix[i][index]) {
            ret = true;
            break;
        }
    }

    return ret;
}



////

bool SonobusAudioProcessor::formatInfoToAooFormat(const AudioCodecFormatInfo & info, int channels, AooFormatStorage & retformat) {
                
        if (info.codec == CodecPCM) {
            AooFormatPcm *fmt = (AooFormatPcm *)&retformat;

            AooFormatPcm_init(fmt, channels, getSampleRate(), currSamplesPerBlock >= info.min_preferred_blocksize ? currSamplesPerBlock : info.min_preferred_blocksize, info.bitdepth == 2 ? kAooPcmInt16 : info.bitdepth == 3 ? kAooPcmInt24 : info.bitdepth == 4 ? kAooPcmFloat32 : info.bitdepth == 8 ? kAooPcmFloat64 : kAooPcmInt16);

            return true;
        } 
        else if (info.codec == CodecOpus) {
            AooFormatOpus *fmt = (AooFormatOpus *)&retformat;
            AooFormatOpus_init(fmt, channels, getSampleRate(), currSamplesPerBlock >= info.min_preferred_blocksize ? currSamplesPerBlock : info.min_preferred_blocksize, OPUS_APPLICATION_RESTRICTED_LOWDELAY);

            return true;
        }
    
    return false;
}
    
    
void SonobusAudioProcessor::setupSourceFormat(SonobusAudioProcessor::RemotePeer * peer, AooSource * source, bool latencymode)
{
    // have choice and parameters
    int formatIndex = (!peer || peer->formatIndex < 0) ? mDefaultAudioFormatIndex : peer->formatIndex;
    if (formatIndex < 0 || formatIndex >= mAudioFormats.size()) formatIndex = 4; //emergency default
    const AudioCodecFormatInfo & info =  mAudioFormats.getReference(formatIndex);
    
    AooFormatStorage f;
    int channels = latencymode ? 1  :  peer ? peer->sendChannels :  mSendChannels.get() <= 0 ?  mActiveSendChannels : mSendChannels.get();
    
    if (formatInfoToAooFormat(info, channels, f)) {        
        source->setFormat(f.header);

        if (info.codec == CodecOpus) {
            // set these this other way
            AooSource_setOpusComplexity(source, 0, info.complexity);
            AooSource_setOpusSignalType(source, 0, info.signal_type);
            AooSource_setOpusBitrate(source, 0, info.bitrate);
        }
    }
}

ValueTree SonobusAudioProcessor::getSendUserFormatLayoutTree()
{
    // get userformat from send info
    ValueTree fmttree(channelLayoutsKey);

    if (mSendChannels.get() == 1 || mSendChannels.get() == 2) {
        // not multichannel, this is a mixdown
        ChannelGroupParams tmpgrp;
        tmpgrp.chanStartIndex = 0;
        tmpgrp.numChannels = mSendChannels.get();
        fmttree.appendChild(tmpgrp.getChannelLayoutValueTree(), nullptr);
    }
    else {
        int chstart = 0;
        for (int i=0; i < mInputChannelGroupCount; ++i) {
            ChannelGroupParams tmpgrp = mInputChannelGroups[i].params;
            tmpgrp.chanStartIndex = chstart;
            fmttree.appendChild(tmpgrp.getChannelLayoutValueTree(), nullptr);

            chstart += tmpgrp.numChannels;
        }
        // add met and file playback and soundboard if necessary
        if (mSendMet.get()) {
            ChannelGroupParams tmpgrp = mMetChannelGroup.params;
            tmpgrp.chanStartIndex = chstart;
            fmttree.appendChild(tmpgrp.getChannelLayoutValueTree(), nullptr);
            chstart += tmpgrp.numChannels;
        }
        if (mSendPlaybackAudio.get()) {
            ChannelGroupParams tmpgrp = mFilePlaybackChannelGroup.params;
            tmpgrp.chanStartIndex = chstart;
            fmttree.appendChild(tmpgrp.getChannelLayoutValueTree(), nullptr);
            chstart += tmpgrp.numChannels;
        }
        if (mSendSoundboardAudio.get()) {
            ChannelGroupParams tmpgrp = soundboardChannelProcessor->getChannelGroupParams();
            tmpgrp.chanStartIndex = chstart;
            fmttree.appendChild(tmpgrp.getChannelLayoutValueTree(), nullptr);
            chstart += tmpgrp.numChannels;
        }
    }

    return fmttree;
}


void SonobusAudioProcessor::setupSourceUserFormat(RemotePeer * peer, AooSource * source)
{
    // get userformat from send info
    ValueTree fmttree = getSendUserFormatLayoutTree();

    MemoryBlock destData;
    MemoryOutputStream stream(destData, false);

    DBG("setup source user format data: " << fmttree.toXmlString());

    fmttree.writeToStream(stream);

    JUCE_COMPILER_WARNING ("TODO: USERFORMAT");
   // source->set_userformat(destData.getData(), (int32_t) destData.getSize());
}

void SonobusAudioProcessor::updateRemotePeerUserFormat(int index, RemotePeer * onlypeer)
{
    // get userformat from send info
    ValueTree fmttree = getSendUserFormatLayoutTree();

    MemoryBlock destData;
    MemoryOutputStream stream(destData, false);

    DBG("format data: " << fmttree.toXmlString());

    fmttree.writeToStream(stream);

    char buf[AOO_MAX_PACKET_SIZE];


    if (destData.getSize() > AOO_MAX_PACKET_SIZE - 100) {
        DBG("Info too big for packet!");
        return;
    }


    const ScopedReadLock sl (mCoreLock);
    for (int i=0;  i < mRemotePeers.size(); ++i) {
        auto * peer = mRemotePeers.getUnchecked(i);
        if (index >= 0 && index != i) continue;
        if (onlypeer && onlypeer != peer) continue;

        osc::OutboundPacketStream msg(buf, sizeof(buf));

        try {
            msg << osc::BeginMessage(SONOBUS_FULLMSG_LAYOUTINFO)
            << peer->remoteSinkId
            << osc::Blob(destData.getData(), (int) destData.getSize())
            << osc::EndMessage;
        } catch (const osc::Exception& e){
            DBG("exception in osc LAYOUTINFO: " << e.what());
            continue;
        }

        DBG("Sending channellayout message to " << peer->userId);
        this->sendPeerMessage(peer, (AooByte*)msg.Data(), (int32_t) msg.Size());

        if (onlypeer && onlypeer == peer) break;
        if (index >= 0 && index == i) break;
    }

}

void SonobusAudioProcessor::restoreLayoutFormatForPeer(RemotePeer * remote, bool resetmulti)
{
    DBG("Restoring layout userformat for peer" );
    remote->numChanGroups = remote->origNumChanGroups;

    for (int i=0; i < MAX_CHANGROUPS && i < remote->numChanGroups; ++i) {
        remote->chanGroups[i].params = remote->origChanParams[i];

        // possibly reset monitoring pan
        if (remote->chanGroups[i].params.numChannels > remote->chanGroups[i].params.panDestChannels) {
            // set the monDestChannel count to be equal (but not more than total output channels)
            remote->chanGroups[i].params.panDestChannels = jmin(remote->chanGroups[i].params.numChannels, getTotalNumOutputChannels());
            remote->chanGroups[i].params.panDestStartIndex = jmax(0, jmin(remote->chanGroups[i].params.panDestStartIndex,
                                                                          getTotalNumOutputChannels() - remote->chanGroups[i].params.panDestChannels));
        }

        remote->chanGroups[i].commitAllParams();
    }

    remote->modifiedChanGroups = false;
    if (resetmulti) {
        remote->modifiedMultiChanGroups = false;
    }
}

void SonobusAudioProcessor::applyLayoutFormatToPeer(RemotePeer * remote, const ValueTree & valtree)
{
    DBG("Got layout userformat for peer: " << valtree.toXmlString());


    // apply this valtree to the channelgroups for this peer
    for (int i=0; i < valtree.getNumChildren(); ++i) {
        const auto & child = valtree.getChild(i);
        if (i < MAX_CHANGROUPS) {
            // first copy from our active
            remote->origChanParams[i] = remote->chanGroups[i].params;
            // then override with stuff from value tree
            remote->origChanParams[i].setFromChannelLayoutValueTree(child);
        }
    }

    remote->origNumChanGroups = jmin(valtree.getNumChildren(), MAX_CHANGROUPS);

    // check conditions for applying these changes
    bool doapply = !remote->modifiedChanGroups;

    int origchans = 0;
    for (int i=0; i < remote->origNumChanGroups; ++i) {
        origchans += remote->origChanParams[i].numChannels;
    }

    int modchans = 0;
    for (int i=0; i < remote->numChanGroups; ++i) {
        modchans += remote->chanGroups[i].params.numChannels;
    }

    // if the total number of channels changed do apply
    if (modchans != origchans) {
        doapply = true;
    }

    // if the new source (origchans) is now <= 2 and previously it was more
    // save our old one to multichan state for possible later restore
    if (origchans <= 2 && modchans > 2 && remote->modifiedMultiChanGroups) {
        DBG("Saving last multchan");
        for (int i=0; i < remote->numChanGroups; ++i) {
            remote->lastMultiChanParams[i] = remote->chanGroups[i].params;
        }
        remote->lastMultiNumChanGroups = remote->numChanGroups;
    }
    else if (origchans > 2 && modchans <= 2 && remote->lastMultiNumChanGroups > 0 && remote->modifiedMultiChanGroups) {
        // if it's now switched from mono/stereo to multichannel, possibly restore last multichanparams
        int multchans = 0;
        for (int i=0; i < remote->lastMultiNumChanGroups; ++i) {
            multchans += remote->lastMultiChanParams[i].numChannels;
        }

        if (multchans == origchans) {
            DBG("Restoring last saved multichannel");
            for (int i=0; i < remote->numChanGroups; ++i) {
                remote->chanGroups[i].params = remote->lastMultiChanParams[i];
                remote->chanGroups[i].commitAllParams();
            }
            remote->numChanGroups = remote->lastMultiNumChanGroups;
            remote->modifiedChanGroups = true;
            doapply = false;
        }
    }


    if (doapply) {
        restoreLayoutFormatForPeer(remote);
    }
}



void SonobusAudioProcessor::parameterChanged (const String &parameterID, float newValue)
{
    if (parameterID == paramDry) {
        mDry = newValue;
    }
    else if (parameterID == paramInGain) {
        //mInGain = newValue; // NO LONGER USE GLOBAL mInGain

        // for now just apply it to the first input channel group
        //mInputChannelGroups[0].gain = newValue;
    }
    else if (parameterID == paramMetGain) {
        mMetGain = newValue;
        //mMetChannelGroup.params.gain = newValue;
    }
    else if (parameterID == paramMetTempo) {
        mMetTempo = newValue;
    }
    else if (parameterID == paramMetEnabled) {
        mMetEnabled = newValue > 0;
    }
    else if (parameterID == paramDynamicResampling) {
        mDynamicResampling = newValue > 0;
        updateDynamicResampling();
    }
    else if (parameterID == paramAutoReconnectLast) {
        mAutoReconnectLast = newValue > 0;
    }
    else if (parameterID == paramDefaultPeerLevel) {
        mDefUserLevel = newValue;
    }
    else if (parameterID == paramSyncMetToHost) {
        mSyncMetToHost = newValue > 0;
    }
    else if (parameterID == paramSyncMetToFilePlayback) {
        mSyncMetStartToPlayback = newValue > 0;
    }
    else if (parameterID == paramSendChannels) {
        mSendChannels = (int) newValue;
        
        setRemotePeerNominalSendChannelCount(-1, mSendChannels.get());
        updateRemotePeerUserFormat();
    }
    else if (parameterID == paramMainReverbSize)
    {
        mMainReverbSize = newValue;
        mMainReverbParams.roomSize = jmap(mMainReverbSize.get(), 0.55f, 1.0f); //  mMainReverbSize.get() * 0.55f + 0.45f;
        mReverbParamsChanged = true;
        
        mMReverb.setParameter(MVerbFloat::SIZE, jmap(mMainReverbSize.get(), 0.45f, 0.95f)); 
        mMReverb.setParameter(MVerbFloat::DECAY, jmap(mMainReverbSize.get(), 0.45f, 0.95f));

        
        //mZitaControl.setParamValue("/Zita_Rev1/Decay_Times_in_Bands_(see_tooltips)/Low_RT60", jlimit(1.0f, 8.0f, mMainReverbSize.get() * 7.0f + 1.0f));
        //mZitaControl.setParamValue("/Zita_Rev1/Decay_Times_in_Bands_(see_tooltips)/Mid_RT60", jlimit(1.0f, 8.0f, mMainReverbSize.get() * 7.0f + 1.0f));
        mZitaControl.setParamValue("/Zita_Rev1/Decay_Times_in_Bands_(see_tooltips)/Low_RT60", jlimit(1.0f, 8.0f, mMainReverbSize.get() * 7.0f + 1.0f));
        mZitaControl.setParamValue("/Zita_Rev1/Decay_Times_in_Bands_(see_tooltips)/Mid_RT60", jlimit(1.0f, 8.0f, mMainReverbSize.get() * 7.0f + 1.0f));

        //mMainReverb->setParameters(mMainReverbParams);
    }
    else if (parameterID == paramMainReverbLevel)
    {
        mMainReverbLevel = newValue;
        mMainReverbParams.wetLevel = mMainReverbLevel.get() * 0.35f;
        mReverbParamsChanged = true;
        mMReverb.setParameter(MVerbFloat::GAIN, jmap(mMainReverbLevel.get(), 0.0f, 0.8f)); 

        //mZitaControl.setParamValue("/Zita_Rev1/Output/Level", jlimit(-70.0f, 40.0f, Decibels::gainToDecibels(mMainReverbLevel.get()) + 0.0f));
        mZitaControl.setParamValue("/Zita_Rev1/Output/Level", jlimit(-70.0f, 40.0f, Decibels::gainToDecibels(mMainReverbLevel.get()) + 6.0f));
        //mMainReverb->setParameters(mMainReverbParams);
    }
    else if (parameterID == paramMainReverbDamping)
    {
        mMainReverbDamping = newValue;
        mMainReverbParams.damping = mMainReverbDamping.get();
        mReverbParamsChanged = true;
        mZitaControl.setParamValue("/Zita_Rev1/Decay_Times_in_Bands_(see_tooltips)/HF_Damping",                                    
                                   jmap(mMainReverbDamping.get(), 23520.0f, 1500.0f));
        mMReverb.setParameter(MVerbFloat::DAMPINGFREQ, jmap(mMainReverbDamping.get(), 0.0f, 0.85f));                

    }
    else if (parameterID == paramMainReverbPreDelay)
    {
        mMainReverbPreDelay = newValue;
        mZitaControl.setParamValue("/Zita_Rev1/Input/In_Delay",                                    
                                   jlimit(0.0f, 100.0f, mMainReverbPreDelay.get()));      
        mMReverb.setParameter(MVerbFloat::PREDELAY, jmap(mMainReverbPreDelay.get(), 0.0f, 100.0f, 0.0f, 0.5f)); // takes 0->1  where = 200ms

    }
    else if (parameterID == paramMainReverbEnabled) {
        mMainReverbEnabled = newValue > 0;
    }
    else if (parameterID == paramMainReverbModel) {
        mMainReverbModel = (int) newValue;

        mReverbParamsChanged = true;        
    }
    else if (parameterID == paramInputReverbSize)
    {
        mInputReverbSize = newValue;

        mInputReverb.setParameter(MVerbFloat::SIZE, jmap(mInputReverbSize.get(), 0.45f, 0.95f));
        mInputReverb.setParameter(MVerbFloat::DECAY, jmap(mInputReverbSize.get(), 0.45f, 0.95f));
    }
    else if (parameterID == paramInputReverbLevel)
    {
        mInputReverbLevel = newValue;
        mInputReverb.setParameter(MVerbFloat::GAIN, jmap(mInputReverbLevel.get(), 0.0f, 0.8f));
    }
    else if (parameterID == paramInputReverbDamping)
    {
        mInputReverbDamping = newValue;
        mInputReverb.setParameter(MVerbFloat::DAMPINGFREQ, jmap(mInputReverbDamping.get(), 0.0f, 0.85f));

    }
    else if (parameterID == paramInputReverbPreDelay)
    {
        mInputReverbPreDelay = newValue;
        mInputReverb.setParameter(MVerbFloat::PREDELAY, jmap(mInputReverbPreDelay.get(), 0.0f, 100.0f, 0.0f, 0.5f)); // takes 0->1  where = 200ms
    }

    else if (parameterID == paramSendFileAudio) {
        mSendPlaybackAudio = newValue > 0;

#if 0
        if (mTransportSource.isPlaying() && mSendPlaybackAudio.get()) {
            // override sending
            int srcchans = mCurrentAudioFileSource ? mCurrentAudioFileSource->getAudioFormatReader()->numChannels : 2;
            setRemotePeerOverrideSendChannelCount(-1, jmax(getMainBusNumInputChannels(), srcchans)); // should be something different?
        }
        else if (!mSendPlaybackAudio.get()) {
            // remove override
            setRemotePeerOverrideSendChannelCount(-1, -1); 
        }
#endif
    }
    else if (parameterID == paramSendSoundboardAudio) {
        mSendSoundboardAudio = newValue > 0;
    }
    else if (parameterID == paramHearLatencyTest) {
        mHearLatencyTest = newValue > 0;
    }
    else if (parameterID == paramMetIsRecorded) {
        mMetIsRecorded = newValue > 0;
    }
    else if (parameterID == paramSendMetAudio) {
        mSendMet = newValue > 0;
    }
    else if (parameterID == paramMainInMute) {
        mMainInMute = newValue > 0;
    }
    else if (parameterID == paramMainMonitorSolo) {
        mMainMonitorSolo = newValue > 0;

        bool anysoloed = mMainMonitorSolo.get();
        for (int i=0; i < getNumberRemotePeers(); ++i) {
            if (getRemotePeerSoloed(i)) {
                anysoloed = true;
                break;
            }
        }
        mAnythingSoloed = anysoloed;
    }
    else if (parameterID == paramMainSendMute) {
        // allow or disallow sending to all peers

        mMainSendMute = newValue > 0;
                       
        for (int i=0; i < getNumberRemotePeers(); ++i) {
            bool connected  = getRemotePeerConnected(i);
            bool isGroupPeer = getRemotePeerUserName(i).isNotEmpty();
            
            if (!connected && !isGroupPeer) {
                /*
                 String hostname;
                 int port = 0;
                 processor.getRemotePeerAddressInfo(i, hostname, port);
                 processor.connectRemotePeer(hostname, port);
                 */
            }
            else
            {
                if (!mMainSendMute.get()) {
                    // turns on allow and starts sending
                    // actually restores from cached values
                    bool cached = getRemotePeerSendAllow(i, true);

                    if (cached) {
                        setRemotePeerSendActive(i, cached);
                    } else {
                        setRemotePeerSendAllow(i, cached);                        
                    }
                } else {
                    // turns off sending and allow

                    // set cached value
                    bool curr = getRemotePeerSendAllow(i);
                    setRemotePeerSendAllow(i, curr, true); // set cached only

                    setRemotePeerSendAllow(i, false);
                }
            }
        }
    }
    else if (parameterID == paramMainRecvMute) {
        // allow or disallow sending to all peers

        mMainRecvMute = newValue > 0;
                       
        for (int i=0; i < getNumberRemotePeers(); ++i) {
            bool connected  = getRemotePeerConnected(i);
            bool isGroupPeer = getRemotePeerUserName(i).isNotEmpty();
            
            if (!connected && !isGroupPeer) {
                /*
                 String hostname;
                 int port = 0;
                 processor.getRemotePeerAddressInfo(i, hostname, port);
                 processor.connectRemotePeer(hostname, port);
                 */
            }
            else
            {
                if (!mMainRecvMute.get()) {
                    // turns on allow and allows receiving
                    // actually restores from cached values
                    bool cached = getRemotePeerRecvAllow(i, true);
                    setRemotePeerRecvActive(i, cached);
                } else {
                    // turns off sending and allow
                    // set cached value
                    bool curr = getRemotePeerRecvAllow(i);
                    setRemotePeerRecvAllow(i, curr, true); // set cached only
                    
                    setRemotePeerRecvAllow(i, false);
                }
            }
        }
    }
    else if (parameterID == paramWet) {
        mWet = newValue;
    }
    else if (parameterID == paramInMonitorMonoPan) {
        // old one
        mInMonMonoPan = newValue;
        mInputChannelGroups[0].params.pan[0] = newValue; // XXX
    }
    else if (parameterID == paramInMonitorPan1) {
        // old one
        mInMonPan1 = newValue;
        mInputChannelGroups[0].params.panStereo[0] = newValue;
    }
    else if (parameterID == paramInMonitorPan2) {
        // old one
        mInMonPan2 = newValue;
        mInputChannelGroups[0].params.panStereo[1] = newValue;
    }
    else if (parameterID == paramDefaultSendQual) {
        mDefaultAudioFormatIndex = (int) newValue;
        
        if (mChangingDefaultAudioCodecChangesAll) {
            for (int i=0; i < getNumberRemotePeers(); ++i) {
                setRemotePeerAudioCodecFormat(i, mDefaultAudioFormatIndex);
            }
        }
        
    }
    else if (parameterID == paramDefaultNetbufMs) {
        mBufferTime = newValue;
    }
    else if (parameterID == paramDefaultAutoNetbuf) {
        defaultAutoNetbufMode = (int) newValue;
    }

}

//==============================================================================
const String SonobusAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SonobusAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SonobusAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SonobusAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SonobusAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SonobusAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SonobusAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SonobusAudioProcessor::setCurrentProgram (int index)
{
}

const String SonobusAudioProcessor::getProgramName (int index)
{
    return {};
}

void SonobusAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void SonobusAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    bool blocksizechanged = lastSamplesPerBlock != samplesPerBlock;

    int inchannels =  getTotalNumInputChannels(); // getMainBusNumInputChannels();
    int outchannels = getMainBusNumOutputChannels();


    lastSamplesPerBlock = currSamplesPerBlock = samplesPerBlock;

    DBG("Prepare to play: SR " <<  sampleRate << "  prevrate: " << mPrevSampleRate <<  "  blocksize: " <<  samplesPerBlock << "  totinch: " << getTotalNumInputChannels() << "  mbinch: " << getMainBusNumInputChannels() << "  outch: " << getMainBusNumOutputChannels());
    DBG("  numinbuses: " << getBusCount(true) << "  numoutbuses: " << getBusCount(false));

    const ScopedReadLock sl (mCoreLock);        
    
    mMetronome->setSampleRate(sampleRate);
    mMainReverb->setSampleRate(sampleRate);
    mMReverb.setSampleRate(sampleRate);
    mInputReverb.setSampleRate(sampleRate);

    mZitaReverb.init(sampleRate);
    mZitaReverb.buildUserInterface(&mZitaControl);

    //DBG("Zita Reverb Params:");
    //for(int i=0; i < mZitaControl.getParamsCount(); i++){
    //    DBG(mZitaControl.getParamAddress(i));
    //}
    
    // setting default values for the Faust module parameters
    mZitaControl.setParamValue("/Zita_Rev1/Output/Dry/Wet_Mix", -1.0f);
    mZitaControl.setParamValue("/Zita_Rev1/Decay_Times_in_Bands_(see_tooltips)/Low_RT60", jlimit(1.0f, 8.0f, mMainReverbSize.get() * 7.0f + 1.0f));
    mZitaControl.setParamValue("/Zita_Rev1/Decay_Times_in_Bands_(see_tooltips)/Mid_RT60", jlimit(1.0f, 8.0f, mMainReverbSize.get() * 7.0f + 1.0f));
    mZitaControl.setParamValue("/Zita_Rev1/Output/Level", jlimit(-70.0f, 40.0f, Decibels::gainToDecibels(mMainReverbLevel.get()) + 6.0f));
    mZitaControl.setParamValue("/Zita_Rev1/Decay_Times_in_Bands_(see_tooltips)/HF_Damping",                                    
                               jmap(mMainReverbDamping.get(), 23520.0f, 1500.0f));


    //



    
    mMReverb.setParameter(MVerbFloat::MIX, 1.0f); // full wet
    mMReverb.setParameter(MVerbFloat::GAIN, jmap(mMainReverbLevel.get(), 0.0f, 0.8f)); 
    mMReverb.setParameter(MVerbFloat::SIZE, jmap(mMainReverbSize.get(), 0.45f, 0.95f)); 
    mMReverb.setParameter(MVerbFloat::DECAY, jmap(mMainReverbSize.get(), 0.45f, 0.95f));
    mMReverb.setParameter(MVerbFloat::EARLYMIX, 0.75f);
    mMReverb.setParameter(MVerbFloat::PREDELAY, jmap(mMainReverbPreDelay.get(), 0.0f, 100.0f, 0.0f, 0.5f)); // takes 0->1  where = 200ms
    mMReverb.setParameter(MVerbFloat::DAMPINGFREQ, mMainReverbDamping.get());
    mMReverb.setParameter(MVerbFloat::DAMPINGFREQ, jmap(mMainReverbDamping.get(), 0.0f, 0.85f));                
    mMReverb.setParameter(MVerbFloat::BANDWIDTHFREQ, 1.0f);
    mMReverb.setParameter(MVerbFloat::DENSITY, 0.5f);


    mInputReverb.setParameter(MVerbFloat::MIX, 1.0f); // full wet
    mInputReverb.setParameter(MVerbFloat::GAIN, jmap(mInputReverbLevel.get(), 0.0f, 0.8f));
    mInputReverb.setParameter(MVerbFloat::SIZE, jmap(mInputReverbSize.get(), 0.45f, 0.95f));
    mInputReverb.setParameter(MVerbFloat::DECAY, jmap(mInputReverbSize.get(), 0.45f, 0.95f));
    mInputReverb.setParameter(MVerbFloat::EARLYMIX, 0.75f);
    mInputReverb.setParameter(MVerbFloat::PREDELAY, jmap(mInputReverbPreDelay.get(), 0.0f, 100.0f, 0.0f, 0.5f)); // takes 0->1  where = 200ms
    mInputReverb.setParameter(MVerbFloat::DAMPINGFREQ, mInputReverbDamping.get());
    mInputReverb.setParameter(MVerbFloat::DAMPINGFREQ, jmap(mInputReverbDamping.get(), 0.0f, 0.85f));
    mInputReverb.setParameter(MVerbFloat::BANDWIDTHFREQ, 1.0f);
    mInputReverb.setParameter(MVerbFloat::DENSITY, 0.5f);

    
    mMainReverbParams.roomSize = jmap(mMainReverbSize.get(), 0.55f, 1.0f);
    mMainReverb->setParameters(mMainReverbParams);



    mTransportSource.prepareToPlay(currSamplesPerBlock, getSampleRate());

    //mAooSource->set_format(fmt->header);
    setupSourceFormat(0, mAooCommonSource.get());
    int mainsendchans = mSendChannels.get() <= 0 ?  mActiveSendChannels : mSendChannels.get();
    mAooCommonSource->setup( mainsendchans, sampleRate, samplesPerBlock, 0);
    mAooCommonSource->startStream(nullptr);


    if (lastInputChannels == 0 || lastOutputChannels == 0 || mInputChannelGroupCount == 0) {
        // first time these are set, do some initialization

        lastInputChannels = inchannels;
        lastOutputChannels = outchannels;

        if (mInputChannelGroupCount == 0) {
            mInputChannelGroupCount = 1;
            mInputChannelGroups[0].params.chanStartIndex = 0;
            mInputChannelGroups[0].params.numChannels = getMainBusNumInputChannels(); // default to only as many channels as the main input bus has
            mInputChannelGroups[0].params.monDestStartIndex = 0;
            mInputChannelGroups[0].params.monDestChannels = jmin(2, outchannels);
            mInputChannelGroups[0].commitMonitorDelayParams(); // need to do this too
        }
    }
    else if (lastInputChannels != inchannels || lastOutputChannels != outchannels) {

        lastInputChannels = inchannels;
        lastOutputChannels = outchannels;
    }

    //if (mInputChannelGroupCount == 0) {
    //    mInputChannelGroupCount = inchannels;
    //}

    // can't be more than number of inchannels
    //mInputChannelGroupCount = jmin(inchannels, mInputChannelGroupCount);
    mInputChannelGroupCount = jmin(MAX_CHANGROUPS, mInputChannelGroupCount);


    for (int i=0; /*i < mInputChannelGroupCount && */ i < MAX_CHANGROUPS; ++i) {
        mInputChannelGroups[i].init(sampleRate);
    }

    meterRmsWindow = sampleRate * METER_RMS_SEC / currSamplesPerBlock;

    int totsendchans = 0;
    int fileplaychans = mCurrentAudioFileSource ? mCurrentAudioFileSource->getAudioFormatReader()->numChannels : 2;
    int soundboardchans = soundboardChannelProcessor->getFileSourceNumberOfChannels();

    soundboardChannelProcessor->prepareToPlay(sampleRate, meterRmsWindow, currSamplesPerBlock);

    for (int cgi=0; cgi < mInputChannelGroupCount && cgi < MAX_CHANGROUPS ; ++cgi) {
        totsendchans += mInputChannelGroups[cgi].params.numChannels;
    }
    // plus a possible metronome send
    if (mSendMet.get()) {
        totsendchans += 1;
    }
    if (mSendPlaybackAudio.get()) {
        // plus a possible file sending
        totsendchans += fileplaychans;
    }
    if (mSendSoundboardAudio.get()) {
        // plus a possible soundboard sending
        totsendchans += soundboardchans;
    }

    int realsendchans = mSendChannels.get() <= 0 ? totsendchans : mSendChannels.get();
    mActiveSendChannels = totsendchans;

    inputMeterSource.resize (inchannels, meterRmsWindow);
    outputMeterSource.resize (outchannels, meterRmsWindow);
    postinputMeterSource.resize (totsendchans, meterRmsWindow);
    metMeterSource.resize (1, 2*meterRmsWindow);
    filePlaybackMeterSource.resize (fileplaychans, meterRmsWindow);

    if (sendMeterSource.getNumChannels() < realsendchans) {
        sendMeterSource.resize (realsendchans, meterRmsWindow);
    }

    setupSourceFormatsForAll();

    
    ensureBuffers(samplesPerBlock);


    mMetChannelGroup.init(sampleRate);
    mFilePlaybackChannelGroup.init(sampleRate);
    mRecMetChannelGroup.init(sampleRate);
    mRecFilePlaybackChannelGroup.init(sampleRate);


    if (lrintf(mPrevSampleRate) != lrintf(sampleRate) || blocksizechanged) {


        // reset all initial auto if blocksize changed
        if (blocksizechanged) {
            for (int i = 0; i < mRemotePeers.size(); ++i) {
                bool initCompleted;
                if (getRemotePeerAutoresizeBufferMode(i, initCompleted) == SonobusAudioProcessor::AutoNetBufferModeInitAuto) {
                    float buftime = 0.0;
                    setRemotePeerBufferTime(i, buftime);
                }
            }
        }

        // reset all incoming by toggling muting
        if (!mMainRecvMute.get()) {
            DBG("Toggling main recv mute");
            mState.getParameter(paramMainRecvMute)->setValueNotifyingHost(1.0f);
            mPendingUnmuteAtStamp = Time::getMillisecondCounter() + 250;
            mPendingUnmute = true;
        }

        mPrevSampleRate = sampleRate;
    }

    sendRemotePeerInfoUpdate();
}

void SonobusAudioProcessor::setupSourceFormatsForAll()
{
    const ScopedReadLock sl (mCoreLock);
    //const ScopedLock slformat (mSourceFormatLock);

    double sampleRate = getSampleRate();
    int inchannels = mActiveSendChannels; // getTotalNumInputChannels(); // getMainBusNumInputChannels();
    int outchannels = getMainBusNumOutputChannels();

    int mainsendchans = mSendChannels.get() <= 0 ?  mActiveSendChannels : mSendChannels.get();
    mAooCommonSource->setup(mainsendchans, sampleRate, currSamplesPerBlock, 0);
    setupSourceFormat(nullptr, mAooCommonSource.get());
    
    int i=0;
    for (auto s : mRemotePeers) {
        if (s->workBuffer.getNumSamples() < currSamplesPerBlock) {
            s->workBuffer.setSize(jmax(2, s->recvChannels), currSamplesPerBlock, false, false, true);
        }

        s->sendChannels = isAnythingRoutedToPeer(i) ? outchannels : s->nominalSendChannels <= 0 ? inchannels : s->nominalSendChannels;
        if (s->sendChannelsOverride > 0) {
            s->sendChannels = jmin(outchannels, s->sendChannelsOverride);
        }

        if (s->oursource) {
            setupSourceFormat(s, s->oursource.get());
            //setupSourceUserFormat(s, s->oursource.get());

            s->oursource->setup(s->sendChannels, sampleRate, currSamplesPerBlock, 0);  // todo use inchannels maybe?
            float sendbufsize = jmax(10.0, SENDBUFSIZE_SCALAR * 1000.0f * currSamplesPerBlock / getSampleRate());
            s->oursource->setBufferSize(sendbufsize * 1e-3);

        }
        if (s->oursink) {
            const ScopedWriteLock sl (s->sinkLock);
            int sinkchan = jmax(outchannels, s->recvChannels);
            s->oursink->setup(sinkchan, sampleRate, currSamplesPerBlock, 0);
        }

        s->recvMeterSource.resize (s->recvChannels, meterRmsWindow);
        //s->sendMeterSource.resize (s->sendChannels, meterRmsWindow);

        // XXX
        for (auto chgrpi = 0; /*chgrpi < s->numChanGroups && */ chgrpi < MAX_CHANGROUPS; ++chgrpi) {
            s->chanGroups[chgrpi].init(sampleRate);
        };

        // for now the first channel group has them all
        //s->chanGroups[0].init(sampleRate);
        //s->chanGroups[0].numChannels = s->recvChannels;

        ++i;
    }

    updateRemotePeerUserFormat();

}


void SonobusAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    mTransportSource.releaseResources();
    soundboardChannelProcessor->releaseResources();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SonobusAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
#else
    

    auto plugtype = PluginHostType::getPluginLoadedAs();

    if (plugtype == AudioProcessor::wrapperType_VST) {
        // for now only allow mono or stereo usage with VST2
        // It really only exists for compatibility with OBS

        if (layouts.getMainInputChannelSet() != AudioChannelSet::mono()
            && layouts.getMainInputChannelSet() != AudioChannelSet::stereo()
            && layouts.getMainInputChannelSet() != AudioChannelSet::disabled()
            ) {
            return false;
        }
        if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
            && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo()
            && layouts.getMainOutputChannelSet() != AudioChannelSet::disabled()) {
            return false;
        }
    }

    return true;
  #endif
}
#endif

void SonobusAudioProcessor::ensureBuffers(int numSamples)
{
    auto mainBusNumInputChannels  = getTotalNumInputChannels(); // getMainBusNumInputChannels();
    auto mainBusNumOutputChannels = getTotalNumOutputChannels();
    auto maxchans = jmax(2, jmax(mainBusNumOutputChannels, mainBusNumInputChannels));

    int totsendchans = 0;
    int selfrecchans = 0;

    for (int cgi=0; cgi < mInputChannelGroupCount && cgi < MAX_CHANGROUPS ; ++cgi) {
        totsendchans += mInputChannelGroups[cgi].params.numChannels;
    }
    selfrecchans = totsendchans;

    // plus a possible metronome send
    if (mSendMet.get()) {
        totsendchans += 1;
    }
    int fileplaychans = mCurrentAudioFileSource ? mCurrentAudioFileSource->getAudioFormatReader()->numChannels : 2;
    int fileplaymaxchans = jmax(maxchans, fileplaychans);
    if (mSendPlaybackAudio.get()) {
        // plus a possible file sending
        totsendchans += fileplaychans;
    }

    meterRmsWindow = getSampleRate() * METER_RMS_SEC / currSamplesPerBlock;

    int soundboardplaychans = soundboardChannelProcessor->getFileSourceNumberOfChannels();

    soundboardChannelProcessor->ensureBuffers(numSamples, maxchans, meterRmsWindow);

    if (mSendSoundboardAudio.get()) {
        // plus a possible soundboard sending
        totsendchans += soundboardplaychans;
    }

    bool needpeersendupdate = false;
    if (mActiveSendChannels != totsendchans) {
        mActiveSendChannels = totsendchans;
        needpeersendupdate = true;
    }

    int realsendchans = mSendChannels.get() <= 0 ? totsendchans : mSendChannels.get();


    mActiveInputChannels = selfrecchans;

    if (sendMeterSource.getNumChannels() < realsendchans) {
        sendMeterSource.resize (realsendchans, meterRmsWindow);
    }

    if (filePlaybackMeterSource.getNumChannels() < fileplaychans) {
        filePlaybackMeterSource.resize (fileplaychans, meterRmsWindow);
    }


    auto maxworkbufchans = jmax(maxchans, totsendchans);

    if (tempBuffer.getNumSamples() < numSamples || tempBuffer.getNumChannels() < maxchans) {
        tempBuffer.setSize(maxchans, numSamples, false, false, true);
    }
    if (mixBuffer.getNumSamples() < numSamples || mixBuffer.getNumChannels() < maxchans) {
        mixBuffer.setSize(maxchans, numSamples, false, false, true);
    }
    if (workBuffer.getNumSamples() < numSamples || workBuffer.getNumChannels() != maxworkbufchans) {

        // only grow the work buffer channel count
        if (maxworkbufchans > workBuffer.getNumChannels() || workBuffer.getNumSamples() < numSamples) {
            workBuffer.setSize(maxworkbufchans, numSamples, false, false, true);
        }

        needpeersendupdate = true;
    }
    if (inputBuffer.getNumSamples() < numSamples || inputBuffer.getNumChannels() != maxchans) {
        inputBuffer.setSize(maxchans, numSamples, false, false, true);
    }
    if (monitorBuffer.getNumSamples() < numSamples || monitorBuffer.getNumChannels() != maxchans) {
        monitorBuffer.setSize(maxchans, numSamples, false, false, true);
    }
    if (sendWorkBuffer.getNumSamples() < numSamples || sendWorkBuffer.getNumChannels() != maxworkbufchans) {
        sendWorkBuffer.setSize(maxworkbufchans, numSamples, false, false, true);
    }
    if (inputPostBuffer.getNumSamples() < numSamples || inputPostBuffer.getNumChannels() != totsendchans) {
        inputPostBuffer.setSize(totsendchans, numSamples, false, false, true);
        inputPreBuffer.setSize(totsendchans, numSamples, false, false, true);
    }
    if (fileBuffer.getNumSamples() < numSamples || fileBuffer.getNumChannels() != fileplaymaxchans) {
        fileBuffer.setSize(fileplaymaxchans, numSamples, false, false, true);
    }
    if (metBuffer.getNumSamples() < numSamples || metBuffer.getNumChannels() != maxchans) {
        metBuffer.setSize(maxchans, numSamples, false, false, true);
    }
    if (mainFxBuffer.getNumSamples() < numSamples || mainFxBuffer.getNumChannels() != maxchans) {
        mainFxBuffer.setSize(maxchans, numSamples, false, false, true);
    }
    if (inputRevBuffer.getNumSamples() < numSamples || inputRevBuffer.getNumChannels() != maxchans) {
        inputRevBuffer.setSize(maxchans, numSamples, false, false, true);
    }
    if (silentBuffer.getNumSamples() < numSamples) {
        silentBuffer.setSize(1, numSamples, false, false, true);
        silentBuffer.clear();
    }

    if (needpeersendupdate) {
        const ScopedReadLock sl (mCoreLock);
        // could be -1 as index meaning all remote peers
        for (int i=0; i < mRemotePeers.size(); ++i) {
            RemotePeer * remote = mRemotePeers.getUnchecked(i);
            updateRemotePeerSendChannels(i, remote);
        }

        setupSourceFormat(nullptr, mAooCommonSource.get());
        int mainsendchans = mSendChannels.get() <= 0 ?  mActiveSendChannels : mSendChannels.get();
        mAooCommonSource->setup(mainsendchans, getSampleRate(), currSamplesPerBlock, 0);
    }

    mTempBufferSamples = jmax(mTempBufferSamples, numSamples);
    mTempBufferChannels = jmax(maxchans, mTempBufferChannels);
}


void SonobusAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    auto totalInputChannels  = getTotalNumInputChannels();
    auto mainBusInputChannels  = getMainBusNumInputChannels();
    auto mainBusOutputChannels = getMainBusNumOutputChannels();

    auto totalOutputChannels = getTotalNumOutputChannels();

    auto maxchans = jmax(2, jmax(totalInputChannels, mainBusOutputChannels));
    auto maxsendchans = jmax(2, jmax(totalInputChannels, mainBusOutputChannels));

    float inGain = mInGain.get();
    float drynow = mDry.get(); // DB_CO(dry_level);
    float wetnow = mWet.get(); // DB_CO(wet_level);
    float inmonMonoPan = mInMonMonoPan.get();
    float inmonPan1 = mInMonPan1.get();
    float inmonPan2 = mInMonPan2.get();

    int sendChans = mSendChannels.get();
    bool sendfileaudio = mSendPlaybackAudio.get();
    bool sendsoundboardaudio = mSendSoundboardAudio.get();
    bool sendmet = mSendMet.get();

    bool userwritingpossible = userWritingPossible.load();
    bool writingpossible = writingPossible.load();

    inGain = mMainInMute.get() ? 0.0f : inGain;

    drynow = (mAnythingSoloed.get() && !mMainMonitorSolo.get()) ? 0.0f : drynow;

    int numSamples = buffer.getNumSamples();


    if (numSamples != lastSamplesPerBlock) {
        //DBG("blocksize changed from " << lastSamplesPerBlock << " to " << numSamples);
        blocksizeCounter = 0;
    }
    else if (blocksizeCounter >= 0) {
        ++blocksizeCounter;

        if (blocksizeCounter > 30) { // change currblocksize if stable
            DBG("sample frames stabilized at: " << numSamples);
            currSamplesPerBlock = numSamples;
            blocksizeCounter = -1;
            mNeedsSampleSetup = true;

            //setupSourceFormatsForAll();
        }
    }


    int totsendchans = 0;
    for (auto i = 0; i < mInputChannelGroupCount && i < MAX_CHANGROUPS; ++i)
    {
        totsendchans += mInputChannelGroups[i].params.numChannels;
    }
    // plus a possible metronome send
    if (sendmet) {
        totsendchans += 1;
    }
    if (sendfileaudio) {
        // plus a possible file sending
        totsendchans += mCurrentAudioFileSource ? mCurrentAudioFileSource->getAudioFormatReader()->numChannels : 2;
    }
    if (sendsoundboardaudio) {
        // plus a possible soundboard sending
        totsendchans += soundboardChannelProcessor->getFileSourceNumberOfChannels();
    }

    int realsendchans = sendChans <= 0 ? totsendchans :sendChans;


    // THIS SHOULDN"T GENERALLY HAPPEN, it should have been taken care of in prepareToPlay, but just in case.
    // I know this isn't RT safe. UGLY... bad... badness.
    if (numSamples > mTempBufferSamples || maxchans > mTempBufferChannels || inputPostBuffer.getNumChannels() != totsendchans || inputPostBuffer.getNumSamples() < numSamples  || sendMeterSource.getNumChannels() < realsendchans) {
        ensureBuffers(numSamples);
    }

    double useBpm = mMetTempo.get();
    bool syncmethost = mSyncMetToHost.get();
    bool syncmetplayback = mSyncMetStartToPlayback.get();

    AudioPlayHead * playhead = getPlayHead();
    Optional<AudioPlayHead::PositionInfo> rposInfo;

    if (playhead) {
        rposInfo = playhead->getPosition();
    }

    bool hostPlaying = rposInfo && rposInfo->getIsPlaying();
    auto hostBpm = rposInfo->getBpm();
    if (hostBpm && *hostBpm > 0.0) {
        useBpm = *hostBpm;
    }

    if (syncmethost) {
        if (rposInfo && fabs(useBpm - mMetTempo.get()) > 0.001) {
            mMetTempo = useBpm;
            mTempoParameter->setValueNotifyingHost(mTempoParameter->convertTo0to1(mMetTempo.get()));
        }
    }
    
    // main bus only
    
    for (auto i = mainBusInputChannels; i < mainBusOutputChannels; ++i) {
        // replicate last input channel (basically only if it's the 2nd channel)
        if (mainBusInputChannels > 0 && i < 2) {
            buffer.copyFrom(i, 0, buffer, mainBusInputChannels-1, 0, numSamples);
        } else {
            buffer.clear (i, 0, numSamples);            
        }
    }
    
    // other buses need clearing
    for (auto i = jmax(mainBusOutputChannels, totalInputChannels); i < jmax(totalOutputChannels, totalInputChannels); ++i) {
        buffer.clear (i, 0, numSamples);            
    }
    
    
    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.


    uint64_t t = aoo::time_tag::now();

    // meter input pre everything
    inputMeterSource.measureBlock (buffer, 0, numSamples);


    inputPostBuffer.clear(0, numSamples);
    if (writingpossible && mRecordInputPreFX) {
        inputPreBuffer.clear(0, numSamples);
    }

    bool inReverbEnabled = false;
    for (auto i = 0; i < mInputChannelGroupCount && i < MAX_CHANGROUPS; ++i)
    {
        if (mInputChannelGroups[i].params.inReverbSend > 0.0f) {
            inReverbEnabled = true;
            break;
        }
    }

    bool doinreverb = inReverbEnabled || mLastInputReverbEnabled;
    int revfxchannels = 2;

    if (doinreverb) {
        inputRevBuffer.clear(0, numSamples);
    }


    // Input Gain and FX processing
    int destch = 0;
    for (auto i = 0; i < mInputChannelGroupCount && i < MAX_CHANGROUPS; ++i)
    {
        auto * revbuf = doinreverb ? &inputRevBuffer : nullptr;

        mInputChannelGroups[i].processBlock(buffer, inputPostBuffer, destch, mInputChannelGroups[i].params.numChannels, silentBuffer, numSamples, inGain,
                                            nullptr, revbuf, 0, revfxchannels, inReverbEnabled);

        if (writingpossible && mRecordInputPreFX) {
            // copy input as-is for later recording
            for (int ch = 0; ch < mInputChannelGroups[i].params.numChannels; ++ch) {
                int usech = mInputChannelGroups[i].params.chanStartIndex + ch;
                const auto * srcbuf = usech < buffer.getNumChannels() ? buffer.getReadPointer(usech) : silentBuffer.getReadPointer(0);
                inputPreBuffer.copyFrom(destch + ch, 0, srcbuf, numSamples);
            }
        }

        destch += mInputChannelGroups[i].params.numChannels;
    }


    postinputMeterSource.measureBlock (inputPostBuffer, 0, numSamples);


    // compressor makeup meter level per channel
    destch = 0;
    for (auto i = 0; i < mInputChannelGroupCount && i < MAX_CHANGROUPS; ++i) {
        float redlev = 1.0f;
        if (mInputChannelGroups[i].params.compressorParams.enabled && mInputChannelGroups[i].compressorOutputLevel) {
            redlev = jlimit(0.0f, 1.0f, Decibels::decibelsToGain(*mInputChannelGroups[i].compressorOutputLevel));
        }
        for (auto j=0; j < mInputChannelGroups[i].params.numChannels; ++j) {
            //int ch = mInputChannelGroups[i].chanStartIndex + j;
            int ch = destch + j;
            postinputMeterSource.setReductionLevel(ch, redlev);
        }
        destch += mInputChannelGroups[i].params.numChannels;
    }
    
    
    // do the input panning before everything else
    int sendCh = mSendChannels.get();
    //int sendPanChannels = sendCh == 0 ?  inputBuffer.getNumChannels() : jmin(inputBuffer.getNumChannels(), jmax(mainBusOutputChannels, sendCh));
    int sendPanChannels = sendCh == 0 ?  inputPostBuffer.getNumChannels() : jmin(sendWorkBuffer.getNumChannels(), sendCh);
    //int panChannels = jmin(inputBuffer.getNumChannels(), jmax(mainBusOutputChannels, sendCh));
    // if sending as mono, split the difference about applying gain attentuation for the number of input channels
    float tgain = sendPanChannels == 1 && inputPostBuffer.getNumChannels() > 0 ? (1.0f/std::max(1.0f, (float)(inputPostBuffer.getNumChannels() * 0.5f))) : 1.0f;

    sendWorkBuffer.clear(0, numSamples);

    if (sendPanChannels > 2 || sendCh == 0) {
        // copy straight-thru
        for (auto i = 0; i < sendWorkBuffer.getNumChannels() && i < inputPostBuffer.getNumChannels(); ++i) {
            sendWorkBuffer.copyFrom (i, 0, inputPostBuffer, i, 0, numSamples);
        }
    }
    else {
        int srcstart = 0;

        for (auto i = 0; i < mInputChannelGroupCount && i < MAX_CHANGROUPS; ++i)
        {
            // change dest ch target
            int dstch = mInputChannelGroups[i].params.panDestStartIndex;  // todo change dest ch target
            int dstcnt = jmin(sendPanChannels, mInputChannelGroups[i].params.panDestChannels);

            mInputChannelGroups[i].processPan(inputPostBuffer, srcstart, sendWorkBuffer, dstch, dstcnt, numSamples, tgain);
            srcstart += mInputChannelGroups[i].params.numChannels;
        }
    }

    // MAIN EFFECTS BUS
    bool mainReverbEnabled = mMainReverbEnabled.get();
    bool doreverb = mainReverbEnabled || mLastMainReverbEnabled;
    bool hasmainfx = doreverb;
    int fxchannels = 2;

    mainFxBuffer.clear(0, numSamples);


    // handle what's going to be monitored
    inputBuffer.clear(0, numSamples);

    bool anyinputsoloed = false;
    for (auto i = 0; i < mInputChannelGroupCount; ++i) {
        if (mInputChannelGroups[i].params.soloed) {
            anyinputsoloed = true;
            break;
        }
    }

    int monPanChannels = jmin(inputBuffer.getNumChannels(), totalOutputChannels);
    float tmgain = monPanChannels == 1 && mainBusInputChannels > 0 ? (1.0f/std::max(1.0f, (float)(mainBusInputChannels * 0.5f))): 1.0f;
    int srcstart = 0;
    for (auto i = 0; i < mInputChannelGroupCount; ++i)
    {
        float utmgain = anyinputsoloed && !mInputChannelGroups[i].params.soloed ? 0.0f : tmgain;
        int dstch = mInputChannelGroups[i].params.monDestStartIndex;
        int dstcnt = jmin(monPanChannels, mInputChannelGroups[i].params.monDestChannels);

        auto * revbuffer = doreverb ? &mainFxBuffer : nullptr;

        mInputChannelGroups[i].processMonitor(inputPostBuffer, srcstart,
                                              inputBuffer, dstch, dstcnt,
                                              numSamples, utmgain, nullptr, 
                                              revbuffer, 0, fxchannels, mainReverbEnabled, drynow);

        srcstart += mInputChannelGroups[i].params.numChannels;
    }

    // for multichannel send, met and file playback and soundboard follow the last input channel
    auto metstartch = srcstart;
    auto filestartch = sendmet ? metstartch + 1 : srcstart;

    // write out self-only output bus XXXX FIXME
    /*
    if (auto selfbus = getBus(false, OutSelfBusIndex)) {
        if (selfbus->isEnabled()) {
            int index = getChannelIndexInProcessBlockBuffer(false, OutSelfBusIndex, 0);
            int cnt = getChannelCountOfBus(false, OutSelfBusIndex);
            for (int i=0; i < cnt && i < inputPostBuffer.getNumChannels(); ++i) {
                int srcchan = i; //  i < mainBusInputChannels ? i : mainBusInputChannels - 1;
                buffer.copyFrom(index+i, 0, inputPostBuffer, srcchan, 0, numSamples);
            }
        }
    }
     */

    
    // file playback goes to everyone

    bool hasfiledata = false;
    double transportPos = mTransportSource.getCurrentPosition();
    int fileChannels = mCurrentAudioFileSource ? mCurrentAudioFileSource->getAudioFormatReader()->numChannels : 2;

    if (mTransportSource.getTotalLength() > 0)
    {
        AudioSourceChannelInfo info (&fileBuffer, 0, numSamples);
        mTransportSource.getNextAudioBlock (info);
        hasfiledata = true;

        filePlaybackMeterSource.measureBlock(fileBuffer);

        int srcchans = fileChannels;
        mFilePlaybackChannelGroup.params.numChannels = srcchans;
        mFilePlaybackChannelGroup.commitMonitorDelayParams(); // need to do this too

        mRecFilePlaybackChannelGroup.params.numChannels = srcchans;
        mRecFilePlaybackChannelGroup.commitMonitorDelayParams(); // need to do this too

        if (sendfileaudio) {

            //add to main buffer for going out, mix as appropriate depending on how many channels being sent
            if (sendPanChannels == 1) {
                float fgain = sendPanChannels == 1 && srcchans > 0 ? (1.0f/std::max(1.0f, (float)(srcchans))): 1.0f;
                fgain *= mFilePlaybackChannelGroup.params.gain;
                auto lastfgain = _lastfplaygain;

                for (int channel = 0; channel < srcchans; ++channel) {
                    //sendWorkBuffer.addFrom(0, 0, fileBuffer, channel, 0, numSamples, fgain);
                    sendWorkBuffer.addFromWithRamp(0, 0, fileBuffer.getReadPointer(channel), numSamples, fgain, lastfgain);
                }

                _lastfplaygain = fgain;
            }
            else if (sendPanChannels > 2){
                // straight-thru

                // copy straight-thru
                // find file channels TODO
                auto filech = filestartch; // XXX
                //sendWorkBuffer.addFrom (filech, 0, fileBuffer, 0, 0, numSamples);
                auto fgain = mFilePlaybackChannelGroup.params.gain;
                auto lastfgain = _lastfplaygain;

                for (int channel = 0; channel < srcchans && filech < sendWorkBuffer.getNumChannels(); ++channel) {
                    //sendWorkBuffer.addFrom(filech, 0, fileBuffer, channel, 0, numSamples);
                    sendWorkBuffer.addFromWithRamp(filech, 0, fileBuffer.getReadPointer(channel), numSamples, fgain, lastfgain);
                    ++filech;
                }

                _lastfplaygain = fgain;
            }
            else if (sendPanChannels == 2) {
                // change dest ch target
                int dstch = mFilePlaybackChannelGroup.params.panDestStartIndex;  // todo change dest ch target
                int dstcnt = jmin(sendPanChannels, mFilePlaybackChannelGroup.params.panDestChannels);
                auto fgain = mFilePlaybackChannelGroup.params.gain;

                mFilePlaybackChannelGroup.processPan(fileBuffer, 0, sendWorkBuffer, dstch, dstcnt, numSamples, fgain);

                _lastfplaygain = fgain;
            }
        }

    }

    bool hassoundboarddata = soundboardChannelProcessor->processAudioBlock(numSamples);
    if (hassoundboarddata && sendsoundboardaudio) {
        int startChannel = sendfileaudio ? filestartch + fileChannels : filestartch;
        soundboardChannelProcessor->sendAudioBlock(sendWorkBuffer, numSamples, sendPanChannels, startChannel);
    }

    // process metronome
    bool metenabled = mMetEnabled.get();
    float metgain = mMetGain.get();
    double mettempo = mMetTempo.get();
    bool metrecorded = mMetIsRecorded.get();
    bool dometfilesyncstart = syncmetplayback && mTransportWasPlaying != mTransportSource.isPlaying();
    bool syncmet = (syncmethost && hostPlaying) || (syncmetplayback && mTransportSource.isPlaying());

    if (dometfilesyncstart) {
        metenabled = mTransportSource.isPlaying();
        mMetEnabled = metenabled;
        // notify host
        mState.getParameter(paramMetEnabled)->setValueNotifyingHost(metenabled ? 1.0f : 0.0f);
    }

    if (metenabled != mLastMetEnabled) {
        if (metenabled) {
            mMetronome->setGain(metgain, true);
            if (!syncmet) {
                mMetronome->resetRelativeStart();
            }
        } else {
            metgain = 0.0f;
            mMetronome->setGain(0.0f);
        }
    }
    if (mLastMetEnabled || metenabled) {
        metBuffer.clear(0, numSamples);
        mMetronome->setGain(metgain);
        mMetronome->setTempo(mettempo);
        double beattime = 0.0;
        if (syncmethost && hostPlaying && rposInfo->getPpqPosition()) {
            beattime = *rposInfo->getPpqPosition();
        }
        else if (syncmetplayback && mTransportSource.isPlaying()) {
            beattime = (mettempo / 60.0) * transportPos;
        }
        mMetronome->processMix(numSamples, metBuffer.getWritePointer(0), metBuffer.getWritePointer(mainBusOutputChannels > 1 ? 1 : 0), beattime, !syncmet);

        //

        metMeterSource.measureBlock(metBuffer);

        if (sendmet) {

            if (sendPanChannels > 2) {
                // copy straight-thru
                // find Met channel TODO
                int metch = metstartch;
                sendWorkBuffer.addFrom (metch, 0, metBuffer, 0, 0, numSamples);
            }
            else {
                // change dest ch target
                int dstch = mMetChannelGroup.params.panDestStartIndex;  // todo change dest ch target
                int dstcnt = jmin(sendPanChannels, mMetChannelGroup.params.panDestChannels);

                mMetChannelGroup.processPan(metBuffer, 0, sendWorkBuffer, dstch, dstcnt, numSamples, 1.0f);
            }


            //add to main buffer for going out
            //for (int channel = 0; channel < mainBusOutputChannels; ++channel) {
            //    sendWorkBuffer.addFrom(channel, 0, metBuffer, channel, 0, numSamples);
            //}
        }
    }
    mLastMetEnabled = metenabled;


    // process and mix in input reverb into sendworkbuffer (if sending mono or stereo)
    if (doinreverb) {

        if (inReverbEnabled != mLastInputReverbEnabled && inReverbEnabled) {
            mInputReverb.reset();
        }

        mInputReverb.process((float **)inputRevBuffer.getArrayOfWritePointers(), (float **)inputRevBuffer.getArrayOfWritePointers(), numSamples);

        if (inReverbEnabled != mLastInputReverbEnabled ) {
            float sgain = inReverbEnabled ? 0.0f : 1.0f;
            float egain = inReverbEnabled ? 1.0f : 0.0f;

            inputRevBuffer.applyGainRamp(0, numSamples, sgain, egain);
        }

        if (sendCh == 1 || sendCh == 2) {
            // mix it into send workbuffer
            for (int channel = 0; channel < sendPanChannels && channel < 2; ++channel) {
                sendWorkBuffer.addFrom(channel, 0, inputRevBuffer, channel, 0, numSamples);
            }
        }
    }

    mLastInputReverbEnabled = inReverbEnabled;


    // send meter post panning (and post file and met)
    sendMeterSource.measureBlock (sendWorkBuffer, 0, numSamples);


    bool hearlatencytest = mHearLatencyTest.get();

    bool anysoloed = mMainMonitorSolo.get();


    // push data for going out
    {
        const ScopedReadLock sl (mCoreLock);        
        
        mAooCommonSource->process( (AooSample**)sendWorkBuffer.getArrayOfReadPointers(), numSamples, t);
        
        for (auto & remote : mRemotePeers) 
        {
            if (remote->soloed) {
                anysoloed = true;
                break;
            }
        }
        
        tempBuffer.clear(0, numSamples);
        
        int rindex = 0;
        
        for (auto & remote : mRemotePeers) 
        {
            
            if (!remote->oursink) { 
                ++rindex;
                continue;                
            }

            // just in case, should be exceedingly rare this is necessary
            if (remote->workBuffer.getNumSamples() < currSamplesPerBlock
                || remote->recvChannels > remote->workBuffer.getNumChannels()
                || mainBusOutputChannels > remote->workBuffer.getNumChannels()) {
                remote->workBuffer.setSize(jmax(2, jmax(mainBusOutputChannels, remote->recvChannels)), currSamplesPerBlock, false, false, true);
            }

            remote->workBuffer.clear(0, numSamples);

            // calculate fill ratio before processing the sink
            double retratio = 0.0f;
            AooEndpoint aep = { remote->endpoint->address.address_ptr(), (AooAddrSize) remote->endpoint->address.length(), remote->remoteSourceId };
#if 0
            if (remote->oursink->getBufferFillRatio(aep, retratio) == kAooOk) {
                remote->fillRatio.Z *= 0.95;
                remote->fillRatio.push(retratio);
                remote->fillRatioSlow.Z *= 0.99;
                remote->fillRatioSlow.push(retratio);
            }
#endif
            
            {
                // get audio data coming in from outside into tempbuf
                const ScopedReadLock sl (remote->sinkLock); // not contended, should be able to get rid of

                // just in case, should be exceedingly rare this is necessary
                if (remote->workBuffer.getNumSamples() < currSamplesPerBlock
                    || remote->recvChannels > remote->workBuffer.getNumChannels()
                    || mainBusOutputChannels > remote->workBuffer.getNumChannels()) {
                    remote->workBuffer.setSize(jmax(2, jmax(mainBusOutputChannels, remote->recvChannels)), currSamplesPerBlock, false, false, true);
                }

                remote->workBuffer.clear(0, numSamples);

                remote->oursink->process((float **)remote->workBuffer.getArrayOfWritePointers(), numSamples, t, nullptr, nullptr);
            }

            
            // record individual tracks pre-compressor/level/pan, ignoring muting/solo, raw material

            if (userwritingpossible) {
                const ScopedTryLock sl (writerLock);
                if (sl.isLocked() && remote->fileWriter)
                {
                    float *tmpbuf[MAX_PANNERS];
                    int numchan = remote->fileWriter->getWriter()->getNumChannels();
                    for (int i = 0; i < numchan && i < MAX_PANNERS; ++i) {
                        if (i < remote->recvChannels) {
                            tmpbuf[i] = remote->workBuffer.getWritePointer(i);
                        }
                        else {
                            tmpbuf[i] = silentBuffer.getWritePointer(0);
                        }
                    }
                    remote->fileWriter->write (tmpbuf, numSamples);
                }
            }

            // write out per-user output bus
            if (remote->recvActive && remote->recvChannels > 0) {
                if (auto userbus = getBus(false, OutUserBaseBusIndex + rindex)) {
                    if (userbus->isEnabled()) {
                        int index = getChannelIndexInProcessBlockBuffer(false, OutUserBaseBusIndex + rindex, 0);
                        int cnt = getChannelCountOfBus(false, OutUserBaseBusIndex + rindex);
                        for (int i=0; i < cnt; ++i) {
                            if (i < remote->recvChannels) {
                                buffer.copyFrom(index+i, 0, remote->workBuffer, i, 0, numSamples);
                            }
                            else {
                                // it should already be clear
                                //buffer.clear(index+i, 0, numSamples);
                            }
                        }
                    }
                }
            }

            
            // apply effects

            float usegain = remote->gain;
            bool wasSilent = false;

            bool forceSilent = false;

            // we get the stuff, but ignore it (either muted or others soloed)
            if (!remote->recvActive || (anysoloed && !remote->soloed) || remote->resetSafetyMuted) {

                usegain = 0.0f;
                forceSilent = true;

                if (remote->_lastgain <= 0.0f) {
                    wasSilent = true;
                }
            }

            bool anysubsolo = false;
            for (auto cgi = 0; cgi < remote->numChanGroups; ++cgi) {
                if (remote->chanGroups[cgi].params.soloed) {
                    anysubsolo = true;
                    break;
                }
            }

            for (auto cgi = 0; cgi < remote->numChanGroups; ++cgi) {
                remote->chanGroups[cgi].processBlock(remote->workBuffer, remote->workBuffer, remote->chanGroups[cgi].params.chanStartIndex,  remote->chanGroups[cgi].params.numChannels, silentBuffer, numSamples, usegain);
            }

            remote->_lastgain = usegain;


            remote->recvMeterSource.measureBlock (remote->workBuffer, 0, numSamples);

            for (auto cgi = 0; cgi < remote->numChanGroups; ++cgi) {
                float redlev = 1.0f;
                if (remote->chanGroups[cgi].params.compressorParams.enabled && remote->chanGroups[cgi].compressorOutputLevel) {
                    redlev = jlimit(0.0f, 1.0f, Decibels::decibelsToGain(*remote->chanGroups[cgi].compressorOutputLevel));
                }
                for (auto j=0; j < remote->chanGroups[cgi].params.numChannels; ++j) {
                    int ch = remote->chanGroups[cgi].params.chanStartIndex + j;
                    remote->recvMeterSource.setReductionLevel(ch, redlev);
                }
            }

            ++rindex;
            
            if (wasSilent) continue; // can skip the rest, already fully muted/absent
            
            float tgain = mainBusOutputChannels == 1 && remote->recvChannels > 0 ? 1.0f/(float)remote->recvChannels : 1.0f;
            tgain *= usegain; // handles main solo


            for (auto i = 0; i < remote->numChanGroups; ++i)
            {
                // apply solo muting to the gain here
                float adjgain = anysubsolo && !remote->chanGroups[i].params.soloed ? 0.0f : tgain;
                // todo change dest ch target
                int dstch = remote->chanGroups[i].params.panDestStartIndex;
                int dstcnt = jmin(totalOutputChannels, remote->chanGroups[i].params.panDestChannels);
                remote->chanGroups[i].processPan(remote->workBuffer, remote->chanGroups[i].params.chanStartIndex, tempBuffer, dstch, dstcnt, numSamples, adjgain);

                if (doreverb) {
                    remote->chanGroups[i].processReverbSend(remote->workBuffer, remote->chanGroups[i].params.chanStartIndex, remote->chanGroups[i].params.numChannels, mainFxBuffer, 0, fxchannels, numSamples, mainReverbEnabled, false, adjgain);
                }

            }

        }
        
        
        
        // send out final outputs
        int i=0;
        for (auto & remote : mRemotePeers) 
        {
            if (remote->oursource /*&& remote->sendActive */) {

                workBuffer.clear(0, numSamples);

                int sendchans = jmin(workBuffer.getNumChannels(), remote->sendChannels);

                for (int channel = 0; channel < remote->sendChannels && channel < sendWorkBuffer.getNumChannels() && channel < workBuffer.getNumChannels() ; ++channel) {
                    workBuffer.addFrom(channel, 0, sendWorkBuffer, channel, 0, numSamples);
                }

                // now add any cross-routed input
                int j=0;
                for (auto & crossremote : mRemotePeers) 
                {
                    if (mRemoteSendMatrix[j][i]) {
                        for (int channel = 0; channel < remote->sendChannels; ++channel) {

                            // now apply panning

                            if (crossremote->recvChannels > 0 && remote->sendChannels > 1) {
                                for (int ch=0; ch < crossremote->recvChannels; ++ch) {
                                    const float pan = crossremote->recvChannels == 2 ? crossremote->recvStereoPan[ch] : crossremote->recvPan[ch];
                                    const float lastpan = crossremote->recvPanLast[ch];
                                    
                                    // apply pan law
                                    // -1 is left, 1 is right
                                    float pgain = channel == 0 ? (pan >= 0.0f ? (1.0f - pan) : 1.0f) : (pan >= 0.0f ? 1.0f : (1.0f+pan)) ;

                                    if (pan != lastpan) {
                                        float plastgain = channel == 0 ? (lastpan >= 0.0f ? (1.0f - lastpan) : 1.0f) : (lastpan >= 0.0f ? 1.0f : (1.0f+lastpan));
                                        
                                        workBuffer.addFromWithRamp(channel, 0, crossremote->workBuffer.getReadPointer(ch), numSamples, plastgain, pgain);
                                    } else {
                                        workBuffer.addFrom (channel, 0, crossremote->workBuffer, ch, 0, numSamples, pgain);                            
                                    }

                                    //remote->recvPanLast[ch] = pan;
                                }
                            } else {
                                
                                workBuffer.addFrom(channel, 0, crossremote->workBuffer, channel, 0, numSamples);
                            }
                            
                        }                        
                    }                    
                    ++j;
                }
                
                
                remote->oursource->process((AooSample**)workBuffer.getArrayOfReadPointers(), numSamples, t);
                
                //remote->sendMeterSource.measureBlock (workBuffer);
                
                
            }
            
            ++i;
        }

        // update last state
        for (auto & remote : mRemotePeers) 
        {
            for (int i=0; i < remote->recvChannels; ++i) {
                const float pan = remote->recvChannels == 2 ? remote->recvStereoPan[i] : remote->recvPan[i];

                remote->recvPanLast[i] = pan;
            }
        }
        
        // end scoped lock
    }


    // BEGIN MAIN OUTPUT BUFFER WRITING


    bool inrevdirect = !(anysoloed && !mMainMonitorSolo.get()) && drynow == 0.0;
    bool dryrampit =  (fabsf(drynow - mLastDry) > 0.00001);

    // process monitoring


    // copy from input buffer with dry gain as-is
    for (int channel = 0; channel < totalOutputChannels; ++channel) {
        //int usechan = channel < totalNumInputChannels ? channel : channel > 0 ? channel-1 : 0;
        if (dryrampit) {
            buffer.copyFromWithRamp(channel, 0, inputBuffer.getReadPointer(channel), numSamples, mLastDry, drynow);
        }
        else {
            buffer.copyFrom(channel, 0, inputBuffer.getReadPointer(channel), numSamples, drynow);
        }
    }

    // EFFECTS


    // TODO other main FX
    if (mainReverbEnabled != mLastMainReverbEnabled) {

        float sgain = mainReverbEnabled ? 0.0f : 1.0f;
        float egain = mainReverbEnabled ? 1.0f : 0.0f;
        
        if (mainReverbEnabled) {
            mMainReverb->reset();
            mMReverb.reset();
            mZitaReverb.instanceClear();
        }

        /*
        for (int channel = 0; channel < mainBusOutputChannels; ++channel) {
            // others
            mainFxBuffer.addFromWithRamp(channel, 0, tempBuffer.getReadPointer(channel), numSamples, sgain, egain);

            // input
            if (inrevdirect) {
                mainFxBuffer.addFromWithRamp(channel, 0, inputBuffer.getReadPointer(channel), numSamples, sgain, egain);
            } else {
                mainFxBuffer.addFromWithRamp(channel, 0, buffer.getReadPointer(channel), numSamples, sgain, egain);                
            }
        }
         */
    }
    else if (mainReverbEnabled){

        /*
        for (int channel = 0; channel < mainBusOutputChannels; ++channel) {
            // others
            mainFxBuffer.addFrom(channel, 0, tempBuffer, channel, 0, numSamples);

            // input
            if (inrevdirect) {
                mainFxBuffer.addFrom(channel, 0, inputBuffer, channel, 0, numSamples);
            } else {
                mainFxBuffer.addFrom(channel, 0, buffer, channel, 0, numSamples);                
            }
        }
         */

    }
    
    
    // add from tempBuffer (audio from remote peers)
    for (int channel = 0; channel < totalOutputChannels; ++channel) {

        buffer.addFrom(channel, 0, tempBuffer, channel, 0, numSamples);
    }
    
    

    if (doreverb) {
        // assumes reverb is NO dry
        
        if (mReverbParamsChanged) {
            mMainReverb->setParameters(mMainReverbParams);
            mReverbParamsChanged = false;
        }
        
        if (mLastReverbModel != mMainReverbModel.get()) {
            mMReverb.reset();
            mMainReverb->reset();
            mZitaReverb.instanceClear();
        }
        
        if (mMainReverbModel.get() == ReverbModelMVerb) {
            if (mainBusOutputChannels > 1) {            
                mMReverb.process((float **)mainFxBuffer.getArrayOfWritePointers(), (float **)mainFxBuffer.getArrayOfWritePointers(), numSamples);
            } 
        }
        else if (mMainReverbModel.get() == ReverbModelZita) {
            if (mainBusOutputChannels > 1) {            
                mZitaReverb.compute(numSamples, (float **)mainFxBuffer.getArrayOfWritePointers(), (float **)mainFxBuffer.getArrayOfWritePointers());
            }
        }
        else {
            if (mainBusOutputChannels > 1) {            
                mMainReverb->processStereo(mainFxBuffer.getWritePointer(0), mainFxBuffer.getWritePointer(1), numSamples);
            } else {
                mMainReverb->processMono(mainFxBuffer.getWritePointer(0), numSamples);
            }            
        }
    }

    if (mLastHasMainFx != hasmainfx) {
        float sgain = hasmainfx ? 0.0f : 1.0f;
        float egain = hasmainfx ? 1.0f : 0.0f;

        mainFxBuffer.applyGainRamp(0, numSamples, sgain, egain);
    } 

    mLastHasMainFx = hasmainfx;
    mLastMainReverbEnabled = mainReverbEnabled;
    mLastReverbModel = (ReverbModel) mMainReverbModel.get();
    
    // add from main FX
    if (hasmainfx) {
        for (int channel = 0; channel < mainBusOutputChannels; ++channel) {
            buffer.addFrom(channel, 0, mainFxBuffer, channel, 0, numSamples);
        }        
    }


    // mix input reverb into main buffer
    if (doinreverb) {
        for (int channel = 0; channel < mainBusOutputChannels && channel < 2; ++channel) {
            if (drynow > 0.0f || dryrampit) {
                // attenuate reverb with monitor level if used
                if (dryrampit) {
                    buffer.addFromWithRamp(channel, 0, inputRevBuffer.getReadPointer(channel), numSamples, mLastDry, drynow);
                }
                else {
                    buffer.addFrom(channel, 0, inputRevBuffer.getReadPointer(channel), numSamples, drynow);
                }
            }
            else if (inrevdirect) {
                // add the full input reverb to mix, if monitoring is off
                buffer.addFrom(channel, 0, inputRevBuffer.getReadPointer(channel), numSamples);
            }
        }
    }

    // add from file playback buffer
    if (hasfiledata) {

        int dstch = mFilePlaybackChannelGroup.params.monDestStartIndex;
        int dstcnt = jmin(totalOutputChannels, mFilePlaybackChannelGroup.params.monDestChannels);
        auto fgain = mFilePlaybackChannelGroup.params.gain;

        // process the monitor part of the metchannelgroup
        mFilePlaybackChannelGroup.processMonitor(fileBuffer, 0, buffer, dstch, dstcnt, numSamples, fgain);
    }

    if (hassoundboarddata) {
        soundboardChannelProcessor->processMonitor(buffer, numSamples, totalOutputChannels);
    }

    if (metenabled) {
        int dstch = mMetChannelGroup.params.monDestStartIndex;
        int dstcnt = jmin(totalOutputChannels, mMetChannelGroup.params.monDestChannels);
        auto fgain = mMetChannelGroup.params.gain;


        // process the monitor part of the metchannelgroup
        mMetChannelGroup.processMonitor(metBuffer, 0, buffer, dstch, dstcnt, numSamples, fgain);
    }


    // apply wet (output) gain to original buf, main bus only
    if (fabsf(wetnow - mLastWet) > 0.00001) {
        for (int channel = 0; channel < mainBusOutputChannels; ++channel) {
            buffer.applyGainRamp(channel, 0, numSamples, mLastWet, wetnow);
        }
    } else {
        for (int channel = 0; channel < mainBusOutputChannels; ++channel) {
            buffer.applyGain(channel, 0, numSamples, wetnow);
        }
    }

    
    outputMeterSource.measureBlock (buffer, 0, numSamples);

    // output to file writer if necessary
    if (writingpossible) {
        const ScopedTryLock sl (writerLock);
        if (sl.isLocked())
        {
            if (activeMixWriter.load() != nullptr 
                || activeMixMinusWriter.load() != nullptr
                || activeSelfWriters[0].load() != nullptr
                )
            {
                // write the raw (pre or post FX) input
                if (activeSelfWriters[0].load() != nullptr) {
                    const float * const* inbufs = mRecordInputPreFX ? inputPreBuffer.getArrayOfReadPointers() : inputPostBuffer.getArrayOfReadPointers();
                    int chindex = 0;
                    for (int i=0; i < mInputChannelGroupCount; ++i) {
                        int chcnt = mInputChannelGroups[i].params.numChannels;
                        if (activeSelfWriters[i].load() != nullptr) {
                            // we need to make sure the writer has at least all the inputs it expects
                            const float * useinbufs[MAX_PANNERS];
                            for (int j=0; j < mSelfRecordChans[i] && j < MAX_PANNERS; ++j) {
                                useinbufs[j] = j < chcnt ? inbufs[chindex+j] : silentBuffer.getReadPointer(0);
                            }
                            activeSelfWriters[i].load()->write (useinbufs, numSamples);
                        }
                        chindex += chcnt;
                    }
                }

                // we need to mix the input, audio from remote peers, and the file playback together here
                workBuffer.clear(0, numSamples);


                bool rampit =  (fabsf(wetnow - mLastWet) > 0.00001);
                
                for (int channel = 0; channel < totalRecordingChannels; ++channel) {
                    
                    // apply Main out gain to audio from remote peers and file playback (should we?)
                    if (rampit) {
                        workBuffer.addFromWithRamp(channel, 0, tempBuffer.getReadPointer(channel), numSamples, mLastWet, wetnow);
                        if (hasmainfx) {
                            workBuffer.addFromWithRamp(channel, 0, mainFxBuffer.getReadPointer(channel), numSamples, mLastWet, wetnow);
                        }
                   }
                    else {
                        workBuffer.addFrom(channel, 0, tempBuffer, channel, 0, numSamples, wetnow);

                        if (hasmainfx) {
                            workBuffer.addFrom(channel, 0, mainFxBuffer, channel, 0, numSamples, wetnow);
                        }
                    }
                }

                if (hasfiledata) {
                    int dstch = mRecFilePlaybackChannelGroup.params.monDestStartIndex;
                    int dstcnt = jmin(totalOutputChannels, mRecFilePlaybackChannelGroup.params.monDestChannels);
                    auto fgain = mRecFilePlaybackChannelGroup.params.gain * wetnow;
                    // process the monitor part of the metchannelgroup
                    mRecFilePlaybackChannelGroup.processMonitor(fileBuffer, 0, workBuffer, dstch, dstcnt, numSamples, fgain);
                }

                if (hassoundboarddata) {
                    soundboardChannelProcessor->processMonitor(workBuffer, numSamples, totalOutputChannels, wetnow);
                }

                if (metenabled && metrecorded) {
                    int dstch = mRecMetChannelGroup.params.monDestStartIndex;
                    int dstcnt = jmin(totalOutputChannels, mRecMetChannelGroup.params.monDestChannels);
                    auto fgain = mRecMetChannelGroup.params.gain * wetnow;

                    // process the monitor part of the metchannelgroup
                    mRecMetChannelGroup.processMonitor(metBuffer, 0, workBuffer, dstch, dstcnt, numSamples, fgain);
                }

                if (activeMixMinusWriter.load() != nullptr) {
                    activeMixMinusWriter.load()->write (workBuffer.getArrayOfReadPointers(), numSamples);
                }

                // mix in input
                for (int channel = 0; channel < totalRecordingChannels; ++channel) {
                    if (channel >= inputBuffer.getNumChannels()) continue;
                    //int usechan = channel < mainBusInputChannels ? channel : channel > 0 ? channel-1 : 0;
                    auto usechan = channel;

                    if (mDry.get() > 0.0f) {
                        // copy input with monitor gain if > 0
                        if (dryrampit) {
                            workBuffer.addFromWithRamp(channel, 0, inputBuffer.getReadPointer(usechan), numSamples, mLastDry, drynow);

                            if (doinreverb && channel < 2) {
                                workBuffer.addFromWithRamp(channel, 0, inputRevBuffer.getReadPointer(channel), numSamples, mLastDry, drynow);
                            }
                        }
                        else {
                            workBuffer.addFrom(channel, 0, inputBuffer.getReadPointer(usechan), numSamples, drynow);

                            if (doinreverb && channel < 2) {
                                workBuffer.addFrom(channel, 0, inputRevBuffer.getReadPointer(usechan), numSamples, drynow);
                            }
                        }
                    }
                    else if (!anysoloed || mMainMonitorSolo.get()) {
                        // monitoring is off, we just mix it into written file at full volume, as long as no one else is soloed
                        workBuffer.addFrom(channel, 0, inputBuffer.getReadPointer(usechan), numSamples);

                        if (doinreverb && channel < 2) {
                            workBuffer.addFrom(channel, 0, inputRevBuffer.getReadPointer(usechan), numSamples);
                        }
                    }

                }

                if (activeMixWriter.load() != nullptr) {
                    // write out full mix
                    activeMixWriter.load()->write (workBuffer.getArrayOfReadPointers(), numSamples);
                }
                
            }
        }
    }

    if (writingpossible || userwritingpossible) {
        mElapsedRecordSamples += numSamples;
    }


    lastSamplesPerBlock = numSamples;

    notifySendThread();
    
    mLastWet = wetnow;
    mLastDry = drynow;
    mLastInputGain = inGain;
    mLastInMonPan1 = inmonPan1;
    mLastInMonPan2 = inmonPan2;
    mLastInMonMonoPan = inmonMonoPan;
    mAnythingSoloed =  anysoloed;

    mTransportWasPlaying = mTransportSource.isPlaying();
}

//==============================================================================
bool SonobusAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* SonobusAudioProcessor::createEditor()
{
    return new SonobusAudioProcessorEditor (*this);
}

AudioProcessorValueTreeState& SonobusAudioProcessor::getValueTreeState()
{
    return mState;
}

ValueTree AooServerConnectionInfo::getValueTree() const
{
    ValueTree item(recentsItemKey);
    
    item.setProperty("userName", userName, nullptr);
    item.setProperty("userPassword", userPassword, nullptr);
    item.setProperty("groupName", groupName, nullptr);
    item.setProperty("groupPassword", groupPassword, nullptr);
    item.setProperty("serverHost", serverHost, nullptr);
    item.setProperty("serverPort", serverPort, nullptr);
    item.setProperty("timestamp", timestamp, nullptr);
    item.setProperty("groupIsPublic", groupIsPublic, nullptr);

    return item;
}

void AooServerConnectionInfo::setFromValueTree(const ValueTree & item)
{    
    userName = item.getProperty("userName", userName);
    userPassword = item.getProperty("userPassword", userPassword);
    groupName = item.getProperty("groupName", groupName);
    groupPassword = item.getProperty("groupPassword", groupPassword);
    serverHost = item.getProperty("serverHost", serverHost);
    serverPort = item.getProperty("serverPort", serverPort);
    timestamp = item.getProperty("timestamp", timestamp);
    groupIsPublic = item.getProperty("groupIsPublic", groupIsPublic);
}


static String videoLinkInfoKey("VideoLinkInfo");
static String videoLinkRoomModeKey("roomMode");
static String videoLinkShowNamesKey("showNames");
static String videoLinkExtraParamsKey("extraParams");
static String videoLinkBeDirectorKey("beDir");

ValueTree SonobusAudioProcessor::VideoLinkInfo::getValueTree() const
{
    ValueTree item(videoLinkInfoKey);
    
    item.setProperty(videoLinkRoomModeKey, roomMode, nullptr);
    item.setProperty(videoLinkShowNamesKey, showNames, nullptr);
    item.setProperty(videoLinkExtraParamsKey, extraParams, nullptr);
    item.setProperty(videoLinkBeDirectorKey, beDirector, nullptr);

    return item;
}

void SonobusAudioProcessor::VideoLinkInfo::setFromValueTree(const ValueTree & item)
{
    roomMode = item.getProperty(videoLinkRoomModeKey, roomMode);
    showNames = item.getProperty(videoLinkShowNamesKey, showNames);
    extraParams = item.getProperty(videoLinkExtraParamsKey, extraParams);
    beDirector = item.getProperty(videoLinkBeDirectorKey, beDirector);
}


void SonobusAudioProcessor::getStateInformationWithOptions(MemoryBlock& destData, bool includecache, bool includeInputGroups, bool xmlformat)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    MemoryOutputStream stream(destData, false);

    auto tempstate = mState.copyState();

    ValueTree recentsTree = tempstate.getOrCreateChildWithName(recentsCollectionKey, nullptr);

    if (includecache) {
        // update state with our recents info
        recentsTree.removeAllChildren(nullptr);
        for (auto & info : mRecentConnectionInfos) {
            recentsTree.appendChild(info.getValueTree(), nullptr);
        }
    } else {
        tempstate.removeChild(recentsTree, nullptr);
    }

    ValueTree extraTree = tempstate.getOrCreateChildWithName(extraStateCollectionKey, nullptr);
    // update state with our recents info
    extraTree.removeAllChildren(nullptr);
    extraTree.setProperty(useSpecificUdpPortKey, mUseSpecificUdpPort, nullptr);
    extraTree.setProperty(changeQualForAllKey, mChangingDefaultAudioCodecChangesAll, nullptr);
    extraTree.setProperty(changeRecvQualForAllKey, mChangingDefaultRecvAudioCodecChangesAll, nullptr);
    extraTree.setProperty(defRecordOptionsKey, var((int)mDefaultRecordingOptions), nullptr);
    extraTree.setProperty(defRecordFormatKey, var((int)mDefaultRecordingFormat), nullptr);
    extraTree.setProperty(defRecordBitsKey, var((int)mDefaultRecordingBitsPerSample), nullptr);
    extraTree.setProperty(recordSelfPreFxKey, mRecordInputPreFX, nullptr);
    extraTree.setProperty(recordFinishOpenKey, mRecordFinishOpens, nullptr);

    if (mDefaultRecordDir.isLocalFile()) {
        // backwards compat
        extraTree.setProperty(defRecordDirKey, mDefaultRecordDir.getLocalFile().getFullPathName(), nullptr);
    }
    extraTree.setProperty(defRecordDirURLKey, mDefaultRecordDir.toString(false), nullptr);

    extraTree.setProperty(lastBrowseDirKey, mLastBrowseDir, nullptr);
    extraTree.setProperty(sliderSnapKey, mSliderSnapToMouse, nullptr);
    extraTree.setProperty(disableShortcutsKey, mDisableKeyboardShortcuts, nullptr);
    extraTree.setProperty(peerDisplayModeKey, var((int)mPeerDisplayMode), nullptr);
    extraTree.setProperty(lastChatWidthKey, var((int)mLastChatWidth), nullptr);
    extraTree.setProperty(lastChatShownKey, mLastChatShown, nullptr);
    extraTree.setProperty(chatFontSizeOffsetKey, var((int)mChatFontSizeOffset), nullptr);
    extraTree.setProperty(chatUseFixedWidthFontKey, mChatUseFixedWidthFont, nullptr);
    extraTree.setProperty(lastSoundboardWidthKey, var((int)mLastSoundboardWidth), nullptr);
    extraTree.setProperty(lastSoundboardShownKey, mLastSoundboardShown, nullptr);
    extraTree.setProperty(linkMonitoringDelayTimesKey, mLinkMonitoringDelayTimes, nullptr);
    extraTree.setProperty(lastUsernameKey, mCurrentUsername, nullptr);
    extraTree.setProperty(langOverrideCodeKey, mLangOverrideCode, nullptr);
    extraTree.setProperty(lastWindowWidthKey, var((int)mPluginWindowWidth), nullptr);
    extraTree.setProperty(lastWindowHeightKey, var((int)mPluginWindowHeight), nullptr);
    extraTree.setProperty(autoresizeDropRateThreshKey, var((float)mAutoresizeDropRateThresh), nullptr);
    extraTree.setProperty(reconnectServerLossKey, mReconnectAfterServerLoss.get(), nullptr);

    extraTree.appendChild(mVideoLinkInfo.getValueTree(), nullptr);
    
    ValueTree inputChannelGroupsTree = tempstate.getOrCreateChildWithName(inputChannelGroupsStateKey, nullptr);
    if (includeInputGroups) {
        inputChannelGroupsTree.removeAllChildren(nullptr);
        inputChannelGroupsTree.setProperty(numChanGroupsKey, mInputChannelGroupCount, nullptr);
        
        for (auto i = 0; i < mInputChannelGroupCount && i < MAX_CHANGROUPS; ++i) {
            inputChannelGroupsTree.appendChild(mInputChannelGroups[i].params.getValueTree(), nullptr);
        }
    }
    else {
        tempstate.removeChild(inputChannelGroupsTree, nullptr);
    }

    
    ValueTree extraChannelGroupsTree = tempstate.getOrCreateChildWithName(extraChannelGroupsStateKey, nullptr);
    extraChannelGroupsTree.removeAllChildren(nullptr);
    
    auto fpcg = mFilePlaybackChannelGroup.params.getValueTree();
    fpcg.setProperty("chgID", "filepb", nullptr);
    extraChannelGroupsTree.appendChild(fpcg, nullptr);

    auto metcg = mMetChannelGroup.params.getValueTree();
    metcg.setProperty("chgID", "met", nullptr);
    extraChannelGroupsTree.appendChild(metcg, nullptr);

    auto sbcg = soundboardChannelProcessor->getChannelGroupParams().getValueTree();
    sbcg.setProperty("chgID", "soundboard", nullptr);
    extraChannelGroupsTree.appendChild(sbcg, nullptr);

    
    ValueTree peerCacheTree = tempstate.getOrCreateChildWithName(peerStateCacheMapKey, nullptr);
    if (includecache) {
        // update state with our recents info
        peerCacheTree.removeAllChildren(nullptr);
        for (auto & info : mPeerStateCacheMap) {
            peerCacheTree.appendChild(info.second.getValueTree(), nullptr);
        }
    } else {
        tempstate.removeChild(peerCacheTree, nullptr);
    }

    if (xmlformat) {
        stream.writeString(tempstate.toXmlString());
    }
    else {
        tempstate.writeToStream (stream);
    }

    DBG("GETSTATE: " << tempstate.toXmlString());
}


//==============================================================================
void SonobusAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    getStateInformationWithOptions(destData, true);

}

void SonobusAudioProcessor::setStateInformationWithOptions (const void* data, int sizeInBytes, bool includecache, bool includeInputGroups, bool xmlformat)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    ValueTree tree;

    if (xmlformat) {
        tree = ValueTree::fromXml(String::createStringFromData(data, sizeInBytes));
    }
    else {
        tree = ValueTree::readFromData (data, sizeInBytes);
    }

    if (tree.isValid()) {
        mState.state = tree;
        
        DBG("SETSTATE: " << mState.state.toXmlString());

        if (includecache) {
            ValueTree recentsTree = mState.state.getChildWithName(recentsCollectionKey);
            if (recentsTree.isValid()) {
                mRecentConnectionInfos.clear();
                for (auto child : recentsTree) {
                    AooServerConnectionInfo info;
                    info.setFromValueTree(child);
                    mRecentConnectionInfos.add(info);
                }
            }
        }

        ValueTree extraTree = mState.state.getChildWithName(extraStateCollectionKey);
        if (extraTree.isValid()) {
            int port = extraTree.getProperty(useSpecificUdpPortKey, mUseSpecificUdpPort);
            setUseSpecificUdpPort(port);

            bool chqual = extraTree.getProperty(changeQualForAllKey, mChangingDefaultAudioCodecChangesAll);
            setChangingDefaultAudioCodecSetsExisting(chqual);

            bool chrqual = extraTree.getProperty(changeRecvQualForAllKey, mChangingDefaultRecvAudioCodecChangesAll);
            setChangingDefaultRecvAudioCodecSetsExisting(chrqual);

            uint32 opts = (uint32)(int) extraTree.getProperty(defRecordOptionsKey, (int)mDefaultRecordingOptions);
            setDefaultRecordingOptions(opts);

            uint32 fmt = (uint32)(int) extraTree.getProperty(defRecordFormatKey, (int)mDefaultRecordingFormat);
            setDefaultRecordingFormat((RecordFileFormat)fmt);

            int bps = (uint32)(int) extraTree.getProperty(defRecordBitsKey, (int)mDefaultRecordingBitsPerSample);
            setDefaultRecordingBitsPerSample(bps);

            bool linkmon = extraTree.getProperty(linkMonitoringDelayTimesKey, mLinkMonitoringDelayTimes);
            setLinkMonitoringDelayTimes(linkmon);

            bool prefx = extraTree.getProperty(recordSelfPreFxKey, mRecordInputPreFX);
            setSelfRecordingPreFX(prefx);

            setRecordFinishOpens(extraTree.getProperty(recordFinishOpenKey, mRecordFinishOpens));


#if !(JUCE_IOS)
            String urlstr = extraTree.getProperty(defRecordDirURLKey, "");
            if (urlstr.isNotEmpty()) {
                // doublecheck it's a valid URL due to bad update
                auto url = URL(urlstr);
                if (url.getScheme().isEmpty()) {
                    // asssume its a file
                    DBG("Bad saved record URL: " << urlstr);
                    url = URL(File(urlstr));
                }
                setDefaultRecordingDirectory(url);
            } else {
#if ! JUCE_ANDROID
                // backward compat (but not on android)
                String filestr = extraTree.getProperty(defRecordDirKey, "");
                if (filestr.isNotEmpty()) {
                    File recdir = File(filestr);
                    setDefaultRecordingDirectory(URL(recdir));
                }
#endif
            }
#endif

#if !(JUCE_IOS || JUCE_ANDROID)
            setLastBrowseDirectory(extraTree.getProperty(lastBrowseDirKey, mLastBrowseDir));
#endif
            setSlidersSnapToMousePosition(extraTree.getProperty(sliderSnapKey, mSliderSnapToMouse));
            setDisableKeyboardShortcuts(extraTree.getProperty(disableShortcutsKey, mDisableKeyboardShortcuts));
            setPeerDisplayMode((PeerDisplayMode)(int)extraTree.getProperty(peerDisplayModeKey, (int)mPeerDisplayMode));
            setLastChatWidth((int)extraTree.getProperty(lastChatWidthKey, (int)mLastChatWidth));
            setLastChatShown(extraTree.getProperty(lastChatShownKey, mLastChatShown));
            setLastSoundboardWidth((int)extraTree.getProperty(lastSoundboardWidthKey, (int)mLastSoundboardWidth));
            setLastSoundboardShown(extraTree.getProperty(lastSoundboardShownKey, mLastSoundboardShown));
            mCurrentUsername = extraTree.getProperty(lastUsernameKey, mCurrentUsername);
            mLangOverrideCode = extraTree.getProperty(langOverrideCodeKey, mLangOverrideCode);
            setChatFontSizeOffset((int) extraTree.getProperty(chatFontSizeOffsetKey, (int)mChatFontSizeOffset));
            setChatUseFixedWidthFont(extraTree.getProperty(chatUseFixedWidthFontKey, mChatUseFixedWidthFont));;

            setLastPluginBounds(juce::Rectangle<int>(0, 0, extraTree.getProperty(lastWindowWidthKey, (int)mPluginWindowWidth),
                                                     extraTree.getProperty(lastWindowHeightKey, (int)mPluginWindowHeight)));

            setAutoresizeBufferDropRateThreshold(extraTree.getProperty(autoresizeDropRateThreshKey, (float)mAutoresizeDropRateThresh));

            setReconnectAfterServerLoss(extraTree.getProperty(reconnectServerLossKey, mReconnectAfterServerLoss.get()));

            
            ValueTree videoinfo = extraTree.getChildWithName(videoLinkInfoKey);
            if (videoinfo.isValid()) {
                mVideoLinkInfo.setFromValueTree(videoinfo);
            }
        }

        if (includeInputGroups) {
            ValueTree inputChannelGroupsTree = mState.state.getChildWithName(inputChannelGroupsStateKey);
            if (inputChannelGroupsTree.isValid()) {
                
                mInputChannelGroupCount = inputChannelGroupsTree.getProperty(numChanGroupsKey, (int)mInputChannelGroupCount);
                
                int i = 0;
                for (auto channelGroupTree : inputChannelGroupsTree) {
                    if (!channelGroupTree.isValid()) continue;
                    if (i >= MAX_CHANGROUPS) break;
                    
                    mInputChannelGroups[i].params.setFromValueTree(channelGroupTree);
                    
                    ++i;
                }
                
            }
        }

        ValueTree extraChannelGroupsTree = mState.state.getChildWithName(extraChannelGroupsStateKey);
        if (extraChannelGroupsTree.isValid()) {
            
            for (auto channelGroupTree : extraChannelGroupsTree) {
                if (!channelGroupTree.isValid()) continue;
                
                auto cid = channelGroupTree.getProperty("chgID");

                ChannelGroupParams params;
                params.setFromValueTree(channelGroupTree);

                if (cid == "filepb") {
                    mFilePlaybackChannelGroup.params = params;
                    mFilePlaybackChannelGroup.commitAllParams();
                    mRecFilePlaybackChannelGroup.params = params;
                    mRecFilePlaybackChannelGroup.commitAllParams();
                }
                else if (cid == "met") {
                    mMetChannelGroup.params = params;
                    mMetChannelGroup.commitAllParams();
                    mRecMetChannelGroup.params = params;
                    mRecMetChannelGroup.commitAllParams();
                } else if (cid == "soundboard") {
                    soundboardChannelProcessor->setChannelGroupParams(params);
                }
            }
        }

        
        if (includecache) {
            loadPeerCacheFromState();
        }
        
        // don't recover the metronome enable state, always default it to off
        mState.getParameter(paramMetEnabled)->setValueNotifyingHost(0.0f);

        // don't recover the recv mute either (actually recover it after all)
        //mState.getParameter(paramMainRecvMute)->setValueNotifyingHost(0.0f);

        // don't recover main solo
        mState.getParameter(paramMainMonitorSolo)->setValueNotifyingHost(0.0f);
        
        if (mFreshInit) {
            // only do initial auto reconnect on the first state restore
            
            if (getAutoReconnectToLast() && !isConnectedToServer()) {
                reconnectToMostRecent();
            }

            mFreshInit = false;
        }
    }
}

void SonobusAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    setStateInformationWithOptions(data, sizeInBytes, true, true);
}


bool SonobusAudioProcessor::saveCurrentAsDefaultPluginSettings()
{
    MemoryBlock data;
    // as xml, with no cache, and no channel group stuff
    getStateInformationWithOptions(data, false, false, true);
    File defFile = getSupportDir().getChildFile("PluginDefault.xml");
    
    return defFile.replaceWithData (data.getData(), data.getSize());
}

bool SonobusAudioProcessor::loadDefaultPluginSettings()
{
    File defFile = getSupportDir().getChildFile("PluginDefault.xml");
    MemoryBlock data;
    if (defFile.loadFileAsData (data)) {
        // no cache, no channel group, xml
        setStateInformationWithOptions (data.getData(), (int) data.getSize(), false, false, true);
        return true;
    }
    return false;
}

void SonobusAudioProcessor::resetDefaultPluginSettings()
{
    // remove the default file
    File defFile = getSupportDir().getChildFile("PluginDefault.xml");
    defFile.deleteFile();
}


void SonobusAudioProcessor::ServerReconnectTimer::timerCallback()
{
    if (!processor.isConnectedToServer() && !processor.mPendingReconnect) {
        processor.reconnectToMostRecent();
    }
    else if (processor.isConnectedToServer()){
        processor.mRecoveringFromServerLoss = false;

        stopTimer();
    }
}

bool SonobusAudioProcessor::reconnectToMostRecent()
{
    Array<AooServerConnectionInfo> recents;
    getRecentServerConnectionInfos(recents);
    
    if (recents.size() > 0) {
        const auto & info = recents.getReference(0);

        if (info.serverHost.isNotEmpty() && info.userName.isNotEmpty()) {
            DBG("Reconnecting to server and group: " << info.groupName);
            mPendingReconnectInfo = info;
            mPendingReconnect = true;
            connectToServer(info.serverHost, info.serverPort, info.userName, info.userPassword);
            return true;
        }
    }

    return false;
}

SonobusAudioProcessor::PeerStateCache::PeerStateCache()
{
    // set up up channel groups with default layout
    for (auto i = 0; i < MAX_CHANGROUPS; ++i) {
        channelGroupParams[i].chanStartIndex = i;
        channelGroupParams[i].numChannels = 1;
    }
}


ValueTree SonobusAudioProcessor::PeerStateCache::getValueTree() const
{
    ValueTree item(peerStateCacheKey);
    
    item.setProperty(peerNameKey, name, nullptr);
    item.setProperty(peerNetbufKey, netbuf, nullptr);
    item.setProperty(peerNetbufAutoKey, netbufauto, nullptr);
    item.setProperty(peerSendFormatKey, sendFormat, nullptr);
    item.setProperty(numChanGroupsKey, numChanGroups, nullptr);
    item.setProperty(peerLevelKey, mainGain, nullptr);
    item.setProperty(peerOrderPriorityKey, orderPriority, nullptr);

    ValueTree channelGroupsTree(channelGroupsStateKey);

    for (auto i = 0; i < numChanGroups && i < MAX_CHANGROUPS; ++i) {
        channelGroupsTree.appendChild(channelGroupParams[i].getValueTree(), nullptr);
    }

    item.appendChild(channelGroupsTree, nullptr);

    // multichan state
    ValueTree channelGroupsMultiTree(channelGroupsMultiStateKey);

    for (auto i = 0; i < numMultiChanGroups && i < MAX_CHANGROUPS; ++i) {
        channelGroupsMultiTree.appendChild(channelGroupMultiParams[i].getValueTree(), nullptr);
    }
    item.setProperty(numMultiChanGroupsKey, numMultiChanGroups, nullptr);
    item.setProperty(modifiedChanGroupsKey, modifiedChanGroups, nullptr);
    item.appendChild(channelGroupsMultiTree, nullptr);


    return item;
}

void SonobusAudioProcessor::PeerStateCache::setFromValueTree(const ValueTree & item)
{    
    name = item.getProperty(peerNameKey, name);
    netbuf = item.getProperty(peerNetbufKey, netbuf);
    netbufauto = item.getProperty(peerNetbufAutoKey, netbufauto);
    sendFormat = item.getProperty(peerSendFormatKey, sendFormat);
    numChanGroups = std::max(0, std::min((int) (MAX_CHANGROUPS-1), (int)item.getProperty(numChanGroupsKey, numChanGroups)));

    mainGain = item.getProperty(peerLevelKey, mainGain);
    orderPriority = item.getProperty(peerOrderPriorityKey, orderPriority);

    // backwards compat
    channelGroupParams[0].pan[0] = item.getProperty(peerMonoPanKey, channelGroupParams[0].pan[0]);
    channelGroupParams[0].panStereo[0] = item.getProperty(peerPan1Key, channelGroupParams[0].panStereo[0]);
    channelGroupParams[0].panStereo[1] = item.getProperty(peerPan2Key, channelGroupParams[0].panStereo[1]);

    ValueTree compressorTree = item.getChildWithName(compressorStateKey);
    if (compressorTree.isValid()) {
        channelGroupParams[0].compressorParams.setFromValueTree(compressorTree);
    }
    // end backwards compat

    // new state
    ValueTree channelGroupsTree = item.getChildWithName(channelGroupsStateKey);
    if (channelGroupsTree.isValid()) {

        int i = 0;
        for (auto channelGroupTree : channelGroupsTree) {
            if (!channelGroupTree.isValid()) continue;
            if (i >= MAX_CHANGROUPS) break;

            channelGroupParams[i].setFromValueTree(channelGroupTree);
            ++i;
        }
    }

    // multistate
    numMultiChanGroups = std::max(0, std::min((int) (MAX_CHANGROUPS-1), (int)item.getProperty(numMultiChanGroupsKey, numMultiChanGroups)));
    modifiedChanGroups = item.getProperty(modifiedChanGroupsKey, modifiedChanGroups);

    ValueTree channelGroupsMultiTree = item.getChildWithName(channelGroupsStateKey);
    if (channelGroupsMultiTree.isValid()) {

        int i = 0;
        for (auto channelGroupTree : channelGroupsMultiTree) {
            if (!channelGroupTree.isValid()) continue;
            if (i >= MAX_CHANGROUPS) break;

            channelGroupMultiParams[i].setFromValueTree(channelGroupTree);
            ++i;
        }
    }
}

void SonobusAudioProcessor::loadPeerCacheFromState()
{
    ValueTree peerCacheMapTree = mState.state.getChildWithName(peerStateCacheMapKey);
    if (peerCacheMapTree.isValid()) {
        mPeerStateCacheMap.clear();
        for (auto child : peerCacheMapTree) {
            PeerStateCache info;
            info.setFromValueTree(child);
            mPeerStateCacheMap.insert(PeerStateCacheMap::value_type(info.name, info));
        }
    }
    
}

void SonobusAudioProcessor::storePeerCacheToState()
{
    ValueTree peerCacheTree = mState.state.getOrCreateChildWithName(peerStateCacheMapKey, nullptr);
    // update state with our recents info
    peerCacheTree.removeAllChildren(nullptr);
    for (auto & info : mPeerStateCacheMap) {
        peerCacheTree.appendChild(info.second.getValueTree(), nullptr);        
    }

}

void SonobusAudioProcessor::loadGlobalState()
{
    File file = mSupportDir.getChildFile("GlobalState.xml");
    
    if (!file.existsAsFile()) {
        return;
    }

    XmlDocument doc(file);
    auto tree = ValueTree::fromXml(*doc.getDocumentElement());

    if (tree.isValid()) {
        mGlobalState = tree;
    }
}

bool SonobusAudioProcessor::storeGlobalState()
{
    File file = mSupportDir.getChildFile("GlobalState.xml");

    // Make sure  the parent directory exists
    file.getParentDirectory().createDirectory();

    return mGlobalState.createXml()->writeTo(file);
}

bool SonobusAudioProcessor::isAddressBlocked(const String & ipaddr) const
{
    auto blocklist = mGlobalState.getChildWithName(blockedAddressesKey);
    if (!blocklist.isValid()) return false;
    
    for (const auto & child : blocklist) {
        auto prop = child.getProperty(addressValueKey);
        if (prop.isString() && ipaddr == prop.toString()) {
            // is blocked
            return true;
        }
    }
    
    return false;
}

void SonobusAudioProcessor::addBlockedAddress(const String & ipaddr)
{
    auto blocklist = mGlobalState.getOrCreateChildWithName(blockedAddressesKey, nullptr);
    
    auto newchild = ValueTree(addressKey);
    newchild.setProperty(addressValueKey, ipaddr, nullptr);
    
    for (const auto & child : blocklist) {
        auto prop = child.getProperty(addressValueKey);
        if (prop.isString() && ipaddr == prop.toString()) {
            // already exists
            return;
        }
    }
    
    blocklist.appendChild(newchild, nullptr);
    
    storeGlobalState();
}

void SonobusAudioProcessor::removeBlockedAddress(const String & ipaddr)
{
    auto blocklist = mGlobalState.getOrCreateChildWithName(blockedAddressesKey, nullptr);

    if (blocklist.isValid()) {
        bool gotit = false;
        for (const auto & child : blocklist) {
            auto prop = child.getProperty(addressValueKey);
            if (prop.isString() && ipaddr == prop.toString()) {
                blocklist.removeChild(child, nullptr);
                gotit = true;
                break;
            }
        }
        
        if (gotit)
            storeGlobalState();
    }
}

StringArray SonobusAudioProcessor::getAllBlockedAddresses() const
{
    StringArray retlist;

    auto blocklist = mGlobalState.getChildWithName(blockedAddressesKey);
    if (blocklist.isValid()) {
        for (const auto & child : blocklist) {
            auto prop = child.getProperty(addressValueKey);
            if (prop.isString()) {
                auto val = prop.toString();
                if (val.isNotEmpty()) {
                    retlist.add(val);
                }
            }
        }
     }
    return retlist;
}


bool SonobusAudioProcessor::startRecordingToFile(const URL & recordLocationUrl, const String & filename, URL & mainreturl, uint32 recordOptions, RecordFileFormat fileformat)
{
    if (!recordingThread) {
        recordingThread = std::make_unique<TimeSliceThread>("Recording Thread");
        recordingThread->startThread();        
    }
    
    stopRecordingToFile();

    bool ret = false;
    
    // Now create a WAV writer object that writes to our output stream...
    //WavAudioFormat audioFormat;
    std::unique_ptr<AudioFormat> audioFormat;
    std::unique_ptr<AudioFormat> wavAudioFormat;

    int qualindex = 0;
    
    int bitsPerSample = mDefaultRecordingBitsPerSample;

    if (getSampleRate() <= 0)
    {
        return false;
    }
    
    // just put a bogus directory in it, we'll be using only the filename part
    File usefile = File::getCurrentWorkingDirectory().getChildFile(filename);
    String mimetype;
    
    if (fileformat == FileFormatDefault) {
        fileformat = mDefaultRecordingFormat;
    }
    
    if (recordOptions == RecordDefaultOptions) {
        recordOptions = mDefaultRecordingOptions;
    }

    totalRecordingChannels = getMainBusNumOutputChannels();
    if (totalRecordingChannels == 0) {
        totalRecordingChannels = 2;
    }

    if (fileformat == FileFormatFLAC && totalRecordingChannels > 8) {
        // flac doesn't support > 8 channels
        fileformat = FileFormatWAV;
    }

    if (fileformat == FileFormatFLAC || (fileformat == FileFormatAuto && usefile.getFileExtension().toLowerCase() == ".flac")) {
        audioFormat = std::make_unique<FlacAudioFormat>();
        usefile = usefile.withFileExtension(".flac");
        mimetype = "audio/flac" ;
    }
    else if (fileformat == FileFormatWAV || (fileformat == FileFormatAuto && usefile.getFileExtension().toLowerCase() == ".wav")) {
        audioFormat = std::make_unique<WavAudioFormat>();
        usefile = usefile.withFileExtension(".wav");
        mimetype = "audio/wav" ;
    }
    else if (fileformat == FileFormatOGG || (fileformat == FileFormatAuto && usefile.getFileExtension().toLowerCase() == ".ogg")) {
        audioFormat = std::make_unique<OggVorbisAudioFormat>();
        qualindex = 8; // 256k
        usefile = usefile.withFileExtension(".ogg");
        mimetype = "audio/ogg" ;
    }
    else {
        mLastError = TRANS("Could not find format for filename");
        DBG(mLastError);
        return false;
    }

    bool userwriting = false;

    
#if JUCE_ANDROID

    auto makeStream = [this,mimetype](const URL & parent, String & name, URL & returl) {
        auto parentTree = AndroidDocument::fromDocument(parent);
        if (!parentTree.hasValue() && parent.isLocalFile()) {
            parentTree = AndroidDocument::fromFile(parent.getLocalFile());
        }
        if (parentTree.hasValue()) {
            auto fileurl = parent.getChildURL(name);
            //auto doc = AndroidDocument::fromDocument(fileurl);
            auto bname = File::getCurrentWorkingDirectory().getChildFile(name).getFileNameWithoutExtension();
            auto doc = parentTree.createChildDocumentWithTypeAndName(mimetype, bname);
            if (!doc.hasValue()) {
                // fall back to file ops
                DBG("creating fallback file stream");

                if (fileurl.isLocalFile()) {
                    auto file = fileurl.getLocalFile().getNonexistentSibling();
                    name = file.getFileName();
                    returl = URL(file);
                    return std::unique_ptr<OutputStream> (file.createOutputStream());
                }
            }
            else {
                // TODO, prevent creating existing filename
                DBG("creating doc stream: " << doc.getUrl().toString(false));
                returl = doc.getUrl();
                return doc.createOutputStream();
            }
        }
        return std::unique_ptr<OutputStream>();
    };

    auto deleteExisting = [this,mimetype](const URL & parent, String name) {
        auto childurl = parent.getChildURL(name);
        auto doc = AndroidDocument::fromDocument(childurl);
        
        if (doc.hasValue()) {
            DBG("deleting doc: " << doc.getUrl().toString(false));
            return doc.deleteDocument();
        }
        else if (childurl.isLocalFile()) {
            return childurl.getLocalFile().deleteFile();
        }

        return false;
    };

    auto makeReturnUrl = [this,mimetype](const URL & parent, String name) {
        auto childurl = parent.getChildURL(name);
        return childurl;
    };
    
    
    auto makeChildDirUrl = [this](const URL & parent, String name) {
        auto parentTree = AndroidDocument::fromDocument(parent);
        if (!parentTree.hasValue() && parent.isLocalFile()) {
            parentTree = AndroidDocument::fromFile(parent.getLocalFile());
        }
        if (parentTree.hasValue()) {
            auto childDir = parentTree.createChildDirectory(name);
            // TODO, create unique name
            if (childDir.hasValue()) {
                DBG("Created child dir: " << childDir.getUrl().toString(false));
                return childDir.getUrl();
            }
        }
        else {
            auto recdirurl = parent.getChildURL(name);
            if (recdirurl.isLocalFile()) {
                File recdir = recdirurl.getLocalFile().getNonexistentSibling();
                if (recdir.createDirectory()) {
                    DBG("Created child dir as file: " << name);
                    return URL(recdir);
                }
            }
        }
            
        return URL();
    };

    
#else
    
    auto makeStream = [this](const URL & parent, String & name, URL & returl) {
        URL fileurl = parent.getChildURL(name);
        if (fileurl.isLocalFile()) {
            auto file = fileurl.getLocalFile().getNonexistentSibling();
            name = file.getFileName();
            returl = URL(file);
            return std::unique_ptr<OutputStream> (file.createOutputStream());
        }
        return std::unique_ptr<OutputStream>();
    };

    auto deleteExisting = [this](const URL & parent, String name) {
        URL fileurl = parent.getChildURL(name);
        if (fileurl.isLocalFile()) {
            return fileurl.getLocalFile().deleteFile();
        }
        return false;
    };

    auto makeReturnUrl = [this](const URL & parent, String name) {
        return parent.getChildURL(name);
    };
    
    
    auto makeChildDirUrl = [this](const URL & parent, String name) {
        auto recdirurl = parent.getChildURL(name);
        if (recdirurl.isLocalFile()) {
            File recdir = recdirurl.getLocalFile().getNonexistentSibling();
            if (recdir.createDirectory()) {
                return URL(recdir);
            }
        }
            
        return URL();
    };

#endif
    
    
    
    if (recordOptions == RecordMix) {

        // Create an OutputStream to write to our destination file...
        deleteExisting(recordLocationUrl, usefile.getFileName());
        
        String filename = usefile.getFileName();
        URL returl;
        
        if (auto fileStream = makeStream(recordLocationUrl, filename, returl))
        {
            if (auto writer = audioFormat->createWriterFor (fileStream.get(), getSampleRate(), totalRecordingChannels, bitsPerSample, {}, qualindex))
            {
                fileStream.release(); // (passes responsibility for deleting the stream to the writer object that is now using it)
                
                // Now we'll create one of these helper objects which will act as a FIFO buffer, and will
                // write the data to disk on our background thread.
                threadedMixWriter.reset (new AudioFormatWriter::ThreadedWriter (writer, *recordingThread, 32768));
                
                DBG("Started recording only mix file " << returl.toString(false));

                mainreturl = returl;
                ret = true;
            } else {
                mLastError.clear();
                mLastError << TRANS("Error creating writer for ") << returl.toString(false);
                DBG(mLastError);
            }
        } else {
            auto returl = makeReturnUrl(recordLocationUrl, filename);
            mLastError.clear();
            mLastError << TRANS("Error creating output file: ") << returl.toString(false);
            DBG(mLastError);
        }
        
    }
    else {
        // make directory from the filename
        auto recdir = makeChildDirUrl(recordLocationUrl, usefile.getFileNameWithoutExtension());
        
        //File recdir = usefile.getParentDirectory().getChildFile(usefile.getFileNameWithoutExtension()).getNonexistentSibling();
        //if (!recdir.createDirectory()) {
        if (recdir.isEmpty()) {
            mLastError.clear();
            mLastError << TRANS("Error creating directory for recording: ") << makeReturnUrl(recordLocationUrl, usefile.getFileNameWithoutExtension()).toString(false);
            DBG(mLastError);
            return false;
        }

        // create writers for all appropriate things

        if (recordOptions & RecordMixMinusSelf) {
            String filename = usefile.getFileNameWithoutExtension() + "-MIXMINUS" + usefile.getFileExtension();
            filename = File::createLegalFileName(filename);
            
            //File thefile = recdir.getChildFile(filename).getNonexistentSibling();
            URL returl;
            
            if (auto fileStream = makeStream(recdir, filename, returl))
            //if (auto fileStream = std::unique_ptr<FileOutputStream> (thefile.createOutputStream()))
            {
                if (auto writer = audioFormat->createWriterFor (fileStream.get(), getSampleRate(), totalRecordingChannels, bitsPerSample, {}, qualindex))
                {
                    fileStream.release(); // (passes responsibility for deleting the stream to the writer object that is now using it)
                    
                    // Now we'll create one of these helper objects which will act as a FIFO buffer, and will
                    // write the data to disk on our background thread.
                    threadedMixMinusWriter.reset (new AudioFormatWriter::ThreadedWriter (writer, *recordingThread, 32768));

                    DBG("Created mix minus output file: " << returl.toString(false));
             
                    mainreturl = returl;
                    ret = true;
                } else {
                    DBG("Error creating mix minus writer for " << returl.toString(false));
                }
            } else {
                DBG("Error creating mix minus output file: " << makeReturnUrl(recdir, filename).toString(false));
            }
        }
        
        if (recordOptions & RecordSelf) {
            mSelfRecordChannels = mActiveInputChannels;

            for (int i=0; i < mInputChannelGroupCount && i < MAX_CHANGROUPS; ++i) {
                int chans = mInputChannelGroups[i].params.numChannels;
                mSelfRecordChans[i] = chans;
                String inname = mInputChannelGroups[i].params.name;
                String filename = usefile.getFileNameWithoutExtension() + (inname.isEmpty() ? "-SELF" : ("-SELF-" + inname)) + usefile.getFileExtension();
                filename = File::createLegalFileName(filename);
                
                //File thefile = recdir.getChildFile(filename).getNonexistentSibling();
                URL returl;

                if (auto fileStream = makeStream(recdir, filename, returl))
                //if (auto fileStream = std::unique_ptr<FileOutputStream> (thefile.createOutputStream()))
                {
                    if (auto writer = audioFormat->createWriterFor (fileStream.get(), getSampleRate(), chans, bitsPerSample, {}, qualindex))
                    {
                        fileStream.release(); // (passes responsibility for deleting the stream to the writer object that is now using it)

                        // Now we'll create one of these helper objects which will act as a FIFO buffer, and will
                        // write the data to disk on our background thread.
                        threadedSelfWriters.add (new AudioFormatWriter::ThreadedWriter (writer, *recordingThread, 32768));

                        DBG("Created self output file: " << returl.toString(false));

                        mainreturl = returl;
                        ret = true;

                    } else {
                        DBG("Error creating self writer for " << returl.toString(false));
                    }
                } else {
                    DBG("Error creating self output file: " << makeReturnUrl(recdir, filename).toString(false));
                }
            }
        }

        if (recordOptions & RecordMix) {
            String filename = usefile.getFileNameWithoutExtension() + "-MIX" + usefile.getFileExtension();
            filename = File::createLegalFileName(filename);
            
            //File thefile = recdir.getChildFile(filename).getNonexistentSibling();
            URL returl;

            if (auto fileStream = makeStream(recdir, filename, returl))
            //if (auto fileStream = std::unique_ptr<FileOutputStream> (thefile.createOutputStream()))
            {

                if (auto writer = audioFormat->createWriterFor (fileStream.get(), getSampleRate(), totalRecordingChannels, bitsPerSample, {}, qualindex))
                {
                    fileStream.release(); // (passes responsibility for deleting the stream to the writer object that is now using it)
                    
                    // Now we'll create one of these helper objects which will act as a FIFO buffer, and will
                    // write the data to disk on our background thread.
                    threadedMixWriter.reset (new AudioFormatWriter::ThreadedWriter (writer, *recordingThread, 32768));

                    DBG("Created mix output file: " << returl.toString(false));

                    mainreturl = returl;
                    ret = true;
                } else {
                    DBG("Error creating mix writer for " << returl.toString(false));
                }
            } else {
                DBG("Error creating mix output file: " << makeReturnUrl(recdir, filename).toString(false));
            }
        }

        
       

        
        if (recordOptions & RecordIndividualUsers) {
            const ScopedReadLock sl (mCoreLock);        

            for (auto & remote : mRemotePeers) {

                int numchan = remote->recvChannels;

                AudioFormat * useformat = audioFormat.get();
                String fileext = usefile.getFileExtension();

                if (fileformat == FileFormatFLAC && numchan > 8) {
                    if (!wavAudioFormat) {
                        wavAudioFormat = std::make_unique<WavAudioFormat>();
                    }
                    useformat = wavAudioFormat.get();
                    fileext = ".wav";
                }

                String userfilename = usefile.getFileNameWithoutExtension() + "-" + remote->userName + fileext;
                userfilename = File::createLegalFileName(userfilename);

                //File thefile = recdir.getChildFile(userfilename).getNonexistentSibling();
                URL returl;

                if (auto fileStream = makeStream(recdir, userfilename, returl))
                // if (auto fileStream = std::unique_ptr<FileOutputStream> (thefile.createOutputStream()))
                {

                    // flac has a max of FLAC__MAX_CHANNELS, if we exceed that, fallback to WAV
                    if (auto writer = useformat->createWriterFor (fileStream.get(), getSampleRate(), numchan, bitsPerSample, {}, qualindex))
                    {
                        fileStream.release(); // (passes responsibility for deleting the stream to the writer object that is now using it)
                        
                        // Now we'll create one of these helper objects which will act as a FIFO buffer, and will
                        // write the data to disk on our background thread.
                        remote->fileWriter = std::make_unique<AudioFormatWriter::ThreadedWriter>(writer, *recordingThread, 32768);

                        DBG("Created user output file: " << returl.toString(false));
                        ret = true;
                        userwriting = true;
                    } else {
                        DBG("Error user writer for " << returl.toString(false));
                    }
                } else {
                    DBG("Error creating user output file: " << makeReturnUrl(recdir, filename).toString(false));
                }
            }
        }
    }
    
    if (ret) {
        // And now, swap over our active writer pointers so that the audio callback will start using it..
        const ScopedLock sl (writerLock);
        mElapsedRecordSamples = 0;
        activeMixWriter = threadedMixWriter.get();
        activeMixMinusWriter = threadedMixMinusWriter.get();

        for (int i=0; i < MAX_CHANGROUPS; ++i) {
            if (i < threadedSelfWriters.size()) {
                activeSelfWriters[i] = threadedSelfWriters.getUnchecked(i);
            }
            else {
                activeSelfWriters[i] = nullptr;
            }
        }

        writingPossible.store(activeMixWriter || threadedSelfWriters.size() || activeMixMinusWriter);

        userWritingPossible.store(userwriting);

        //DBG("Started recording file " << usefile.getFullPathName());
    }

    sendRemotePeerInfoUpdate();

    return ret;
}

bool SonobusAudioProcessor::stopRecordingToFile()
{
    // First, clear this pointer to stop the audio callback from using our writer object..

    OwnedArray<AudioFormatWriter::ThreadedWriter> userwriters;

    {
        const ScopedReadLock scl (mCoreLock);

        const ScopedLock sl (writerLock);
        activeMixWriter = nullptr;
        activeMixMinusWriter = nullptr;
        for (int i=0; i < MAX_CHANGROUPS; ++i) {
            activeSelfWriters[i] = nullptr;
        }

        writingPossible.store(false);
        userWritingPossible.store(false);

        // transfer ownership of writers to our temporary OwnedArray to be cleared below
        for (auto & remote : mRemotePeers) {
            if (remote->fileWriter) {
                userwriters.add(std::move(remote->fileWriter));
            }
        }

    }
    
    bool didit = false;
    
    if (threadedMixWriter) {
        
        // Now we can delete the writer object. It's done in this order because the deletion could
        // take a little time while remaining data gets flushed to disk, and we can't be blocking
        // the audio callback while this happens.
        threadedMixWriter.reset();
        
        DBG("Stopped recording mix file");
        didit = true;
    }

    if (!threadedSelfWriters.isEmpty()) {
        threadedSelfWriters.clearQuick(true);
        DBG("Stopped recording self file(s)");
        didit = true;
    }

    if (threadedMixMinusWriter) {
        threadedMixMinusWriter.reset();
        
        DBG("Stopped recording mix-minus file");
        didit = true;
    }

    // cleanup any user writers
    if (!userwriters.isEmpty()) {
        userwriters.clear();
        DBG("Stopped recording user files");
        didit = true;
    }

    sendRemotePeerInfoUpdate();

    return didit;
}

bool SonobusAudioProcessor::isRecordingToFile()
{
    return (activeMixWriter.load() != nullptr 
            || threadedSelfWriters.size() > 0
            || activeMixMinusWriter.load() != nullptr 
            || userWritingPossible.load()
            );
}

void SonobusAudioProcessor::clearTransportURL()
{
    // unload the previous file source and delete it..
    mTransportSource.stop();
    mTransportSource.setSource (nullptr);
    mCurrentAudioFileSource.reset();
    mCurrTransportURL = URL();
}

bool SonobusAudioProcessor::loadURLIntoTransport (const URL& audioURL)
{
    if (!mDiskThread.isThreadRunning()) {
        mDiskThread.startThread (Thread::Priority::normal);
    }

    // unload the previous file source and delete it..
    clearTransportURL();
    
    AudioFormatReader* reader = nullptr;
    
#if ! (JUCE_IOS || JUCE_ANDROID)
    if (audioURL.isLocalFile())
    {
        reader = mFormatManager.createReaderFor (audioURL.getLocalFile());
    }
    else
#endif
    {
        if (reader == nullptr) {
#if JUCE_ANDROID
            auto doc = AndroidDocument::fromDocument(audioURL);
            if (!doc.hasValue()) {
                DBG("Fallback to from file for audiourl: " << audioURL.toString(false));
                doc = AndroidDocument::fromFile(audioURL.getLocalFile());
            }

            if (doc.hasValue()) {
                if (doc.getInfo().canRead()) {

                    DBG("Opening Android doc: " << doc.getUrl().toString(false));
                    if (auto istr = doc.createInputStream()) {
                        reader = mFormatManager.createReaderFor (std::move(istr));
                    }
                }
                else {
                    DBG("No permission to read android doc with URL: " << audioURL.toString(false));
                }
            }
#else
            reader = mFormatManager.createReaderFor (audioURL.createInputStream(URL::InputStreamOptions(URL::ParameterHandling::inAddress)));
#endif
        }
    }


    if (reader != nullptr)
    {
        mCurrTransportURL = URL(audioURL);

        mCurrentAudioFileSource.reset (new AudioFormatReaderSource (reader, true));

        mTransportSource.prepareToPlay(currSamplesPerBlock, getSampleRate());

        // ..and plug it into our transport source
        mTransportSource.setSource (mCurrentAudioFileSource.get(),
                                    65536,                   // tells it to buffer this many samples ahead
                                    &mDiskThread,                 // this is the background thread to use for reading-ahead
                                    reader->sampleRate,     // allows for sample rate correction
                                    reader->numChannels);

        return true;
    }

    return false;
}


#pragma Effects

void SonobusAudioProcessor::setMainReverbEnabled(bool flag)
{    
    mMainReverbEnabled = flag;
    mState.getParameter(paramMainReverbEnabled)->setValueNotifyingHost(flag ? 1.0f : 0.0f);
}

void  SonobusAudioProcessor::setMainReverbDryLevel(float level)
{
    // not used
    mMainReverbParams.dryLevel = jlimit(0.0f, 1.0f, level);
    mMainReverb->setParameters(mMainReverbParams);    
}

float SonobusAudioProcessor::getMainReverbDryLevel() const
{
    // not used
    return mMainReverbParams.dryLevel;
}

void  SonobusAudioProcessor::setMainReverbWetLevel(float level)
{
    mState.getParameter(paramMainReverbLevel)->setValueNotifyingHost(level);

   // mMainReverbParams.wetLevel = jlimit(0.0f, 1.0f, level);
   // mMainReverb->setParameters(mMainReverbParams);
}


void  SonobusAudioProcessor::setMainReverbSize(float value)
{
    //mMainReverbParams.roomSize = jlimit(0.0f, 1.0f, value);
    //mMainReverb->setParameters(mMainReverbParams);    
    mState.getParameter(paramMainReverbSize)->setValueNotifyingHost(value);
}

void  SonobusAudioProcessor::setMainReverbPreDelay(float valuemsec)
{
    mState.getParameter(paramMainReverbPreDelay)->setValueNotifyingHost(mState.getParameter(paramMainReverbPreDelay)->convertTo0to1(valuemsec));
}

void  SonobusAudioProcessor::setInputReverbWetLevel(float level)
{
    mState.getParameter(paramInputReverbLevel)->setValueNotifyingHost(level);
}


void  SonobusAudioProcessor::setInputReverbSize(float value)
{
    mState.getParameter(paramInputReverbSize)->setValueNotifyingHost(value);
}

void  SonobusAudioProcessor::setInputReverbPreDelay(float valuemsec)
{
    mState.getParameter(paramInputReverbPreDelay)->setValueNotifyingHost(mState.getParameter(paramInputReverbPreDelay)->convertTo0to1(valuemsec));
}



void SonobusAudioProcessor::setMainReverbModel(ReverbModel flag) 
{ 
    mMainReverbModel = flag; 
    mState.getParameter(paramMainReverbModel)->setValueNotifyingHost(mState.getParameter(paramMainReverbModel)->convertTo0to1(flag));
}

double SonobusAudioProcessor::getMonitoringDelayTimeFromAvgPeerLatency(float scalar)
{
    double deltimems = 0.0f;
    int cnt = 0;

    for (int i = 0; i < mRemotePeers.size(); ++i) {
        LatencyInfo info;
        if (getRemotePeerLatencyInfo(i, info)) {
            if (info.isreal) {
                deltimems += info.outgoingMs;
                ++cnt;
            }
        }
    }

    if (cnt > 0) {
        deltimems = (deltimems / cnt);
    }

    DBG("Set monitoring delay avg: " << deltimems << " * " << scalar << " = " << deltimems*scalar);

    deltimems *= scalar;

    return deltimems;
    //setMonitoringDelayTimeMs(deltimems);
}

// called in NRT thread
AooServerWrapper::AooServerWrapper(SonobusAudioProcessor & proc, int port, const String & password)
    : processor_(proc), port_(port)
{
    try {
        // setup UDP server
        udpserver_.start(port,
                         [this](auto&&... args) { handleUdpReceive(args...); });
    } catch (const std::exception& e){
        DBG("Error binding UDP: " << e.what());
        return;
    }

    try {
        // setup TCP server
        tcpserver_.start(port,
                         [this](auto&&... args) { return handleAccept(args...); },
                         [this](auto&&... args) { return handleReceive(args...); });
    } catch (const std::exception& e){
        DBG("Error binding TCP: " << e.what());
        udpserver_.stop();
        return;
    }

    // success
    server_ = ::AooServer::create(nullptr);
    if (password.isNotEmpty()) {
        server_->setPassword(password.toRawUTF8());
    }
    LOG_VERBOSE("AooServer: listening on port " << port);
    // first set event handler!
    server_->setEventHandler([](void *x, const AooEvent *e, AooThreadLevel level) {
        static_cast<AooServerWrapper *>(x)->handleEvent(e, level);
    }, this, kAooEventModeCallback);
    // then start network threads
    udpthread_ = std::thread([this](){
        udpserver_.run(-1);
    });
    tcpthread_ = std::thread([this](){
        tcpserver_.run();
    });
}

AooServerWrapper::~AooServerWrapper(){
    if (server_) {
        udpserver_.stop();
        if (udpthread_.joinable()) {
            udpthread_.join();
        }

        tcpserver_.stop();
        if (tcpthread_.joinable()) {
            tcpthread_.join();
        }
    }
}

void AooServerWrapper::handleEvent(const AooEvent *event, AooThreadLevel level){

    processor_.handleAooServerEvent(event, level);
}

AooId AooServerWrapper::handleAccept(int e, const aoo::ip_address& addr) {
    if (e == 0) {
        // reply function
        auto replyfn = [](void *x, AooId client,
                const AooByte *data, AooSize size) -> AooInt32 {
            return static_cast<aoo::tcp_server *>(x)->send(client, data, size);
        };
        AooId client;
        server_->addClient(replyfn, &tcpserver_, &client); // doesn't fail
        return client;
    } else {
        DBG("AooServer: accept() failed: " << aoo::socket_strerror(e));
        // TODO handle error?
        return kAooIdInvalid;
    }
}

void AooServerWrapper::handleReceive(int e, AooId client, const aoo::ip_address& addr, const AooByte *data, AooSize size) {
    if (e == 0 && size > 0) {
        if (server_->handleClientMessage(client, data, size) != kAooOk) {
            server_->removeClient(client);
            tcpserver_.close(client);
        }
    } else {
        // remove client!
        server_->removeClient(client);
        if (e == 0) {
            DBG("AooServer: client " << client << " disconnected");
        } else {
            DBG("AooServer: TCP error in client "
                      << client << ": " << aoo::socket_strerror(e));
        }
    }
}

void AooServerWrapper::handleUdpReceive(int e, const aoo::ip_address& addr,
                                 const AooByte *data, AooSize size) {
    if (e == 0) {
        // reply function
        auto replyfn = [](void *x, const AooByte *data, AooInt32 size,
                const void *address, AooAddrSize addrlen, AooFlag) -> AooInt32 {
            aoo::ip_address addr((const struct sockaddr *)address, addrlen);
            return static_cast<aoo::udp_server *>(x)->send(addr, data, size);
        };
        server_->handleUdpMessage(data, size, addr.address(), addr.length(),
                                  replyfn, &udpserver_);
    } else {
        DBG("AooServer: UDP error: " << aoo::socket_strerror(e));
        // TODO handle error?
    }
}




//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SonobusAudioProcessor();
}
