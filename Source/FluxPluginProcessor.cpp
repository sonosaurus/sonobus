/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "FluxPluginProcessor.h"
#include "FluxPluginEditor.h"

#include "RunCumulantor.h"

#include "DebugLogC.h"

#include "aoo/aoo_net.h"
#include "aoo/aoo_pcm.h"
#include "aoo/aoo_opus.h"

String FluxAoOAudioProcessor::paramInGain     ("ingain");
String FluxAoOAudioProcessor::paramDry     ("dry");
String FluxAoOAudioProcessor::paramWet     ("wet");
String FluxAoOAudioProcessor::paramBufferTime  ("buffertime");

#define METER_RMS_SEC 0.04
#define MAX_PANNERS 4

struct FluxAoOAudioProcessor::EndpointState {
    EndpointState(String ipaddr_="", int port_=0) : ipaddr(ipaddr_), port(port_) {}
    DatagramSocket *owner;
    //struct sockaddr_storage addr;
    //socklen_t addrlen;
    std::unique_ptr<DatagramSocket::RemoteAddrInfo> peer;
    String ipaddr;
    int port = 0;
    // runtime state
    int64_t sentBytes = 0;
    int64_t recvBytes = 0;
};



struct FluxAoOAudioProcessor::RemotePeer {
    RemotePeer(EndpointState * ep = 0, int id_=0, aoo::isink::pointer oursink_ = 0, aoo::isource::pointer oursource_ = 0) : endpoint(ep), 
        ourId(id_), 
        oursink(std::move(oursink_)), oursource(std::move(oursource_)) {
            for (int i=0; i < MAX_PANNERS; ++i) {
                recvPan[i] = recvPanLast[i] = 0.0f;
            }
        }

    EndpointState * endpoint = 0;
    int32_t ourId = AOO_ID_NONE;
    int32_t remoteSinkId = AOO_ID_NONE;
    int32_t remoteSourceId = AOO_ID_NONE;
    aoo::isink::pointer oursink;
    aoo::isource::pointer oursource;
    float gain = 1.0f;
    float buffertimeMs = 15.0f;
    bool  autosizeBuffer = true;
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
    int64_t dataPacketsReceived = 0;
    int64_t dataPacketsSent = 0;
    int64_t dataPacketsDropped = 0;
    int64_t dataPacketsResent = 0;
    double lastDroptime = 0;
    int64_t lastDropCount = 0;
    float pingTime = 0.0f; // ms
    stats::RunCumulantor1D  smoothPingTime; // ms
    float totalLatency = 0.0f;
    AudioSampleBuffer workBuffer;
    float recvPanLast[MAX_PANNERS];
    // metering
    foleys::LevelMeterSource sendMeterSource;
    foleys::LevelMeterSource recvMeterSource;

};



static int endpoint_send(void *e, const char *data, int size)
{
    FluxAoOAudioProcessor::EndpointState * endpoint = static_cast<FluxAoOAudioProcessor::EndpointState*>(e);
    int result = -1;
    if (endpoint->peer) {
        result = endpoint->owner->write(*(endpoint->peer), data, size);
    } else {
        result = endpoint->owner->write(endpoint->ipaddr, endpoint->port, data, size);
    }
    
    if (result > 0) {
        endpoint->sentBytes += result;
    }
    
    return result;
}


class FluxAoOAudioProcessor::SendThread : public juce::Thread
{
public:
    SendThread(FluxAoOAudioProcessor & processor) : Thread("FluxSendThread") , _processor(processor) 
    {}
    
    void run() override {
        
        while (!threadShouldExit()) {
         
            if (_processor.mSendWaitable.wait(10)) {
                _processor.doSendData();
            }            
        }
        DebugLogC("Send thread finishing");
    }
    
    FluxAoOAudioProcessor & _processor;
    
};

class FluxAoOAudioProcessor::RecvThread : public juce::Thread
{
public:
    RecvThread(FluxAoOAudioProcessor & processor) : Thread("FluxRecvThread") , _processor(processor) 
    {}
    
    void run() override {

        while (!threadShouldExit()) {
         
            if (_processor.mUdpSocket->waitUntilReady(true, 20) == 1) {
                _processor.doReceiveData();
            }
        }

        DebugLogC("Recv thread finishing");        
    }
    
    FluxAoOAudioProcessor & _processor;
    
};

class FluxAoOAudioProcessor::EventThread : public juce::Thread
{
public:
    EventThread(FluxAoOAudioProcessor & processor) : Thread("FluxEventThread") , _processor(processor) 
    {}
    
    void run() override {

        while (!threadShouldExit()) {
         
            sleep(20);
            
            _processor.handleEvents();                       
        }
        
        DebugLogC("Event thread finishing");
    }
    
    FluxAoOAudioProcessor & _processor;
    
};

//==============================================================================
FluxAoOAudioProcessor::FluxAoOAudioProcessor()
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
mState (*this, &mUndoManager, "FluxAoO",
{
    std::make_unique<AudioParameterFloat>(paramInGain,     TRANS ("In Gain"),    NormalisableRange<float>(0.0, 1.0, 0.0, 0.25), mInGain.get(), "", AudioProcessorParameter::genericParameter, 
                                          [](float v, int maxlen) -> String { return Decibels::toString(Decibels::gainToDecibels(v), 1); }, 
                                          //[](float v, int maxlen) -> String { return String::formatted("%.1f %%",v*100.0f); }, 
                                          [](const String& s) -> float { return Decibels::decibelsToGain(s.getFloatValue()); }),
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

    for (int i=0; i < MAX_PEERS; ++i) {
        for (int j=0; j < MAX_PEERS; ++j) {
            mRemoteSendMatrix[i][j] = false;
        }
    }
    
    initializeAoo();
}

FluxAoOAudioProcessor::~FluxAoOAudioProcessor()
{
    cleanupAoo();
}

void FluxAoOAudioProcessor::initializeAoo()
{
    int udpport = 11000;
    // we have both an AOO source and sink
        
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
    
    mSendThread = std::make_unique<SendThread>(*this);
    mSendThread->setPriority(8);
    mRecvThread = std::make_unique<RecvThread>(*this);
    mRecvThread->setPriority(7);
    mEventThread = std::make_unique<EventThread>(*this);
    
    mSendThread->startThread();
    mRecvThread->startThread();
    mEventThread->startThread();
}

void FluxAoOAudioProcessor::cleanupAoo()
{
    DebugLogC("waiting on recv thread to die");
    mRecvThread->stopThread(400);
    DebugLogC("waiting on send thread to die");
    mSendThread->stopThread(400);
    DebugLogC("waiting on event thread to die");
    mEventThread->stopThread(400);

    {
        const ScopedWriteLock sl (mCoreLock);        

        mUdpSocket.reset();
        
        mAooDummySource.reset();
        
        mRemotePeers.clear();
        
        mEndpoints.clear();
    }
    
}


void FluxAoOAudioProcessor::initFormats()
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


String FluxAoOAudioProcessor::getAudioCodeFormatName(int formatIndex) const
{
    if (formatIndex >= mAudioFormats.size()) return "";
    
    const AudioCodecFormatInfo & info = mAudioFormats.getReference(formatIndex);
    return info.name;    
}

void FluxAoOAudioProcessor::setRemotePeerAudioCodecFormat(int index, int formatIndex)
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
    }
}

int FluxAoOAudioProcessor::getRemotePeerAudioCodecFormat(int index) const
{
    if (index >= mRemotePeers.size()) return -1;
    
    const ScopedReadLock sl (mCoreLock);        
    auto remote = mRemotePeers.getUnchecked(index);
    return remote->formatIndex;
}

int FluxAoOAudioProcessor::getRemotePeerSendPacketsize(int index) const
{
    if (index >= mRemotePeers.size()) return -1;
    const ScopedReadLock sl (mCoreLock);        
    auto remote = mRemotePeers.getUnchecked(index);
    return remote->packetsize;
    
}

void FluxAoOAudioProcessor::setRemotePeerSendPacketsize(int index, int psize)
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
foleys::LevelMeterSource * FluxAoOAudioProcessor::getRemotePeerRecvMeterSource(int index)
{
    if (index >= mRemotePeers.size()) return nullptr;
    const ScopedReadLock sl (mCoreLock);        
    auto remote = mRemotePeers.getUnchecked(index);
    return &(remote->recvMeterSource);
}

foleys::LevelMeterSource * FluxAoOAudioProcessor::getRemotePeerSendMeterSource(int index)
{
    if (index >= mRemotePeers.size()) return nullptr;
    const ScopedReadLock sl (mCoreLock);        
    auto remote = mRemotePeers.getUnchecked(index);
    return &(remote->sendMeterSource);
}




FluxAoOAudioProcessor::EndpointState * FluxAoOAudioProcessor::findOrAddEndpoint(const String & host, int port)
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

void FluxAoOAudioProcessor::doReceiveData()
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
                        
                    }
                }

                
            } else if (type == AOO_TYPE_CLIENT || type == AOO_TYPE_PEER){
                // forward OSC packet to matching client
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
            } else {
                DebugLogC("FluxAoo bug: unknown aoo type: %d", type);
            }
        }

        // notify send thread
        notifySendThread();

    } 
    else {
        // not a valid AoO OSC message
        DebugLogC("FluxAoO: not a valid AOO message!");
    }
        
}

void FluxAoOAudioProcessor::doSendData()
{
    // just try to send for everybody
    const ScopedReadLock sl (mCoreLock);        

    // send stuff until there is nothing left to send
    
    bool didsomething = true;
    
    while (didsomething) {
        //mAooSource->send();
        didsomething = false;
        
        didsomething |= mAooDummySource->send();
        
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
        }
        
    }
}

struct ProcessorIdPair
{
    ProcessorIdPair(FluxAoOAudioProcessor *proc, int32_t id_) : processor(proc), id(id_) {}
    FluxAoOAudioProcessor * processor;
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


void FluxAoOAudioProcessor::handleEvents()
{
    const ScopedReadLock sl (mCoreLock);        
    int32_t dummy;
    
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

}

int32_t FluxAoOAudioProcessor::handleSourceEvents(const aoo_event ** events, int32_t n, int32_t sourceId)
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

                // todo smooth it
                peer->pingTime = rtt * 0.5;
                peer->smoothPingTime.Z *= 0.9;
                peer->smoothPingTime.push(peer->pingTime);
                peer->totalLatency =  peer->smoothPingTime.xbar + peer->buffertimeMs + (1e3*currSamplesPerBlock/getSampleRate());
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
                        // not one of our sources 
                        DebugLogC("No source, how is this possible?")

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

int32_t FluxAoOAudioProcessor::handleSinkEvents(const aoo_event ** events, int32_t n, int32_t sinkId)
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
                
                if (peer->autosizeBuffer) {
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
                                peer->totalLatency = peer->smoothPingTime.xbar + peer->buffertimeMs + (1e3*currSamplesPerBlock/getSampleRate());
                                peer->oursink->set_buffersize(peer->buffertimeMs);

                                DebugLogC("AUTO-Increasing buffer time by 1 ms to %d  droprate: %g", (int) peer->buffertimeMs, droprate);
                            }
                            
                            peer->lastDroptime = nowtime;
                            peer->lastDropCount = peer->dataPacketsDropped;
                        }
                    }
                    else {
                        peer->lastDroptime = nowtime;
                        peer->lastDropCount = peer->dataPacketsDropped;
                    }
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



int FluxAoOAudioProcessor::connectRemotePeer(const String & host, int port, bool reciprocate)
{
    EndpointState * endpoint = findOrAddEndpoint(host, port);

    RemotePeer * remote = doAddRemotePeerIfNecessary(endpoint); // get new one

    remote->recvAllow = true;
    
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

bool FluxAoOAudioProcessor::disconnectRemotePeer(const String & host, int port, int32_t ourId)
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

bool FluxAoOAudioProcessor::disconnectRemotePeer(int index)
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

bool FluxAoOAudioProcessor::getPatchMatrixValue(int srcindex, int destindex) const
{
    if (srcindex < MAX_PEERS && destindex < MAX_PEERS) {
        return mRemoteSendMatrix[srcindex][destindex];
    }
    return false;
}

void FluxAoOAudioProcessor::setPatchMatrixValue(int srcindex, int destindex, bool value)
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


void FluxAoOAudioProcessor::adjustRemoteSendMatrix(int index, bool removed)
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

bool FluxAoOAudioProcessor::removeRemotePeer(int index)
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


int FluxAoOAudioProcessor::getNumberRemotePeers() const
{
    return mRemotePeers.size();
}

void FluxAoOAudioProcessor::setRemotePeerLevelGain(int index, float levelgain)
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        remote->gain = levelgain;
    }
}

float FluxAoOAudioProcessor::getRemotePeerLevelGain(int index) const
{
    float levelgain = 0.0f;
    
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        levelgain = remote->gain;
    }
    return levelgain;
}

void FluxAoOAudioProcessor::setRemotePeerChannelPan(int index, int chan, float pan)
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        if (chan < MAX_PANNERS) {
            remote->recvPan[chan] = pan;
        }
    }
}

float FluxAoOAudioProcessor::getRemotePeerChannelPan(int index, int chan) const
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

int FluxAoOAudioProcessor::getRemotePeerChannelCount(int index) const
{
    int ret = 0;
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        ret = remote->recvChannels;
    }
    return ret;
}


void FluxAoOAudioProcessor::setRemotePeerBufferTime(int index, float bufferMs)
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        remote->buffertimeMs = bufferMs;
        remote->totalLatency = remote->smoothPingTime.xbar + remote->buffertimeMs + (1e3*currSamplesPerBlock/getSampleRate());
        remote->oursink->set_buffersize(remote->buffertimeMs); // ms
    }
}

float FluxAoOAudioProcessor::getRemotePeerBufferTime(int index) const
{
    float buftimeMs = 30.0f;
    
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        buftimeMs = remote->buffertimeMs;
    }
    return buftimeMs;    
}

void FluxAoOAudioProcessor::setRemotePeerAutoresizeBuffer(int index, bool flag)
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        remote->autosizeBuffer = flag;
    }
}

bool FluxAoOAudioProcessor::getRemotePeerAutoresizeBuffer(int index) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return remote->autosizeBuffer;
    }
    return false;    
}


void FluxAoOAudioProcessor::setRemotePeerRecvActive(int index, bool active)
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

bool FluxAoOAudioProcessor::getRemotePeerRecvActive(int index) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return remote->recvActive;
    }
    return false;        
}

void FluxAoOAudioProcessor::setRemotePeerSendAllow(int index, bool allow)
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

bool FluxAoOAudioProcessor::getRemotePeerSendAllow(int index) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return remote->sendAllow;
    }
    return false;        
}

void FluxAoOAudioProcessor::setRemotePeerRecvAllow(int index, bool allow)
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

bool FluxAoOAudioProcessor::getRemotePeerRecvAllow(int index) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return remote->recvAllow;
    }
    return false;        
}


int64_t FluxAoOAudioProcessor::getRemotePeerPacketsReceived(int index) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return remote->dataPacketsReceived;
    }
    return 0;      
}

int64_t FluxAoOAudioProcessor::getRemotePeerPacketsSent(int index) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return remote->dataPacketsSent;
    }
    return 0;      
}

int64_t FluxAoOAudioProcessor::getRemotePeerBytesSent(int index) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return remote->endpoint->sentBytes;
    }
    return 0;          
}

int64_t FluxAoOAudioProcessor::getRemotePeerBytesReceived(int index) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return remote->endpoint->recvBytes;
    }
    return 0;      
}

int64_t  FluxAoOAudioProcessor::getRemotePeerPacketsDropped(int index) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return remote->dataPacketsDropped;
    }
    return 0;      
}

int64_t  FluxAoOAudioProcessor::getRemotePeerPacketsResent(int index) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return remote->dataPacketsResent;
    }
    return 0;      
}


float FluxAoOAudioProcessor::getRemotePeerPingMs(int index) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return remote->smoothPingTime.xbar;
    }
    return 0;          
}

float FluxAoOAudioProcessor::getRemotePeerTotalLatencyMs(int index) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return remote->totalLatency ;
    }
    return 0;          
}


void FluxAoOAudioProcessor::setRemotePeerSendActive(int index, bool active)
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

bool FluxAoOAudioProcessor::getRemotePeerSendActive(int index) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return remote->sendActive;
    }
    return false;        
}

void FluxAoOAudioProcessor::setRemotePeerConnected(int index, bool active)
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        remote->connected = active;
    }
}

bool FluxAoOAudioProcessor::getRemotePeerConnected(int index) const
{
    const ScopedReadLock sl (mCoreLock);        
    if (index < mRemotePeers.size()) {
        RemotePeer * remote = mRemotePeers.getUnchecked(index);
        return remote->connected;
    }
    return false;        
}


bool FluxAoOAudioProcessor::getRemotePeerAddressInfo(int index, String & rethost, int & retport) const
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


FluxAoOAudioProcessor::RemotePeer *  FluxAoOAudioProcessor::findRemotePeer(EndpointState * endpoint, int32_t ourId)
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

FluxAoOAudioProcessor::RemotePeer *  FluxAoOAudioProcessor::findRemotePeerByRemoteSourceId(EndpointState * endpoint, int32_t sourceId)
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

FluxAoOAudioProcessor::RemotePeer *  FluxAoOAudioProcessor::findRemotePeerByRemoteSinkId(EndpointState * endpoint, int32_t sinkId)
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



FluxAoOAudioProcessor::RemotePeer * FluxAoOAudioProcessor::doAddRemotePeerIfNecessary(EndpointState * endpoint, int32_t ourId)
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

        retpeer = mRemotePeers.add(new RemotePeer(endpoint, newid, aoo::isink::pointer(aoo::isink::create(newid)), aoo::isource::pointer(aoo::isource::create(newid))));

        retpeer->oursink->setup(getSampleRate(), currSamplesPerBlock, getTotalNumOutputChannels());

        retpeer->sendChannels = getTotalNumInputChannels(); // by default
        
        setupSourceFormat(retpeer, retpeer->oursource.get());
        retpeer->oursource->setup(getSampleRate(), currSamplesPerBlock, retpeer->sendChannels);
        retpeer->oursource->set_packetsize(retpeer->packetsize);

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

bool FluxAoOAudioProcessor::doRemoveRemotePeerIfNecessary(EndpointState * endpoint, int32_t ourId)
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

bool FluxAoOAudioProcessor::isAnythingRoutedToPeer(int index) const
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

void FluxAoOAudioProcessor::setupSourceFormat(FluxAoOAudioProcessor::RemotePeer * peer, aoo::isource * source)
{
    // have choice and parameters
    
    const AudioCodecFormatInfo & info =  mAudioFormats.getReference((!peer || peer->formatIndex < 0) ? mDefaultAudioFormatIndex : peer->formatIndex);
    
    aoo_format_storage f;
    
    int channels = peer ? peer->sendChannels : getTotalNumInputChannels();
    
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

        source->set_format(fmt->header);        
    }
}


void FluxAoOAudioProcessor::parameterChanged (const String &parameterID, float newValue)
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
    else if (parameterID == paramBufferTime) {
       
    }

}

//==============================================================================
const String FluxAoOAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool FluxAoOAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool FluxAoOAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool FluxAoOAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double FluxAoOAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int FluxAoOAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int FluxAoOAudioProcessor::getCurrentProgram()
{
    return 0;
}

void FluxAoOAudioProcessor::setCurrentProgram (int index)
{
}

const String FluxAoOAudioProcessor::getProgramName (int index)
{
    return {};
}

void FluxAoOAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void FluxAoOAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
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

void FluxAoOAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FluxAoOAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

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

void FluxAoOAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    float inGain = mInGain.get();
    float drynow = mDry.get(); // DB_CO(dry_level);
    float wetnow = mWet.get(); // DB_CO(wet_level);

    
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
    
    // apply dry gain to original buf
    if (fabsf(drynow - mLastDry) > 0.00001) {
        buffer.applyGainRamp(0, numSamples, mLastDry, drynow);
    } else {
        buffer.applyGain(drynow);
    }
    
    // add from tempBuffer
    for (int channel = 0; channel < totalNumOutputChannels; ++channel) {
        
        buffer.addFrom(channel, 0, tempBuffer, channel, 0, numSamples);
    }
    
    // apply wet gain to original buf
    if (fabsf(wetnow - mLastWet) > 0.00001) {
        buffer.applyGainRamp(0, numSamples, mLastWet, wetnow);
    } else {
        buffer.applyGain(wetnow);
    }
    
    outputMeterSource.measureBlock (buffer);

    
    if (needsblockchange) {
        lastSamplesPerBlock = numSamples;
    }

    notifySendThread();
    
    mLastWet = wetnow;
    mLastDry = drynow;
    mLastInputGain = inGain;
}

//==============================================================================
bool FluxAoOAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* FluxAoOAudioProcessor::createEditor()
{
    return new FluxAoOAudioProcessorEditor (*this);
}

AudioProcessorValueTreeState& FluxAoOAudioProcessor::getValueTreeState()
{
    return mState;
}

//==============================================================================
void FluxAoOAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    MemoryOutputStream stream(destData, false);
    mState.state.writeToStream (stream);
    
    DebugLogC("GETSTATE: %s", mState.state.toXmlString().toRawUTF8());
}

void FluxAoOAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    ValueTree tree = ValueTree::readFromData (data, sizeInBytes);
    if (tree.isValid()) {
        mState.state = tree;
        
        DebugLogC("SETSTATE: %s", mState.state.toXmlString().toRawUTF8());
        
    }
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FluxAoOAudioProcessor();
}
