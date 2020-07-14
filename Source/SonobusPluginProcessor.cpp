

#include "SonobusPluginProcessor.h"
#include "SonobusPluginEditor.h"

#include "RunCumulantor.h"

#include "DebugLogC.h"

#include "aoo/aoo_net.h"
#include "aoo/aoo_pcm.h"
#include "aoo/aoo_opus.h"

#include "mtdm.h"

#ifdef _WIN32
#include <winsock2.h>
typedef int socklen_t;
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

String SonobusAudioProcessor::paramInGain     ("ingain");
String SonobusAudioProcessor::paramDry     ("dry");
String SonobusAudioProcessor::paramInMonitorPan1     ("inmonpan1");
String SonobusAudioProcessor::paramInMonitorPan2     ("inmonpan2");
String SonobusAudioProcessor::paramWet     ("wet");
String SonobusAudioProcessor::paramBufferTime  ("buffertime");

static String recentsCollectionKey("RecentConnections");
static String recentsItemKey("ServerConnectionInfo");


#define METER_RMS_SEC 0.04
#define MAX_PANNERS 4

// get sockaddr, IPv4 or IPv6:
static void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return &(((struct sockaddr_in*)sa)->sin_addr);
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

static in_port_t get_in_port(struct sockaddr *sa)
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
    
    float gain = 1.0f;
    float buffertimeMs = 15.0f;
    AutoNetBufferMode  autosizeBufferMode = AutoNetBufferModeAutoFull;
    bool sendActive = false;
    bool recvActive = false;
    bool sendAllow = true;
    bool recvAllow = true;
    bool recvMuted = false;
    bool invitedPeer = false;
    int  formatIndex = -1; // default
    int packetsize = 512;
    int sendChannels = 2;
    int recvChannels = 0;
    float recvPan[MAX_PANNERS];
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
    float totalEstLatency = 0.0f;
    float totalLatency = 0.0f;
    bool hasRealLatency = false;
    bool latencyDirty = false;
    AudioSampleBuffer workBuffer;
    float recvPanLast[MAX_PANNERS];
    // metering
    foleys::LevelMeterSource sendMeterSource;
    foleys::LevelMeterSource recvMeterSource;

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
        endpoint->sentBytes += result;
    }
    else if (result < 0) {
        DebugLogC("Error sending %d bytes to endpoint %s", endpoint->ipaddr.toRawUTF8());
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
        endpoint->sentBytes += result;
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
        DebugLogC("Send thread finishing");
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

        DebugLogC("Recv thread finishing");        
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
        
        DebugLogC("Event thread finishing");
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
        
        DebugLogC("Server thread finishing");        
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
        
        DebugLogC("Client thread finishing");        
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
    std::make_unique<AudioParameterFloat>(paramInGain,     TRANS ("In Gain"),    NormalisableRange<float>(0.0, 1.0, 0.0, 0.25), mInGain.get(), "", AudioProcessorParameter::genericParameter, 
                                          [](float v, int maxlen) -> String { return Decibels::toString(Decibels::gainToDecibels(v), 1); }, 
                                          [](const String& s) -> float { return Decibels::decibelsToGain(s.getFloatValue()); }),

    std::make_unique<AudioParameterFloat>(paramInMonitorPan1,     TRANS ("In Mon Pan 1"),    NormalisableRange<float>(-1.0, 1.0, 0.0), mInMonPan1.get(), "", AudioProcessorParameter::genericParameter, 
                                          [](float v, int maxlen) -> String { if (fabs(v) < 0.01) return TRANS("C"); return String((int)rint(abs(v*100.0f))) + ((v > 0 ? "% R" : "% L")) ; },
                                          [](const String& s) -> float { return s.getFloatValue()*1e-2f; }),

    std::make_unique<AudioParameterFloat>(paramInMonitorPan2,     TRANS ("In Mon Pan 2"),    NormalisableRange<float>(-1.0, 1.0, 0.0), mInMonPan2.get(), "", AudioProcessorParameter::genericParameter, 
                                          [](float v, int maxlen) -> String { if (fabs(v) < 0.01) return TRANS("C"); return String((int)rint(abs(v*100.0f))) + ((v > 0 ? "% R" : "% L")) ; },
                                          [](const String& s) -> float { return s.getFloatValue()*1e-2f; }),

    std::make_unique<AudioParameterFloat>(paramDry,     TRANS ("Dry Level"),    NormalisableRange<float>(0.0,    1.0, 0.0, 0.25), mDry.get(), "", AudioProcessorParameter::genericParameter, 
                                          [](float v, int maxlen) -> String { return Decibels::toString(Decibels::gainToDecibels(v), 1); }, 
                                          [](const String& s) -> float { return Decibels::decibelsToGain(s.getFloatValue()); }),

    std::make_unique<AudioParameterFloat>(paramWet,     TRANS ("Output Level"),    NormalisableRange<float>(0.0, 1.0, 0.0, 0.25), mWet.get(), "", AudioProcessorParameter::genericParameter, 
                                          [](float v, int maxlen) -> String { return Decibels::toString(Decibels::gainToDecibels(v), 1); }, 
                                          [](const String& s) -> float { return Decibels::decibelsToGain(s.getFloatValue()); }),

    std::make_unique<AudioParameterFloat>(paramBufferTime,     TRANS ("Buffer Time"),    NormalisableRange<float>(0.0, mMaxBufferTime.get(), 0.001, 0.5), mBufferTime.get(), "", AudioProcessorParameter::genericParameter, 
                                          [](float v, int maxlen) -> String { return String(v*1000.0) + " ms"; }, 
                                          [](const String& s) -> float { return s.getFloatValue()*1e-3f; })
})
{
    mState.addParameterListener (paramInGain, this);
    mState.addParameterListener (paramDry, this);
    mState.addParameterListener (paramWet, this);
    mState.addParameterListener (paramBufferTime, this);
    mState.addParameterListener (paramInMonitorPan1, this);
    mState.addParameterListener (paramInMonitorPan2, this);

    for (int i=0; i < MAX_PEERS; ++i) {
        for (int j=0; j < MAX_PEERS; ++j) {
            mRemoteSendMatrix[i][j] = false;
        }
    }
    
    initializeAoo();
}

SonobusAudioProcessor::~SonobusAudioProcessor()
{
    cleanupAoo();
}

void SonobusAudioProcessor::initializeAoo()
{
    int udpport = 11000;
    // we have both an AOO source and sink
        
    const int echoId = 12321;
    
    aoo_initialize();
    
    initFormats();

    const ScopedWriteLock sl (mCoreLock);        

    //mAooSink.reset(aoo::isink::create(1));

    mAooDummySource.reset(aoo::isource::create(0));



    
    //mAooSink->set_buffersize(mBufferTime.get()); // ms
    //mAooSource->set_buffersize(mBufferTime.get()); // ms
    
    //mAooSink->set_packetsize(1024);
    //mAooSource->set_packetsize(1024);
    

    
    mUdpSocket = std::make_unique<DatagramSocket>();

    int attempts = 100;
    while (attempts > 0) {
        if (mUdpSocket->bindToPort(udpport)) {
            DebugLogC("Bound udp port to %d", udpport);
            break;
        }
        ++udpport;
        --attempts;
    }
    
    if (attempts <= 0) {
        DebugLogC("COULD NOT BIND TO PORT!");
        udpport = 0;
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
    DebugLogC("waiting on recv thread to die");
    mRecvThread->stopThread(400);
    DebugLogC("waiting on send thread to die");
    mSendThread->stopThread(400);
    DebugLogC("waiting on event thread to die");
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
            DebugLogC("Error creating Aoo Server: %d : %s", err);
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
        DebugLogC("waiting on recv thread to die");
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
        DebugLogC("Error connecting to server: %d", retval);
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


void SonobusAudioProcessor::setAutoconnectToGroupPeers(bool flag)
{
    mAutoconnectGroupPeers = flag;
}


bool SonobusAudioProcessor::joinServerGroup(const String & group, const String & groupsecret)
{
    if (!mAooClient) return false;

    int32_t retval = mAooClient->group_join(group.toRawUTF8(), groupsecret.toRawUTF8());
    
    if (retval < 0) {
        DebugLogC("Error joining group %s: %d", group.toRawUTF8(), retval);
    }
    
    return retval >= 0;
}

bool SonobusAudioProcessor::leaveServerGroup(const String & group)
{
    if (!mAooClient) return false;

    int32_t retval = mAooClient->group_leave(group.toRawUTF8());
    
    if (retval < 0) {
        DebugLogC("Error leaving group %s: %d", group.toRawUTF8(), retval);
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


void SonobusAudioProcessor::initFormats()
{
    mAudioFormats.clear();
    
    // low bandwidth to high
    mAudioFormats.add(AudioCodecFormatInfo("8 kbps/ch",  8000, 10, OPUS_SIGNAL_MUSIC, 960));
    mAudioFormats.add(AudioCodecFormatInfo("16 kbps/ch",  16000, 10, OPUS_SIGNAL_MUSIC, 960));
    mAudioFormats.add(AudioCodecFormatInfo("24 kbps/ch", 24000, 10, OPUS_SIGNAL_MUSIC, 480));
    mAudioFormats.add(AudioCodecFormatInfo("48 kbps/ch",  48000, 10, OPUS_SIGNAL_MUSIC, 240));
    mAudioFormats.add(AudioCodecFormatInfo("64 kbps/ch",  64000, 10, OPUS_SIGNAL_MUSIC, 240));
    mAudioFormats.add(AudioCodecFormatInfo("96 kbps/ch",  96000, 10, OPUS_SIGNAL_MUSIC, 120));
    mAudioFormats.add(AudioCodecFormatInfo("128 kbps/ch",  128000, 10, OPUS_SIGNAL_MUSIC, 120));
    mAudioFormats.add(AudioCodecFormatInfo("160 kbps/ch",  160000, 10, OPUS_SIGNAL_MUSIC, 120));
    mAudioFormats.add(AudioCodecFormatInfo("256 kbps/ch",  256000, 10, OPUS_SIGNAL_MUSIC, 120));
    
    mAudioFormats.add(AudioCodecFormatInfo("PCM 16 bit", 2));
    mAudioFormats.add(AudioCodecFormatInfo("PCM 24 bit", 3));
    mAudioFormats.add(AudioCodecFormatInfo("PCM 32 bit float", 4));
    //mAudioFormats.add(AudioCodecFormatInfo(CodecPCM, 8));

    mDefaultAudioFormatIndex = 5; // 96kpbs/ch Opus
}


String SonobusAudioProcessor::getAudioCodeFormatName(int formatIndex) const
{
    if (formatIndex >= mAudioFormats.size()) return "";
    
    const AudioCodecFormatInfo & info = mAudioFormats.getReference(formatIndex);
    return info.name;    
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

    DebugLogC("Setting packet size to %d", psize);
    
    auto remote = mRemotePeers.getUnchecked(index);
    remote->packetsize = psize;
    
    if (remote->oursource) {
        //setupSourceFormat(remote, remote->oursource.get());

        //remote->oursource->setup(getSampleRate(), remote->packetsize, getTotalNumInputChannels());

        remote->oursource->set_packetsize(remote->packetsize);
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
        DebugLogC("Error converting raw addr to IP");
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
        DebugLogC("Added new endpoint for %s:%d", host.toRawUTF8(), port);
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
        DebugLogC("Error receiving UDP");
        return;
    }
    
    // find endpoint from sender info
    EndpointState * endpoint = findOrAddEndpoint(senderIP, senderPort);
    
    endpoint->recvBytes += nbytes;
    
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
                    
                    if (id == AOO_ID_WILDCARD || (remote->oursink->get_id(dummyid) && id == dummyid) ) {
                        remote->oursink->handle_message(buf, nbytes, endpoint, endpoint_send);
                        remote->dataPacketsReceived += 1;
                        
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

                //DebugLogC("Got AOO_CLIENT or PEER data");

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
                DebugLogC("Got AOO_SERVER data");

                if (mAooServer) {
                    // mAooServer->handle_message(buf, nbytes, endpoint);
                }
                
            } else {
                DebugLogC("SonoBus bug: unknown aoo type: %d", type);
            }
        }

        // notify send thread
        notifySendThread();

    } 
    else {
        // not a valid AoO OSC message
        DebugLogC("SonoBus: not a valid AOO message!");
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
            DebugLogC("ping to source %d recvd from %s:%d -- %g %g %g", sourceId, es->ipaddr.toRawUTF8(), es->port, diff1, diff2, rtt);
            
            RemotePeer * peer = findRemotePeer(es, sourceId);
            if (peer) {
                const ScopedReadLock sl (mCoreLock);        

                // smooth it
                peer->pingTime = rtt; // * 0.5;
                if (rtt < 1000.0) {
                    peer->smoothPingTime.Z *= 0.9;
                    peer->smoothPingTime.push(peer->pingTime);
                }
                peer->totalEstLatency =  peer->smoothPingTime.xbar + 2*peer->buffertimeMs + (1e3*currSamplesPerBlock/getSampleRate());

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
                                peer->buffertimeMs -= 2;
                                
                                peer->buffertimeMs = std::max(peer->buffertimeMs, peer->netBufAutoBaseline);
                                
                                peer->totalEstLatency = peer->smoothPingTime.xbar + 2*peer->buffertimeMs + (1e3*currSamplesPerBlock/getSampleRate());
                                peer->oursink->set_buffersize(peer->buffertimeMs);
                                peer->echosink->set_buffersize(peer->buffertimeMs);
                                peer->latencysink->set_buffersize(peer->buffertimeMs);
                                peer->latencyDirty = true;

                                DebugLogC("AUTO-Decreasing buffer time by 2 ms to %d ", (int) peer->buffertimeMs);

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
                        DebugLogC("Already had remote peer for %s:%d  ourId: %d", es->ipaddr.toRawUTF8(), es->port, peer->ourId);
                    } else {
                        peer = doAddRemotePeerIfNecessary(es);
                    }

                    const ScopedReadLock sl (mCoreLock);        

                    peer->remoteSinkId = e->id;

                    // add their sink
                    if (peer->sendAllow) {
                        peer->oursource->add_sink(es, peer->remoteSinkId, endpoint_send);
                        peer->oursource->start();
                        peer->sendActive = true;
                    }
                    
                    DebugLogC("Was invited by remote peer %s:%d sourceId: %d  ourId: %d", es->ipaddr.toRawUTF8(), es->port, peer->remoteSinkId,  peer->ourId);

                    // now try to invite them back at their port , with the same ID, they 
                    // should have a source waiting for us with the same id

                    DebugLogC("Inviting them back to our sink")
                    peer->remoteSourceId = peer->remoteSinkId;
                    peer->oursink->invite_source(es, peer->remoteSourceId, endpoint_send);

                    // now remove dummy handshake one
                    mAooDummySource->remove_sink(es, dummyid);
                    
                }
                else {
                    // invited 
                    DebugLogC("Invite received to our source: %d   from %s:%d  %d", sourceId, es->ipaddr.toRawUTF8(), es->port, e->id);

                    RemotePeer * peer = findRemotePeer(es, sourceId);
                    if (peer) {
                        
                        peer->remoteSinkId = e->id;

                        peer->oursource->add_sink(es, peer->remoteSinkId, endpoint_send);
                        
                        if (peer->sendAllow) {
                            peer->oursource->start();
                            
                            peer->sendActive = true;
                            DebugLogC("Starting to send, we allow it", es->ipaddr.toRawUTF8(), es->port, peer->remoteSinkId);
                        }
                        
                        peer->connected = true;
                        
                        DebugLogC("Finishing peer connection for %s:%d  %d", es->ipaddr.toRawUTF8(), es->port, peer->remoteSinkId);
                        
                    }
                    else {
                        // find by echo id
                        if (auto * echopeer = findRemotePeerByEchoId(es, sourceId)) {
                            echopeer->echosource->add_sink(es, e->id, endpoint_send);                            
                            echopeer->echosource->start();
                            DebugLogC("Invite to echo source adding sink %d", e->id);
                        }
                        else if (auto * latpeer = findRemotePeerByLatencyId(es, sourceId)) {
                            echopeer->latencysource->add_sink(es, e->id, endpoint_send);                                                        
                            echopeer->latencysource->start();
                            DebugLogC("Invite to our latency source adding sink %d", e->id);
                        }
                        else {
                            // not one of our sources 
                            DebugLogC("No source %d invited, how is this possible?", sourceId);
                        }

                    }
                }
                
            } else {
                DebugLogC("Invite received");
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
                    
                    DebugLogC("Uninvited, removed remote peer %s:%d", es->ipaddr.toRawUTF8(), es->port);
                    
                }
                else if (auto * echopeer = findRemotePeerByEchoId(es, sourceId)) {
                    echopeer->echosource->remove_sink(es, e->id);                            
                    echopeer->echosource->stop();
                    DebugLogC("UnInvite to echo source adding sink %d", e->id);
                }
                else if (auto * latpeer = findRemotePeerByLatencyId(es, sourceId)) {
                    echopeer->latencysource->remove_sink(es, e->id);
                    echopeer->latencysource->stop();
                    DebugLogC("UnInvite to latency source adding sink %d", e->id);
                }

                else {
                    DebugLogC("Uninvite received to unknown");
                }

            } else {
                DebugLogC("Uninvite received");
            }

            break;
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
                    DebugLogC("Got dummy handshake add from %s:%d", es->ipaddr.toRawUTF8(), es->port);
                }
                else {
                    DebugLogC("Added source %s:%d  %d  to our %d", es->ipaddr.toRawUTF8(), es->port, e->id, sinkId);
                    peer->remoteSourceId = e->id;
                    
                    peer->oursink->uninvite_source(es, 0, endpoint_send); // get rid of existing bogus one

                    if (peer->recvAllow) {
                        peer->oursink->invite_source(es, peer->remoteSourceId, endpoint_send);
                        peer->recvActive = true;
                    } else {
                        DebugLogC("we aren't accepting recv right now, politely decline it")
                        peer->oursink->uninvite_source(es, peer->remoteSourceId, endpoint_send);
                        peer->recvActive = false;                        
                    }
                    
                    peer->connected = true;
                }
                
                // do invite here?
                
            }
            else {
                DebugLogC("Added source to unknown %d", e->id);
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
                    DebugLogC("Got source format event from %s:%d  %d   channels: %d", es->ipaddr.toRawUTF8(), es->port, e->id, f.header.nchannels);
                    peer->recvMeterSource.resize(f.header.nchannels, meterRmsWindow);
                    if (peer->recvChannels != f.header.nchannels) {
                        peer->recvChannels = std::min(MAX_PANNERS, f.header.nchannels);
                        if (peer->recvChannels == 1) {
                            // center pan
                            peer->recvPan[0] = 0.0f;
                        } else if (peer->recvChannels == 2) {
                            // Left/Right
                            peer->recvPan[0] = -1.0f;
                            peer->recvPan[1] = 1.0f;
                        } else if (peer->recvChannels > 2) {
                            peer->recvPan[0] = -1.0f;
                            peer->recvPan[1] = 1.0f;
                            for (int i=2; i < peer->recvChannels; ++i) {
                                peer->recvPan[i] = 0.0f;
                            }
                        }
                    }
                    
                    clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientPeerChangedState, this, "format");
                }                
            }
            else { 
                DebugLogC("format event to unknown %d", e->id);
                
            }
            
            break;
        }
        case AOO_SOURCE_STATE_EVENT:
        {
            aoo_source_state_event *e = (aoo_source_state_event *)events[i];
            EndpointState * es = (EndpointState *)e->endpoint;

            DebugLogC("Got source state event from %s:%d -- %d", es->ipaddr.toRawUTF8(), es->port, e->state);

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

            DebugLogC("Got source block lost event from %s:%d  %d -- %d", es->ipaddr.toRawUTF8(), es->port, e->id, e->count);

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
                                peer->buffertimeMs += 2;
                                peer->totalEstLatency = peer->smoothPingTime.xbar + 2*peer->buffertimeMs + (1e3*currSamplesPerBlock/getSampleRate());
                                peer->oursink->set_buffersize(peer->buffertimeMs);
                                peer->echosink->set_buffersize(peer->buffertimeMs);
                                peer->latencysink->set_buffersize(peer->buffertimeMs);
                                peer->latencyDirty = true;

                                DebugLogC("AUTO-Increasing buffer time by 1 ms to %d  droprate: %g", (int) peer->buffertimeMs, droprate);

                                if (peer->autosizeBufferMode == AutoNetBufferModeAutoFull) {

                                    const float timesincedecrthresh = 2.0;
                                    if (peer->lastNetBufDecrTime > 0 && (nowtime - peer->lastNetBufDecrTime)*1e-3 < timesincedecrthresh ) {
                                        peer->netBufAutoBaseline = peer->buffertimeMs;
                                        DebugLogC("Got drop within short time thresh, setting minimum baseline for future decr to %g", peer->netBufAutoBaseline);
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

            DebugLogC("Got source block reordered event from %s:%d  %d-- %d", es->ipaddr.toRawUTF8(), es->port, e->id,e->count);

            break;
        }
        case AOO_BLOCK_RESENT_EVENT:
        {
            aoo_block_resent_event *e = (aoo_block_resent_event *)events[i];
            EndpointState * es = (EndpointState *)e->endpoint;

            DebugLogC("Got source block resent event from %s:%d  %d-- %d", es->ipaddr.toRawUTF8(), es->port, e->id, e->count);
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

            DebugLogC("Got source block gap event from %s:%d  %d-- %d", es->ipaddr.toRawUTF8(), es->port, e->id, e->count);

            break;
        }
        case AOO_PING_EVENT:
        {
            aoo_ping_event *e = (aoo_ping_event *)events[i];
            EndpointState * es = (EndpointState *)e->endpoint;


            double diff = aoo_osctime_duration(e->tt1, e->tt2) * 1000.0;
            DebugLogC("Got source block ping event from %s:%d  %d -- diff %g", es->ipaddr.toRawUTF8(), es->port, e->id, diff);

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
                
                DebugLogC("Server - User joined: %s", e->name);
                
                break;
            }
            case AOONET_SERVER_USER_LEAVE_EVENT:
            {
                aoonet_server_user_event *e = (aoonet_server_user_event *)events[i];
                
                DebugLogC("Server - User left: %s", e->name);
                
                
                break;
            }
            case AOONET_SERVER_GROUP_JOIN_EVENT:
            {
                aoonet_server_group_event *e = (aoonet_server_group_event *)events[i];
                
                DebugLogC("Server - Group Joined: %s  by user: %s", e->group, e->user);
                
                break;
            }
            case AOONET_SERVER_GROUP_LEAVE_EVENT:
            {
                aoonet_server_group_event *e = (aoonet_server_group_event *)events[i];
                
                DebugLogC("Server - Group Left: %s  by user: %s", e->group, e->user);
                
                break;
            }
            case AOONET_SERVER_ERROR_EVENT:
            {
                aoonet_server_event *e = (aoonet_server_event *)events[i];
                
                DebugLogC("Server error: %s", e->errormsg);
                
                break;
            }
            default:
                DebugLogC("Got unknown server event: %d", events[i]->type);
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
                DebugLogC("Connected to server!");
                mIsConnectedToServer = true;
            } else {
                DebugLogC("Couldn't connect to server - %s!", e->errormsg);
                mIsConnectedToServer = false;
            }
            
            clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientConnected, this, e->result > 0, e->errormsg);
            
            break;
        }
        case AOONET_CLIENT_DISCONNECT_EVENT:
        {
            aoonet_client_group_event *e = (aoonet_client_group_event *)events[i];
            if (e->result == 0){
                DebugLogC("Disconnected from server - %s!", e->errormsg);
            }

            removeAllRemotePeers();
            mIsConnectedToServer = false;

            clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientDisconnected, this, e->result > 0, e->errormsg);

            break;
        }
        case AOONET_CLIENT_GROUP_JOIN_EVENT:
        {
            aoonet_client_group_event *e = (aoonet_client_group_event *)events[i];
            if (e->result > 0){
                DebugLogC("Joined group - %s", e->name);
                const ScopedLock sl (mClientLock);        
                mCurrentJoinedGroup = e->name;
            } else {
                DebugLogC("Couldn't join group %s - %s!", e->name, e->errormsg);
            }
            clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientGroupJoined, this, e->result > 0, e->name, e->errormsg);
            break;
        }
        case AOONET_CLIENT_GROUP_LEAVE_EVENT:
        {
            aoonet_client_group_event *e = (aoonet_client_group_event *)events[i];
            if (e->result > 0){

                DebugLogC("Group leave - %s", e->name);

                const ScopedLock sl (mClientLock);        
                mCurrentJoinedGroup.clear();

                // assume they are all part of the group, XXX
                removeAllRemotePeers();

                //aoo_node_remove_group(x->x_node, gensym(e->name));
                

            } else {
                DebugLogC("Couldn't leave group %s - %s!", e->name, e->errormsg);
            }

            clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientGroupLeft, this, e->result > 0, e->name, e->errormsg);

            break;
        }
        case AOONET_CLIENT_PEER_JOIN_EVENT:
        {
            aoonet_client_peer_event *e = (aoonet_client_peer_event *)events[i];

            if (e->result > 0){
                DebugLogC("Peer joined group %s - user %s", e->group, e->user);

                if (mAutoconnectGroupPeers) {
                    connectRemotePeerRaw(e->address, e->user, e->group);
                }
                
                //aoo_node_add_peer(x->x_node, gensym(e->group), gensym(e->user),
                //                  (const struct sockaddr *)e->address, e->length);

                clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientPeerJoined, this, e->group, e->user);
                
            } else {
                DebugLogC("bug bad result on join event");
            }
            

            break;
        }
        case AOONET_CLIENT_PEER_LEAVE_EVENT:
        {
            aoonet_client_peer_event *e = (aoonet_client_peer_event *)events[i];

            if (e->result > 0){

                DebugLogC("Peer leave group %s - user %s", e->group, e->user);

                EndpointState * endpoint = findOrAddRawEndpoint(e->address);
                if (endpoint) {
                    
                    removeAllRemotePeersWithEndpoint(endpoint);
                }
                
                //aoo_node_remove_peer(x->x_node, gensym(e->group), gensym(e->user));
                clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientPeerLeft, this, e->group, e->user);

            } else {
                DebugLogC("bug bad result on leave event");
            }
            break;
        }
        case AOONET_CLIENT_ERROR_EVENT:
        {
            aoonet_client_event *e = (aoonet_client_event *)events[i];
            DebugLogC("client error: %s", e->errormsg);
            clientListeners.call(&SonobusAudioProcessor::ClientListener::aooClientError, this, e->errormsg);
            break;
        }
        default:
            DebugLogC("Got unknown client event: %d", events[i]->type);
            break;
        }
    }
    return 1;
}


int SonobusAudioProcessor::connectRemotePeerRaw(void * sockaddr, const String & username, const String & groupname, bool reciprocate)
{
    EndpointState * endpoint = findOrAddRawEndpoint(sockaddr);
    
    if (!endpoint) {
        DebugLogC("Error getting endpoint from raw address");
        return 0;
    }
    
    RemotePeer * remote = doAddRemotePeerIfNecessary(endpoint); // get new one

    remote->recvAllow = true;
    remote->userName = username;
    remote->groupName = groupname;
    
    // special - use 0 
    bool ret = remote->oursink->invite_source(endpoint, 0, endpoint_send) == 1;
    
    if (ret) {
        DebugLogC("Successfully invited remote peer at %s:%d - ourId %d", endpoint->ipaddr.toRawUTF8(), endpoint->port, remote->ourId);
        remote->connected = true;
        remote->invitedPeer = reciprocate;
        remote->recvActive = reciprocate;
        remote->sendActive = true;
        remote->oursource->start();
        
    } else {
        DebugLogC("Error inviting remote peer at %s:%d - ourId %d", endpoint->ipaddr.toRawUTF8(), endpoint->port, remote->ourId);
    }

    return ret;    
}

int SonobusAudioProcessor::connectRemotePeer(const String & host, int port, const String & username, const String & groupname, bool reciprocate)
{
    EndpointState * endpoint = findOrAddEndpoint(host, port);

    RemotePeer * remote = doAddRemotePeerIfNecessary(endpoint); // get new one

    remote->recvAllow = true;
    remote->userName = username;
    remote->groupName = groupname;

    // special - use 0 
    bool ret = remote->oursink->invite_source(endpoint, 0, endpoint_send) == 1;
    
    if (ret) {
        DebugLogC("Successfully invited remote peer at %s:%d - ourId %d", host.toRawUTF8(), port, remote->ourId);
        remote->connected = true;
        remote->invitedPeer = reciprocate;
        remote->recvActive = reciprocate;
        remote->sendActive = true;
        remote->oursource->start();
        
    } else {
        DebugLogC("Error inviting remote peer at %s:%d - ourId %d", host.toRawUTF8(), port, remote->ourId);
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
                DebugLogC("uninviting all remote source %d", remote->remoteSourceId);
                ret = remote->oursink->uninvite_all();
                //ret = remote->oursink->uninvite_source(endpoint, remote->remoteSourceId, endpoint_send) == 1;
            }
         
            // if we auto-invited the other end's remote source, remove that as a dest sink
            if (remote->oursource) {
                DebugLogC("removing all remote sink %d", remote->remoteSinkId);
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
        DebugLogC("Successfully disconnected remote peer %d  at %s:%d", ourId, host.toRawUTF8(), port);
    } else {
        DebugLogC("Error disconnecting remote peer %d at %s:%d", ourId, host.toRawUTF8(), port);
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
                DebugLogC("uninviting all remote source %d", remote->remoteSourceId);
                ret = remote->oursink->uninvite_all();
            }

            // if we auto-invited the other end's remote source, remove that as a dest sink
            if (remote->oursource && remote->remoteSinkId >= 0) {
                DebugLogC("removing all remote sink %d", remote->remoteSinkId);
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
            int newchancnt = isAnythingRoutedToPeer(destindex) ? getTotalNumOutputChannels() : getTotalNumInputChannels();

            if (peer->sendChannels != newchancnt) {
                peer->sendChannels = newchancnt;
                DebugLogC("Peer %d  has new sendChannel count: %d", peer->sendChannels);
                if (peer->oursource) {
                    setupSourceFormat(peer, peer->oursource.get());
                    peer->oursource->setup(getSampleRate(), currSamplesPerBlock, peer->sendChannels);
                }
            }
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
            remote->recvPan[chan] = pan;
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
            pan = remote->recvPan[chan];
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
        remote->netBufAutoBaseline = 0.0f; // reset
        remote->latencyDirty = true;
    }
}

float SonobusAudioProcessor::getRemotePeerBufferTime(int index) const
{
    float buftimeMs = 30.0f;
    
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        buftimeMs = remote->buffertimeMs;
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
            remote->netBufAutoBaseline = 0.0; // reset
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


void SonobusAudioProcessor::setRemotePeerRecvActive(int index, bool active)
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        remote->recvActive = active;

        if (remote->recvActive) {
            remote->recvAllow = true;
        }
        
        // TODO
#if 1
        if (active) {
            DebugLogC("inviting peer %d source %d", remote->ourId, remote->remoteSourceId);
            remote->oursink->invite_source(remote->endpoint,remote->remoteSourceId, endpoint_send);
        } else {
            DebugLogC("uninviting peer %d source %d", remote->ourId, remote->remoteSourceId);
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

void SonobusAudioProcessor::setRemotePeerSendAllow(int index, bool allow)
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        remote->sendAllow = allow;
        
        if (!allow) {
            //remote->oursource->remove_all();
            setRemotePeerSendActive(index, allow);
        }
    }
}

bool SonobusAudioProcessor::getRemotePeerSendAllow(int index) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return remote->sendAllow;
    }
    return false;        
}

void SonobusAudioProcessor::setRemotePeerRecvAllow(int index, bool allow)
{
     const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        remote->recvAllow = allow;

        if (!allow) {
            setRemotePeerRecvActive(index, allow);
        }
    }
}

bool SonobusAudioProcessor::getRemotePeerRecvAllow(int index) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return remote->recvAllow;
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


float SonobusAudioProcessor::getRemotePeerPingMs(int index) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return remote->smoothPingTime.xbar;
    }
    return 0;          
}

float SonobusAudioProcessor::getRemotePeerTotalLatencyMs(int index, bool & estimated) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        if (remote->activeLatencyTest && remote->latencyProcessor) {
            if (remote->latencyProcessor->resolve() < 0) {
                DebugLogC("Latency Signal below threshold...");

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

                DebugLogC("Peer %d:  %10.3lf frames %8.3lf ms   - %s", d, d * t, quest ? "???" : (inv ? "Inv" : ""));

                remote->totalLatency = d*t;
                remote->hasRealLatency = true;
                remote->latencyDirty = false;
                estimated = false;
                return remote->totalLatency;
            }
        }

        if (remote->hasRealLatency) {
            estimated = remote->latencyDirty;
            return remote->totalLatency;
        }
        else {
            estimated = true;
            return remote->totalEstLatency ;
        }
    }
    return 0;          
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
            remote->latencysink->invite_source(remote->endpoint, remote->remoteSourceId+ECHO_ID_OFFSET, endpoint_send);
            
            // start our latency source sending to remote's echosink
            remote->latencysource->add_sink(remote->endpoint, remote->remoteSinkId+ECHO_ID_OFFSET, endpoint_send);            
            remote->latencysource->start();
            
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
        if (s->endpoint == endpoint && s->ourId == ourId) {
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



SonobusAudioProcessor::RemotePeer * SonobusAudioProcessor::doAddRemotePeerIfNecessary(EndpointState * endpoint, int32_t ourId)
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

        retpeer->oursink->setup(getSampleRate(), currSamplesPerBlock, getTotalNumOutputChannels());
        retpeer->oursink->set_buffersize(retpeer->buffertimeMs);

        retpeer->sendChannels = getTotalNumInputChannels(); // by default
        
        setupSourceFormat(retpeer, retpeer->oursource.get());
        retpeer->oursource->setup(getSampleRate(), currSamplesPerBlock, retpeer->sendChannels);
        retpeer->oursource->set_packetsize(retpeer->packetsize);

        setupSourceFormat(retpeer, retpeer->latencysource.get(), true);
        retpeer->latencysource->setup(getSampleRate(), currSamplesPerBlock, 1);
        retpeer->latencysource->set_packetsize(retpeer->packetsize);
        setupSourceFormat(retpeer, retpeer->echosource.get(), true);
        retpeer->echosource->setup(getSampleRate(), currSamplesPerBlock, 1);
        retpeer->echosource->set_packetsize(retpeer->packetsize);

        retpeer->latencysink->setup(getSampleRate(), currSamplesPerBlock, 1);
        retpeer->echosink->setup(getSampleRate(), currSamplesPerBlock, 1);

        retpeer->latencysink->set_buffersize(retpeer->buffertimeMs);
        retpeer->echosink->set_buffersize(retpeer->buffertimeMs);

        retpeer->latencyProcessor.reset(new MTDM(getSampleRate()));
        
        retpeer->workBuffer.setSize(2, currSamplesPerBlock);

        int outchannels = getTotalNumOutputChannels();
        
        retpeer->recvMeterSource.resize (outchannels, meterRmsWindow);
        retpeer->sendMeterSource.resize (retpeer->sendChannels, meterRmsWindow);


    }
    else {
        DebugLogC("Remote peer already exists");
    }
    
    return retpeer;    
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

void SonobusAudioProcessor::setupSourceFormat(SonobusAudioProcessor::RemotePeer * peer, aoo::isource * source, bool latencymode)
{
    // have choice and parameters
    
    const AudioCodecFormatInfo & info =  mAudioFormats.getReference((!peer || peer->formatIndex < 0) ? mDefaultAudioFormatIndex : peer->formatIndex);
    
    aoo_format_storage f;
    
    int channels = latencymode ? 1  :  peer ? peer->sendChannels : getTotalNumInputChannels();
    
    if (info.codec == CodecPCM) {
        aoo_format_pcm *fmt = (aoo_format_pcm *)&f;
        fmt->header.codec = AOO_CODEC_PCM;
        fmt->header.blocksize = lastSamplesPerBlock; // peer ? peer->packetsize : lastSamplesPerBlock; // lastSamplesPerBlock;
        fmt->header.samplerate = getSampleRate();
        fmt->header.nchannels = channels;
        fmt->bitdepth = info.bitdepth == 2 ? AOO_PCM_INT16 : info.bitdepth == 3 ? AOO_PCM_INT24 : info.bitdepth == 4 ? AOO_PCM_FLOAT32 : info.bitdepth == 8 ? AOO_PCM_FLOAT64 : AOO_PCM_INT16;
        source->set_format(fmt->header);
    } 
    else if (info.codec == CodecOpus) {
        aoo_format_opus *fmt = (aoo_format_opus *)&f;
        fmt->header.codec = AOO_CODEC_OPUS;
        fmt->header.blocksize = lastSamplesPerBlock >= info.min_preferred_blocksize ? lastSamplesPerBlock : info.min_preferred_blocksize;
        fmt->header.samplerate = getSampleRate();
        fmt->header.nchannels = channels;
        fmt->bitrate = info.bitrate * fmt->header.nchannels;
        fmt->complexity = info.complexity;
        fmt->signal_type = info.signal_type;
        fmt->application_type = OPUS_APPLICATION_RESTRICTED_LOWDELAY;
        //fmt->application_type = OPUS_APPLICATION_AUDIO;
        
        source->set_format(fmt->header);        
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
    else if (parameterID == paramWet) {
        mWet = newValue;
    }
    else if (parameterID == paramInMonitorPan1) {
        mInMonPan1 = newValue;
    }
    else if (parameterID == paramInMonitorPan2) {
        mInMonPan2 = newValue;
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

    DebugLogC("Prepare to play: SR %g  blocksize: %d  inch: %d  outch: %d", sampleRate, samplesPerBlock, getTotalNumInputChannels(), getTotalNumOutputChannels());

    const ScopedReadLock sl (mCoreLock);        
    
    //mAooSource->set_format(fmt->header);
    setupSourceFormat(0, mAooDummySource.get());
    mAooDummySource->setup(sampleRate, samplesPerBlock, getTotalNumInputChannels());

    int inchannels = getTotalNumInputChannels();
    int outchannels = getTotalNumOutputChannels();

    meterRmsWindow = sampleRate * METER_RMS_SEC / currSamplesPerBlock;
    
    inputMeterSource.resize (inchannels, meterRmsWindow);
    outputMeterSource.resize (outchannels, meterRmsWindow);

    if (lastInputChannels != inchannels || lastOutputChannels != outchannels) {
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

        lastInputChannels = inchannels;
        lastOutputChannels = outchannels;
    }
    
    int i=0;
    for (auto s : mRemotePeers) {
        if (s->workBuffer.getNumSamples() < samplesPerBlock) {
            s->workBuffer.setSize(2, samplesPerBlock);
        }

        s->sendChannels = isAnythingRoutedToPeer(i) ? outchannels : inchannels;
        
        if (s->oursource) {
            setupSourceFormat(s, s->oursource.get());
            s->oursource->setup(sampleRate, samplesPerBlock, s->sendChannels);  // todo use inchannels maybe?
        }
        if (s->oursink) {
            s->oursink->setup(sampleRate, samplesPerBlock, outchannels);
        }
        
        if (s->latencysource) {
            setupSourceFormat(s, s->latencysource.get(), true);
            s->latencysource->setup(getSampleRate(), currSamplesPerBlock, 1);
            setupSourceFormat(s, s->echosource.get(), true);
            s->echosource->setup(getSampleRate(), currSamplesPerBlock, 1);

            s->latencysink->setup(sampleRate, samplesPerBlock, 1);
            s->echosink->setup(sampleRate, samplesPerBlock, 1);
            
            s->latencyProcessor.reset(new MTDM(sampleRate));
        }


        s->recvMeterSource.resize (outchannels, meterRmsWindow);
        s->sendMeterSource.resize (s->sendChannels, meterRmsWindow);
        
        ++i;
    }
    
    
    if (tempBuffer.getNumSamples() < samplesPerBlock) {
        tempBuffer.setSize(2, samplesPerBlock);
    }
    if (workBuffer.getNumSamples() < samplesPerBlock) {
        workBuffer.setSize(2, samplesPerBlock);
    }

    
}

void SonobusAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
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

void SonobusAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    float inGain = mInGain.get();
    float drynow = mDry.get(); // DB_CO(dry_level);
    float wetnow = mWet.get(); // DB_CO(wet_level);
    float inmonPan1 = mInMonPan1.get();
    float inmonPan2 = mInMonPan2.get();
    
    int numSamples = buffer.getNumSamples();
    
    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
        buffer.clear (i, 0, numSamples);
    }
    
    tempBuffer.clear(0, numSamples);
    
    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.

    aoo_sample *databufs[totalNumOutputChannels];
    aoo_sample *tmpbufs[totalNumOutputChannels];

    uint64_t t = aoo_osctime_get();

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        databufs[channel] = channelData;        
        tmpbufs[channel] = tempBuffer.getWritePointer(channel);
    }

    // apply input gain
    if (fabsf(inGain - mLastInputGain) > 0.00001) {
        buffer.applyGainRamp(0, numSamples, mLastInputGain, inGain);
    } else {
        buffer.applyGain(inGain);
    }
    
    inputMeterSource.measureBlock (buffer);

    
    bool needsblockchange = false; //lastSamplesPerBlock != numSamples;

    // push data for going out
    {
        const ScopedReadLock sl (mCoreLock);        
        
        //mAooSource->process( buffer.getArrayOfReadPointers(), numSamples, t);
        
        
        tempBuffer.clear();
        
        for (auto & remote : mRemotePeers) 
        {
            
            if (!remote->oursink) continue;
            
            remote->workBuffer.clear(0, numSamples);

            // get audio data coming in from outside into tempbuf
            remote->oursink->process(remote->workBuffer.getArrayOfWritePointers(), numSamples, t);


            
            float usegain = remote->gain;
            
            // we get the stuff, but ignore it TODO smooth
            if (!remote->recvActive) {
                if (remote->_lastgain > 0.0f) {
                    // fade out
                    usegain = 0.0f;
                }
                else {
                    continue;
                }
            } 

            // pre-meter before gain adjust
            remote->recvMeterSource.measureBlock (remote->workBuffer);
            
            
            // apply wet gain for this to tempbuf
            if (fabsf(usegain - remote->_lastgain) > 0.00001) {
                remote->workBuffer.applyGainRamp(0, numSamples, remote->_lastgain, usegain);
                remote->_lastgain = usegain;
            } else {
                remote->workBuffer.applyGain(remote->gain);
            }


            
            for (int channel = 0; channel < totalNumOutputChannels; ++channel) {

                // now apply panning

                if (remote->recvChannels > 0 && totalNumOutputChannels > 1) {
                    for (int i=0; i < remote->recvChannels; ++i) {
                        const float pan = remote->recvPan[i];
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
                
                for (int channel = 0; channel < totalNumOutputChannels && totalNumInputChannels > 0; ++channel) {
                    int inchan = channel < totalNumInputChannels ? channel : totalNumInputChannels-1;
                    // add our input
                    workBuffer.addFrom(channel, 0, buffer, inchan, 0, numSamples);

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
                                    const float pan = crossremote->recvPan[ch];
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
                
                remote->sendMeterSource.measureBlock (workBuffer);
                
                // now process echo and latency stuff
                
                workBuffer.clear(0, 0, numSamples);
                if (remote->echosink->process(workBuffer.getArrayOfWritePointers(), numSamples, t)) {
                    //DebugLogC("received something from our ECHO sink");
                    remote->echosource->process(workBuffer.getArrayOfReadPointers(), numSamples, t);
                }

                if (remote->activeLatencyTest && remote->latencyProcessor) {
                    workBuffer.clear(0, 0, numSamples);
                    if (remote->latencysink->process(workBuffer.getArrayOfWritePointers(), numSamples, t)) {
                        //DebugLogC("received something from our latency sink");
                    }
                    
                    remote->latencyProcessor->process(numSamples, workBuffer.getWritePointer(0), workBuffer.getWritePointer(0));
                    

                    remote->latencysource->process(workBuffer.getArrayOfReadPointers(), numSamples, t);                                        
                }
                
            }
            
            ++i;
        }

        // update last state
        for (auto & remote : mRemotePeers) 
        {
            for (int i=0; i < remote->recvChannels; ++i) {
                remote->recvPanLast[i] = remote->recvPan[i];
            }
        }
        
        // end scoped lock
    }

    
    // apply dry (input monitor) gain to original buf
    // and panning
    
    if (totalNumInputChannels > 0 && totalNumOutputChannels > 1) {
        
        // copy input into workbuffer
        workBuffer.clear();
        
        for (int channel = 0; channel < totalNumOutputChannels; ++channel) {
            for (int i=0; i < totalNumInputChannels; ++i) {
                const float pan = (i==0 ? inmonPan1 : inmonPan2);
                const float lastpan = (i==0 ? mLastInMonPan1 : mLastInMonPan2);
                
                // apply pan law
                // -1 is left, 1 is right
                float pgain = channel == 0 ? (pan >= 0.0f ? (1.0f - pan) : 1.0f) : (pan >= 0.0f ? 1.0f : (1.0f+pan)) ;
                
                if (fabsf(pan - lastpan) > 0.00001f) {
                    float plastgain = channel == 0 ? (lastpan >= 0.0f ? (1.0f - lastpan) : 1.0f) : (lastpan >= 0.0f ? 1.0f : (1.0f+lastpan));
                    
                    workBuffer.addFromWithRamp(channel, 0, buffer.getReadPointer(i), numSamples, plastgain, pgain);
                } else {
                    workBuffer.addFrom (channel, 0, buffer, i, 0, numSamples, pgain);                            
                }
            }
        }
        
        // now copy workbuffer into buffer with gain
        bool rampit =  (fabsf(drynow - mLastDry) > 0.00001);
        for (int channel = 0; channel < totalNumOutputChannels; ++channel) {
            if (rampit) {
                buffer.copyFromWithRamp(channel, 0, workBuffer.getReadPointer(channel), numSamples, mLastDry, drynow);                
            }
            else {
                buffer.copyFrom(channel, 0, workBuffer.getReadPointer(channel), numSamples, drynow);
            }
        }
        
        
    } else {

        if (fabsf(drynow - mLastDry) > 0.00001) {
            buffer.applyGainRamp(0, numSamples, mLastDry, drynow);
        } else {
            buffer.applyGain(drynow);
        }
        
    }
    
    
    // add from tempBuffer
    for (int channel = 0; channel < totalNumOutputChannels; ++channel) {
        
        buffer.addFrom(channel, 0, tempBuffer, channel, 0, numSamples);
    }
    
    // apply wet (output) gain to original buf
    if (fabsf(wetnow - mLastWet) > 0.00001) {
        buffer.applyGainRamp(0, numSamples, mLastWet, wetnow);
    } else {
        buffer.applyGain(wetnow);
    }
    
    outputMeterSource.measureBlock (buffer);

    // TODO output to file writer if necessary
    
    
    if (needsblockchange) {
        lastSamplesPerBlock = numSamples;
    }

    notifySendThread();
    
    mLastWet = wetnow;
    mLastDry = drynow;
    mLastInputGain = inGain;
    mLastInMonPan1 = inmonPan1;
    mLastInMonPan2 = inmonPan2;
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
    
    
    mState.state.writeToStream (stream);
    
    DebugLogC("GETSTATE: %s", mState.state.toXmlString().toRawUTF8());
}

void SonobusAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    ValueTree tree = ValueTree::readFromData (data, sizeInBytes);
    if (tree.isValid()) {
        mState.state = tree;
        
        DebugLogC("SETSTATE: %s", mState.state.toXmlString().toRawUTF8());
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
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SonobusAudioProcessor();
}
