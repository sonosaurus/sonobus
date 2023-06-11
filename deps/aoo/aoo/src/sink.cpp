/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "sink.hpp"

#include <algorithm>
#include <cmath>

namespace aoo {

// OSC data message
const int32_t kDataMaxAddrSize = kAooMsgDomainLen + kAooMsgSourceLen + 16 + kAooMsgDataLen;
// typetag string: 4 bytes
// args: 8 bytes (sink ID + stream ID)
const int32_t kDataHeaderSize = kDataMaxAddrSize + 8;

// binary data message:
// args: 8 bytes (stream ID + count)
const int32_t kBinDataHeaderSize = kAooBinMsgLargeHeaderSize + 8;

} // aoo

//------------------------- Sink ------------------------------//

AOO_API AooSink * AOO_CALL AooSink_new(AooId id, AooError *err) {
    try {
        if (err) {
            *err = kAooErrorNone;
        }
        return aoo::construct<aoo::Sink>(id);
    } catch (const std::bad_alloc&) {
        if (err) {
            *err = kAooErrorOutOfMemory;
        }
        return nullptr;
    }
}

aoo::Sink::Sink(AooId id)
    : id_(id) {}

AOO_API void AOO_CALL AooSink_free(AooSink *sink) {
    // cast to correct type because base class
    // has no virtual destructor!
    aoo::destroy(static_cast<aoo::Sink *>(sink));
}

aoo::Sink::~Sink(){
    // free remaining source requests
    source_request r;
    while (requestqueue_.try_pop(r)) {
        if (r.type == request_type::invite){
            // free metadata
            auto md = r.invite.metadata;
            if (md != nullptr){
                auto size = flat_metadata_size(*md);
                aoo::rt_deallocate(md, size);
            }
        }
    }
}

AOO_API AooError AOO_CALL AooSink_setup(
        AooSink *sink, AooInt32 nchannels, AooSampleRate samplerate,
        AooInt32 blocksize, AooFlag flags) {
    return sink->setup(nchannels, samplerate, blocksize, flags);
}

AooError AOO_CALL aoo::Sink::setup(
        AooInt32 nchannels, AooSampleRate samplerate,
        AooInt32 blocksize, AooFlag flags) {
    if (nchannels >= 0 && samplerate > 0 && blocksize > 0)
    {
        if (nchannels != nchannels_ || samplerate != samplerate_ ||
            blocksize != blocksize_)
        {
            nchannels_ = nchannels;
            samplerate_ = samplerate;
            blocksize_ = blocksize;

            realsr_.store(samplerate);

            reset_sources();
        }

        reset_timer(); // always reset!

        return kAooOk;
    } else {
        return kAooErrorBadArgument;
    }
}

namespace aoo {

template<typename T>
T& as(void *p){
    return *reinterpret_cast<T *>(p);
}

} // aoo

#define CHECKARG(type) assert(size == sizeof(type))

#define GETSOURCEARG \
    source_lock lock(sources_);         \
    auto src = get_source_arg(index);   \
    if (!src) {                         \
        return kAooErrorNotFound;       \
    }                                   \

AOO_API AooError AOO_CALL AooSink_control(
        AooSink *sink, AooCtl ctl, AooIntPtr index, void *ptr, AooSize size)
{
    return sink->control(ctl, index, ptr, size);
}

AooError AOO_CALL aoo::Sink::control(
        AooCtl ctl, AooIntPtr index, void *ptr, AooSize size)
{
    switch (ctl){
    // id
    case kAooCtlSetId:
    {
        CHECKARG(int32_t);
        auto newid = as<int32_t>(ptr);
        if (id_.exchange(newid) != newid){
            // LATER clear source list here
        }
        break;
    }
    case kAooCtlGetId:
        CHECKARG(AooId);
        as<AooId>(ptr) = id();
        break;
    // reset
    case kAooCtlReset:
    {
        if (index != 0){
            GETSOURCEARG
            src->reset(*this);
        } else {
            reset_sources();
            reset_timer();
        }
        break;
    }
    // get format
    case kAooCtlGetFormat:
    {
        assert(size >= sizeof(AooFormat));
        GETSOURCEARG
        return src->get_format(as<AooFormat>(ptr), size);
    }
    // latency
    case kAooCtlSetLatency:
    {
        CHECKARG(AooSeconds);
        auto bufsize = std::max<AooSeconds>(0, as<AooSeconds>(ptr));
        if (latency_.exchange(bufsize) != bufsize){
            reset_sources();
        }
        break;
    }
    case kAooCtlGetLatency:
        CHECKARG(AooSeconds);
        as<AooSeconds>(ptr) = latency_.load();
        break;
    // buffer size
    case kAooCtlSetBufferSize:
    {
        CHECKARG(AooSeconds);
        auto bufsize = std::max<AooSeconds>(0, as<AooSeconds>(ptr));
        if (buffersize_.exchange(bufsize) != bufsize){
            reset_sources();
        }
        break;
    }
    case kAooCtlGetBufferSize:
        CHECKARG(AooSeconds);
        as<AooSeconds>(ptr) = buffersize_.load();
        break;
    // get buffer fill ratio
    case kAooCtlGetBufferFillRatio:
    {
        CHECKARG(double);
        GETSOURCEARG
        as<double>(ptr) = src->get_buffer_fill_ratio();
        break;
    }
    case kAooCtlReportXRun:
        CHECKARG(int32_t);
        handle_xrun(as<int32_t>(ptr));
        break;
    // set/get dynamic resampling
    case kAooCtlSetDynamicResampling:
    {
        CHECKARG(AooBool);
        bool b = as<AooBool>(ptr);
        dynamic_resampling_.store(b);
        reset_timer();
        break;
    }
    case kAooCtlGetDynamicResampling:
        CHECKARG(AooBool);
        as<AooBool>(ptr) = dynamic_resampling_.load();
        break;
    // time DLL filter bandwidth
    case kAooCtlSetDllBandwidth:
    {
        CHECKARG(float);
        auto bw = std::max<double>(0, std::min<double>(1, as<float>(ptr)));
        dll_bandwidth_.store(bw);
        reset_timer();
        break;
    }
    case kAooCtlGetDllBandwidth:
        CHECKARG(float);
        as<float>(ptr) = dll_bandwidth_.load();
        break;
    // real samplerate
    case kAooCtlGetRealSampleRate:
        CHECKARG(double);
        as<double>(ptr) = realsr_.load();
        break;
    // set/get ping interval
    case kAooCtlSetPingInterval:
    {
        CHECKARG(AooSeconds);
        auto interval = std::max<AooSeconds>(0, as<AooSeconds>(ptr));
        ping_interval_.store(interval);
        break;
    }
    case kAooCtlGetPingInterval:
        CHECKARG(AooSeconds);
        as<AooSeconds>(ptr) = ping_interval_.load();
        break;
    // packetsize
    case kAooCtlSetPacketSize:
    {
        CHECKARG(int32_t);
        const int32_t minpacketsize = kDataHeaderSize + 64;
        auto packetsize = as<int32_t>(ptr);
        if (packetsize < minpacketsize){
            LOG_WARNING("AooSink: packet size too small! setting to " << minpacketsize);
            packetsize_.store(minpacketsize);
        } else if (packetsize > AOO_MAX_PACKET_SIZE){
            LOG_WARNING("AooSink: packet size too large! setting to " << AOO_MAX_PACKET_SIZE);
            packetsize_.store(AOO_MAX_PACKET_SIZE);
        } else {
            packetsize_.store(packetsize);
        }
        break;
    }
    case kAooCtlGetPacketSize:
        CHECKARG(int32_t);
        as<int32_t>(ptr) = packetsize_.load();
        break;
    // resend data
    case kAooCtlSetResendData:
        CHECKARG(AooBool);
        resend_.store(as<AooBool>(ptr));
        break;
    case kAooCtlGetResendData:
        CHECKARG(AooBool);
        as<AooBool>(ptr) = resend_.load();
        break;
    // resend interval
    case kAooCtlSetResendInterval:
    {
        CHECKARG(AooSeconds);
        auto interval = std::max<AooSeconds>(0, as<AooSeconds>(ptr));
        resend_interval_.store(interval);
        break;
    }
    case kAooCtlGetResendInterval:
        CHECKARG(AooSeconds);
        as<AooSeconds>(ptr) = resend_interval_.load();
        break;
    // resend limit
    case kAooCtlSetResendLimit:
    {
        CHECKARG(int32_t);
        auto limit = std::max<int32_t>(1, as<int32_t>(ptr));
        resend_limit_.store(limit);
        break;
    }
    case kAooCtlGetResendLimit:
        CHECKARG(AooSeconds);
        as<int32_t>(ptr) = resend_limit_.load();
        break;
    // source timeout
    case kAooCtlSetSourceTimeout:
    {
        CHECKARG(AooSeconds);
        auto timeout = std::max<AooSeconds>(0, as<AooSeconds>(ptr));
        source_timeout_.store(timeout);
        break;
    }
    case kAooCtlGetSourceTimeout:
        CHECKARG(AooSeconds);
        as<AooSeconds>(ptr) = source_timeout_.load();
        break;
    case kAooCtlSetInviteTimeout:
    {
        CHECKARG(AooSeconds);
        auto timeout = std::max<AooSeconds>(0, as<AooSeconds>(ptr));
        invite_timeout_.store(timeout);
        break;
    }
    case kAooCtlGetInviteTimeout:
        CHECKARG(AooSeconds);
        as<AooSeconds>(ptr) = invite_timeout_.load();
        break;
#if AOO_NET
    case kAooCtlSetClient:
        client_ = reinterpret_cast<AooClient *>(index);
        break;
#endif
    // unknown
    default:
        LOG_WARNING("AooSink: unsupported control " << ctl);
        return kAooErrorNotImplemented;
    }
    return kAooOk;
}

AOO_API AooError AOO_CALL AooSink_codecControl(
        AooSink *sink, AooCtl ctl, AooIntPtr index, void *data, AooSize size)
{
    return sink->codecControl(ctl, index, data, size);
}

AooError AOO_CALL aoo::Sink::codecControl(
        AooCtl ctl, AooIntPtr index, void *data, AooSize size) {
    return kAooErrorNotImplemented;
}

AOO_API AooError AOO_CALL AooSink_handleMessage(
        AooSink *sink, const AooByte *data, AooInt32 size,
        const void *address, AooAddrSize addrlen) {
    return sink->handleMessage(data, size, address, addrlen);
}

AooError AOO_CALL aoo::Sink::handleMessage(
        const AooByte *data, AooInt32 size,
        const void *address, AooAddrSize addrlen)
{
    if (samplerate_ == 0){
        return kAooErrorNotInitialized; // not setup yet
    }

    AooMsgType type;
    AooId sinkid;
    AooInt32 onset;
    auto err = aoo_parsePattern(data, size, &type, &sinkid, &onset);
    if (err != kAooOk){
        LOG_WARNING("AooSink: not an AOO message!");
        return kAooErrorBadFormat;
    }

    if (type != kAooMsgTypeSink){
        LOG_WARNING("AooSink: not a sink message!");
        return kAooErrorBadArgument;
    }
    if (sinkid != id()){
        LOG_WARNING("AooSink: wrong sink ID!");
        return kAooErrorBadArgument;
    }

    ip_address addr((const sockaddr *)address, addrlen);

    if (aoo::binmsg_check(data, size)){
        // binary message
        auto cmd = aoo::binmsg_cmd(data, size);
        auto id = aoo::binmsg_from(data, size);
        switch (cmd){
        case kAooBinMsgCmdData:
            return handle_data_message(data + onset, size - onset, id, addr);
        default:
            LOG_WARNING("AooSink: unsupported binary message");
            return kAooErrorNotImplemented;
        }
    } else {
        // OSC message
        try {
            osc::ReceivedPacket packet((const char *)data, size);
            osc::ReceivedMessage msg(packet);

            auto pattern = msg.AddressPattern() + onset;
            if (!strcmp(pattern, kAooMsgStart)){
                return handle_start_message(msg, addr);
            } else if (!strcmp(pattern, kAooMsgStop)){
                return handle_stop_message(msg, addr);
            } else if (!strcmp(pattern, kAooMsgDecline)){
                return handle_decline_message(msg, addr);
            } else if (!strcmp(pattern, kAooMsgData)){
                return handle_data_message(msg, addr);
            } else if (!strcmp(pattern, kAooMsgPing)){
                return handle_ping_message(msg, addr);
            } else if (!strcmp(pattern, kAooMsgPong)){
                return handle_pong_message(msg, addr);
            } else {
                LOG_WARNING("AooSink: unknown message " << pattern);
                return kAooErrorNotImplemented;
            }
        } catch (const osc::Exception& e){
            LOG_ERROR("AooSink: exception in handle_message: " << e.what());
            return kAooErrorBadFormat;
        }
    }
}

AOO_API AooError AOO_CALL AooSink_send(
        AooSink *sink, AooSendFunc fn, void *user){
    return sink->send(fn, user);
}

AooError AOO_CALL aoo::Sink::send(AooSendFunc fn, void *user){
    dispatch_requests();

    sendfn reply(fn, user);

    source_lock lock(sources_);
    for (auto& s : sources_){
        s.send(*this, reply);
    }
    lock.unlock(); // !

    // free unused sources
    if (!sources_.update()){
        // LOG_DEBUG("AooSink: try_free() would block");
    }

    return kAooOk;
}

AOO_API AooError AOO_CALL AooSink_process(
        AooSink *sink, AooSample **data, AooInt32 nsamples, AooNtpTime t,
        AooStreamMessageHandler messageHandler, void *user) {
    return sink->process(data, nsamples, t, messageHandler, user);
}

AooError AOO_CALL aoo::Sink::process(
        AooSample **data, AooInt32 nsamples, AooNtpTime t,
        AooStreamMessageHandler messageHandler, void *user) {
    // Always update timer and DLL, even if there are no sinks.
    // Do it *before* trying to lock the mutex.
    // (The DLL is only ever touched in this method.)
    bool dynamic_resampling = dynamic_resampling_.load();
    AooNtpTime start_time = 0;
    if (start_time_.compare_exchange_strong(start_time, t)) {
        // start timer
        LOG_DEBUG("AooSource: start timer");
        elapsed_time_.store(0);
        // reset DLL
        auto bw = dll_bandwidth_.load();
        dll_.setup(samplerate_, blocksize_, bw, 0);
        realsr_.store(samplerate_);
    } else {
        // advance timer
        // NB: start_time has been updated by the CAS above!
        auto elapsed = aoo::time_tag::duration(start_time, t);
        elapsed_time_.store(elapsed, std::memory_order_relaxed);
        // update time DLL, but only if nsamples matches blocksize!
        if (dynamic_resampling) {
            if (nsamples == blocksize_){
                dll_.update(elapsed);
            #if AOO_DEBUG_DLL
                LOG_ALL("AooSource: time elapsed: " << elapsed << ", period: "
                        << dll_.period() << ", samplerate: " << dll_.samplerate());
            #endif
            } else {
                // reset time DLL with nominal samplerate
                auto bw = dll_bandwidth_.load();
                dll_.setup(samplerate_, blocksize_, bw, elapsed);
            }
            realsr_.store(dll_.samplerate());
        }
    }

    // clear outputs
    if (data) {
        for (int i = 0; i < nchannels_; ++i){
            std::fill(data[i], data[i] + nsamples, 0);
        }
    }
#if 1
    if (sources_.empty() && requestqueue_.empty()) {
        // nothing to process and no need to call send()
        return kAooErrorIdle;
    }
#endif
    bool didsomething = false;

    // NB: we only remove sources in this thread,
    // so we do not need to lock the source mutex!
    source_lock lock(sources_);
    for (auto it = sources_.begin(); it != sources_.end();){
        if (it->process(*this, data, nsamples, messageHandler, user)){
            didsomething = true;
        } else if (!it->check_active(*this)){
            LOG_VERBOSE("AooSink: removed inactive source " << it->ep);
            auto e = make_event<source_event>(kAooEventSourceRemove, it->ep);
            send_event(std::move(e), kAooThreadLevelAudio);
            // move source to garbage list (will be freed in send())
            it = sources_.erase(it);
            continue;
        }
        ++it;
    }

    if (didsomething){
    #if AOO_CLIP_OUTPUT
        for (int i = 0; i < nchannels_; ++i){
            auto chn = data[i];
            for (int j = 0; j < nsamples; ++j){
                if (chn[j] > 1.0){
                    chn[j] = 1.0;
                } else if (chn[j] < -1.0){
                    chn[j] = -1.0;
                }
            }
        }
    #endif
    }
    return kAooOk;
}

AOO_API AooError AOO_CALL AooSink_setEventHandler(
        AooSink *sink, AooEventHandler fn, void *user, AooEventMode mode)
{
    return sink->setEventHandler(fn, user, mode);
}

AooError AOO_CALL aoo::Sink::setEventHandler(
        AooEventHandler fn, void *user, AooEventMode mode)
{
    eventhandler_ = fn;
    eventcontext_ = user;
    eventmode_ = mode;
    return kAooOk;
}

AOO_API AooBool AOO_CALL AooSink_eventsAvailable(AooSink *sink){
    return sink->eventsAvailable();
}

AooBool AOO_CALL aoo::Sink::eventsAvailable(){
    if (!eventqueue_.empty()){
        return true;
    }

    source_lock lock(sources_);
    for (auto& src : sources_){
        if (src.has_events()){
            return true;
        }
    }

    return false;
}

AOO_API AooError AOO_CALL AooSink_pollEvents(AooSink *sink){
    return sink->pollEvents();
}

#define EVENT_THROTTLE 1000

AooError AOO_CALL aoo::Sink::pollEvents(){
    int total = 0;
    event_ptr e;
    while (eventqueue_.try_pop(e)){
        eventhandler_(eventcontext_, &e->cast(), kAooThreadLevelUnknown);
        total++;
    }
    source_lock lock(sources_);
    for (auto& src : sources_){
        total += src.poll_events(*this, eventhandler_, eventcontext_);
        if (total > EVENT_THROTTLE){
            break;
        }
    }
    return kAooOk;
}

AOO_API AooError AOO_CALL AooSink_inviteSource(
        AooSink *sink, const AooEndpoint *source, const AooData *metadata)
{
    if (sink) {
        return sink->inviteSource(*source, metadata);
    } else {
        return kAooErrorBadArgument;
    }
}

AooError AOO_CALL aoo::Sink::inviteSource(
        const AooEndpoint& ep, const AooData *md) {
    ip_address addr((const sockaddr *)ep.address, ep.addrlen);

    AooData *metadata = nullptr;
    if (md) {
        auto size = flat_metadata_size(*md);
        metadata = (AooData *)aoo::rt_allocate(size);
        flat_metadata_copy(*md, *metadata);
    }

    source_request r(request_type::invite, addr, ep.id);
    r.invite.token = get_random_id();
    r.invite.metadata = metadata;
    push_request(r);

    return kAooOk;
}

AOO_API AooError AOO_CALL AooSink_uninviteSource(
        AooSink *sink, const AooEndpoint *source)
{
    if (source){
        return sink->uninviteSource(*source);
    } else {
        return kAooErrorBadArgument;
    }
}

AooError AOO_CALL aoo::Sink::uninviteSource(const AooEndpoint& ep) {
    ip_address addr((const sockaddr *)ep.address, ep.addrlen);
    push_request(source_request { request_type::uninvite, addr, ep.id });
    return kAooOk;
}

AOO_API AooError AOO_CALL AooSink_uninviteAll(AooSink *sink)
{
    return sink->uninviteAll();
}

AooError AOO_CALL aoo::Sink::uninviteAll() {
    push_request(source_request { request_type::uninvite_all });
    return kAooOk;
}

namespace aoo {

void Sink::send_event(event_ptr e, AooThreadLevel level) const {
    switch (eventmode_){
    case kAooEventModePoll:
        eventqueue_.push(std::move(e));
        break;
    case kAooEventModeCallback:
        eventhandler_(eventcontext_, &e->cast(), level);
        break;
    default:
        break;
    }
}

// only called if mode is kAooEventModeCallback
void Sink::call_event(event_ptr e, AooThreadLevel level) const {
    eventhandler_(eventcontext_, &e->cast(), level);
}

void Sink::dispatch_requests(){
    source_request r;
    while (requestqueue_.try_pop(r)){
        switch (r.type) {
        case request_type::invite:
        {
            // try to find existing source
            // we might want to invite an existing source,
            // e.g. when it is currently uninviting
            // NB: sources can also be added in the network receive
            // thread - see handle_data() or handle_format() -, so we
            // have to lock the source mutex to avoid the ABA problem.
            sync::scoped_lock<sync::mutex> lock1(source_mutex_);
            source_lock lock2(sources_);
            auto src = find_source(r.address, r.id);
            if (!src){
                src = add_source(r.address, r.id);
            }
            src->invite(*this, r.invite.token, r.invite.metadata);
            break;
        }
        case request_type::uninvite:
        {
            // try to find existing source
            source_lock lock(sources_);
            auto src = find_source(r.address, r.id);
            if (src){
                src->uninvite(*this);
            } else {
                LOG_WARNING("AooSink: can't uninvite - source not found");
            }
            break;
        }
        case request_type::uninvite_all:
        {
            source_lock lock(sources_);
            for (auto& src : sources_){
                src.uninvite(*this);
            }
            break;
        }
        default:
            break;
        }
    }
}

aoo::source_desc * Sink::find_source(const ip_address& addr, AooId id){
    for (auto& src : sources_){
        if (src.match(addr, id)){
            return &src;
        }
    }
    return nullptr;
}

aoo::source_desc * Sink::get_source_arg(intptr_t index){
    auto ep = (const AooEndpoint *)index;
    if (!ep){
        LOG_ERROR("AooSink: missing source argument");
        return nullptr;
    }
    ip_address addr((const sockaddr *)ep->address, ep->addrlen);
    auto src = find_source(addr, ep->id);
    if (!src){
        LOG_ERROR("AooSink: couldn't find source");
    }
    return src;
}

// called with source mutex locked
source_desc * Sink::add_source(const ip_address& addr, AooId id){
    // add new source
#if AOO_NET
    ip_address relay;
    // check if the peer needs to be relayed
    if (client_){
        AooEndpoint ep { addr.address(), (AooAddrSize)addr.length(), id };
        AooBool b;
        if (client_->control(kAooCtlNeedRelay,
                             reinterpret_cast<intptr_t>(&ep),
                             &b, sizeof(b)) == kAooOk)
        {
            if (b == kAooTrue){
                LOG_DEBUG("AooSink: source " << addr << " needs to be relayed");
                // get relay address
                client_->control(kAooCtlGetRelayAddress,
                                 reinterpret_cast<intptr_t>(&ep),
                                 &relay, sizeof(relay));
            }
        }
    }
    auto it = sources_.emplace_front(addr, id, relay, elapsed_time());
#else
    auto it = sources_.emplace_front(addr, id, elapsed_time());
#endif
    return &(*it);
}

void Sink::reset_sources(){
    source_lock lock(sources_);
    for (auto& src : sources_){
        src.reset(*this);
    }
}

void Sink::handle_xrun(int32_t nsamples) {
    LOG_DEBUG("AooSink: handle xrun (" << nsamples << " samples)");
    auto nblocks = (double)nsamples / (double)blocksize_;
    source_lock lock(sources_);
    for (auto& src : sources_){
        src.add_xrun(nblocks);
    }
    // also reset time DLL!
    reset_timer();
}

// /aoo/sink/<id>/start <src> <version> <stream_id> <flags> <lastformat>
// <nchannels> <samplerate> <blocksize> <codec> <options>
// [<metadata_type> <metadata_content>]
AooError Sink::handle_start_message(const osc::ReceivedMessage& msg,
                                    const ip_address& addr)
{
    auto it = msg.ArgumentsBegin();

    AooId id = (it++)->AsInt32();
    auto version = (it++)->AsString();

    // LATER handle this in the source_desc (e.g. ignoring further messages)
    if (auto err = check_version(version); err != kAooOk){
        if (err == kAooErrorVersionNotSupported) {
            LOG_ERROR("AooSource: source version not supported");
        } else {
            LOG_ERROR("AooSource: bad source version format");
        }
        return err;
    }

    AooId stream = (it++)->AsInt32();
    AooId format_id = (it++)->AsInt32();

    // get stream format
    AooFormat f;
    f.numChannels = (it++)->AsInt32();
    f.sampleRate = (it++)->AsInt32();
    f.blockSize = (it++)->AsInt32();
    snprintf(f.codecName, sizeof(f.codecName), "%s", (it++)->AsString());
    f.structSize = sizeof(AooFormat);

    const void *extension;
    osc::osc_bundle_element_size_t size;
    (it++)->AsBlob(extension, size);

    AooData metadata = osc_read_metadata(it); // optional

    if (id < 0){
        LOG_WARNING("AooSink: bad ID for " << kAooMsgStart << " message");
        return kAooErrorBadArgument;
    }
    // try to find existing source
    // NB: sources can also be added in the network send thread,
    // so we have to lock the source mutex to avoid the ABA problem!
    sync::scoped_lock<sync::mutex> lock1(source_mutex_);
    source_lock lock2(sources_);
    auto src = find_source(addr, id);
    if (!src){
        src = add_source(addr, id);
    }
    return src->handle_start(*this, stream, format_id, f,
                             (const AooByte *)extension, size,
                             (metadata.data ? &metadata : nullptr));
}

// /aoo/sink/<id>/stop <src> <stream>
AooError Sink::handle_stop_message(const osc::ReceivedMessage& msg,
                                   const ip_address& addr) {
    auto it = msg.ArgumentsBegin();

    AooId id = (it++)->AsInt32();
    AooId stream = (it++)->AsInt32();

    if (id < 0){
        LOG_WARNING("AooSink: bad ID for " << kAooMsgStop << " message");
        return kAooErrorBadArgument;
    }
    // try to find existing source
    source_lock lock(sources_);
    auto src = find_source(addr, id);
    if (src){
        return src->handle_stop(*this, stream);
    } else {
        return kAooErrorNotFound;
    }
}

// /aoo/sink/<id>/decline <src> <token>
AooError Sink::handle_decline_message(const osc::ReceivedMessage& msg,
                                      const ip_address& addr) {
    auto it = msg.ArgumentsBegin();

    AooId id = (it++)->AsInt32();
    AooId token = (it++)->AsInt32();

    if (id < 0){
        LOG_WARNING("AooSink: bad ID for " << kAooMsgDecline << " message");
        return kAooErrorBadFormat;
    }
    // try to find existing source
    source_lock lock(sources_);
    auto src = find_source(addr, id);
    if (src){
        return src->handle_decline(*this, token);
    } else {
        return kAooErrorNotFound;
    }
}

// /aoo/sink/<id>/data <src> <stream_id> <seq> <sr> <channel_onset>
// <totalsize> <msgsize> <nframes> <frame> <data>

AooError Sink::handle_data_message(const osc::ReceivedMessage& msg,
                                   const ip_address& addr)
{
    auto it = msg.ArgumentsBegin();

    auto id = (it++)->AsInt32();

    net_packet d;
    d.stream_id = (it++)->AsInt32();
    d.sequence = (it++)->AsInt32();
    d.samplerate = (it++)->AsDouble();
    d.channel = (it++)->AsInt32();
    d.totalsize = (it++)->AsInt32();
    d.msgsize = (it++)->AsInt32();
    d.nframes = (it++)->AsInt32();
    d.frame = (it++)->AsInt32();
    const void *blobdata;
    osc::osc_bundle_element_size_t blobsize;
    (it++)->AsBlob(blobdata, blobsize);
    d.data = (const AooByte *)blobdata;
    d.size = blobsize;
    d.flags = 0;

    return handle_data_packet(d, false, addr, id);
}

// binary data message:
// stream_id (int32), seq (int32), channel (uint8), flags (uint8), size (uint16)
// [total (int32), nframes (int16), frame (int16)], [msgsize (int32)], [sr (float64)], data...

AooError Sink::handle_data_message(const AooByte *msg, int32_t n,
                                   AooId id, const ip_address& addr)
{
    AooFlag flags;
    net_packet d;
    auto it = msg;
    auto end = it + n;

    // check basic size (stream_id, seq, channel, flags, size)
    if (n < 12){
        goto wrong_size;
    }
    d.stream_id = aoo::read_bytes<int32_t>(it);
    d.sequence = aoo::read_bytes<int32_t>(it);
    d.channel = aoo::read_bytes<uint8_t>(it);
    d.flags = aoo::read_bytes<uint8_t>(it);
    d.size = aoo::read_bytes<uint16_t>(it);
    if (d.flags & kAooBinMsgDataFrames) {
        if ((end - it) < 8) {
            goto wrong_size;
        }
        d.totalsize = aoo::read_bytes<uint32_t>(it);
        d.nframes = aoo::read_bytes<uint16_t>(it);
        d.frame = aoo::read_bytes<uint16_t>(it);
    } else {
        d.totalsize = d.size;
        d.nframes = d.size > 0;
        d.frame = 0;
    }
    if (d.flags & kAooBinMsgDataStreamMessage) {
        if ((end - it) < 4) {
            goto wrong_size;
        }
        d.msgsize = aoo::read_bytes<uint32_t>(it);
    } else {
        d.msgsize = 0;
    }
    if (d.flags & kAooBinMsgDataSampleRate) {
        if ((end - it) < 8) {
            goto wrong_size;
        }
        d.samplerate = aoo::read_bytes<double>(it);
    } else {
        d.samplerate = 0;
    }
    d.data = it;

    if ((end - it) < d.size) {
        goto wrong_size;
    }

    return handle_data_packet(d, true, addr, id);

wrong_size:
    LOG_ERROR("AooSink: binary data message too small!");
    return kAooErrorBadFormat;
}

AooError Sink::handle_data_packet(net_packet& d, bool binary,
                                  const ip_address& addr, AooId id)
{
    if (id < 0){
        LOG_WARNING("AooSink: bad ID for " << kAooMsgData << " message");
        return kAooErrorBadArgument;
    }
    // try to find existing source
    // NOTE: sources can also be added in the network send thread,
    // so we have to lock a mutex to avoid the ABA problem!
    sync::scoped_lock<sync::mutex> lock1(source_mutex_);
    source_lock lock2(sources_);
    auto src = find_source(addr, id);
    if (!src){
        src = add_source(addr, id);
    }
    return src->handle_data(*this, d, binary);
}

AooError Sink::handle_ping_message(const osc::ReceivedMessage& msg,
                                   const ip_address& addr)
{
    auto it = msg.ArgumentsBegin();

    auto id = (it++)->AsInt32();
    time_tag tt = (it++)->AsTimeTag();

    LOG_DEBUG("AooSink: handle ping");

    // try to find existing source
    source_lock lock(sources_);
    auto src = find_source(addr, id);
    if (src){
        return src->handle_ping(*this, tt);
    } else {
        LOG_WARNING("AooSink: couldn't find source " << addr << "|" << id
                    << " for " << kAooMsgPing << " message");
        return kAooErrorNotFound;
    }
}

AooError Sink::handle_pong_message(const osc::ReceivedMessage& msg,
                                   const ip_address& addr)
{
    auto it = msg.ArgumentsBegin();
    AooId id = (it++)->AsInt32();
    time_tag tt1 = (it++)->AsTimeTag();
    time_tag tt2 = (it++)->AsTimeTag();

    LOG_DEBUG("AooSink: handle pong");

    // try to find existing source
    source_lock lock(sources_);
    auto src = find_source(addr, id);
    if (src){
        return src->handle_pong(*this, tt1, tt2);
    } else {
        LOG_WARNING("AooSink: couldn't find source " << addr << "|" << id
                    << " for " << kAooMsgPong << " message");
        return kAooErrorNotFound;
    }
}

//----------------------- source_desc --------------------------//

#if AOO_NET
source_desc::source_desc(const ip_address& addr, AooId id,
                         const ip_address& relay, double time)
    : ep(addr, id, relay), last_packet_time_(time)
#else
source_desc::source_desc(const ip_address& addr, AooId id, double time)
    : ep(addr, id), last_packet_time_(time)
#endif
{
    // resendqueue_.reserve(256);
    eventbuffer_.reserve(6); // start, stop, active, inactive, overrun, underrun
    LOG_DEBUG("AooSink: source_desc");
}

source_desc::~source_desc() {
    // flush packet queue
    net_packet d;
    while (packetqueue_.try_pop(d)){
        if (d.data) {
            memory_.deallocate((void *)d.data);
        }
    }
    // flush stream message queue
    reset_stream_messages();
    LOG_DEBUG("AooSink: ~source_desc");
}

bool source_desc::check_active(const Sink& s) {
    auto elapsed = s.elapsed_time();
    // check source idle timeout
    auto delta = elapsed - last_packet_time_.load(std::memory_order_relaxed);
    if (delta > s.source_timeout()){
        return false; // source timeout
    }
    return true;
}

AooError source_desc::get_format(AooFormat &format, size_t size){
    // synchronize with handle_format() and update()!
    scoped_shared_lock lock(mutex_);
    if (format_) {
        if (size >= format_->structSize){
            memcpy(&format, format_.get(), format_->structSize);
            return kAooOk;
        } else {
            return kAooErrorBadArgument;
        }
    } else {
        return kAooErrorNotInitialized;
    }
}

AooError source_desc::codec_control(
        AooCtl ctl, void *data, AooSize size) {
    // we don't know which controls are setters and which
    // are getters, so we just take a writer lock for either way.
    unique_lock lock(mutex_);
    if (decoder_){
        return AooDecoder_control(decoder_.get(), ctl, data, size);
    } else {
        return kAooErrorNotInitialized;
    }
}

void source_desc::reset(const Sink& s){
    // take writer lock!
    scoped_lock lock(mutex_);
    update(s);
}

// always called with writer lock!
void source_desc::update(const Sink& s){
    // resize audio ring buffer
    if (format_ && format_->blockSize > 0 && format_->sampleRate > 0){
        assert(decoder_ != nullptr);
        auto convert = (double)format_->sampleRate / (double)format_->blockSize;
        // calculate latency
        auto latency = s.latency();
        int32_t latency_blocks = std::ceil(latency * convert);
        // minimum buffer size depends on resampling and reblocking!
        auto resample = (double)s.samplerate() / (double)format_->sampleRate;
        auto reblock = (double)s.blocksize() / (double)format_->blockSize;
        auto min_latency_blocks = std::ceil(reblock / resample);
        latency_blocks_ = std::max<int32_t>(latency_blocks, min_latency_blocks);
        latency_samples_ = (double)latency_blocks_ * (double)format_->blockSize * resample + 0.5;
        // calculate jitter buffer size
        auto buffersize = s.buffersize();
        if (buffersize <= 0) {
            buffersize = latency * 2; // default
        } else if (buffersize < latency) {
            LOG_VERBOSE("AooSink: buffer size (" << (buffersize * 1000)
                        << " ms) smaller than latency (" << (latency * 1000) << " ms)");
            buffersize = latency;
        }
        auto jitter_buffersize = std::max<int32_t>(latency_blocks_,
                                                   std::ceil(buffersize * convert));
        LOG_DEBUG("AooSink: latency (ms): " << (latency * 1000)
                  << ", num blocks: " << latency_blocks_
                  << ", min. blocks: " << min_latency_blocks
                  << ", jitter buffersize: " << (buffersize * 1000)
                  << ", num blocks: " << jitter_buffersize);

        reset_stream_messages();

        // setup resampler
        resampler_.setup(format_->blockSize, s.blocksize(),
                         format_->sampleRate, s.samplerate(),
                         format_->numChannels);

        // NB: the actual latency is still numbuffers_ because that is the number
        // of buffers we wait before we start decoding, see try_decode_block().
        // LATER optimize max. block size, see remark in packet_buffer.hpp.
        auto nbytes = format_->numChannels * format_->blockSize * sizeof(double);
        jitterbuffer_.resize(jitter_buffersize, nbytes);

        channel_ = 0;
        underrun_ = false;
        stopped_ = false;
        did_update_ = true;

        dropped_blocks_.store(0);
        last_ping_time_.store(-1e007); // force ping
        last_stop_time_ = 0;

        // reset decoder to avoid garbage from previous stream
        AooDecoder_reset(decoder_.get());
    }
}

void source_desc::invite(const Sink& s, AooId token, AooData *metadata){
    // always invite, even if we're already running!
    invite_token_.store(token);
    // NOTE: the metadata is only read/set in this thread (= send thread)
    // so we don't have to worry about race conditions!
    invite_metadata_.reset(metadata);

    auto elapsed = s.elapsed_time();
    // reset invite timeout, in case we're not running;
    // otherwise this won't do anything.
    last_packet_time_.store(elapsed);
#if 1
    last_invite_time_.store(0.0); // start immediately
#else
    last_invite_time_.store(elapsed); // wait
#endif
    invite_start_time_.store(elapsed);

    state_.store(source_state::invite);

    LOG_DEBUG("AooSink: source_desc: invite");
}

void source_desc::uninvite(const Sink& s){
    // only uninvite when running!
    // state can change in different threads, so we need a CAS loop
    auto state = state_.load(std::memory_order_acquire);
    while (state == source_state::run){
        // reset uninvite timeout, see handle_data()
        invite_start_time_.store(s.elapsed_time());
        if (state_.compare_exchange_weak(state, source_state::uninvite)){
            LOG_DEBUG("AooSink: source_desc: uninvite");
            return;
        }
    }
    LOG_WARNING("AooSink: couldn't uninvite source - not running");
}

float source_desc::get_buffer_fill_ratio(){
    scoped_shared_lock lock(mutex_);
    if (decoder_){
        // consider samples in resampler!
        auto nsamples = format_->numChannels * format_->blockSize;
        auto available = (double)jitterbuffer_.size() +
                (double)resampler_.size() / (double)nsamples;
        auto ratio = available / (double)jitterbuffer_.capacity();
        LOG_DEBUG("AooSink: fill ratio: " << ratio << ", jitter buffer: " << jitterbuffer_.size()
                  << ", resampler: " << (double)resampler_.size() / (double)nsamples);
        return std::min<float>(1.0, ratio);
    } else {
        return 0.0;
    }
}

void source_desc::add_xrun(double nblocks){
    xrunblocks_ += nblocks;
}

// /aoo/sink/<id>/start <src> <version> <stream_id> <lastformat>
// <nchannels> <samplerate> <blocksize> <codec> <options> <metadata>

AooError source_desc::handle_start(const Sink& s, int32_t stream, int32_t format_id,
                                   const AooFormat& f, const AooByte *extension, int32_t size,
                                   const AooData *md) {
    LOG_DEBUG("AooSink: handle start (" << stream << ")");
    auto state = state_.load(std::memory_order_acquire);
    if (state == source_state::invite) {
        // ignore /start messages that don't match the desired stream id
        if (stream != invite_token_.load()){
            LOG_DEBUG("AooSink: handle_start: doesn't match invite token");
            return kAooOk;
        }
    }
    // ignore redundant /start messages!
    // NOTE: stream_id_ can only change in this thread,
    // so we don't need a lock to safely *read* it!
    if (stream == stream_id_){
        LOG_DEBUG("AooSink: handle_start: ignore redundant /start message");
        return kAooErrorNone;
    }

    std::unique_ptr<AooFormat, format_deleter> new_format;
    std::unique_ptr<AooCodec, decoder_deleter> new_decoder;

    // ignore redundant /format messages!
    // NOTE: format_id_ is only used in this method,
    // so we don't need a lock!
    bool format_changed = format_id != format_id_;
    format_id_ = format_id;

    AooFormatStorage fmt;

    if (format_changed){
        // look up codec
        auto c = aoo::find_codec(f.codecName);
        if (!c){
            LOG_ERROR("AooSink: codec '" << f.codecName << "' not supported!");
            return kAooErrorNotImplemented;
        }

        // copy format header
        memcpy(&fmt, &f, sizeof(AooFormat));

        // try to deserialize format extension
        fmt.header.structSize = sizeof(AooFormatStorage);
        auto err = c->deserialize(extension, size, &fmt.header, &fmt.header.structSize);
        if (err != kAooOk){
            return err;
        }

        // create a new decoder - will validate format!
        auto dec = c->decoderNew(&fmt.header, &err);
        if (!dec){
            return err;
        }
        new_decoder.reset(dec);

        // save validated format
        auto fp = aoo::allocate(fmt.header.structSize);
        memcpy(fp, &fmt, fmt.header.structSize);
        new_format.reset((AooFormat *)fp);
    }

    // copy metadata
    AooData *metadata = nullptr;
    if (md) {
        assert(md->data && md->size > 0);
        LOG_DEBUG("AooSink: stream metadata: "
                  << md->type << ", " << md->size << " bytes");
        // allocate flat metadata
        auto md_size = flat_metadata_size(*md);
        metadata = (AooData *)rt_allocate(md_size);
        flat_metadata_copy(*md, *metadata);
    }

    unique_lock lock(mutex_); // writer lock!

    // NOTE: the stream ID must always be in sync with the format,
    // so we have to set it while holding the lock!
    bool first_stream = stream_id_ == kAooIdInvalid;
    stream_id_ = stream;

    // TODO handle 'flags' (future)

    if (format_changed){
        // set new format
        format_ = std::move(new_format);
        decoder_ = std::move(new_decoder);
    }

    // replace metadata
    metadata_.reset(metadata);

    // always update!
    update(s);

    lock.unlock();

    // send "add" event *before* setting the state to avoid
    // possible wrong ordering with subsequent "start" event
    if (first_stream){
        // first /start message -> source added.
        auto e = make_event<source_event>(kAooEventSourceAdd, ep);
        send_event(s, std::move(e), kAooThreadLevelNetwork);
        LOG_DEBUG("AooSink: add new source " << ep);
    }

    if (format_changed){
        // send "format" event
        auto e = make_event<format_change_event>(ep, fmt.header);
        send_event(s, std::move(e), kAooThreadLevelNetwork);
    }

    state_.store(source_state::start);

    return kAooOk;
}

// /aoo/sink/<id>/stop <src> <stream_id>

AooError source_desc::handle_stop(const Sink& s, int32_t stream) {
    LOG_DEBUG("AooSink: handle stop (" << stream << ")");
    // ignore redundant /stop messages!
    // NOTE: stream_id_ can only change in this thread,
    // so we don't need a lock to safely *read* it!
    if (stream == stream_id_){
        // check if we're already idle to avoid duplicate "stop" events
        auto state = state_.load(std::memory_order_relaxed);
        while (state != source_state::idle){
            if (state_.compare_exchange_weak(state, source_state::stop)){
                return kAooOk;
            }
        }
        LOG_DEBUG("AooSink: handle_stop: already idle");
    } else {
        LOG_DEBUG("AooSink: handle_stop: ignore redundant /stop message");
    }

    return kAooOk;
}

// /aoo/sink/<id>/decline <src> <token>

AooError source_desc::handle_decline(const Sink& s, int32_t token) {
    LOG_DEBUG("AooSink: handle decline (" << token << ")");
    // ignore /decline messages that don't match the token
    if (token != invite_token_.load()){
        LOG_DEBUG("AooSink: /decline message doesn't match invite token");
        return kAooOk;
    }

    auto expected = source_state::invite;
    if (state_.compare_exchange_strong(expected, source_state::timeout)) {
        auto e = make_event<source_event>(kAooEventInviteDecline, ep);
        s.send_event(std::move(e), kAooThreadLevelNetwork);
    } else {
        LOG_DEBUG("AooSink: received /decline while not inviting");
    }

    return kAooOk;
}

// /aoo/sink/<id>/data <src> <stream_id> <seq> <sr> <channel_onset>
// <totalsize> <msgsize> <numpackets> <packetnum> <data>

AooError source_desc::handle_data(const Sink& s, net_packet& d, bool binary)
{
    binary_.store(binary, std::memory_order_relaxed);

    // always update packet time to signify that we're receiving packets
    last_packet_time_.store(s.elapsed_time(), std::memory_order_relaxed);

    auto state = state_.load(std::memory_order_acquire);
    if (state == source_state::invite) {
        // ignore data messages that don't match the desired stream id.
        if (d.stream_id != invite_token_.load()){
            LOG_DEBUG("AooSink: /data message doesn't match invite token");
            return kAooOk;
        }
    } else if (state == source_state::uninvite) {
        // ignore data and send uninvite request. only try for a certain
        // amount of time to avoid spamming the source.
        auto delta = s.elapsed_time() - invite_start_time_.load(std::memory_order_relaxed);
        if (delta < s.invite_timeout()){
            LOG_DEBUG("AooSink: request uninvite (elapsed: " << delta << ")");
            request r(request_type::uninvite);
            r.uninvite.token = d.stream_id;
            push_request(r);
        } else {
            // transition into 'timeout' state, but only if the state
            // hasn't changed in between.
            if (state_.compare_exchange_strong(state, source_state::timeout)) {
                LOG_DEBUG("AooSink: uninvite -> timeout");
            } else {
                LOG_DEBUG("AooSink: uninvite -> timeout failed");
            }
            // always send timeout event
            LOG_VERBOSE("AooSink: " << ep << ": uninvitation timed out");
            auto e = make_event<source_event>(kAooEventUninviteTimeout, ep);
            s.send_event(std::move(e), kAooThreadLevelNetwork);
        }
        return kAooOk;
    } else if (state == source_state::timeout) {
        // we keep ignoring any data until we
        // a) invite a new stream (change to 'invite' state)
        // b) receive a new stream
        //
        // (if the user doesn't want to receive anything, they
        // would actually have to *deactivate* the source [TODO])
        if (d.stream_id == stream_id_){
            // LOG_DEBUG("AooSink: handle_data: ignore (invite timeout)");
            return kAooOk;
        }
    } else if (state == source_state::idle) {
        if (d.stream_id == stream_id_) {
            // this can happen when /data messages are reordered after
            // a /stop message.
            LOG_DEBUG("AooSink: received data message for idle stream!");
        #if 1
            // NOTE: during the 'idle' state no packets are being processed,
            // so incoming data messages would pile up indefinitely.
            return kAooOk;
        #endif
        }
    }

    // the source format might have changed and we haven't noticed,
    // e.g. because of dropped UDP packets.
    // NOTE: stream_id_ can only change in this thread!
    if (d.stream_id != stream_id_){
        LOG_DEBUG("AooSink: received data message before /start message");
        push_request(request(request_type::start));
        return kAooOk;
    }

    // synchronize with update()!
    scoped_shared_lock lock(mutex_);

#if 1
    if (!decoder_){
        LOG_DEBUG("AooSink: ignore data message");
        return kAooErrorNotInitialized;
    }
#else
    assert(decoder_ != nullptr);
#endif
    // check and fix up samplerate
    if (d.samplerate == 0){
        // no dynamic resampling, just use nominal samplerate
        d.samplerate = format_->sampleRate;
    }

    // copy blob data and push to queue
    if (d.size > 0) {
        auto data = (AooByte *)memory_.allocate(d.size);
        memcpy(data, d.data, d.size);
        d.data = data;
    } else {
        d.data = nullptr;
    }

    packetqueue_.push(d);

#if AOO_DEBUG_DATA
    LOG_DEBUG("AooSink: got block: seq = " << d.sequence << ", sr = " << d.samplerate
              << ", chn = " << d.channel << ", msgsize = " << d.msgsize
              << ", totalsize = " << d.totalsize << ", nframes = " << d.nframes
              << ", frame = " << d.frame << ", size " << d.size);
#endif

    return kAooOk;
}

// /aoo/sink/<id>/ping <src> <time>

AooError source_desc::handle_ping(const Sink& s, time_tag tt){
    LOG_DEBUG("AooSink: handle ping");

#if 1
    // only handle pings if active
    auto state = state_.load(std::memory_order_acquire);
    if (!(state == source_state::start || state == source_state::run)){
        return kAooOk;
    }
#endif

    // push pong request
    request r(request_type::pong);
    r.pong.time = tt;
    push_request(r);

    return kAooOk;
}


// /aoo/sink/<id>/pong <src> <tt1> <tt2>

AooError source_desc::handle_pong(const Sink& s, time_tag tt1, time_tag tt2){
    LOG_DEBUG("AooSink: handle ping");

#if 1
    // only handle pongs if active
    auto state = state_.load(std::memory_order_acquire);
    if (!(state == source_state::start || state == source_state::run)){
        return kAooOk;
    }
#endif

#if 0
    time_tag tt3 = s.absolute_time(); // use last stream time
#else
    time_tag tt3 = aoo::time_tag::now(); // use real system time
#endif

    // send ping event
    auto e = make_event<source_ping_event>(ep, tt1, tt2, tt3);
    s.send_event(std::move(e), kAooThreadLevelNetwork);

    return kAooOk;
}

void send_uninvitation(const Sink& s, const endpoint& ep, AooId token, const sendfn &fn);

void source_desc::send(const Sink& s, const sendfn& fn){
    // handle requests
    request r;
    while (request_queue_.try_pop(r)){
        switch (r.type){
        case request_type::pong:
            send_pong(s, r.pong.time, fn);
            break;
        case request_type::start:
            send_start_request(s, fn);
            break;
        case request_type::stop:
            send_stop_request(s, r.stop.stream, fn);
            break;
        case request_type::uninvite:
            send_uninvitation(s, ep, r.uninvite.token, fn);
            break;
        default:
            break;
        }
    }

    send_ping(s, fn);

    send_invitations(s, fn);

    send_data_requests(s, fn);
}

#define XRUN_THRESHOLD 0.1
#define STOP_INTERVAL 1.0

bool source_desc::process(const Sink& s, AooSample **buffer, int32_t nsamples,
                          AooStreamMessageHandler handler, void *user)
{
    // synchronize with update()!
    // the mutex should be uncontended most of the time.
    shared_lock lock(mutex_, sync::try_to_lock);
    if (!lock.owns_lock()) {
        if (stream_state_ != stream_state::inactive) {
            add_xrun(1);
            LOG_DEBUG("AooSink: process would block");
        }
        // how to report this to the client?
        return false;
    }

    if (!decoder_){
        return false;
    }

    // store events in buffer and only dispatch at the very end,
    // after we release the lock!
    assert(eventbuffer_.empty());

    auto state = state_.load(std::memory_order_acquire);
    // handle state transitions in a CAS loop
    while (state != source_state::run) {
        if (state == source_state::start){
            // start -> run
            if (state_.compare_exchange_weak(state, source_state::run)) {
                LOG_DEBUG("AooSink: start -> run");
            #if 1
                if (stream_state_ != stream_state::inactive) {
                    // send missing /stop message
                    auto e = make_event<stream_stop_event>(ep);
                    eventbuffer_.push_back(std::move(e));
                }
            #endif

                // *move* metadata into event
                auto e1 = make_event<stream_start_event>(ep, metadata_.release());
                eventbuffer_.push_back(std::move(e1));

                if (stream_state_ != stream_state::buffering) {
                    stream_state_ = stream_state::buffering;
                    LOG_DEBUG("AooSink: stream buffering");
                    auto e2 = make_event<stream_state_event>(ep, kAooStreamStateBuffering, 0);
                    eventbuffer_.push_back(std::move(e2));
                }

                break; // continue processing
            }
        } else if (state == source_state::stop){
            // stop -> run
            if (state_.compare_exchange_weak(state, source_state::run)) {
                LOG_DEBUG("AooSink: stop -> run");
                // wait until the buffer has run out!
                stopped_ = true;

                break; // continue processing
            }
        } else if (state == source_state::uninvite) {
            // NB: we keep processing data during the 'uninvite' state to flush
            // the packet buffer. If we receive a /stop message (= the sink has
            // accepted our uninvitation), we will enter the 'idle' state.
            // Otherwise it will eventually enter the 'timeout' state.
            break;
        } else {
            // invite, timeout or idle
            if (stream_state_ != stream_state::inactive) {
                // deactivate stream immediately
                stream_state_ = stream_state::inactive;

                lock.unlock(); // unlock before sending event!

                auto e = make_event<stream_state_event>(ep, kAooStreamStateInactive, 0);
                send_event(s, std::move(e), kAooThreadLevelAudio);
            }
            return false;
        }
    }

    if (did_update_) {
        did_update_ = false;

        xrunblocks_ = 0; // must do here!

        if (stream_state_ != stream_state::buffering) {
            stream_state_ = stream_state::buffering;
            LOG_DEBUG("AooSink: stream buffering");
            auto e = make_event<stream_state_event>(ep, kAooStreamStateBuffering, 0);
            eventbuffer_.push_back(std::move(e));
        }
    }

    stream_stats stats;

    if (!packetqueue_.empty()){
        // check for buffer underrun (only if packets arrive!)
        if (underrun_){
            handle_underrun(s);
        }

        net_packet d;
        while (packetqueue_.try_pop(d)){
            // check data packet
            add_packet(s, d, stats);
            // return memory
            if (d.data) {
                memory_.deallocate((void *)d.data);
            }
        }
    }

    check_missing_blocks(s);

#if AOO_DEBUG_JITTER_BUFFER
    LOG_ALL(jitterbuffer_);
    LOG_ALL("oldest: " << jitterbuffer_.last_popped()
            << ", newest: " << jitterbuffer_.last_pushed());
    get_buffer_fill_ratio();
#endif

    auto nchannels = format_->numChannels;
    auto outsize = nsamples * nchannels;
    assert(outsize > 0);
    auto buf = (AooSample *)alloca(outsize * sizeof(AooSample));
#if AOO_DEBUG_STREAM_MESSAGE
    LOG_ALL("AooSink: process samples: " << process_samples_
            << ", stream samples: " << stream_samples_
            << ", diff: " << (stream_samples_ - (double)process_samples_)
            << ", resampler size: " << resampler_.size());
#endif
    // try to read samples from resampler
    for (;;) {
        if (resampler_.read(buf, outsize)){
            // if there have been xruns, skip one block of audio and try again.
            if (xrunblocks_ > XRUN_THRESHOLD){
                LOG_DEBUG("AooSink: skip process block for xrun");
                // decrement xrun counter and advance process time
                xrunblocks_ -= 1.0;
                process_samples_ += nsamples;
            } else {
                // got samples
                break;
            }
        } else if (!try_decode_block(s, stats)) {
            // buffer ran out -> "inactive"
            if (stream_state_ != stream_state::inactive) {
                stream_state_ = stream_state::inactive;
                LOG_DEBUG("AooSink: stream inactive");
                // TODO: read out partial data from resampler and send sample offset
                auto e = make_event<stream_state_event>(ep, kAooStreamStateInactive, 0);
                eventbuffer_.push_back(std::move(e));
            }

            if (stopped_) {
                // we received a /stop message
                auto e = make_event<stream_stop_event>(ep);
                eventbuffer_.push_back(std::move(e));

                // try to change source state to idle (if still running!)
                auto expected = source_state::run;
                state_.compare_exchange_strong(expected, source_state::idle);
                LOG_DEBUG("AooSink: run -> idle");
            }

            underrun_ = true;
            // check if we should send a stop request
            if (last_stop_time_ > 0) {
                auto now = s.elapsed_time();
                auto delta = now - last_stop_time_;
                if (delta >= STOP_INTERVAL) {
                    request r(request_type::stop);
                    r.stop.stream = stream_id_;
                    push_request(r);
                    last_stop_time_ = now;
                }
            } else {
                // initialize stop request timer.
                // underruns are often temporary and may happen in short succession,
                // so we don't want to immediately send a stop request; instead we
                // we wait one interval as a simple debouncing mechanism.
                last_stop_time_ = s.elapsed_time();
            }

            lock.unlock(); // unlock before sending event!

            flush_event_buffer(s);

            return false;
        }
    }

    // dispatch stream messages
    auto deadline = process_samples_ + nsamples;
    while (stream_messages_) {
        auto it = stream_messages_;
        if (it->time < deadline) {
            AooStreamMessage msg;
            int32_t offset = it->time - process_samples_ + 0.5;
            if (offset >= nsamples) {
                break;
            }
            if (offset >= 0) {
                msg.sampleOffset = offset;
                msg.type = it->type;
                msg.size = it->size;
                msg.data = (const AooByte *)reinterpret_cast<flat_stream_message *>(it)->data;

                AooEndpoint ep;
                ep.address = this->ep.address.address();
                ep.addrlen = this->ep.address.length();
                ep.id = this->ep.id;

            #if AOO_DEBUG_STREAM_MESSAGE
                LOG_ALL("AooSink: dispatch stream message "
                        << "(type: " << aoo_dataTypeToString(msg.type)
                        << ", size: " << msg.size
                        << ", offset: " << msg.sampleOffset << ")");
            #endif

                // NB: the stream message handler is called with the mutex locked!
                // See the documentation of AooStreamMessageHandler.
                handler(user, &msg, &ep);
            } else {
                // this may happen with xruns
                LOG_VERBOSE("AooSink: skip stream message (offset: " << offset << ")");
            }

            auto next = it->next;
            auto alloc_size = sizeof(stream_message_header) + it->size;
            aoo::rt_deallocate(it, alloc_size);
            stream_messages_ = next;
        } else {
            break;
        }
    }
    process_samples_ = deadline;

    // sum source into sink (interleaved -> non-interleaved),
    // starting at the desired sink channel offset.
    // out of bound source channels are silently ignored.
    if (buffer) {
        auto realnchannels = s.nchannels();
        for (int i = 0; i < nchannels; ++i){
            auto chn = i + channel_;
            if (chn < realnchannels){
                auto out = buffer[chn];
                for (int j = 0; j < nsamples; ++j){
                    out[j] += buf[j * nchannels + i];
                }
            }
        }
    }

    // send events
    lock.unlock();

    flush_event_buffer(s);

    if (stats.dropped > 0){
        // add to dropped blocks for packet loss reporting
        dropped_blocks_.fetch_add(stats.dropped, std::memory_order_relaxed);
        // push block dropped event
        auto e = make_event<block_drop_event>(ep, stats.dropped);
        send_event(s, std::move(e), kAooThreadLevelAudio);
    }
    if (stats.resent > 0){
        // push block resent event
        auto e = make_event<block_resend_event>(ep, stats.resent);
        send_event(s, std::move(e), kAooThreadLevelAudio);
    }
    if (stats.xrun > 0){
        // push block xrun event
        auto e = make_event<block_xrun_event>(ep, stats.xrun);
        send_event(s, std::move(e), kAooThreadLevelAudio);
    }

    return true;
}

int32_t source_desc::poll_events(Sink& s, AooEventHandler fn, void *user){
    // always lockfree!
    int count = 0;
    event_ptr e;
    while (eventqueue_.try_pop(e)) {
        fn(user, &e->cast(), kAooThreadLevelUnknown);
        count++;
    }
    return count;
}

void source_desc::handle_underrun(const Sink& s){
    LOG_VERBOSE("AooSink: jitter buffer underrun");

    if (!jitterbuffer_.empty()) {
        LOG_ERROR("AooSink: bug: jitter buffer not empty");
    #if 1
        jitterbuffer_.clear();
    #endif
    }

    resampler_.reset(); // !

    last_ping_time_.store(-1e007); // force ping
    last_stop_time_ = 0; // reset stop request timer

    reset_stream_messages();

    auto e1 = make_event<source_event>(kAooEventBufferUnderrun, ep);
    eventbuffer_.push_back(std::move(e1));

    if (stream_state_ != stream_state::buffering) {
        stream_state_ = stream_state::buffering;
        LOG_DEBUG("AooSink: stream buffering");
        auto e2 = make_event<stream_state_event>(ep, kAooStreamStateBuffering, 0);
        eventbuffer_.push_back(std::move(e2));
    }

    underrun_ = false;
}

bool source_desc::add_packet(const Sink& s, const net_packet& d,
                             stream_stats& stats){
    // we have to check the stream_id (again) because the stream
    // might have changed in between!
    if (d.stream_id != stream_id_){
        LOG_DEBUG("AooSink: ignore data packet from previous stream");
        return false;
    }

    if (d.sequence <= jitterbuffer_.last_popped()) {
        // try to detect wrap around
        if ((jitterbuffer_.last_popped() - d.sequence) >= (INT32_MAX / 2)) {
            LOG_VERBOSE("AooSink: stream sequence has wrapped around!");
            jitterbuffer_.clear();
            // continue!
        } else {
            // block too old, discard!
            LOG_VERBOSE("AooSink: discard old block " << d.sequence);
            LOG_DEBUG("AooSink: oldest: " << jitterbuffer_.last_popped());
            return false;
        }
    }

    auto newest = jitterbuffer_.last_pushed();

    auto block = jitterbuffer_.find(d.sequence);
    if (!block){
        // add new block
    #if 1
        // can this ever happen!?
        if (d.sequence <= newest){
            LOG_VERBOSE("AooSink: discard outdated block " << d.sequence);
            LOG_DEBUG("AooSink: newest: " << newest);
            return false;
        }
    #endif
        if (newest >= 0){
            auto numblocks = newest >= 0 ? d.sequence - newest : 1;

            // notify for gap
            if (numblocks > 1){
                LOG_VERBOSE("AooSink: skipped " << (numblocks - 1) << " blocks");
            }

            // check for jitter buffer overrun.
            // can happen if the latency resp. buffer size is too small.
            auto space = jitterbuffer_.capacity() - jitterbuffer_.size();
            if (numblocks > space){
                LOG_VERBOSE("AooSink: jitter buffer overrun!");
                // reset the buffer to latency_blocks_, considering both the stored blocks
                // and the incoming block(s)
                auto excess_blocks = numblocks + jitterbuffer_.size() - latency_blocks_;
                assert(excess_blocks > 0);
                // *first* discard stored blocks
                // remove one extra block to make space for new block
                auto discard_stored = std::min(jitterbuffer_.size(), latency_blocks_ + 1);
                if (discard_stored > 0) {
                    LOG_DEBUG("AooSink: discard " << discard_stored << " stored blocks");
                    // TODO: consider popping blocks from the back of the queue
                    for (int32_t i = 0; i < discard_stored; ++i) {
                        jitterbuffer_.pop();
                    }
                }
                // then discard incoming blocks (except for the most recent one)
                auto discard_incoming = std::min(numblocks - 1, excess_blocks - discard_stored);
                if (discard_incoming > 0) {
                    LOG_DEBUG("AooSink: discard " << discard_incoming << " incoming blocks");
                    newest += discard_incoming;
                    jitterbuffer_.reset_head(); // !
                }
              #if 0
                // TODO: should we report these as lost blocks? or only the incomplete ones?
                stats.lost += excess_blocks;
              #endif
                auto e = make_event<source_event>(kAooEventBufferOverrun, ep);
                eventbuffer_.push_back(std::move(e));
            }
            // fill gaps with placeholder blocks
            for (int32_t i = newest + 1; i < d.sequence; ++i){
                jitterbuffer_.push(i)->init(i);
            }
        }

        // add new block
        block = jitterbuffer_.push(d.sequence);

        block->init(d);
    } else {
        if (block->placeholder()){
            block->init(d);
        } else if (block->empty()) {
            // empty block already received
            LOG_VERBOSE("AooSink: empty block " << d.sequence << " already received");
            return false;
        } else if (block->has_frame(d.frame)){
            // frame already received
            LOG_VERBOSE("AooSink: frame " << d.frame << " of block " << d.sequence << " already received");
            return false;
        }

        if (d.sequence != newest){
            // out of order or resent
            if (block->resend_count() > 0){
                LOG_VERBOSE("AooSink: resent frame " << d.frame << " of block " << d.sequence);
                // only record first resent frame!
                if (block->received_frames() == 0) {
                    stats.resent++;
                }
            } else {
                LOG_VERBOSE("AooSink: frame " << d.frame << " of block " << d.sequence << " out of order!");
            }
        }
    }

    // add frame to block (if not empty)
    if (d.size > 0) {
        block->add_frame(d.frame, d.data, d.size);
    }

    return true;
}

#define BUFFER_SILENT 0

#define BUFFER_BLOCKS 0
#define BUFFER_SAMPLES 1
#define BUFFER_BLOCKS_OR_SAMPLES 2
#define BUFFER_BLOCKS_AND_SAMPLES 3

// TODO: make this a compile time option, or even a runtime option?
#ifndef BUFFER_METHOD
# define BUFFER_METHOD BUFFER_BLOCKS_AND_SAMPLES
#endif

bool source_desc::try_decode_block(const Sink& s, stream_stats& stats){
    auto nsamples = format_->blockSize * format_->numChannels;

    // first handle buffering.
    if (stream_state_ == stream_state::buffering) {
        // if stopped during buffering, just fake a buffer underrun.
        if (stopped_) {
            LOG_DEBUG("AooSink: stopped during buffering");
            return false;
        }
        // (We take either the process samples or stream samples, depending on
        // which has the smaller granularity)
        auto elapsed = std::min<int32_t>(process_samples_, latency_samples_ + 0.5);
        LOG_DEBUG("AooSink: buffering ("
                  << jitterbuffer_.size() << " / " << latency_blocks_ << " blocks, "
                  << elapsed << " / " << latency_samples_ << " samples)");
    #if BUFFER_METHOD == BUFFER_BLOCKS
        // Wait until the jitter buffer has a certain number of blocks.
        if (jitterbuffer_.size() < latency_blocks_) {
    #elif BUFFER_METHOD == BUFFER_SAMPLES
        // Wait for a certain amount of time.
        // NB: with low latencies this has a tendency to get stuck in a cycle
        // of overrun and underruns...
        if (elapsed < latency_samples_) {
    #elif BUFFER_METHOD == BUFFER_BLOCKS_OR_SAMPLES
        // Wait for a certain amount of time OR until the jitter buffer has a certain
        // number of blocks or number of samples.
        if ((elapsed < latency_samples_) && (jitterbuffer_.size() < latency_blocks_)) {
    #elif BUFFER_METHOD == BUFFER_BLOCKS_AND_SAMPLES
        // Wait for a certain amount of time AND until the jitter buffer has a certain
        // number of blocks or number of samples.
        // NB: with low latencies this has a tendency to get stuck in a cycle
        // of overrun and underruns...
        if ((elapsed < latency_samples_) || (jitterbuffer_.size() < latency_blocks_)) {
    #else
        #error "unknown buffer method"
    #endif
            // HACK: stop buffering after waiting too long; this is for the case where
            // where the source stops sending data while still buffering, but we don't
            // receive a /stop message (and thus never would become 'inactive').
            if (elapsed > latency_samples_ * 4) {
                LOG_VERBOSE("AooSink: abort buffering");
                return false;
            }
            // use nominal sample rate
            resampler_.update(format_->sampleRate, s.samplerate());

            auto buffer = (AooSample *)alloca(nsamples * sizeof(AooSample));
        #if BUFFER_SILENT
            std::fill(buffer, buffer + nsamples, 0);
        #else
            // use packet loss concealment
            AooInt32 n = nsamples;
            if (AooDecoder_decode(decoder_.get(), nullptr, 0, buffer, &n) != kAooOk) {
                LOG_WARNING("AooSink: couldn't decode block!");
                // fill with zeros
                std::fill(buffer, buffer + nsamples, 0);
            }
        #endif
            // advance stream time! use nominal sample rate.
            double resample = (double)s.samplerate() / (double)format_->sampleRate;
            stream_samples_ += (double)format_->blockSize * resample;

            if (resampler_.write(buffer, nsamples)) {
                return true;
            } else {
                LOG_ERROR("AooSink: bug: couldn't write to resampler");
                // let the buffer run out
                return false;
            }
        }
    }

    if (jitterbuffer_.empty()) {
    #if 0
        LOG_DEBUG("AooSink: jitter buffer empty");
    #endif
        // buffer empty -> underrun
        return false;
    }

    if (stream_state_ != stream_state::active) {
        // first block after buffering
        stream_state_ = stream_state::active;
        int32_t offset = stream_samples_ -  (double)process_samples_ + 0.5;
        LOG_DEBUG("AooSink: stream active (offset: " << offset << ")");
        auto e = make_event<stream_state_event>(ep, kAooStreamStateActive, offset);
        eventbuffer_.push_back(std::move(e));
    }

    const AooByte *data;
    int32_t size;
    int32_t msgsize;
    double sr;
    int32_t channel;

    auto& b = jitterbuffer_.front();
    if (b.complete()){
        // block is ready
        if (b.flags & kAooBinMsgDataXRun) {
            stats.xrun++;
        }
        size = b.size();
        data = size > 0 ? b.data() : nullptr;
        msgsize = b.message_size;
        sr = b.samplerate;
        channel = b.channel;
    #if AOO_DEBUG_JITTER_BUFFER
        LOG_ALL("jitter buffer: write samples for block ("
                << b.sequence << ")");
    #endif
    } else {
        // we need audio, so we have to drop a block
        data = nullptr;
        size = msgsize = 0;
        sr = format_->sampleRate; // nominal samplerate
        channel = -1; // current channel
        stats.dropped++;
        LOG_VERBOSE("AooSink: dropped block " << b.sequence);
        LOG_DEBUG("AooSink: remaining blocks: " << jitterbuffer_.size() - 1);
    }

    // decode and push audio data to resampler
    AooInt32 n = nsamples;
    auto buffer = (AooSample *)alloca(nsamples * sizeof(AooSample));
    if (AooDecoder_decode(decoder_.get(), data + msgsize, size - msgsize, buffer, &n) != kAooOk) {
        LOG_WARNING("AooSink: couldn't decode block!");
        // decoder failed - fill with zeros
        std::fill(buffer, buffer + nsamples, 0);
    }
    assert(n == nsamples);

    if (resampler_.write(buffer, nsamples)){
        // update resampler
        // real_samplerate() is our real samplerate, sr is the real source samplerate.
        // This will dynamically adjust the resampler so that it corrects the clock
        // difference between source and sink.
        // NB: If dynamic resampling is disabled, the nominal samplerate will be used.
        resampler_.update(sr, s.real_samplerate());
        // set channel; negative = current
        if (channel >= 0){
            channel_ = channel;
        }
    } else {
        LOG_ERROR("AooSink: bug: couldn't write to resampler");
        // let the buffer run out
        jitterbuffer_.pop(); // !
        return false;
    }

    // push messages
    double resample = (double)s.samplerate() / sr;

    if (msgsize > 0) {
        int32_t num_messages = aoo::from_bytes<int32_t>(data);
        auto msgptr = data + 4;
        auto endptr = data + msgsize;
        for (int32_t i = 0; i < num_messages; ++i) {
            auto offset = aoo::read_bytes<uint16_t>(msgptr);
            auto type = aoo::read_bytes<uint16_t>(msgptr);
            auto size = aoo::read_bytes<uint32_t>(msgptr);
            auto aligned_size = (size + 3) & ~3; // aligned to 4 bytes
            if ((endptr - msgptr) < aligned_size) {
                LOG_ERROR("AooSink: stream message with bad size argument");
                break;
            }
            auto alloc_size = sizeof(stream_message_header) + size;
            auto msg = (flat_stream_message *)aoo::rt_allocate(alloc_size);
            msg->header.next = nullptr;
            msg->header.time = stream_samples_ + offset * resample;
            msg->header.type = type;
            msg->header.size = size;
        #if AOO_DEBUG_STREAM_MESSAGE
            LOG_ALL("AooSink: schedule stream message "
                    << "(type: " << aoo_dataTypeToString(type)
                    << ", size: " << size << ", offset: " << offset
                    << ", time: " << (int64_t)msg->header.time << ")");
        #endif
            memcpy(msg->data, msgptr, size);
            if (stream_messages_) {
                // append to list; LATER cache list tail
                auto it = stream_messages_;
                while (it->next) {
                    it = it->next;
                }
                it->next = &msg->header;
            } else {
                stream_messages_ = &msg->header;
            }
            msgptr += aligned_size;
        }
    }

    stream_samples_ += (double)format_->blockSize * resample;

    jitterbuffer_.pop();

    return true;
}

// /aoo/src/<id>/data <sink> <stream_id> <seq0> <frame0> <seq1> <frame1> ...

// deal with "holes" in block queue
void source_desc::check_missing_blocks(const Sink& s){
    // only check if it has more than a single pending block!
    if (jitterbuffer_.size() <= 1 || !s.resend_enabled()){
        return;
    }
    int32_t resent = 0;
    int32_t maxnumframes = s.resend_limit();
    double interval = s.resend_interval();
    double elapsed = s.elapsed_time();

    // resend incomplete blocks except for the last block
    auto n = jitterbuffer_.size() - 1;
    for (auto b = jitterbuffer_.begin(); n--; ++b){
        if (!b->complete() && b->update(elapsed, interval)){
            auto nframes = b->num_frames();

            if (b->received_frames() > 0){
                // a) only some frames missing
                // we use a frame offset + bitset to indicate which frames are missing
                for (int16_t offset = 0; offset < nframes; offset += 16){
                    uint16_t bitset = 0;
                    // fill the bitset with missing frames
                    for (int i = 0; i < 16; ++i) {
                        auto frame = offset + i;
                        if (frame >= nframes) {
                            break;
                        }
                        if (!b->has_frame(frame)){
                            if (resent < maxnumframes) {
                            #if AOO_DEBUG_RESEND
                                LOG_DEBUG("AooSink: request " << b->sequence
                                          << " (" << frame << " / " << nframes << ")");
                            #endif
                                bitset |= (uint16_t)1 << i;
                                resent++;
                            } else {
                                break;
                            }
                        }
                    }
                    if (bitset != 0) {
                        push_data_request({ b->sequence, offset, bitset });
                    }
                    if (resent >= maxnumframes) {
                        LOG_DEBUG("AooSource: resend limit reached");
                        goto resend_done;
                    }
                }
            } else {
                // b) all frames missing
                // TODO: what is the frame count of placeholder blocks!?
                if (resent + nframes <= maxnumframes){
                    push_data_request({ b->sequence, -1, 0 }); // whole block
                #if AOO_DEBUG_RESEND
                    LOG_DEBUG("AooSink: request " << b->sequence << " (all)");
                #endif
                    resent += nframes;
                } else {
                    LOG_DEBUG("AooSource: resend limit reached");
                    goto resend_done;
                }
            }
        }
    }
resend_done:

    assert(resent <= maxnumframes);
    if (resent > 0){
        LOG_DEBUG("AooSink: requested " << resent << " frames");
    }
}

// /aoo/src/<id>/ping <id> <tt1>
void source_desc::send_ping(const Sink&s, const sendfn& fn) {
    if (state_.load(std::memory_order_relaxed) != source_state::run) {
        return;
    }
    auto elapsed = s.elapsed_time();
    auto pingtime = last_ping_time_.load();
    auto interval = s.ping_interval(); // 0: no ping
    if (interval > 0 && (elapsed - pingtime) >= interval){
        auto tt = aoo::time_tag::now();
        // send ping to source
        LOG_DEBUG("AooSink: send " kAooMsgPing " to " << ep);

        char buf[AOO_MAX_PACKET_SIZE];
        osc::OutboundPacketStream msg(buf, sizeof(buf));

        const int32_t max_addr_size = kAooMsgDomainLen
                + kAooMsgSourceLen + 16 + kAooMsgPingLen;
        char address[max_addr_size];
        snprintf(address, sizeof(address), "%s/%d%s",
                 kAooMsgDomain kAooMsgSource, ep.id, kAooMsgPing);

        msg << osc::BeginMessage(address) << s.id() << osc::TimeTag(tt)
            << osc::EndMessage;

        ep.send(msg, fn);

        last_ping_time_.store(elapsed);
    }
}

// /aoo/src/<id>/pong <id> <tt1> <tt2> <packetloss>
// called without lock!
void source_desc::send_pong(const Sink &s, AooNtpTime tt1, const sendfn &fn) {
    LOG_DEBUG("AooSink: send " kAooMsgPong " to " << ep);

    time_tag tt2 = aoo::time_tag::now(); // use real system time

    // cache samplerate and blocksize
    shared_lock lock(mutex_);
    if (!format_){
        LOG_DEBUG("AooSink: send_pong: no format");
        return; // shouldn't happen
    }
    auto sr = format_->sampleRate;
    auto blocksize = format_->blockSize;
    lock.unlock();

    // get lost blocks since last pong and calculate packet loss percentage.
    auto dropped_blocks = dropped_blocks_.exchange(0);
    auto last_ping_time = std::exchange(last_ping_reply_time_, tt2);
    // NOTE: the delta can be very large for the first ping in a stream,
    // but this is not an issue because there's no packet loss anyway.
    auto delta = time_tag::duration(last_ping_time, tt2);
    float packetloss = (float)dropped_blocks * (float)blocksize / ((float)sr * delta);
    if (packetloss > 1.0){
        LOG_DEBUG("AooSink: packet loss percentage larger than 1");
        packetloss = 1.0;
    }
    LOG_DEBUG("AooSink: ping delta: " << delta << ", packet loss: " << packetloss);

    char buffer[AOO_MAX_PACKET_SIZE];
    osc::OutboundPacketStream msg(buffer, sizeof(buffer));

    // make OSC address pattern
    const int32_t max_addr_size = kAooMsgDomainLen
            + kAooMsgSourceLen + 16 + kAooMsgPongLen;
    char address[max_addr_size];
    snprintf(address, sizeof(address), "%s/%d%s",
             kAooMsgDomain kAooMsgSource, ep.id, kAooMsgPong);

    msg << osc::BeginMessage(address) << s.id()
        << osc::TimeTag(tt1) << osc::TimeTag(tt2) << packetloss
        << osc::EndMessage;

    ep.send(msg, fn);
}

// /aoo/src/<id>/start <sink> <version>
// called without lock!
void source_desc::send_start_request(const Sink& s, const sendfn& fn) {
    LOG_VERBOSE("AooSink: request " kAooMsgStart " for source " << ep);

    AooByte buf[AOO_MAX_PACKET_SIZE];
    osc::OutboundPacketStream msg((char *)buf, sizeof(buf));

    // make OSC address pattern
    const int32_t max_addr_size = kAooMsgDomainLen + kAooMsgSourceLen
                                  + 16 + kAooMsgStartLen;
    char address[max_addr_size];
    snprintf(address, sizeof(address), "%s/%d%s",
             kAooMsgDomain kAooMsgSource, ep.id, kAooMsgStart);

    msg << osc::BeginMessage(address)
        << s.id() << aoo_getVersionString()
        << osc::EndMessage;

    ep.send(msg, fn);
}

// /aoo/src/<id>/stop <sink> <stream>
// called without lock!
void source_desc::send_stop_request(const Sink& s, int32_t stream, const sendfn& fn) {
    LOG_VERBOSE("AooSink: request " kAooMsgStop " for source " << ep);

    AooByte buf[AOO_MAX_PACKET_SIZE];
    osc::OutboundPacketStream msg((char *)buf, sizeof(buf));

    // make OSC address pattern
    const int32_t max_addr_size = kAooMsgDomainLen + kAooMsgSourceLen
                                  + 16 + kAooMsgStopLen;
    char address[max_addr_size];
    snprintf(address, sizeof(address), "%s/%d%s",
             kAooMsgDomain kAooMsgSource, ep.id, kAooMsgStop);

    msg << osc::BeginMessage(address)
        << s.id() << stream
        << osc::EndMessage;

    ep.send(msg, fn);
}

// /aoo/src/<id>/data <id> <stream_id> <seq1> <frame1> <seq2> <frame2> etc.
// or
// header, stream_id (int32), count (int32),
// seq1 (int32), offset1 (int16), bitset1 (uint16), ... // offset < 0 -> all

void source_desc::send_data_requests(const Sink& s, const sendfn& fn){
    if (data_requests_.empty()){
        return;
    }

    shared_lock lock(mutex_);
    int32_t stream_id = stream_id_; // cache!
    lock.unlock();

    AooByte buf[AOO_MAX_PACKET_SIZE];

    if (binary_.load(std::memory_order_relaxed)){
        // --- binary version ---
        const int32_t maxdatasize = s.packetsize() - kBinDataHeaderSize;
        const int32_t maxrequests = maxdatasize / 8; // 2 * int32

        // write header
        auto onset = aoo::binmsg_write_header(buf, sizeof(buf), kAooMsgTypeSource,
                                              kAooBinMsgCmdData, ep.id, s.id());
        // write arguments
        auto it = buf + onset;
        aoo::write_bytes<int32_t>(stream_id, it);
        // skip 'count' field
        it += sizeof(int32_t);

        auto head = it; // cache pointer

        int32_t numrequests = 0;
        data_request r;
        while (data_requests_.try_pop(r)){
            aoo::write_bytes<int32_t>(r.sequence, it);
            aoo::write_bytes<int16_t>(r.offset, it);
            aoo::write_bytes<uint16_t>(r.bitset, it);
            if (++numrequests >= maxrequests){
                // write 'count' field
                aoo::to_bytes<int32_t>(numrequests, head - sizeof(int32_t));
            #if AOO_DEBUG_RESEND
                LOG_DEBUG("AooSink: send binary data request ("
                          << r.sequence << " " << r.offset << " " << r.bitset << ")");
            #endif
                // send it off
                ep.send(buf, it - buf, fn);
                // prepare next message (just rewind)
                it = head;
                numrequests = 0;
            }
        }

        if (numrequests > 0){
            // write 'count' field
            aoo::to_bytes(numrequests, head - sizeof(int32_t));
            // send it off
            ep.send(buf, it - buf, fn);
        }
    } else {
        // --- OSC version ---
        char buf[AOO_MAX_PACKET_SIZE];
        osc::OutboundPacketStream msg(buf, sizeof(buf));

        // make OSC address pattern
        char pattern[kDataMaxAddrSize];
        snprintf(pattern, sizeof(pattern), "%s/%d%s",
                 kAooMsgDomain kAooMsgSource, ep.id, kAooMsgData);

        const int32_t maxdatasize = s.packetsize() - kDataHeaderSize;
        const int32_t maxrequests = maxdatasize / 10; // 2 * (int32_t + typetag + padding)
        int32_t numrequests = 0;

        msg << osc::BeginMessage(pattern) << s.id() << stream_id;

        data_request r;
        while (data_requests_.try_pop(r)){
            if (r.offset < 0) {
                // request whole block
            #if AOO_DEBUG_RESEND
                LOG_DEBUG("AooSink: send data request (" << r.sequence << " -1)");
            #endif
                msg << r.sequence << (int32_t)-1;
                if (++numrequests >= maxrequests){
                    // send it off
                    msg << osc::EndMessage;

                    ep.send(msg, fn);

                    // prepare next message
                    msg.Clear();
                    msg << osc::BeginMessage(pattern) << s.id() << stream_id;
                    numrequests = 0;
                }
            } else {
                // get frames from offset and bitset
                uint16_t bitset = r.bitset;
                for (int i = 0; bitset != 0; ++i, bitset >>= 1) {
                    if (bitset & 1) {
                        auto frame = r.offset + i;
                    #if AOO_DEBUG_RESEND
                        LOG_DEBUG("AooSink: send data request ("
                                  << r.sequence << " " << frame << ")");
                    #endif
                        msg << r.sequence << frame;
                        if (++numrequests >= maxrequests){
                            // send it off
                            msg << osc::EndMessage;

                            ep.send(msg, fn);

                            // prepare next message
                            msg.Clear();
                            msg << osc::BeginMessage(pattern) << s.id() << stream_id;
                            numrequests = 0;
                        }
                    }
                }
            }
        }

        if (numrequests > 0){
            // send it off
            msg << osc::EndMessage;

            ep.send(msg, fn);
        }
    }
}

// /aoo/src/<id>/invite <sink> <stream_id> [<metadata_type> <metadata_content>]

// called without lock!
void send_invitation(const Sink& s, const endpoint& ep, AooId token,
                     const AooData *metadata, const sendfn& fn){
    char buffer[AOO_MAX_PACKET_SIZE];
    osc::OutboundPacketStream msg(buffer, sizeof(buffer));

    // make OSC address pattern
    const int32_t max_addr_size = kAooMsgDomainLen
            + kAooMsgSourceLen + 16 + kAooMsgInviteLen;
    char address[max_addr_size];
    snprintf(address, sizeof(address), "%s/%d%s",
             kAooMsgDomain kAooMsgSource, ep.id, kAooMsgInvite);

    msg << osc::BeginMessage(address) << s.id() << token;
    if (metadata){
        msg << metadata->type << osc::Blob(metadata->data, metadata->size);
    }
    msg << osc::EndMessage;

    LOG_DEBUG("AooSink: send " kAooMsgInvite " to source " << ep
              << " (" << token << ")");

    ep.send(msg, fn);
}

// /aoo/<id>/uninvite <sink>

void send_uninvitation(const Sink& s, const endpoint& ep,
                       AooId token, const sendfn &fn){
    LOG_DEBUG("AooSink: send " kAooMsgUninvite " to source " << ep);

    char buffer[AOO_MAX_PACKET_SIZE];
    osc::OutboundPacketStream msg(buffer, sizeof(buffer));

    // make OSC address pattern
    const int32_t max_addr_size = kAooMsgDomainLen
            + kAooMsgSourceLen + 16 + kAooMsgUninviteLen;
    char address[max_addr_size];
    snprintf(address, sizeof(address), "%s/%d%s",
             kAooMsgDomain kAooMsgSource, ep.id, kAooMsgUninvite);

    msg << osc::BeginMessage(address) << s.id() << token
        << osc::EndMessage;

    ep.send(msg, fn);
}

// only send every 50 ms! LATER we might make this settable
#define INVITE_INTERVAL 0.05

void source_desc::send_invitations(const Sink &s, const sendfn &fn){
    auto state = state_.load(std::memory_order_acquire);
    if (state != source_state::invite){
        return;
    }

    auto now = s.elapsed_time();
    auto delta = now - invite_start_time_.load(std::memory_order_acquire);
    if (delta >= s.invite_timeout()){
        // transition into 'timeout' state, but only if the state
        // hasn't changed in between.
        if (state_.compare_exchange_strong(state, source_state::timeout)){
            LOG_DEBUG("AooSink: send_invitation: invite -> timeout");
        } else {
            LOG_DEBUG("AooSink: send_invitation: invite -> timeout failed");
        }
        // always send timeout event
        LOG_VERBOSE("AooSink: " << ep << ": invitation timed out");
        auto e = make_event<source_event>(kAooEventInviteTimeout, ep);
        s.send_event(std::move(e), kAooThreadLevelNetwork);
    } else {
        delta = now - last_invite_time_.load(std::memory_order_relaxed);
        if (delta >= INVITE_INTERVAL){
            auto token = invite_token_.load();
            // NOTE: the metadata is only read/set in the send thread,
            // so we don't have to worry about race conditions!
            auto metadata = invite_metadata_.get();
            send_invitation(s, ep, token, metadata, fn);

            last_invite_time_.store(now);
        }
    }
}

void source_desc::reset_stream_messages() {
    for (auto it = stream_messages_; it; ){
        auto next = it->next;
        auto alloc_size = sizeof(stream_message_header) + it->size;
        aoo::rt_deallocate(it, alloc_size);
        it = next;
    }
    stream_messages_ = nullptr;
    process_samples_ = stream_samples_ = 0;
}

void source_desc::send_event(const Sink& s, event_ptr e, AooThreadLevel level){
    switch (s.event_mode()){
    case kAooEventModePoll:
        eventqueue_.push(std::move(e));
        break;
    case kAooEventModeCallback:
        s.call_event(std::move(e), level);
        break;
    default:
        break;
    }
}

void source_desc::flush_event_buffer(const Sink& s) {
    for (auto& e : eventbuffer_) {
        send_event(s, std::move(e), kAooThreadLevelAudio);
    }
    eventbuffer_.clear();
}

} // aoo
