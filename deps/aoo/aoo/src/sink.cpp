/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "sink.hpp"

#include <algorithm>
#include <cmath>

const size_t kAooEventQueueSize = 8;

//------------------------- Sink ------------------------------//

AOO_API AooSink * AOO_CALL AooSink_new(
        AooId id, AooFlag flags, AooError *err) {
    return aoo::construct<aoo::Sink>(id, flags, err);
}

aoo::Sink::Sink(AooId id, AooFlag flags, AooError *err)
    : id_(id) {
    eventqueue_.reserve(kAooEventQueueSize);
}

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
                auto mdsize = flat_metadata_size(*md);
                aoo::deallocate(md, mdsize);
            }
        }
    }
}

AOO_API AooError AOO_CALL AooSink_setup(
        AooSink *sink, AooSampleRate samplerate,
        AooInt32 blocksize, AooInt32 nchannels) {
    return sink->setup(samplerate, blocksize, nchannels);
}

AooError AOO_CALL aoo::Sink::setup(
        AooSampleRate samplerate, AooInt32 blocksize, AooInt32 nchannels){
    if (samplerate > 0 && blocksize > 0 && nchannels > 0)
    {
        if (samplerate != samplerate_ || blocksize != blocksize_ ||
            nchannels != nchannels_)
        {
            nchannels_ = nchannels;
            samplerate_ = samplerate;
            blocksize_ = blocksize;

            realsr_.store(samplerate);

            reset_sources();
        }

        // always reset timer + time DLL filter
        timer_.setup(samplerate_, blocksize_, timer_check_.load());

        return kAooOk;
    } else {
        return kAooErrorUnknown;
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
        return kAooErrorUnknown;        \
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
            // reset all sources
            reset_sources();
            // reset time DLL
            timer_.reset();
        }
        break;
    }
    // get format
    case kAooCtlGetFormat:
    {
        assert(size >= sizeof(AooFormat));
        GETSOURCEARG
        return src->get_format(as<AooFormat>(ptr));
    }
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
    // timer check
    case kAooCtlSetXRunDetection:
        CHECKARG(AooBool);
        timer_check_.store(as<AooBool>(ptr));
        break;
    case kAooCtlGetXRunDetection:
        CHECKARG(AooBool);
        as<AooBool>(ptr) = timer_check_.load();
        break;
    // dynamic resampling
    case kAooCtlSetDynamicResampling:
        CHECKARG(AooBool);
        dynamic_resampling_.store(as<AooBool>(ptr));
        timer_.reset(); // !
        break;
    case kAooCtlGetDynamicResampling:
        CHECKARG(AooBool);
        as<AooBool>(ptr) = dynamic_resampling_.load();
        break;
    // time DLL filter bandwidth
    case kAooCtlSetDllBandwidth:
    {
        CHECKARG(double);
        auto bw = std::max<double>(0, std::min<double>(1, as<float>(ptr)));
        dll_bandwidth_.store(bw);
        timer_.reset(); // will update time DLL and reset timer
        break;
    }
    case kAooCtlGetDllBandwidth:
        CHECKARG(double);
        as<double>(ptr) = dll_bandwidth_.load();
        break;
    // real samplerate
    case kAooCtlGetRealSampleRate:
        CHECKARG(double);
        as<double>(ptr) = realsr_.load();
        break;
    // packetsize
    case kAooCtlSetPacketSize:
    {
        CHECKARG(int32_t);
        const int32_t minpacketsize = 64;
        auto packetsize = as<int32_t>(ptr);
        if (packetsize < minpacketsize){
            LOG_WARNING("packet size too small! setting to " << minpacketsize);
            packetsize_.store(minpacketsize);
        } else if (packetsize > AOO_MAX_PACKET_SIZE){
            LOG_WARNING("packet size too large! setting to " << AOO_MAX_PACKET_SIZE);
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
#if USE_AOO_NET
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
        return kAooErrorUnknown; // not setup yet
    }

    AooMsgType type;
    AooId sinkid;
    AooInt32 onset;
    auto err = aoo_parsePattern(data, size, &type, &sinkid, &onset);
    if (err != kAooOk){
        LOG_WARNING("not an AOO message!");
        return kAooErrorUnknown;
    }

    if (type != kAooTypeSink){
        LOG_WARNING("not a sink message!");
        return kAooErrorUnknown;
    }
    if (sinkid != id()){
        LOG_WARNING("wrong sink ID!");
        return kAooErrorUnknown;
    }

    ip_address addr((const sockaddr *)address, addrlen);

    if (data[0] == 0){
        // binary message
        auto cmd = aoo::from_bytes<int16_t>(data + kAooBinMsgDomainSize + 2);
        switch (cmd){
        case kAooBinMsgCmdData:
            return handle_data_message(data + onset, size - onset, addr);
        default:
            return kAooErrorUnknown;
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
            } else if (!strcmp(pattern, kAooMsgData)){
                return handle_data_message(msg, addr);
            } else if (!strcmp(pattern, kAooMsgPing)){
                return handle_ping_message(msg, addr);
            } else {
                LOG_WARNING("unknown message " << pattern);
            }
        } catch (const osc::Exception& e){
            LOG_ERROR("AooSink: exception in handle_message: " << e.what());
        }
        return kAooErrorUnknown;
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
    lock.unlock();

    // free unused source_descs
    if (!sources_.try_free()){
        // LOG_DEBUG("AooSink: try_free() would block");
    }

    return kAooOk;
}

AOO_API AooError AOO_CALL AooSink_process(
        AooSink *sink, AooSample **data, AooInt32 nsamples, AooNtpTime t) {
    return sink->process(data, nsamples, t);
}

AooError AOO_CALL aoo::Sink::process(
        AooSample **data, AooInt32 nsamples, AooNtpTime t){
    // clear outputs
    for (int i = 0; i < nchannels_; ++i){
        std::fill(data[i], data[i] + nsamples, 0);
    }

    // update timer
    // always do this, even if there are no sources!
    bool dynamic_resampling = dynamic_resampling_.load();
    double error;
    auto state = timer_.update(t, error);
    if (state == timer::state::reset){
        LOG_DEBUG("setup time DLL filter for sink");
        auto bw = dll_bandwidth_.load();
        dll_.setup(samplerate_, blocksize_, bw, 0);
        realsr_.store(samplerate_);
    } else if (state == timer::state::error){
        // recover sources
        int32_t xrunsamples = error * samplerate_ + 0.5;

        // no lock needed - sources are only removed in this thread!
        for (auto& s : sources_){
            s.add_xrun(xrunsamples);
        }

        endpoint_event e(kAooEventXRun);
        e.xrun.count = (float)xrunsamples / (float)blocksize_;
        send_event(e, kAooThreadLevelAudio);

        timer_.reset();
    } else if (dynamic_resampling) {
        // update time DLL, but only if n matches blocksize!
        auto elapsed = timer_.get_elapsed();
        if (nsamples == blocksize_){
            dll_.update(elapsed);
        #if AOO_DEBUG_DLL
            LOG_ALL("time elapsed: " << elapsed << ", period: "
                    << dll_.period() << ", samplerate: " << dll_.samplerate());
        #endif
        } else {
            // reset time DLL with nominal samplerate
            auto bw = dll_bandwidth_.load();
            dll_.setup(samplerate_, blocksize_, bw, elapsed);
        }
        realsr_.store(dll_.samplerate());
    }

    bool didsomething = false;

    // no lock needed - sources are only removed in this thread!
    for (auto it = sources_.begin(); it != sources_.end();){
        if (it->process(*this, data, nsamples)){
            didsomething = true;
        } else if (!it->check_active(*this)){
            LOG_VERBOSE("AooSink: removed inactive source " << it->ep);
            endpoint_event e(kAooEventSourceRemove, it->ep);
            send_event(e, kAooThreadLevelAudio);
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
    endpoint_event e;
    while (eventqueue_.try_pop(e)){
        eventhandler_(eventcontext_, &e.event, kAooThreadLevelUnknown);
        total++;
    }
    // we only need to protect against source removal
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
        AooSink *sink, const AooEndpoint *source, const AooDataView *metadata)
{
    if (sink) {
        return sink->inviteSource(*source, metadata);
    } else {
        return kAooErrorBadArgument;
    }
}

AooError AOO_CALL aoo::Sink::inviteSource(
        const AooEndpoint& ep, const AooDataView *md) {
    ip_address addr((const sockaddr *)ep.address, ep.addrlen);

    AooDataView *metadata = nullptr;
    if (md) {
        auto mdsize = flat_metadata_size(*md);
        metadata = (AooDataView *)aoo::allocate(mdsize);
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

void Sink::send_event(const endpoint_event &e, AooThreadLevel level) const {
    switch (eventmode_){
    case kAooEventModePoll:
        eventqueue_.push(e);
        break;
    case kAooEventModeCallback:
        eventhandler_(eventcontext_, &e.event, level);
        break;
    default:
        break;
    }
}

// only called if mode is kAooEventModeCallback
void Sink::call_event(const source_event &e, AooThreadLevel level) const {
    eventhandler_(eventcontext_, &e.event, level);
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
            // NOTE that sources can also be added in the network
            // receive thread (see handle_data() or handle_format()),
            // so we have to lock a mutex to avoid the ABA problem.
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
                LOG_WARNING("can't uninvite - source not found");
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

source_desc * Sink::add_source(const ip_address& addr, AooId id){
    // add new source
    uint32_t flags = 0;
#if USE_AOO_NET
    // check if the peer needs to be relayed
    if (client_){
        AooEndpoint ep { addr.address(), (AooAddrSize)addr.length(), id };
        AooBool relay;
        if (client_->control(kAooCtlNeedRelay,
                             reinterpret_cast<intptr_t>(&ep),
                             &relay, sizeof(relay)) == kAooOk)
        {
            if (relay == kAooTrue){
                LOG_DEBUG("source " << addr << " needs to be relayed");
                flags |= kAooEndpointRelay;
            }
        }
    }
#endif
    auto it = sources_.emplace_front(addr, id, flags, elapsed_time());
    return &(*it);
}

void Sink::reset_sources(){
    source_lock lock(sources_);
    for (auto& src : sources_){
        src.reset(*this);
    }
}

// /aoo/sink/<id>/start <src> <version> <stream_id> <flags> <lastformat>
// <nchannels> <samplerate> <blocksize> <codec> <options>
// [<metadata_type> <metadata_content>]
AooError Sink::handle_start_message(const osc::ReceivedMessage& msg,
                                    const ip_address& addr)
{
    auto it = msg.ArgumentsBegin();

    AooId id = (it++)->AsInt32();
    int32_t version = (it++)->AsInt32();

    // LATER handle this in the source_desc (e.g. ignoring further messages)
    if (!check_version(version)){
        LOG_ERROR("AooSink: source version not supported");
        return kAooErrorUnknown;
    }

    AooId stream = (it++)->AsInt32();
    AooFlag flags = (it++)->AsInt32();
    AooId lastformat = (it++)->AsInt32();

    // get stream format
    AooFormat f;
    f.numChannels = (it++)->AsInt32();
    f.sampleRate = (it++)->AsInt32();
    f.blockSize = (it++)->AsInt32();
    snprintf(f.codec, sizeof(f.codec), "%s", (it++)->AsString());
    f.size = sizeof(AooFormat);

    const void *extension;
    osc::osc_bundle_element_size_t size;
    (it++)->AsBlob(extension, size);

    // get stream metadata
    AooDataView md;
    if (msg.ArgumentCount() >= 12) {
        md.type = (it++)->AsString();
        const void *md_data;
        osc::osc_bundle_element_size_t md_size;
        (it++)->AsBlob(md_data, md_size);
        md.data = (const AooByte *)md_data;
        md.size = md_size;
    } else {
        md.type = kAooDataTypeInvalid;
        md.data = nullptr;
        md.size = 0;
    }

    if (id < 0){
        LOG_WARNING("bad ID for " << kAooMsgStart << " message");
        return kAooErrorUnknown;
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
    return src->handle_start(*this, stream, flags, lastformat, f,
                             (const AooByte *)extension, size, md);
}

// /aoo/sink/<id>/stop <src> <stream>
AooError Sink::handle_stop_message(const osc::ReceivedMessage& msg,
                                   const ip_address& addr) {
    auto it = msg.ArgumentsBegin();

    AooId id = (it++)->AsInt32();
    AooId stream = (it++)->AsInt32();

    if (id < 0){
        LOG_WARNING("bad ID for " << kAooMsgStop << " message");
        return kAooErrorUnknown;
    }
    // try to find existing source
    source_lock lock(sources_);
    auto src = find_source(addr, id);
    if (src){
        return src->handle_stop(*this, stream);
    } else {
        return kAooErrorUnknown;
    }
}

AooError Sink::handle_data_message(const osc::ReceivedMessage& msg,
                                   const ip_address& addr)
{
    auto it = msg.ArgumentsBegin();

    auto id = (it++)->AsInt32();

    aoo::net_packet d;
    d.stream_id = (it++)->AsInt32();
    d.sequence = (it++)->AsInt32();
    d.samplerate = (it++)->AsDouble();
    d.channel = (it++)->AsInt32();
    d.totalsize = (it++)->AsInt32();
    d.nframes = (it++)->AsInt32();
    d.frame = (it++)->AsInt32();
    const void *blobdata;
    osc::osc_bundle_element_size_t blobsize;
    (it++)->AsBlob(blobdata, blobsize);
    d.data = (const AooByte *)blobdata;
    d.size = blobsize;

    return handle_data_packet(d, false, addr, id);
}

// binary data message:
// id (int32), stream_id (int32), seq (int32), channel (int16), flags (int16),
// [total (int32), nframes (int16), frame (int16)],  [sr (float64)],
// size (int32), data...

AooError Sink::handle_data_message(const AooByte *msg, int32_t n,
                                   const ip_address& addr)
{
    // check size (excluding samplerate, frames and data)
    if (n < 20){
        LOG_ERROR("handle_data_message: header too small!");
        return kAooErrorUnknown;
    }

    auto it = msg;

    auto id = aoo::read_bytes<int32_t>(it);

    aoo::net_packet d;
    d.stream_id = aoo::read_bytes<int32_t>(it);
    d.sequence = aoo::read_bytes<int32_t>(it);
    d.channel = aoo::read_bytes<int16_t>(it);
    auto flags = aoo::read_bytes<int16_t>(it);
    if (flags & kAooBinMsgDataFrames){
        d.totalsize = aoo::read_bytes<int32_t>(it);
        d.nframes = aoo::read_bytes<int16_t>(it);
        d.frame = aoo::read_bytes<int16_t>(it);
    } else {
        d.totalsize = 0;
        d.nframes = 1;
        d.frame = 0;
    }
    if (flags & kAooBinMsgDataSampleRate){
        d.samplerate = aoo::read_bytes<double>(it);
    } else {
        d.samplerate = 0;
    }

    d.size = aoo::read_bytes<int32_t>(it);
    if (d.totalsize == 0){
        d.totalsize = d.size;
    }

    if (n < ((it - msg) + d.size)){
        LOG_ERROR("handle_data_bin_message: wrong data size!");
        return kAooErrorUnknown;
    }

    d.data = it;

    return handle_data_packet(d, true, addr, id);
}

AooError Sink::handle_data_packet(net_packet& d, bool binary,
                                  const ip_address& addr, AooId id)
{
    if (id < 0){
        LOG_WARNING("bad ID for " << kAooMsgData << " message");
        return kAooErrorUnknown;
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

    if (id < 0){
        LOG_WARNING("bad ID for " << kAooMsgPing << " message");
        return kAooErrorUnknown;
    }
    // try to find existing source
    source_lock lock(sources_);
    auto src = find_source(addr, id);
    if (src){
        return src->handle_ping(*this, tt);
    } else {
        LOG_WARNING("couldn't find source " << addr << "|" << id
                    << " for " << kAooMsgPing << " message");
        return kAooErrorUnknown;
    }
}

//----------------------- source_desc --------------------------//

source_desc::source_desc(const ip_address& addr, AooId id,
                         uint32_t flags, double time)
    : ep(addr, id, flags), last_packet_time_(time)
{
    // reserve some memory, so we don't have to allocate memory
    // when pushing events in the audio thread.
    eventqueue_.reserve(kAooEventQueueSize);
    // resendqueue_.reserve(256);
    LOG_DEBUG("source_desc");
}

source_desc::~source_desc(){
    // flush event queue
    source_event e;
    while (eventqueue_.try_pop(e)){
        free_event(e);
    }
    // flush packet queue
    net_packet d;
    while (packetqueue_.try_pop(d)){
        memory_.deallocate((void *)d.data);
    }
    // free metadata
    if (metadata_){
        memory_.deallocate((void *)metadata_);
    }
    LOG_DEBUG("~source_desc");
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

AooError source_desc::get_format(AooFormat &format){
    // synchronize with handle_format() and update()!
    scoped_shared_lock lock(mutex_);
    if (format_){
        if (format.size >= format_->size){
            memcpy(&format, format_.get(), format_->size);
            return kAooOk;
        } else {
            return kAooErrorBadArgument;
        }
    } else {
        return kAooErrorUnknown;
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
        return kAooErrorUnknown;
    }
}

void source_desc::reset(const Sink& s){
    // take writer lock!
    scoped_lock lock(mutex_);
    update(s);
}

#define MAXHWBUFSIZE 2048
#define MINSAMPLERATE 44100

void source_desc::update(const Sink& s){
    // resize audio ring buffer
    if (format_ && format_->blockSize > 0 && format_->sampleRate > 0){
        assert(decoder_ != nullptr);
        // recalculate buffersize from seconds to samples
        int32_t bufsize = s.buffersize() * format_->sampleRate;
        // number of buffers (round up!)
        int32_t nbuffers = std::ceil((double)bufsize / (double)format_->blockSize);
        // minimum buffer size depends on resampling and reblocking!
        auto downsample = (double)format_->sampleRate / (double)s.samplerate();
        auto reblock = (double)s.blocksize() / (double)format_->blockSize;
        minblocks_ = std::ceil(downsample * reblock);
        nbuffers = std::max<int32_t>(nbuffers, minblocks_);
        LOG_DEBUG("source_desc: buffersize (ms): " << (s.buffersize() * 1000)
                  << ", samples: " << bufsize << ", nbuffers: " << nbuffers
                  << ", minimum: " << minblocks_);

    #if 0
        // don't touch the event queue once constructed
        eventqueue_.reset();
    #endif

        auto nsamples = format_->numChannels * format_->blockSize;
        double sr = format_->sampleRate; // nominal samplerate

        // setup audio buffer
        auto nbytes = sizeof(block_data::header) + nsamples * sizeof(AooSample);
        // align to 8 bytes
        nbytes = (nbytes + 7) & ~7;
        audioqueue_.resize(nbytes, nbuffers);
    #if 1
        audioqueue_.shrink_to_fit();
    #endif
        // fill buffer
        for (int i = 0; i < nbuffers; ++i){
            auto b = (block_data *)audioqueue_.write_data();
            // push nominal samplerate, channel + silence
            b->header.samplerate = sr;
            b->header.channel = 0;
            std::fill(b->data, b->data + nsamples, 0);
            audioqueue_.write_commit();
        }

        // setup resampler
        resampler_.setup(format_->blockSize, s.blocksize(),
                         format_->sampleRate, s.samplerate(),
                         format_->numChannels);

        // setup jitter buffer.
        // if we use a very small audio buffer size, we have to make sure that
        // we have enough space in the jitter buffer in case the source uses
        // a larger hardware buffer size and consequently sends packets in batches.
        // we don't know the actual source samplerate and hardware buffer size,
        // so we have to make a pessimistic guess.
        auto hwsamples = (double)format_->sampleRate / MINSAMPLERATE * MAXHWBUFSIZE;
        auto minbuffers = std::ceil(hwsamples / (double)format_->blockSize);
        auto jitterbufsize = std::max<int32_t>(nbuffers, minbuffers);
        // LATER optimize max. block size
        jitterbuffer_.resize(jitterbufsize, nsamples * sizeof(double));
        LOG_DEBUG("jitter buffer: " << jitterbufsize << " blocks");

        lost_blocks_.store(0);
        channel_ = 0;
        skipblocks_ = 0;
        underrun_ = false;
        didupdate_ = true;

        // reset decoder to avoid garbage from previous stream
        AooDecoder_control(decoder_.get(), kAooCodecCtlReset, nullptr, 0);
    }
}

void source_desc::invite(const Sink& s, AooId token, AooDataView *metadata){
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

    LOG_DEBUG("source_desc: invite");
}

void source_desc::uninvite(const Sink& s){
    // only uninvite when running!
    // state can change in different threads, so we need a CAS loop
    auto state = state_.load(std::memory_order_acquire);
    while (state == source_state::run){
        // reset uninvite timeout, see handle_data()
        invite_start_time_.store(s.elapsed_time());
        if (state_.compare_exchange_weak(state, source_state::uninvite)){
            LOG_DEBUG("source_desc: uninvite");
            return;
        }
    }
    LOG_WARNING("couldn't uninvite source - not running");
}

double source_desc::get_buffer_fill_ratio(){
    scoped_shared_lock lock(mutex_);
    if (decoder_){
        // consider samples in resampler!
        auto nsamples = format_->numChannels * format_->blockSize;
        auto available = (double)audioqueue_.read_available() +
                (double)resampler_.size() / (double)nsamples;
        auto ratio = available / (double)audioqueue_.capacity();
        LOG_DEBUG("fill ratio: " << ratio << ", audioqueue: " << audioqueue_.read_available()
                  << ", resampler: " << (double)resampler_.size() / (double)nsamples);
        // FIXME sometimes the result is bigger than 1.0
        return std::min<double>(1.0, ratio);
    } else {
        return 0.0;
    }
}

// /aoo/sink/<id>/start <src> <version> <stream_id> <flags>
// <lastformat> <nchannels> <samplerate> <blocksize> <codec> <options>

AooError source_desc::handle_start(const Sink& s, int32_t stream, uint32_t flags,
                                   int32_t format_id, const AooFormat& f,
                                   const AooByte *extension, int32_t size,
                                   const AooDataView& md) {
    LOG_DEBUG("handle start (" << stream << ")");
    auto state = state_.load(std::memory_order_acquire);
    if (state == source_state::invite) {
        // ignore /start messages that don't match the desired stream id
        if (stream != invite_token_.load()){
            LOG_DEBUG("handle_start: passed streamid " << stream << " doesn't match invite token " << invite_token_.load());
            return kAooOk;
        }
    }
    // ignore redundant /start messages!
    // NOTE: stream_id_ can only change in this thread,
    // so we don't need a lock to safely *read* it!
    if (stream == stream_id_){
        LOG_DEBUG("handle_start: ignore redundant /start message");
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
        auto c = aoo::find_codec(f.codec);
        if (!c){
            LOG_ERROR("codec '" << f.codec << "' not supported!");
            return kAooErrorUnknown;
        }

        // copy format header
        memcpy(&fmt, &f, sizeof(AooFormat));

        // try to deserialize format extension
        fmt.header.size = sizeof(AooFormatStorage);
        auto err = c->deserialize(extension, size, &fmt.header, &fmt.header.size);
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
        auto fp = aoo::allocate(fmt.header.size);
        memcpy(fp, &fmt, fmt.header.size);
        new_format.reset((AooFormat *)fp);
    }

    // copy metadata
    AooDataView *metadata = nullptr;
    if (md.data){
        assert(md.size > 0);
        LOG_DEBUG("stream metadata: "
                  << md.type << ", " << md.size << " bytes");
        // allocate flat metadata
        auto mdsize = flat_metadata_size(md);
        metadata = (AooDataView *)memory_.allocate(mdsize);
        flat_metadata_copy(md, *metadata);
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

    // free old metadata
    if (metadata_){
        memory_.deallocate((void *)metadata_);
    }
    // set new metadata (can be NULL!)
    metadata_ = metadata;

    // always update!
    update(s);

    state_.store(source_state::start);

    lock.unlock();

    // send "add" event *before* setting the state to avoid
    // possible wrong ordering with subsequent "start" event
    if (first_stream){
        // first /start message -> source added.
        source_event e(kAooEventSourceAdd, ep);
        send_event(s, e, kAooThreadLevelNetwork);
        LOG_DEBUG("add new source " << ep);
    }

    if (format_changed){
        // send "format" event
        source_event e(kAooEventFormatChange, ep);

        auto mode = s.event_mode();
        if (mode == kAooEventModeCallback){
            // use stack
            e.format.format = &fmt.header;
        } else if (kAooEventModePoll){
            // use heap
            auto fp = (AooFormat *)memory_.allocate(fmt.header.size);
            memcpy(fp, &fmt, fmt.header.size);
            e.format.format = fp;
        }

        send_event(s, e, kAooThreadLevelNetwork);
    }

    return kAooOk;
}

// /aoo/sink/<id>/stop <src> <stream_id>

AooError source_desc::handle_stop(const Sink& s, int32_t stream) {
    LOG_DEBUG("handle stop (" << stream << ")");
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
        LOG_DEBUG("handle_stop: already idle");
    } else {
        LOG_DEBUG("handle_stop: ignore redundant /stop message");
    }

    return kAooOk;
}

// /aoo/sink/<id>/data <src> <stream_id> <seq> <sr> <channel_onset> <totalsize> <numpackets> <packetnum> <data>

AooError source_desc::handle_data(const Sink& s, net_packet& d, bool binary)
{
    binary_.store(binary, std::memory_order_relaxed);

    // always update packet time to signify that we're receiving packets
    last_packet_time_.store(s.elapsed_time(), std::memory_order_relaxed);

    auto state = state_.load(std::memory_order_acquire);
    if (state == source_state::invite) {
        // ignore data messages that don't match the desired stream id.
        if (d.stream_id != invite_token_.load()){
            LOG_DEBUG("handle_data: passed streamid " << d.stream_id << " doesn't match invite token " << invite_token_.load());
            return kAooOk;
        }
    } else if (state == source_state::uninvite) {
        // ignore data and send uninvite request. only try for a certain
        // amount of time to avoid spamming the source.
        auto delta = s.elapsed_time() - invite_start_time_.load(std::memory_order_relaxed);
        if (delta < s.invite_timeout()){
            LOG_DEBUG("handle data: uninvite (elapsed: " << delta << ")");
            request r(request_type::uninvite);
            r.uninvite.token = d.stream_id;
            push_request(r);
        } else {
            // transition into 'timeout' state, but only if the state
            // hasn't changed in between.
            if (state_.compare_exchange_strong(state, source_state::timeout)) {
                LOG_DEBUG("handle data: uninvite -> timeout");
            } else {
                LOG_DEBUG("handle data: uninvite -> timeout failed");
            }
            // always send timeout event
            LOG_VERBOSE(ep << ": uninvitation timed out");
            endpoint_event e(kAooEventUninviteTimeout, ep);
            s.send_event(e, kAooThreadLevelNetwork);
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
            // LOG_DEBUG("handle_data: ignore (invite timeout)");
            return kAooOk;
        }
    } else if (state == source_state::idle) {
        if (d.stream_id == stream_id_) {
            // this can happen when /data messages are reordered after
            // a /stop message.
            LOG_DEBUG("received data message for idle stream!");
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
        LOG_DEBUG("received data message before /start message");
        push_request(request(request_type::start));
        return kAooOk;
    }

    // synchronize with update()!
    scoped_shared_lock lock(mutex_);

#if 1
    if (!decoder_){
        LOG_DEBUG("ignore data message");
        return kAooErrorUnknown;
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
    auto data = (AooByte *)memory_.allocate(d.size);
    memcpy(data, d.data, d.size);
    d.data = data;

    packetqueue_.push(d);

#if AOO_DEBUG_DATA
    LOG_DEBUG("got block: seq = " << d.sequence << ", sr = " << d.samplerate
              << ", chn = " << d.channel << ", totalsize = " << d.totalsize
              << ", nframes = " << d.nframes << ", frame = " << d.frame << ", size " << d.size);
#endif

    return kAooOk;
}

// /aoo/sink/<id>/ping <src> <time>

AooError source_desc::handle_ping(const Sink& s, time_tag tt){
    LOG_DEBUG("handle ping");

#if 1
    // only handle pings if active
    auto state = state_.load(std::memory_order_acquire);
    if (!(state == source_state::start || state == source_state::run)){
        return kAooOk;
    }
#endif

#if 0
    time_tag tt2 = s.absolute_time(); // use last stream time
#else
    time_tag tt2 = aoo::time_tag::now(); // use real system time
#endif

    // push ping reply request
    request r(request_type::ping_reply);
    r.ping.tt1 = tt;
    r.ping.tt2 = tt2;
    push_request(r);

    // push "ping" event
    source_event e(kAooEventPing, ep);
    e.ping.t1 = tt;
    e.ping.t2 = tt2;
    send_event(s, e, kAooThreadLevelNetwork);

    return kAooOk;
}

void send_uninvitation(const Sink& s, const endpoint& ep,
                       AooId token, const sendfn &fn);

void source_desc::send(const Sink& s, const sendfn& fn){
    request r;
    while (requestqueue_.try_pop(r)){
        switch (r.type){
        case request_type::ping_reply:
            send_ping_reply(s, r.ping.tt1, r.ping.tt2, fn);
            break;
        case request_type::start:
            send_start_request(s, fn);
            break;
        case request_type::uninvite:
            send_uninvitation(s, ep, r.uninvite.token, fn);
            break;
        default:
            break;
        }
    }

    send_invitations(s, fn);

    send_data_requests(s, fn);
}

#define XRUN_THRESHOLD 0.1

// TODO: make sure not to send events while holding a lock!

bool source_desc::process(const Sink& s, AooSample **buffer, int32_t nsamples)
{
    // synchronize with update()!
    // the mutex should be uncontended most of the time.
    shared_lock lock(mutex_, sync::try_to_lock);
    if (!lock.owns_lock()) {
        if (streamstate_ == kAooStreamStateActive) {
            xrun_ += 1.0;
            LOG_VERBOSE("AooSink: source_desc::process() would block");
        } else {
            // I'm not sure if this can happen...
        }
        // how to report this to the client?
        return false;
    }

    if (!decoder_){
        return false;
    }

    auto state = state_.load(std::memory_order_acquire);
    // handle state transitions in a CAS loop
    while (state != source_state::run) {
        if (state == source_state::start){
            // start -> run
            if (state_.compare_exchange_weak(state, source_state::run)) {
                LOG_DEBUG("start -> run");
                if (streamstate_ == kAooStreamStateActive){
                #if 0
                    streamstate_ = kAooStreamStateInactive;
                #endif
                    // send missing /stop message
                    source_event e(kAooEventStreamStop, ep);
                    send_event(s, e, kAooThreadLevelAudio);
                }

                source_event e(kAooEventStreamStart, ep);
                // move metadata into event
                e.stream_start.metadata = metadata_;
                metadata_ = nullptr;

                send_event(s, e, kAooThreadLevelAudio);

                // deallocate metadata if we don't need it anymore, see also poll_events().
                if (s.event_mode() != kAooEventModePoll && e.stream_start.metadata){
                    memory_.deallocate((void *)e.stream_start.metadata);
                }

                // stream state is handled at the end of the function

                break; // continue processing
            }
        } else if (state == source_state::stop){
            // stop -> idle
            if (state_.compare_exchange_weak(state, source_state::idle)) {
                lock.unlock(); // !

                LOG_DEBUG("stop -> idle");

                if (streamstate_ != kAooStreamStateInactive){
                    streamstate_ = kAooStreamStateInactive;

                    source_event e(kAooEventStreamState, ep);
                    e.stream_state.state = kAooStreamStateInactive;
                    send_event(s, e, kAooThreadLevelAudio);
                }

                source_event e(kAooEventStreamStop, ep);
                send_event(s, e, kAooThreadLevelAudio);

                return false;
            }
        } else {
            // invite, uninvite, timeout or idle
            if (streamstate_ != kAooStreamStateInactive){
                lock.unlock(); // !

                streamstate_ = kAooStreamStateInactive;

                source_event e(kAooEventStreamState, ep);
                e.stream_state.state = kAooStreamStateInactive;
                send_event(s, e, kAooThreadLevelAudio);
            }

            return false;
        }
    }

    // check for sink xruns
    if (didupdate_){
        xrunsamples_ = 0;
        xrun_ = 0;
        assert(underrun_ == false);
        assert(skipblocks_ == 0);
        didupdate_ = false;
    } else if (xrunsamples_ > 0) {
        auto xrunblocks = (float)xrunsamples_ / (float)format_->blockSize;
        xrun_ += xrunblocks;
        xrunsamples_ = 0;
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
            memory_.deallocate((void *)d.data);
        }
    }

    if (skipblocks_ > 0){
        skip_blocks(s);
    }

    process_blocks(s, stats);

    check_missing_blocks(s);

#if AOO_DEBUG_JITTER_BUFFER
    LOG_ALL(jitterbuffer_);
    LOG_ALL("oldest: " << jitterbuffer_.last_popped()
            << ", newest: " << jitterbuffer_.last_pushed());
#endif

    if (stats.lost > 0){
        // push packet loss event
        source_event e(kAooEventBlockLost, ep);
        e.block_lost.count = stats.lost;
        send_event(s, e, kAooThreadLevelAudio);
    }
    if (stats.reordered > 0){
        // push packet reorder event
        source_event e(kAooEventBlockReordered, ep);
        e.block_reordered.count = stats.reordered;
        send_event(s, e, kAooThreadLevelAudio);
    }
    if (stats.resent > 0){
        // push packet resend event
        source_event e(kAooEventBlockResent, ep);
        e.block_resent.count = stats.resent;
        send_event(s, e, kAooThreadLevelAudio);
    }
    if (stats.dropped > 0){
        // push packet resend event
        source_event e(kAooEventBlockDropped, ep);
        e.block_dropped.count = stats.dropped;
        send_event(s, e, kAooThreadLevelAudio);
    }

    auto nchannels = format_->numChannels;
    auto insize = format_->blockSize * nchannels;
    auto outsize = nsamples * nchannels;
    // if dynamic resampling is disabled, this will simply
    // return the nominal samplerate
    double sr = s.real_samplerate();

#if AOO_DEBUG_AUDIO_BUFFER
    // will print audio buffer and resampler balance
    get_buffer_fill_ratio();
#endif

    // try to read samples from resampler
    auto buf = (AooSample *)alloca(outsize * sizeof(AooSample));

    while (!resampler_.read(buf, outsize)){
        // try to write samples from buffer into resampler
        if (audioqueue_.read_available()){
            auto d = (block_data *)audioqueue_.read_data();

            if (xrun_ > XRUN_THRESHOLD){
                // skip audio and decrement xrun counter proportionally
                xrun_ -= sr / format_->sampleRate;
            } else {
                // try to write audio into resampler
                if (resampler_.write(d->data, insize)){
                    // update resampler
                    resampler_.update(d->header.samplerate, sr);
                    // set channel; negative = current
                    if (d->header.channel >= 0){
                        channel_ = d->header.channel;
                    }
                } else {
                    LOG_ERROR("bug: couldn't write to resampler");
                    // let the buffer run out
                }
            }

            audioqueue_.read_commit();
        } else {
            // buffer ran out -> "inactive"
            if (streamstate_ != kAooStreamStateInactive){
                streamstate_ = kAooStreamStateInactive;

                source_event e(kAooEventStreamState, ep);
                e.stream_state.state = kAooStreamStateInactive;
                send_event(s, e, kAooThreadLevelAudio);
            }
            underrun_ = true;

            return false;
        }
    }

    // sum source into sink (interleaved -> non-interleaved),
    // starting at the desired sink channel offset.
    // out of bound source channels are silently ignored.
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

    // LOG_DEBUG("read samples from source " << id_);

    if (streamstate_ != kAooStreamStateActive){
        streamstate_ = kAooStreamStateActive;

        source_event e(kAooEventStreamState, ep);
        e.stream_state.state = kAooStreamStateActive;
        send_event(s, e, kAooThreadLevelAudio);
    }

    return true;
}

int32_t source_desc::poll_events(Sink& s, AooEventHandler fn, void *user){
    // always lockfree!
    int count = 0;
    source_event e;
    while (eventqueue_.try_pop(e)){
        fn(user, &e.event, kAooThreadLevelUnknown);
        free_event(e);
        count++;
    }
    return count;
}

void source_desc::add_lost(stream_stats& stats, int32_t n) {
    stats.lost += n;
    lost_blocks_.fetch_add(n, std::memory_order_relaxed);
}

#define SILENT_REFILL 0
#define SKIP_BLOCKS 0

void source_desc::handle_underrun(const Sink& s){
    LOG_VERBOSE("audio buffer underrun");

    int32_t n = audioqueue_.write_available();
    auto nsamples = format_->blockSize * format_->numChannels;
    // reduce by blocks in resampler!
    n -= static_cast<int32_t>((double)resampler_.size() / (double)nsamples + 0.5);

    LOG_DEBUG("audioqueue: " << audioqueue_.read_available()
              << ", resampler: " << (double)resampler_.size() / (double)nsamples);

    if (n > 0){
        double sr = format_->sampleRate;
        for (int i = 0; i < n; ++i){
            auto b = (block_data *)audioqueue_.write_data();
            // push nominal samplerate, channel + silence
            b->header.samplerate = sr;
            b->header.channel = -1; // last channel
        #if SILENT_REFILL
            // push silence
            std::fill(b->data, b->data + nsamples, 0);
        #else
            // use packet loss concealment
            AooInt32 size = nsamples;
            if (AooDecoder_decode(decoder_.get(), nullptr, 0,
                                  b->data, &size) != kAooOk) {
                LOG_WARNING("AooSink: couldn't decode block!");
                // fill with zeros
                std::fill(b->data, b->data + nsamples, 0);
            }
        #endif
            audioqueue_.write_commit();
        }

        LOG_DEBUG("write " << n << " empty blocks to audio buffer");

    #if SKIP_BLOCKS
        skipblocks_ += n;

        LOG_DEBUG("skip next " << n << " blocks");
    #endif
    }

    source_event e(kAooEventBufferUnderrun, ep);
    send_event(s, e, kAooThreadLevelAudio);

    underrun_ = false;
}

bool source_desc::add_packet(const Sink& s, const net_packet& d,
                             stream_stats& stats){
    // we have to check the stream_id (again) because the stream
    // might have changed in between!
    if (d.stream_id != stream_id_){
        LOG_DEBUG("ignore data packet from previous stream");
        return false;
    }

    if (d.sequence <= jitterbuffer_.last_popped()){
        // block too old, discard!
        LOG_VERBOSE("discard old block " << d.sequence);
        LOG_DEBUG("oldest: " << jitterbuffer_.last_popped());
        return false;
    }

    // check for large gap between incoming block and most recent block
    // (either network problem or stream has temporarily stopped.)
    auto newest = jitterbuffer_.last_pushed();
    auto diff = d.sequence - newest;
    if (newest >= 0 && diff > jitterbuffer_.capacity()){
        // jitter buffer should be empty.
        if (!jitterbuffer_.empty()){
            LOG_VERBOSE("source_desc: transmission gap, but jitter buffer is not empty");
            jitterbuffer_.clear();
        }
        // we don't need to skip blocks!
        skipblocks_ = 0;
        // No need to refill, because audio buffer should have ran out.
        if (audioqueue_.write_available()){
            LOG_VERBOSE("source_desc: transmission gap, but audio buffer is not empty");
        }
        // report gap to source
        lost_blocks_.fetch_add(diff - 1);
        // send event
        source_event e(kAooEventBlockLost, ep);
        e.block_lost.count = diff - 1;
        send_event(s, e, kAooThreadLevelAudio);
    }

    auto block = jitterbuffer_.find(d.sequence);
    if (!block){
    #if 1
        // can this ever happen!?
        if (d.sequence <= newest){
            LOG_VERBOSE("discard outdated block " << d.sequence);
            LOG_DEBUG("newest: " << newest);
            return false;
        }
    #endif

        if (newest >= 0){
            // notify for gap
            if (diff > 1){
                LOG_VERBOSE("skipped " << (diff - 1) << " blocks");
            }

            // check for jitter buffer overrun
            // can happen if the sink blocks for a longer time
            // or with extreme network jitter (packets have piled up)
        try_again:
            auto space = jitterbuffer_.capacity() - jitterbuffer_.size();
            if (diff > space){
                if (skipblocks_ > 0){
                    LOG_DEBUG("jitter buffer would overrun!");
                    skip_blocks(s);
                    goto try_again;
                } else {
                    // for now, just clear the jitter buffer and let the
                    // audio buffer underrun.
                    LOG_VERBOSE("jitter buffer overrun!");
                    jitterbuffer_.clear();

                    newest = d.sequence; // !
                }
            }

            // fill gaps with empty blocks
            for (int32_t i = newest + 1; i < d.sequence; ++i){
                jitterbuffer_.push_back(i)->init(i, false);
            }
        }

        // add new block
        block = jitterbuffer_.push_back(d.sequence);

        if (d.totalsize == 0){
            // dropped block
            block->init(d.sequence, true);
            return true;
        } else {
            block->init(d.sequence, d.samplerate,
                        d.channel, d.totalsize, d.nframes);
        }
    } else {
        if (d.totalsize == 0){
            if (!block->dropped()){
                // dropped block arrived out of order
                LOG_VERBOSE("empty block " << d.sequence << " out of order");
                block->init(d.sequence, true); // don't call before dropped()!
                return true;
            } else {
                LOG_VERBOSE("empty block " << d.sequence << " already received");
                return false;
            }
        }

        if (block->num_frames() == 0){
            // placeholder block
            block->init(d.sequence, d.samplerate,
                        d.channel, d.totalsize, d.nframes);
        } else if (block->has_frame(d.frame)){
            // frame already received
            LOG_VERBOSE("frame " << d.frame << " of block " << d.sequence << " already received");
            return false;
        }

        if (d.sequence != newest){
            // out of order or resent
            if (block->resend_count() > 0){
                LOG_VERBOSE("resent frame " << d.frame << " of block " << d.sequence);
                stats.resent++;
            } else {
                LOG_VERBOSE("frame " << d.frame << " of block " << d.sequence << " out of order!");
                stats.reordered++;
            }
        }
    }

    // add frame to block
    block->add_frame(d.frame, d.data, d.size);

    return true;
}

void source_desc::process_blocks(const Sink& s, stream_stats& stats){
    if (jitterbuffer_.empty()){
        return;
    }

    auto nsamples = format_->blockSize * format_->numChannels;

    // Transfer all consecutive complete blocks
    while (!jitterbuffer_.empty() && audioqueue_.write_available()){
        const AooByte *data;
        int32_t size;
        double sr;
        int32_t channel;

        auto& b = jitterbuffer_.front();
        if (b.complete()){
            if (b.dropped()){
                data = nullptr;
                size = 0;
                sr = format_->sampleRate; // nominal samplerate
                channel = -1; // current channel
            #if AOO_DEBUG_JITTER_BUFFER
                LOG_ALL("jitter buffer: write empty block ("
                        << b.sequence << ") for source xrun");
            #endif
                // record dropped block
                stats.dropped++;
            } else {
                // block is ready
                data = b.data();
                size = b.size();
                sr = b.samplerate; // real samplerate
                channel = b.channel;
            #if AOO_DEBUG_JITTER_BUFFER
                LOG_ALL("jitter buffer: write samples for block ("
                        << b.sequence << ")");
            #endif
            }
        } else {
            // we also have to consider the content of the resampler!
            auto remaining = audioqueue_.read_available() + resampler_.size() / nsamples;
            if (remaining < minblocks_){
                // we need audio, so we have to drop a block
                LOG_DEBUG("remaining: " << remaining << " / " << audioqueue_.capacity()
                          << ", limit: " << minblocks_);
                data = nullptr;
                size = 0;
                sr = format_->sampleRate; // nominal samplerate
                channel = -1; // current channel
                add_lost(stats, 1);
                LOG_VERBOSE("dropped block " << b.sequence);
            } else {
                // wait for block
            #if AOO_DEBUG_JITTER_BUFFER
                LOG_ALL("jitter buffer: wait");
            #endif
                break;
            }
        }

        // push samples and channel
        auto d = (block_data *)audioqueue_.write_data();
        d->header.samplerate = sr;
        d->header.channel = channel;
        // decode and push audio data
        AooInt32 n = nsamples;
        if (AooDecoder_decode(decoder_.get(), data, size, d->data, &n) != kAooOk){
            LOG_WARNING("AooSink: couldn't decode block!");
            // decoder failed - fill with zeros
            std::fill(d->data, d->data + nsamples, 0);
        }
        audioqueue_.write_commit();

    #if 0
        Log log;
        for (int i = 0; i < nsamples; ++i){
            log << d->data[i] << " ";
        }
    #endif

        jitterbuffer_.pop_front();
    }
}

void source_desc::skip_blocks(const Sink& s){
    auto n = std::min<int>(skipblocks_, jitterbuffer_.size());
    LOG_VERBOSE("skip " << n << " blocks");
    while (n--){
        jitterbuffer_.pop_front();
    }
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

            if (b->count_frames() > 0){
                // only some frames missing
                for (int i = 0; i < nframes; ++i){
                    if (!b->has_frame(i)){
                        if (resent < maxnumframes){
                            push_data_request({ b->sequence, i });
                        #if 0
                            DO_LOG_DEBUG("request " << b->sequence << " (" << i << ")");
                        #endif
                            resent++;
                        } else {
                            goto resend_done;
                        }
                    }
                }
            } else {
                // all frames missing
                if (resent + nframes <= maxnumframes){
                    push_data_request({ b->sequence, -1 }); // whole block
                #if 0
                    DO_LOG_DEBUG("request " << b->sequence << " (all)");
                #endif
                    resent += nframes;
                } else {
                    goto resend_done;
                }
            }
        }
    }
resend_done:

    assert(resent <= maxnumframes);
    if (resent > 0){
        LOG_DEBUG("requested " << resent << " frames");
    }
}

// /aoo/<id>/ping <id> <tt1> <tt2> <packetloss>
// called without lock!
void source_desc::send_ping_reply(const Sink &s, AooNtpTime tt1,
                                  AooNtpTime tt2, const sendfn &fn) {
    LOG_DEBUG("send " kAooMsgPing " to " << ep);

    // cache samplerate and blocksize
    shared_lock lock(mutex_);
    if (!format_){
        LOG_DEBUG("send_ping_reply: no format");
        return; // shouldn't happen
    }
    auto sr = format_->sampleRate;
    auto blocksize = format_->blockSize;
    lock.unlock();

    // get lost blocks since last ping reply and calculate
    // packet loss percentage.
    auto lost_blocks = lost_blocks_.exchange(0);
    auto last_ping_time = std::exchange(last_ping_reply_time_, tt2);
    // NOTE: the delta can be very large for the first ping in a stream,
    // but this is not an issue because there's no packetloss anyway.
    auto delta = time_tag::duration(last_ping_time, tt2);
    float packetloss = (float)lost_blocks * (float)blocksize
            / ((float)sr * delta);
    if (packetloss > 1.0){
        LOG_DEBUG("packet loss percentage larger than 1");
        packetloss = 1.0;
    }
    LOG_DEBUG("ping delta: " << delta << ", packet loss: " << packetloss);

    char buffer[AOO_MAX_PACKET_SIZE];
    osc::OutboundPacketStream msg(buffer, sizeof(buffer));

    // make OSC address pattern
    const int32_t max_addr_size = kAooMsgDomainLen
            + kAooMsgSourceLen + 16 + kAooMsgPingLen;
    char address[max_addr_size];
    snprintf(address, sizeof(address), "%s%s/%d%s",
             kAooMsgDomain, kAooMsgSource, ep.id, kAooMsgPing);

    msg << osc::BeginMessage(address) << s.id()
        << osc::TimeTag(tt1) << osc::TimeTag(tt2) << packetloss
        << osc::EndMessage;

    fn((const AooByte *)msg.Data(), msg.Size(), ep);
}

// /aoo/src/<id>/start <sink>
// called without lock!
void source_desc::send_start_request(const Sink& s, const sendfn& fn) {
    LOG_VERBOSE("request " kAooMsgStart " for source " << ep);

    AooByte buf[AOO_MAX_PACKET_SIZE];
    osc::OutboundPacketStream msg((char *)buf, sizeof(buf));

    // make OSC address pattern
    const int32_t max_addr_size = kAooMsgDomainLen +
            kAooMsgSourceLen + 16 + kAooMsgStartLen;
    char address[max_addr_size];
    snprintf(address, sizeof(address), "%s%s/%d%s",
             kAooMsgDomain, kAooMsgSource, ep.id, kAooMsgStart);

    msg << osc::BeginMessage(address) << s.id()
        << (int32_t)make_version() << osc::EndMessage;

    fn((const AooByte *)msg.Data(), msg.Size(), ep);
}

// /aoo/src/<id>/data <id> <stream_id> <seq1> <frame1> <seq2> <frame2> etc.
// or
// (header), id (int32), stream_id (int32), count (int32),
// seq1 (int32), frame1(int32), seq2(int32), frame2(seq), etc.

void source_desc::send_data_requests(const Sink& s, const sendfn& fn){
    if (datarequestqueue_.empty()){
        return;
    }

    shared_lock lock(mutex_);
    int32_t stream_id = stream_id_; // cache!
    lock.unlock();

    AooByte buf[AOO_MAX_PACKET_SIZE];

    if (binary_.load(std::memory_order_relaxed)){
        const int32_t maxdatasize = s.packetsize()
                - (kAooBinMsgHeaderSize + 8); // id + stream_id
        const int32_t maxrequests = maxdatasize / 8; // 2 * int32
        int32_t numrequests = 0;

        auto it = buf;
        // write header
        memcpy(it, kAooBinMsgDomain, kAooBinMsgDomainSize);
        it += kAooBinMsgDomainSize;
        aoo::write_bytes<int16_t>(kAooTypeSource, it);
        aoo::write_bytes<int16_t>(kAooBinMsgCmdData, it);
        aoo::write_bytes<int32_t>(ep.id, it);
        // write first 2 args (constant)
        aoo::write_bytes<int32_t>(s.id(), it);
        aoo::write_bytes<int32_t>(stream_id, it);
        // skip 'count' field
        it += sizeof(int32_t);

        auto head = it;

        data_request r;
        while (datarequestqueue_.try_pop(r)){
            LOG_DEBUG("send binary data request ("
                      << r.sequence << " " << r.frame << ")");

            aoo::write_bytes<int32_t>(r.sequence, it);
            aoo::write_bytes<int32_t>(r.frame, it);
            if (++numrequests >= maxrequests){
                // write 'count' field
                aoo::to_bytes(numrequests, head - sizeof(int32_t));
                // send it off
                fn(buf, it - buf, ep);
                // prepare next message (just rewind)
                it = head;
                numrequests = 0;
            }
        }

        if (numrequests > 0){
            // write 'count' field
            aoo::to_bytes(numrequests, head - sizeof(int32_t));
            // send it off
            fn(buf, it - buf, ep);
        }
    } else {
        char buf[AOO_MAX_PACKET_SIZE];
        osc::OutboundPacketStream msg(buf, sizeof(buf));

        // make OSC address pattern
        const int32_t maxaddrsize = kAooMsgDomainLen +
                kAooMsgSourceLen + 16 + kAooMsgDataLen;
        char pattern[maxaddrsize];
        snprintf(pattern, sizeof(pattern), "%s%s/%d%s",
                 kAooMsgDomain, kAooMsgSource, ep.id, kAooMsgData);

        const int32_t maxdatasize = s.packetsize() - maxaddrsize - 16; // id + stream_id + padding
        const int32_t maxrequests = maxdatasize / 10; // 2 * (int32_t + typetag)
        int32_t numrequests = 0;

        msg << osc::BeginMessage(pattern) << s.id() << stream_id;

        data_request r;
        while (datarequestqueue_.try_pop(r)){
            LOG_DEBUG("send data request (" << r.sequence
                      << " " << r.frame << ")");

            msg << r.sequence << r.frame;
            if (++numrequests >= maxrequests){
                // send it off
                msg << osc::EndMessage;

                fn((const AooByte *)msg.Data(), msg.Size(), ep);

                // prepare next message
                msg.Clear();
                msg << osc::BeginMessage(pattern) << s.id() << stream_id;
                numrequests = 0;
            }
        }

        if (numrequests > 0){
            // send it off
            msg << osc::EndMessage;

            fn((const AooByte *)msg.Data(), msg.Size(), ep);
        }
    }
}

// /aoo/src/<id>/invite <sink> <stream_id> [<metadata_type> <metadata_content>]

// called without lock!
void send_invitation(const Sink& s, const endpoint& ep, AooId token,
                     const AooDataView *metadata, const sendfn& fn){
    char buffer[AOO_MAX_PACKET_SIZE];
    osc::OutboundPacketStream msg(buffer, sizeof(buffer));

    // make OSC address pattern
    const int32_t max_addr_size = kAooMsgDomainLen
            + kAooMsgSourceLen + 16 + kAooMsgInviteLen;
    char address[max_addr_size];
    snprintf(address, sizeof(address), "%s%s/%d%s",
             kAooMsgDomain, kAooMsgSource, ep.id, kAooMsgInvite);

    msg << osc::BeginMessage(address) << s.id() << token;
    if (metadata){
        msg << metadata->type << osc::Blob(metadata->data, metadata->size);
    }
    msg << osc::EndMessage;

    LOG_DEBUG("send " kAooMsgInvite " to source " << ep
              << " (" << token << ")");

    fn((const AooByte *)msg.Data(), msg.Size(), ep);
}

// /aoo/<id>/uninvite <sink>

void send_uninvitation(const Sink& s, const endpoint& ep,
                       AooId token, const sendfn &fn){
    LOG_DEBUG("send " kAooMsgUninvite " to source " << ep);

    char buffer[AOO_MAX_PACKET_SIZE];
    osc::OutboundPacketStream msg(buffer, sizeof(buffer));

    // make OSC address pattern
    const int32_t max_addr_size = kAooMsgDomainLen
            + kAooMsgSourceLen + 16 + kAooMsgUninviteLen;
    char address[max_addr_size];
    snprintf(address, sizeof(address), "%s%s/%d%s",
             kAooMsgDomain, kAooMsgSource, ep.id, kAooMsgUninvite);

    msg << osc::BeginMessage(address) << s.id() << token
        << osc::EndMessage;

    fn((const AooByte *)msg.Data(), msg.Size(), ep);
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
            LOG_DEBUG("send_invitation: invite -> timeout");
        } else {
            LOG_DEBUG("send_invitation: invite -> timeout failed");
        }
        // always send timeout event
        LOG_VERBOSE(ep << ": invitation timed out");
        endpoint_event e(kAooEventInviteTimeout, ep);
        s.send_event(e, kAooThreadLevelNetwork);
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

void source_desc::send_event(const Sink& s, const source_event& e,
                             AooThreadLevel level){
    switch (s.event_mode()){
    case kAooEventModePoll:
        eventqueue_.push(e);
        break;
    case kAooEventModeCallback:
        s.call_event(e, level);
        break;
    default:
        break;
    }
}

void source_desc::free_event(const source_event &e){
    if (e.type == kAooEventFormatChange){
        memory_.deallocate((void *)e.format.format);
    } else if (e.type == kAooEventStreamStart){
        if (e.stream_start.metadata){
            memory_.deallocate((void *)e.stream_start.metadata);
        }
    }
}

} // aoo
