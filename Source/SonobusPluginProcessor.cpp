// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell



#include "SonobusPluginProcessor.h"
#include "SonobusPluginEditor.h"

#include "RunCumulantor.h"


#include "aoo/aoo_net.h"
#include "aoo/aoo_pcm.h"
#include "aoo/aoo_opus.h"

#include "oscpack/osc/OscOutboundPacketStream.h"
#include "oscpack/osc/OscReceivedElements.h"

#include "mtdm.h"

#include <algorithm>

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
String SonobusAudioProcessor::paramMaxRecvPaddingMs  ("maxrecvpadms");

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
static String recordSelfSilenceMutedKey("RecordSelfSilenceWhenMuted");
static String recordFinishOpenKey("RecordFinishOpen");
static String recordStealthKey("RecordStealth");
static String defRecordDirKey("DefaultRecordDir");
static String defRecordDirURLKey("DefaultRecordDirURL");
static String lastBrowseDirKey("LastBrowseDir");
static String oscEnabledKey("OSCEnabled");
static String oscSendStateOnStartKey("OSCSendStateOnStart");
static String oscTargetIPAddressKey("OSCTargetIPAddress");
static String oscTargetPortKey("OSCTargetPort");
static String oscReceivePortKey("OSCReceivePort");
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
static String useUnivFontKey("useUnivFont");
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




#define LATENCY_ID_OFFSET 20000
#define ECHO_ID_OFFSET    40000

enum {
    RemoteNetTypeUnknown = 0,
    RemoteNetTypeEthernet = 1,
    RemoteNetTypeWifi = 2,
    RemoteNetTypeMobileData = 3
};


struct SonobusAudioProcessor::RemotePeer {
    RemotePeer(EndpointState * ep = 0, int id_=0, aoo::isink::pointer oursink_ = 0, aoo::isource::pointer oursource_ = 0) : endpoint(ep), 
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
        
        oursink.reset(aoo::isink::create(ourId));
        oursource.reset(aoo::isource::create(ourId));
        
        // create latency sink/sources
        latencysink.reset(aoo::isink::create(ourId + LATENCY_ID_OFFSET));
        latencysource.reset(aoo::isource::create(ourId + LATENCY_ID_OFFSET));
        echosink.reset(aoo::isink::create(ourId + ECHO_ID_OFFSET));
        echosource.reset(aoo::isource::create(ourId + ECHO_ID_OFFSET));
    }

    EndpointState * endpoint = 0;
    int32_t ourId = AOO_ID_NONE;
    int32_t remoteSinkId = AOO_ID_NONE;
    int32_t remoteSourceId = AOO_ID_NONE;
    aoo::isink::pointer oursink;
    aoo::isource::pointer oursource;

    aoo::isink::pointer latencysink;
    aoo::isource::pointer latencysource;
    aoo::isink::pointer echosink;
    aoo::isource::pointer echosource;
    bool activeLatencyTest = false;
    std::unique_ptr<MTDM> latencyProcessor;
    std::unique_ptr<LatencyMeasurer> latencyMeasurer;

    float gain = 1.0f;

    float buffertimeMs = 0.0f;
    float padBufferTimeMs = 0.0f;
    AutoNetBufferMode  autosizeBufferMode = AutoNetBufferModeAutoFull;
    bool sendActive = false;
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

    std::unique_ptr<AudioFormatWriter::ThreadedWriter> fileWriter;

    ReadWriteLock    sinkLock;
};



static int32_t endpoint_send(void *e, const char *data, int32_t size)
{
    SonobusAudioProcessor::EndpointState * endpoint = static_cast<SonobusAudioProcessor::EndpointState*>(e);
    int result = -1;
    if (endpoint->peer) {
        result = endpoint->owner->write(*(endpoint->peer), data, size);
    } else {
        result = endpoint->owner->write(endpoint->ipaddr, endpoint->port, data, size);
    }
    
    if (result > 0) {
        // include UDP overhead
        endpoint->sentBytes += result + UDP_OVERHEAD_BYTES;
    }
    else if (result < 0) {
        DBG("Error sending bytes to endpoint " << endpoint->ipaddr);
    }
    return result;
}

static int32_t client_send(void *e, const char *data, int32_t size, void *raddr)
{
    SonobusAudioProcessor::EndpointState * endpoint = static_cast<SonobusAudioProcessor::EndpointState*>(e);
    int result = -1;

    const struct sockaddr *addr = (struct sockaddr *)raddr;
    
    if (addr->sa_family == AF_INET){
        result = (int) ::sendto(endpoint->owner->getRawSocketHandle(), data, (size_t)size, 0, addr, sizeof(struct sockaddr_in));
    }
    
    if (result > 0) {
        endpoint->sentBytes += result + UDP_OVERHEAD_BYTES;
    }
    
    return result;
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
         
            if (_processor.mUdpSocket->waitUntilReady(true, 20) == 1) {
                _processor.doReceiveData();
            }
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

        if (_processor.mAooServer) {
            _processor.mAooServer->run();
        }
        
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
            _processor.mAooClient->run();
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
    std::make_unique<AudioParameterFloat>(ParameterID(paramMaxRecvPaddingMs, 1),     TRANS ("Sync Receive Padding"),    NormalisableRange<float>(0.0, 500.0, 1.0, 1.0), mMaxRecvPaddingMs.get(), "", AudioProcessorParameter::genericParameter,
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
    mState.addParameterListener (paramMaxRecvPaddingMs, this);

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

    // Initialize OSC only if enabled
    if (mOSCEnabled) {
        // Initialize the OSC receiver with configurable port
        if (!oscManager.initializeReceiver(mOSCReceivePort))
        {
            juce::Logger::writeToLog("Failed to initialize OSC Receiver on port " + juce::String(mOSCReceivePort));
        }

        // Initialize the OSC sender with configurable target
        if (!oscManager.initializeSender(mOSCTargetIPAddress, mOSCTargetPort))
        {
            juce::Logger::writeToLog("Failed to initialize OSC Sender to " + mOSCTargetIPAddress + ":" + juce::String(mOSCTargetPort));
        }
    }

    mFreshInit = false; // need to ensure this before loaddefaultpluginstate

    if (isplugin) {
        loadDefaultPluginSettings();
    }
    
    loadGlobalState();
    
    moveOldMisplacedFiles();

    mFreshInit = true;
}



SonobusAudioProcessor::~SonobusAudioProcessor()
{
    mTransportSource.setSource(nullptr);
    mTransportSource.removeChangeListener(this);

    cleanupAoo();
}

void SonobusAudioProcessor::moveOldMisplacedFiles()
{
    // old dummy mistake location
    PropertiesFile::Options dummyoptions;
    dummyoptions.applicationName     = "dummy";
    dummyoptions.filenameSuffix      = ".xml";
    dummyoptions.osxLibrarySubFolder = "Application Support/SonoBus";
   #if JUCE_LINUX
    dummyoptions.folderName          = "~/.config/sonobus";
   #else
    dummyoptions.folderName          = "";
   #endif
    auto dummyloc = dummyoptions.getDefaultFile().getParentDirectory();
    
    if (dummyloc.getFullPathName().contains("dummy") && dummyloc.exists()) {
        // move any files from here into our real one... (and outside the iterator loop in case it gets mad)
        std::vector<File> tomove;
        for (auto entry : RangedDirectoryIterator(dummyloc, false)) {
            tomove.push_back(entry.getFile());
        }
        for (auto & file : tomove) {
            auto targfile = mSupportDir.getNonexistentChildFile(file.getFileNameWithoutExtension(), file.getFileExtension());
            if (file.moveFileTo(targfile)) {
                DBG("Moving old misplaced file: " << file.getFullPathName() << " to: " << targfile.getFullPathName());
            }
        }
    }
    
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
        
    aoo_initialize();
    

    const ScopedWriteLock sl (mCoreLock);        

    //mAooSink.reset(aoo::isink::create(1));

    mAooDummySource.reset(aoo::isource::create(0));



    
    //mAooSink->set_buffersize(mBufferTime.get()); // ms
    //mAooSource->set_buffersize(mBufferTime.get()); // ms
    
    //mAooSink->set_packetsize(1024);
    //mAooSource->set_packetsize(1024);
    

    
    mUdpSocket = std::make_unique<DatagramSocket>();
    mUdpSocket->setSendBufferSize(1048576);
    mUdpSocket->setReceiveBufferSize(1048576);

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
            if (mUdpSocket->bindToPort(udpport)) {
                udpport = mUdpSocket->getBoundPort();
                DBG("Bound udp port to " << udpport);
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
        if (!mUdpSocket->bindToPort(0)) {
            DBG("Error binding to any udp port!");
        }
        else {
            udpport = mUdpSocket->getBoundPort();
            DBG("Bound system chosen udp port to " << udpport);
        }
    }
    
    
    mUdpLocalPort = udpport;

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
    
    mServerEndpoint = std::make_unique<EndpointState>();
    mServerEndpoint->owner = mUdpSocket.get();
    
    if (mUdpLocalPort > 0) {
        mAooClient.reset(aoo::net::iclient::create(mServerEndpoint.get(), client_send, mUdpLocalPort));
    }

    
    mSendThread = std::make_unique<SendThread>(*this);
    mRecvThread = std::make_unique<RecvThread>(*this);
    mEventThread = std::make_unique<EventThread>(*this);

    if (mAooClient) {
        mClientThread = std::make_unique<ClientThread>(*this);
    }
    
    uint32_t estWorkDurationMs = 10; // just a guess
    int rtprio = 1; // all that is necessary

#if JUCE_WINDOWS
    // do not use startRealtimeThread() call because it triggers the whole process to be realtime, which we don't want
    mSendThread->startThread(Thread::Priority::highest);
    mRecvThread->startThread(Thread::Priority::highest);
#else
    if (!mSendThread->startRealtimeThread( juce::Thread::RealtimeOptions{}.withPriority(rtprio).withMaximumProcessingTimeMs(estWorkDurationMs)))
    {
        DBG("Send thread failed to start realtime: trying regular");
        mSendThread->startThread(Thread::Priority::highest);
    }

    if (!mRecvThread->startRealtimeThread(juce::Thread::RealtimeOptions{}.withPriority(rtprio).withMaximumProcessingTimeMs(estWorkDurationMs)))
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
        mAooClient->disconnect();
        mAooClient->quit();
        mClientThread->stopThread(400);        
    }

    
    {
        const ScopedWriteLock sl (mCoreLock);        

        mAooClient.reset();

        mUdpSocket.reset();
        
        mAooDummySource.reset();
        
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
        int32_t err;
        mAooServer.reset(aoo::net::iserver::create(10999, &err));
        
        if (err != 0) {
            DBG("Error creating Aoo Server: " << err);
        }
    }
    
    if (mAooServer) {
        mServerThread = std::make_unique<ServerThread>(*this);    
        mServerThread->startThread();
    }
}
    
void SonobusAudioProcessor::stopAooServer()
{
    if (mAooServer) {
        DBG("waiting on recv thread to die");
        mAooServer->quit();
        mServerThread->stopThread(400);    

        const ScopedWriteLock sl (mCoreLock);
        mAooServer.reset();
    }
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
    
    
    mServerEndpoint->ipaddr = host;
    mServerEndpoint->port = port;
    mServerEndpoint->peer.reset();

    mCurrentUsername = username;

    int32_t retval = mAooClient->connect(host.toRawUTF8(), port, username.toRawUTF8(), passwd.toRawUTF8());
    
    if (retval < 0) {
        DBG("Error connecting to server: " << retval);
    }
    
    return retval >= 0;
}

bool SonobusAudioProcessor::isConnectedToServer() const
{
    if (!mAooClient) return false;

    return mIsConnectedToServer;    
}

bool SonobusAudioProcessor::disconnectFromServer()
{
    if (!mAooClient) return false;
 
    mAooClient->disconnect();
    
    // disconnect from everything else!
    removeAllRemotePeers();

    {
        const ScopedLock sl (mClientLock);

        mIsConnectedToServer = false;
        mSessionConnectionStamp = 0.0;

        mCurrentJoinedGroup.clear();
    }

    {
        const ScopedLock sl (mPublicGroupsLock);

        mPublicGroupInfos.clear();
    }



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

    int32_t retval = mAooClient->group_watch_public(flag);

    const ScopedLock sl (mPublicGroupsLock);

    mPublicGroupInfos.clear();


    if (retval < 0) {
        DBG("Error watching public groups: " << retval);
    }

    return retval >= 0;

}


bool SonobusAudioProcessor::joinServerGroup(const String & group, const String & groupsecret, bool isPublic)
{
    if (!mAooClient) return false;

    int32_t retval = mAooClient->group_join(group.toRawUTF8(), groupsecret.toRawUTF8(), isPublic);
    
    if (retval < 0) {
        DBG("Error joining group " << group << " : " << retval);
    }
    
    return retval >= 0;
}

bool SonobusAudioProcessor::leaveServerGroup(const String & group)
{
    if (!mAooClient) return false;

    int32_t retval = mAooClient->group_leave(group.toRawUTF8());
    
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
        remote->oursource->setup(getSampleRate(), currSamplesPerBlock, remote->sendChannels);
        //remote->oursource->setup(getSampleRate(), remote->packetsize    , getTotalNumOutputChannels());        
        
        setupSourceFormat(remote, remote->latencysource.get(), true);
        remote->latencysource->setup(getSampleRate(), currSamplesPerBlock, 1);
        setupSourceFormat(remote, remote->echosource.get(), true);
        remote->echosource->setup(getSampleRate(), currSamplesPerBlock, 1);
        
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

    aoo_format_storage fmt;
    
    
    if (formatIndex >= 0) {
        const AudioCodecFormatInfo & info = mAudioFormats.getReference(formatIndex);

        if (formatInfoToAooFormat(info, remote->recvChannels, fmt)) {
            remote->oursink->request_source_codec_change(remote->endpoint, remote->remoteSourceId, fmt.header);

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

        remote->oursource->set_packetsize(remote->packetsize);
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




SonobusAudioProcessor::EndpointState * SonobusAudioProcessor::findOrAddRawEndpoint(void * rawaddr)
{
    String ipaddr;
    int port = 0 ;

    char hostip[INET6_ADDRSTRLEN];
    if (inet_ntop(AF_INET, get_in_addr((struct sockaddr *)rawaddr), hostip, sizeof(hostip)) == nullptr) {
        DBG("Error converting raw addr to IP");
        return nullptr;
    } else {
        ipaddr = hostip;
        port = ntohs(get_in_port((struct sockaddr *)rawaddr));        
        return findOrAddEndpoint(ipaddr, port);    
    }    
}

SonobusAudioProcessor::EndpointState * SonobusAudioProcessor::findOrAddEndpoint(const String & host, int port)
{
    const ScopedLock sl (mEndpointsLock);        
    
    EndpointState * endpoint = 0;
    
    for (auto ep : mEndpoints) {
        if (ep->ipaddr == host && ep->port == port) {
            endpoint = ep;
            break;
        }
    }
    
    if (!endpoint) {
        // add it as new
        endpoint = mEndpoints.add(new EndpointState(host, port));
        endpoint->owner = mUdpSocket.get();
        endpoint->peer = std::make_unique<DatagramSocket::RemoteAddrInfo>(host, port);
        DBG("Added new endpoint for " << host << ":" << port);
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
    char buf[AOO_MAXPACKETSIZE];
    String senderIP;
    int senderPort;
    
    int nbytes = mUdpSocket->read(buf, AOO_MAXPACKETSIZE, false, senderIP, senderPort);

    if (nbytes == 0) return;
    else if (nbytes < 0) {
        DBG("Error receiving UDP");
        return;
    }
    
    // find endpoint from sender info
    EndpointState * endpoint = findOrAddEndpoint(senderIP, senderPort);
    
    endpoint->recvBytes += nbytes + UDP_OVERHEAD_BYTES;
    
    // parse packet for AOO events
    
    int32_t type, id, dummyid;
    if ((aoo_parse_pattern(buf, nbytes, &type, &id) > 0)
        || (aoonet_parse_pattern(buf, nbytes, &type) > 0))
    {
        {
            
            if (type == AOO_TYPE_SINK){
                // forward OSC packet to matching sink(s)
                const ScopedReadLock sl (mCoreLock);        
                
                for (auto & remote : mRemotePeers) {
                    if (!remote->oursink) continue;
                    
                    if (id == AOO_ID_NONE) {
                        // this is a compact data message, try them all
                        if (remote->oursink->handle_message(buf, nbytes, endpoint, endpoint_send)) {
                            remote->dataPacketsReceived += 1;
                            if (remote->recvAllow && !remote->recvActive) {
                                remote->recvActive = true;
                            }
                            if (remote->resetSafetyMuted) {
                                updateSafetyMuting(remote);
                            }
                            break;
                        }
                    }
                    
                    if (id == AOO_ID_WILDCARD || (remote->oursink->get_id(dummyid) && id == dummyid) ) {
                        if (remote->oursink->handle_message(buf, nbytes, endpoint, endpoint_send)) {
                            remote->dataPacketsReceived += 1;
                            if (remote->recvAllow && !remote->recvActive) {
                                remote->recvActive = true;
                            }
                            if (remote->resetSafetyMuted) {
                                updateSafetyMuting(remote);
                            }
                        }
                        
                        if (id != AOO_ID_WILDCARD) break;
                    }
                    
                    if (remote->echosink->get_id(dummyid) && id == dummyid) {
                        remote->echosink->handle_message(buf, nbytes, endpoint, endpoint_send);
                        break;
                    }
                    else if (remote->latencysink->get_id(dummyid) && id == dummyid) {
                        remote->latencysink->handle_message(buf, nbytes, endpoint, endpoint_send);
                        break;
                    }
                    
                }
                
            } else if (type == AOO_TYPE_SOURCE){
                // forward OSC packet to matching sources(s)
                const ScopedReadLock sl (mCoreLock);        

                
                if (mAooDummySource->get_id(dummyid) && id == dummyid) {
                    // this is the special one that can accept blind invites
                    mAooDummySource->handle_message(buf, nbytes, endpoint, endpoint_send);
                }
                else {
                    for (auto & remote : mRemotePeers) {
                        if (!remote->oursource) continue;
                        if (id == AOO_ID_WILDCARD || (remote->oursource->get_id(dummyid) && id == dummyid)) {
                            remote->oursource->handle_message(buf, nbytes, endpoint, endpoint_send);
                            if (id != AOO_ID_WILDCARD) break;
                        }
                        
                        if (remote->echosource->get_id(dummyid) && id == dummyid) {
                            remote->echosource->handle_message(buf, nbytes, endpoint, endpoint_send);
                            break;
                        }
                        else if (remote->latencysource->get_id(dummyid) && id == dummyid) {
                            remote->latencysource->handle_message(buf, nbytes, endpoint, endpoint_send);
                            break;
                        }
                    }
                }

                
            } else if (type == AOO_TYPE_CLIENT || type == AOO_TYPE_PEER){
                // forward OSC packet to matching client

                //DBG("Got AOO_CLIENT or PEER data");

                if (mAooClient) {
                    mAooClient->handle_message(buf, nbytes, endpoint->getRawAddr());
                }
                
                /*
                 for (int i = 0; i < x->x_numclients; ++i){
                 if (pd_class(x->x_clients[i].c_obj) == aoo_client_class)
                 {
                 t_aoo_client *c = (t_aoo_client *)x->x_clients[i].c_obj;
                 aoo_client_handle_message(c, buf, nbytes,
                 ep, (aoo_replyfn)endpoint_send);
                 break;
                 }
                 }
                 */
            } else if (type == AOO_TYPE_SERVER){
                // ignore
                DBG("Got AOO_SERVER data");

                if (mAooServer) {
                    // mAooServer->handle_message(buf, nbytes, endpoint);
                }
                
            } else {
                DBG("SonoBus bug: unknown aoo type: " << type);
            }
        }

        // notify send thread
        notifySendThread();

    }
    else if (handleOtherMessage(endpoint, buf, nbytes)) {

    }
    else {
        // not a valid AoO OSC message
        DBG("SonoBus: not a valid AOO message!");
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

#define SONOBUS_MSG_SUGGEST_GROUP "/suggestgroup"
#define SONOBUS_MSG_SUGGEST_GROUP_LEN 14
#define SONOBUS_FULLMSG_SUGGEST_GROUP SONOBUS_MSG_DOMAIN SONOBUS_MSG_SUGGEST_GROUP


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
    SONOBUS_MSGTYPE_BLOCKEDINFO,
    SONOBUS_MSGTYPE_SUGGESTGROUP
};

static int32_t sonobusOscParsePattern(const char *msg, int32_t n, int32_t & rettype)
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
        else if (n >= (offset + SONOBUS_MSG_SUGGEST_GROUP_LEN)
            && !memcmp(msg + offset, SONOBUS_MSG_SUGGEST_GROUP, SONOBUS_MSG_SUGGEST_GROUP_LEN))
        {
            rettype = SONOBUS_MSGTYPE_SUGGESTGROUP;
            offset += SONOBUS_MSG_SUGGEST_GROUP_LEN;
            return offset;
        }
        else {
            return 0;
        }
    }
    return 0;
}

bool SonobusAudioProcessor::handleOtherMessage(EndpointState * endpoint, const char *msg, int32_t n)
{
    // try to parse it as an OSC /sb  message
    int32_t type = SONOBUS_MSGTYPE_UNKNOWN;
    int32_t onset = 0;

    if (! (onset = sonobusOscParsePattern(msg, n, type))) {
        return false;
    }

    try {
        osc::ReceivedPacket packet(msg, n);
        osc::ReceivedMessage message(packet);

        if (type == SONOBUS_MSGTYPE_PING) {
            // received from the other side
            // args: t:origtime

            auto it = message.ArgumentsBegin();
            auto tt = (it++)->AsTimeTag();

            // now prepare and send ack immediately
            auto tt2 = aoo_osctime_get(); // use real system time

            char buf[AOO_MAXPACKETSIZE];
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

            endpoint_send(endpoint, outmsg.Data(), (int) outmsg.Size());

            DBG("Received ping from " << endpoint->ipaddr << ":" << endpoint->port << "  stamp: " << tt);

        }
        else if (type == SONOBUS_MSGTYPE_PINGACK) {
            // received from the other side
            // args: t:origtime t:theirtime

            auto it = message.ArgumentsBegin();
            auto tt = (it++)->AsTimeTag();
            auto tt2 = (it++)->AsTimeTag();
            auto tt3 = aoo_osctime_get(); // use real system time

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
                    DBG("Could not find peer for endpoint");
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
                    DBG("Could not find peer for endpoint: " << endpoint->ipaddr << "src: " <<  sourceid);
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
                
                char buf[AOO_MAXPACKETSIZE];
                osc::OutboundPacketStream outmsg(buf, sizeof(buf));
                
                String jsonstr = JSON::toString(latinfo, true, 6);
                
                if (jsonstr.getNumBytesAsUTF8() > AOO_MAXPACKETSIZE - 100) {
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
                
                endpoint_send(endpoint, outmsg.Data(), (int) outmsg.Size());
                
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
            auto username = String(CharPointer_UTF8((it++)->AsString()));
            auto latency = (it++)->AsFloat();

            if (!isAddressBlocked(endpoint->ipaddr)) {
                clientListeners.call(&SonobusAudioProcessor::ClientListener::peerRequestedLatencyMatch, this, username, latency);
            }
        }
        else if (type == SONOBUS_MSGTYPE_SUGGESTGROUP) {
            // received from the other side
            // args: s:username  s:newgroup s:ispublic

            auto it = message.ArgumentsBegin();
            if (message.ArgumentCount() >= 1) {
                const void *infojson;
                osc::osc_bundle_element_size_t size;

                (it++)->AsBlob(infojson, size);

                String jsonstr = String::createStringFromData(infojson, size);

                juce::var infodata;
                auto result = juce::JSON::parse(jsonstr, infodata);

                if (!isAddressBlocked(endpoint->ipaddr) && infodata.isObject()) {
                    auto username = infodata.getProperty("user", "");
                    auto newgroup = infodata.getProperty("group", "");
                    auto grouppass = infodata.getProperty("group_pass", "");
                    auto ispublic = infodata.getProperty("public", false);
                    auto others = infodata.getProperty("others", {});
                    StringArray otherpeers;
                    if (others.isArray()) {
                        for (auto item : *others.getArray()) {
                            otherpeers.add(item);
                        }
                    }

                    clientListeners.call(&SonobusAudioProcessor::ClientListener::peerSuggestedNewGroup, this, username, newgroup, grouppass, ispublic, otherpeers);
                }
            }
        }
        else if (type == SONOBUS_MSGTYPE_BLOCKEDINFO) {
            // received from the other side when they have blocked/unblocked us
            // args: s:username  b:blocked

            auto it = message.ArgumentsBegin();
            auto username = String(CharPointer_UTF8((it++)->AsString()));
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
    char buf[AOO_MAXPACKETSIZE];
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
        this->sendPeerMessage(peer, msg.Data(), (int32_t) msg.Size());

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
    char buf[AOO_MAXPACKETSIZE];
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
    
    endpoint_send(endpoint, outmsg.Data(), (int) outmsg.Size());
}


void SonobusAudioProcessor::sendReqLatInfoToAll()
{
    char buf[AOO_MAXPACKETSIZE];
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
        this->sendPeerMessage(peer, msg.Data(), (int32_t) msg.Size());
    }
}

void SonobusAudioProcessor::sendLatencyMatchToAll(float latency)
{
    // suggest to all that they should adjust all receiving latencies to be this value

    char buf[AOO_MAXPACKETSIZE];
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
        this->sendPeerMessage(peer, msg.Data(), (int32_t) msg.Size());
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


void SonobusAudioProcessor::suggestNewGroupToPeers(const String & group, const String & groupPass, const StringArray & peernames, bool ispublic)
{
    // suggest to other peers to join a new group

    // send our info to this remote peer
    DynamicObject::Ptr info = new DynamicObject(); // this will delete itself

    info->setProperty("user", getCurrentUsername());
    info->setProperty("group", group);
    info->setProperty("group_pass", groupPass);
    info->setProperty("public", ispublic);

    info->setProperty("others", peernames);

    char buf[AOO_MAXPACKETSIZE];
    osc::OutboundPacketStream msg(buf, sizeof(buf));


    String jsonstr = JSON::toString(info.get(), true, 6);

    if (jsonstr.getNumBytesAsUTF8() > AOO_MAXPACKETSIZE - 100) {
        DBG("Info too big for packet!");
        return;
    }

    try {
        msg << osc::BeginMessage(SONOBUS_FULLMSG_SUGGEST_GROUP)
        << osc::Blob(jsonstr.toRawUTF8(), (int) jsonstr.getNumBytesAsUTF8())
        << osc::EndMessage;
    }
    catch (const osc::Exception& e){
        DBG("exception in suggest group message constructions: " << e.what());
        return;
    }

    const ScopedReadLock sl (mCoreLock);
    for (int i=0;  i < mRemotePeers.size(); ++i) {
        auto * peer = mRemotePeers.getUnchecked(i);
        if (peernames.contains(peer->userName)) {
            DBG("Sending invite to new group: " << group << " message to " << i);
            this->sendPeerMessage(peer, msg.Data(), (int32_t) msg.Size());
        }
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

    // make the recording icon appear on remote hosts unless we're stealth
    if (!mRecordStealth){
      info->setProperty("rec", isRecordingToFile());
    }

    // nettype TODO

    char buf[AOO_MAXPACKETSIZE];

    const ScopedReadLock sl (mCoreLock);
    for (int i=0;  i < mRemotePeers.size(); ++i) {
        auto * peer = mRemotePeers.getUnchecked(i);
        if (topeer && topeer != peer) continue;
        if (index >= 0 && index != i) continue;

        osc::OutboundPacketStream msg(buf, sizeof(buf));

        auto buftimeMs = jmax((double)peer->buffertimeMs, 1e3 * currSamplesPerBlock / getSampleRate());
        info->setProperty("jitbuf", buftimeMs);

        String jsonstr = JSON::toString(info.get(), true, 6);

        if (jsonstr.getNumBytesAsUTF8() > AOO_MAXPACKETSIZE - 100) {
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

        DBG("Sending peerinfo message to " << i);
        this->sendPeerMessage(peer, msg.Data(), (int32_t) msg.Size());

        if (index == i || topeer == peer) break;
    }

}


int32_t SonobusAudioProcessor::sendPeerMessage(RemotePeer * peer, const char *msg, int32_t n)
{
    return endpoint_send(peer->endpoint, msg, n);
}


void SonobusAudioProcessor::doSendData()
{
    // just try to send for everybody
    const ScopedReadLock sl (mCoreLock);        

    // send stuff until there is nothing left to send
    
    int32_t didsomething = 1;

    auto nowtimems = Time::getMillisecondCounterHiRes();

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

            if (remote->latencysource) {
                didsomething |= remote->latencysource->send();
                didsomething |= remote->latencysink->send();
                didsomething |= remote->echosource->send();
                didsomething |= remote->echosink->send();
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

struct ProcessorIdPair
{
    ProcessorIdPair(SonobusAudioProcessor *proc, int32_t id_) : processor(proc), id(id_) {}
    SonobusAudioProcessor * processor;
    int32_t id;
};

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


void SonobusAudioProcessor::handleEvents()
{
    const ScopedReadLock sl (mCoreLock);        
    int32_t dummy = 0;
    
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

        
        if (remote->latencysink) {
            remote->latencysink->get_id(dummy);
            ProcessorIdPair pp(this, dummy);
            remote->latencysink->handle_events(gHandleSinkEvents, &pp);
        }
        if (remote->echosink) {
            remote->echosink->get_id(dummy);
            ProcessorIdPair pp(this, dummy);
            remote->echosink->handle_events(gHandleSinkEvents, &pp);
        }
         
        if (remote->latencysource) {
            remote->latencysource->get_id(dummy);
            ProcessorIdPair pp(this, dummy);
            remote->latencysource->handle_events(gHandleSourceEvents, &pp);
        }
        if (remote->echosource) {
            remote->echosource->get_id(dummy);
            ProcessorIdPair pp(this, dummy);
            remote->echosource->handle_events(gHandleSourceEvents, &pp);
        }
        
    }

}

void SonobusAudioProcessor::sendPingEvent(RemotePeer * peer)
{

    auto tt = aoo_osctime_get();

    char buf[AOO_MAXPACKETSIZE];
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

    sendPeerMessage(peer, outmsg.Data(), (int32_t) outmsg.Size());

    DBG("Sent ping to peer: " << peer->endpoint->ipaddr);
}


void SonobusAudioProcessor::handlePingEvent(EndpointState * endpoint, uint64_t tt1, uint64_t tt2, uint64_t tt3)
{
    double diff1 = aoo_osctime_duration(tt1, tt2) * 1000.0;
    double diff2 = aoo_osctime_duration(tt2, tt3) * 1000.0;
    double rtt = aoo_osctime_duration(tt1, tt3) * 1000.0;

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


int32_t SonobusAudioProcessor::handleSourceEvents(const aoo_event ** events, int32_t n, int32_t sourceId)
{
    for (int i = 0; i < n; ++i){
        switch (events[i]->type){
        case AOO_PING_EVENT:
        {
            aoo_ping_event *e = (aoo_ping_event *)events[i];
            double diff1 = aoo_osctime_duration(e->tt1, e->tt2) * 1000.0;
            double diff2 = aoo_osctime_duration(e->tt2, e->tt3) * 1000.0;
            double rtt = aoo_osctime_duration(e->tt1, e->tt3) * 1000.0;

            EndpointState * es = (EndpointState *)e->endpoint;
            
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
        case AOO_INVITE_EVENT:
        {
            aoo_sink_event *e = (aoo_sink_event *)events[i];

            // accepts invites
            if (true){
                EndpointState * es = (EndpointState *)e->endpoint;
                // handle dummy source specially
                
                int32_t dummyid;
                
                if (mAooDummySource->get_id(dummyid) && dummyid == sourceId) {
                    // dummy source is special, and creates a remote peer

                    RemotePeer * peer = findRemotePeerByRemoteSinkId(es, e->id);
                    if (peer) {
                        // we already have a peer for this, interesting
                        DBG("Already had remote peer for " <<   es->ipaddr << ":" << es->port << "  ourId: " << peer->ourId);
                    } else if (!mIsConnectedToServer) {
                        peer = doAddRemotePeerIfNecessary(es);
                    }
                    else {
                        // connected to server, don't just respond to anyone
                        peer = findRemotePeer(es, -1);
                        if (!peer) {
                            DBG("Not reacting to invite from a peer not known in the group");
                            break;
                        }

                        DBG("Got invite from peer in the group");
                    }

                    const ScopedReadLock sl (mCoreLock);        

                    peer->remoteSinkId = e->id;

                    // add their sink
                    peer->oursource->add_sink(es, peer->remoteSinkId, endpoint_send);
                    peer->oursource->set_sinkoption(es, peer->remoteSinkId, aoo_opt_protocol_flags, &e->flags, sizeof(int32_t));

                    if (peer->sendAllow) {
                        peer->oursource->start();
                        peer->sendActive = true;
                    } else {
                        peer->oursource->stop();
                        peer->sendActive = false;
                    }
                    
                    DBG("Was invited by remote peer " <<  es->ipaddr << ":" << es->port << " sourceId: " << peer->remoteSinkId << "  ourId: " <<  peer->ourId);

                    // now try to invite them back at their port , with the same ID, they 
                    // should have a source waiting for us with the same id

                    DBG("Inviting them back to our sink");
                    peer->remoteSourceId = peer->remoteSinkId;
                    peer->oursink->invite_source(es, peer->remoteSourceId, endpoint_send);

                    // now remove dummy handshake one
                    mAooDummySource->remove_sink(es, dummyid);
                    
                }
                else {
                    // invited 
                    DBG("Invite received to our source: " << sourceId << " from " << es->ipaddr << ":" << es->port << "  " << e->id);

                    RemotePeer * peer = findRemotePeer(es, sourceId);
                    if (peer) {
                        
                        peer->remoteSinkId = e->id;

                        peer->oursource->add_sink(es, peer->remoteSinkId, endpoint_send);
                        peer->oursource->set_sinkoption(es, peer->remoteSinkId, aoo_opt_protocol_flags, &e->flags, sizeof(int32_t));
                        
                        if (peer->sendAllow) {
                            peer->oursource->start();
                            
                            peer->sendActive = true;
                            DBG("Starting to send, we allow it");
                        } else {
                            peer->sendActive = false;
                            peer->oursource->stop();
                        }
                        
                        peer->connected = true;

                        updateRemotePeerUserFormat(-1, peer);
                        sendRemotePeerInfoUpdate(-1, peer);

                        DBG("Finishing peer connection for " << es->ipaddr << ":" << es->port  << "  " << peer->remoteSinkId);
                        
                    }
                    else {
                        // find by echo id
                        if (auto * echopeer = findRemotePeerByEchoId(es, sourceId)) {
                            echopeer->echosource->add_sink(es, e->id, endpoint_send);                            
                            echopeer->echosource->start();
                            DBG("Invite to echo source adding sink " << e->id);
                        }
                        else if (auto * latpeer = findRemotePeerByLatencyId(es, sourceId)) {
                            echopeer->latencysource->add_sink(es, e->id, endpoint_send);                                                        
                            echopeer->latencysource->start();
                            DBG("Invite to our latency source adding sink " << e->id);
                        }
                        else {
                            // not one of our sources 
                            DBG("No source " << sourceId << " invited, how is this possible?");
                        }

                    }
                }
                
            } else {
                DBG("Invite received");
            }

            break;
        }
        case AOO_UNINVITE_EVENT:
        {
            aoo_sink_event *e = (aoo_sink_event *)events[i];

            // accepts uninvites
            if (true){
                EndpointState * es = (EndpointState *)e->endpoint;
                int32_t dummyid;

                
                RemotePeer * peer = findRemotePeerByRemoteSinkId(es, e->id);
                
                if (peer) {
                    int ourid = AOO_ID_NONE;
                    {
                        const ScopedReadLock sl (mCoreLock);        
                        ourid = peer->ourId;
                        peer->oursource->remove_all();
                        //peer->oursink->uninvite_all(); // ??
                        //peer->connected = false;
                        peer->sendActive = false;
                        peer->dataPacketsSent = 0;

                        if (!peer->recvActive) {
                            peer->connected = false;
                        }
                    }
                    
                    //doRemoveRemotePeerIfNecessary(es, ourid);
                    
                    DBG("Uninvited, removed remote peer " << es->ipaddr << ":" << es->port);
                    
                }
                else if (auto * echopeer = findRemotePeerByEchoId(es, sourceId)) {
                    echopeer->echosource->remove_sink(es, e->id);                            
                    echopeer->echosource->stop();
                    DBG("UnInvite to echo source adding sink " << e->id);
                }
                else if (auto * latpeer = findRemotePeerByLatencyId(es, sourceId)) {
                    echopeer->latencysource->remove_sink(es, e->id);
                    echopeer->latencysource->stop();
                    DBG("UnInvite to latency source adding sink " << e->id);
                }

                else {
                    DBG("Uninvite received to unknown");
                }

            } else {
                DBG("Uninvite received");
            }

            break;
        }
        case AOO_CHANGECODEC_EVENT:
        {
            aoo_source_event *e = (aoo_source_event *)events[i];
            DBG("Change codec received from sink " << e->id);
            
            EndpointState * es = (EndpointState *)e->endpoint;
            RemotePeer * peer = findRemotePeerByRemoteSinkId(es, e->id);
            
            if (peer) {
                // now we need to set our latency and echo source to match our main source's format
                aoo_format_storage fmt;
                if (peer->oursource->get_format(fmt) > 0) {
                    peer->latencysource->set_format(fmt.header);
                    peer->echosource->set_format(fmt.header);

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
        default:
            break;
        }
    }
    return 1;

}

int32_t SonobusAudioProcessor::handleSinkEvents(const aoo_event ** events, int32_t n, int32_t sinkId)
{
    for (int i = 0; i < n; ++i){
        switch (events[i]->type){
        case AOO_SOURCE_ADD_EVENT:
        {
            aoo_source_event *e = (aoo_source_event *)events[i];
            EndpointState * es = (EndpointState *)e->endpoint;

            RemotePeer * peer = findRemotePeer(es, sinkId);
            if (peer) {
                // someone has added us, thus accepting our invitation
                int32_t dummyid;
                                            
                if (mAooDummySource->get_id(dummyid) && dummyid == e->id ) {
                    // ignoring dummy add
                    DBG("Got dummy handshake add from " << es->ipaddr << ":" << es->port);
                }
                else {
                    DBG("Added source " << es->ipaddr << ":" << es->port << "  " <<  e->id  << " to our " << sinkId);
                    peer->remoteSourceId = e->id;
                    
                    peer->oursink->uninvite_source(es, 0, endpoint_send); // get rid of existing bogus one

                    if (peer->recvAllow) {
                        peer->oursink->invite_source(es, peer->remoteSourceId, endpoint_send);
                        //peer->recvActive = true;
                    } else {
                        DBG("we aren't accepting recv right now, politely decline it");
                        peer->oursink->uninvite_source(es, peer->remoteSourceId, endpoint_send);
                        peer->recvActive = false;                        
                    }
                    
                    peer->connected = true;
                    
                }
                
                // do invite here?
            }
            else {
                DBG("Added source to unknown " << e->id);
            }
            // add remote source
            //doAddRemoteSourceIfNecessary(es, e->id);


            break;
        }
        case AOO_SOURCE_FORMAT_EVENT:
        {
            aoo_source_event *e = (aoo_source_event *)events[i];
            EndpointState * es = (EndpointState *)e->endpoint;
            
            const ScopedReadLock sl (mCoreLock);        

            RemotePeer * peer = findRemotePeer(es, sinkId);
            if (peer) {
                aoo_format_storage f;
                if (peer->oursink->get_source_format(e->endpoint, e->id, f) > 0) {
                    DBG("Got source format event from " << es->ipaddr << ":" << es->port << "  " <<  e->id  << "  channels: " << f.header.nchannels);
                    peer->recvMeterSource.resize(f.header.nchannels, meterRmsWindow);

                    // check for layout
                    bool gotuserformat = false;
                    char userfmtdata[1024];
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

                    if (peer->recvChannels != f.header.nchannels) {

                        {
                            const ScopedWriteLock sl (peer->sinkLock);

                            peer->recvChannels = std::min(MAX_PANNERS, f.header.nchannels);

                            // set up this sink with new channel count

                            int sinkchan = std::max(getMainBusNumOutputChannels(), peer->recvChannels);

                            peer->oursink->setup(getSampleRate(), currSamplesPerBlock, sinkchan);
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


                    
                    AudioCodecFormatCodec codec = String(f.header.codec) == AOO_CODEC_OPUS ? CodecOpus : CodecPCM;
                    if (codec == CodecOpus) {
                        aoo_format_opus *fmt = (aoo_format_opus *)&f;
                        peer->recvFormat = AudioCodecFormatInfo(fmt->bitrate/fmt->header.nchannels, fmt->complexity, fmt->signal_type);
                        //peer->recvFormatIndex = findFormatIndex(codec, fmt->bitrate / fmt->header.nchannels, 0);
                    } else {
                        aoo_format_pcm *fmt = (aoo_format_pcm *)&f;
                        int bitdepth = fmt->bitdepth == AOO_PCM_INT16 ? 2 : fmt->bitdepth == AOO_PCM_INT24  ? 3  : fmt->bitdepth == AOO_PCM_FLOAT32 ? 4 : fmt->bitdepth == AOO_PCM_FLOAT64  ? 8 : 2;
                        peer->recvFormat = AudioCodecFormatInfo(bitdepth);
                        //peer->recvFormatIndex = findFormatIndex(codec, 0, fmt->bitdepth);
                    }
                    
                    clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientPeerChangedState, this, "format");
                }                
            }
            else { 
                DBG("format event to unknown " << e->id);
                
            }
            
            break;
        }
        case AOO_SOURCE_STATE_EVENT:
        {
            aoo_source_state_event *e = (aoo_source_state_event *)events[i];
            EndpointState * es = (EndpointState *)e->endpoint;

            DBG("Got source state event from " << es->ipaddr << ":" << es->port << " -- " << e->state);

            const ScopedReadLock sl (mCoreLock);        

            RemotePeer * peer = findRemotePeer(es, sinkId);
            if (peer) {
                peer->recvActive = peer->recvAllow && e->state > 0;
                if (!peer->recvActive && !peer->sendActive) {
                    peer->connected = false;
                } else {
                    peer->connected = true;                    
                }
            }
            
            //clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientPeerChangedState, this, "state");
            
            break;
        }
        case AOO_BLOCK_LOST_EVENT:
        {
            aoo_block_lost_event *e = (aoo_block_lost_event *)events[i];

            EndpointState * es = (EndpointState *)e->endpoint;

            DBG("Got source block lost event from " << es->ipaddr << ":" << es->port << "   " << e->id << " -- " << e->count);

            const ScopedReadLock sl (mCoreLock);                    
            RemotePeer * peer = findRemotePeer(es, sinkId);
            if (peer) {
                peer->dataPacketsDropped += e->count;
                
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
                                peer->oursink->set_buffersize(peer->buffertimeMs);
                                peer->echosink->set_buffersize(peer->buffertimeMs);
                                peer->latencysink->set_buffersize(peer->buffertimeMs);
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
        case AOO_BLOCK_REORDERED_EVENT:
        {
            aoo_block_reordered_event *e = (aoo_block_reordered_event *)events[i];

            EndpointState * es = (EndpointState *)e->endpoint;

            DBG("Got source block reordered event from " << es->ipaddr << ":" << es->port << "  " << e->id << " -- " << e->count);

            break;
        }
        case AOO_BLOCK_RESENT_EVENT:
        {
            aoo_block_resent_event *e = (aoo_block_resent_event *)events[i];
            EndpointState * es = (EndpointState *)e->endpoint;

            DBG("Got source block resent event from " << es->ipaddr << ":" << es->port << "  " << e->id << " -- " << e->count);
            const ScopedReadLock sl (mCoreLock);                    
            RemotePeer * peer = findRemotePeer(es, sinkId);
            if (peer) {
                peer->dataPacketsResent += e->count;
            }

            break;
        }
        case AOO_BLOCK_GAP_EVENT:
        {
            aoo_block_gap_event *e = (aoo_block_gap_event *)events[i];

            EndpointState * es = (EndpointState *)e->endpoint;

            DBG("Got source block gap event from " << es->ipaddr << ":" << es->port << "  " << e->id << " -- " << e->count);

            break;
        }
        case AOO_PING_EVENT:
        {
            aoo_ping_event *e = (aoo_ping_event *)events[i];
            EndpointState * es = (EndpointState *)e->endpoint;


            double diff = aoo_osctime_duration(e->tt1, e->tt2) * 1000.0;
            DBG("Got source block ping event from " << es->ipaddr << ":" << es->port << "  " << e->id << " -- " << diff);


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
                                peer->oursink->set_buffersize(peer->buffertimeMs);
                                peer->echosink->set_buffersize(peer->buffertimeMs);
                                peer->latencysink->set_buffersize(peer->buffertimeMs);
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
            }


            break;
        }
        default:
            break;
        }
    }
    return 1;
}

int32_t SonobusAudioProcessor::handleServerEvents(const aoo_event ** events, int32_t n)
{
    for (int i = 0; i < n; ++i){
        switch (events[i]->type){
            case AOONET_SERVER_USER_JOIN_EVENT:
            {
                aoonet_server_user_event *e = (aoonet_server_user_event *)events[i];
                
                DBG("Server - User joined: " << e->name);
                
                break;
            }
            case AOONET_SERVER_USER_LEAVE_EVENT:
            {
                aoonet_server_user_event *e = (aoonet_server_user_event *)events[i];
                
                DBG("Server - User left: " << e->name);
                
                
                break;
            }
            case AOONET_SERVER_GROUP_JOIN_EVENT:
            {
                aoonet_server_group_event *e = (aoonet_server_group_event *)events[i];
                
                DBG("Server - Group Joined: " << e->group << "  by user: " << e->user);
                
                break;
            }
            case AOONET_SERVER_GROUP_LEAVE_EVENT:
            {
                aoonet_server_group_event *e = (aoonet_server_group_event *)events[i];
                
                DBG("Server - Group Left: " << e->group << "  by user: " << e->user);
                
                break;
            }
            case AOONET_SERVER_ERROR_EVENT:
            {
                aoonet_server_event *e = (aoonet_server_event *)events[i];
                
                DBG("Server error: " << String::fromUTF8(e->errormsg));
                
                break;
            }
            default:
                DBG("Got unknown server event: " << events[i]->type);
                break;
        }
    }
    return 1;
}

int32_t SonobusAudioProcessor::handleClientEvents(const aoo_event ** events, int32_t n)
{
    for (int i = 0; i < n; ++i){
        switch (events[i]->type){
        case AOONET_CLIENT_CONNECT_EVENT:
        {
            aoonet_client_group_event *e = (aoonet_client_group_event *)events[i];

            if (e->result > 0){
                DBG("Connected to server!");
                mIsConnectedToServer = true;
                mSessionConnectionStamp = Time::getMillisecondCounterHiRes();
            } else {
                DBG("Couldn't connect to server - " << String::fromUTF8(e->errormsg));
                mIsConnectedToServer = false;
                mSessionConnectionStamp = 0.0;
            }

            if (mPendingReconnect) {
                
                if (mIsConnectedToServer && mPendingReconnectInfo.groupName.isNotEmpty()) {
                    mPendingReconnectInfo.timestamp = Time::getCurrentTime().toMilliseconds();
                    addRecentServerConnectionInfo(mPendingReconnectInfo);
                    setWatchPublicGroups(false);
                    DBG("Joining group after pending reconnect: " << mPendingReconnectInfo.groupName);
                    joinServerGroup(mPendingReconnectInfo.groupName, mPendingReconnectInfo.groupPassword, mPendingReconnectInfo.groupIsPublic);
                }
                
                mPendingReconnect = false;
            }

            if (mIsConnectedToServer && mReconnectTimer.isTimerRunning()) {
                DBG("Stopping reconnect timer");
                mReconnectTimer.stopTimer();
                mRecoveringFromServerLoss = false;
            }
            
            clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientConnected, this, e->result > 0, String::fromUTF8(e->errormsg));
            
            break;
        }
        case AOONET_CLIENT_DISCONNECT_EVENT:
        {
            aoonet_client_group_event *e = (aoonet_client_group_event *)events[i];
            if (e->result == 0){
                DBG("Disconnected from server - " << String::fromUTF8(e->errormsg));
                
                if (mCurrentJoinedGroup.isNotEmpty() && mReconnectAfterServerLoss.get() && !mReconnectTimer.isTimerRunning()) {
                    DBG("Starting reconnect timer");
                    mRecoveringFromServerLoss = true;
                    mReconnectTimer.startTimer(1000);
                }
            }

            mPendingReconnect = false;

            // don't remove all peers?
            //removeAllRemotePeers();
            
            mIsConnectedToServer = false;
            mSessionConnectionStamp = 0.0;
            
            clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientDisconnected, this, e->result > 0, String::fromUTF8(e->errormsg));

            break;
        }
        case AOONET_CLIENT_GROUP_JOIN_EVENT:
        {
            aoonet_client_group_event *e = (aoonet_client_group_event *)events[i];
            if (e->result > 0){
                DBG("Joined group - " << e->name);
                const ScopedLock sl (mClientLock);        
                mCurrentJoinedGroup = CharPointer_UTF8 (e->name);

                mSessionConnectionStamp = Time::getMillisecondCounterHiRes();


            } else {
                DBG("Couldn't join group " << e->name << " - " << String::fromUTF8(e->errormsg));
            }
            clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientGroupJoined, this, e->result > 0, CharPointer_UTF8 (e->name), String::fromUTF8(e->errormsg));
            break;
        }
        case AOONET_CLIENT_GROUP_LEAVE_EVENT:
        {
            aoonet_client_group_event *e = (aoonet_client_group_event *)events[i];
            if (e->result > 0){

                DBG("Group leave - " << e->name);

                const ScopedLock sl (mClientLock);        
                mCurrentJoinedGroup.clear();

                // assume they are all part of the group, XXX
                removeAllRemotePeers();

                //aoo_node_remove_group(x->x_node, gensym(e->name));
                

            } else {
                DBG("Couldn't leave group " << e->name << " - " << String::fromUTF8(e->errormsg));
            }

            clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientGroupLeft, this, e->result > 0, CharPointer_UTF8 (e->name), String::fromUTF8(e->errormsg));

            break;
        }
        case AOONET_CLIENT_GROUP_PUBLIC_ADD_EVENT:
        {
            aoonet_client_group_event *e = (aoonet_client_group_event *)events[i];
            DBG("Public group add/changed - " << e->name << " count: " << e->result);
            {
                const ScopedLock sl (mPublicGroupsLock);
                String group = CharPointer_UTF8 (e->name);
                AooPublicGroupInfo & ginfo = mPublicGroupInfos[group];
                ginfo.groupName = group;
                ginfo.activeCount = e->result;
                ginfo.timestamp = Time::getCurrentTime().toMilliseconds();
            }

            clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientPublicGroupModified, this, CharPointer_UTF8 (e->name), e->result,  String::fromUTF8(e->errormsg));
            break;
        }
        case AOONET_CLIENT_GROUP_PUBLIC_DEL_EVENT:
        {
            aoonet_client_group_event *e = (aoonet_client_group_event *)events[i];
            DBG("Public group deleted - " << e->name);
            {
                const ScopedLock sl (mPublicGroupsLock);
                String group = CharPointer_UTF8 (e->name);
                mPublicGroupInfos.erase(group);
            }

            clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientPublicGroupDeleted, this, CharPointer_UTF8 (e->name), String::fromUTF8(e->errormsg));
            break;
        }

        case AOONET_CLIENT_PEER_PREJOIN_EVENT:
        {
            aoonet_client_peer_event *e = (aoonet_client_peer_event *)events[i];
            
            if (e->result > 0){
                DBG("Peer attempting to join group " <<  e->group << " - user " << e->user);
                
                clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientPeerPendingJoin, this, CharPointer_UTF8 (e->group), CharPointer_UTF8 (e->user));
                
            } else {
                DBG("bug bad result on join event");
            }
                        
            break;
        }
        case AOONET_CLIENT_PEER_JOIN_EVENT:
        {
            aoonet_client_peer_event *e = (aoonet_client_peer_event *)events[i];

            if (e->result > 0){
                DBG("Peer joined group " <<  e->group << " - user " << e->user);

                EndpointState * endpoint = findOrAddRawEndpoint(e->address);
                if (endpoint) {
                 
                    // check if blocked
                    if (isAddressBlocked(endpoint->ipaddr)) {
                        
                        clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientPeerJoinBlocked, this, CharPointer_UTF8 (e->group), CharPointer_UTF8 (e->user), endpoint->ipaddr, endpoint->port);

                        // after a short delay
                        Timer::callAfterDelay(400, [this, endpoint] {
                            sendBlockedInfoMessage(endpoint, true);
                        });
                    }
                    else {
                        if (mAutoconnectGroupPeers) {
                            connectRemotePeerRaw(e->address, CharPointer_UTF8 (e->user), CharPointer_UTF8 (e->group), !mMainRecvMute.get());
                        }
                        
                        //aoo_node_add_peer(x->x_node, gensym(e->group), gensym(e->user),
                        //                  (const struct sockaddr *)e->address, e->length);
                        
                        clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientPeerJoined, this, CharPointer_UTF8 (e->group), CharPointer_UTF8 (e->user));
                    }
                }
                
            } else {
                DBG("bug bad result on join event");
            }
            

            break;
        }
        case AOONET_CLIENT_PEER_JOINFAIL_EVENT:
        {
            aoonet_client_peer_event *e = (aoonet_client_peer_event *)events[i];
            
            if (e->result > 0){
                DBG("Peer failed to join group " <<  e->group << " - user " << e->user);
                
                clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientPeerJoinFailed, this, CharPointer_UTF8 (e->group), CharPointer_UTF8 (e->user));
                
            } else {
                DBG("bug bad result on join event");
            }
                        
            break;
        }
        case AOONET_CLIENT_PEER_LEAVE_EVENT:
        {
            aoonet_client_peer_event *e = (aoonet_client_peer_event *)events[i];

            if (e->result > 0){

                DBG("Peer leave group " <<  e->group << " - user " << e->user);

                // Notify listeners BEFORE removing peer so they can capture peer index for OSC cleanup
                clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientPeerLeft, this, CharPointer_UTF8 (e->group), CharPointer_UTF8 (e->user));

                EndpointState * endpoint = findOrAddRawEndpoint(e->address);
                if (endpoint) {
                    
                    removeAllRemotePeersWithEndpoint(endpoint);
                }
                
                //aoo_node_remove_peer(x->x_node, gensym(e->group), gensym(e->user));

            } else {
                DBG("bug bad result on leave event");
            }
            break;
        }
        case AOONET_CLIENT_ERROR_EVENT:
        {
            aoonet_client_event *e = (aoonet_client_event *)events[i];
            DBG("client error: " << String::fromUTF8(e->errormsg));
            clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientError, this, String::fromUTF8(e->errormsg));
            break;
        }
        default:
            DBG("Got unknown client event: " << events[i]->type);
            break;
        }
    }
    return 1;
}


int SonobusAudioProcessor::connectRemotePeerRaw(void * sockaddr, const String & username, const String & groupname, bool reciprocate)
{
    EndpointState * endpoint = findOrAddRawEndpoint(sockaddr);
    
    if (!endpoint) {
        DBG("Error getting endpoint from raw address");
        return 0;
    }
    
    RemotePeer * remote = doAddRemotePeerIfNecessary(endpoint, AOO_ID_NONE, username, groupname); // get new one

    remote->recvAllow = !mMainRecvMute.get();
    
    // special - use 0 
    bool ret = remote->oursink->invite_source(endpoint, 0, endpoint_send) == 1;
    
    if (ret) {
        DBG("Successfully invited remote peer at " << endpoint->ipaddr << ":" << endpoint->port << " - ourId " << remote->ourId);
        remote->connected = true;
        remote->invitedPeer = reciprocate;
        //remote->recvActive = reciprocate;
        if (!mMainSendMute.get()) {
            remote->sendActive = true;
            remote->oursource->start();
            updateRemotePeerUserFormat(-1, remote);
            sendRemotePeerInfoUpdate(-1, remote);
        }
        else {
            remote->sendActive = false;
            remote->oursource->stop();
        }
        sendBlockedInfoMessage(remote->endpoint, false);
        
    } else {
        DBG("Error inviting remote peer at " << endpoint->ipaddr << ":" << endpoint->port << " - ourId " << remote->ourId);
    }

    return ret;    
}

int SonobusAudioProcessor::connectRemotePeer(const String & host, int port, const String & username, const String & groupname, bool reciprocate)
{
    EndpointState * endpoint = findOrAddEndpoint(host, port);

    RemotePeer * remote = doAddRemotePeerIfNecessary(endpoint, AOO_ID_NONE, username, groupname); // get new one

    remote->recvAllow = !mMainRecvMute.get();

    // special - use 0 
    bool ret = remote->oursink->invite_source(endpoint, 0, endpoint_send) == 1;
    
    if (ret) {
        DBG("Successfully invited remote peer at " <<  host << ":" << port << " - ourId " << remote->ourId);
        remote->connected = true;
        remote->invitedPeer = reciprocate;
        //remote->recvActive = reciprocate;
        if (!mMainSendMute.get()) {
            remote->sendActive = true;
            remote->oursource->start();
            updateRemotePeerUserFormat(-1, remote);
        }

        sendRemotePeerInfoUpdate(-1, remote);
        sendBlockedInfoMessage(remote->endpoint, false);
        
    } else {
        DBG("Error inviting remote peer at " << host << ":" << port << " - ourId " << remote->ourId);
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
                ret = remote->oursink->uninvite_all();
                //ret = remote->oursink->uninvite_source(endpoint, remote->remoteSourceId, endpoint_send) == 1;
            }
         
            // if we auto-invited the other end's remote source, remove that as a dest sink
            if (remote->oursource) {
                DBG("removing all remote sink " << remote->remoteSinkId);
                remote->oursource->remove_all();
                //ret |= remote->oursource->remove_sink(endpoint, remote->remoteSinkId) == 1;
            }
            
            remote->connected = false;
            remote->recvActive = false;
            remote->sendActive = false;
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
                ret = remote->oursink->uninvite_all();
            }

            // if we auto-invited the other end's remote source, remove that as a dest sink
            if (remote->oursource && remote->remoteSinkId >= 0) {
                DBG("removing all remote sink " << remote->remoteSinkId);
                remote->oursource->remove_all();
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
        remote->oursink->set_dynamic_resampling(newval ? 1 : 0);
        remote->oursource->set_dynamic_resampling(newval ? 1 : 0);
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

            remote->oursource->setup(getSampleRate(), currSamplesPerBlock, remote->sendChannels);

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
        remote->oursink->set_buffersize(remote->buffertimeMs); // ms
        remote->echosink->set_buffersize(remote->buffertimeMs);
        remote->latencysink->set_buffersize(remote->buffertimeMs);
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
            remote->oursink->invite_source(remote->endpoint,remote->remoteSourceId, endpoint_send);
        } else {
            DBG("uninviting peer " <<  remote->ourId << " source " << remote->remoteSourceId);
            remote->oursink->uninvite_source(remote->endpoint, remote->remoteSourceId, endpoint_send);
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

#if 1
        if (remote->activeLatencyTest && remote->latencyMeasurer) {
            if (remote->latencyMeasurer->state > 1 /*remote->latencyMeasurer->measurementCount */) {

                DBG("Latency calculated: " << remote->latencyMeasurer->latencyMs);

                remote->totalLatency = remote->latencyMeasurer->latencyMs;
                remote->bufferTimeAtRealLatency = remote->buffertimeMs;
                remote->hasRealLatency = true;
                remote->latencyDirty = false;
                remote->totalEstLatency = remote->totalLatency;
            }
            else {
                DBG("Latency not calculated yet...");

            }
        }
#else
        if (remote->activeLatencyTest && remote->latencyProcessor) {
            if (remote->latencyProcessor->resolve() < 0) {
                DBG("Latency Signal below threshold...");

            }
            else 
            {
                const bool excludeBuffer = false;

                double         d, dcapt, dplay, t;
                bool inv = false;
                bool quest = false;
                t = 1000.0 / getSampleRate();
                dcapt = currSamplesPerBlock;
                dplay = currSamplesPerBlock;
                
                if (remote->latencyProcessor->err () > 0.35f) 
                {
                    remote->latencyProcessor->invert ();
                    remote->latencyProcessor->resolve ();
                }

                d = remote->latencyProcessor->del ();

                if (excludeBuffer) {
                    d -= dcapt + dplay;   
                }
                if (remote->latencyProcessor->err () > 0.30f) {
                    //printf ("???  ");   
                    quest = true;
                }
                else if (remote->latencyProcessor->inv ()) {
                    inv = true;
                }

                DBG("Peer " << index << ":  " << d << " frames " << d*t << " ms   - " << (quest ? "???" : (inv ? "Inv" : "")));

                remote->totalLatency = d*t;
                remote->hasRealLatency = true;
                remote->latencyDirty = false;
                estimated = false;
                return remote->totalLatency;
            }
        }
#endif

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

bool SonobusAudioProcessor::isRemotePeerLatencyTestActive(int index)
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return remote->activeLatencyTest;
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


bool SonobusAudioProcessor::startRemotePeerLatencyTest(int index, float durationsec)
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        if (!remote->activeLatencyTest) {
            // invite remote's echosource to send to our latency sink

            remote->latencysink->uninvite_all();
            remote->latencysink->reset();
            remote->latencysource->remove_all();

            remote->latencysink->invite_source(remote->endpoint, remote->remoteSourceId+ECHO_ID_OFFSET, endpoint_send);
            
            // start our latency source sending to remote's echosink
            remote->latencysource->add_sink(remote->endpoint, remote->remoteSinkId+ECHO_ID_OFFSET, endpoint_send);            
            remote->latencysource->start();
            
#if 1
            remote->latencyMeasurer->measurementCount = 10000;
            //remote->latencyMeasurer->initializeWithThreshold(-50.0f);
            remote->latencyMeasurer->overrideThreshold = 0.2f;
            remote->latencyMeasurer->noiseMeasureTime = 0.2f;
            remote->latencyMeasurer->toggle(true);

#endif
            remote->hasRealLatency = false;
            remote->activeLatencyTest = true;
        }
        return true;
    }
    return false;
}

bool SonobusAudioProcessor::stopRemotePeerLatencyTest(int index)
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        if (remote->activeLatencyTest) {
            // uninvite remote's echosource
            remote->latencysink->uninvite_all();
            
            // stop our latency source sending to anyone
            remote->latencysource->remove_all();
            remote->latencysource->stop();

            remote->activeLatencyTest = false;            
        }
        return true;
    }
    return false;          
}



void SonobusAudioProcessor::setRemotePeerSendActive(int index, bool active)
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        remote->sendActive = active;
        if (active) {
            remote->sendAllow = true; // implied
            remote->sendAllowCache = true; // implied
            remote->oursource->start();
        } else {
            remote->oursource->stop();            
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

SonobusAudioProcessor::RemotePeer *  SonobusAudioProcessor::findRemotePeerByEchoId(EndpointState * endpoint, int32_t echoId)
{
    const ScopedReadLock sl (mCoreLock);        

    RemotePeer * retpeer = 0;

    for (auto s : mRemotePeers) {
        if (s->endpoint == endpoint && s->ourId+ECHO_ID_OFFSET == echoId) {
            retpeer = s;
            break;
        }
    }
        
    return retpeer;        
}

SonobusAudioProcessor::RemotePeer *  SonobusAudioProcessor::findRemotePeerByLatencyId(EndpointState * endpoint, int32_t latId)
{
    const ScopedReadLock sl (mCoreLock);        

    RemotePeer * retpeer = 0;

    for (auto s : mRemotePeers) {
        if (s->endpoint == endpoint && s->ourId+LATENCY_ID_OFFSET == latId) {
            retpeer = s;
            break;
        }
    }
        
    return retpeer;        
}



SonobusAudioProcessor::RemotePeer * SonobusAudioProcessor::doAddRemotePeerIfNecessary(EndpointState * endpoint, int32_t ourId, const String & username, const String & groupname)
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
        int32_t newid = 1;
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
        retpeer->groupName = groupname;
        
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
        retpeer->blockedUs = false;
        
        retpeer->oursink->setup(getSampleRate(), currSamplesPerBlock, getMainBusNumOutputChannels());
        retpeer->oursink->set_buffersize(retpeer->buffertimeMs);

        int32_t flags = AOO_PROTOCOL_FLAG_COMPACT_DATA;
        retpeer->oursink->set_option(aoo_opt_protocol_flags, &flags, sizeof(int32_t));

        retpeer->nominalSendChannels = mSendChannels.get();
        retpeer->sendChannels =  mSendChannels.get() <= 0 ?  mActiveSendChannels : mSendChannels.get();

        setupSourceFormat(retpeer, retpeer->oursource.get());
        float sendbufsize = jmax(10.0, SENDBUFSIZE_SCALAR * 1000.0f * currSamplesPerBlock / getSampleRate());
        retpeer->oursource->setup(getSampleRate(), currSamplesPerBlock, retpeer->sendChannels);
        retpeer->oursource->set_buffersize(sendbufsize);
        retpeer->oursource->set_packetsize(retpeer->packetsize);        
        //setupSourceUserFormat(retpeer, retpeer->oursource.get());

        setupSourceFormat(retpeer, retpeer->latencysource.get(), true);
        retpeer->latencysource->setup(getSampleRate(), currSamplesPerBlock, 1);
        retpeer->latencysource->set_packetsize(retpeer->packetsize);
        setupSourceFormat(retpeer, retpeer->echosource.get(), true);
        retpeer->echosource->setup(getSampleRate(), currSamplesPerBlock, 1);
        retpeer->echosource->set_buffersize(1000.0f * currSamplesPerBlock / getSampleRate());
        retpeer->echosource->set_packetsize(retpeer->packetsize);

        retpeer->latencysink->setup(getSampleRate(), currSamplesPerBlock, 1);
        retpeer->echosink->setup(getSampleRate(), currSamplesPerBlock, 1);

        retpeer->latencysink->set_option(aoo_opt_protocol_flags, &flags, sizeof(int32_t));
        retpeer->echosink->set_option(aoo_opt_protocol_flags, &flags, sizeof(int32_t));

        retpeer->latencysink->set_buffersize(retpeer->buffertimeMs);
        retpeer->echosink->set_buffersize(retpeer->buffertimeMs);

        // never dynamic resampling the latency and echo ones
        retpeer->latencysink->set_dynamic_resampling(0);
        retpeer->echosink->set_dynamic_resampling(0);
        retpeer->latencysource->set_dynamic_resampling(0);
        retpeer->echosource->set_dynamic_resampling(0);

        
        retpeer->oursource->set_ping_interval(2000);
        retpeer->latencysource->set_ping_interval(2000);
        retpeer->echosource->set_ping_interval(2000);

        retpeer->oursource->set_respect_codec_change_requests(1);
        retpeer->latencysource->set_respect_codec_change_requests(1);
        retpeer->echosource->set_respect_codec_change_requests(1);
        
        //retpeer->latencyProcessor.reset(new MTDM(getSampleRate()));
        retpeer->latencyMeasurer.reset(new LatencyMeasurer());
        
        retpeer->oursink->set_dynamic_resampling(mDynamicResampling.get() ? 1 : 0);
        retpeer->oursource->set_dynamic_resampling(mDynamicResampling.get() ? 1 : 0);

        
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
            mRemotePeers.add(retpeer);
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
                setupSourceFormat(retpeer, retpeer->latencysource.get(), true);
                setupSourceFormat(retpeer, retpeer->echosource.get(), true);

                retpeer->oursink->set_buffersize(retpeer->buffertimeMs);
                retpeer->latencysink->set_buffersize(retpeer->buffertimeMs);
                retpeer->echosink->set_buffersize(retpeer->buffertimeMs);
                
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

bool SonobusAudioProcessor::formatInfoToAooFormat(const AudioCodecFormatInfo & info, int channels, aoo_format_storage & retformat) {
                
        if (info.codec == CodecPCM) {
            aoo_format_pcm *fmt = (aoo_format_pcm *)&retformat;
            fmt->header.codec = AOO_CODEC_PCM;
            fmt->header.blocksize = currSamplesPerBlock >= info.min_preferred_blocksize ? currSamplesPerBlock : info.min_preferred_blocksize;
            fmt->header.samplerate = getSampleRate();
            fmt->header.nchannels = channels;
            fmt->bitdepth = info.bitdepth == 2 ? AOO_PCM_INT16 : info.bitdepth == 3 ? AOO_PCM_INT24 : info.bitdepth == 4 ? AOO_PCM_FLOAT32 : info.bitdepth == 8 ? AOO_PCM_FLOAT64 : AOO_PCM_INT16;

            return true;
        } 
        else if (info.codec == CodecOpus) {
            aoo_format_opus *fmt = (aoo_format_opus *)&retformat;
            fmt->header.codec = AOO_CODEC_OPUS;
            fmt->header.blocksize = currSamplesPerBlock >= info.min_preferred_blocksize ? currSamplesPerBlock : info.min_preferred_blocksize;
            fmt->header.samplerate = getSampleRate();
            fmt->header.nchannels = channels;
            fmt->bitrate = info.bitrate * fmt->header.nchannels;
            fmt->complexity = info.complexity;
            fmt->signal_type = info.signal_type;
            fmt->application_type = OPUS_APPLICATION_RESTRICTED_LOWDELAY;
            //fmt->application_type = OPUS_APPLICATION_AUDIO;
            
            return true;
        }
    
    return false;
}
    
    
void SonobusAudioProcessor::setupSourceFormat(SonobusAudioProcessor::RemotePeer * peer, aoo::isource * source, bool latencymode)
{
    // have choice and parameters
    int formatIndex = (!peer || peer->formatIndex < 0) ? mDefaultAudioFormatIndex : peer->formatIndex;
    if (formatIndex < 0 || formatIndex >= mAudioFormats.size()) formatIndex = 4; //emergency default
    const AudioCodecFormatInfo & info =  mAudioFormats.getReference(formatIndex);
    
    aoo_format_storage f;
    int channels = latencymode ? 1  :  peer ? peer->sendChannels : getMainBusNumInputChannels();
    
    if (formatInfoToAooFormat(info, channels, f)) {        
        source->set_format(f.header);        
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


void SonobusAudioProcessor::setupSourceUserFormat(RemotePeer * peer, aoo::isource * source)
{
    // get userformat from send info
    ValueTree fmttree = getSendUserFormatLayoutTree();

    MemoryBlock destData;
    MemoryOutputStream stream(destData, false);

    DBG("setup source user format data: " << fmttree.toXmlString());

    fmttree.writeToStream(stream);

    source->set_userformat(destData.getData(), (int32_t) destData.getSize());
}

void SonobusAudioProcessor::updateRemotePeerUserFormat(int index, RemotePeer * onlypeer)
{
    // get userformat from send info
    ValueTree fmttree = getSendUserFormatLayoutTree();

    MemoryBlock destData;
    MemoryOutputStream stream(destData, false);

    DBG("format data: " << fmttree.toXmlString());

    fmttree.writeToStream(stream);

    char buf[AOO_MAXPACKETSIZE];


    if (destData.getSize() > AOO_MAXPACKETSIZE - 100) {
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

        DBG("Sending channellayout message to " << i);
        this->sendPeerMessage(peer, msg.Data(), (int32_t) msg.Size());

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
    else if (parameterID == paramMaxRecvPaddingMs) {
        mMaxRecvPaddingMs = newValue;
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
    setupSourceFormat(0, mAooDummySource.get());
    mAooDummySource->setup(sampleRate, samplesPerBlock, getTotalNumInputChannels());



    if (lastInputChannels == 0 || lastOutputChannels == 0 || mInputChannelGroupCount == 0) {
        // first time these are set, do some initialization

        lastInputChannels = inchannels;
        lastOutputChannels = outchannels;

        if (mInputChannelGroupCount == 0) {
            mInputChannelGroupCount = 1;
            mInputChannelGroups[0].params.chanStartIndex = 0;
            mInputChannelGroups[0].params.numChannels = jmax(1, getMainBusNumInputChannels()); // default to only as many channels as the main input bus has
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

            s->oursource->setup(sampleRate, currSamplesPerBlock, s->sendChannels);  // todo use inchannels maybe?
            float sendbufsize = jmax(10.0, SENDBUFSIZE_SCALAR * 1000.0f * currSamplesPerBlock / getSampleRate());
            s->oursource->set_buffersize(sendbufsize);

        }
        if (s->oursink) {
            const ScopedWriteLock sl (s->sinkLock);
            int sinkchan = jmax(outchannels, s->recvChannels);
            s->oursink->setup(sampleRate, currSamplesPerBlock, sinkchan);
        }

        if (s->latencysource) {
            setupSourceFormat(s, s->latencysource.get(), true);
            s->latencysource->setup(getSampleRate(), currSamplesPerBlock, 1);
            setupSourceFormat(s, s->echosource.get(), true);
            s->echosource->setup(getSampleRate(), currSamplesPerBlock, 1);
            float sendbufsize = jmax(10.0, SENDBUFSIZE_SCALAR * 1000.0f * currSamplesPerBlock / getSampleRate());
            s->echosource->set_buffersize(sendbufsize);

            s->netBufAutoBaseline = (1e3*currSamplesPerBlock/getSampleRate()); // at least a process block

            {
                const ScopedWriteLock sl (s->sinkLock);

                s->latencysink->setup(sampleRate, currSamplesPerBlock, 1);
                s->echosink->setup(sampleRate, currSamplesPerBlock, 1);
            }

            //s->latencyProcessor.reset(new MTDM(sampleRate));
            //s->latencyMeasurer.reset(new LatencyMeasurer());
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


    uint64_t t = aoo_osctime_get();

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
    // if sending as mono, split the difference about applying gain attenuation for the number of input channels
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
        
        //mAooSource->process( buffer.getArrayOfReadPointers(), numSamples, t);
        
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
            float retratio = 0.0f;
            if (remote->oursink->get_sourceoption(remote->endpoint, remote->remoteSourceId, aoo_opt_buffer_fill_ratio, &retratio, sizeof(retratio)) > 0) {
                remote->fillRatio.Z *= 0.95;
                remote->fillRatio.push(retratio);
                remote->fillRatioSlow.Z *= 0.99;
                remote->fillRatioSlow.push(retratio);
            }

            
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

                remote->oursink->process((float **)remote->workBuffer.getArrayOfWritePointers(), numSamples, t);
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
                
                
                remote->oursource->process((const float **)workBuffer.getArrayOfReadPointers(), numSamples, t);
                
                //remote->sendMeterSource.measureBlock (workBuffer);
                
                
                // now process echo and latency stuff
                
                workBuffer.clear(0, 0, numSamples);
                if (remote->echosink->process((float **)workBuffer.getArrayOfWritePointers(), numSamples, t)) {
                    //DBG("received something from our ECHO sink");
                    remote->echosource->process((const float **)workBuffer.getArrayOfReadPointers(), numSamples, t);
                }

                
                if (remote->activeLatencyTest && remote->latencyMeasurer) {
                    workBuffer.clear(0, 0, numSamples);
                    if (remote->latencysink->process((float **)workBuffer.getArrayOfWritePointers(), numSamples, t)) {
                        //DBG("received something from our latency sink");
                    }

                    // hear latency measure stuff (recv into right channel)
                    if (hearlatencytest) {
                        tempBuffer.addFrom(mainBusOutputChannels > 1 ? 1 : 0, 0, workBuffer, 0, 0, numSamples);
                    }

#if 1
                    remote->latencyMeasurer->processInput(workBuffer.getWritePointer(0), (int)lrint(getSampleRate()), numSamples);
                    remote->latencyMeasurer->processOutput(workBuffer.getWritePointer(0));
#else
                    remote->latencyProcessor->process(numSamples, workBuffer.getWritePointer(0), workBuffer.getWritePointer(0));
#endif

                    // hear latency measure stuff (send into left channel)
                    if (hearlatencytest) {
                        tempBuffer.addFrom(0, 0, workBuffer, 0, 0, numSamples);
                    }

                    
                    remote->latencysource->process((const float **)workBuffer.getArrayOfReadPointers(), numSamples, t);
                }
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

        if (inReverbEnabled != mLastInputReverbEnabled && inReverbEnabled) {
            mInputReverb.reset();
        }

        mInputReverb.process((float **)inputRevBuffer.getArrayOfWritePointers(), (float **)inputRevBuffer.getArrayOfWritePointers(), numSamples);

        if (inReverbEnabled != mLastInputReverbEnabled ) {
            float sgain = inReverbEnabled ? 0.0f : 1.0f;
            float egain = inReverbEnabled ? 1.0f : 0.0f;

            inputRevBuffer.applyGainRamp(0, numSamples, sgain, egain);
        }

        // mix it into send workbuffer for each channel up to 4
        for (int channel = 0; channel < sendPanChannels && channel < 4; ++channel) {
              sendWorkBuffer.addFrom(channel, 0, inputRevBuffer, channel, 0, numSamples);
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
                        const bool silenceIns = mRecordInputSilenceWhenMuted && (inGain == 0.0f || mInputChannelGroups[i].params.muted);
                        if (activeSelfWriters[i].load() != nullptr) {
                            // we need to make sure the writer has at least all the inputs it expects
                            const float * useinbufs[MAX_PANNERS];
                            for (int j=0; j < mSelfRecordChans[i] && j < MAX_PANNERS; ++j) {
                                useinbufs[j] = (j < chcnt && !silenceIns) ? inbufs[chindex+j] : silentBuffer.getReadPointer(0);
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

OSCManager& SonobusAudioProcessor::getOSCManager()
{
    return oscManager;
}

void SonobusAudioProcessor::setOSCEnabled(bool enabled)
{
    mOSCEnabled = enabled;
    
    if (enabled) {
        // Initialize OSC when enabled
        oscManager.initializeReceiver(mOSCReceivePort);
        oscManager.initializeSender(mOSCTargetIPAddress, mOSCTargetPort);
        
        // Register OSC controls in the editor
        if (auto* editor = dynamic_cast<SonobusAudioProcessorEditor*>(getActiveEditor())) {
            editor->registerAllOSCControls();
        }
    } else {
        // Unregister OSC controls in the editor
        if (auto* editor = dynamic_cast<SonobusAudioProcessorEditor*>(getActiveEditor())) {
            editor->unregisterAllOSCControls();
        }
        
        // Disconnect OSC when disabled
        oscManager.disconnectReceiver();
        oscManager.disconnectSender();
    }
}

void SonobusAudioProcessor::setOSCTargetIPAddress(const String& ipAddress)
{
    mOSCTargetIPAddress = ipAddress;
    if (mOSCEnabled) {
        oscManager.initializeSender(mOSCTargetIPAddress, mOSCTargetPort);
    }
}

void SonobusAudioProcessor::setOSCTargetPort(int port)
{
    mOSCTargetPort = port;
    if (mOSCEnabled) {
        oscManager.initializeSender(mOSCTargetIPAddress, mOSCTargetPort);
    }
}

void SonobusAudioProcessor::setOSCReceivePort(int port)
{
    mOSCReceivePort = port;
    if (mOSCEnabled) {
        oscManager.initializeReceiver(mOSCReceivePort);
    }
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
static String videoLinkLargeShareKey("largeShare");
static String videoLinkPushViewModeKey("pushViewMode");
static String videoLinkScreenShareParamsKey("screenShare");

ValueTree SonobusAudioProcessor::VideoLinkInfo::getValueTree() const
{
    ValueTree item(videoLinkInfoKey);
    
    item.setProperty(videoLinkRoomModeKey, roomMode, nullptr);
    item.setProperty(videoLinkShowNamesKey, showNames, nullptr);
    item.setProperty(videoLinkScreenShareParamsKey, screenShareMode, nullptr);
    item.setProperty(videoLinkExtraParamsKey, extraParams, nullptr);
    item.setProperty(videoLinkBeDirectorKey, beDirector, nullptr);
    item.setProperty(videoLinkLargeShareKey, largeShare, nullptr);
    item.setProperty(videoLinkPushViewModeKey, pushViewMode, nullptr);

    return item;
}

void SonobusAudioProcessor::VideoLinkInfo::setFromValueTree(const ValueTree & item)
{
    roomMode = item.getProperty(videoLinkRoomModeKey, roomMode);
    showNames = item.getProperty(videoLinkShowNamesKey, showNames);
    screenShareMode = item.getProperty(videoLinkScreenShareParamsKey, screenShareMode);
    extraParams = item.getProperty(videoLinkExtraParamsKey, extraParams);
    beDirector = item.getProperty(videoLinkBeDirectorKey, beDirector);
    largeShare = item.getProperty(videoLinkLargeShareKey, largeShare);
    pushViewMode = item.getProperty(videoLinkPushViewModeKey, pushViewMode);
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
    extraTree.setProperty(recordSelfSilenceMutedKey, mRecordInputSilenceWhenMuted, nullptr);
    extraTree.setProperty(recordFinishOpenKey, mRecordFinishOpens, nullptr);
    extraTree.setProperty(recordStealthKey, mRecordStealth, nullptr);

    if (mDefaultRecordDir.isLocalFile()) {
        // backwards compat
        extraTree.setProperty(defRecordDirKey, mDefaultRecordDir.getLocalFile().getFullPathName(), nullptr);
    }
    extraTree.setProperty(defRecordDirURLKey, mDefaultRecordDir.toString(false), nullptr);

    // OSC Configuration
    extraTree.setProperty(oscEnabledKey, mOSCEnabled, nullptr);
    extraTree.setProperty(oscSendStateOnStartKey, mOSCSendStateOnStart, nullptr);
    extraTree.setProperty(oscTargetIPAddressKey, mOSCTargetIPAddress, nullptr);
    extraTree.setProperty(oscTargetPortKey, mOSCTargetPort, nullptr);
    extraTree.setProperty(oscReceivePortKey, mOSCReceivePort, nullptr);

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
    extraTree.setProperty(useUnivFontKey, mUseUniversalFont, nullptr);
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

            bool silmute = extraTree.getProperty(recordSelfSilenceMutedKey, mRecordInputSilenceWhenMuted);
            setSelfRecordingSilenceWhenMuted(silmute);


            setRecordFinishOpens(extraTree.getProperty(recordFinishOpenKey, mRecordFinishOpens));
            setRecordStealth(extraTree.getProperty(recordStealthKey, mRecordStealth));
            
            // OSC Configuration
            mOSCEnabled = extraTree.getProperty(oscEnabledKey, mOSCEnabled);
            mOSCSendStateOnStart = extraTree.getProperty(oscSendStateOnStartKey, mOSCSendStateOnStart);
            mOSCTargetIPAddress = extraTree.getProperty(oscTargetIPAddressKey, mOSCTargetIPAddress);
            mOSCTargetPort = extraTree.getProperty(oscTargetPortKey, mOSCTargetPort);
            mOSCReceivePort = extraTree.getProperty(oscReceivePortKey, mOSCReceivePort);
            
            // Reinitialize OSC with loaded settings if enabled
            if (mOSCEnabled) {
                oscManager.initializeSender(mOSCTargetIPAddress, mOSCTargetPort);
                oscManager.initializeReceiver(mOSCReceivePort);
            }


#if !(JUCE_IOS)
            String urlstr = extraTree.getProperty(defRecordDirURLKey, "");
            if (urlstr.isNotEmpty()) {
                // doublecheck it's a valid URL due to bad update
                auto url = URL(urlstr);
                if (url.getScheme().isEmpty()) {
                    // assume it's a file
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
            mUseUniversalFont = extraTree.getProperty(useUnivFontKey, mUseUniversalFont);
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
            
            URL returl;
            
            if (auto fileStream = makeStream(recdir, filename, returl))
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
                
                URL returl;

                if (auto fileStream = makeStream(recdir, filename, returl))
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
            
            URL returl;

            if (auto fileStream = makeStream(recdir, filename, returl))
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
                if (numchan == 0) {
                    // assume there will be something eventually
                    numchan = 2;
                }
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

                URL returl;

                if (auto fileStream = makeStream(recdir, userfilename, returl))
                {
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
    userwriters.ensureStorageAllocated(mRemotePeers.size());

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



//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SonobusAudioProcessor();
}
