/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "source.hpp"

#include <cstring>
#include <algorithm>
#include <cmath>

// avoid processing if there are no sinks
#define IDLE_IF_NO_SINKS 1

namespace aoo {

// OSC data message
const int32_t kDataMaxAddrSize = kAooMsgDomainLen + kAooMsgSinkLen + 16 + kAooMsgDataLen;
// typetag string: max. 12 bytes
// args (without blob data): 40 bytes
const int32_t kDataHeaderSize = kDataMaxAddrSize + 52;

// binary data message:
// args: 32 bytes (max.)
const int32_t kBinDataHeaderSize = kAooBinMsgLargeHeaderSize + 32;

//-------------------- sink_desc ------------------------//

// called while locked
void sink_desc::start(){
    // clear requests, just to make sure we don't resend
    // frames with a previous format.
    data_requests_.clear();
    if (is_active()){
        stream_id_.store(get_random_id());
        LOG_DEBUG("AooSource: " << ep << ": start new stream (" << stream_id() << ")");
        notify_start();
    }
}

// called while locked
void sink_desc::stop(Source& s){
    if (is_active()){
        LOG_DEBUG("AooSource: " << ep << ": stop stream (" << stream_id() << ")");
        sink_request r(request_type::stop, ep);
        r.stop.stream = stream_id();
        s.push_request(r);
    }
}

bool sink_desc::need_invite(AooId token){
    // avoid redundant invitation events
    if (token != invite_token_){
        invite_token_ = token;
        LOG_DEBUG("AooSource: " << ep << ": received invitation ("
                  << token << ")");
        return true;
    } else {
        LOG_DEBUG("AooSource: " << ep << ": invitation already received ("
                  << token << ")");
        return false;
    }
}

void sink_desc::handle_invite(Source &s, AooId token, bool accept){
    if (accept){
        stream_id_.store(token); // activates sink
        LOG_DEBUG("AooSource: " << ep << ": accept invitation (" << token << ")");
        if (s.is_running()){
            notify_start();
            s.notify_start();
        }
    } else {
        LOG_DEBUG("AooSource: " << ep << ": decline invitation (" << token << ")");
        if (s.is_running()){
            sink_request r(request_type::decline, ep);
            r.decline.token = token;
            s.push_request(r);
        }
    }
}

bool sink_desc::need_uninvite(AooId token){
    // avoid redundant invitation events
    if (token != uninvite_token_){
        uninvite_token_ = token;
        LOG_DEBUG("AooSource: " << ep << ": received uninvitation ("
                  << token << ")");
        return true;
    } else {
        LOG_DEBUG("AooSource: " << ep << ": uninvitation already received ("
                  << token << ")");
        return false;
    }
}

void sink_desc::handle_uninvite(Source &s, AooId token, bool accept){
    if (accept) {
        stream_id_.store(kAooIdInvalid); // deactivates sink
        LOG_DEBUG("AooSource: " << ep << ": accept uninvitation (" << token << ")");
        if (s.is_running()){
            sink_request r(request_type::stop, ep);
            r.stop.stream = token;
            s.push_request(r);
        }
    } else {
        // nothing to do, just let the remote side timeout
        LOG_DEBUG("AooSource: " << ep << ": decline uninvitation (" << token << ")");
    }
}

void sink_desc::activate(Source& s, bool b) {
    if (b){
        stream_id_.store(get_random_id());
        LOG_DEBUG("AooSource: " << ep << ": activate (" << stream_id() << ")");
        if (s.is_running()){
            notify_start();
            s.notify_start();
        }
    } else {
        auto stream = stream_id_.exchange(kAooIdInvalid);
        LOG_DEBUG("AooSource: " << ep << ": deactivate (" << stream << ")");
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
    : id_(id) {}

AOO_API void AOO_CALL AooSource_free(AooSource *src){
    // cast to correct type because base class
    // has no virtual destructor!
    aoo::destroy(static_cast<aoo::Source *>(src));
}

aoo::Source::~Source() {}

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
    // set/get format
    case kAooCtlSetFormat:
        assert(size >= sizeof(AooFormat));
        return set_format(as<AooFormat>(ptr));
    case kAooCtlGetFormat:
        assert(size >= sizeof(AooFormat));
        return get_format(as<AooFormat>(ptr), size);
    // set/get channel onset
    case kAooCtlSetChannelOnset:
    {
        CHECKARG(int32_t);
        GETSINKARG
        auto chn = as<int32_t>(ptr);
        sink->set_channel(chn);
        LOG_VERBOSE("AooSource: send to sink " << sink->ep
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
        const int32_t minpacketsize = kDataHeaderSize + 64;
        auto packetsize = as<int32_t>(ptr);
        if (packetsize < minpacketsize){
            LOG_WARNING("AooSource: packet size too small! setting to " << minpacketsize);
            packetsize_.store(minpacketsize);
        } else if (packetsize > AOO_MAX_PACKET_SIZE){
            LOG_WARNING("AooSource: packet size too large! setting to " << AOO_MAX_PACKET_SIZE);
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
        CHECKARG(AooSeconds);
        as<AooSeconds>(ptr) = ping_interval_.load();
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
    case kAooCtlSetBinaryDataMsg:
        CHECKARG(AooBool);
        binary_.store(as<AooBool>(ptr));
        timer_.reset(); // !
        break;
    case kAooCtlGetBinaryDataMsg:
        CHECKARG(AooBool);
        as<AooBool>(ptr) = binary_.load();
        break;
#if AOO_NET
    case kAooCtlSetClient:
        client_ = reinterpret_cast<AooClient *>(index);
        break;
#endif
    // unknown
    default:
        LOG_WARNING("AooSource: unsupported control " << ctl);
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

            make_new_stream(state_.load() == stream_state::run);
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
        LOG_WARNING("AooSource: not an AoO message!");
        return kAooErrorBadArgument;
    }
    if (type != kAooTypeSource){
        LOG_WARNING("AooSource: not a source message!");
        return kAooErrorBadArgument;
    }
    if (src != id()){
        LOG_WARNING("AooSource: wrong source ID!");
        return kAooErrorBadArgument;
    }

    ip_address addr((const sockaddr *)address, addrlen);

    if (aoo::binmsg_check(data, size)){
        // binary message
        auto cmd = aoo::binmsg_cmd(data, size);
        auto id = aoo::binmsg_from(data, size);
        switch (cmd){
        case kAooBinMsgCmdData:
            handle_data_request(data + onset, size - onset, id, addr);
            return kAooOk;
        default:
            LOG_WARNING("AooSink: unsupported binary message");
            return kAooErrorBadArgument;
        }
    } else {
        // OSC message
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
            } else if (!strcmp(pattern, kAooMsgPong)){
                handle_pong(msg, addr);
            } else {
                LOG_WARNING("AooSource: unknown message " << pattern);
                return kAooErrorUnknown;
            }
            return kAooOk;
        } catch (const osc::Exception& e){
            LOG_ERROR("AooSource: exception in handle_message: " << e.what());
            return kAooErrorUnknown;
        }
    }
}

// find out if sendto() blocks
#define DEBUG_SEND_TIME 0

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

#if DEBUG_SEND_TIME
    auto t1 = aoo::time_tag::now();
#endif
    send_data(reply);
#if DEBUG_SEND_TIME
    auto t2 = aoo::time_tag::now();
    auto delta1 = (t2 - t1).to_seconds() * 1000.0;
    if (delta1 > 1.0) {
        LOG_DEBUG("AooSource: send_data took " << delta1 << " ms");
    }

    auto t3 = aoo::time_tag::now();
#endif
    resend_data(reply);
#if DEBUG_SEND_TIME
    auto t4 = aoo::time_tag::now();
    auto delta2 = (t4 - t3).to_seconds() * 1000.0;
    if (delta2 > 1.0) {
        LOG_DEBUG("AooSource: resend_data took " << delta2 << " ms");
    }
#endif

    send_ping(reply);

    if (!sinks_.try_free()){
        // LOG_DEBUG("AooSource: try_free() would block");
    }

    return kAooOk;
}

/** \copydoc AooSource::addMessage() */
AOO_API AooError AOO_CALL AooSource_addStreamMessage(
        AooSource *src, const AooStreamMessage *message) {
    if (message) {
        return src->addStreamMessage(*message);
    } else {
        return kAooErrorBadArgument;
    }
}

AooError AOO_CALL aoo::Source::addStreamMessage(const AooStreamMessage& message) {
#if 1
    // avoid piling up stream messages
    if (state_.load(std::memory_order_relaxed) == stream_state::idle) {
        return kAooErrorIdle;
    }
#endif
#if IDLE_IF_NO_SINKS
    if (sinks_.empty()) {
        return kAooErrorIdle;
    }
#endif
    auto time = process_samples_ + message.sampleOffset;
    message_queue_.push(time, message.type, (char *)message.data, message.size);
#if AOO_DEBUG_STREAM_MESSAGE
    LOG_ALL("AooSource: add stream message "
            << "(type: " << aoo_dataTypeToString(message.type)
            << ", size: " << message.size
            << ", offset: " << message.sampleOffset
            << ", time: " << time<< ")");
#endif
    return kAooErrorNone;
}

AOO_API AooError AOO_CALL AooSource_process(
        AooSource *src, AooSample **data, AooInt32 n, AooNtpTime t) {
    return src->process(data, n, t);
}

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
            LOG_DEBUG("AooSource: process would block");
            // no need to call xrun()!
            return kAooErrorIdle; // ?
        }

        make_new_stream(true);

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
        LOG_DEBUG("AooSource: setup time DLL filter");
        auto bw = dll_bandwidth_.load();
        dll_.setup(samplerate_, blocksize_, bw, 0);
        realsr_.store(samplerate_);
        // it is safe to set 'last_ping_time' after updating
        // the timer, because in the worst case the ping
        // is simply sent the next time.
        last_ping_time_.store(-1e007); // force first ping
    } else if (timerstate == timer::state::error){
        // calculate xrun blocks
        double nblocks = error * (double)samplerate_ / (double)blocksize_;
    #if IDLE_IF_NO_SINKS
        // only when we have sinks, to avoid accumulating empty blocks
        if (!sinks_.empty())
    #endif
        {
            add_xrun(nblocks);
        }
        LOG_DEBUG("AooSource: xrun: " << nblocks << " blocks");

        auto count = std::ceil(nblocks); // ?
        send_event(make_event<xrun_event>(count), kAooThreadLevelAudio);

        timer_.reset();
    } else if (dynamic_resampling){
        // update time DLL, but only if n matches blocksize!
        auto elapsed = timer_.get_elapsed();
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

#if IDLE_IF_NO_SINKS
    if (sinks_.empty()){
        // nothing to do. users still have to check for pending events,
        // but there is no reason to call send()
        if (resampler_.size() > 0 || audioqueue_.read_available() > 0) {
            // clear this so no garbage gets in when we have sinks again
            resampler_.reset();
            audioqueue_.reset();
            if (encoder_) {
                AooEncoder_control(encoder_.get(), kAooCodecCtlReset, nullptr, 0);
            }
            LOG_DEBUG("clear state on no sinks");
        }
        return kAooErrorIdle;
    }
#endif

    // the mutex should be available most of the time.
    // it is only locked exclusively when setting certain options,
    // e.g. changing the buffer size.
    shared_lock lock(update_mutex_, sync::try_to_lock); // reader lock!
    if (!lock.owns_lock()){
        LOG_DEBUG("AooSource: process would block");
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
    LOG_DEBUG("AooSource: audioqueue: " << audioqueue_.read_available() / resampler_.ratio()
              << ", resampler: " << resampler_size / resampler_.ratio()
              << ", capacity: " << audioqueue_.capacity() / resampler_.ratio());
#endif
    process_samples_ += nsamples;
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
            LOG_WARNING("AooSource: send buffer overflow");
            add_xrun(1);
            // don't return kAooErrorIdle, otherwise the send thread
            // wouldn't drain the buffer.
            // TODO: send event?
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
            LOG_WARNING("AooSource: send buffer overflow");
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
    event_ptr e;
    while (eventqueue_.try_pop(e)) {
        eventhandler_(eventcontext_, &e->cast(), kAooThreadLevelUnknown);
    }
    return kAooOk;
}

AOO_API AooError AOO_CALL AooSource_startStream(
        AooSource *source, const AooData *metadata)
{
    return source->startStream(metadata);
}

AooError AOO_CALL aoo::Source::startStream(const AooData *md) {
    // check metadata
    if (md) {
        // check data size
        if (md->size == 0){
            LOG_ERROR("AooSource: stream metadata cannot be empty!");
            return kAooErrorBadArgument;
        }

        LOG_DEBUG("AooSource: start stream with " << md->type << " metadata");
    } else {
        LOG_DEBUG("AooSource: start stream");
    }

    // copy metadata
    AooData *metadata = nullptr;
    if (md && md->size > 0) {
        auto size = flat_metadata_size(*md);
        metadata = (AooData *)rt_allocate(size);
        flat_metadata_copy(*md, *metadata);
    }
    // exchange metadata
    {
        scoped_spinlock lock(metadata_lock_);
        metadata_.reset(metadata);
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
        LOG_WARNING("AooSource: sink already added!");
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

AOO_API AooError AOO_CALL AooSource_handleInvite(
        AooSource *source, const AooEndpoint *sink, AooId token, AooBool accept)
{
    if (sink) {
        return source->handleInvite(*sink, token, accept);
    } else {
        return kAooErrorBadArgument;
    }
}

AooError AOO_CALL aoo::Source::handleInvite(const AooEndpoint& ep, AooId token, AooBool accept) {
    ip_address addr((const sockaddr *)ep.address, ep.addrlen);
    sink_lock lock(sinks_);
    auto sink = find_sink(addr, ep.id);
    if (sink){
        sink->handle_invite(*this, token, accept);
        return kAooOk;
    } else {
        LOG_ERROR("AooSource: couldn't find sink");
        return kAooErrorBadArgument;
    }
}

AOO_API AooError AOO_CALL AooSource_handleUninvite(
        AooSource *source, const AooEndpoint *sink, AooId token, AooBool accept)
{
    if (sink) {
        return source->handleUninvite(*sink, token, accept);
    } else {
        return kAooErrorBadArgument;
    }
}

AooError AOO_CALL aoo::Source::handleUninvite(const AooEndpoint& ep, AooId token, AooBool accept) {
    ip_address addr((const sockaddr *)ep.address, ep.addrlen);
    sink_lock lock(sinks_);
    auto sink = find_sink(addr, ep.id);
    if (sink){
        sink->handle_uninvite(*this, token, accept);
        return kAooOk;
    } else {
        LOG_ERROR("AooSource: couldn't find sink");
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
        LOG_ERROR("AooSource: missing sink argument");
        return nullptr;
    }
    ip_address addr((const sockaddr *)ep->address, ep->addrlen);
    auto sink = find_sink(addr, ep->id);
    if (!sink){
        LOG_ERROR("AooSource: couldn't find sink");
    }
    return sink;
}

// always called with sink mutex locked
sink_desc * Source::do_add_sink(const ip_address& addr, AooId id, AooId stream_id)
{
#if IDLE_IF_NO_SINKS
    // reset everything if sinks have been empty. For efficiency reasons we do not
    // process if there are no sinks; instead we return kAooErrorIdle, so the user
    // might not notify the send thread. This means that the resampler, audio buffer
    // and encoder might contain garbage.
    if (sinks_.empty()) {
        scoped_lock lock(update_mutex_); // writer lock!
        resampler_.reset();
        audioqueue_.reset();
        if (encoder_) {
            AooEncoder_control(encoder_.get(), kAooCodecCtlReset, nullptr, 0);
        }
        LOG_DEBUG("AooSource: reset after no sinks");
    }
#endif

#if AOO_NET
    ip_address relay;
    // check if the peer needs to be relayed
    if (client_){
        AooBool b;
        AooEndpoint ep { addr.address(), (AooAddrSize)addr.length(), id };
        if (client_->control(kAooCtlNeedRelay, reinterpret_cast<intptr_t>(&ep),
                             &b, sizeof(b)) == kAooOk) {
            if (b == kAooTrue){
                LOG_DEBUG("AooSource: sink " << addr << "|" << ep.id
                          << " needs to be relayed");
                // get relay address
                client_->control(kAooCtlGetRelayAddress,
                                 reinterpret_cast<intptr_t>(&ep),
                                 &relay, sizeof(relay));

            }
        }
    }
    auto it = sinks_.emplace_front(addr, id, stream_id, relay);
#else
    auto it = sinks_.emplace_front(addr, id, stream_id);
#endif
    // send /start if needed!
    if (is_running() && it->is_active()){
        it->notify_start();
        notify_start();
    }

    return &(*it);
}

// always called with sink mutex locked
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
    LOG_WARNING("AooSource: sink not found!");
    return false;
}

AooError Source::set_format(AooFormat &f){
    auto codec = aoo::find_codec(f.codec);
    if (!codec){
        LOG_ERROR("AooSource: codec '" << f.codec << "' not supported!");
        return kAooErrorUnknown;
    }

    std::unique_ptr<AooFormat, format_deleter> new_format;
    std::unique_ptr<AooCodec, encoder_deleter> new_encoder;

    // create a new encoder - will validate format!
    AooError err;
    auto enc = codec->encoderNew(&f, &err);
    if (!enc){
        LOG_ERROR("AooSource: couldn't create encoder!");
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
    // could answer a /start request by an existing stream with
    // the wrong format, before process() starts the new stream.
    //
    // NB: there's a slight race condition because 'xrunblocks_'
    // might be incremented right afterwards, but I'm not
    // sure if this could cause any real problems...
    make_new_stream(state_.load() == stream_state::run);

    return kAooOk;
}

AooError Source::get_format(AooFormat &fmt, size_t size){
    shared_lock lock(update_mutex_); // read lock!
    if (format_){
        if (size >= format_->size){
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
    // LATER add an option for fixed block sizes
    return true;
#else
    return blocksize_ != format_->blockSize || samplerate_ != format_->sampleRate;
#endif
}

void Source::push_request(const sink_request &r){
    requests_.push(r);
}

void Source::notify_start(){
    LOG_DEBUG("AooSource: notify_start()");
    needstart_.exchange(true, std::memory_order_release);
}

void Source::send_event(event_ptr e, AooThreadLevel level){
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

// must be real-time safe because it might be called in process()!
// always called with update lock!
void Source::make_new_stream(bool notify){
    // implicitly reset time DLL to be on the safe side
    timer_.reset();

    sequence_ = 0;
    xrunblocks_.store(0.0); // !

    // "accept" stream metadata, see send_start()
    {
        scoped_spinlock lock(metadata_lock_);
        if (metadata_) {
            metadata_accepted_ = true;
        }
    }

    // remove audio from previous stream
    resampler_.reset();

    audioqueue_.reset();

    history_.clear(); // !

    message_queue_.clear();
    message_prio_queue_.clear();
    process_samples_ = stream_samples_ = 0;

    // reset encoder to avoid garbage from previous stream
    if (encoder_) {
        AooEncoder_control(encoder_.get(), kAooCodecCtlReset, nullptr, 0);
    }

    sink_lock lock(sinks_);
    for (auto& s : sinks_){
        s.start();
    }

    if (notify) {
        notify_start();
    }
}

void Source::add_xrun(double nblocks){
    // add with CAS loop
    auto current = xrunblocks_.load(std::memory_order_relaxed);
    while (!xrunblocks_.compare_exchange_weak(current, current + nblocks))
        ;
    // NB: do not advance process_samples_! See send_data().
}

void Source::update_audioqueue(){
    if (encoder_ && samplerate_ > 0){
        // recalculate buffersize from seconds to samples
        int32_t bufsize = buffersize_.load() * format_->sampleRate;
        auto d = std::div(bufsize, format_->blockSize);
        int32_t nbuffers = d.quot + (d.rem != 0); // round up
        // minimum buffer size depends on resampling and reblocking!
        auto resample = (double)format_->sampleRate / (double)samplerate_;
        auto reblock = (double)format_->blockSize / (double)blocksize_;
        int32_t minblocks = std::ceil(resample * reblock);
        nbuffers = std::max<int32_t>(nbuffers, minblocks);
        LOG_DEBUG("AooSource: buffersize (ms): " << (buffersize_.load() * 1000.0)
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
        auto d = std::div(bufsize, format_->blockSize);
        int32_t nbuffers = d.quot + (d.rem != 0); // round up
        history_.resize(nbuffers);
        LOG_DEBUG("AooSource: history buffersize (ms): "
                  << (resend_buffersize_.load() * 1000.0)
                  << ", samples: " << bufsize << ", nbuffers: " << nbuffers);

    }
}

// /aoo/sink/<id>/start <src> <version> <stream_id> <flags>
// <lastformat> <nchannels> <samplerate> <blocksize> <codec> <extension>
// [<metadata_type> <metadata_content>]
void send_start_msg(const endpoint& ep, int32_t id, int32_t stream, int32_t lastformat,
                    const AooFormat& f, const AooByte *extension, AooInt32 size,
                    const AooData* metadata, const sendfn& fn) {
    LOG_DEBUG("AooSource: send " kAooMsgStart " to " << ep
              << " (stream = " << stream << ")");

    char buf[AOO_MAX_PACKET_SIZE];
    osc::OutboundPacketStream msg(buf, sizeof(buf));

    const int32_t max_addr_size = kAooMsgDomainLen
            + kAooMsgSinkLen + 16 + kAooMsgStartLen;
    char address[max_addr_size];
    snprintf(address, sizeof(address), "%s/%d%s",
             kAooMsgDomain kAooMsgSink, ep.id, kAooMsgStart);

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

    ep.send(msg, fn);
}

// /aoo/sink/<id>/stop <src> <stream_id>
void send_stop_msg(const endpoint& ep, int32_t id, int32_t stream, const sendfn& fn) {
    LOG_DEBUG("AooSource: send " kAooMsgStop " to " << ep
              << " (stream = " << stream << ")");

    char buf[AOO_MAX_PACKET_SIZE];
    osc::OutboundPacketStream msg(buf, sizeof(buf));

    const int32_t max_addr_size = kAooMsgDomainLen
            + kAooMsgSinkLen + 16 + kAooMsgStopLen;
    char address[max_addr_size];
    snprintf(address, sizeof(address), "%s/%d%s",
             kAooMsgDomain kAooMsgSink, ep.id, kAooMsgStop);

    msg << osc::BeginMessage(address) << id << stream << osc::EndMessage;

    ep.send(msg, fn);
}

// /aoo/sink/<id>/decline <src> <token>
void send_decline_msg(const endpoint& ep, int32_t id, int32_t token, const sendfn& fn) {
    LOG_DEBUG("AooSource: send " kAooMsgDecline " to " << ep
              << " (stream = " << token << ")");

    char buf[AOO_MAX_PACKET_SIZE];
    osc::OutboundPacketStream msg(buf, sizeof(buf));

    const int32_t max_addr_size = kAooMsgDomainLen
            + kAooMsgSinkLen + 16 + kAooMsgDeclineLen;
    char address[max_addr_size];
    snprintf(address, sizeof(address), "%s/%d%s",
             kAooMsgDomain kAooMsgSink, ep.id, kAooMsgDecline);

    msg << osc::BeginMessage(address) << id << token << osc::EndMessage;

    ep.send(msg, fn);
}

// /aoo/sink/<id>/pong <src> <tt1> <tt2>
void send_pong_msg(const endpoint& ep, int32_t id, aoo::time_tag tt1, const sendfn& fn) {
    LOG_DEBUG("AooSource: send " kAooMsgPong " to " << ep);

    auto tt2 = aoo::time_tag::now(); // use real system time

    char buf[AOO_MAX_PACKET_SIZE];
    osc::OutboundPacketStream msg(buf, sizeof(buf));

    const int32_t max_addr_size = kAooMsgDomainLen
            + kAooMsgSinkLen + 16 + kAooMsgDeclineLen;
    char address[max_addr_size];
    snprintf(address, sizeof(address), "%s/%d%s",
             kAooMsgDomain kAooMsgSink, ep.id, kAooMsgPong);

    msg << osc::BeginMessage(address) << id
        << osc::TimeTag(tt1) << osc::TimeTag(tt2)
        << osc::EndMessage;

    ep.send(msg, fn);
}

void Source::dispatch_requests(const sendfn& fn){
    sink_request r;
    while (requests_.try_pop(r)){
        switch (r.type) {
        case request_type::stop:
            send_stop_msg(r.ep, id(), r.stop.stream, fn);
            break;
        case request_type::decline:
            send_decline_msg(r.ep, id(), r.decline.token, fn);
            break;
        case request_type::pong:
            send_pong_msg(r.ep, id(), r.pong.time, fn);
            break;
        default:
            LOG_ERROR("AooSource: unknown request type");
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

    if (encoder_->cls->serialize(
                &f.header, extension, &size) != kAooOk) {
        return;
    }

    // cache stream metadata
    AooData *md = nullptr;
    {
        scoped_spinlock lock(metadata_lock_);
        // only send metadata if "accepted" in make_new_stream().
        if (metadata_accepted_) {
            assert(metadata_ != nullptr);
            assert(metadata_->size > 0);
            auto mdsize = flat_metadata_size(*metadata_);
            md = (AooData *)alloca(mdsize);
            flat_metadata_copy(*metadata_, *md);
        }
    }

    // cache sinks that need to send a /start message
#if 0
    // we only free sinks in this thread, so we don't have to lock
    sink_lock lock(sinks_);
#endif
    if (cached_sinks_.empty()) {
        // if there were no (active) sinks, we need to reset the encoder to
        // prevent nasty artifacts. (For efficiency reasons, we skip the encoding
        // process in send_data() if there are no active sinks.)
        AooEncoder_control(encoder_.get(), kAooCodecCtlReset, nullptr, 0);
        LOG_DEBUG("AooSource: sinks previously empty/inactive - reset encoder");
    }
    cached_sinks_.clear();
    for (auto& s : sinks_){
        if (s.need_start()){
            cached_sinks_.emplace_back(s);
        }
    }

    // send messages without lock!
    updatelock.unlock();

    for (auto& s : cached_sinks_){
        send_start_msg(s.ep, id(), s.stream_id, format_id, f.header, extension, size, md, fn);
    }
}

// binary data message:
// stream_id (int32), seq (int32), channel (uint8), flags (uint8), data_size (uint16),
// [total (int32), nframes (int16), frame (int16)], [msgsize (int32)], [sr (float64)], data...

AooSize write_bin_data(AooByte *buffer, AooSize size,
                       AooId stream_id, const data_packet& d)
{
    assert(size >= 32);

    int16_t flags = 0;
    if (d.samplerate != 0){
        flags |= kAooBinMsgDataSampleRate;
    }
    if (d.nframes > 1){
        flags |= kAooBinMsgDataFrames;
    }
    if (d.msgsize > 0){
        flags |= kAooBinMsgDataStreamMessage;
    }
    // write arguments
    auto it = buffer;
    aoo::write_bytes<int32_t>(stream_id, it);
    aoo::write_bytes<int32_t>(d.sequence, it);
    aoo::write_bytes<uint8_t>(d.channel, it);
    aoo::write_bytes<uint8_t>(flags, it);
    aoo::write_bytes<uint16_t>(d.size, it);
    if (flags & kAooBinMsgDataFrames){
        aoo::write_bytes<uint32_t>(d.totalsize, it);
        aoo::write_bytes<uint16_t>(d.nframes, it);
        aoo::write_bytes<uint16_t>(d.frame, it);
    }
    if (flags & kAooBinMsgDataStreamMessage){
        aoo::write_bytes<uint32_t>(d.msgsize, it);
    }
    if (flags & kAooBinMsgDataSampleRate){
         aoo::write_bytes<double>(d.samplerate, it);
    }
    // write audio data
    memcpy(it, d.data, d.size);
    it += d.size;

    return (it - buffer);
}

// /aoo/sink/<id>/data <src> <stream_id> <seq> <sr> <channel_onset>
// <totalsize> <msgsize> <nframes> <frame> <data>

void send_packet_osc(const endpoint& ep, AooId id, int32_t stream_id,
                     const aoo::data_packet& d, const sendfn& fn) {
    char buf[AOO_MAX_PACKET_SIZE];
    osc::OutboundPacketStream msg(buf, sizeof(buf));

    char address[kDataMaxAddrSize];
    snprintf(address, sizeof(address), "%s/%d%s",
             kAooMsgDomain kAooMsgSink, ep.id, kAooMsgData);

    msg << osc::BeginMessage(address) << id << stream_id << d.sequence << d.samplerate
        << d.channel << d.totalsize << d.msgsize << d.nframes << d.frame
        << osc::Blob(d.data, d.size) << osc::EndMessage;

#if AOO_DEBUG_DATA
    LOG_DEBUG("AooSource: send block: seq = " << d.sequence << ", sr = " << d.samplerate
              << ", chn = " << d.channel << ", totalsize = " << d.totalsize
              << ", msgsize = " << d.msgsize << ", nframes = " << d.nframes
              << ", frame = " << d.frame << ", size " << d.size);
#endif
    ep.send(msg, fn);
}

// binary data message:
// stream_id (int32), seq (int32), channel (uint8), flags (uint8), size (uint16)
// [total (int32), nframes (int16), frame (int16)], [msgsize (int32)], [sr (float64)], data...

void send_packet_bin(const endpoint& ep, AooId id, AooId stream_id,
                     const aoo::data_packet& d, const sendfn& fn) {
    AooByte buf[AOO_MAX_PACKET_SIZE];

    auto onset = aoo::binmsg_write_header(buf, sizeof(buf), kAooTypeSink,
                                          kAooBinMsgCmdData, ep.id, id);
    auto argsize = write_bin_data(buf + onset, sizeof(buf) - onset, stream_id, d);
    auto size = onset + argsize;

#if AOO_DEBUG_DATA
    LOG_DEBUG("AooSource: send block: seq = " << d.sequence << ", sr = "
              << d.samplerate << ", chn = " << s.channel << ", msgsize = "
              << d.msgsize << ", totalsize = " << d.totalsize << ", nframes = "
              << d.nframes << ", frame = " << d.frame << ", size " << d.size);
#endif

    ep.send(buf, size, fn);
}

void send_packet(const aoo::vector<cached_sink>& sinks, const AooId id,
                 data_packet& d, const sendfn &fn, bool binary) {
    if (binary){
    #if AOO_DEBUG_DATA
        LOG_DEBUG("AooSource: send block: seq = " << d.sequence << ", sr = "
                  << d.samplerate << ", chn = " << s.channel << ", msgsize = "
                  << d.msgsize << ", totalsize = " << d.totalsize << ", nframes = "
                  << d.nframes << ", frame = " << d.frame << ", size " << d.size);
    #endif
        AooByte buf[AOO_MAX_PACKET_SIZE];

        // start at max. header size
        auto args = buf + kAooBinMsgLargeHeaderSize;
        auto argsize = write_bin_data(args, sizeof(buf) - kAooBinMsgLargeHeaderSize,
                                      kAooIdInvalid, d);
        auto end = args + argsize;

        for (auto& s : sinks) {
            // write header
            bool large =  s.ep.id > 255 || id > 255;
            auto start = large ? buf : (args - kAooBinMsgHeaderSize);
            aoo::binmsg_write_header(start, args - start, kAooTypeSink,
                                     kAooBinMsgCmdData, s.ep.id, id);
            // replace stream ID and channel
            aoo::to_bytes(s.stream_id, args);
            args[8] = s.channel;

            s.ep.send(start, end - start, fn);
        }
    } else {
        for (auto& s : sinks){
            // set channel!
            d.channel = s.channel;
            send_packet_osc(s.ep, id, s.stream_id, d, fn);
        }
    }
}

#define XRUN_THRESHOLD 0.1
#define SKIP_OUTDATED_MESSAGES 1
#define XRUN_BLOCKS_CEIL 1

void Source::handle_xruns(const sendfn &fn) {
    // *first* check for dropped blocks
    if (xrunblocks_.load(std::memory_order_relaxed) > XRUN_THRESHOLD){
        shared_lock updatelock(update_mutex_); // reader lock
        // send empty stream blocks for xrun blocks to fill up the missing time.
        // NB: do not advance stream_samples_! See also add_xrun().
        // (We *could* advance process and stream time proportionally,
        // but at the moment it wouldn't help with anything. On the contrary:
        // the stream time might accidentally run ahead of the process time,
        // causing issues with negative stream message sample offsets...)
        auto xrunblocks = xrunblocks_.exchange(0.0);
        auto convert = resampler_.ratio() * (double)blocksize_ / (double)format_->blockSize;
        // convert xrunblocks to stream blocks.
        // If the format uses a larger blocksize, the stream might run a little bit ahead
        // of time. To mitigate this problem, the block difference is subtracted from
        // xrunblocks_ so that processing may catch up with subsequent calls to add_xrun().
        // NB: if XRUN_BLOCK_CEIL is 0, the difference may be negative, in which case
        // it is effectively added back to xrunblocks_.
    #if XRUN_BLOCKS_CEIL
        int stream_blocks = std::ceil(xrunblocks * convert);
    #else
        int stream_blocks = xrunblocks * convert + 0.5;
    #endif
        auto process_blocks = stream_blocks / convert;
        auto diff = process_blocks - xrunblocks;
    #if XRUN_BLOCKS_CEIL
        assert(diff >= 0);
    #endif
        // subtract diff with a CAS loop
        auto current = xrunblocks_.load(std::memory_order_relaxed);
        while (!xrunblocks_.compare_exchange_weak(current, current - diff))
            ;

        // cache sinks
    #if 0
        // we only free sinks in this thread, so we don't have to lock
        sink_lock lock(sinks_);
    #endif
        cached_sinks_.clear();
        for (auto& s : sinks_){
            if (s.is_active()){
                cached_sinks_.emplace_back(s);
            }
        }
        // if we don't have any (active) sinks, we do not actually need to send anything!
        if (cached_sinks_.empty()) {
            return;
        }

        // send empty blocks
        if (stream_blocks > 0) {
            LOG_DEBUG("AooSource: send " << stream_blocks << " empty blocks for "
                      "xrun (" << xrunblocks << " blocks)");
        }
        while (stream_blocks--){
            // check the encoder and make snapshost of stream_id
            // in every iteration because we release the lock
            if (!encoder_){
                return;
            }
            // send empty block
            // NOTE: we're the only thread reading 'sequence_', so we can increment
            // it even while holding a reader lock!
            data_packet d;
            d.sequence = sequence_++;
            d.samplerate = format_->sampleRate; // use nominal samplerate
            d.channel = 0;
            d.totalsize = 0;
            d.msgsize = 0;
            d.nframes = 0;
            d.frame = 0;
            d.data = nullptr;
            d.size = 0;

            // wrap around to prevent signed integer overflow
            if (sequence_ == INT32_MAX) {
                sequence_ = 0;
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
}

void Source::send_data(const sendfn& fn){
    // *first* handle xruns
    handle_xruns(fn);

    // then send audio
    shared_lock updatelock(update_mutex_); // reader lock
    while (audioqueue_.read_available()) {
        // NB: recheck one every iteration because we temporarily release the lock!
        if (!encoder_) {
            return;
        }

        // reset and reserve space for message count
        sendbuffer_.resize(4);
        uint32_t msg_count = 0;
        double deadline = stream_samples_ + (double)format_->blockSize / resampler_.ratio();

        // handle stream messages.
        // Copy into priority queue to avoid draining the RT memory pool
        // when scheduling many messages in the future.
        // NB: we have to pop messages in sync with the audio queue!
        message_queue_.consume_all([&](auto& msg) {
            auto offset = (int64_t)msg.time - (int64_t)stream_samples_;
        #if SKIP_OUTDATED_MESSAGES
            if (offset < 0) {
                // skip outdated message; can happen with xrun blocks
                LOG_VERBOSE("AooSource: skip stream message (offset: " << offset << ")");
            } else
        #endif
            message_prio_queue_.emplace(msg.time, msg.type, msg.data, msg.size);
        #if AOO_DEBUG_STREAM_MESSAGE
            LOG_ALL("AooSource: schedule stream message "
                    << "(type: " << aoo_dataTypeToString(msg.type)
                    << ", size: " << msg.size
                    << ", time: " << msg.time << ")");
        #endif
        });
        // dispatch scheduled stream messages
        while (!message_prio_queue_.empty()) {
            auto& msg = message_prio_queue_.top();
            if (msg.time < (uint64_t)deadline) {
                // add header
                std::array<char, 8> buffer;
                auto offset = ((int64_t)msg.time - (int64_t)stream_samples_) * resampler_.ratio();
            #if SKIP_OUTDATED_MESSAGES
                assert(offset >= 0);
            #else
                offset = std::max(0.0, offset);
            #endif
                aoo::to_bytes<uint16_t>(offset, &buffer[0]);
                aoo::to_bytes<uint16_t>(msg.type, &buffer[2]);
                aoo::to_bytes<uint32_t>(msg.size, &buffer[4]);
                sendbuffer_.insert(sendbuffer_.end(), buffer.begin(), buffer.end());
                // add data
                sendbuffer_.insert(sendbuffer_.end(), msg.data, msg.data + msg.size);
                // add padding bytes (total size is rounded up to 4 bytes.)
                auto remainder = msg.size & 3;
                if (remainder > 0) {
                    sendbuffer_.resize(sendbuffer_.size() + 4 - remainder);
                }
            #if AOO_DEBUG_STREAM_MESSAGE
                LOG_ALL("AooSource: send stream message "
                        << "(type: " << aoo_dataTypeToString(msg.type)
                        << ", size: " << msg.size
                        << ", offset: " << offset << ")");
            #endif
                msg_count++;
                message_prio_queue_.pop();
            } else {
                break;
            }
        }
        if (msg_count > 0) {
            // finally write message count
            aoo::to_bytes<uint32_t>(msg_count, sendbuffer_.data());
        } else {
            // no stream message data
            sendbuffer_.clear();
        }

        stream_samples_ = deadline;

        // cache sinks
    #if 0
        // we only free sinks in this thread, so we don't have to lock
        sink_lock lock(sinks_);
    #endif
        cached_sinks_.clear();
        for (auto& s : sinks_){
            if (s.is_active()){
                cached_sinks_.emplace_back(s);
            }
        }
        // if we don't have any (active) sinks, we do not actually need
        // to encode and send the data!
        if (cached_sinks_.empty()) {
            audioqueue_.read_commit(); // !
            continue;
        }

        auto ptr = (block_data *)audioqueue_.read_data();

        data_packet d;
        d.samplerate = ptr->sr;
        d.msgsize = sendbuffer_.size();
        // message size must be aligned to 4 byte boundary!
        assert((d.msgsize & 3) == 0);

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

        int32_t audio_size = sizeof(double) * nsamples; // overallocate
        sendbuffer_.resize(d.msgsize + audio_size);

        auto err = AooEncoder_encode(encoder_.get(), ptr->data, nsamples,
            sendbuffer_.data() + d.msgsize, &audio_size);
        d.totalsize = d.msgsize + audio_size;

        audioqueue_.read_commit(); // always commit!

        if (err != kAooOk){
            LOG_WARNING("AooSource: couldn't encode audio data!");
            return;
        }

        // NOTE: we're the only thread reading 'sequence_', so we can increment
        // it even while holding a reader lock!
        d.sequence = sequence_++;
        // wrap around to prevent signed integer overflow
        if (sequence_ == INT32_MAX) {
            sequence_ = 0;
        }

        // calculate number of frames
        bool binary = binary_.load();
        auto packetsize = packetsize_.load();
        auto maxpacketsize = packetsize -
                (binary ? kBinDataHeaderSize : kDataHeaderSize);
        auto dv = std::div(d.totalsize, maxpacketsize);
        d.nframes = dv.quot + (dv.rem != 0);

        // save block (if we have a history buffer)
        if (history_.capacity() > 0){
            history_.push()->set(d.sequence, d.samplerate, sendbuffer_.data(),
                                 d.totalsize, d.msgsize, d.nframes, maxpacketsize);
        }

        // unlock before sending!
        updatelock.unlock();

        // from here on we don't hold any lock!

        // send a single frame to all sinks
        // /aoo/<sink>/data <src> <stream_id> <seq> <sr> <channel_onset>
        // <totalsize> <msgsize> <numframes> <frame> <data>
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
    }
}

void Source::resend_data(const sendfn &fn){
    shared_lock updatelock(update_mutex_); // reader lock for history buffer!
    if (!history_.capacity()){
        // NB: there should not be any requests if resending is disabled,
        // see handle_data_request().
        return;
    }

    // we only free sources in this thread, so we don't have to lock
#if 0
    // this is not a real lock, so we don't have worry about dead locks
    sink_lock lock(sinks_);
#endif
    // send block to sinks
    for (auto& s : sinks_){
        data_request r;
        while (s.get_data_request(r)){
        #if AOO_DEBUG_RESEND && 0
            LOG_DEBUG("AooSource: dispatch data request (" << r.sequence
                      << " " << r.offset << " " << r.bitset << ")");
        #endif
            auto block = history_.find(r.sequence);
            if (block){
                bool binary = binary_.load();

                auto stream_id = s.stream_id();

                aoo::data_packet d;
                d.sequence = block->sequence;
                d.msgsize = block->message_size;
                d.samplerate = block->samplerate;
                d.channel = s.channel();
                d.totalsize = block->size();
                d.nframes = block->num_frames();
                // We need to copy all (requested) frames before sending
                // because we temporarily release the update lock!
                // We use a buffer on the heap because blocks and even frames
                // can be quite large and we don't want them to sit on the stack.
                sendbuffer_.resize(d.totalsize);
                AooByte *buf = sendbuffer_.data();
                // Keep track of the frames we will eventually send.
                struct frame_data {
                    int32_t index;
                    int32_t size;
                    AooByte *data;
                };
                auto framevec = (frame_data *)alloca(d.nframes * sizeof(frame_data));
                int32_t numframes = 0;
                int32_t buf_offset = 0;

                auto copy_frame = [&](int32_t index) {
                    auto nbytes = block->get_frame(index, buf + buf_offset,
                                                   d.totalsize - buf_offset);
                    if (nbytes > 0) {
                        auto& frame = framevec[numframes];
                        frame.index = index;
                        frame.size = nbytes;
                        frame.data = buf + buf_offset;

                        buf_offset += nbytes;
                        numframes++;
                    } else {
                        LOG_ERROR("AooSource: empty frame!");
                    }
                };

                if (r.offset < 0) {
                    // a) whole block: copy all frames
                    for (int i = 0; i < d.nframes; ++i){
                        copy_frame(i);
                    }
                } else {
                    // b) only copy requested frames
                    uint16_t bitset = r.bitset;
                    for (int i = 0; bitset != 0; ++i, bitset >>= 1) {
                        if (bitset & 1) {
                            auto index = r.offset + i;
                            if (index < d.nframes) {
                                copy_frame(index);
                            } else {
                                LOG_ERROR("AooSource: frame number " << index << " out of range!");
                            }
                        }
                    }
                }
                // unlock before sending
                updatelock.unlock();

                // send frames to sink
                for (int i = 0; i < numframes; ++i) {
                    auto& frame = framevec[i];
                    d.frame = frame.index;
                    d.size = frame.size;
                    d.data = frame.data;
                #if AOO_DEBUG_RESEND
                    LOG_DEBUG("AooSource: resend " << d.sequence
                              << " (" << d.frame << " / " << d.nframes << ")");
                #endif
                    if (binary){
                        send_packet_bin(s.ep, id(), stream_id, d, fn);
                    } else {
                        send_packet_osc(s.ep, id(), stream_id, d, fn);
                    }
                }

                // lock again
                updatelock.lock();
            } else {
            #if AOO_DEBUG_RESEND
                LOG_DEBUG("AooSource: cannot find block " << r.sequence);
            #endif
            }
        }
    }
}

void Source::send_ping(const sendfn& fn){
    // if stream is stopped, the timer won't increment anyway
    auto elapsed = timer_.get_elapsed();
    auto pingtime = last_ping_time_.load();
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
                LOG_DEBUG("AooSource: send " kAooMsgPing " to " << sink.ep);

                char buf[AOO_MAX_PACKET_SIZE];
                osc::OutboundPacketStream msg(buf, sizeof(buf));

                const int32_t max_addr_size = kAooMsgDomainLen
                        + kAooMsgSinkLen + 16 + kAooMsgPingLen;
                char address[max_addr_size];
                snprintf(address, sizeof(address), "%s/%d%s",
                         kAooMsgDomain kAooMsgSink, sink.ep.id, kAooMsgPing);

                msg << osc::BeginMessage(address) << id() << osc::TimeTag(tt)
                    << osc::EndMessage;

                sink.ep.send(msg, fn);
            }
        }

        last_ping_time_.store(elapsed);
    }
}

// /start <id> <version>
void Source::handle_start_request(const osc::ReceivedMessage& msg,
                                  const ip_address& addr)
{
    LOG_DEBUG("AooSource: handle start request");

    auto it = msg.ArgumentsBegin();

    auto id = (it++)->AsInt32();
    auto version = (it++)->AsInt32();

    // LATER handle this in the sink_desc (e.g. not sending data)
    if (!check_version(version)){
        LOG_ERROR("AooSource: sink version not supported");
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
            LOG_VERBOSE("AooSource: ignoring '" << kAooMsgStart << "' message: sink not active");
        }
    } else {
        LOG_VERBOSE("AooSource: ignoring '" << kAooMsgStart << "' message: sink not found");
    }
}

// /aoo/src/<id>/data <id> <stream_id> <seq1> <frame1> <seq2> <frame2> etc.

void Source::handle_data_request(const osc::ReceivedMessage& msg,
                                 const ip_address& addr)
{
    auto it = msg.ArgumentsBegin();
    auto id = (it++)->AsInt32();
    auto stream_id = (it++)->AsInt32(); // we can ignore the stream_id

    if (resend_buffersize_.load() <= 0) {
    #if AOO_DEBUG_RESEND
        LOG_DEBUG("AooSource: ignore data request");
    #endif
        return;
    }

#if AOO_DEBUG_RESEND
    LOG_DEBUG("AooSource: handle data request");
#endif

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
                data_request r;
                r.sequence = (it++)->AsInt32();
                r.offset = (it++)->AsInt32(); // -1: whole block
                r.bitset = r.offset >= 0; // only first bit
                sink->push_data_request(r);
            }
        } else {
            LOG_VERBOSE("AooSource: ignoring '" << kAooMsgData << "' message: sink not active");
        }
    } else {
        LOG_VERBOSE("AooSource: ignoring '" << kAooMsgData << "' message: sink not found");
    }
}

// (header), stream_id (int32), count (int32),
// seq1 (int32), offset1 (int16), bitset1 (uint16), ... // offset < 0 -> all

void Source::handle_data_request(const AooByte *msg, int32_t n,
                                 AooId id, const ip_address& addr)
{
    // check size (stream_id, count)
    if (n < 8){
        LOG_ERROR("AooSource: binary data message too small!");
        return;
    }

    if (resend_buffersize_.load() <= 0) {
    #if AOO_DEBUG_RESEND
        LOG_DEBUG("AooSource: ignore data request");
    #endif
        return;
    }

    auto it = msg;
    auto end = it + n;

    auto stream_id = aoo::read_bytes<int32_t>(it);

#if AOO_DEBUG_RESEND
    LOG_DEBUG("AooSource: handle data request");
#endif

    sink_lock lock(sinks_);
    auto sink = find_sink(addr, id);
    if (sink){
        if (sink->stream_id() != stream_id){
            LOG_VERBOSE("AooSource: ignore binary data message: stream ID mismatch (outdated?)");
            return;
        }
        if (sink->is_active()){
            // get pairs of sequence + frame
            int count = aoo::read_bytes<int32_t>(it);
            if ((end - it) < (count * sizeof(int32_t) * 2)){
                LOG_ERROR("AooSource: bad 'count' argument for binary data message!");
                return;
            }
            while (count--){
                data_request r;
                r.sequence = aoo::read_bytes<int32_t>(it);
                r.offset = aoo::read_bytes<int16_t>(it);
                r.bitset = aoo::read_bytes<uint16_t>(it);
                sink->push_data_request(r);
            }
        } else {
            LOG_VERBOSE("AooSource: ignore binary data message: sink not active");
        }
    } else {
        LOG_VERBOSE("AooSource: ignore binary data message: sink not found");
    }
}

// /aoo/src/<id>/invite <id> <stream_id> [<metadata_type> <metadata_content>]

void Source::handle_invite(const osc::ReceivedMessage& msg,
                           const ip_address& addr)
{
    auto it = msg.ArgumentsBegin();

    auto id = (it++)->AsInt32();

    auto token = (it++)->AsInt32();

    AooInt32 type = kAooDataUnspecified;
    const void *ptr = nullptr;
    osc::osc_bundle_element_size_t size = 0;

    if (msg.ArgumentCount() > 2){
        type = (it++)->AsInt32();
        (it++)->AsBlob(ptr, size);
    }

    LOG_DEBUG("AooSource: handle invitation by " << addr << "|" << id);

    event_ptr e1, e2;

    // sinks can be added/removed from different threads
    sync::unique_lock<sync::mutex> lock1(sink_mutex_);
    sink_lock lock2(sinks_);
    auto sink = find_sink(addr, id);
    if (!sink){
        // the sink is initially deactivated.
        sink = do_add_sink(addr, id, kAooIdInvalid);
        // push "add" event
        e1 = make_event<sink_event>(kAooEventSinkAdd, addr, id);
    }
    // make sure that the event is only sent once per invitation.
    if (sink->need_invite(token)) {
        // push "invite" event
        AooData md;
        md.type = type;
        md.data = (AooByte *)ptr;
        md.size = size;

        e2 = make_event<invite_event>(addr, id, token,
                                      (type != kAooDataUnspecified) ? &md : nullptr);
    }

    lock1.unlock(); // unlock before sending events

    if (e1) {
        send_event(std::move(e1), kAooThreadLevelNetwork);
    }
    if (e2) {
        send_event(std::move(e2), kAooThreadLevelNetwork);
    }
}

// /aoo/src/<id>/uninvite <id> <stream_id>

void Source::handle_uninvite(const osc::ReceivedMessage& msg,
                             const ip_address& addr)
{
    auto it = msg.ArgumentsBegin();

    auto id = (it++)->AsInt32();

    auto token = (it++)->AsInt32();

    LOG_DEBUG("AooSource: handle uninvitation by " << addr << "|" << id);

    // check if sink exists (not strictly necessary, but might help catch errors)
    sink_lock lock(sinks_);
    auto sink = find_sink(addr, id);
    if (sink) {
        if (sink->stream_id() == token){
            if (sink->is_active()){
                // push "uninvite" event
                if (sink->need_uninvite(token)) {
                    auto e = make_event<uninvite_event>(addr, id, token);
                    send_event(std::move(e), kAooThreadLevelNetwork);
                }
                return; // don't send /stop message!
            } else {
                // if the sink is inactive, it probably means that we have
                // accepted the uninvitation, but the /stop message got lost.
                LOG_DEBUG("AooSource: ignoring '" << kAooMsgUninvite << "' message: "
                          << " sink not active (/stop message got lost?)");
            }
        } else {
            LOG_VERBOSE("AooSource: ignoring '" << kAooMsgUninvite
                        << "' message: stream token mismatch (outdated?)");
        }
    } else {
        LOG_VERBOSE("ignoring '" << kAooMsgUninvite << "' message: sink not found");
        // Don't return because we still want to send a /stop message, see below.
    }
    // Tell the remote side that we have stopped. Don't use the sink because it can be NULL!
#if AOO_NET
    auto relay = sink ? sink->ep.relay : false; // ?
    sink_request r(request_type::stop, endpoint(addr, id, relay));
#else
    sink_request r(request_type::stop, endpoint(addr, id));
#endif
    r.stop.stream = token; // use remote stream id!
    push_request(r);
    LOG_DEBUG("AooSource: resend " << kAooMsgStop << " message");
}

// /aoo/src/<id>/ping <id> <tt1>

void Source::handle_ping(const osc::ReceivedMessage& msg,
                         const ip_address& addr)
{
    auto it = msg.ArgumentsBegin();
    AooId id = (it++)->AsInt32();
    time_tag tt1 = (it++)->AsTimeTag();

    LOG_DEBUG("AooSource: handle ping");

    // check if sink exists (not strictly necessary, but might help catch errors)
    sink_lock lock(sinks_);
    auto sink = find_sink(addr, id);
    if (sink) {
        if (sink->is_active()){
            // push pong request
            sink_request r(request_type::pong, sink->ep);
            r.pong.time = tt1;
            push_request(r);
        } else {
            LOG_VERBOSE("AooSource: ignoring '" << kAooMsgPing << "' message: sink not active");
        }
    } else {
        LOG_VERBOSE("AooSource: ignoring '" << kAooMsgPing << "' message: sink not found");
    }
}

// /aoo/src/<id>/pong <id> <tt1> <tt2> <packetloss>

void Source::handle_pong(const osc::ReceivedMessage& msg,
                         const ip_address& addr)
{
    auto it = msg.ArgumentsBegin();
    AooId id = (it++)->AsInt32();
    time_tag tt1 = (it++)->AsTimeTag();
    time_tag tt2 = (it++)->AsTimeTag();
    float packetloss = (it++)->AsFloat();

    LOG_DEBUG("AooSource: handle pong");

    // check if sink exists (not strictly necessary, but might help catch errors)
    sink_lock lock(sinks_);
    auto sink = find_sink(addr, id);
    if (sink) {
        if (sink->is_active()){
        #if 0
            auto tt3 = timer_.get_absolute(); // use last stream time
        #else
            auto tt3 = aoo::time_tag::now(); // use real system time
        #endif
            // send ping event
            auto e = make_event<ping_event>(sink->ep, tt1, tt2, tt3, packetloss);
            send_event(std::move(e), kAooThreadLevelNetwork);
        } else {
            LOG_VERBOSE("AooSource: ignoring '" << kAooMsgPong << "' message: sink not active");
        }
    } else {
        LOG_VERBOSE("AooSource: ignoring '" << kAooMsgPong << "' message: sink not found");
    }
}

} // aoo
