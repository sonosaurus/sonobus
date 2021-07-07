/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "sink.hpp"

#include <algorithm>
#include <cmath>

/*//////////////////// aoo_sink /////////////////////*/

aoo_sink * aoo_sink_new(aoo_id id, uint32_t flags) {
    return aoo::construct<aoo::sink_imp>(id, flags);
}

aoo::sink_imp::sink_imp(aoo_id id, uint32_t flags)
    : id_(id) {
    eventqueue_.reserve(AOO_EVENTQUEUESIZE);
}

void aoo_sink_free(aoo_sink *sink) {
    // cast to correct type because base class
    // has no virtual destructor!
    aoo::destroy(static_cast<aoo::sink_imp *>(sink));
}

aoo::sink_imp::~sink_imp(){}

aoo_error aoo_sink_setup(aoo_sink *sink, int32_t samplerate,
                         int32_t blocksize, int32_t nchannels) {
    return sink->setup(samplerate, blocksize, nchannels);
}

aoo_error aoo::sink_imp::setup(int32_t samplerate,
                               int32_t blocksize, int32_t nchannels){
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

        return AOO_OK;
    } else {
        return AOO_ERROR_UNSPECIFIED;
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
        return AOO_ERROR_UNSPECIFIED;   \
    }                                   \

aoo_error aoo_sink_ctl(aoo_sink *sink, int32_t ctl,
                       intptr_t index, void *p, size_t size)
{
    return sink->control(ctl, index, p, size);
}

aoo_error aoo::sink_imp::control(int32_t ctl, intptr_t index,
                                 void *ptr, size_t size)
{
    switch (ctl){
    // invite source
    case AOO_CTL_INVITE_SOURCE:
    {
        auto ep = (const aoo_endpoint *)index;
        if (!ep){
            return AOO_ERROR_UNSPECIFIED;
        }
        ip_address addr((const sockaddr *)ep->address, ep->addrlen);

        push_request(source_request { request_type::invite, addr, ep->id });

        break;
    }
    // uninvite source(s)
    case AOO_CTL_UNINVITE_SOURCE:
    {
        auto ep = (const aoo_endpoint *)index;
        if (ep){
            // single source
            ip_address addr((const sockaddr *)ep->address, ep->addrlen);

            push_request(source_request { request_type::uninvite, addr, ep->id });
        } else {
            // all sources
            push_request(source_request { request_type::uninvite_all });
        }
        break;
    }
    // id
    case AOO_CTL_SET_ID:
    {
        CHECKARG(int32_t);
        auto newid = as<int32_t>(ptr);
        if (id_.exchange(newid) != newid){
            // LATER clear source list here
        }
        break;
    }
    case AOO_CTL_GET_ID:
        CHECKARG(aoo_id);
        as<aoo_id>(ptr) = id();
        break;
    // reset
    case AOO_CTL_RESET:
    {
        if (index != 0){
            GETSOURCEARG;
            src->reset(*this);
        } else {
            // reset all sources
            reset_sources();
            // reset time DLL
            timer_.reset();
        }
        break;
    }
    // request format
    case AOO_CTL_REQUEST_FORMAT:
    {
        CHECKARG(aoo_format);
        GETSOURCEARG;
        return src->request_format(*this, as<aoo_format>(ptr));
    }
    // get format
    case AOO_CTL_GET_FORMAT:
    {
        assert(size >= sizeof(aoo_format));
        GETSOURCEARG;
        return src->get_format(as<aoo_format>(ptr), size);
    }
    // buffer size
    case AOO_CTL_SET_BUFFERSIZE:
    {
        CHECKARG(int32_t);
        auto bufsize = std::max<int32_t>(0, as<int32_t>(ptr));
        if (bufsize != buffersize_){
            buffersize_.store(bufsize);
            reset_sources();
        }
        break;
    }
    case AOO_CTL_GET_BUFFERSIZE:
        CHECKARG(int32_t);
        as<int32_t>(ptr) = buffersize_.load();
        break;
    // get buffer fill ratio
    case AOO_CTL_GET_BUFFER_FILL_RATIO:
    {
        CHECKARG(float);
        GETSOURCEARG;
        as<float>(ptr) = src->get_buffer_fill_ratio();
        break;
    }
    // timer check
    case AOO_CTL_SET_TIMER_CHECK:
        CHECKARG(aoo_bool);
        timer_check_.store(as<aoo_bool>(ptr));
        break;
    case AOO_CTL_GET_TIMER_CHECK:
        CHECKARG(aoo_bool);
        as<aoo_bool>(ptr) = timer_check_.load();
        break;
    // dynamic resampling
    case AOO_CTL_SET_DYNAMIC_RESAMPLING:
        CHECKARG(aoo_bool);
        dynamic_resampling_.store(as<aoo_bool>(ptr));
        timer_.reset(); // !
        break;
    case AOO_CTL_GET_DYNAMIC_RESAMPLING:
        CHECKARG(aoo_bool);
        as<aoo_bool>(ptr) = dynamic_resampling_.load();
        break;
    // time DLL filter bandwidth
    case AOO_CTL_SET_DLL_BANDWIDTH:
    {
        CHECKARG(float);
        auto bw = std::max<double>(0, std::min<double>(1, as<float>(ptr)));
        dll_bandwidth_.store(bw);
        timer_.reset(); // will update time DLL and reset timer
        break;
    }
    case AOO_CTL_GET_DLL_BANDWIDTH:
        CHECKARG(float);
        as<float>(ptr) = dll_bandwidth_.load();
        break;
    // real samplerate
    case AOO_CTL_GET_REAL_SAMPLERATE:
        CHECKARG(double);
        as<double>(ptr) = realsr_.load(std::memory_order_relaxed);
        break;
    // packetsize
    case AOO_CTL_SET_PACKETSIZE:
    {
        CHECKARG(int32_t);
        const int32_t minpacketsize = 64;
        auto packetsize = as<int32_t>(ptr);
        if (packetsize < minpacketsize){
            LOG_WARNING("packet size too small! setting to " << minpacketsize);
            packetsize_.store(minpacketsize);
        } else if (packetsize > AOO_MAXPACKETSIZE){
            LOG_WARNING("packet size too large! setting to " << AOO_MAXPACKETSIZE);
            packetsize_.store(AOO_MAXPACKETSIZE);
        } else {
            packetsize_.store(packetsize);
        }
        break;
    }
    case AOO_CTL_GET_PACKETSIZE:
        CHECKARG(int32_t);
        as<int32_t>(ptr) = packetsize_.load();
        break;
    // resend data
    case AOO_CTL_SET_RESEND_DATA:
        CHECKARG(aoo_bool);
        resend_.store(as<aoo_bool>(ptr));
        break;
    case AOO_CTL_GET_RESEND_DATA:
        CHECKARG(aoo_bool);
        as<aoo_bool>(ptr) = resend_.load();
        break;
    // resend interval
    case AOO_CTL_SET_RESEND_INTERVAL:
    {
        CHECKARG(int32_t);
        auto interval = std::max<int32_t>(0, as<int32_t>(ptr)) * 0.001;
        resend_interval_.store(interval);
        break;
    }
    case AOO_CTL_GET_RESEND_INTERVAL:
        CHECKARG(int32_t);
        as<int32_t>(ptr) = resend_interval_.load() * 1000.0;
        break;
    // resend limit
    case AOO_CTL_SET_RESEND_LIMIT:
    {
        CHECKARG(int32_t);
        auto limit = std::max<int32_t>(1, as<int32_t>(ptr));
        resend_limit_.store(limit);
        break;
    }
    case AOO_CTL_GET_RESEND_LIMIT:
        CHECKARG(int32_t);
        as<int32_t>(ptr) = resend_limit_.load();
        break;
    // source timeout
    case AOO_CTL_SET_SOURCE_TIMEOUT:
    {
        CHECKARG(int32_t);
        auto timeout = std::max<int32_t>(0, as<int32_t>(ptr)) * 0.001;
        source_timeout_.store(timeout);
        break;
    }
    case AOO_CTL_GET_SOURCE_TIMEOUT:
        CHECKARG(int32_t);
        as<int32_t>(ptr) = source_timeout_.load() * 1000.0;
        break;
    // unknown
    default:
        LOG_WARNING("aoo_sink: unsupported control " << ctl);
        return AOO_ERROR_UNSPECIFIED;
    }
    return AOO_OK;
}

aoo_error aoo_sink_handle_message(aoo_sink *sink, const char *data, int32_t n,
                                  const void *address, int32_t addrlen) {
    return sink->handle_message(data, n, address, addrlen);
}

aoo_error aoo::sink_imp::handle_message(const char *data, int32_t n,
                                        const void *address, int32_t addrlen) {
    if (samplerate_ == 0){
        return AOO_ERROR_UNSPECIFIED; // not setup yet
    }

    aoo_type type;
    aoo_id sinkid;
    int32_t onset;
    auto err = aoo_parse_pattern(data, n, &type, &sinkid, &onset);
    if (err != AOO_OK){
        LOG_WARNING("not an AoO message!");
        return AOO_ERROR_UNSPECIFIED;
    }

    if (type != AOO_TYPE_SINK){
        LOG_WARNING("not a sink message!");
        return AOO_ERROR_UNSPECIFIED;
    }
    if (sinkid != id()){
        LOG_WARNING("wrong sink ID!");
        return AOO_ERROR_UNSPECIFIED;
    }

    ip_address addr((const sockaddr *)address, addrlen);

    if (data[0] == 0){
        // binary message
        auto cmd = aoo::from_bytes<int16_t>(data + AOO_BIN_MSG_DOMAIN_SIZE + 2);
        switch (cmd){
        case AOO_BIN_MSG_CMD_DATA:
            return handle_data_message(data + onset, n - onset, addr);
        default:
            return AOO_ERROR_UNSPECIFIED;
        }
    } else {
        // OSC message
        try {
            osc::ReceivedPacket packet(data, n);
            osc::ReceivedMessage msg(packet);

            auto pattern = msg.AddressPattern() + onset;
            if (!strcmp(pattern, AOO_MSG_FORMAT)){
                return handle_format_message(msg, addr);
            } else if (!strcmp(pattern, AOO_MSG_DATA)){
                return handle_data_message(msg, addr);
            } else if (!strcmp(pattern, AOO_MSG_PING)){
                return handle_ping_message(msg, addr);
            } else {
                LOG_WARNING("unknown message " << pattern);
            }
        } catch (const osc::Exception& e){
            LOG_ERROR("aoo_sink: exception in handle_message: " << e.what());
        }
        return AOO_ERROR_UNSPECIFIED;
    }
}

aoo_error aoo_sink_send(aoo_sink *sink, aoo_sendfn fn, void *user){
    return sink->send(fn, user);
}

aoo_error aoo::sink_imp::send(aoo_sendfn fn, void *user){
    sendfn reply(fn, user);

    // handle requests
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
            src->invite(*this);
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
                LOG_WARNING("aoo: can't uninvite - source not found");
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

    source_lock lock(sources_);
    for (auto& s : sources_){
        s.send(*this, reply);
    }
    lock.unlock();

    // free unused source_descs
    if (!sources_.try_free()){
        // LOG_DEBUG("aoo::sink: try_free() would block");
    }

    return AOO_OK;
}

aoo_error aoo_sink_process(aoo_sink *sink, aoo_sample **data,
                           int32_t nsamples, uint64_t t) {
    return sink->process(data, nsamples, t);
}

#define AOO_MAXNUMEVENTS 256

aoo_error aoo::sink_imp::process(aoo_sample **data, int32_t nsamples, uint64_t t){
    // clear outputs
    for (int i = 0; i < nchannels_; ++i){
        std::fill(data[i], data[i] + nsamples, 0);
    }

    // update timer
    // always do this, even if there are no sources!
    bool dynamic_resampling = dynamic_resampling_.load(std::memory_order_relaxed);
    double error;
    auto state = timer_.update(t, error);
    if (state == timer::state::reset){
        LOG_DEBUG("setup time DLL filter for sink");
        auto bw = dll_bandwidth_.load(std::memory_order_relaxed);
        dll_.setup(samplerate_, blocksize_, bw, 0);
        realsr_.store(samplerate_, std::memory_order_relaxed);
    } else if (state == timer::state::error){
        // recover sources
        int32_t xrunsamples = error * samplerate_ + 0.5;

        // no lock needed - sources are only removed in this thread!
        for (auto& s : sources_){
            s.add_xrun(xrunsamples);
        }

        sink_event e(AOO_XRUN_EVENT);
        e.count = (float)xrunsamples / (float)blocksize_;
        send_event(e, AOO_THREAD_AUDIO);

        timer_.reset();
    } else if (dynamic_resampling) {
        // update time DLL, but only if n matches blocksize!
        auto elapsed = timer_.get_elapsed();
        if (nsamples == blocksize_){
            dll_.update(elapsed);
        #if AOO_DEBUG_DLL
            DO_LOG_DEBUG("time elapsed: " << elapsed << ", period: "
                      << dll_.period() << ", samplerate: " << dll_.samplerate());
        #endif
        } else {
            // reset time DLL with nominal samplerate
            auto bw = dll_bandwidth_.load(std::memory_order_relaxed);
            dll_.setup(samplerate_, blocksize_, bw, elapsed);
        }
        realsr_.store(dll_.samplerate(), std::memory_order_relaxed);
    }

    bool didsomething = false;

    // no lock needed - sources are only removed in this thread!
    for (auto it = sources_.begin(); it != sources_.end();){
        if (it->process(*this, data, nsamples, t)){
            didsomething = true;
        } else if (!it->is_active(*this)){
            // move source to garbage list (will be freed in send())
            if (it->is_inviting()){
                LOG_VERBOSE("aoo::sink: invitation for " << it->address().name()
                            << " " << it->address().port() << " timed out");
                sink_event e(AOO_INVITE_TIMEOUT_EVENT, *it);
                send_event(e, AOO_THREAD_AUDIO);
            } else {
                LOG_VERBOSE("aoo::sink: removed inactive source " << it->address().name()
                            << " " << it->address().port());
                sink_event e(AOO_SOURCE_REMOVE_EVENT, *it);
                send_event(e, AOO_THREAD_AUDIO);
            }
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
    return AOO_OK;
}

aoo_error aoo_sink_set_eventhandler(aoo_sink *sink, aoo_eventhandler fn,
                                    void *user, int32_t mode)
{
    return sink->set_eventhandler(fn, user, mode);
}

aoo_error aoo::sink_imp::set_eventhandler(aoo_eventhandler fn, void *user, int32_t mode)
{
    eventhandler_ = fn;
    eventcontext_ = user;
    eventmode_ = (aoo_event_mode)mode;
    return AOO_OK;
}

aoo_bool aoo_sink_events_available(aoo_sink *sink){
    return sink->events_available();
}

aoo_bool aoo::sink_imp::events_available(){
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

aoo_error aoo_sink_poll_events(aoo_sink *sink){
    return sink->poll_events();
}

#define EVENT_THROTTLE 1000

aoo_error aoo::sink_imp::poll_events(){
    int total = 0;
    sink_event e;
    while (eventqueue_.try_pop(e)){
        if (e.type == AOO_XRUN_EVENT){
            aoo_xrun_event xe;
            xe.type = e.type;
            xe.count = e.count;
            eventhandler_(eventcontext_, (const aoo_event *)&xe,
                          AOO_THREAD_UNKNOWN);
        } else {
            aoo_source_event se;
            se.type = e.type;
            se.ep.address = e.address.address();
            se.ep.addrlen = e.address.length();
            se.ep.id = e.id;
            eventhandler_(eventcontext_, (const aoo_event *)&se,
                          AOO_THREAD_UNKNOWN);
        }

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
    return AOO_OK;
}

namespace aoo {

void sink_imp::send_event(const sink_event &e, aoo_thread_level level) {
    switch (eventmode_){
    case AOO_EVENT_POLL:
        eventqueue_.push(e);
        break;
    case AOO_EVENT_CALLBACK:
    {
        aoo_sink_event se;
        se.type = e.type;
        se.ep.address = e.address.address();
        se.ep.addrlen = e.address.length();
        se.ep.id = e.id;
        eventhandler_(eventcontext_, (const aoo_event *)&se, level);
        break;
    }
    default:
        break;
    }
}

// only called if mode is AOO_EVENT_CALLBACK
void sink_imp::call_event(const event &e, aoo_thread_level level) const {
    eventhandler_(eventcontext_, &e.event_, level);
    // some events use dynamic memory
    if (e.type_ == AOO_FORMAT_CHANGE_EVENT){
        memory.free(memory_block::from_bytes((void *)e.format.format));
    }
}

aoo::source_desc * sink_imp::find_source(const ip_address& addr, aoo_id id){
    for (auto& src : sources_){
        if (src.match(addr, id)){
            return &src;
        }
    }
    return nullptr;
}

aoo::source_desc * sink_imp::get_source_arg(intptr_t index){
    auto ep = (const aoo_endpoint *)index;
    if (!ep){
        LOG_ERROR("aoo_sink: missing source argument");
        return nullptr;
    }
    ip_address addr((const sockaddr *)ep->address, ep->addrlen);
    auto src = find_source(addr, ep->id);
    if (!src){
        LOG_ERROR("aoo_sink: couldn't find source");
    }
    return src;
}

source_desc * sink_imp::add_source(const ip_address& addr, aoo_id id){
    // add new source
    sources_.emplace_front(addr, id, elapsed_time());
    return &sources_.front();
}

void sink_imp::reset_sources(){
    source_lock lock(sources_);
    for (auto& src : sources_){
        src.reset(*this);
    }
}

// /format <id> <version> <salt> <channels> <sr> <blocksize> <codec> <options...>
aoo_error sink_imp::handle_format_message(const osc::ReceivedMessage& msg,
                                          const ip_address& addr)
{
    auto it = msg.ArgumentsBegin();

    aoo_id id = (it++)->AsInt32();
    int32_t version = (it++)->AsInt32();

    // LATER handle this in the source_desc (e.g. ignoring further messages)
    if (!check_version(version)){
        LOG_ERROR("aoo_sink: source version not supported");
        return AOO_ERROR_UNSPECIFIED;
    }

    int32_t salt = (it++)->AsInt32();
    // get format from arguments
    aoo_format f;
    f.nchannels = (it++)->AsInt32();
    f.samplerate = (it++)->AsInt32();
    f.blocksize = (it++)->AsInt32();
    f.codec = (it++)->AsString();
    f.size = sizeof(aoo_format);
    const void *settings;
    osc::osc_bundle_element_size_t size;
    (it++)->AsBlob(settings, size);
    // for backwards comptability (later remove check)
    uint32_t flags = (it != msg.ArgumentsEnd() && it->IsInt32()) ?
                (uint32_t)(it++)->AsInt32() : 0;

    if (id < 0){
        LOG_WARNING("bad ID for " << AOO_MSG_FORMAT << " message");
        return AOO_ERROR_UNSPECIFIED;
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
    return src->handle_format(*this, salt, f, (const char *)settings, size, flags);
}

aoo_error sink_imp::handle_data_message(const osc::ReceivedMessage& msg,
                                        const ip_address& addr)
{
    auto it = msg.ArgumentsBegin();

    auto id = (it++)->AsInt32();

    aoo::net_packet d;
    d.salt = (it++)->AsInt32();
    d.sequence = (it++)->AsInt32();
    d.samplerate = (it++)->AsDouble();
    d.channel = (it++)->AsInt32();
    d.totalsize = (it++)->AsInt32();
    d.nframes = (it++)->AsInt32();
    d.frame = (it++)->AsInt32();
    const void *blobdata;
    osc::osc_bundle_element_size_t blobsize;
    (it++)->AsBlob(blobdata, blobsize);
    d.data = (const char *)blobdata;
    d.size = blobsize;

    return handle_data_packet(d, false, addr, id);
}

// binary data message:
// id (int32), salt (int32), seq (int32), channel (int16), flags (int16),
// [total (int32), nframes (int16), frame (int16)],  [sr (float64)],
// size (int32), data...

aoo_error sink_imp::handle_data_message(const char *msg, int32_t n,
                                        const ip_address& addr)
{
    // check size (excluding samplerate, frames and data)
    if (n < 20){
        LOG_ERROR("handle_data_message: header too small!");
        return AOO_ERROR_UNSPECIFIED;
    }

    auto it = msg;

    auto id = aoo::read_bytes<int32_t>(it);

    aoo::net_packet d;
    d.salt = aoo::read_bytes<int32_t>(it);
    d.sequence = aoo::read_bytes<int32_t>(it);
    d.channel = aoo::read_bytes<int16_t>(it);
    auto flags = aoo::read_bytes<int16_t>(it);
    if (flags & AOO_BIN_MSG_DATA_FRAMES){
        d.totalsize = aoo::read_bytes<int32_t>(it);
        d.nframes = aoo::read_bytes<int16_t>(it);
        d.frame = aoo::read_bytes<int16_t>(it);
    } else {
        d.totalsize = 0;
        d.nframes = 1;
        d.frame = 0;
    }
    if (flags & AOO_BIN_MSG_DATA_SAMPLERATE){
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
        return AOO_ERROR_UNSPECIFIED;
    }

    d.data = it;

    return handle_data_packet(d, true, addr, id);
}

aoo_error sink_imp::handle_data_packet(net_packet& d, bool binary,
                                       const ip_address& addr, aoo_id id)
{
    if (id < 0){
        LOG_WARNING("bad ID for " << AOO_MSG_DATA << " message");
        return AOO_ERROR_UNSPECIFIED;
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

aoo_error sink_imp::handle_ping_message(const osc::ReceivedMessage& msg,
                                        const ip_address& addr)
{
    auto it = msg.ArgumentsBegin();

    auto id = (it++)->AsInt32();
    time_tag tt = (it++)->AsTimeTag();

    if (id < 0){
        LOG_WARNING("bad ID for " << AOO_MSG_PING << " message");
        return AOO_ERROR_UNSPECIFIED;
    }
    // try to find existing source
    source_lock lock(sources_);
    auto src = find_source(addr, id);
    if (src){
        return src->handle_ping(*this, tt);
    } else {
        LOG_WARNING("couldn't find source " << id << " for " << AOO_MSG_PING << " message");
        return AOO_ERROR_UNSPECIFIED;
    }
}

/*////////////////////////// event ///////////////////////////////////*/

// 'event' is always used inside 'source_desc', so we can safely
// store a pointer to the sockaddr. the ip_address itself
// never changes during lifetime of the 'source_desc'!
// NOTE: this assumes that the event queue is polled regularly,
// i.e. before a source_desc can be possibly autoremoved.
event::event(aoo_event_type type, const source_desc& desc){
    source.type = type;
    source.ep.address = desc.address().address();
    source.ep.addrlen = desc.address().length();
    source.ep.id = desc.id();
}

// 'sink_event' is used in 'sink' for source events that can outlive
// its corresponding 'source_desc', therefore the ip_address is copied!
sink_event::sink_event(aoo_event_type _type, const source_desc &desc)
    : type(_type), address(desc.address()), id(desc.id()) {}

/*////////////////////////// source_desc /////////////////////////////*/

source_desc::source_desc(const ip_address& addr, aoo_id id, double time)
    : addr_(addr), id_(id), last_packet_time_(time)
{
    // reserve some memory, so we don't have to allocate memory
    // when pushing events in the audio thread.
    eventqueue_.reserve(AOO_EVENTQUEUESIZE);
    // resendqueue_.reserve(256);
    LOG_DEBUG("source_desc");
}

source_desc::~source_desc(){
    // flush event queue
    event e;
    while (eventqueue_.try_pop(e)){
        if (e.type_ == AOO_FORMAT_CHANGE_EVENT){
            auto mem = memory_block::from_bytes((void *)e.format.format);
            memory_block::free(mem);
        }
    }
    // flush packet queue
    net_packet d;
    while (packetqueue_.try_pop(d)){
        auto mem = memory_block::from_bytes((void *)d.data);
        memory_block::free(mem);
    }
    LOG_DEBUG("~source_desc");
}

bool source_desc::is_active(const sink_imp& s) const {
    auto last = last_packet_time_.load(std::memory_order_relaxed);
    return (s.elapsed_time() - last) < s.source_timeout();
}

aoo_error source_desc::get_format(aoo_format &format, size_t size){
    // synchronize with handle_format() and update()!
    scoped_shared_lock lock(mutex_);
    if (decoder_){
        return decoder_->get_format(format, size);
    } else {
        return AOO_ERROR_UNSPECIFIED;
    }
}

void source_desc::reset(const sink_imp& s){
    // take writer lock!
    scoped_lock lock(mutex_);
    update(s);
}

#define MAXHWBUFSIZE 2048
#define MINSAMPLERATE 44100

void source_desc::update(const sink_imp& s){
    // resize audio ring buffer
    if (decoder_ && decoder_->blocksize() > 0 && decoder_->samplerate() > 0){
        // recalculate buffersize from ms to samples
        int32_t bufsize = (double)s.buffersize() * 0.001 * decoder_->samplerate();
        // number of buffers (round up!)
        int32_t nbuffers = std::ceil((double)bufsize / (double)decoder_->blocksize());
        // minimum buffer size depends on resampling and reblocking!
        auto downsample = (double)decoder_->samplerate() / (double)s.samplerate();
        auto reblock = (double)s.blocksize() / (double)decoder_->blocksize();
        minblocks_ = std::ceil(downsample * reblock);
        nbuffers = std::max<int32_t>(nbuffers, minblocks_);
        LOG_DEBUG("source_desc: buffersize (ms): " << s.buffersize()
                  << ", samples: " << bufsize << ", nbuffers: " << nbuffers
                  << ", minimum: " << minblocks_);

    #if 0
        // don't touch the event queue once constructed
        eventqueue_.reset();
    #endif

        auto nsamples = decoder_->nchannels() * decoder_->blocksize();
        double sr = decoder_->samplerate(); // nominal samplerate

        // setup audio buffer
        auto nbytes = sizeof(block_data::header) + nsamples * sizeof(aoo_sample);
        // align to 8 bytes
        nbytes = (nbytes + 7) & ~7;
        audioqueue_.resize(nbytes, nbuffers);
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
        resampler_.setup(decoder_->blocksize(), s.blocksize(),
                         decoder_->samplerate(), s.samplerate(),
                         decoder_->nchannels());

        // setup jitter buffer.
        // if we use a very small audio buffer size, we have to make sure that
        // we have enough space in the jitter buffer in case the source uses
        // a larger hardware buffer size and consequently sends packets in batches.
        // we don't know the actual source samplerate and hardware buffer size,
        // so we have to make a pessimistic guess.
        auto hwsamples = (double)decoder_->samplerate() / MINSAMPLERATE * MAXHWBUFSIZE;
        auto minbuffers = std::ceil(hwsamples / (double)decoder_->blocksize());
        auto jitterbufsize = std::max<int32_t>(nbuffers, minbuffers);
        // LATER optimize max. block size
        jitterbuffer_.resize(jitterbufsize, nsamples * sizeof(double));
        LOG_DEBUG("jitter buffer: " << jitterbufsize << " blocks");

        streamstate_ = AOO_STREAM_STATE_INIT;
        lost_since_ping_.store(0);
        channel_ = 0;
        skipblocks_ = 0;
        underrun_ = false;
        didupdate_ = true;

        // reset decoder to avoid garbage from previous stream
        decoder_->reset();
    }
}

// called from the network thread
void source_desc::invite(const sink_imp& s){
    // only invite when idle or uninviting!
    // NOTE: state can only change in this thread (= send thread),
    // so we don't need a CAS loop.
    auto state = state_.load(std::memory_order_relaxed);
    while (state != source_state::stream){
        // special case: (re)invite shortly after uninvite
        if (state == source_state::uninvite){
            // update last packet time to reset timeout!
            last_packet_time_.store(s.elapsed_time());
            // force new format, otherwise handle_format() would ignore
            // the format messages and we would spam the source with
            // redundant invitation messages until we time out.
            // NOTE: don't use a negative value, otherwise we would get
            // a redundant "add" event, see handle_format().
            scoped_lock lock(mutex_);
            salt_++;
        }
    #if 1
        state_time_.store(0.0); // start immediately
    #else
        state_time_.store(s.elapsed_time()); // wait
    #endif
        if (state_.compare_exchange_weak(state, source_state::invite)){
            LOG_DEBUG("source_desc: invite");
            return;
        }
    }
    LOG_WARNING("aoo: couldn't invite source - already active");
}

// called from the network thread
void source_desc::uninvite(const sink_imp& s){
    // state can change in different threads, so we need a CAS loop
    auto state = state_.load(std::memory_order_relaxed);
    while (state != source_state::idle){
        // update start time for uninvite phase, see handle_data()
        state_time_.store(s.elapsed_time());
        if (state_.compare_exchange_weak(state, source_state::uninvite)){
            LOG_DEBUG("source_desc: uninvite");
            return;
        }
    }
    LOG_WARNING("aoo: couldn't uninvite source - not active");
}

aoo_error source_desc::request_format(const sink_imp& s, const aoo_format &f){
    if (state_.load(std::memory_order_relaxed) == source_state::uninvite){
        // requesting a format during uninvite doesn't make sense.
        // also, we couldn't use 'state_time', because it has a different
        // meaning during the uninvite phase.
        return AOO_ERROR_UNSPECIFIED;
    }

    if (!aoo::find_codec(f.codec)){
        LOG_WARNING("request_format: codec '" << f.codec << "' not supported");
        return AOO_ERROR_UNSPECIFIED;
    }

    // copy format
    auto fmt = (aoo_format *)aoo::allocate(f.size);
    memcpy(fmt, &f, f.size);

    LOG_DEBUG("source_desc: request format");

    scoped_lock lock(mutex_); // writer lock!

    format_request_.reset(fmt);

    format_time_ = s.elapsed_time();
#if 1
    state_time_.store(0.0); // start immediately
#else
    state_time_.store(s.elapsed_time()); // wait
#endif

    return AOO_OK;
}

float source_desc::get_buffer_fill_ratio(){
    scoped_shared_lock lock(mutex_);
    if (decoder_){
        // consider samples in resampler!
        auto nsamples = decoder_->nchannels() * decoder_->blocksize();
        auto available = (double)audioqueue_.read_available() +
                (double)resampler_.size() / (double)nsamples;
        auto ratio = available / (double)audioqueue_.capacity();
        LOG_DEBUG("fill ratio: " << ratio << ", audioqueue: " << audioqueue_.read_available()
                  << ", resampler: " << (double)resampler_.size() / (double)nsamples);
        // FIXME sometimes the result is bigger than 1.0
        return std::min<float>(1.0, ratio);
    } else {
        return 0.0;
    }
}

// /aoo/sink/<id>/format <src> <salt> <numchannels> <samplerate> <blocksize> <codec> <settings...>

aoo_error source_desc::handle_format(const sink_imp& s, int32_t salt, const aoo_format& f,
                                     const char *settings, int32_t size, uint32_t flags){
    LOG_DEBUG("handle_format");
    // ignore redundant format messages!
    // NOTE: salt_ can only change in this thread,
    // so we don't need a lock to safely *read* it!
    if (salt == salt_){
        return AOO_ERROR_UNSPECIFIED;
    }

    // look up codec
    auto c = aoo::find_codec(f.codec);
    if (!c){
        LOG_ERROR("codec '" << f.codec << "' not supported!");
        return AOO_ERROR_UNSPECIFIED;
    }

    // try to deserialize format
    aoo_format_storage fmt;
    if (c->deserialize(f, settings, size,
                       fmt.header, sizeof(fmt)) != AOO_OK){
        return AOO_ERROR_UNSPECIFIED;
    }

    // Create a new decoder if necessary.
    // This is the only thread where the decoder can possibly
    // change, so we don't need a lock to safely *read* it!
    std::unique_ptr<decoder> new_decoder;
    bool changed = false;

    if (!decoder_ || strcmp(decoder_->name(), f.codec)){
        new_decoder = c->create_decoder();
        if (!new_decoder){
            LOG_ERROR("couldn't create decoder!");
            return AOO_ERROR_UNSPECIFIED;
        }
        changed = true;
    } else {
        changed = !decoder_->compare(fmt.header); // thread-safe
    }

    unique_lock lock(mutex_); // writer lock!
    if (new_decoder){
        decoder_ = std::move(new_decoder);
    }

    auto oldsalt = salt_;
    salt_ = salt;
    flags_ = flags;
    format_request_ = nullptr;

    // set format (if changed)
    if (changed && decoder_->set_format(fmt.header) != AOO_OK){
        return AOO_ERROR_UNSPECIFIED;
    }

    // always update!
    update(s);

    lock.unlock();

    // NOTE: state can be changed in both network threads,
    // so we need a CAS loop.
    auto state = state_.load(std::memory_order_relaxed);
    while (state == source_state::idle || state == source_state::invite){
        if (state_.compare_exchange_weak(state, source_state::stream)){
            // only push "add" event, if this is the first format message!
            if (oldsalt < 0){
                event e(AOO_SOURCE_ADD_EVENT, *this);
                send_event(s, e, AOO_THREAD_AUDIO);
                LOG_DEBUG("add new source with id " << id());
            }
            break;
        }
    }

    // send format event (if changed)
    // NOTE: we could just allocate 'aoo_format_storage', but it would be wasteful.
    if (changed){
        auto mem = s.memory.alloc(fmt.header.size);
        memcpy(mem->data(), &fmt, fmt.header.size);

        event e(AOO_FORMAT_CHANGE_EVENT, *this);
        e.format.format = (const aoo_format *)mem->data();

        send_event(s, e, AOO_THREAD_NETWORK);
    }

    return AOO_OK;
}

// /aoo/sink/<id>/data <src> <salt> <seq> <sr> <channel_onset> <totalsize> <numpackets> <packetnum> <data>

aoo_error source_desc::handle_data(const sink_imp& s, net_packet& d, bool binary)
{
    binary_.store(binary, std::memory_order_relaxed);

    // always update packet time to signify that we're receiving packets
    last_packet_time_.store(s.elapsed_time(), std::memory_order_relaxed);

    // if we're in uninvite state, ignore data and send uninvite request.
    if (state_.load(std::memory_order_acquire) == source_state::uninvite){
        // only try for a certain amount of time to avoid spamming the source.
        auto delta = s.elapsed_time() - state_time_.load(std::memory_order_relaxed);
        if (delta < s.source_timeout()){
            push_request(request(request_type::uninvite));
        }
        // ignore data message
        return AOO_OK;
    }

    // the source format might have changed and we haven't noticed,
    // e.g. because of dropped UDP packets.
    // NOTE: salt_ can only change in this thread!
    if (d.salt != salt_){
        push_request(request(request_type::format));
        return AOO_OK;
    }

    // synchronize with update()!
    scoped_shared_lock lock(mutex_);

#if 1
    if (!decoder_){
        LOG_DEBUG("ignore data message");
        return AOO_ERROR_UNSPECIFIED;
    }
#else
    assert(decoder_ != nullptr);
#endif
    // check and fix up samplerate
    if (d.samplerate == 0){
        // no dynamic resampling, just use nominal samplerate
        d.samplerate = decoder_->samplerate();
    }

    // copy blob data and push to queue
    auto data = (char *)s.memory.alloc(d.size)->data();
    memcpy(data, d.data, d.size);
    d.data = data;

    packetqueue_.push(d);

#if AOO_DEBUG_DATA
    LOG_DEBUG("got block: seq = " << d.sequence << ", sr = " << d.samplerate
              << ", chn = " << d.channel << ", totalsize = " << d.totalsize
              << ", nframes = " << d.nframes << ", frame = " << d.frame << ", size " << d.size);
#endif

    return AOO_OK;
}

// /aoo/sink/<id>/ping <src> <time>

aoo_error source_desc::handle_ping(const sink_imp& s, time_tag tt){
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
    event e(AOO_PING_EVENT, *this);
    e.ping.tt1 = tt;
    e.ping.tt2 = tt2;
    e.ping.tt3 = 0;
    send_event(s, e, AOO_THREAD_NETWORK);

    return AOO_OK;
}

// only send every 50 ms! LATER we might make this settable
#define INVITE_INTERVAL 0.05

void source_desc::send(const sink_imp& s, const sendfn& fn){
    request r;
    while (requestqueue_.try_pop(r)){
        switch (r.type){
        case request_type::ping_reply:
            send_ping_reply(s, fn, r);
            break;
        case request_type::format:
            send_format_request(s, fn);
            break;
        case request_type::uninvite:
            send_uninvitation(s, fn);
            break;
        default:
            break;
        }
    }

    auto now = s.elapsed_time();
    if ((now - state_time_.load(std::memory_order_relaxed)) >= INVITE_INTERVAL){
        // send invitations
        if (state_.load(std::memory_order_acquire) == source_state::invite){
            send_invitation(s, fn);
        }

        // the check is not really threadsafe, but it's safe because we only
        // dereference the pointer later after grabbing the mutex
        if (format_request_){
            send_format_request(s, fn, true);
        }

        state_time_.store(now);
    }

    send_data_requests(s, fn);
}

#define XRUN_THRESHOLD 0.1

bool source_desc::process(const sink_imp& s, aoo_sample **buffer,
                          int32_t nsamples, time_tag tt)
{
#if 1
    auto current = state_.load(std::memory_order_acquire);
    if (current != source_state::stream){
        if (current == source_state::uninvite
                && streamstate_ != AOO_STREAM_STATE_STOP){
            streamstate_ = AOO_STREAM_STATE_STOP;

            // push "stop" event
            event e(AOO_STREAM_STATE_EVENT, *this);
            e.source_state.state = AOO_STREAM_STATE_STOP;
            send_event(s, e, AOO_THREAD_AUDIO);
        }
        return false;
    }
#endif
    // synchronize with update()!
    // the mutex should be uncontended most of the time.
    shared_lock lock(mutex_, std::try_to_lock_t{});
    if (!lock.owns_lock()){
        xrun_ += 1.0;
        LOG_VERBOSE("aoo::sink: source_desc::process() would block");
        return false;
    }

    if (!decoder_){
        return false;
    }

    stream_state state;

    // check for sink xruns
    if (didupdate_){
        xrunsamples_ = 0;
        xrun_ = 0;
        assert(underrun_ == false);
        assert(skipblocks_ == 0);
        didupdate_ = false;
    } else if (xrunsamples_ > 0) {
        auto xrunblocks = (float)xrunsamples_ / (float)decoder_->blocksize();
        xrun_ += xrunblocks;
        xrunsamples_ = 0;
    }

    if (!packetqueue_.empty()){
        // check for buffer underrun (only if packets arrive!)
        if (underrun_){
            handle_underrun(s);
        }

        net_packet d;
        while (packetqueue_.try_pop(d)){
            // check data packet
            add_packet(s, d, state);
            // return memory
            s.memory.free(memory_block::from_bytes((void *)d.data));
        }
    }

    if (skipblocks_ > 0){
        skip_blocks(s);
    }

    process_blocks(s, state);

    check_missing_blocks(s);

#if AOO_DEBUG_JITTER_BUFFER
    DO_LOG_DEBUG(jitterbuffer_);
    DO_LOG_DEBUG("oldest: " << jitterbuffer_.last_popped()
              << ", newest: " << jitterbuffer_.last_pushed());
#endif

    if (state.lost > 0){
        // push packet loss event
        event e(AOO_BLOCK_LOST_EVENT, *this);
        e.block_loss.count = state.lost;
        send_event(s, e, AOO_THREAD_AUDIO);
    }
    if (state.reordered > 0){
        // push packet reorder event
        event e(AOO_BLOCK_REORDERED_EVENT, *this);
        e.block_reorder.count = state.reordered;
        send_event(s, e, AOO_THREAD_AUDIO);
    }
    if (state.resent > 0){
        // push packet resend event
        event e(AOO_BLOCK_RESENT_EVENT, *this);
        e.block_resend.count = state.resent;
        send_event(s, e, AOO_THREAD_AUDIO);
    }
    if (state.dropped > 0){
        // push packet resend event
        event e(AOO_BLOCK_DROPPED_EVENT, *this);
        e.block_dropped.count = state.dropped;
        send_event(s, e, AOO_THREAD_AUDIO);
    }

    auto nchannels = decoder_->nchannels();
    auto insize = decoder_->blocksize() * nchannels;
    auto outsize = nsamples * nchannels;
    // if dynamic resampling is disabled, this will simply
    // return the nominal samplerate
    double sr = s.real_samplerate();

#if AOO_DEBUG_AUDIO_BUFFER
    // will print audio buffer and resampler balance
    get_buffer_fill_ratio();
#endif

    // try to read samples from resampler
    auto buf = (aoo_sample *)alloca(outsize * sizeof(aoo_sample));

    while (!resampler_.read(buf, outsize)){
        // try to write samples from buffer into resampler
        if (audioqueue_.read_available()){
            auto d = (block_data *)audioqueue_.read_data();

            if (xrun_ > XRUN_THRESHOLD){
                // skip audio and decrement xrun counter proportionally
                xrun_ -= sr / decoder_->samplerate();
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
            // buffer ran out -> push "stop" event
            if (streamstate_ != AOO_STREAM_STATE_STOP){
                streamstate_ = AOO_STREAM_STATE_STOP;

                // push "stop" event
                event e(AOO_STREAM_STATE_EVENT, *this);
                e.source_state.state = AOO_STREAM_STATE_STOP;
                send_event(s, e, AOO_THREAD_AUDIO);
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

    if (streamstate_ != AOO_STREAM_STATE_PLAY){
        streamstate_ = AOO_STREAM_STATE_PLAY;

        // push "start" event
        event e(AOO_STREAM_STATE_EVENT, *this);
        e.source_state.state = AOO_STREAM_STATE_PLAY;
        send_event(s, e, AOO_THREAD_AUDIO);
    }

    return true;
}

int32_t source_desc::poll_events(sink_imp& s, aoo_eventhandler fn, void *user){
    // always lockfree!
    int count = 0;
    event e;
    while (eventqueue_.try_pop(e)){
        fn(user, &e.event_, AOO_THREAD_UNKNOWN);
        // some events use dynamic memory
        if (e.type_ == AOO_FORMAT_CHANGE_EVENT){
            auto mem = memory_block::from_bytes((void *)e.format.format);
            s.memory.free(mem);
        }
        count++;
    }
    return count;
}

void source_desc::add_lost(stream_state& state, int32_t n) {
    state.lost += n;
    lost_since_ping_.fetch_add(n, std::memory_order_relaxed);
}

#define SILENT_REFILL 0
#define SKIP_BLOCKS 0

void source_desc::handle_underrun(const sink_imp& s){
    LOG_VERBOSE("audio buffer underrun");

    int32_t n = audioqueue_.write_available();
    auto nsamples = decoder_->blocksize() * decoder_->nchannels();
    // reduce by blocks in resampler!
    n -= static_cast<int32_t>((double)resampler_.size() / (double)nsamples + 0.5);

    LOG_DEBUG("audioqueue: " << audioqueue_.read_available()
              << ", resampler: " << (double)resampler_.size() / (double)nsamples);

    if (n > 0){
        double sr = decoder_->samplerate();
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
            int32_t size = nsamples;
            if (decoder_->decode(nullptr, 0, b->data, size) != AOO_OK){
                LOG_WARNING("aoo_sink: couldn't decode block!");
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

    event e(AOO_BUFFER_UNDERRUN_EVENT, *this);
    send_event(s, e, AOO_THREAD_AUDIO);

    underrun_ = false;
}

bool source_desc::add_packet(const sink_imp& s, const net_packet& d,
                             stream_state& state){
    // we have to check the salt (again) because the stream
    // might have changed in between!
    if (d.salt != salt_){
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
        lost_since_ping_.fetch_add(diff - 1);
        // send event
        event e(AOO_BLOCK_GAP_EVENT, *this);
        e.block_gap.count = diff - 1;
        send_event(s, e, AOO_THREAD_AUDIO);
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
                state.resent++;
            } else {
                LOG_VERBOSE("frame " << d.frame << " of block " << d.sequence << " out of order!");
                state.reordered++;
            }
        }
    }

    // add frame to block
    block->add_frame(d.frame, (const char *)d.data, d.size);

    return true;
}

void source_desc::process_blocks(const sink_imp& s, stream_state& state){
    if (jitterbuffer_.empty()){
        return;
    }

    auto nsamples = decoder_->blocksize() * decoder_->nchannels();

    // Transfer all consecutive complete blocks
    while (!jitterbuffer_.empty() && audioqueue_.write_available()){
        const char *data;
        int32_t size;
        double sr;
        int32_t channel;

        auto& b = jitterbuffer_.front();
        if (b.complete()){
            if (b.dropped()){
                data = nullptr;
                size = 0;
                sr = decoder_->samplerate(); // nominal samplerate
                channel = -1; // current channel
            #if AOO_DEBUG_JITTER_BUFFER
                DO_LOG_DEBUG("jitter buffer: write empty block ("
                          << b.sequence << ") for source xrun");
            #endif
                // record dropped block
                state.dropped++;
            } else {
                // block is ready
                data = b.data();
                size = b.size();
                sr = b.samplerate; // real samplerate
                channel = b.channel;
            #if AOO_DEBUG_JITTER_BUFFER
                DO_LOG_DEBUG("jitter buffer: write samples for block ("
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
                sr = decoder_->samplerate(); // nominal samplerate
                channel = -1; // current channel
                add_lost(state, 1);
                LOG_VERBOSE("dropped block " << b.sequence);
            } else {
                // wait for block
            #if AOO_DEBUG_JITTER_BUFFER
                DO_LOG_DEBUG("jitter buffer: wait");
            #endif
                break;
            }
        }

        // push samples and channel
        auto d = (block_data *)audioqueue_.write_data();
        d->header.samplerate = sr;
        d->header.channel = channel;
        // decode and push audio data
        auto n = nsamples;
        if (decoder_->decode(data, size, d->data, n) != AOO_OK){
            LOG_WARNING("aoo_sink: couldn't decode block!");
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

void source_desc::skip_blocks(const sink_imp& s){
    auto n = std::min<int>(skipblocks_, jitterbuffer_.size());
    LOG_VERBOSE("skip " << n << " blocks");
    while (n--){
        jitterbuffer_.pop_front();
    }
}

// /aoo/src/<id>/data <sink> <salt> <seq0> <frame0> <seq1> <frame1> ...

// deal with "holes" in block queue
void source_desc::check_missing_blocks(const sink_imp& s){
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

// /aoo/<id>/ping <sink>
// called without lock!
void source_desc::send_ping_reply(const sink_imp &s, const sendfn &fn,
                                  const request& r){
    auto lost_blocks = lost_since_ping_.exchange(0);

    char buffer[AOO_MAXPACKETSIZE];
    osc::OutboundPacketStream msg(buffer, sizeof(buffer));

    // make OSC address pattern
    const int32_t max_addr_size = AOO_MSG_DOMAIN_LEN
            + AOO_MSG_SOURCE_LEN + 16 + AOO_MSG_PING_LEN;
    char address[max_addr_size];
    snprintf(address, sizeof(address), "%s%s/%d%s",
             AOO_MSG_DOMAIN, AOO_MSG_SOURCE, id_, AOO_MSG_PING);

    msg << osc::BeginMessage(address) << s.id()
        << osc::TimeTag(r.ping.tt1)
        << osc::TimeTag(r.ping.tt2)
        << lost_blocks
        << osc::EndMessage;

    fn(msg.Data(), msg.Size(), addr_, flags_);

    LOG_DEBUG("send /ping to source " << id_);
}

// /aoo/src/<id>/format <sink> <version>
// [<salt> <numchannels> <samplerate> <blocksize> <codec> <options>]
// called without lock!
void source_desc::send_format_request(const sink_imp& s, const sendfn& fn,
                                      bool format) {
    LOG_VERBOSE("request format for source " << id_);
    char buf[AOO_MAXPACKETSIZE];
    osc::OutboundPacketStream msg(buf, sizeof(buf));

    // make OSC address pattern
    const int32_t max_addr_size = AOO_MSG_DOMAIN_LEN +
            AOO_MSG_SOURCE_LEN + 16 + AOO_MSG_FORMAT_LEN;
    char address[max_addr_size];
    snprintf(address, sizeof(address), "%s%s/%d%s",
             AOO_MSG_DOMAIN, AOO_MSG_SOURCE, id_, AOO_MSG_FORMAT);

    msg << osc::BeginMessage(address) << s.id() << (int32_t)make_version();

    if (format){
        scoped_shared_lock lock(mutex_); // !
        // check again!
        if (!format_request_){
            return;
        }

        auto delta = s.elapsed_time() - format_time_;
        // for now just reuse source timeout
        if (delta < s.source_timeout()){
            auto salt = salt_;

            auto& f = *format_request_;

            auto c = aoo::find_codec(f.codec);
            assert(c != nullptr);

            char buf[AOO_CODEC_MAXSETTINGSIZE];
            int32_t size;
            if (c->serialize(f, buf, size) == AOO_OK){
                LOG_DEBUG("codec = " << f.codec << ", nchannels = "
                          << f.nchannels << ", sr = " << f.samplerate
                          << ", blocksize = " << f.blocksize
                          << ", option size = " << size);

                msg << salt << f.nchannels << f.samplerate
                    << f.blocksize << f.codec << osc::Blob(buf, size);
            }
        } else {
            LOG_DEBUG("format request timeout");

            event e(AOO_FORMAT_TIMEOUT_EVENT, *this);

            send_event(s, e, AOO_THREAD_NETWORK);

            // clear request
            // this is safe even with a reader lock,
            // because elsewhere it is always read/written
            // with a writer lock, see request_format()
            // and handle_format().
            format_request_ = nullptr;
        }
    }

    msg << osc::EndMessage;

    fn(msg.Data(), msg.Size(), addr_, flags_);
}

// /aoo/src/<id>/data <id> <salt> <seq1> <frame1> <seq2> <frame2> etc.
// or
// (header), id (int32), salt (int32), count (int32),
// seq1 (int32), frame1(int32), seq2(int32), frame2(seq), etc.

void source_desc::send_data_requests(const sink_imp& s, const sendfn& fn){
    if (datarequestqueue_.empty()){
        return;
    }

    shared_lock lock(mutex_);
    int32_t salt = salt_; // cache!
    lock.unlock();

    char buf[AOO_MAXPACKETSIZE];

    if (binary_.load(std::memory_order_relaxed)){
        const int32_t maxdatasize = s.packetsize()
                - (AOO_BIN_MSG_HEADER_SIZE + 8); // id + salt
        const int32_t maxrequests = maxdatasize / 8; // 2 * int32
        int32_t numrequests = 0;

        auto it = buf;
        // write header
        memcpy(it, AOO_BIN_MSG_DOMAIN, AOO_BIN_MSG_DOMAIN_SIZE);
        it += AOO_BIN_MSG_DOMAIN_SIZE;
        aoo::write_bytes<int16_t>(AOO_TYPE_SOURCE, it);
        aoo::write_bytes<int16_t>(AOO_BIN_MSG_CMD_DATA, it);
        aoo::write_bytes<int32_t>(id(), it);
        // write first 2 args (constant)
        aoo::write_bytes<int32_t>(s.id(), it);
        aoo::write_bytes<int32_t>(salt, it);
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
                fn(buf, it - buf, address(), flags_);
                // prepare next message (just rewind)
                it = head;
                numrequests = 0;
            }
        }

        if (numrequests > 0){
            // write 'count' field
            aoo::to_bytes(numrequests, head - sizeof(int32_t));
            // send it off
            fn(buf, it - buf, address(), flags_);
        }
    } else {
        char buf[AOO_MAXPACKETSIZE];
        osc::OutboundPacketStream msg(buf, sizeof(buf));

        // make OSC address pattern
        const int32_t maxaddrsize = AOO_MSG_DOMAIN_LEN +
                AOO_MSG_SOURCE_LEN + 16 + AOO_MSG_DATA_LEN;
        char pattern[maxaddrsize];
        snprintf(pattern, sizeof(pattern), "%s%s/%d%s",
                 AOO_MSG_DOMAIN, AOO_MSG_SOURCE, id_, AOO_MSG_DATA);

        const int32_t maxdatasize = s.packetsize() - maxaddrsize - 16; // id + salt + padding
        const int32_t maxrequests = maxdatasize / 10; // 2 * (int32_t + typetag)
        int32_t numrequests = 0;

        msg << osc::BeginMessage(pattern) << s.id() << salt;

        data_request r;
        while (datarequestqueue_.try_pop(r)){
            LOG_DEBUG("send data request (" << r.sequence
                      << " " << r.frame << ")");

            msg << r.sequence << r.frame;
            if (++numrequests >= maxrequests){
                // send it off
                msg << osc::EndMessage;

                fn(msg.Data(), msg.Size(), address(), flags_);

                // prepare next message
                msg.Clear();
                msg << osc::BeginMessage(pattern) << s.id() << salt;
                numrequests = 0;
            }
        }

        if (numrequests > 0){
            // send it off
            msg << osc::EndMessage;

            fn(msg.Data(), msg.Size(), address(), flags_);
        }
    }
}

// /aoo/src/<id>/invite <sink>

// called without lock!
void source_desc::send_invitation(const sink_imp& s, const sendfn& fn){
    char buffer[AOO_MAXPACKETSIZE];
    osc::OutboundPacketStream msg(buffer, sizeof(buffer));

    // make OSC address pattern
    const int32_t max_addr_size = AOO_MSG_DOMAIN_LEN
            + AOO_MSG_SOURCE_LEN + 16 + AOO_MSG_INVITE_LEN;
    char address[max_addr_size];
    snprintf(address, sizeof(address), "%s%s/%d%s",
             AOO_MSG_DOMAIN, AOO_MSG_SOURCE, id_, AOO_MSG_INVITE);

    msg << osc::BeginMessage(address) << s.id() << osc::EndMessage;

    fn(msg.Data(), msg.Size(), addr_, flags_);

    LOG_DEBUG("send /invite to source " << id_);
}

// called without lock!
void source_desc::send_uninvitation(const sink_imp& s, const sendfn &fn){
    // /aoo/<id>/uninvite <sink>
    char buffer[AOO_MAXPACKETSIZE];
    osc::OutboundPacketStream msg(buffer, sizeof(buffer));

    // make OSC address pattern
    const int32_t max_addr_size = AOO_MSG_DOMAIN_LEN
            + AOO_MSG_SOURCE_LEN + 16 + AOO_MSG_UNINVITE_LEN;
    char address[max_addr_size];
    snprintf(address, sizeof(address), "%s%s/%d%s",
             AOO_MSG_DOMAIN, AOO_MSG_SOURCE, id_, AOO_MSG_UNINVITE);

    msg << osc::BeginMessage(address) << s.id() << osc::EndMessage;

    fn(msg.Data(), msg.Size(), addr_, flags_);

    LOG_DEBUG("send /uninvite source " << id_);
}

void source_desc::send_event(const sink_imp& s, const event& e,
                             aoo_thread_level level){
    switch (s.event_mode()){
    case AOO_EVENT_POLL:
        eventqueue_.push(e);
        break;
    case AOO_EVENT_CALLBACK:
        s.call_event(e, level);
        break;
    default:
        break;
    }
}

} // aoo
