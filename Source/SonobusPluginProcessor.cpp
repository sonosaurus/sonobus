// SPDX-License-Identifier: GPLv3-or-later
// Copyright (C) 2020 Jesse Chappell



#include "SonobusPluginProcessor.h"
#include "SonobusPluginEditor.h"

#include "RunCumulantor.h"


#include "aoo/aoo_net.h"
#include "aoo/aoo_pcm.h"
#include "aoo/aoo_opus.h"

#include "mtdm.h"

#include <algorithm>

#include "LatencyMeasurer.h"
#include "Metronome.h"

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
String SonobusAudioProcessor::paramHearLatencyTest   ("hearlatencytest");
String SonobusAudioProcessor::paramMetIsRecorded   ("metisrecorded");
String SonobusAudioProcessor::paramMainReverbEnabled  ("mainreverbenabled");
String SonobusAudioProcessor::paramMainReverbLevel  ("mainreverblevel");
String SonobusAudioProcessor::paramMainReverbSize  ("mainreverbsize");
String SonobusAudioProcessor::paramMainReverbDamping  ("mainreverbdamp");
String SonobusAudioProcessor::paramMainReverbPreDelay  ("mainreverbpredelay");
String SonobusAudioProcessor::paramMainReverbModel  ("mainreverbmodel");
String SonobusAudioProcessor::paramDynamicResampling  ("dynamicresampling");

static String recentsCollectionKey("RecentConnections");
static String recentsItemKey("ServerConnectionInfo");

static String extraStateCollectionKey("ExtraState");
static String useSpecificUdpPortKey("UseUdpPort");
static String changeQualForAllKey("ChangeQualForAll");

static String compressorStateKey("CompressorState");
static String expanderStateKey("ExpanderState");
static String limiterStateKey("LimiterState");
static String compressorEnabledKey("enabled");
static String compressorThresholdKey("threshold");
static String compressorRatioKey("ratio");
static String compressorAttackKey("attack");
static String compressorReleaseKey("release");
static String compressorMakeupGainKey("makeupgain");
static String compressorAutoMakeupGainKey("automakeup");

static String eqStateKey("ParametricEqState");
static String eqEnabledKey("enabled");
static String eqLowShelfGainKey("lsgain");
static String eqLowShelfFreqKey("lsfreq");
static String eqHighShelfGainKey("hsgain");
static String eqHighShelfFreqKey("hsfreq");
static String eqPara1GainKey("p1gain");
static String eqPara1FreqKey("p1freq");
static String eqPara1QKey("p1q");
static String eqPara2GainKey("p2gain");
static String eqPara2FreqKey("p2freq");
static String eqPara2QKey("p1q");


static String inputEffectsStateKey("InputEffects");

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



#define METER_RMS_SEC 0.04
#define MAX_PANNERS 4

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

struct SonobusAudioProcessor::RemotePeer {
    RemotePeer(EndpointState * ep = 0, int id_=0, aoo::isink::pointer oursink_ = 0, aoo::isource::pointer oursource_ = 0) : endpoint(ep), 
        ourId(id_), 
        oursink(std::move(oursink_)), oursource(std::move(oursource_)) {
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
    float buffertimeMs = 15.0f;
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
    int sendChannels = 2; // actual current send channel count
    int nominalSendChannels = 0; // 0 matches input
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
    int64_t lastDropCount = 0;
    double lastNetBufDecrTime = 0;
    float netBufAutoBaseline = 0.0f;
    float pingTime = 0.0f; // ms
    stats::RunCumulantor1D  smoothPingTime; // ms
    stats::RunCumulantor1D  fillRatio;
    stats::RunCumulantor1D  fillRatioSlow;
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
    // compressor
    CompressorParams compressorParams;
    std::unique_ptr<faustCompressor> compressor;
    std::unique_ptr<MapUI> compressorUI;
    float * compressorOutputLevel = nullptr;
    bool compressorParamsChanged = false;
    bool lastCompressorEnabled = false;
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
        
        while (!threadShouldExit()) {
         
            if (_processor.mSendWaitable.wait(10)) {
                _processor.doSendData();
            }            
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
         
            sleep(20);
            
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


//==============================================================================
SonobusAudioProcessor::SonobusAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
mState (*this, &mUndoManager, "SonoBusAoO",
{
    std::make_unique<AudioParameterFloat>(paramInGain,     TRANS ("In Gain"),    NormalisableRange<float>(0.0, 4.0, 0.0, 0.25), mInGain.get(), "", AudioProcessorParameter::genericParameter, 
                                          [](float v, int maxlen) -> String { return Decibels::toString(Decibels::gainToDecibels(v), 1); }, 
                                          [](const String& s) -> float { return Decibels::decibelsToGain(s.getFloatValue()); }),

    std::make_unique<AudioParameterFloat>(paramInMonitorMonoPan,     TRANS ("In Pan"),    NormalisableRange<float>(-1.0, 1.0, 0.0), mInMonMonoPan.get(), "", AudioProcessorParameter::genericParameter, 
                                          [](float v, int maxlen) -> String { if (fabs(v) < 0.01) return TRANS("C"); return String((int)rint(abs(v*100.0f))) + ((v > 0 ? "% R" : "% L")) ; },
                                          [](const String& s) -> float { return s.getFloatValue()*1e-2f; }),

    std::make_unique<AudioParameterFloat>(paramInMonitorPan1,     TRANS ("In Pan 1"),    NormalisableRange<float>(-1.0, 1.0, 0.0), mInMonPan1.get(), "", AudioProcessorParameter::genericParameter, 
                                          [](float v, int maxlen) -> String { if (fabs(v) < 0.01) return TRANS("C"); return String((int)rint(abs(v*100.0f))) + ((v > 0 ? "% R" : "% L")) ; },
                                          [](const String& s) -> float { return s.getFloatValue()*1e-2f; }),

    std::make_unique<AudioParameterFloat>(paramInMonitorPan2,     TRANS ("In Pan 2"),    NormalisableRange<float>(-1.0, 1.0, 0.0), mInMonPan2.get(), "", AudioProcessorParameter::genericParameter, 
                                          [](float v, int maxlen) -> String { if (fabs(v) < 0.01) return TRANS("C"); return String((int)rint(abs(v*100.0f))) + ((v > 0 ? "% R" : "% L")) ; },
                                          [](const String& s) -> float { return s.getFloatValue()*1e-2f; }),

    std::make_unique<AudioParameterFloat>(paramDry,     TRANS ("Dry Level"),    NormalisableRange<float>(0.0,    1.0, 0.0, 0.25), mDry.get(), "", AudioProcessorParameter::genericParameter, 
                                          [](float v, int maxlen) -> String { return Decibels::toString(Decibels::gainToDecibels(v), 1); }, 
                                          [](const String& s) -> float { return Decibels::decibelsToGain(s.getFloatValue()); }),

    std::make_unique<AudioParameterFloat>(paramWet,     TRANS ("Output Level"),    NormalisableRange<float>(0.0, 2.0, 0.0, 0.25), mWet.get(), "", AudioProcessorParameter::genericParameter,
                                          [](float v, int maxlen) -> String { return Decibels::toString(Decibels::gainToDecibels(v), 1); }, 
                                          [](const String& s) -> float { return Decibels::decibelsToGain(s.getFloatValue()); }),

    std::make_unique<AudioParameterFloat>(paramDefaultNetbufMs,     TRANS ("Default Net Buffer Time"),    NormalisableRange<float>(0.0, mMaxBufferTime.get(), 0.001, 0.5), mBufferTime.get(), "", AudioProcessorParameter::genericParameter,
                                          [](float v, int maxlen) -> String { return String(v*1000.0) + " ms"; }, 
                                          [](const String& s) -> float { return s.getFloatValue()*1e-3f; }),
    std::make_unique<AudioParameterChoice>(paramSendChannels, TRANS ("Send Channels"), StringArray({ "Match # Inputs", "Send Mono", "Send Stereo"}), 0),

    std::make_unique<AudioParameterBool>(paramMetEnabled, TRANS ("Metronome Enabled"), mMetEnabled.get()),
    std::make_unique<AudioParameterBool>(paramSendMetAudio, TRANS ("Send Metronome Audio"), mSendMet.get()),
    std::make_unique<AudioParameterFloat>(paramMetGain,     TRANS ("Metronome Gain"),    NormalisableRange<float>(0.0, 1.0, 0.0, 0.25), mMetGain.get(), "", AudioProcessorParameter::genericParameter,
                                          [](float v, int maxlen) -> String { return Decibels::toString(Decibels::gainToDecibels(v), 1); },
                                          [](const String& s) -> float { return Decibels::decibelsToGain(s.getFloatValue()); }),

    std::make_unique<AudioParameterFloat>(paramMetTempo,     TRANS ("Metronome Tempo"),    NormalisableRange<float>(10.0, 400.0, 1, 0.5), mMetTempo.get(), "", AudioProcessorParameter::genericParameter,
                                          [](float v, int maxlen) -> String { return String(v) + " bpm"; },
                                          [](const String& s) -> float { return s.getFloatValue(); }),

    std::make_unique<AudioParameterBool>(paramSendFileAudio, TRANS ("Send Playback Audio"), mSendPlaybackAudio.get()),
    std::make_unique<AudioParameterBool>(paramHearLatencyTest, TRANS ("Hear Latency Test"), mHearLatencyTest.get()),
    std::make_unique<AudioParameterBool>(paramMetIsRecorded, TRANS ("Record Metronome to File"), mMetIsRecorded.get()),
    std::make_unique<AudioParameterBool>(paramMainReverbEnabled, TRANS ("Main Reverb Enabled"), mMainReverbEnabled.get()),
    std::make_unique<AudioParameterFloat>(paramMainReverbLevel,     TRANS ("Main Reverb Level"),    NormalisableRange<float>(0.0,    1.0, 0.0, 0.4), mMainReverbLevel.get(), "", AudioProcessorParameter::genericParameter, 
                                          [](float v, int maxlen) -> String { return Decibels::toString(Decibels::gainToDecibels(v), 1); }, 
                                          [](const String& s) -> float { return Decibels::decibelsToGain(s.getFloatValue()); }),
    std::make_unique<AudioParameterFloat>(paramMainReverbSize,     TRANS ("Main Reverb Size"),    NormalisableRange<float>(0.0, 1.0, 0.0), mMainReverbSize.get(), "", AudioProcessorParameter::genericParameter,
                                          [](float v, int maxlen) -> String { return String((int)(v*100)) + " %"; },
                                          [](const String& s) -> float { return s.getFloatValue()*0.01f; }),
    std::make_unique<AudioParameterFloat>(paramMainReverbDamping,     TRANS ("Main Reverb Damping"),    NormalisableRange<float>(0.0, 1.0, 0.0), mMainReverbDamping.get(), "", AudioProcessorParameter::genericParameter,
                                          [](float v, int maxlen) -> String { return String((int)(v*100)) + " %"; },
                                          [](const String& s) -> float { return s.getFloatValue()*0.01f; }),
    std::make_unique<AudioParameterFloat>(paramMainReverbPreDelay,     TRANS ("Pre-Delay Time"),    NormalisableRange<float>(0.0, 100.0, 1.0, 1.0), mMainReverbPreDelay.get(), "", AudioProcessorParameter::genericParameter,
                                          [](float v, int maxlen) -> String { return String(v, 0) + " ms"; }, 
                                          [](const String& s) -> float { return s.getFloatValue(); }),

    std::make_unique<AudioParameterChoice>(paramMainReverbModel, TRANS ("Main Reverb Model"), StringArray({ "Freeverb", "MVerb", "Zita"}), mMainReverbModel.get()),

    std::make_unique<AudioParameterBool>(paramMainSendMute, TRANS ("Main Send Mute"), mMainSendMute.get()),
    std::make_unique<AudioParameterBool>(paramMainRecvMute, TRANS ("Main Receive Mute"), mMainRecvMute.get()),
    std::make_unique<AudioParameterBool>(paramMainInMute, TRANS ("Main In Mute"), mMainInMute.get()),
    std::make_unique<AudioParameterBool>(paramMainMonitorSolo, TRANS ("Main Monitor Solo"), mMainMonitorSolo.get()),

    std::make_unique<AudioParameterChoice>(paramDefaultAutoNetbuf, TRANS ("Def Auto Net Buffer Mode"), StringArray({ "Off", "Auto-Increase", "Auto-Full"}), defaultAutoNetbufMode),

    std::make_unique<AudioParameterInt>(paramDefaultSendQual, TRANS ("Def Send Format"), 0, 14, mDefaultAudioFormatIndex),
    std::make_unique<AudioParameterBool>(paramDynamicResampling, TRANS ("Dynamic Resampling"), mDynamicResampling.get()),
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
    mState.addParameterListener (paramMainInMute, this);
    mState.addParameterListener (paramMainMonitorSolo, this);

    for (int i=0; i < MAX_PEERS; ++i) {
        for (int j=0; j < MAX_PEERS; ++j) {
            mRemoteSendMatrix[i][j] = false;
        }
    }
    
    initFormats();
    
    mDefaultAutoNetbufModeParam = mState.getParameter(paramDefaultAutoNetbuf);
    mDefaultAudioFormatParam = mState.getParameter(paramDefaultSendQual);

    if (!JUCEApplicationBase::isStandaloneApp()) {
        // default dry to 1.0 if plugin
        mDry = 1.0;
    } else {
        mDry = 0.0;
    }

    mState.getParameter(paramDry)->setValue(mDry.get());

    
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
    
    
    // default expander params
    mInputExpanderParams.thresholdDb = -60.0f;
    mInputExpanderParams.releaseMs = 200.0f;
    mInputExpanderParams.attackMs = 1.0f;
    mInputExpanderParams.ratio = 2.0f;
    
    if (mInputCompressorParams.automakeupGain) {
        // makeupgain = (-thresh -  abs(thresh/ratio))/2
        mInputCompressorParams.makeupGainDb = (-mInputCompressorParams.thresholdDb - abs(mInputCompressorParams.thresholdDb/mInputCompressorParams.ratio)) * 0.5f;
    }

    mInputLimiterParams.enabled = true; // XXX
    mInputLimiterParams.thresholdDb = -1.0f;
    mInputLimiterParams.ratio = 4.0f;
    mInputLimiterParams.attackMs = 0.01;
    mInputLimiterParams.releaseMs = 100.0;
    
    mTransportSource.addChangeListener(this);
    
    // audio setup
    mFormatManager.registerBasicFormats();    
    
    initializeAoo();
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
    auto addresses = IPAddress::getAllInterfaceAddresses (false);
    for (auto& a : addresses) {
        if (a.first != IPAddress::local(false)) {
            mLocalIPAddress = a.first;
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
    mSendThread->setPriority(8);
    mRecvThread = std::make_unique<RecvThread>(*this);
    mRecvThread->setPriority(7);
    mEventThread = std::make_unique<EventThread>(*this);

    if (mAooClient) {
        mClientThread = std::make_unique<ClientThread>(*this);
    }
    
    mSendThread->startThread();
    mRecvThread->startThread();
    mEventThread->startThread();

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

bool SonobusAudioProcessor::connectToServer(const String & host, int port, const String & username, const String & passwd)
{
    if (!mAooClient) return false;
    
    // disconnect from everything else!
    removeAllRemotePeers();
    
    mServerEndpoint->ipaddr = host;
    mServerEndpoint->port = port;
    mServerEndpoint->peer.reset();
    
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

    const ScopedLock sl (mClientLock);        
    
    mIsConnectedToServer = false;
    mSessionConnectionStamp = 0.0;

    mCurrentJoinedGroup.clear();

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



void SonobusAudioProcessor::setAutoconnectToGroupPeers(bool flag)
{
    mAutoconnectGroupPeers = flag;
}


bool SonobusAudioProcessor::joinServerGroup(const String & group, const String & groupsecret)
{
    if (!mAooClient) return false;

    int32_t retval = mAooClient->group_join(group.toRawUTF8(), groupsecret.toRawUTF8());
    
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
        remote->oursource->setup(getSampleRate(), lastSamplesPerBlock, remote->sendChannels);        
        //remote->oursource->setup(getSampleRate(), remote->packetsize    , getTotalNumOutputChannels());        
        
        setupSourceFormat(remote, remote->latencysource.get(), true);
        remote->latencysource->setup(getSampleRate(), lastSamplesPerBlock, 1);
        setupSourceFormat(remote, remote->echosource.get(), true);
        remote->echosource->setup(getSampleRate(), lastSamplesPerBlock, 1);
        
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

void SonobusAudioProcessor::setRemotePeerCompressorParams(int index, CompressorParams & params)
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

    remote->compressorParams = params;
    remote->compressorParamsChanged = true;
}

bool SonobusAudioProcessor::getRemotePeerCompressorParams(int index, CompressorParams & retparams)
{
    if (index >= mRemotePeers.size()) return false;
    const ScopedReadLock sl (mCoreLock);        
    auto remote = mRemotePeers.getUnchecked(index);
    retparams = remote->compressorParams;
    return true;

}

void SonobusAudioProcessor::setInputCompressorParams(CompressorParams & params)
{
    // sanity check and compute automakeupgain if necessary    
    params.ratio = jlimit(1.0f, 120.0f, params.ratio);
    if (params.automakeupGain) {
        // makeupgain = (-thresh -  abs(thresh/ratio))/2
        params.makeupGainDb = (-params.thresholdDb - abs(params.thresholdDb/params.ratio)) * 0.5f;
    }

    mInputCompressorParams = params;
    mInputCompressorParamsChanged = true;
}

bool SonobusAudioProcessor::getInputCompressorParams(CompressorParams & retparams)
{
    retparams = mInputCompressorParams;
    return true;
}

void SonobusAudioProcessor::setInputLimiterParams(CompressorParams & params)
{
    // sanity check and compute automakeupgain if necessary    
    params.ratio = jlimit(1.0f, 120.0f, params.ratio);

    mInputLimiterParams = params;
    mInputLimiterParamsChanged = true;
}

bool SonobusAudioProcessor::getInputLimiterParams(CompressorParams & retparams)
{
    retparams = mInputLimiterParams;
    return true;
}


void SonobusAudioProcessor::setInputExpanderParams(CompressorParams & params)
{
    // sanity check and compute automakeupgain if necessary    
    params.ratio = jlimit(1.0f, 120.0f, params.ratio);

    mInputExpanderParams = params;
    mInputExpanderParamsChanged = true;
}

bool SonobusAudioProcessor::getInputExpanderParams(CompressorParams & retparams)
{
    retparams = mInputExpanderParams;
    return true;
}

void SonobusAudioProcessor::setInputEqParams(ParametricEqParams & params)
{
    mInputEqParams = params;
    mInputEqParamsChanged = true;
}

bool SonobusAudioProcessor::getInputEqParams(ParametricEqParams & retparams)
{
    retparams = mInputEqParams;
    return true;
}

void SonobusAudioProcessor::commitCompressorParams(RemotePeer * peer)
{   
    peer->compressorUI->setParamValue("/compressor/Bypass", peer->compressorParams.enabled ? 0.0f : 1.0f);
    peer->compressorUI->setParamValue("/compressor/knee", 2.0f);
    peer->compressorUI->setParamValue("/compressor/threshold", peer->compressorParams.thresholdDb);
    peer->compressorUI->setParamValue("/compressor/ratio", peer->compressorParams.ratio);
    peer->compressorUI->setParamValue("/compressor/attack", peer->compressorParams.attackMs * 1e-3);
    peer->compressorUI->setParamValue("/compressor/release", peer->compressorParams.releaseMs * 1e-3);
    peer->compressorUI->setParamValue("/compressor/makeup_gain", peer->compressorParams.makeupGainDb);
    
    float * tmp = peer->compressorUI->getParamZone("/compressor/outgain");
    if (tmp != peer->compressorOutputLevel) {
        peer->compressorOutputLevel = tmp; // pointer
    }
}

void SonobusAudioProcessor::commitInputCompressorParams()
{   
    mInputCompressorControl.setParamValue("/compressor/Bypass", mInputCompressorParams.enabled ? 0.0f : 1.0f);
    mInputCompressorControl.setParamValue("/compressor/knee", 2.0f);
    mInputCompressorControl.setParamValue("/compressor/threshold", mInputCompressorParams.thresholdDb);
    mInputCompressorControl.setParamValue("/compressor/ratio", mInputCompressorParams.ratio);
    mInputCompressorControl.setParamValue("/compressor/attack", mInputCompressorParams.attackMs * 1e-3);
    mInputCompressorControl.setParamValue("/compressor/release", mInputCompressorParams.releaseMs * 1e-3);
    mInputCompressorControl.setParamValue("/compressor/makeup_gain", mInputCompressorParams.makeupGainDb);

    float * tmp = mInputCompressorControl.getParamZone("/compressor/outgain");
    
    if (tmp != mInputCompressorOutputGain) {
        mInputCompressorOutputGain = tmp; // pointer
    }

}

void SonobusAudioProcessor::commitInputExpanderParams()
{
   
    //mInputCompressorControl.setParamValue("/compressor/Bypass", mInputCompressorParams.enabled ? 0.0f : 1.0f);
    mInputExpanderControl.setParamValue("/expander/knee", 3.0f);
    mInputExpanderControl.setParamValue("/expander/threshold", mInputExpanderParams.thresholdDb);
    mInputExpanderControl.setParamValue("/expander/ratio", mInputExpanderParams.ratio);
    mInputExpanderControl.setParamValue("/expander/attack", mInputExpanderParams.attackMs * 1e-3);
    mInputExpanderControl.setParamValue("/expander/release", mInputExpanderParams.releaseMs * 1e-3);

    float * tmp = mInputExpanderControl.getParamZone("/expander/outgain");

    
    if (tmp != mInputExpanderOutputGain) {
        mInputExpanderOutputGain = tmp; // pointer
    }

}

void SonobusAudioProcessor::commitInputLimiterParams()
{   
    //mInputLimiterControl.setParamValue("/limiter/Bypass", mInputLimiterParams.enabled ? 0.0f : 1.0f);
    //mInputLimiterControl.setParamValue("/limiter/threshold", mInputLimiterParams.thresholdDb);
    //mInputLimiterControl.setParamValue("/limiter/ratio", mInputLimiterParams.ratio);
    //mInputLimiterControl.setParamValue("/limiter/attack", mInputLimiterParams.attackMs * 1e-3);
    //mInputLimiterControl.setParamValue("/limiter/release", mInputLimiterParams.releaseMs * 1e-3);

    mInputLimiterControl.setParamValue("/compressor/Bypass", mInputLimiterParams.enabled ? 0.0f : 1.0f);
    mInputLimiterControl.setParamValue("/compressor/threshold", mInputLimiterParams.thresholdDb);
    mInputLimiterControl.setParamValue("/compressor/ratio", mInputLimiterParams.ratio);
    mInputLimiterControl.setParamValue("/compressor/attack", mInputLimiterParams.attackMs * 1e-3);
    mInputLimiterControl.setParamValue("/compressor/release", mInputLimiterParams.releaseMs * 1e-3);

}


void SonobusAudioProcessor::commitInputEqParams()
{
    for (int i=0; i < 2; ++i) {
        mInputEqControl[i].setParamValue("/parametric_eq/low_shelf/gain", mInputEqParams.lowShelfGain);
        mInputEqControl[i].setParamValue("/parametric_eq/low_shelf/transition_freq", mInputEqParams.lowShelfFreq);
        mInputEqControl[i].setParamValue("/parametric_eq/para1/peak_gain", mInputEqParams.para1Gain);
        mInputEqControl[i].setParamValue("/parametric_eq/para1/peak_frequency", mInputEqParams.para1Freq);
        mInputEqControl[i].setParamValue("/parametric_eq/para1/peak_q", mInputEqParams.para1Q);
        mInputEqControl[i].setParamValue("/parametric_eq/para2/peak_gain", mInputEqParams.para2Gain);
        mInputEqControl[i].setParamValue("/parametric_eq/para2/peak_frequency", mInputEqParams.para2Freq);
        mInputEqControl[i].setParamValue("/parametric_eq/para2/peak_q", mInputEqParams.para2Q);
        mInputEqControl[i].setParamValue("/parametric_eq/high_shelf/gain", mInputEqParams.highShelfGain);
        mInputEqControl[i].setParamValue("/parametric_eq/high_shelf/transition_freq", mInputEqParams.highShelfFreq);
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
                        
                            break;
                        }
                    }
                    
                    if (id == AOO_ID_WILDCARD || (remote->oursink->get_id(dummyid) && id == dummyid) ) {
                        if (remote->oursink->handle_message(buf, nbytes, endpoint, endpoint_send)) {
                            remote->dataPacketsReceived += 1;
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
    else {
        // not a valid AoO OSC message
        DBG("SonoBus: not a valid AOO message!");
    }
        
}

void SonobusAudioProcessor::doSendData()
{
    // just try to send for everybody
    const ScopedReadLock sl (mCoreLock);        

    // send stuff until there is nothing left to send
    
    bool didsomething = true;
    
    while (didsomething) {
        //mAooSource->send();
        didsomething = false;
        
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

        }
        
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
            if (peer) {
                const ScopedReadLock sl (mCoreLock);        

                // smooth it
                peer->pingTime = rtt; // * 0.5;
                if (rtt < 800.0 ) {
                    peer->smoothPingTime.Z *= 0.9f;
                    peer->smoothPingTime.push(peer->pingTime);
                }

                DBG("ping to source " << sourceId << " recvd from " <<  es->ipaddr << ":" << es->port << " -- " << diff1 << " " << diff2 << " " <<  rtt << " smooth: " << peer->smoothPingTime.xbar << " stdev: " <<peer->smoothPingTime.s2xx);
                
                
                if (!peer->hasRealLatency) {
                    peer->totalEstLatency =  peer->smoothPingTime.xbar + 2*peer->buffertimeMs + (1e3*currSamplesPerBlock/getSampleRate());
                }

                if (peer->autosizeBufferMode == AutoNetBufferModeAutoFull) {                    
                    // possibly adjust net buffer down, if it has been longer than threshold since last drop
                    double nowtime = Time::getMillisecondCounterHiRes();
                    const float nodropsthresh = 10.0; // no drops in 10 seconds
                    const float adjustlimit = 10; // don't adjust more often than once every 10 seconds

                    if (peer->lastNetBufDecrTime > 0 && peer->buffertimeMs > peer->netBufAutoBaseline) {
                        double deltatime = (nowtime - peer->lastNetBufDecrTime) * 1e-3;  
                        if (deltatime > adjustlimit) {
                            //float droprate =  (peer->dataPacketsDropped - peer->lastDropCount) / deltatime;
                            //if (droprate < dropratethresh) {
                            if (nowtime - peer->lastDroptime > nodropsthresh) {
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
                        peer->recvActive = true;
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
                    if (peer->recvChannels != f.header.nchannels) {
                        peer->recvChannels = std::min(MAX_PANNERS, f.header.nchannels);

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
                peer->recvActive = e->state > 0;                    
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
                    const float dropratethresh = 1.0/30.0; // 1 every 30 seconds
                    const float adjustlimit = 0.5; // don't adjust more often than once every 0.5 seconds

                    if (peer->lastDroptime > 0) {
                        double deltatime = (nowtime - peer->lastDroptime) * 1e-3;  
                        if (deltatime > adjustlimit) {
                            float droprate =  (peer->dataPacketsDropped - peer->lastDropCount) / deltatime;
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
                            }
                            
                            peer->lastDroptime = nowtime;
                            peer->lastDropCount = peer->dataPacketsDropped;
                        }
                    }
                    else {
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
                
                DBG("Server error: " << e->errormsg);
                
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
                DBG("Couldn't connect to server - " << e->errormsg);
                mIsConnectedToServer = false;
                mSessionConnectionStamp = 0.0;
            }
            
            clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientConnected, this, e->result > 0, e->errormsg);
            
            break;
        }
        case AOONET_CLIENT_DISCONNECT_EVENT:
        {
            aoonet_client_group_event *e = (aoonet_client_group_event *)events[i];
            if (e->result == 0){
                DBG("Disconnected from server - " << e->errormsg);
            }

            // don't remove all peers?
            //removeAllRemotePeers();
            
            mIsConnectedToServer = false;
            mSessionConnectionStamp = 0.0;

            clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientDisconnected, this, e->result > 0, e->errormsg);

            break;
        }
        case AOONET_CLIENT_GROUP_JOIN_EVENT:
        {
            aoonet_client_group_event *e = (aoonet_client_group_event *)events[i];
            if (e->result > 0){
                DBG("Joined group - " << e->name);
                const ScopedLock sl (mClientLock);        
                mCurrentJoinedGroup = CharPointer_UTF8 (e->name);
            } else {
                DBG("Couldn't join group " << e->name << " - " << e->errormsg);
            }
            clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientGroupJoined, this, e->result > 0, CharPointer_UTF8 (e->name), e->errormsg);
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
                DBG("Couldn't leave group " << e->name << " - " << e->errormsg);
            }

            clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientGroupLeft, this, e->result > 0, CharPointer_UTF8 (e->name), e->errormsg);

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

                if (mAutoconnectGroupPeers) {
                    connectRemotePeerRaw(e->address, CharPointer_UTF8 (e->user), CharPointer_UTF8 (e->group), !mMainRecvMute.get());
                }
                
                //aoo_node_add_peer(x->x_node, gensym(e->group), gensym(e->user),
                //                  (const struct sockaddr *)e->address, e->length);

                clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientPeerJoined, this, CharPointer_UTF8 (e->group), CharPointer_UTF8 (e->user));
                
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

                EndpointState * endpoint = findOrAddRawEndpoint(e->address);
                if (endpoint) {
                    
                    removeAllRemotePeersWithEndpoint(endpoint);
                }
                
                //aoo_node_remove_peer(x->x_node, gensym(e->group), gensym(e->user));
                clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientPeerLeft, this, CharPointer_UTF8 (e->group), CharPointer_UTF8 (e->user));

            } else {
                DBG("bug bad result on leave event");
            }
            break;
        }
        case AOONET_CLIENT_ERROR_EVENT:
        {
            aoonet_client_event *e = (aoonet_client_event *)events[i];
            DBG("client error: " << e->errormsg);
            clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientError, this, e->errormsg);
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

    remote->recvAllow = true;
    
    // special - use 0 
    bool ret = remote->oursink->invite_source(endpoint, 0, endpoint_send) == 1;
    
    if (ret) {
        DBG("Successfully invited remote peer at " << endpoint->ipaddr << ":" << endpoint->port << " - ourId " << remote->ourId);
        remote->connected = true;
        remote->invitedPeer = reciprocate;
        remote->recvActive = reciprocate;
        if (!mMainSendMute.get()) {
            remote->sendActive = true;
            remote->oursource->start();
        }
        else {
            remote->sendActive = false;
            remote->oursource->stop();
        }
        
    } else {
        DBG("Error inviting remote peer at " << endpoint->ipaddr << ":" << endpoint->port << " - ourId " << remote->ourId);
    }

    return ret;    
}

int SonobusAudioProcessor::connectRemotePeer(const String & host, int port, const String & username, const String & groupname, bool reciprocate)
{
    EndpointState * endpoint = findOrAddEndpoint(host, port);

    RemotePeer * remote = doAddRemotePeerIfNecessary(endpoint, AOO_ID_NONE, username, groupname); // get new one

    remote->recvAllow = true;

    // special - use 0 
    bool ret = remote->oursink->invite_source(endpoint, 0, endpoint_send) == 1;
    
    if (ret) {
        DBG("Successfully invited remote peer at " <<  host << ":" << port << " - ourId " << remote->ourId);
        remote->connected = true;
        remote->invitedPeer = reciprocate;
        remote->recvActive = reciprocate;
        if (!mMainSendMute.get()) {
            remote->sendActive = true;
            remote->oursource->start();
        }
        
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
        const ScopedWriteLock sl (mCoreLock);        
        
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
    const ScopedWriteLock sl (mCoreLock);        

    for (int index = 0; index < mRemotePeers.size(); ++index) {  
        auto remote = mRemotePeers.getUnchecked(index);
        
        commitCacheForPeer(remote);

        if (remote->connected) {
            disconnectRemotePeer(index);
        }
    }

    mRemotePeers.clear();
    
    // reset matrix
    for (int i=0; i < MAX_PEERS; ++i) {
        for (int j=0; j < MAX_PEERS; ++j) {
            mRemoteSendMatrix[i][j] = false;
        }
    }
    
    return true;
}


bool SonobusAudioProcessor::removeRemotePeer(int index)
{
    RemotePeer * remote = 0;
    bool ret = false;
    {
        const ScopedWriteLock sl (mCoreLock);        
        
        if (index < mRemotePeers.size()) {            
            remote = mRemotePeers.getUnchecked(index);

            commitCacheForPeer(remote);
            
            if (remote->connected) {
                disconnectRemotePeer(index);
            }
                        
            adjustRemoteSendMatrix(index, true);
            

            mRemotePeers.remove(index);
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

void SonobusAudioProcessor::setRemotePeerChannelPan(int index, int chan, float pan)
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        if (chan < MAX_PANNERS) {
            if (remote->recvChannels == 2) {
                remote->recvStereoPan[chan] = pan;                
            }
            else {
                remote->recvPan[chan] = pan;
            }
        }
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


float SonobusAudioProcessor::getRemotePeerChannelPan(int index, int chan) const
{
    float pan = 0.0f;
    
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        if (chan < remote->recvChannels) {            
            pan = remote->recvChannels == 2 ? remote->recvStereoPan[chan] : remote->recvPan[chan];
        }
    }
    return pan;
}

int SonobusAudioProcessor::getRemotePeerChannelCount(int index) const
{
    int ret = 0;
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        ret = remote->recvChannels;
    }
    return ret;
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
        newchancnt = isAnythingRoutedToPeer(index) ? getTotalNumOutputChannels() :  remote->nominalSendChannels <= 0 ? getTotalNumInputChannels() : remote->nominalSendChannels;
    }
    else {
        newchancnt = jmin(getTotalNumOutputChannels(), remote->sendChannelsOverride);
    }
    
    if (remote->sendChannels != newchancnt) {
        remote->sendChannels = newchancnt;
        DBG("Peer " << index << "  has new sendChannel count: " << remote->sendChannels);
        if (remote->oursource) {
            setupSourceFormat(remote, remote->oursource.get());
            remote->oursource->setup(getSampleRate(), currSamplesPerBlock, remote->sendChannels);
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
        
        if (mTransportSource.isPlaying() && mSendPlaybackAudio.get()) {
            // override sending
            int srcchans = mCurrentAudioFileSource ? mCurrentAudioFileSource->getAudioFormatReader()->numChannels : 2;
            setRemotePeerOverrideSendChannelCount(-1, jmax(getTotalNumInputChannels(), srcchans));
        }
        else if (!mTransportSource.isPlaying()) {
            // remove override
            setRemotePeerOverrideSendChannelCount(-1, -1); 
        }
    }
}

void SonobusAudioProcessor::setRemotePeerBufferTime(int index, float bufferMs)
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
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
        if (remote->hasRealLatency) {
            remote->totalEstLatency = remote->totalLatency + (remote->buffertimeMs - remote->bufferTimeAtRealLatency);
        }
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
        }
    }
}

SonobusAudioProcessor::AutoNetBufferMode SonobusAudioProcessor::getRemotePeerAutoresizeBufferMode(int index) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return remote->autosizeBufferMode;
    }
    return AutoNetBufferModeOff;    
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
        remote->recvActive = active;

        if (remote->recvActive) {
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

void  SonobusAudioProcessor::resetRemotePeerPacketStats(int index)
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        remote->dataPacketsResent = 0;
        remote->dataPacketsDropped = 0;
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
    const ScopedWriteLock sl (mCoreLock);        

    RemotePeer * retpeer = 0;

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

        retpeer = mRemotePeers.add(new RemotePeer(endpoint, newid));

        retpeer->userName = username;
        retpeer->groupName = groupname;
        
        retpeer->buffertimeMs = mBufferTime.get() * 1000.0f;
        retpeer->formatIndex = mDefaultAudioFormatIndex;
        retpeer->autosizeBufferMode = (AutoNetBufferMode) defaultAutoNetbufMode;
        
        findAndLoadCacheForPeer(retpeer);
        
        retpeer->oursink->setup(getSampleRate(), currSamplesPerBlock, getTotalNumOutputChannels());
        retpeer->oursink->set_buffersize(retpeer->buffertimeMs);

        int32_t flags = AOO_PROTOCOL_FLAG_COMPACT_DATA;
        retpeer->oursink->set_option(aoo_opt_protocol_flags, &flags, sizeof(int32_t));
        
        retpeer->sendChannels =  mSendChannels.get() <= 0 ?  getTotalNumInputChannels() : mSendChannels.get();
        
        setupSourceFormat(retpeer, retpeer->oursource.get());
        retpeer->oursource->setup(getSampleRate(), currSamplesPerBlock, retpeer->sendChannels);
        retpeer->oursource->set_buffersize(1000.0f * currSamplesPerBlock / getSampleRate());
        retpeer->oursource->set_packetsize(retpeer->packetsize);        

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
        
        retpeer->latencyProcessor.reset(new MTDM(getSampleRate()));
        retpeer->latencyMeasurer.reset(new LatencyMeasurer());
        
        retpeer->oursink->set_dynamic_resampling(mDynamicResampling.get() ? 1 : 0);
        retpeer->oursource->set_dynamic_resampling(mDynamicResampling.get() ? 1 : 0);

        
        retpeer->workBuffer.setSize(2, currSamplesPerBlock);

        int outchannels = getTotalNumOutputChannels();
        
        retpeer->recvMeterSource.resize (outchannels, meterRmsWindow);
        retpeer->sendMeterSource.resize (retpeer->sendChannels, meterRmsWindow);

        retpeer->sendAllow = !mMainSendMute.get();
        retpeer->sendAllowCache = true; // cache is allowed for new ones, so when it is unmuted it actually does
        
        retpeer->recvAllow = !mMainRecvMute.get();
        retpeer->recvAllowCache = true;
        
        // compressor stuff
        retpeer->compressor = std::make_unique<faustCompressor>();
        retpeer->compressorUI = std::make_unique<MapUI>();
        retpeer->compressor->init(getSampleRate());
        retpeer->compressor->buildUserInterface(retpeer->compressorUI.get());
        //for(int i=0; i < retpeer->compressorUI->getParamsCount(); i++){
        //    DBG(retpeer->compressorUI->getParamAddress(i));
        //}
        
        commitCompressorParams(retpeer);
        
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
                
                commitCompressorParams(retpeer);
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
    newcache.compressorParams = retpeer->compressorParams;
    newcache.netbuf = retpeer->buffertimeMs;
    newcache.netbufauto = retpeer->autosizeBufferMode;
    newcache.monopan = retpeer->recvPan[0];
    newcache.stereopan1 = retpeer->recvStereoPan[0];
    newcache.stereopan2 = retpeer->recvStereoPan[1];
    newcache.level = retpeer->gain;
    newcache.name = retpeer->userName;
    newcache.sendFormat = retpeer->formatIndex;
    
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
        retpeer->compressorParams = cache.compressorParams;
        retpeer->buffertimeMs = cache.netbuf;
        retpeer->autosizeBufferMode = (AutoNetBufferMode) cache.netbufauto;
        retpeer->recvPan[0] = cache.monopan;
        retpeer->recvStereoPan[0] = cache.stereopan1;
        retpeer->recvStereoPan[1] = cache.stereopan2;
        retpeer->gain = cache.level;
        retpeer->formatIndex = cache.sendFormat;
        
        return true;
    }
    return false;
}


bool SonobusAudioProcessor::removeAllRemotePeersWithEndpoint(EndpointState * endpoint)
{
    const ScopedWriteLock sl (mCoreLock);        

    bool didremove = false;
    
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
            mRemotePeers.remove(i);
        }
    }
    
    return didremove;
}


bool SonobusAudioProcessor::doRemoveRemotePeerIfNecessary(EndpointState * endpoint, int32_t ourId)
{
    const ScopedWriteLock sl (mCoreLock);        

    bool didremove = false;
    
    int i=0;
    for (auto s : mRemotePeers) {
        if (s->endpoint == endpoint && s->ourId == ourId) {
            didremove = true;
            commitCacheForPeer(s);
            mRemotePeers.remove(i);
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
            fmt->header.blocksize = lastSamplesPerBlock >= info.min_preferred_blocksize ? lastSamplesPerBlock : info.min_preferred_blocksize;
            fmt->header.samplerate = getSampleRate();
            fmt->header.nchannels = channels;
            fmt->bitdepth = info.bitdepth == 2 ? AOO_PCM_INT16 : info.bitdepth == 3 ? AOO_PCM_INT24 : info.bitdepth == 4 ? AOO_PCM_FLOAT32 : info.bitdepth == 8 ? AOO_PCM_FLOAT64 : AOO_PCM_INT16;

            return true;
        } 
        else if (info.codec == CodecOpus) {
            aoo_format_opus *fmt = (aoo_format_opus *)&retformat;
            fmt->header.codec = AOO_CODEC_OPUS;
            fmt->header.blocksize = lastSamplesPerBlock >= info.min_preferred_blocksize ? lastSamplesPerBlock : info.min_preferred_blocksize;
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
    int channels = latencymode ? 1  :  peer ? peer->sendChannels : getTotalNumInputChannels();
    
    if (formatInfoToAooFormat(info, channels, f)) {        
        source->set_format(f.header);        
    }
}


void SonobusAudioProcessor::parameterChanged (const String &parameterID, float newValue)
{
    if (parameterID == paramDry) {
        mDry = newValue;
    }
    else if (parameterID == paramInGain) {
        mInGain = newValue;
    }
    else if (parameterID == paramMetGain) {
        mMetGain = newValue;
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
    else if (parameterID == paramSendChannels) {
        mSendChannels = (int) newValue;
        
        setRemotePeerNominalSendChannelCount(-1, mSendChannels.get());        
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
    else if (parameterID == paramSendFileAudio) {
        mSendPlaybackAudio = newValue > 0;
        
        if (mTransportSource.isPlaying() && mSendPlaybackAudio.get()) {
            // override sending
            int srcchans = mCurrentAudioFileSource ? mCurrentAudioFileSource->getAudioFormatReader()->numChannels : 2;
            setRemotePeerOverrideSendChannelCount(-1, jmax(getTotalNumInputChannels(), srcchans)); // should be something different?
        }
        else if (!mSendPlaybackAudio.get()) {
            // remove override
            setRemotePeerOverrideSendChannelCount(-1, -1); 
        }
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
        mInMonMonoPan = newValue;
    }
    else if (parameterID == paramInMonitorPan1) {
        mInMonPan1 = newValue;
    }
    else if (parameterID == paramInMonitorPan2) {
        mInMonPan2 = newValue;
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
    
    lastSamplesPerBlock = currSamplesPerBlock = samplesPerBlock;

    DBG("Prepare to play: SR " <<  sampleRate << "  blocksize: " <<  samplesPerBlock << "  inch: " << getTotalNumInputChannels() << "  outch: " << getTotalNumOutputChannels());

    const ScopedReadLock sl (mCoreLock);        
    
    mMetronome->setSampleRate(sampleRate);
    mMainReverb->setSampleRate(sampleRate);
    mMReverb.setSampleRate(sampleRate);
    
    mZitaReverb.init(sampleRate);
    mZitaReverb.buildUserInterface(&mZitaControl);

    DBG("Zita Reverb Params:");    
    for(int i=0; i < mZitaControl.getParamsCount(); i++){
        DBG(mZitaControl.getParamAddress(i));
    }
    
    // setting default values for the Faust module parameters
    mZitaControl.setParamValue("/Zita_Rev1/Output/Dry/Wet_Mix", -1.0f);
    mZitaControl.setParamValue("/Zita_Rev1/Decay_Times_in_Bands_(see_tooltips)/Low_RT60", jlimit(1.0f, 8.0f, mMainReverbSize.get() * 7.0f + 1.0f));
    mZitaControl.setParamValue("/Zita_Rev1/Decay_Times_in_Bands_(see_tooltips)/Mid_RT60", jlimit(1.0f, 8.0f, mMainReverbSize.get() * 7.0f + 1.0f));
    mZitaControl.setParamValue("/Zita_Rev1/Output/Level", jlimit(-70.0f, 40.0f, Decibels::gainToDecibels(mMainReverbLevel.get()) + 6.0f));
    mZitaControl.setParamValue("/Zita_Rev1/Decay_Times_in_Bands_(see_tooltips)/HF_Damping",                                    
                               jmap(mMainReverbDamping.get(), 23520.0f, 1500.0f));

    mInputCompressor.init(sampleRate);
    mInputCompressor.buildUserInterface(&mInputCompressorControl);

    DBG("Compressor Params:");
    for(int i=0; i < mInputCompressorControl.getParamsCount(); i++){
        DBG(mInputCompressorControl.getParamAddress(i));
    }

    mInputExpander.init(sampleRate);
    mInputExpander.buildUserInterface(&mInputExpanderControl);

    DBG("Expander Params:");
    for(int i=0; i < mInputExpanderControl.getParamsCount(); i++){
        DBG(mInputExpanderControl.getParamAddress(i));
    }

    for (int i=0; i < 2; ++i) {
        mInputEq[i].init(sampleRate);
        mInputEq[i].buildUserInterface(&mInputEqControl[i]);
    }

    DBG("EQ Params:");
    for(int i=0; i < mInputEqControl[0].getParamsCount(); i++){
        DBG(mInputEqControl[0].getParamAddress(i));
    }

    mInputLimiter.init(sampleRate);
    mInputLimiter.buildUserInterface(&mInputLimiterControl);

    DBG("Limiter Params:");
    for(int i=0; i < mInputLimiterControl.getParamsCount(); i++){
        DBG(mInputLimiterControl.getParamAddress(i));
    }


    
    commitInputCompressorParams();
    commitInputExpanderParams();
    commitInputEqParams();
    commitInputLimiterParams();
    
    
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

    
    mMainReverbParams.roomSize = jmap(mMainReverbSize.get(), 0.55f, 1.0f);
    mMainReverb->setParameters(mMainReverbParams);

    
    mTransportSource.prepareToPlay(currSamplesPerBlock, getSampleRate());

    //mAooSource->set_format(fmt->header);
    setupSourceFormat(0, mAooDummySource.get());
    mAooDummySource->setup(sampleRate, samplesPerBlock, getTotalNumInputChannels());

    int inchannels = getTotalNumInputChannels();
    int outchannels = getTotalNumOutputChannels();
    int maxchans = jmax(inchannels, outchannels);
    
    meterRmsWindow = sampleRate * METER_RMS_SEC / currSamplesPerBlock;
    
    inputMeterSource.resize (inchannels, meterRmsWindow);
    outputMeterSource.resize (outchannels, meterRmsWindow);

    if (lastInputChannels == 0 || lastOutputChannels == 0) {
        lastInputChannels = inchannels;
        lastOutputChannels = outchannels;        
    }
    else if (lastInputChannels != inchannels || lastOutputChannels != outchannels) {
        /*
        if (inchannels < outchannels) {
            // center pan it
            mState.getParameter(paramInMonitorPan1)->setValueNotifyingHost(mState.getParameter(paramInMonitorPan1)->convertTo0to1(0.0));
            mState.getParameter(paramInMonitorPan2)->setValueNotifyingHost(mState.getParameter(paramInMonitorPan2)->convertTo0to1(0.0));
        }
        else {
            // hard pan left/right
            mState.getParameter(paramInMonitorPan1)->setValueNotifyingHost(mState.getParameter(paramInMonitorPan1)->convertTo0to1(-1.0));
            mState.getParameter(paramInMonitorPan2)->setValueNotifyingHost(mState.getParameter(paramInMonitorPan2)->convertTo0to1(1.0));
        }
         */

        lastInputChannels = inchannels;
        lastOutputChannels = outchannels;
    }
    
    int i=0;
    for (auto s : mRemotePeers) {
        if (s->workBuffer.getNumSamples() < samplesPerBlock) {
            s->workBuffer.setSize(2, samplesPerBlock);
        }

        s->sendChannels = isAnythingRoutedToPeer(i) ? outchannels : s->nominalSendChannels <= 0 ? inchannels : s->nominalSendChannels;
        if (s->sendChannelsOverride > 0) {
            s->sendChannels = jmin(outchannels, s->sendChannelsOverride);
        }
        
        if (s->oursource) {
            setupSourceFormat(s, s->oursource.get());
            s->oursource->setup(sampleRate, samplesPerBlock, s->sendChannels);  // todo use inchannels maybe?
            s->oursource->set_buffersize(1000.0f * currSamplesPerBlock / getSampleRate());
        }
        if (s->oursink) {
            s->oursink->setup(sampleRate, samplesPerBlock, outchannels);
        }
        
        if (s->latencysource) {
            setupSourceFormat(s, s->latencysource.get(), true);
            s->latencysource->setup(getSampleRate(), currSamplesPerBlock, 1);
            setupSourceFormat(s, s->echosource.get(), true);
            s->echosource->setup(getSampleRate(), currSamplesPerBlock, 1);
            s->echosource->set_buffersize(1000.0f * currSamplesPerBlock / getSampleRate());

            s->netBufAutoBaseline = (1e3*samplesPerBlock/getSampleRate()); // at least a process block

            s->latencysink->setup(sampleRate, samplesPerBlock, 1);
            s->echosink->setup(sampleRate, samplesPerBlock, 1);
            
            s->latencyProcessor.reset(new MTDM(sampleRate));
            s->latencyMeasurer.reset(new LatencyMeasurer());
        }

        s->recvMeterSource.resize (s->recvChannels, meterRmsWindow);
        s->sendMeterSource.resize (s->sendChannels, meterRmsWindow);
        
        s->compressor->init(sampleRate);
        
        ++i;
    }
    
    if (samplesPerBlock > mTempBufferSamples || maxchans > mTempBufferChannels) {
        ensureBuffers(samplesPerBlock);
    }
   
    
}

void SonobusAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    mTransportSource.releaseResources();

}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SonobusAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    
    // actually allow anything! what the hell.
    
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
//    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
//     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
//        return false;

    // allow different input counts than outputs
    
    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
   // if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
   //     return false;
   #endif

    return true;
  #endif
}
#endif

void SonobusAudioProcessor::ensureBuffers(int numSamples)
{
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    auto maxchans = jmax(2, jmax(totalNumInputChannels, totalNumOutputChannels));

    if (tempBuffer.getNumSamples() < numSamples || tempBuffer.getNumChannels() != maxchans) {
        tempBuffer.setSize(maxchans, numSamples);
    }
    if (workBuffer.getNumSamples() < numSamples || workBuffer.getNumChannels() != maxchans) {
        workBuffer.setSize(maxchans, numSamples);
    }
    if (inputBuffer.getNumSamples() < numSamples || inputBuffer.getNumChannels() != maxchans) {
        inputBuffer.setSize(maxchans, numSamples);
    }
    if (inputWorkBuffer.getNumSamples() < numSamples || inputWorkBuffer.getNumChannels() != maxchans) {
        inputWorkBuffer.setSize(maxchans, numSamples);
    }
    if (fileBuffer.getNumSamples() < numSamples || fileBuffer.getNumChannels() != maxchans) {
        fileBuffer.setSize(maxchans, numSamples);
    }
    if (metBuffer.getNumSamples() < numSamples || metBuffer.getNumChannels() != maxchans) {
        metBuffer.setSize(maxchans, numSamples);
    }
    if (mainFxBuffer.getNumSamples() < numSamples || mainFxBuffer.getNumChannels() != maxchans) {
        mainFxBuffer.setSize(maxchans, numSamples);
    }
    
    mTempBufferSamples = jmax(mTempBufferSamples, numSamples);
    mTempBufferChannels = jmax(maxchans, mTempBufferChannels);
}


void SonobusAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    auto maxchans = jmax(2, jmax(totalNumInputChannels, totalNumOutputChannels));
    
    float inGain = mInGain.get();
    float drynow = mDry.get(); // DB_CO(dry_level);
    float wetnow = mWet.get(); // DB_CO(wet_level);
    float inmonMonoPan = mInMonMonoPan.get();
    float inmonPan1 = mInMonPan1.get();
    float inmonPan2 = mInMonPan2.get();
    
    inGain = mMainInMute.get() ? 0.0f : inGain;
    
    int numSamples = buffer.getNumSamples();
    
    // THIS SHOULDN"T GENERALLY HAPPEN, it should have been taken care of in prepareToPlay, but just in case.
    // I know this isn't RT safe.
    if (numSamples > mTempBufferSamples || maxchans > mTempBufferChannels) {
        ensureBuffers(numSamples);
    }
    
    
    
    AudioPlayHead::CurrentPositionInfo posInfo;
    posInfo.resetToDefault();
    posInfo.bpm = mMetTempo.get();
    AudioPlayHead * playhead = getPlayHead();
    bool posValid = playhead && playhead->getCurrentPosition(posInfo);
    bool hostPlaying = posValid && posInfo.isPlaying;
    if (posInfo.bpm <= 0.0) {
        posInfo.bpm = mMetTempo.get();        
    }
    if (posValid && fabs(posInfo.bpm - mMetTempo.get()) > 0.001) {
        mMetTempo = posInfo.bpm;
        mTempoParameter->setValueNotifyingHost(mTempoParameter->convertTo0to1(mMetTempo.get()));
    }
    
    
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
        // replicate last input channel
        if (totalNumInputChannels > 0) {
            buffer.copyFrom(i, 0, buffer, totalNumInputChannels-1, 0, numSamples);
        } else {
            buffer.clear (i, 0, numSamples);            
        }
    }
    
    
    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.


    uint64_t t = aoo_osctime_get();


    inputWorkBuffer.clear(0, numSamples);


    
    // apply input gain
    if (fabsf(inGain - mLastInputGain) > 0.00001) {
        buffer.applyGainRamp(0, numSamples, mLastInputGain, inGain);
    } else {
        buffer.applyGain(inGain);
    }


    // apply input expander 
    if (mInputExpanderParamsChanged) {
        commitInputExpanderParams();
        mInputExpanderParamsChanged = false;
    }
    if (mLastInputExpanderEnabled || mInputExpanderParams.enabled) {
        if (buffer.getNumChannels() > 1) {
            mInputExpander.compute(numSamples, buffer.getArrayOfWritePointers(), buffer.getArrayOfWritePointers());
        } else {
            float *tmpbuf[2];
            tmpbuf[0] = buffer.getWritePointer(0);
            tmpbuf[1] = inputWorkBuffer.getWritePointer(0); // just a silent dummy buffer
            mInputExpander.compute(numSamples, tmpbuf, tmpbuf);
        }
    }
    mLastInputExpanderEnabled = mInputExpanderParams.enabled;

    
    // apply input compressor
    if (mInputCompressorParamsChanged) {
        commitInputCompressorParams();
        mInputCompressorParamsChanged = false;
    }
    if (mLastInputCompressorEnabled || mInputCompressorParams.enabled) {
        if (buffer.getNumChannels() > 1) {
            mInputCompressor.compute(numSamples, buffer.getArrayOfWritePointers(), buffer.getArrayOfWritePointers());
        } else {
            float *tmpbuf[2];
            tmpbuf[0] = buffer.getWritePointer(0);
            tmpbuf[1] = inputWorkBuffer.getWritePointer(0); // just a silent dummy buffer
            mInputCompressor.compute(numSamples, tmpbuf, tmpbuf);
        }
    }
    mLastInputCompressorEnabled = mInputCompressorParams.enabled;


    // apply input EQ
    if (mInputEqParamsChanged) {
        commitInputEqParams();
        mInputEqParamsChanged = false;
    }
    if (mLastInputEqEnabled || mInputEqParams.enabled) {
        if (buffer.getNumChannels() > 1) {
            // only 2 channels support for now... TODO
            float *tmpbuf[2];
            tmpbuf[0] = buffer.getWritePointer(0);
            tmpbuf[1] = buffer.getWritePointer(1);
            mInputEq[0].compute(numSamples, &tmpbuf[0], &tmpbuf[0]);
            mInputEq[1].compute(numSamples, &tmpbuf[1], &tmpbuf[1]);
        } else {
            float *tmpbuf[2];
            tmpbuf[0] = buffer.getWritePointer(0);
            mInputEq[0].compute(numSamples, &tmpbuf[0], &tmpbuf[0]);
        }
    }
    mLastInputEqEnabled = mInputEqParams.enabled;
    
    
    // apply input limiter
    if (mInputLimiterParamsChanged) {
        commitInputLimiterParams();
        mInputLimiterParamsChanged = false;
    }
    if (mLastInputLimiterEnabled || mInputLimiterParams.enabled) {
        if (buffer.getNumChannels() > 1) {
            mInputLimiter.compute(numSamples, buffer.getArrayOfWritePointers(), buffer.getArrayOfWritePointers());
        } else {
            float *tmpbuf[2];
            tmpbuf[0] = buffer.getWritePointer(0);
            tmpbuf[1] = inputWorkBuffer.getWritePointer(0); // just a silent dummy buffer
            mInputLimiter.compute(numSamples, tmpbuf, tmpbuf);
        }
    }
    mLastInputLimiterEnabled = mInputLimiterParams.enabled;

    
    
    // meter pre-panning, and post compressor
    inputMeterSource.measureBlock (buffer);
    float redlev = 1.0f;
    if (mInputCompressorParams.enabled && mInputCompressorOutputGain) {
        redlev = jlimit(0.0f, 1.0f, Decibels::decibelsToGain(*mInputCompressorOutputGain));
    }
    inputMeterSource.setReductionLevel(redlev);
    
    
    // do the input panning before everything else
    int panChannels = jmin(inputBuffer.getNumChannels(), jmax(totalNumOutputChannels, mSendChannels.get()));
    
    if (totalNumInputChannels > 0 && panChannels > 1 ) {
        inputBuffer.clear(0, numSamples);

        for (int channel = 0; channel < panChannels; ++channel) {
            for (int i=0; i < totalNumInputChannels; ++i) {
                const float pan = (totalNumInputChannels == 1 ? inmonMonoPan : i==0 ? inmonPan1 : inmonPan2);
                const float lastpan = (totalNumInputChannels == 1 ? mLastInMonMonoPan : i==0 ? mLastInMonPan1 : mLastInMonPan2);
                
                // apply pan law
                // -1 is left, 1 is right
                float pgain = channel == 0 ? (pan >= 0.0f ? (1.0f - pan) : 1.0f) : (pan >= 0.0f ? 1.0f : (1.0f+pan)) ;
                
                if (fabsf(pan - lastpan) > 0.00001f) {
                    float plastgain = channel == 0 ? (lastpan >= 0.0f ? (1.0f - lastpan) : 1.0f) : (lastpan >= 0.0f ? 1.0f : (1.0f+lastpan));
                    
                    inputBuffer.addFromWithRamp(channel, 0, buffer.getReadPointer(i), numSamples, plastgain, pgain);
                } else {
                    inputBuffer.addFrom (channel, 0, buffer, i, 0, numSamples, pgain);
                }
            }
        }
    } else if (totalNumInputChannels > 0){
        for (int channel = 0; channel < totalNumOutputChannels; ++channel) {
            // used later
            int srcchan = channel < totalNumInputChannels ? channel : totalNumInputChannels - 1;
            inputBuffer.copyFrom(channel, 0, buffer, srcchan, 0, numSamples);
        }
    }
    else {
        inputBuffer.clear(0, numSamples);
        
    }
    

    
    // rework the buffer to contain the contents we want going out

    for (int channel = 0; channel < panChannels /* && totalNumInputChannels > 0 */ ; ++channel) {
        //int inchan = channel < totalNumInputChannels ? channel : totalNumInputChannels-1;
        //int inchan = channel < totalNumInputChannels ? channel : totalNumInputChannels-1;                    
        if (mSendChannels.get() == 1 || (mSendChannels.get() == 0 && totalNumInputChannels == 1)) {
            // add un-panned 1st channel only
            inputWorkBuffer.addFrom(channel, 0, buffer, 0, 0, numSamples);
        }
        else {
            // add our input that is panned
            inputWorkBuffer.addFrom(channel, 0, inputBuffer, channel, 0, numSamples);
        }
    }


    
    // file playback goes to everyone
    bool sendfileaudio = mSendPlaybackAudio.get();

    bool hasfiledata = false;
    if (mTransportSource.getTotalLength() > 0)
    {
        AudioSourceChannelInfo info (&fileBuffer, 0, numSamples);
        mTransportSource.getNextAudioBlock (info);
        hasfiledata = true;
        
        if (sendfileaudio) {
            //add to main buffer for going out
            for (int channel = 0; channel < totalNumOutputChannels; ++channel) {
                inputWorkBuffer.addFrom(channel, 0, fileBuffer, channel, 0, numSamples);
            }
        }

    }
    
    // process metronome
    bool metenabled = mMetEnabled.get();
    bool sendmet = mSendMet.get();
    float metgain = mMetGain.get();
    double mettempo = mMetTempo.get();
    bool metrecorded = mMetIsRecorded.get();

    if (metenabled != mLastMetEnabled) {
        if (metenabled) {
            mMetronome->setGain(metgain, true);
            if (!hostPlaying) {
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
        if (hostPlaying) {
            beattime = posInfo.ppqPosition;
        }
        mMetronome->processMix(numSamples, metBuffer.getWritePointer(0), metBuffer.getWritePointer(totalNumOutputChannels > 1 ? 1 : 0), beattime, !hostPlaying);

        if (sendmet) {
            //add to main buffer for going out
            for (int channel = 0; channel < totalNumOutputChannels; ++channel) {
                inputWorkBuffer.addFrom(channel, 0, metBuffer, channel, 0, numSamples);
            }            
        }
    }
    mLastMetEnabled = metenabled;

    
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
        
        for (auto & remote : mRemotePeers) 
        {
            
            if (!remote->oursink) continue;
            
            remote->workBuffer.clear(0, numSamples);

            // calculate fill ratio before processing the sink
            float retratio = 0.0f;
            if (remote->oursink->get_sourceoption(remote->endpoint, remote->remoteSourceId, aoo_opt_buffer_fill_ratio, &retratio, sizeof(retratio)) > 0) {
                remote->fillRatio.Z *= 0.95;
                remote->fillRatio.push(retratio);
                remote->fillRatioSlow.Z *= 0.99;
                remote->fillRatioSlow.push(retratio);
            }

            
            // get audio data coming in from outside into tempbuf
            remote->oursink->process(remote->workBuffer.getArrayOfWritePointers(), numSamples, t);


            // apply compressor
            if (remote->compressorParamsChanged) {
                commitCompressorParams(remote);
                remote->compressorParamsChanged = false;
            }
            if (remote->lastCompressorEnabled || remote->compressorParams.enabled) {
                remote->compressor->compute(numSamples, remote->workBuffer.getArrayOfWritePointers(), remote->workBuffer.getArrayOfWritePointers());
            }
            remote->lastCompressorEnabled = remote->compressorParams.enabled;

            
            float usegain = remote->gain;
            
            // we get the stuff, but ignore it (either muted or others soloed)
            if (!remote->recvActive || (anysoloed && !remote->soloed)) {
                if (remote->_lastgain > 0.0f) {
                    // fade out
                    usegain = 0.0f;
                }
                else {
                    continue;
                }
            } 

            
            // apply wet gain for this to tempbuf
            if (fabsf(usegain - remote->_lastgain) > 0.00001) {
                remote->workBuffer.applyGainRamp(0, numSamples, remote->_lastgain, usegain);
                remote->_lastgain = usegain;
            } else {
                remote->workBuffer.applyGain(remote->gain);
            }

            remote->recvMeterSource.measureBlock (remote->workBuffer);
            float redlev = 1.0f;
            if (remote->compressorParams.enabled && remote->compressorOutputLevel) {
                redlev = jlimit(0.0f, 1.0f, Decibels::decibelsToGain(*(remote->compressorOutputLevel)));
            }
            remote->recvMeterSource.setReductionLevel(redlev);



            
            for (int channel = 0; channel < totalNumOutputChannels; ++channel) {

                // now apply panning

                if (remote->recvChannels > 0 && totalNumOutputChannels > 1) {
                    for (int i=0; i < remote->recvChannels; ++i) {
                        const float pan = remote->recvChannels == 2 ? remote->recvStereoPan[i] : remote->recvPan[i];
                        const float lastpan = remote->recvPanLast[i];
                        
                        // apply pan law
                        // -1 is left, 1 is right
                        float pgain = channel == 0 ? (pan >= 0.0f ? (1.0f - pan) : 1.0f) : (pan >= 0.0f ? 1.0f : (1.0f+pan)) ;

                        if (pan != lastpan) {
                            float plastgain = channel == 0 ? (lastpan >= 0.0f ? (1.0f - lastpan) : 1.0f) : (lastpan >= 0.0f ? 1.0f : (1.0f+lastpan));
                            
                            tempBuffer.addFromWithRamp(channel, 0, remote->workBuffer.getReadPointer(i), numSamples, plastgain, pgain);
                        } else {
                            tempBuffer.addFrom (channel, 0, remote->workBuffer, i, 0, numSamples, pgain);                            
                        }

                        //remote->recvPanLast[i] = pan;
                    }
                } else {
                    
                    tempBuffer.addFrom(channel, 0, remote->workBuffer, channel, 0, numSamples);
                }
            }
        }
        
        
        
        // send out final outputs
        int i=0;
        for (auto & remote : mRemotePeers) 
        {
            if (remote->oursource /*&& remote->sendActive */) {

                /*
                if (needsblockchange) {
                    setupSourceFormat(remote, remote->oursource.get());
                    remote->oursource->setup(getSampleRate(), numSamples, getTotalNumOutputChannels());

                }
                */
                
                workBuffer.clear(0, numSamples);
                            
                for (int channel = 0; channel < remote->sendChannels /* && totalNumInputChannels > 0 */ ; ++channel) {
                    //int inchan = channel < totalNumInputChannels ? channel : totalNumInputChannels-1;
                    int inchan = channel < totalNumInputChannels ? channel : totalNumInputChannels-1;                    
                    //if (remote->sendChannels == 1) {
                        // add un-panned 1st channel only
                    //    workBuffer.addFrom(channel, 0, buffer, 0, 0, numSamples);
                    //}
                    //else {
                        // add our input that is panned
                        workBuffer.addFrom(channel, 0, inputWorkBuffer, channel, 0, numSamples);
                    //}
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
                
                
                remote->oursource->process(workBuffer.getArrayOfReadPointers(), numSamples, t);      
                
                //remote->sendMeterSource.measureBlock (workBuffer);
                
                
                // now process echo and latency stuff
                
                workBuffer.clear(0, 0, numSamples);
                if (remote->echosink->process(workBuffer.getArrayOfWritePointers(), numSamples, t)) {
                    //DBG("received something from our ECHO sink");
                    remote->echosource->process(workBuffer.getArrayOfReadPointers(), numSamples, t);
                }

                
                if (remote->activeLatencyTest && remote->latencyProcessor) {
                    workBuffer.clear(0, 0, numSamples);
                    if (remote->latencysink->process(workBuffer.getArrayOfWritePointers(), numSamples, t)) {
                        //DBG("received something from our latency sink");
                    }

                    // hear latency measure stuff (recv into right channel)
                    if (hearlatencytest) {
                        tempBuffer.addFrom(totalNumOutputChannels > 1 ? 1 : 0, 0, workBuffer, 0, 0, numSamples);
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

                    
                    remote->latencysource->process(workBuffer.getArrayOfReadPointers(), numSamples, t);                                        
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
    drynow = (anysoloed && !mMainMonitorSolo.get()) ? 0.0f : drynow;

    bool inrevdirect = !(anysoloed && !mMainMonitorSolo.get()) && drynow == 0.0;
    
    // copy from input buffer with dry gain as-is
    bool rampit =  (fabsf(drynow - mLastDry) > 0.00001);
    for (int channel = 0; channel < totalNumOutputChannels; ++channel) {
        //int usechan = channel < totalNumInputChannels ? channel : channel > 0 ? channel-1 : 0;
        if (rampit) {
            buffer.copyFromWithRamp(channel, 0, inputBuffer.getReadPointer(channel), numSamples, mLastDry, drynow);
        }
        else {
            buffer.copyFrom(channel, 0, inputBuffer.getReadPointer(channel), numSamples, drynow);
        }
    }
    
    
    // EFFECTS
    bool mainReverbEnabled = mMainReverbEnabled.get();
    bool doreverb = mainReverbEnabled || mLastMainReverbEnabled;
    bool hasmainfx = doreverb;

    mainFxBuffer.clear(0, numSamples);
    
    // TODO other main FX
    if (mainReverbEnabled != mLastMainReverbEnabled) {

        float sgain = mainReverbEnabled ? 0.0f : 1.0f;
        float egain = mainReverbEnabled ? 1.0f : 0.0f;
        
        if (mainReverbEnabled) {
            mMainReverb->reset();
            mMReverb.reset();
            mZitaReverb.instanceClear();
        }
        
        for (int channel = 0; channel < totalNumOutputChannels; ++channel) {
            // others
            mainFxBuffer.addFromWithRamp(channel, 0, tempBuffer.getReadPointer(channel), numSamples, sgain, egain);

            // input
            if (inrevdirect) {
                mainFxBuffer.addFromWithRamp(channel, 0, inputBuffer.getReadPointer(channel), numSamples, sgain, egain);
            } else {
                mainFxBuffer.addFromWithRamp(channel, 0, buffer.getReadPointer(channel), numSamples, sgain, egain);                
            }
        }                    
    }
    else if (mainReverbEnabled){
        for (int channel = 0; channel < totalNumOutputChannels; ++channel) {
            // others
            mainFxBuffer.addFrom(channel, 0, tempBuffer, channel, 0, numSamples);

            // input
            if (inrevdirect) {
                mainFxBuffer.addFrom(channel, 0, inputBuffer, channel, 0, numSamples);
            } else {
                mainFxBuffer.addFrom(channel, 0, buffer, channel, 0, numSamples);                
            }
        }            

    }
    
    
    // add from tempBuffer (audio from remote peers)
    for (int channel = 0; channel < totalNumOutputChannels; ++channel) {

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
            if (totalNumOutputChannels > 1) {            
                mMReverb.process(mainFxBuffer.getArrayOfWritePointers(), mainFxBuffer.getArrayOfWritePointers(), numSamples);
            } 
        }
        else if (mMainReverbModel.get() == ReverbModelZita) {
            if (totalNumOutputChannels > 1) {            
                mZitaReverb.compute(numSamples, mainFxBuffer.getArrayOfWritePointers(), mainFxBuffer.getArrayOfWritePointers()); 
            }
        }
        else {
            if (totalNumOutputChannels > 1) {            
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
        for (int channel = 0; channel < totalNumOutputChannels; ++channel) {
            buffer.addFrom(channel, 0, mainFxBuffer, channel, 0, numSamples);
        }        
    }
    
    
    
    // add from file playback buffer
    if (hasfiledata) {
        for (int channel = 0; channel < totalNumOutputChannels; ++channel) {
            buffer.addFrom(channel, 0, fileBuffer, channel, 0, numSamples);
        }
    }
    
   
    if (metenabled) {
        //add from met buffer
        for (int channel = 0; channel < totalNumOutputChannels; ++channel) {
            buffer.addFrom(channel, 0, metBuffer, channel, 0, numSamples);
        }            
    }

    
    
    
    
    // apply wet (output) gain to original buf
    if (fabsf(wetnow - mLastWet) > 0.00001) {
        buffer.applyGainRamp(0, numSamples, mLastWet, wetnow);
    } else {
        buffer.applyGain(wetnow);
    }


    
   

    
    outputMeterSource.measureBlock (buffer);

    // output to file writer if necessary
    if (writingPossible) {
        const ScopedTryLock sl (writerLock);
        if (sl.isLocked())
        {
            if (activeWriter.load() != nullptr)
            {
                // we need to mix the input, audio from remote peers, and the file playback together here
                //workBuffer.clear(0, numSamples);
                
                bool rampit =  (fabsf(wetnow - mLastWet) > 0.00001);
                for (int channel = 0; channel < totalNumOutputChannels; ++channel) {
                    int usechan = channel < totalNumInputChannels ? channel : channel > 0 ? channel-1 : 0;

                    // copy input as-is
                    workBuffer.copyFrom(channel, 0, inputBuffer.getReadPointer(usechan), numSamples);

                    // apply Main out gain to audio from remote peers and file playback (should we?)
                    if (rampit) {
                        workBuffer.addFromWithRamp(channel, 0, tempBuffer.getReadPointer(channel), numSamples, mLastWet, wetnow);
                        if (hasmainfx) {
                            workBuffer.addFromWithRamp(channel, 0, mainFxBuffer.getReadPointer(channel), numSamples, mLastWet, wetnow);
                        }
                        if (hasfiledata) {
                            workBuffer.addFromWithRamp(channel, 0, fileBuffer.getReadPointer(channel), numSamples, mLastWet, wetnow);
                        }
                        if (metrecorded && metenabled) {
                            workBuffer.addFromWithRamp(channel, 0, metBuffer.getReadPointer(channel), numSamples, mLastWet, wetnow);
                        }
                    }
                    else {
                        workBuffer.addFrom(channel, 0, tempBuffer, channel, 0, numSamples, wetnow);

                        if (hasmainfx) {
                            workBuffer.addFrom(channel, 0, mainFxBuffer, channel, 0, numSamples, wetnow);
                        }
                        if (hasfiledata) {
                            workBuffer.addFrom(channel, 0, fileBuffer, channel, 0, numSamples, wetnow);
                        }
                        if (metrecorded && metenabled) {
                            workBuffer.addFrom(channel, 0, metBuffer, channel, 0, numSamples, wetnow);
                        }
                    }
                }
                
                activeWriter.load()->write (workBuffer.getArrayOfReadPointers(), numSamples);
                
                mElapsedRecordSamples += numSamples;
            }
        }
    }
    
    //if (needsblockchange) {
    //lastSamplesPerBlock = numSamples;
    //}

    notifySendThread();
    
    mLastWet = wetnow;
    mLastDry = drynow;
    mLastInputGain = inGain;
    mLastInMonPan1 = inmonPan1;
    mLastInMonPan2 = inmonPan2;
    mLastInMonMonoPan = inmonMonoPan;
    mAnythingSoloed =  anysoloed;

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
}


//==============================================================================
void SonobusAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    MemoryOutputStream stream(destData, false);

    ValueTree recentsTree = mState.state.getOrCreateChildWithName(recentsCollectionKey, nullptr);
    // update state with our recents info
    recentsTree.removeAllChildren(nullptr);
    for (auto & info : mRecentConnectionInfos) {
        recentsTree.appendChild(info.getValueTree(), nullptr);        
    }

    ValueTree extraTree = mState.state.getOrCreateChildWithName(extraStateCollectionKey, nullptr);
    // update state with our recents info
    extraTree.removeAllChildren(nullptr);
    extraTree.setProperty(useSpecificUdpPortKey, mUseSpecificUdpPort, nullptr);
    extraTree.setProperty(changeQualForAllKey, mChangingDefaultAudioCodecChangesAll, nullptr);

    ValueTree inputEffectsTree = mState.state.getOrCreateChildWithName(inputEffectsStateKey, nullptr);
    inputEffectsTree.removeAllChildren(nullptr);
    inputEffectsTree.appendChild(mInputCompressorParams.getValueTree(compressorStateKey), nullptr); 
    inputEffectsTree.appendChild(mInputExpanderParams.getValueTree(expanderStateKey), nullptr); 
    inputEffectsTree.appendChild(mInputLimiterParams.getValueTree(limiterStateKey), nullptr); 
    inputEffectsTree.appendChild(mInputEqParams.getValueTree(), nullptr); 

    
    storePeerCacheToState();
    
    mState.state.writeToStream (stream);
    
    DBG("GETSTATE: " << mState.state.toXmlString());
}

void SonobusAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    ValueTree tree = ValueTree::readFromData (data, sizeInBytes);
    if (tree.isValid()) {
        mState.state = tree;
        
        DBG("SETSTATE: " << mState.state.toXmlString());
        ValueTree recentsTree = mState.state.getChildWithName(recentsCollectionKey);
        if (recentsTree.isValid()) {
            mRecentConnectionInfos.clear();
            for (auto child : recentsTree) {
                AooServerConnectionInfo info;
                info.setFromValueTree(child);
                mRecentConnectionInfos.add(info);
            }
        }

        ValueTree extraTree = mState.state.getChildWithName(extraStateCollectionKey);
        if (extraTree.isValid()) {
            int port = extraTree.getProperty(useSpecificUdpPortKey, mUseSpecificUdpPort);
            setUseSpecificUdpPort(port);

            bool chqual = extraTree.getProperty(changeQualForAllKey, mChangingDefaultAudioCodecChangesAll);
            setChangingDefaultAudioCodecSetsExisting(chqual);
        }
        
       ValueTree inputEffectsTree = mState.state.getChildWithName(inputEffectsStateKey);
       if (inputEffectsTree.isValid()) {

           ValueTree compressorTree = inputEffectsTree.getChildWithName(compressorStateKey);
           if (compressorTree.isValid()) {
               mInputCompressorParams.setFromValueTree(compressorTree);
               commitInputCompressorParams();
           }           
           ValueTree expanderTree = inputEffectsTree.getChildWithName(expanderStateKey);
           if (expanderTree.isValid()) {
               mInputExpanderParams.setFromValueTree(expanderTree);
               commitInputExpanderParams();
           }               
           ValueTree limiterTree = inputEffectsTree.getChildWithName(limiterStateKey);
           if (limiterTree.isValid()) {
               mInputLimiterParams.setFromValueTree(limiterTree);
               commitInputLimiterParams();
           } 
           
           ValueTree eqTree = inputEffectsTree.getChildWithName(eqStateKey);
           if (eqTree.isValid()) {
               mInputEqParams.setFromValueTree(eqTree);
               commitInputEqParams();
           }           
       }
       
        
        
        loadPeerCacheFromState();
        
        // don't recover the metronome enable state, always default it to off
        mState.getParameter(paramMetEnabled)->setValueNotifyingHost(0.0f);

        // don't recover the recv mute either
        mState.getParameter(paramMainRecvMute)->setValueNotifyingHost(0.0f);

        // don't recover main solo
        mState.getParameter(paramMainMonitorSolo)->setValueNotifyingHost(0.0f);
        
    }
}

ValueTree SonobusAudioProcessor::CompressorParams::getValueTree(const String & stateKey) const
{
    ValueTree item(stateKey);
    
    item.setProperty(compressorEnabledKey, enabled, nullptr);
    item.setProperty(compressorThresholdKey, thresholdDb, nullptr);
    item.setProperty(compressorRatioKey, ratio, nullptr);
    item.setProperty(compressorAttackKey, attackMs, nullptr);
    item.setProperty(compressorReleaseKey, releaseMs, nullptr);
    item.setProperty(compressorMakeupGainKey, makeupGainDb, nullptr);
    item.setProperty(compressorAutoMakeupGainKey, automakeupGain, nullptr);
    
    return item;
}

void SonobusAudioProcessor::CompressorParams::setFromValueTree(const ValueTree & item)
{    
    enabled = item.getProperty(compressorEnabledKey, enabled);
    thresholdDb = item.getProperty(compressorThresholdKey, thresholdDb);
    ratio = item.getProperty(compressorRatioKey, ratio);
    attackMs = item.getProperty(compressorAttackKey, attackMs);
    releaseMs = item.getProperty(compressorReleaseKey, releaseMs);
    makeupGainDb = item.getProperty(compressorMakeupGainKey, makeupGainDb);
    automakeupGain = item.getProperty(compressorAutoMakeupGainKey, automakeupGain);
}

ValueTree SonobusAudioProcessor::ParametricEqParams::getValueTree() const
{
    ValueTree item(eqStateKey);
    
    item.setProperty(eqEnabledKey, enabled, nullptr);
    item.setProperty(eqLowShelfFreqKey, lowShelfFreq, nullptr);
    item.setProperty(eqLowShelfGainKey, lowShelfGain, nullptr);
    item.setProperty(eqHighShelfFreqKey, highShelfFreq, nullptr);
    item.setProperty(eqHighShelfGainKey, highShelfGain, nullptr);
    item.setProperty(eqPara1GainKey, para1Gain, nullptr);
    item.setProperty(eqPara1FreqKey, para1Freq, nullptr);
    item.setProperty(eqPara1QKey, para1Q, nullptr);
    item.setProperty(eqPara2GainKey, para2Gain, nullptr);
    item.setProperty(eqPara2FreqKey, para2Freq, nullptr);
    item.setProperty(eqPara2QKey, para2Q, nullptr);
    
    return item;
}

void SonobusAudioProcessor::ParametricEqParams::setFromValueTree(const ValueTree & item)
{    
    enabled = item.getProperty(eqEnabledKey, enabled);
    lowShelfFreq = item.getProperty(eqLowShelfFreqKey, lowShelfFreq);
    lowShelfGain = item.getProperty(eqLowShelfGainKey, lowShelfGain);
    highShelfFreq = item.getProperty(eqHighShelfFreqKey, highShelfFreq);
    highShelfGain = item.getProperty(eqHighShelfGainKey, highShelfGain);
    para1Gain = item.getProperty(eqPara1GainKey, para1Gain);
    para1Freq = item.getProperty(eqPara1FreqKey, para1Freq);
    para1Q = item.getProperty(eqPara1QKey, para1Q);
    para2Gain = item.getProperty(eqPara2GainKey, para2Gain);
    para2Freq = item.getProperty(eqPara2FreqKey, para2Freq);
    para2Q = item.getProperty(eqPara2QKey, para2Q);

}


ValueTree SonobusAudioProcessor::PeerStateCache::getValueTree() const
{
    ValueTree item(peerStateCacheKey);
    
    item.setProperty(peerNameKey, name, nullptr);
    item.setProperty(peerMonoPanKey, monopan, nullptr);
    item.setProperty(peerPan1Key, stereopan1, nullptr);
    item.setProperty(peerPan2Key, stereopan2, nullptr);
    item.setProperty(peerLevelKey, level, nullptr);
    item.setProperty(peerNetbufKey, netbuf, nullptr);
    item.setProperty(peerNetbufAutoKey, netbufauto, nullptr);
    item.setProperty(peerSendFormatKey, sendFormat, nullptr);
    
    item.appendChild(compressorParams.getValueTree(compressorStateKey), nullptr);  

    return item;
}

void SonobusAudioProcessor::PeerStateCache::setFromValueTree(const ValueTree & item)
{    
    name = item.getProperty(peerNameKey, name);
    monopan = item.getProperty(peerMonoPanKey, monopan);
    stereopan1 = item.getProperty(peerPan1Key, stereopan1);
    stereopan2 = item.getProperty(peerPan2Key, stereopan2);
    level = item.getProperty(peerLevelKey, level);
    netbuf = item.getProperty(peerNetbufKey, netbuf);
    netbufauto = item.getProperty(peerNetbufAutoKey, netbufauto);
    sendFormat = item.getProperty(peerSendFormatKey, sendFormat);

    ValueTree compressorTree = item.getChildWithName(compressorStateKey);
    if (compressorTree.isValid()) {
        compressorParams.setFromValueTree(compressorTree);
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

bool SonobusAudioProcessor::startRecordingToFile(const File & file)
{
    if (!recordingThread) {
        recordingThread = std::make_unique<TimeSliceThread>("Recording Thread");
        recordingThread->startThread();
        
        writingPossible = true;
    }
    
    stopRecordingToFile();

    bool ret = false;
    
    if (getSampleRate() > 0)
    {
        // Create an OutputStream to write to our destination file...
        file.deleteFile();
        
        if (auto fileStream = std::unique_ptr<FileOutputStream> (file.createOutputStream()))
        {
            // Now create a WAV writer object that writes to our output stream...
            //WavAudioFormat audioFormat;
            std::unique_ptr<AudioFormat> audioFormat;
            int qualindex = 0;
            
            if (file.getFileExtension().toLowerCase() == ".flac") {
                audioFormat = std::make_unique<FlacAudioFormat>();
            }
            else if (file.getFileExtension().toLowerCase() == ".wav") {
                audioFormat = std::make_unique<WavAudioFormat>();
            }
            else if (file.getFileExtension().toLowerCase() == ".ogg") {
                audioFormat = std::make_unique<OggVorbisAudioFormat>();
                qualindex = 8; // 256k
            }
            else {
                DBG("Could not find format for filename");
                return false;
            }
            
            if (auto writer = audioFormat->createWriterFor (fileStream.get(), getSampleRate(), getMainBusNumOutputChannels(), 16, {}, qualindex))
            {
                fileStream.release(); // (passes responsibility for deleting the stream to the writer object that is now using it)
                
                // Now we'll create one of these helper objects which will act as a FIFO buffer, and will
                // write the data to disk on our background thread.
                threadedWriter.reset (new AudioFormatWriter::ThreadedWriter (writer, *recordingThread, 32768));
                                
                // And now, swap over our active writer pointer so that the audio callback will start using it..
                const ScopedLock sl (writerLock);
                mElapsedRecordSamples = 0;
                activeWriter = threadedWriter.get();
                DBG("Started recording file " << file.getFullPathName());
                ret = true;
            } else {
                DBG("Error creating writer for " << file.getFullPathName());
            }
        } else {
            DBG("Error creating output file: " << file.getFullPathName());
        }
    }
    
    return ret;
}

bool SonobusAudioProcessor::stopRecordingToFile()
{
    // First, clear this pointer to stop the audio callback from using our writer object..
    {
        const ScopedLock sl (writerLock);
        activeWriter = nullptr;
    }
    
    if (threadedWriter) {
        
        // Now we can delete the writer object. It's done in this order because the deletion could
        // take a little time while remaining data gets flushed to disk, and we can't be blocking
        // the audio callback while this happens.
        threadedWriter.reset();
        
        DBG("Stopped recording file");
        return true;
    }
    else {
        return false;
    }
}

bool SonobusAudioProcessor::isRecordingToFile()
{
    return activeWriter.load() != nullptr;
}

 bool SonobusAudioProcessor::loadURLIntoTransport (const URL& audioURL)
{
    if (!mDiskThread.isThreadRunning()) {
        mDiskThread.startThread (3);
    }

    // unload the previous file source and delete it..
    mTransportSource.stop();
    mTransportSource.setSource (nullptr);
    mCurrentAudioFileSource.reset();
    
    AudioFormatReader* reader = nullptr;
    
#if ! JUCE_IOS
    if (audioURL.isLocalFile())
    {
        reader = mFormatManager.createReaderFor (audioURL.getLocalFile());
    }
    else
#endif
    {
        if (reader == nullptr)
            reader = mFormatManager.createReaderFor (audioURL.createInputStream (false));
    }
    
    if (reader != nullptr)
    {
        mCurrentAudioFileSource.reset (new AudioFormatReaderSource (reader, true));

        mTransportSource.prepareToPlay(currSamplesPerBlock, getSampleRate());

        // ..and plug it into our transport source
        mTransportSource.setSource (mCurrentAudioFileSource.get(),
                                   32768,                   // tells it to buffer this many samples ahead
                                   &mDiskThread,                 // this is the background thread to use for reading-ahead
                                   reader->sampleRate);     // allows for sample rate correction
        

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


void SonobusAudioProcessor::setMainReverbModel(ReverbModel flag) 
{ 
    mMainReverbModel = flag; 
    mState.getParameter(paramMainReverbModel)->setValueNotifyingHost(mState.getParameter(paramMainReverbModel)->convertTo0to1(flag));
}


//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SonobusAudioProcessor();
}
