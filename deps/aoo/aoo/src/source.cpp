/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "source.hpp"

#include <cstring>
#include <algorithm>
#include <cmath>

const size_t kAooEventQueueSize = 8;

// OSC data message
// address pattern string: max 32 bytes
// typetag string: max. 12 bytes
// args (without blob data): 36 bytes
#define kAooMsgDataHeaderSize 80

// binary data message:
// header: 12 bytes
// args: 48 bytes (max.)
#define kAooBinMsgDataHeaderSize 48

//-------------------- sink_desc ------------------------//

namespace aoo {

// called while locked
void sink_desc::start(){
    // clear requests, just to make sure we don't resend
    // frames with a previous format.
    data_requests_.clear();
    if (is_active()){
        stream_id_.store(get_random_id());
        LOG_DEBUG(ep << ": start new stream (" << stream_id() << ")");
        notify_start();
    }
}

// called while locked
void sink_desc::stop(Source& s){
    if (is_active()){
        LOG_DEBUG(ep << ": stop stream (" << stream_id() << ")");
        sink_request r(request_type::stop, ep);
        r.stop.stream = stream_id();
        s.push_request(r);
    }
}

bool sink_desc::need_invite(AooId token){
    // avoid redundant invitation events
    if (token != invite_token_){
        invite_token_ = token;
        LOG_DEBUG(ep << ": received invitation ("
                  << token << ")");
        return true;
    } else {
        LOG_DEBUG(ep << ": invitation already received ("
                  << token << ")");
        return false;
    }
}

void sink_desc::accept_invitation(Source &s, AooId token){
    if (token != kAooIdInvalid){
        stream_id_.store(token); // activates sink
        LOG_DEBUG(ep << ": accept invitation (" << token << ")");
        if (s.is_running()){
            notify_start();
            s.notify_start();
        }
    } else {
        // nothing to do, just let the remote side timeout
        LOG_DEBUG(ep << ": don't accept invitation (" << token << ")");
    }
}

bool sink_desc::need_uninvite(AooId token){
    // avoid redundant invitation events
    if (token != uninvite_token_){
        uninvite_token_ = token;
        LOG_DEBUG(ep << ": received uninvitation ("
                  << token << ")");
        return true;
    } else {
        LOG_DEBUG(ep << ": uninvitation already received ("
                  << token << ")");
        return false;
    }
}

void sink_desc::accept_uninvitation(Source &s, AooId token){
    if (token != kAooIdInvalid){
        // deactivate the source, but check if we're still on the
        // same stream as the remote end (probably not necessary).
        if (stream_id() == token){
            stream_id_.store(kAooIdInvalid);
            LOG_DEBUG(ep << ": accept uninvitation (" << token << ")");
            if (s.is_running()){
                sink_request r(request_type::stop, ep);
                r.stop.stream = token;
                s.push_request(r);
            }
        } else {
            LOG_DEBUG(ep << ": invitation already accepted (" << token << ")");
        }
    } else {
        // nothing to do, just let the remote side timeout
        LOG_DEBUG(ep << ": don't accept uninvitation (" << token << ")");
    }
}

void sink_desc::activate(Source& s, bool b) {
    if (b){
        stream_id_.store(get_random_id());
        LOG_DEBUG(ep << ": activate (" << stream_id() << ")");
        if (s.is_running()){
            notify_start();
            s.notify_start();
        }
    } else {
        auto stream = stream_id_.exchange(kAooIdInvalid);
        LOG_DEBUG(ep << ": deactivate (" << stream << ")");
        if (s.is_running()){
            sink_request r(request_type::stop, ep);
            r.stop.stream = stream;
            s.push_request(r);
        }
    }
}

} // namespace aoo

//---------------------- Source -------------------------//

AOO_API AooSource * AOO_CALL AooSource_new(
        AooId id, AooFlag flags, AooError *err) {
    return aoo::construct<aoo::Source>(id, flags, err);
}

aoo::Source::Source(AooId id, AooFlag flags, AooError *err)
    : id_(id)
{
    // event queue
    eventqueue_.reserve(kAooEventQueueSize);
    // request queues
    // formatrequestqueue_.resize(64);
    // datarequestqueue_.resize(1024);
    allocate_metadata(metadata_size_.load());
}

AOO_API void AOO_CALL AooSource_free(AooSource *src){
    // cast to correct type because base class
    // has no virtual destructor!
    aoo::destroy(static_cast<aoo::Source *>(src));
}

aoo::Source::~Source() {
    // flush event queue
    endpoint_event e;
    while (eventqueue_.try_pop(e)) {
        free_event(e);
    }
    // free metadata
    if (metadata_){
        auto size = flat_metadata_maxsize(metadata_size_.load());
        aoo::deallocate(metadata_, size);
    }
}

template<typename T>
T& as(void *p){
    return *reinterpret_cast<T *>(p);
}

#define CHECKARG(type) assert(size == sizeof(type))

#define GETSINKARG \
    sink_lock lock(sinks_);             \
    auto sink = get_sink_arg(index);    \
    if (!sink) {                        \
        return kAooErrorUnknown;   \
    }                                   \

AOO_API AooError AOO_CALL AooSource_control(
        AooSource *src, AooCtl ctl, AooIntPtr index, void *ptr, AooSize size)
{
    return src->control(ctl, index, ptr, size);
}

AooError AOO_CALL aoo::Source::control(
        AooCtl ctl, AooIntPtr index, void *ptr, AooSize size)
{
    switch (ctl){
    // stream meta data
    case kAooCtlSetStreamMetadataSize:
    {
        CHECKARG(AooInt32);
        auto size = as<AooInt32>(ptr);
        if (size < 0){
            return kAooErrorBadArgument;
        }
        // the lock simplifies start_stream()
        unique_lock lock(update_mutex_);
        allocate_metadata(size);
        break;
    }
    case kAooCtlActivate:
    {
        CHECKARG(AooBool);
        GETSINKARG
        bool active = as<AooBool>(ptr);
        sink->activate(*this, active);

        break;
    }
    case kAooCtlIsActive:
    {
        CHECKARG(AooBool);
        GETSINKARG
        as<AooBool>(ptr) = sink->is_active();
        break;
    }
    case kAooCtlGetStreamMetadataSize:
        CHECKARG(AooInt32);
        as<AooInt32>(ptr) = metadata_size_.load();
        break;
    // set/get format
    case kAooCtlSetFormat:
        assert(size >= sizeof(AooFormat));
        return set_format(as<AooFormat>(ptr));
    case kAooCtlGetFormat:
        assert(size >= sizeof(AooFormat));
        return get_format(as<AooFormat>(ptr));
    // set/get channel onset
    case kAooCtlSetChannelOnset:
    {
        CHECKARG(int32_t);
        GETSINKARG
        auto chn = as<int32_t>(ptr);
        sink->set_channel(chn);
        LOG_VERBOSE("aoo_source: send to sink " << sink->ep
                    << " on channel " << chn);
        break;
    }
    case kAooCtlGetChannelOnset:
    {
        CHECKARG(int32_t);
        GETSINKARG
        as<int32_t>(ptr) = sink->channel();
        break;
    }
    // id
    case kAooCtlSetId:
    {
        auto newid = as<int32_t>(ptr);
        if (id_.exchange(newid) != newid){
            // if playing, restart
            auto expected = stream_state::run;
            state_.compare_exchange_strong(expected, stream_state::start);
        }
        break;
    }
    case kAooCtlGetId:
        CHECKARG(int32_t);
        as<AooId>(ptr) = id();
        break;
    // set/get buffersize
    case kAooCtlSetBufferSize:
    {
        CHECKARG(AooSeconds);
        auto bufsize = std::max<AooSeconds>(as<AooSeconds>(ptr), 0);
        if (buffersize_.exchange(bufsize) != bufsize){
            scoped_lock lock(update_mutex_); // writer lock!
            update_audioqueue();
        }
        break;
    }
    case kAooCtlGetBufferSize:
        CHECKARG(AooSeconds);
        as<AooSeconds>(ptr) = buffersize_.load();
        break;
    // set/get packetsize
    case kAooCtlSetPacketSize:
    {
        CHECKARG(int32_t);
        const int32_t minpacketsize = kAooMsgDataHeaderSize + 64;
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
    // set/get timer check
    case kAooCtlSetXRunDetection:
        CHECKARG(AooBool);
        timer_check_.store(as<AooBool>(ptr));
        break;
    case kAooCtlGetXRunDetection:
        CHECKARG(AooBool);
        as<AooBool>(ptr) = timer_check_.load();
        break;
    // set/get dynamic resampling
    case kAooCtlSetDynamicResampling:
        CHECKARG(AooBool);
        dynamic_resampling_.store(as<AooBool>(ptr));
        timer_.reset(); // !
        break;
    case kAooCtlGetDynamicResampling:
        CHECKARG(AooBool);
        as<AooBool>(ptr) = dynamic_resampling_.load();
        break;
    // set/get time DLL filter bandwidth
    case kAooCtlSetDllBandwidth:
        CHECKARG(float);
        // time filter
        dll_bandwidth_.store(as<float>(ptr));
        timer_.reset(); // will update
        break;
    case kAooCtlGetDllBandwidth:
        CHECKARG(float);
        as<float>(ptr) = dll_bandwidth_.load();
        break;
    // get real samplerate
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
        CHECKARG(int32_t);
        as<int32_t>(ptr) = ping_interval_.load() * 1000.0;
        break;
    // set/get resend buffer size
    case kAooCtlSetResendBufferSize:
    {
        CHECKARG(AooSeconds);
        // empty buffer is allowed! (no resending)
        auto bufsize = std::max<AooSeconds>(as<AooSeconds>(ptr), 0);
        if (resend_buffersize_.exchange(bufsize) != bufsize){
            scoped_lock lock(update_mutex_); // writer lock!
            update_historybuffer();
        }
        break;
    }
    case kAooCtlGetResendBufferSize:
        CHECKARG(AooSeconds);
        as<AooSeconds>(ptr) = resend_buffersize_.load();
        break;
    // set/get redundancy
    case kAooCtlSetRedundancy:
    {
        CHECKARG(int32_t);
        // limit it somehow, 16 times is already very high
        auto redundancy = std::max<int32_t>(1, std::min<int32_t>(16, as<int32_t>(ptr)));
        redundancy_.store(redundancy);
        break;
    }
    case kAooCtlGetRedundancy:
        CHECKARG(int32_t);
        as<int32_t>(ptr) = redundancy_.load();
        break;
#if USE_AOO_NET
    case kAooCtlSetClient:
        client_ = reinterpret_cast<AooClient *>(index);
        break;
#endif
    // unknown
    default:
        LOG_WARNING("aoo_source: unsupported control " << ctl);
        return kAooErrorNotImplemented;
    }
    return kAooOk;
}

AOO_API AooError AOO_CALL AooSource_codecControl(
        AooSource *source, AooCtl ctl, AooIntPtr index, void *data, AooSize size)
{
    return source->codecControl(ctl, index, data, size);
}

AooError AOO_CALL aoo::Source::codecControl(
        AooCtl ctl, AooIntPtr index, void *data, AooSize size) {
    // we don't know which controls are setters and which
    // are getters, so we just take a writer lock for either way.
    unique_lock lock(update_mutex_);
    if (encoder_){
        return AooEncoder_control(encoder_.get(), ctl, data, size);
    } else {
        return kAooErrorUnknown;
    }
}

AOO_API AooError AOO_CALL AooSource_setup(
        AooSource *src, AooSampleRate samplerate,
        AooInt32 blocksize, AooInt32 nchannels){
    return src->setup(samplerate, blocksize, nchannels);
}

AooError AOO_CALL aoo::Source::setup(
        AooSampleRate samplerate, AooInt32 blocksize, AooInt32 nchannels){
    scoped_lock lock(update_mutex_); // writer lock!
    if (samplerate > 0 && blocksize > 0 && nchannels > 0)
    {
        if (samplerate != samplerate_ || blocksize != blocksize_ ||
            nchannels != nchannels_)
        {
            nchannels_ = nchannels;
            samplerate_ = samplerate;
            blocksize_ = blocksize;

            realsr_.store(samplerate);

            if (encoder_){
                update_audioqueue();

                if (need_resampling()){
                    update_resampler();
                }

                update_historybuffer();
            }

            make_new_stream();
        }

        // always reset timer + time DLL filter
        timer_.setup(samplerate_, blocksize_, timer_check_.load());

        return kAooOk;
    } else {
        return kAooErrorUnknown;
    }
}

AOO_API AooError AOO_CALL AooSource_handleMessage(
        AooSource *src, const AooByte *data, AooInt32 size,
        const void *address, AooAddrSize addrlen) {
    return src->handleMessage(data, size, address, addrlen);
}

// /aoo/src/<id>/format <sink>
AooError AOO_CALL aoo::Source::handleMessage(
        const AooByte *data, AooInt32 size,
        const void *address, AooAddrSize addrlen){
    AooMsgType type;
    AooId src;
    AooInt32 onset;
    auto err = aoo_parsePattern(data, size, &type, &src, &onset);
    if (err != kAooOk){
        LOG_WARNING("aoo_source: not an AoO message!");
        return kAooErrorBadArgument;
    }
    if (type != kAooTypeSource){
        LOG_WARNING("aoo_source: not a source message!");
        return kAooErrorBadArgument;
    }
    if (src != id()){
        LOG_WARNING("aoo_source: wrong source ID!");
        return kAooErrorBadArgument;
    }

    ip_address addr((const sockaddr *)address, addrlen);

    if (data[0] == 0){
        // binary message
        auto cmd = aoo::from_bytes<int16_t>(data + kAooBinMsgDomainSize + 2);
        switch (cmd){
        case kAooBinMsgCmdData:
            handle_data_request(data + onset, size - onset, addr);
            return kAooOk;
        default:
            return kAooErrorBadArgument;
        }
    } else {
        try {
            osc::ReceivedPacket packet((const char *)data, size);
            osc::ReceivedMessage msg(packet);

            auto pattern = msg.AddressPattern() + onset;
            if (!strcmp(pattern, kAooMsgStart)){
                handle_start_request(msg, addr);
            } else if (!strcmp(pattern, kAooMsgData)){
                handle_data_request(msg, addr);
            } else if (!strcmp(pattern, kAooMsgInvite)){
                handle_invite(msg, addr);
            } else if (!strcmp(pattern, kAooMsgUninvite)){
                handle_uninvite(msg, addr);
            } else if (!strcmp(pattern, kAooMsgPing)){
                handle_ping(msg, addr);
            } else {
                LOG_WARNING("unknown message " << pattern);
                return kAooErrorUnknown;
            }
            return kAooOk;
        } catch (const osc::Exception& e){
            LOG_ERROR("aoo_source: exception in handle_message: " << e.what());
            return kAooErrorUnknown;
        }
    }
}

AOO_API AooError AOO_CALL AooSource_send(
        AooSource *src, AooSendFunc fn, void *user) {
    return src->send(fn, user);
}

// This method reads audio samples from the ringbuffer,
// encodes them and sends them to all sinks.
AooError AOO_CALL aoo::Source::send(AooSendFunc fn, void *user) {
#if 0
    // this breaks the /stop message!
    if (state_.load() != stream_state::run){
        return kAooOk; // nothing to do
    }
#endif
    sendfn reply(fn, user);

    // *first* dispatch requests (/stop messages)
    dispatch_requests(reply);

    send_start(reply);

    send_data(reply);

    resend_data(reply);

    send_ping(reply);

    if (!sinks_.try_free()){
        // LOG_DEBUG("AooSource: try_free() would block");
    }

    return kAooOk;
}

AOO_API AooError AOO_CALL AooSource_process(
        AooSource *src, AooSample **data, AooInt32 n, AooNtpTime t) {
    return src->process(data, n, t);
}

#define NO_SINKS_IDLE 1

AooError AOO_CALL aoo::Source::process(
        AooSample **data, AooInt32 nsamples, AooNtpTime t) {
    auto state = state_.load();
    if (state == stream_state::idle){
        return kAooErrorIdle; // pausing
    } else if (state == stream_state::stop){
        sink_lock lock(sinks_);
        for (auto& s : sinks_){
            s.stop(*this);
        }

        // check if we have been started in the meantime
        auto expected = stream_state::stop;
        if (state_.compare_exchange_strong(expected, stream_state::idle)){
            // don't return kAooIdle because we want to send the /stop messages !
            return kAooOk;
        }
    } else if (state == stream_state::start){
        // start -> play
        // the mutex should be uncontended most of the time.
        // although it is repeatedly locked in send(), the latter
        // returns early if we're not already playing.
        unique_lock lock(update_mutex_, sync::try_to_lock); // writer lock!
        if (!lock.owns_lock()){
            LOG_VERBOSE("aoo_source: process would block");
            // no need to call xrun()!
            return kAooErrorIdle; // ?
        }

        make_new_stream();

        // check if we have been stopped in the meantime
        auto expected = stream_state::start;
        if (!state_.compare_exchange_strong(expected, stream_state::run)){
            return kAooErrorIdle; // pausing
        }
    }

    // update timer
    // always do this, even if there are no sinks.
    // do it *before* trying to lock the mutex
    bool dynamic_resampling = dynamic_resampling_.load();
    double error;
    auto timerstate = timer_.update(t, error);
    if (timerstate == timer::state::reset){
        LOG_DEBUG("setup time DLL filter for source");
        auto bw = dll_bandwidth_.load();
        dll_.setup(samplerate_, blocksize_, bw, 0);
        realsr_.store(samplerate_);
        // it is safe to set 'lastpingtime' after updating
        // the timer, because in the worst case the ping
        // is simply sent the next time.
        lastpingtime_.store(-1e007); // force first ping
    } else if (timerstate == timer::state::error){
        // calculate xrun blocks
        double nblocks = error * (double)samplerate_ / (double)blocksize_;
    #if NO_SINKS_IDLE
        // only when we have sinks, to avoid accumulating empty blocks
        if (!sinks_.empty())
    #endif
        {
            add_xrun(nblocks);
        }
        LOG_DEBUG("xrun: " << nblocks << " blocks");

        endpoint_event e(kAooEventXRun);
        e.xrun.count = nblocks + 0.5; // ?
        send_event(e, kAooThreadLevelAudio);

        timer_.reset();
    } else if (dynamic_resampling){
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

#if NO_SINKS_IDLE
    if (sinks_.empty()){
        // nothing to do. users still have to check for pending events,
        // but there is no reason to call send()
        return kAooErrorIdle;
    }
#endif

    // the mutex should be available most of the time.
    // it is only locked exclusively when setting certain options,
    // e.g. changing the buffer size.
    shared_lock lock(update_mutex_, sync::try_to_lock); // reader lock!
    if (!lock.owns_lock()){
        LOG_VERBOSE("aoo_source: process would block");
        add_xrun(1);
        return kAooErrorIdle; // ?
    }

    if (!encoder_){
        return kAooErrorIdle;
    }

    // non-interleaved -> interleaved
    // only as many channels as current format needs
    auto nfchannels = format_->numChannels;
    auto insize = nsamples * nfchannels;
    auto buf = (AooSample *)alloca(insize * sizeof(AooSample));
    for (int i = 0; i < nfchannels; ++i){
        if (i < nchannels_){
            for (int j = 0; j < nsamples; ++j){
                buf[j * nfchannels + i] = data[i][j];
            }
        } else {
            // zero remaining channel
            for (int j = 0; j < nsamples; ++j){
                buf[j * nfchannels + i] = 0;
            }
        }
    }

    double sr;
    if (dynamic_resampling){
        sr = realsr_.load() / (double)samplerate_
                * (double)format_->sampleRate;
    } else {
        sr = format_->sampleRate;
    }

    auto outsize = nfchannels * format_->blockSize;
#if AOO_DEBUG_AUDIO_BUFFER
    auto resampler_size = resampler_.size() / (double)(nchannels_ * blocksize_);
    LOG_DEBUG("audioqueue: " << audioqueue_.read_available() / resampler_.ratio()
              << ", resampler: " << resampler_size / resampler_.ratio()
              << ", capacity: " << audioqueue_.capacity() / resampler_.ratio());
#endif
    if (need_resampling()){
        // *first* try to move samples from resampler to audiobuffer
        while (audioqueue_.write_available()){
            // copy audio samples
            auto ptr = (block_data *)audioqueue_.write_data();
            if (!resampler_.read(ptr->data, outsize)){
                break;
            }
            // push samplerate
            ptr->sr = sr;

            audioqueue_.write_commit();
        }
        // now try to write to resampler
        if (!resampler_.write(buf, insize)){
            LOG_WARNING("aoo_source: send buffer overflow");
            add_xrun(1);
            // don't return kAooErrorIdle, otherwise the send thread
            // wouldn't drain the buffer.
            return kAooErrorUnknown;
        }
    } else {
        // bypass resampler
        if (audioqueue_.write_available()){
            auto ptr = (block_data *)audioqueue_.write_data();
            // copy audio samples
            std::copy(buf, buf + outsize, ptr->data);
            // push samplerate
            ptr->sr = sr;

            audioqueue_.write_commit();
        } else {
            LOG_WARNING("aoo_source: send buffer overflow");
            add_xrun(1);
            // don't return kAooErrorIdle, otherwise the send thread
            // wouldn't drain the buffer.
            return kAooErrorUnknown;
        }
    }
    return kAooOk;
}

AOO_API AooError AOO_CALL AooSource_setEventHandler(
        AooSource *src, AooEventHandler fn,
        void *user, AooEventMode mode)
{
    return src->setEventHandler(fn, user, mode);
}

AooError AOO_CALL aoo::Source::setEventHandler(
        AooEventHandler fn, void *user, AooEventMode mode){
    eventhandler_ = fn;
    eventcontext_ = user;
    eventmode_ = mode;
    return kAooOk;
}

AOO_API AooBool AOO_CALL AooSource_eventsAvailable(AooSource *src){
    return src->eventsAvailable();
}

AooBool AOO_CALL aoo::Source::eventsAvailable(){
    return !eventqueue_.empty();
}

AOO_API AooError AOO_CALL AooSource_pollEvents(AooSource *src){
    return src->pollEvents();
}

AooError AOO_CALL aoo::Source::pollEvents(){
    // always thread-safe
    endpoint_event e;
    while (eventqueue_.try_pop(e)) {
        eventhandler_(eventcontext_, &e.event, kAooThreadLevelUnknown);
        free_event(e);
    }
    return kAooOk;
}

AOO_API AooError AOO_CALL AooSource_startStream(
        AooSource *source, const AooDataView *metadata)
{
    return source->startStream(metadata);
}

#define STREAM_METADATA_WARN 1

AooError AOO_CALL aoo::Source::startStream(const AooDataView *md) {
    // check metadata
    if (md) {
        // check type name length
        if (strlen(md->type) > kAooDataTypeMaxLen){
            LOG_ERROR("stream metadata type name must not be larger than "
                      << kAooDataTypeMaxLen << " characters!");
        #if STREAM_METADATA_WARN
            LOG_WARNING("ignoring stream metadata");
            md = nullptr;
        #else
            return kAooErrorBadArgument;
        #endif
        }
        // check data size
        if (md && md->size == 0){
            LOG_ERROR("stream metadata cannot be empty!");
        #if STREAM_METADATA_WARN
            LOG_WARNING("ignoring stream metadata");
            md = nullptr;
        #else
            return kAooErrorBadArgument;
        #endif
        }
        // the metadata size can only be changed while locking the update mutex!
        auto maxsize = metadata_size_.load(std::memory_order_relaxed);
        if (md && md->size > maxsize){
            LOG_ERROR("stream metadata exceeds size limit ("
                      << maxsize << " bytes)!");
        #if STREAM_METADATA_WARN
            LOG_WARNING("ignoring stream metadata");
            md = nullptr;
        #else
            return kAooErrorBadArgument;
        #endif
        }

        LOG_DEBUG("start stream with " << md->type << " metadata");
    } else {
        LOG_DEBUG("start stream");
    }

    // copy/reset metadata
    {
        scoped_spinlock lock(metadata_lock_);
        if (metadata_) {
            if (md) {
                flat_metadata_copy(*md, *metadata_);
            } else {
                // clear previous metadata
                metadata_->type = kAooDataTypeInvalid;
                metadata_->data = nullptr;
                metadata_->size = 0;
            }
        }
        // metadata needs to be "accepted" in make_new_stream()
        metadata_accepted_ = false;
    }

    state_.store(stream_state::start);

    return kAooOk;
}

AOO_API AooError AOO_CALL AooSource_stopStream(AooSource *source) {
    return source->stopStream();
}

AooError AOO_CALL aoo::Source::stopStream() {
    state_.store(stream_state::stop);
    return kAooOk;
}

AOO_API AooError AOO_CALL AooSource_addSink(
        AooSource *source, const AooEndpoint *sink, AooFlag flags)
{
    if (sink) {
        return source->addSink(*sink, flags);
    } else {
        return kAooErrorBadArgument;
    }
}

AooError AOO_CALL aoo::Source::addSink(const AooEndpoint& ep, AooFlag flags) {
    ip_address addr((const sockaddr *)ep.address, ep.addrlen);
    // sinks can be added/removed from different threads
    sync::scoped_lock<sync::mutex> lock1(sink_mutex_);
    sink_lock lock2(sinks_);
    // check if sink exists!
    if (find_sink(addr, ep.id)){
        LOG_WARNING("aoo_source: sink already added!");
        return kAooErrorUnknown;
    }
    AooId stream = (flags & kAooSinkActive) ? get_random_id() : kAooIdInvalid;
    do_add_sink(addr, ep.id, stream);
    // always succeeds
    return kAooOk;
}

AOO_API AooError AOO_CALL AooSource_removeSink(
        AooSource *source, const AooEndpoint *sink)
{
    if (sink) {
        return source->removeSink(*sink);
    } else {
        return kAooErrorBadArgument;
    }
}

AooError AOO_CALL aoo::Source::removeSink(const AooEndpoint& ep) {
    ip_address addr((const sockaddr *)ep.address, ep.addrlen);

    // sinks can be added/removed from different threads
    sync::scoped_lock<sync::mutex> lock1(sink_mutex_);
    sink_lock lock2(sinks_);
    if (do_remove_sink(addr, ep.id)){
        return kAooOk;
    } else {
        return kAooErrorUnknown;
    }
}

AOO_API AooError AOO_CALL AooSource_removeAll(AooSource *source)
{
    return source->removeAll();
}

AooError AOO_CALL aoo::Source::removeAll() {
    // just lock once for all stream ids
    scoped_shared_lock lock1(update_mutex_);

    bool running = is_running();

    // sinks can be added/removed from different threads
    sync::scoped_lock<sync::mutex> lock2(sink_mutex_);
    sink_lock lock3(sinks_);
    // send /stop messages
    for (auto& s : sinks_){
        if (running && s.is_active()){
            sink_request r(request_type::stop, s.ep);
            r.stop.stream = s.stream_id();
            push_request(r);
        }
    }
    sinks_.clear();
    return kAooOk;
}

AOO_API AooError AOO_CALL AooSource_acceptInvitation(
        AooSource *source, const AooEndpoint *sink, AooId token)
{
    if (sink) {
        return source->acceptInvitation(*sink, token);
    } else {
        return kAooErrorBadArgument;
    }
}

AooError AOO_CALL aoo::Source::acceptInvitation(const AooEndpoint& ep, AooId token) {
    ip_address addr((const sockaddr *)ep.address, ep.addrlen);
    auto sink = find_sink(addr, ep.id);
    if (sink){
        sink->accept_invitation(*this, token);
        return kAooOk;
    } else {
        LOG_ERROR("AooSink: couldn't find sink");
        return kAooErrorBadArgument;
    }
}

AOO_API AooError AOO_CALL AooSource_acceptUninvitation(
        AooSource *source, const AooEndpoint *sink, AooId token)
{
    if (sink) {
        return source->acceptUninvitation(*sink, token);
    } else {
        return kAooErrorBadArgument;
    }
}

AooError AOO_CALL aoo::Source::acceptUninvitation(const AooEndpoint& ep, AooId token) {
    ip_address addr((const sockaddr *)ep.address, ep.addrlen);
    auto sink = find_sink(addr, ep.id);
    if (sink){
        sink->accept_uninvitation(*this, token);
        return kAooOk;
    } else {
        LOG_ERROR("AooSink: couldn't find sink");
        return kAooErrorBadArgument;
    }
}

//------------------------- source --------------------------------//

namespace aoo {

sink_desc * Source::find_sink(const ip_address& addr, AooId id){
    for (auto& sink : sinks_){
        if (sink.ep.address == addr && sink.ep.id == id){
            return &sink;
        }
    }
    return nullptr;
}

aoo::sink_desc * Source::get_sink_arg(intptr_t index){
    auto ep = (const AooEndpoint *)index;
    if (!ep){
        LOG_ERROR("AooSink: missing sink argument");
        return nullptr;
    }
    ip_address addr((const sockaddr *)ep->address, ep->addrlen);
    auto sink = find_sink(addr, ep->id);
    if (!sink){
        LOG_ERROR("AooSink: couldn't find sink");
    }
    return sink;
}

sink_desc * Source::do_add_sink(const ip_address& addr, AooId id, AooId stream_id)
{
    AooFlag flags = 0;
#if USE_AOO_NET
    // check if the peer needs to be relayed
    if (client_){
        AooBool relay;
        AooEndpoint ep { addr.address(), (AooAddrSize)addr.length(), id };
        if (client_->control(kAooCtlNeedRelay,
                             reinterpret_cast<intptr_t>(&ep),
                             &relay, sizeof(relay)) == kAooOk)
        {
            if (relay == kAooTrue){
                LOG_DEBUG("sink " << addr << "|" << ep.id
                          << " needs to be relayed");
                flags |= kAooEndpointRelay;
            }
        }
    }
#endif
    auto it = sinks_.emplace_front(addr, id, flags, stream_id);
    // send /start if needed!
    if (is_running() && it->is_active()){
        it->notify_start();
        notify_start();
    }

    return &(*it);
}

bool Source::do_remove_sink(const ip_address& addr, AooId id){
    for (auto it = sinks_.begin(); it != sinks_.end(); ++it){
        if (it->ep.address == addr && it->ep.id == id){
            // send /stop if needed!
            if (is_running() && it->is_active()) {
                sink_request r(request_type::stop, it->ep);
                {
                    scoped_shared_lock lock(update_mutex_);
                    r.stop.stream = it->stream_id();
                }
                push_request(r);
            }

            sinks_.erase(it);
            return true;
        }
    }
    LOG_WARNING("aoo_source: sink not found!");
    return false;
}

AooError Source::set_format(AooFormat &f){
    auto codec = aoo::find_codec(f.codec);
    if (!codec){
        LOG_ERROR("codec '" << f.codec << "' not supported!");
        return kAooErrorUnknown;
    }

    std::unique_ptr<AooFormat, format_deleter> new_format;
    std::unique_ptr<AooCodec, encoder_deleter> new_encoder;

    // create a new encoder - will validate format!
    AooError err;
    auto enc = codec->encoderNew(&f, &err);
    if (!enc){
        LOG_ERROR("couldn't create encoder!");
        return err;
    }
    new_encoder.reset(enc);

    // save validated format
    auto fmt = aoo::allocate(f.size);
    memcpy(fmt, &f, f.size);
    new_format.reset((AooFormat *)fmt);

    scoped_lock lock(update_mutex_); // writer lock!

    format_ = std::move(new_format);
    encoder_ = std::move(new_encoder);
    format_id_ = get_random_id();

    update_audioqueue();

    if (need_resampling()){
        update_resampler();
    }

    update_historybuffer();

    // we need to start a new stream while holding the lock.
    // it might be tempting to just (atomically) set 'state_'
    // to 'stream_start::start', but then the send() method
    // could answer a format request by an existing stream with
    // the wrong format, before process() starts the new stream.
    //
    // NOTE: there's a slight race condition because 'xrun_'
    // might be incremented right afterwards, but I'm not
    // sure if this could cause any real problems..
    make_new_stream();

    return kAooOk;
}

AooError Source::get_format(AooFormat &fmt){
    shared_lock lock(update_mutex_); // read lock!
    if (format_){
        if (fmt.size >= format_->size){
            memcpy(&fmt, format_.get(), format_->size);
            return kAooOk;
        } else {
            return kAooErrorBadArgument;
        }
    } else {
        return kAooErrorUnknown;
    }
}

bool Source::need_resampling() const {
#if 1
    // always go through resampler, so we can use a variable block size
    return true;
#else
    return blocksize_ != format_->blockSize || samplerate_ != format_->sampleRate;
#endif
}

void Source::push_request(const sink_request &r){
    requests_.push(r);
}

void Source::notify_start(){
    LOG_DEBUG("notify_start()");
    needstart_.exchange(true, std::memory_order_release);
}

void Source::send_event(const endpoint_event& e, AooThreadLevel level){
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

void Source::free_event(const endpoint_event &e){
    if (e.type == kAooEventInvite){
        // free metadata
        if (e.invite.metadata){
            memory_.deallocate((void *)e.invite.metadata);
        }
    }
}

// must be real-time safe because it might be called in process()!
// always called with update lock!
void Source::make_new_stream(){
    // implicitly reset time DLL to be on the safe side
    timer_.reset();

    sequence_ = 0;
    xrun_.store(0.0); // !

    // "accept" stream metadata, see send_start()
    {
        scoped_spinlock lock(metadata_lock_);
        if (metadata_ && metadata_->size > 0) {
            metadata_accepted_ = true;
        }
    }

    // remove audio from previous stream
    resampler_.reset();

    audioqueue_.reset();

    history_.clear(); // !

    // reset encoder to avoid garbage from previous stream
    if (encoder_) {
        AooEncoder_control(encoder_.get(), kAooCodecCtlReset, nullptr, 0);
    }

    sink_lock lock(sinks_);
    for (auto& s : sinks_){
        s.start();
    }

    notify_start();
}

void Source::allocate_metadata(int32_t size){
    assert(size >= 0);

    LOG_DEBUG("allocate metadata (" << size << " bytes)");

    AooDataView * metadata = nullptr;
    if (size > 0){
        auto maxsize = flat_metadata_maxsize(size);
        metadata = (AooDataView *)aoo::allocate(maxsize);
        if (metadata){
            metadata->type = kAooDataTypeInvalid;
            metadata->data = nullptr;
            metadata->size = 0;
        } else {
            // TODO report error
            return;
        }
    }

    AooDataView *olddata;
    AooInt32 oldsize;

    // swap metadata
    {
        scoped_spinlock lock(metadata_lock_);
        olddata = metadata_;
        metadata_ = metadata;
        oldsize = metadata_size_.exchange(size);
        metadata_accepted_ = false;
    }

    // free old metadata
    if (olddata){
        assert(oldsize >= 0);
        auto oldmaxsize = flat_metadata_maxsize(oldsize);
        aoo::deallocate(metadata_, oldmaxsize);
    }
}

void Source::add_xrun(float n){
    // add with CAS loop
    auto current = xrun_.load(std::memory_order_relaxed);
    while (!xrun_.compare_exchange_weak(current, current + n))
        ;
}

void Source::update_audioqueue(){
    if (encoder_ && samplerate_ > 0){
        // recalculate buffersize from seconds to samples
        int32_t bufsize = buffersize_.load() * format_->sampleRate;
        auto d = div(bufsize, format_->blockSize);
        int32_t nbuffers = d.quot + (d.rem != 0); // round up
        // minimum buffer size depends on resampling and reblocking!
        auto downsample = (double)format_->sampleRate / (double)samplerate_;
        auto reblock = (double)format_->blockSize / (double)blocksize_;
        int32_t minblocks = std::ceil(downsample * reblock);
        nbuffers = std::max<int32_t>(nbuffers, minblocks);
        LOG_DEBUG("aoo_source: buffersize (ms): " << (buffersize_.load() * 1000.0)
                  << ", samples: " << bufsize << ", nbuffers: " << nbuffers
                  << ", minimum: " << minblocks);

        // resize audio buffer
        auto nsamples = format_->blockSize * format_->numChannels;
        auto nbytes = sizeof(block_data::sr) + nsamples * sizeof(AooSample);
        // align to 8 bytes
        nbytes = (nbytes + 7) & ~7;
        audioqueue_.resize(nbytes, nbuffers);
    #if 1
        audioqueue_.shrink_to_fit();
    #endif
    }
}

void Source::update_resampler(){
    if (encoder_ && samplerate_ > 0){
        resampler_.setup(blocksize_, format_->blockSize,
                         samplerate_, format_->sampleRate,
                         format_->numChannels);
    }
}

void Source::update_historybuffer(){
    if (encoder_){
        // bufsize can also be 0 (= don't resend)!
        int32_t bufsize = resend_buffersize_.load() * format_->sampleRate;
        auto d = div(bufsize, format_->blockSize);
        int32_t nbuffers = d.quot + (d.rem != 0); // round up
        history_.resize(nbuffers);
        LOG_DEBUG("aoo_source: history buffersize (ms): "
                  << (resend_buffersize_.load() * 1000.0)
                  << ", samples: " << bufsize << ", nbuffers: " << nbuffers);

    }
}

// /aoo/sink/<id>/start <src> <version> <stream_id> <flags>
// <lastformat> <nchannels> <samplerate> <blocksize> <codec> <extension>
// [<metadata_type> <metadata_content>]
void send_start_msg(const endpoint& ep, int32_t id, int32_t stream, int32_t lastformat,
                    const AooFormat& f, const AooByte *extension, AooInt32 size,
                    const AooDataView* metadata, const sendfn& fn) {
    LOG_DEBUG("send " kAooMsgStart " to " << ep << " (stream = " << stream << ")");

    char buf[AOO_MAX_PACKET_SIZE];
    osc::OutboundPacketStream msg(buf, sizeof(buf));

    const int32_t max_addr_size = kAooMsgDomainLen
            + kAooMsgSinkLen + 16 + kAooMsgStartLen;
    char address[max_addr_size];
    snprintf(address, sizeof(address), "%s%s/%d%s",
             kAooMsgDomain, kAooMsgSink, ep.id, kAooMsgStart);

    // stream specific flags (for future use)
    AooFlag flags = 0;

    msg << osc::BeginMessage(address) << id << (int32_t)make_version()
        << stream << (int32_t)flags << lastformat
        << f.numChannels << f.sampleRate << f.blockSize
        << f.codec << osc::Blob(extension, size);
    if (metadata) {
        msg << metadata->type << osc::Blob(metadata->data, metadata->size);
    }
    msg << osc::EndMessage;

    fn((const AooByte *)msg.Data(), msg.Size(), ep);
}

// /aoo/sink/<id>/stop <src> <stream_id>
void send_stop_msg(const endpoint& ep, int32_t id,
                   int32_t stream, const sendfn& fn) {
    LOG_DEBUG("send " kAooMsgStop " to " << ep << " (stream = " << stream << ")");

    char buf[AOO_MAX_PACKET_SIZE];
    osc::OutboundPacketStream msg(buf, sizeof(buf));

    const int32_t max_addr_size = kAooMsgDomainLen
            + kAooMsgSinkLen + 16 + kAooMsgStopLen;
    char address[max_addr_size];
    snprintf(address, sizeof(address), "%s%s/%d%s",
             kAooMsgDomain, kAooMsgSink, ep.id, kAooMsgStop);

    msg << osc::BeginMessage(address) << id << stream << osc::EndMessage;

    fn((const AooByte *)msg.Data(), msg.Size(), ep);
}

void Source::dispatch_requests(const sendfn& fn){
    sink_request r;
    while (requests_.try_pop(r)){
        if (r.type == request_type::stop){
            send_stop_msg(r.ep, id(), r.stop.stream, fn);
        }
    }
}

void Source::send_start(const sendfn& fn){
    if (!needstart_.exchange(false, std::memory_order_acquire)){
        return;
    }

    shared_lock updatelock(update_mutex_); // reader lock!

    if (!encoder_){
        return;
    }

    // cache stream format
    auto format_id = format_id_;

    AooFormatStorage f;
    memcpy(&f, format_.get(), format_->size);

    // serialize format extension
    AooByte extension[kAooFormatExtMaxSize];
    AooInt32 size = sizeof(extension);

    if (encoder_->interface->serialize(
                &f.header, extension, &size) != kAooOk) {
        return;
    }

    // cache stream metadata
    AooDataView *md = nullptr;
    {
        scoped_spinlock lock(metadata_lock_);
        // only send metadata if "accepted" in make_new_stream().
        if (metadata_accepted_) {
            assert(metadata_ != nullptr);
            assert(metadata_->size > 0);
            auto mdsize = flat_metadata_size(*metadata_);
            md = (AooDataView *)alloca(mdsize);
            flat_metadata_copy(*metadata_, *md);
        }
    }

    // cache sinks that need to send a /start message
    // we only free sinks in this thread, so we don't have to lock
#if 0
    sink_lock lock(sinks_);
#endif
    cached_sinks_.clear();
    for (auto& s : sinks_){
        if (s.need_start()){
            cached_sinks_.emplace_back(s);
        }
    }

    // send messages without lock!
    updatelock.unlock();

    for (auto& s : cached_sinks_){
        send_start_msg(s.ep, id(), s.stream_id, format_id,
                       f.header, extension, size, md, fn);
    }
}

// binary data message:
// domain (int32), type (int16), cmd (int16), sink_id (int32)
// source_id (int32), stream_id (int32), seq (int32), channel (int16), flags (int16),
// [total (int32), nframes (int16), frame (int16)],  [sr (float64)],
// size (int32), data...

void write_bin_data(const endpoint* ep, AooId id, AooId stream_id,
                    const data_packet& d, AooByte *buf, int32_t& size)
{
    int16_t flags = 0;
    if (d.samplerate != 0){
        flags |= kAooBinMsgDataSampleRate;
    }
    if (d.nframes > 1){
        flags |= kAooBinMsgDataFrames;
    }

    auto it = buf;
    // write header
    memcpy(it, kAooBinMsgDomain, kAooBinMsgDomainSize);
    it += kAooBinMsgDomainSize;
    aoo::write_bytes<int16_t>(kAooTypeSink, it);
    aoo::write_bytes<int16_t>(kAooBinMsgCmdData, it);
    if (ep){
        aoo::write_bytes<int32_t>(ep->id, it);
    } else {
        // skip
        it += sizeof(int32_t);
    }
    // write arguments
    aoo::write_bytes<int32_t>(id, it);
    aoo::write_bytes<int32_t>(stream_id, it);
    aoo::write_bytes<int32_t>(d.sequence, it);
    aoo::write_bytes<int16_t>(d.channel, it);
    aoo::write_bytes<int16_t>(flags, it);
    if (flags & kAooBinMsgDataFrames){
        aoo::write_bytes<int32_t>(d.totalsize, it);
        aoo::write_bytes<int16_t>(d.nframes, it);
        aoo::write_bytes<int16_t>(d.frame, it);
    }
    if (flags & kAooBinMsgDataSampleRate){
         aoo::write_bytes<double>(d.samplerate, it);
    }
    aoo::write_bytes<int32_t>(d.size, it);
    // write audio data
    memcpy(it, d.data, d.size);
    it += d.size;

    size = it - buf;
}

// /aoo/sink/<id>/data <src> <stream_id> <seq> <sr> <channel_onset> <totalsize> <nframes> <frame> <data>

void send_packet_osc(const endpoint& ep, AooId id, int32_t stream_id,
                     const aoo::data_packet& d, const sendfn& fn) {
    char buf[AOO_MAX_PACKET_SIZE];
    osc::OutboundPacketStream msg(buf, sizeof(buf));

    const int32_t max_addr_size = kAooMsgDomainLen
            + kAooMsgSinkLen + 16 + kAooMsgDataLen;
    char address[max_addr_size];
    snprintf(address, sizeof(address), "%s%s/%d%s",
             kAooMsgDomain, kAooMsgSink, ep.id, kAooMsgData);

    msg << osc::BeginMessage(address) << id << stream_id << d.sequence << d.samplerate
        << d.channel << d.totalsize << d.nframes << d.frame << osc::Blob(d.data, d.size)
        << osc::EndMessage;

#if AOO_DEBUG_DATA
    LOG_DEBUG("send block: seq = " << d.sequence << ", sr = "
              << d.samplerate << ", chn = " << d.channel << ", totalsize = "
              << d.totalsize << ", nframes = " << d.nframes
              << ", frame = " << d.frame << ", size " << d.size);
#endif
    fn((const AooByte *)msg.Data(), msg.Size(), ep);
}

void send_packet(const aoo::vector<cached_sink_desc>& sinks, const AooId id,
                 data_packet& d, const sendfn &fn, bool binary) {
    if (binary){
        AooByte buf[AOO_MAX_PACKET_SIZE];
        int32_t size;

        write_bin_data(nullptr, id, kAooIdInvalid, d, buf, size);

        for (auto& s : sinks){
            // overwrite sink id, stream id and channel!
            aoo::to_bytes(s.ep.id, buf + 8);
            aoo::to_bytes(s.stream_id, buf + 16);
            aoo::to_bytes<int16_t>(s.channel, buf + kAooBinMsgHeaderSize + 12);

        #if AOO_DEBUG_DATA
            LOG_DEBUG("send block: seq = " << d.sequence << ", sr = "
                      << d.samplerate << ", chn = " << s.channel << ", totalsize = "
                      << d.totalsize << ", nframes = " << d.nframes
                      << ", frame = " << d.frame << ", size " << d.size);
        #endif
            fn(buf, size, s.ep);
        }
    } else {
        for (auto& s : sinks){
            // set channel!
            d.channel = s.channel;
            send_packet_osc(s.ep, id, s.stream_id, d, fn);
        }
    }
}

void send_packet_bin(const endpoint& ep, AooId id, AooId stream_id,
                     const aoo::data_packet& d, const sendfn& fn) {
    AooByte buf[AOO_MAX_PACKET_SIZE];
    int32_t size;

    write_bin_data(&ep, id, stream_id, d, buf, size);

#if AOO_DEBUG_DATA
    LOG_DEBUG("send block: seq = " << d.sequence << ", sr = "
              << d.samplerate << ", chn = " << d.channel << ", totalsize = "
              << d.totalsize << ", nframes = " << d.nframes
              << ", frame = " << d.frame << ", size " << d.size);
#endif

    fn((const AooByte *)buf, size, ep);
}

#define XRUN_THRESHOLD 0.1

void Source::send_data(const sendfn& fn){
    int32_t last_sequence = 0;

    // NOTE: we have to lock *before* calling 'read_available'
    // on the audio queue!
    shared_lock updatelock(update_mutex_); // reader lock

    // *first* check for dropped blocks
    if (xrun_.load(std::memory_order_relaxed) > XRUN_THRESHOLD){
        // calculate number of xrun blocks (after resampling)
        float drop = xrun_.exchange(0.0) * (float)resampler_.ratio();
        // round up
        int nblocks = std::ceil(drop);
        // subtract diff with a CAS loop
        float diff = (float)nblocks - drop;
        auto current = xrun_.load(std::memory_order_relaxed);
        while (!xrun_.compare_exchange_weak(current, current - diff))
            ;
        // drop blocks
        LOG_DEBUG("aoo_source: send " << nblocks << " empty blocks for "
                  "xrun (" << (int)drop << " blocks)");
        while (nblocks--){
            // check the encoder and make snapshost of stream_id
            // in every iteration because we release the lock
            if (!encoder_){
                return;
            }
            // send empty block
            // NOTE: we're the only thread reading 'sequence_', so we can increment
            // it even while holding a reader lock!
            data_packet d;
            d.sequence = last_sequence = sequence_++;
            d.samplerate = format_->sampleRate; // use nominal samplerate
            d.channel = 0;
            d.totalsize = 0;
            d.nframes = 0;
            d.frame = 0;
            d.data = nullptr;
            d.size = 0;

            // cache sinks
            // we only free sinks in this thread, so we don't have to lock
        #if 0
            sink_lock lock(sinks_);
        #endif
            cached_sinks_.clear();
            for (auto& s : sinks_){
                if (s.is_active()){
                    cached_sinks_.emplace_back(s);
                }
            }

            // now we can unlock
            updatelock.unlock();

            // we only free sources in this thread, so we don't have to lock
        #if 0
            // this is not a real lock, so we don't have worry about dead locks
            sink_lock lock(sinks_);
        #endif
            // send block to all sinks
            send_packet(cached_sinks_, id(), d, fn, binary_.load());

            updatelock.lock();
        }
    }

    // now send audio
    while (audioqueue_.read_available()){
        if (!encoder_){
            return;
        }

        if (!sinks_.empty()){
            auto ptr = (block_data *)audioqueue_.read_data();

            data_packet d;
            d.samplerate = ptr->sr;

            // copy and convert audio samples to blob data
            auto nchannels = format_->numChannels;
            auto blocksize = format_->blockSize;
            auto nsamples = nchannels * blocksize;
        #if 0
            Log log;
            for (int i = 0; i < nsamples; ++i){
                log << ptr->data[i] << " ";
            }
        #endif

            sendbuffer_.resize(sizeof(double) * nsamples); // overallocate

            AooInt32 size = sendbuffer_.size();
            auto err = AooEncoder_encode(encoder_.get(), ptr->data, nsamples,
                                         sendbuffer_.data(), &size);
            d.totalsize = size;

            audioqueue_.read_commit(); // always commit!

            if (err != kAooOk){
                LOG_WARNING("aoo_source: couldn't encode audio data!");
                return;
            }

            // NOTE: we're the only thread reading 'sequence_', so we can increment
            // it even while holding a reader lock!
            d.sequence = last_sequence = sequence_++;

            // calculate number of frames
            bool binary = binary_.load();
            auto packetsize = packetsize_.load();
            auto maxpacketsize = packetsize -
                    (binary ? kAooBinMsgDataHeaderSize : kAooMsgDataHeaderSize);
            auto dv = div(d.totalsize, maxpacketsize);
            d.nframes = dv.quot + (dv.rem != 0);

            // save block (if we have a history buffer)
            if (history_.capacity() > 0){
                history_.push()->set(d.sequence, d.samplerate, sendbuffer_.data(),
                                     d.totalsize, d.nframes, maxpacketsize);
            }

            // cache sinks
            // we only free sinks in this thread, so we don't have to lock
        #if 0
            sink_lock lock(sinks_);
        #endif
            cached_sinks_.clear();
            for (auto& s : sinks_){
                if (s.is_active()){
                    cached_sinks_.emplace_back(s);
                }
            }

            // unlock before sending!
            updatelock.unlock();

            // from here on we don't hold any lock!

            // send a single frame to all sinks
            // /aoo/<sink>/data <src> <stream_id> <seq> <sr> <channel_onset>
            // <totalsize> <numpackets> <packetnum> <data>
            auto dosend = [&](int32_t frame, const AooByte* data, auto n){
                d.frame = frame;
                d.data = data;
                d.size = n;
                // send block to all sinks
                send_packet(cached_sinks_, id(), d, fn, binary);
            };

            auto ntimes = redundancy_.load();
            for (auto i = 0; i < ntimes; ++i){
                auto ptr = sendbuffer_.data();
                // send large frames (might be 0)
                for (int32_t j = 0; j < dv.quot; ++j, ptr += maxpacketsize){
                    dosend(j, ptr, maxpacketsize);
                }
                // send remaining bytes as a single frame (might be the only one!)
                if (dv.rem){
                    dosend(dv.quot, ptr, dv.rem);
                }
            }

            updatelock.lock();
        } else {
            // drain buffer anyway
            audioqueue_.read_commit();
        }
    }

    // handle overflow (with 64 samples @ 44.1 kHz this happens every 36 days)
    // for now just force a reset by changing the stream_id, LATER think how to handle this better
    if (last_sequence == kAooIdMax){
        updatelock.unlock();
        // not perfectly thread-safe, but shouldn't cause problems AFAICT....
        scoped_lock lock(update_mutex_);
        sequence_ = 0;
        // cache sinks
        // we only free sinks in this thread, so we don't have to lock
    #if 0
        sink_lock lock(sinks_);
    #endif
        for (auto& s : sinks_){
            s.start();
        }
        notify_start();
    }
}

void Source::resend_data(const sendfn &fn){
    shared_lock updatelock(update_mutex_); // reader lock for history buffer!
    if (!history_.capacity()){
        return;
    }

    // we only free sources in this thread, so we don't have to lock
#if 0
    // this is not a real lock, so we don't have worry about dead locks
    sink_lock lock(sinks_);
#endif
    // send block to sinks
    for (auto& s : sinks_){
        data_request request;
        while (s.get_data_request(request)){
            auto block = history_.find(request.sequence);
            if (block){
                bool binary = binary_.load();

                auto stream_id = s.stream_id();

                aoo::data_packet d;
                d.sequence = block->sequence;
                d.samplerate = block->samplerate;
                d.channel = s.channel();
                d.totalsize = block->size();
                d.nframes = block->num_frames();
                // We use a buffer on the heap because blocks and even frames
                // can be quite large and we don't want them to sit on the stack.
                if (request.frame < 0){
                    // Copy whole block and save frame pointers.
                    sendbuffer_.resize(d.totalsize);
                    AooByte *buf = sendbuffer_.data();
                    AooByte *frameptr[256];
                    int32_t framesize[256];
                    int32_t onset = 0;

                    for (int i = 0; i < d.nframes; ++i){
                        auto nbytes = block->get_frame(i, buf + onset, d.totalsize - onset);
                        if (nbytes > 0){
                            frameptr[i] = buf + onset;
                            framesize[i] = nbytes;
                            onset += nbytes;
                        } else {
                            LOG_ERROR("empty frame!");
                        }
                    }
                    // unlock before sending
                    updatelock.unlock();

                    // send frames to sink
                    for (int i = 0; i < d.nframes; ++i){
                        d.frame = i;
                        d.data = frameptr[i];
                        d.size = framesize[i];
                        if (binary){
                            send_packet_bin(s.ep, id(), stream_id, d, fn);
                        } else {
                            send_packet_osc(s.ep, id(), stream_id, d, fn);
                        }
                    }

                    // lock again
                    updatelock.lock();
                } else {
                    // Copy a single frame
                    if (request.frame >= 0 && request.frame < d.nframes){
                        int32_t size = block->frame_size(request.frame);
                        sendbuffer_.resize(size);
                        block->get_frame(request.frame, sendbuffer_.data(), size);
                        // unlock before sending
                        updatelock.unlock();

                        // send frame to sink
                        d.frame = request.frame;
                        d.data = sendbuffer_.data();
                        d.size = size;
                        if (binary){
                            send_packet_bin(s.ep, id(), stream_id, d, fn);
                        } else {
                            send_packet_osc(s.ep, id(), stream_id, d, fn);
                        }

                        // lock again
                        updatelock.lock();
                    } else {
                        LOG_ERROR("frame number " << request.frame << " out of range!");
                    }
                }
            }
        }
    }
}

void Source::send_ping(const sendfn& fn){
    // if stream is stopped, the timer won't increment anyway
    auto elapsed = timer_.get_elapsed();
    auto pingtime = lastpingtime_.load();
    auto interval = ping_interval_.load(); // 0: no ping
    if (interval > 0 && (elapsed - pingtime) >= interval){
        auto tt = timer_.get_absolute();
        // we only free sources in this thread, so we don't have to lock
    #if 0
        // this is not a real lock, so we don't have worry about dead locks
        sink_lock lock(sinks_);
    #endif
        // send ping to sinks
        for (auto& sink : sinks_){
            if (sink.is_active()){
                // /aoo/sink/<id>/ping <src> <time>
                LOG_DEBUG("send " kAooMsgPing " to " << sink.ep);

                char buf[AOO_MAX_PACKET_SIZE];
                osc::OutboundPacketStream msg(buf, sizeof(buf));

                const int32_t max_addr_size = kAooMsgDomainLen
                        + kAooMsgSinkLen + 16 + kAooMsgPingLen;
                char address[max_addr_size];
                snprintf(address, sizeof(address), "%s%s/%d%s",
                         kAooMsgDomain, kAooMsgSink, sink.ep.id, kAooMsgPing);

                msg << osc::BeginMessage(address) << id() << osc::TimeTag(tt)
                    << osc::EndMessage;

                fn((const AooByte *)msg.Data(), msg.Size(), sink.ep);
            }
        }

        lastpingtime_.store(elapsed);
    }
}

// /start <id> <version>
void Source::handle_start_request(const osc::ReceivedMessage& msg,
                                  const ip_address& addr)
{
    LOG_DEBUG("handle start request");

    auto it = msg.ArgumentsBegin();

    auto id = (it++)->AsInt32();
    auto version = (it++)->AsInt32();

    // LATER handle this in the sink_desc (e.g. not sending data)
    if (!check_version(version)){
        LOG_ERROR("aoo_source: sink version not supported");
        return;
    }

    // check if sink exists (not strictly necessary, but might help catch errors)
    sink_lock lock(sinks_);
    auto sink = find_sink(addr, id);

    if (sink){
        if (sink->is_active()){
            // just resend /start message
            sink->notify_start();

            notify_start();
        } else {
            LOG_VERBOSE("ignoring '" << kAooMsgStart << "' message: sink not active");
        }
    } else {
        LOG_VERBOSE("ignoring '" << kAooMsgStart << "' message: sink not found");
    }
}

// /aoo/src/<id>/data <id> <stream_id> <seq1> <frame1> <seq2> <frame2> etc.

void Source::handle_data_request(const osc::ReceivedMessage& msg,
                                 const ip_address& addr)
{
    auto it = msg.ArgumentsBegin();
    auto id = (it++)->AsInt32();
    auto stream_id = (it++)->AsInt32(); // we can ignore the stream_id

    LOG_DEBUG("handle data request");

    sink_lock lock(sinks_);
    auto sink = find_sink(addr, id);
    if (sink){
        if (sink->stream_id() != stream_id){
            LOG_VERBOSE("ignoring '" << kAooMsgData
                        << "' message: stream ID mismatch (outdated?)");
            return;
        }
        if (sink->is_active()){
            // get pairs of sequence + frame
            int npairs = (msg.ArgumentCount() - 2) / 2;
            while (npairs--){
                int32_t sequence = (it++)->AsInt32();
                int32_t frame = (it++)->AsInt32();
                sink->add_data_request(sequence, frame);
            }
        } else {
            LOG_VERBOSE("ignoring '" << kAooMsgData << "' message: sink not active");
        }
    } else {
        LOG_VERBOSE("ignoring '" << kAooMsgData << "' message: sink not found");
    }
}

// (header), id (int32), stream_id (int32), count (int32),
// seq1 (int32), frame1(int32), seq2(int32), frame2(seq), etc.

void Source::handle_data_request(const AooByte *msg, int32_t n,
                                 const ip_address& addr)
{
    // check size (id, stream_id, count)
    if (n < 12){
        LOG_ERROR("handle_data_request: header too small!");
        return;
    }

    auto it = msg;

    auto id = aoo::read_bytes<int32_t>(it);
    auto stream_id = aoo::read_bytes<int32_t>(it); // we can ignore the stream_id

    LOG_DEBUG("handle data request");

    sink_lock lock(sinks_);
    auto sink = find_sink(addr, id);
    if (sink){
        if (sink->stream_id() != stream_id){
            LOG_VERBOSE("ignoring binary data message: stream ID mismatch (outdated?)");
            return;
        }
        if (sink->is_active()){
            // get pairs of sequence + frame
            int count = aoo::read_bytes<int32_t>(it);
            if (n < (12 + count * sizeof(int32_t) * 2)){
                LOG_ERROR("handle_data_request: bad 'count' argument!");
                return;
            }
            while (count--){
                int32_t sequence = aoo::read_bytes<int32_t>(it);
                int32_t frame = aoo::read_bytes<int32_t>(it);
                sink->add_data_request(sequence, frame);
            }
        } else {
            LOG_VERBOSE("ignoring binary data message: sink not active");
        }
    } else {
        LOG_VERBOSE("ignoring binary data message: sink not found");
    }
}

// /aoo/src/<id>/invite <id> <stream_id> [<metadata_type> <metadata_content>]

void Source::handle_invite(const osc::ReceivedMessage& msg,
                           const ip_address& addr)
{
    auto it = msg.ArgumentsBegin();

    auto id = (it++)->AsInt32();

    auto token = (it++)->AsInt32();

    const char *type = nullptr;
    const void *ptr = nullptr;
    osc::osc_bundle_element_size_t size = 0;

    if (msg.ArgumentCount() > 2){
        type = (it++)->AsString();
        (it++)->AsBlob(ptr, size);
    }

    LOG_DEBUG("handle invitation by " << addr << "|" << id);

    // sinks can be added/removed from different threads
    sync::unique_lock<sync::mutex> lock1(sink_mutex_);
    sink_lock lock2(sinks_);
    auto sink = find_sink(addr, id);
    if (!sink){
        // the sink is initially deactivated.
        sink = do_add_sink(addr, id, kAooIdInvalid);

        // push "add" event
        endpoint_event e(kAooEventSinkAdd, addr, id);

        send_event(e, kAooThreadLevelNetwork);
    }
    // make sure that the event is only sent once per invitation.
    if (sink->need_invite(token)) {
        lock1.unlock(); // !

        // push "invite" event
        endpoint_event e(kAooEventInvite, addr, id);
        e.invite.token = token;
        e.invite.reserved = 0;

        if (type){
            // with metadata
            AooDataView src;
            src.type = type;
            src.data = (AooByte *)ptr;
            src.size = size;

            if (eventmode_ == kAooEventModePoll){
                // make copy on heap
                auto mdsize = flat_metadata_size(src);
                auto md = (AooDataView *)memory_.allocate(mdsize);
                flat_metadata_copy(src, *md);
                e.invite.metadata = md;
            } else {
                // use stack
                e.invite.metadata = &src;
            }
        } else {
            // without metadata
            e.invite.metadata = nullptr;
        }

        send_event(e, kAooThreadLevelNetwork);
    }
}

// /aoo/src/<id>/uninvite <id> <stream_id>

void Source::handle_uninvite(const osc::ReceivedMessage& msg,
                             const ip_address& addr)
{
    auto it = msg.ArgumentsBegin();

    auto id = (it++)->AsInt32();

    auto token = (it++)->AsInt32();

    LOG_DEBUG("handle uninvitation by " << addr << "|" << id);

    // check if sink exists (not strictly necessary, but might help catch errors)
    sink_lock lock(sinks_);
    auto sink = find_sink(addr, id);
    if (sink) {
        if (sink->stream_id() == token){
            if (sink->is_active()){
                // push "uninvite" event
                if (sink->need_uninvite(token)) {
                    endpoint_event e(kAooEventUninvite, addr, id);
                    e.uninvite.token = token;

                    send_event(e, kAooThreadLevelNetwork);
                }
                return; // don't send /stop message!
            } else {
                // if the sink is inactive, it probably means that we have
                // accepted the uninvitation, but the /stop message got lost.
                LOG_DEBUG("ignoring '" << kAooMsgUninvite << "' message: "
                          << " sink not active (/stop message got loast?)");
            }
        } else {
            LOG_VERBOSE("ignoring '" << kAooMsgUninvite
                        << "' message: stream token mismatch (outdated?)");
        }
    } else {
        LOG_VERBOSE("ignoring '" << kAooMsgUninvite << "' message: sink not found");
    }
    // tell the remote side that we have stopped.
    // don't use the sink because it can be NULL!
    sink_request r(request_type::stop, endpoint(addr, id, 0));
    r.stop.stream = token; // use remote stream id!
    push_request(r);
    LOG_DEBUG("resend " << kAooMsgStop << " message");
}

// /aoo/src/<id>/ping <id> <tt1> <tt2> <packetloss>

void Source::handle_ping(const osc::ReceivedMessage& msg,
                         const ip_address& addr)
{
    auto it = msg.ArgumentsBegin();
    AooId id = (it++)->AsInt32();
    time_tag tt1 = (it++)->AsTimeTag();
    time_tag tt2 = (it++)->AsTimeTag();
    float packet_loss = (it++)->AsFloat();

    LOG_DEBUG("handle ping");

    // check if sink exists (not strictly necessary, but might help catch errors)
    sink_lock lock(sinks_);
    auto sink = find_sink(addr, id);
    if (sink) {
        if (sink->is_active()){
            // push "ping" event
            endpoint_event e(kAooEventPingReply, addr, id);
            e.ping_reply.t1 = tt1;
            e.ping_reply.t2 = tt2;
        #if 0
            e.ping_reply.tt3 = timer_.get_absolute(); // use last stream time
        #else
            e.ping_reply.t3 = aoo::time_tag::now(); // use real system time
        #endif
            e.ping_reply.packetLoss = packet_loss;

            send_event(e, kAooThreadLevelNetwork);
        } else {
            LOG_VERBOSE("ignoring '" << kAooMsgPing << "' message: sink not active");
        }
    } else {
        LOG_VERBOSE("ignoring '" << kAooMsgPing << "' message: sink not found");
    }
}

} // aoo
