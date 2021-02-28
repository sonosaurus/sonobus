/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others. 
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "source.hpp"
#include "aoo/aoo_utils.hpp"

#include <cstring>
#include <algorithm>
#include <random>
#include <cmath>

/*//////////////////// AoO source /////////////////////*/

#define AOO_DATA_HEADERSIZE 80
// address pattern string: max 32 bytes
// typetag string: max. 12 bytes
// args (without blob data): 36 bytes

aoo_source * aoo_source_new(int32_t id) {
    return new aoo::source(id);
}

aoo::source::source(int32_t id)
    : id_(id)
{
    // event queue
    eventqueue_.resize(AOO_EVENTQUEUESIZE, 1);
    // request queues
    formatrequestqueue_.resize(64, 1);
    datarequestqueue_.resize(1024, 1);
}

void aoo_source_free(aoo_source *src){
    // cast to correct type because base class
    // has no virtual destructor!
    delete static_cast<aoo::source *>(src);
}

aoo::source::~source() {}

template<typename T>
T& as(void *p){
    return *reinterpret_cast<T *>(p);
}

#define CHECKARG(type) assert(size == sizeof(type))

int32_t aoo_source_set_option(aoo_source *src, int32_t opt, void *p, int32_t size)
{
    return src->set_option(opt, p, size);
}

int32_t aoo::source::set_option(int32_t opt, void *ptr, int32_t size)
{
    switch (opt){
    // id
    case aoo_opt_id:
    {
        auto newid = as<int32_t>(ptr);
        if (id_.exchange(newid) != newid){
            unique_lock lock(update_mutex_); // writer lock!
            update();
        }
        break;
    }
    // stop
    case aoo_opt_stop:
        play_ = false;
        break;
    // resume
    case aoo_opt_start:
    {
        unique_lock lock(update_mutex_); // writer lock!
        update();
        play_ = true;
        break;
    }
    // format
    case aoo_opt_format:
        CHECKARG(aoo_format);
        return set_format(as<aoo_format>(ptr));
    // buffersize
    case aoo_opt_buffersize:
    {
        CHECKARG(int32_t);
        auto bufsize = std::max<int32_t>(as<int32_t>(ptr), 0);
        if (bufsize != buffersize_){
            buffersize_ = bufsize;
            unique_lock lock(update_mutex_); // writer lock!
            update();
        }
        break;
    }
    // packetsize
    case aoo_opt_packetsize:
    {
        CHECKARG(int32_t);
        const int32_t minpacketsize = AOO_DATA_HEADERSIZE + 64;
        auto packetsize = as<int32_t>(ptr);
        if (packetsize < minpacketsize){
            LOG_WARNING("packet size too small! setting to " << minpacketsize);
            packetsize_ = minpacketsize;
        } else if (packetsize > AOO_MAXPACKETSIZE){
            LOG_WARNING("packet size too large! setting to " << AOO_MAXPACKETSIZE);
            packetsize_ = AOO_MAXPACKETSIZE;
        } else {
            packetsize_ = packetsize;
        }
        break;
    }
    // dynamic resampling
    case aoo_opt_dynamic_resampling:
        CHECKARG(int32_t);
        dynamic_resampling_ = std::max<int32_t>(0, as<int32_t>(ptr));
        break;
    // timefilter bandwidth
    case aoo_opt_timefilter_bandwidth:
        CHECKARG(float);
        // time filter
        bandwidth_ = as<float>(ptr);
        timer_.reset(); // will update
        break;
    // ping interval
    case aoo_opt_ping_interval:
        CHECKARG(int32_t);
        ping_interval_ = std::max<int32_t>(0, as<int32_t>(ptr)) * 0.001;
        break;
    // resend buffer size
    case aoo_opt_resend_buffersize:
    {
        CHECKARG(int32_t);
        // empty buffer is allowed! (no resending)
        auto bufsize = std::max<int32_t>(as<int32_t>(ptr), 0);
        if (bufsize != resend_buffersize_){
            resend_buffersize_ = bufsize;
            unique_lock lock(update_mutex_); // writer lock!
            update_historybuffer();
        }
        break;
    }
    // ping interval
    case aoo_opt_redundancy:
        CHECKARG(int32_t);
        // limit it somehow, 16 times is already very high
        redundancy_ = std::max<int32_t>(1, std::min<int32_t>(16, as<int32_t>(ptr)));
        break;
    case aoo_opt_respect_codec_change_requests:
        CHECKARG(int32_t);
        respect_codec_change_req_ = as<int32_t>(ptr);
        break;
    // format
    case aoo_opt_userformat:
        return set_userformat(ptr, size);
    // unknown
    default:
        LOG_WARNING("aoo_source: unsupported option " << opt);
        return 0;
    }
    return 1;
}

int32_t aoo_source_get_option(aoo_source *src, int32_t opt, void *p, int32_t size)
{
    return src->get_option(opt, p, size);
}

int32_t aoo::source::get_option(int32_t opt, void *ptr, int32_t size)
{
    switch (opt){
    // id
    case aoo_opt_id:
        CHECKARG(int32_t);
        as<int32_t>(ptr) = id();
        break;
    // format
    case aoo_opt_format:
        CHECKARG(aoo_format_storage);
        if (encoder_){
            shared_lock lock(update_mutex_); // read lock!
            return encoder_->get_format(as<aoo_format_storage>(ptr));
        } else {
            return 0;
        }
        break;
    // buffer size
    case aoo_opt_buffersize:
        CHECKARG(int32_t);
        as<int32_t>(ptr) = buffersize_;
        break;
    // time filter bandwidth
    case aoo_opt_timefilter_bandwidth:
        CHECKARG(float);
        as<float>(ptr) = bandwidth_;
        break;
    // resend buffer size
    case aoo_opt_resend_buffersize:
        CHECKARG(int32_t);
        as<int32_t>(ptr) = resend_buffersize_;
        break;
    // packetsize
    case aoo_opt_packetsize:
        CHECKARG(int32_t);
        as<int32_t>(ptr) = packetsize_;
        break;
    // ping interval
    case aoo_opt_ping_interval:
        CHECKARG(int32_t);
        as<int32_t>(ptr) = ping_interval_ * 1000;
        break;
    // ping interval
    case aoo_opt_redundancy:
        CHECKARG(int32_t);
        as<int32_t>(ptr) = redundancy_;
        break;
    // unknown
    default:
        LOG_WARNING("aoo_source: unsupported option " << opt);
        return 0;
    }
    return 1;
}

int32_t aoo_source_set_sinkoption(aoo_source *src, void *endpoint, int32_t id,
                              int32_t opt, void *p, int32_t size)
{
    return src->set_sinkoption(endpoint, id, opt, p, size);
}

int32_t aoo::source::set_sinkoption(void *endpoint, int32_t id,
                                   int32_t opt, void *ptr, int32_t size)
{
    if (id == AOO_ID_WILDCARD){
        // set option on all sinks on the given endpoint
        switch (opt){
        // channel onset
        case aoo_opt_channelonset:
        {
            CHECKARG(int32_t);
            auto chn = as<int32_t>(ptr);
            shared_lock lock(sink_mutex_); // reader lock!
            for (auto& sink : sinks_){
                if (sink.user == endpoint){
                    sink.channel = chn;
                }
            }
            LOG_VERBOSE("aoo_source: send to all sinks on channel " << chn);
            break;
        }
        // unknown
        default:
            LOG_WARNING("aoo_source: unsupported sink option " << opt);
            return 0;
        }
        return 1;
    } else {
        shared_lock lock(sink_mutex_); // reader lock!
        auto sink = find_sink(endpoint, id);
        if (sink){
            if (sink->id == AOO_ID_WILDCARD){
                LOG_WARNING("aoo_source: can't set individual sink option "
                            << opt << " because of wildcard");
                return 0;
            }

            switch (opt){
            // channel onset
            case aoo_opt_channelonset:
            {
                CHECKARG(int32_t);
                auto chn = as<int32_t>(ptr);
                sink->channel = chn;
                LOG_VERBOSE("aoo_source: send to sink " << sink->id
                            << " on channel " << chn);
                break;
            }
            case aoo_opt_protocol_flags:
            {
                CHECKARG(int32_t);
                auto flags = as<int32_t>(ptr);
                sink->protocol_flags = flags;
                LOG_VERBOSE("aoo_source: protocol_flags to sink " << sink->id
                            << " flags " << flags);
                break;
            }
            // unknown
            default:
                LOG_WARNING("aoo_source: unknown sink option " << opt);
                return 0;
            }
            return 1;
        } else {
            LOG_ERROR("aoo_source: couldn't set option " << opt
                      << " - sink not found!");
            return 0;
        }
    }
}

int32_t aoo_source_get_sinkoption(aoo_source *src, void *endpoint, int32_t id,
                              int32_t opt, void *p, int32_t size)
{
    return src->get_sinkoption(endpoint, id, opt, p, size);
}

int32_t aoo::source::get_sinkoption(void *endpoint, int32_t id,
                              int32_t opt, void *p, int32_t size)
{
    if (id == AOO_ID_WILDCARD){
        LOG_ERROR("aoo_source: can't use wildcard to get sink option");
        return 0;
    }

    shared_lock lock(sink_mutex_); // reader lock!
    auto sink = find_sink(endpoint, id);
    if (sink){
        switch (opt){
        // channel onset
        case aoo_opt_channelonset:
            CHECKARG(int32_t);
            as<int32_t>(p) = sink->channel;
            break;
        // unknown
        default:
            LOG_WARNING("aoo_source: unsupported sink option " << opt);
            return 0;
        }
        return 1;
    } else {
        LOG_ERROR("aoo_source: couldn't get option " << opt
                  << " - sink " << id << " not found!");
        return 0;
    }
}

int32_t aoo_source_setup(aoo_source *src, int32_t samplerate,
                         int32_t blocksize, int32_t nchannels){
    return src->setup(samplerate, blocksize, nchannels);
}

int32_t aoo::source::setup(int32_t samplerate,
                           int32_t blocksize, int32_t nchannels){
    unique_lock lock(update_mutex_); // writer lock!
    if (samplerate > 0 && blocksize > 0 && nchannels > 0)
    {
        nchannels_ = nchannels;
        samplerate_ = samplerate;
        blocksize_ = blocksize;

        // reset timer + time DLL filter
        timer_.setup(samplerate_, blocksize_);

        update();

        return 1;
    }

    return 0;
}

int32_t aoo_source_add_sink(aoo_source *src, void *sink, int32_t id, aoo_replyfn fn) {
    return src->add_sink(sink, id, fn);
}

int32_t aoo::source::add_sink(void *endpoint, int32_t id, aoo_replyfn fn){
    unique_lock lock(sink_mutex_); // writer lock!
    if (id == AOO_ID_WILDCARD){
        // first remove all sinks on the given endpoint!
        auto it = std::remove_if(sinks_.begin(), sinks_.end(), [&](auto& s){
            return s.user == endpoint;
        });
        sinks_.erase(it, sinks_.end());
    } else {
        // check if sink exists!
        auto result = find_sink(endpoint, id);
        if (result){
            if (result->id == AOO_ID_WILDCARD){
                LOG_WARNING("aoo_source: can't add individual sink "
                            << id << " because of wildcard!");
            } else {
                LOG_WARNING("aoo_source: sink already added!");
            }
            return 0;
        }
    }
    // add sink descriptor
    sinks_.emplace_back(endpoint, fn, id);
    // notify send_format()
    format_changed_ = true;

    return 1;
}

int32_t aoo_source_remove_sink(aoo_source *src, void *sink, int32_t id) {
    return src->remove_sink(sink, id);
}

int32_t aoo::source::remove_sink(void *endpoint, int32_t id){
    unique_lock lock(sink_mutex_); // writer lock!
    if (id == AOO_ID_WILDCARD){
        // remove all sinks on the given endpoint
        auto it = std::remove_if(sinks_.begin(), sinks_.end(), [&](auto& s){
            return s.user == endpoint;
        });
        sinks_.erase(it, sinks_.end());
        return 1;
    } else {
        for (auto it = sinks_.begin(); it != sinks_.end(); ++it){
            if (it->user == endpoint){
                if (it->id == AOO_ID_WILDCARD){
                    LOG_WARNING("aoo_source: can't remove individual sink "
                                << id << " because of wildcard!");
                    return 0;
                } else if (it->id == id){
                    sinks_.erase(it);
                    return 1;
                }
            }
        }
        LOG_WARNING("aoo_source: sink not found!");
        return 0;
    }
}

void aoo_source_remove_all(aoo_source *src) {
    src->remove_all();
}

void aoo::source::remove_all(){
    unique_lock lock(sink_mutex_); // writer lock!
    sinks_.clear();
}

int32_t aoo_source_handle_message(aoo_source *src, const char *data, int32_t n,
                              void *sink, aoo_replyfn fn) {
    return src->handle_message(data, n, sink, fn);
}

// /aoo/src/<id>/format <sink>
int32_t aoo::source::handle_message(const char *data, int32_t n, void *endpoint, aoo_replyfn fn){
    try {
        osc::ReceivedPacket packet(data, n);
        osc::ReceivedMessage msg(packet);

        int32_t type, src;
        auto onset = aoo_parse_pattern(data, n, &type, &src);
        if (!onset){
            LOG_WARNING("aoo_source: not an AoO message!");
            return 0;
        }
        if (type != AOO_TYPE_SOURCE){
            LOG_WARNING("aoo_source: not a source message!");
            return 0;
        }
        if (src == AOO_ID_WILDCARD){
            LOG_WARNING("aoo_source: can't handle wildcard messages (yet)!");
            return 0;
        }
        if (src != id()){
            LOG_WARNING("aoo_source: wrong source ID!");
            return 0;
        }

        auto pattern = msg.AddressPattern() + onset;
        if (!strcmp(pattern, AOO_MSG_FORMAT)){
            handle_format_request(endpoint, fn, msg);
            return 1;
        } else if (!strcmp(pattern, AOO_MSG_DATA)){
            handle_data_request(endpoint, fn, msg);
            return 1;
        } else if (!strcmp(pattern, AOO_MSG_INVITE)){
            handle_invite(endpoint, fn, msg);
            return 1;
        } else if (!strcmp(pattern, AOO_MSG_UNINVITE)){
            handle_uninvite(endpoint, fn, msg);
            return 1;
        } else if (!strcmp(pattern, AOO_MSG_PING)){
            handle_ping(endpoint, fn, msg);
            return 1;
        } else if (!strcmp(pattern, AOO_MSG_CODEC_CHANGE)){
            handle_codec_change(endpoint, fn, msg);
            return 1;
        } else {
            LOG_WARNING("unknown message " << pattern);
        }
    } catch (const osc::Exception& e){
        LOG_ERROR("aoo_source: exception in handle_message: " << e.what());
    }
    return 0;
}

int32_t aoo_source_send(aoo_source *src) {
    return src->send();
}

// This method reads audio samples from the ringbuffer,
// encodes them and sends them to all sinks.
// We have to aquire both the update lock and the sink list lock
// and release both before calling the sink's send method
// to avoid possible deadlocks in the client code.
// We have to make a local copy of the sink list, but this should be
// rather cheap in comparison to encoding and sending the audio data.
int32_t aoo::source::send(){
    if (!play_.load() && !activeplay_.load()){
        return false;
    }

    bool didsomething = false;

    if (send_format()){
        didsomething = true;
    }

    if (send_data()){
        didsomething = true;
    }

    if (resend_data()){
        didsomething = true;
    }

    if (send_ping()){
        didsomething = true;
    }

    return didsomething;
}

int32_t aoo_source_process(aoo_source *src, const aoo_sample **data, int32_t n, uint64_t t) {
    return src->process(data, n, t);
}

int32_t aoo::source::process(const aoo_sample **data, int32_t n, uint64_t t){
    if (!play_ && !activeplay_){
        return 0; // pausing
    }

    // update time DLL filter
    double error;
    auto state = timer_.update(t, error);
    if (state == timer::state::reset){
        LOG_DEBUG("setup time DLL filter for source");
        dll_.setup(samplerate_, blocksize_, bandwidth_, 0);
    } else if (state == timer::state::error){
        // skip blocks
        double period = (double)blocksize_ / (double)samplerate_;
        int nblocks = error / period + 0.5;
        LOG_VERBOSE("skip " << nblocks << " blocks");
        dropped_ += nblocks;
        timer_.reset();
    } else {
        auto elapsed = timer_.get_elapsed();
        dll_.update(elapsed);
    #if AOO_DEBUG_DLL
        DO_LOG("time elapsed: " << elapsed << ", period: " << dll_.period()
               << ", samplerate: " << dll_.samplerate());
    #endif
    }

    // if the DLL samplerate is any more than +/- 10% of our nominal, we'll ignore it
    // some shenanigans are going on
    bool ignoredll = !dynamic_resampling_.load();;
    if (fabs(dll_.samplerate() - (double)samplerate_) > 0.1*samplerate_) {
        ignoredll = true;
    }
    
    
    // the mutex should be uncontended most of the time.
    // NOTE: We could use try_lock() and skip the block if we couldn't aquire the lock.
    shared_lock lock(update_mutex_);

    if (!encoder_){
        return 0;
    }

     bool dofadein = play_ && !lastplay_;
     bool dofadeout = !play_ && lastplay_;
     
     if (dofadeout) {
         pushing_silent_frames_ = 4 * encoder_->blocksize();
         LOG_VERBOSE("do play fadeout, pushing silent: " << pushing_silent_frames_);
     }
     if (dofadein) {
         LOG_VERBOSE("do play fadein");
     }
     
     lastplay_ = play_;

     bool pushingSilence = !dofadeout && !play_ && pushing_silent_frames_ > 0;
     
     if (play_) {
         activeplay_ = true;
     } 
     else if (!dofadeout && pushing_silent_frames_ <= 0 && !flushingout_) {
         // already done our fadeout, and we aren't active anymore, so don't do any more processing
         //LOG_WARNING("last play processing");
         //activeplay_ = 0;
         pushing_silent_frames_ = 0;
         flushingout_ = 1;
         // activeplay_ will be set to false in the sending when it runs out of data
         return 0;
     }

    


    
    // non-interleaved -> interleaved
    //auto insamples = blocksize_ * nchannels_;
    auto insamples = n * nchannels_;
    auto outsamples = audioqueue_.blocksize(); // encoder_->blocksize() * nchannels_;
    auto *buf = (aoo_sample *)alloca(insamples * sizeof(aoo_sample));

    if (n > 0 && (dofadein || dofadeout || pushingSilence)) {
        const float fadedelta = dofadeout ? (-1.0f / n) : pushingSilence ? 0.0f : (1.0f / n);
        
        for (int i = 0; i < nchannels_; ++i){
            float gain = dofadeout ? 1.0f : 0.0f;
            for (int j = 0; j < n; ++j){
                buf[j * nchannels_ + i] = data[i][j] * gain;
                gain += fadedelta;
            }
        }
    } else {
        for (int i = 0; i < nchannels_; ++i){
            for (int j = 0; j < n; ++j){
                buf[j * nchannels_ + i] = data[i][j];
            }
        }
    }

    // ALWAYS use resampling buffer, just in case the caller needs to call us 
    // with varying sample counts on occasion. This can happen for various reasons
    // including being used within an audio plugin where the host may split the audio call
    // back calling with fewer samples. More importantly, this allows us to better decouple 
    // the audio process blocksize from the audioqueue blocksize (which matches the codec blocksize).

    //if (encoder_->blocksize() != blocksize_ || encoder_->samplerate() != samplerate_)
    {
        // go through resampler
        auto samplesleft = insamples;
        auto * pbuf = buf;

        auto availsamples = resampler_.write_available();
        
        while (samplesleft > 0) {
            auto usesamples = std::min(samplesleft, availsamples);
            
            resampler_.write(pbuf, usesamples);

            samplesleft -= usesamples;
            pbuf += usesamples;
            
            bool didconsume = false;
            
            while (resampler_.read_available() >= outsamples
                   && audioqueue_.write_available()
                   && srqueue_.write_available())
            {
                // copy audio samples
                resampler_.read(audioqueue_.write_data(), outsamples);
                audioqueue_.write_commit();
                
                // push samplerate
                if (!ignoredll) {
                    auto ratio = (double)encoder_->samplerate() / (double)samplerate_;
                    srqueue_.write(dll_.samplerate() * ratio);
                } else {
                    srqueue_.write(encoder_->samplerate());
                }

                didconsume = true;
            }

            // now update after any processing
            availsamples = resampler_.write_available();
            
            if (!didconsume && samplesleft > availsamples) {
                // didn't consume any, and we can't fit any more
                //LOG_WARNING("resampler could not handle all input samples, " << samplesleft << " unprocessed, avail " << availsamples << " audioqu_wravail: " << audioqueue_.write_available()  << " audqbs: " << audioqueue_.blocksize() << " encbs: " << encoder_->blocksize());
                break;
            }
        }
    } 
#if 0
    else {
        // bypass resampler
        if (audioqueue_.write_available() && srqueue_.write_available()){
            // copy audio samples
            std::copy(buf, buf + outsamples, audioqueue_.write_data());
            audioqueue_.write_commit();

            // push samplerate
            srqueue_.write(dll_.samplerate());
        } else {
            // LOG_DEBUG("couldn't process");
        }
    }
#endif
    
    if (pushing_silent_frames_ > 0) {
        pushing_silent_frames_ -= n;
    }
    
    return 1;
}

int32_t aoo_source_events_available(aoo_source *src){
    return src->events_available();
}

int32_t aoo::source::events_available(){
    return eventqueue_.read_available() > 0;
}

int32_t aoo_source_handle_events(aoo_source *src,
                                aoo_eventhandler fn, void *user){
    return src->handle_events(fn, user);
}

int32_t aoo::source::handle_events(aoo_eventhandler fn, void *user){
    // always thread-safe
    auto n = eventqueue_.read_available();
    if (n > 0){
        // copy events
        auto events = (event *)alloca(sizeof(event) * n);
        for (int i = 0; i < n; ++i){
            eventqueue_.read(events[i]);
        }
        auto vec = (const aoo_event **)alloca(sizeof(aoo_event *) * n);
        for (int i = 0; i < n; ++i){
            vec[i] = (aoo_event *)&events[i];
        }
        // send events
        fn(user, vec, n);
    }
    return n;
}

namespace aoo {

/*//////////////////////////////// endpoint /////////////////////////////////////*/

// /aoo/sink/<id>/data <src> <salt> <seq> <sr> <channel_onset> <totalsize> <nframes> <frame> <data>

void endpoint::send_data(int32_t src, int32_t salt, const aoo::data_packet& d) const{
    // call without lock!

    char buf[AOO_MAXPACKETSIZE];
    osc::OutboundPacketStream msg(buf, sizeof(buf));

    if (id != AOO_ID_WILDCARD){
        const int32_t max_addr_size = AOO_MSG_DOMAIN_LEN
                + AOO_MSG_SINK_LEN + 16 + AOO_MSG_DATA_LEN;
        char address[max_addr_size];
        snprintf(address, sizeof(address), "%s%s/%d%s",
                 AOO_MSG_DOMAIN, AOO_MSG_SINK, id, AOO_MSG_DATA);

        msg << osc::BeginMessage(address);
    } else {
        msg << osc::BeginMessage(AOO_MSG_DOMAIN AOO_MSG_SINK AOO_MSG_WILDCARD AOO_MSG_DATA);
    }

    msg << src << salt << d.sequence << d.samplerate << d.channel
        << d.totalsize << d.nframes << d.framenum
        << osc::Blob(d.data, d.size) << osc::EndMessage;

    LOG_DEBUG("send block: seq = " << d.sequence << ", sr = " << d.samplerate
              << ", chn = " << d.channel << ", totalsize = " << d.totalsize
              << ", nframes = " << d.nframes << ", frame = " << d.framenum << ", size " << d.size << " msgsize: " << msg.Size() << "  overhead = " << (int) (100 * (1.0 - d.size/(double)msg.Size())) << "%");


    send(msg.Data(), (int32_t)msg.Size());
}

// /d <salt> <seq> <data>
// /d <salt> <seq> <srate> <data>

void endpoint::send_data_compact(int32_t src, int32_t salt, const aoo::data_packet& d, bool sendrate) {
    // call without lock!

    char buf[AOO_MAXPACKETSIZE];
    osc::OutboundPacketStream msg(buf, sizeof(buf));
    
    msg << osc::BeginMessage(AOO_MSG_COMPACT_DATA);

    // the salt is how we identify both ourselves and our target
    
    msg << salt << d.sequence;
    
    // only use the 4 argument version (with samplerate as double) if there is big enough divergence from prior samplerate
    if (sendrate) {
        msg << d.samplerate;
    }
    
    msg << osc::Blob(d.data, d.size) << osc::EndMessage;

    LOG_DEBUG("send compact block: seq = " << d.sequence << ", sr = " << d.samplerate
              << ", chn = " << d.channel << ", totalsize = " << d.totalsize
              << ", nframes = " << d.nframes << ", frame = " << d.framenum << ", size " << d.size << " msgsize: " << msg.Size() << "  overhead = " << (int) (100 * (1.0 - d.size/(double)msg.Size())) << "%");


    send(msg.Data(), (int32_t)msg.Size());
}

// /aoo/sink/<id>/format <src> <version> <salt> <numchannels> <samplerate> <blocksize> <codec> <options...> [<userformat..>]

void endpoint::send_format(int32_t src, int32_t salt, const aoo_format& f,
                            const char *options, int32_t size, const char * userformat, int32_t ufsize) const {
    // call without lock!
    LOG_DEBUG("send format to " << id << " (salt = " << salt << ")");

    char buf[AOO_MAXPACKETSIZE];
    osc::OutboundPacketStream msg(buf, sizeof(buf));

    if (id != AOO_ID_WILDCARD){
        const int32_t max_addr_size = AOO_MSG_DOMAIN_LEN
                + AOO_MSG_SINK_LEN + 16 + AOO_MSG_FORMAT_LEN;
        char address[max_addr_size];
        snprintf(address, sizeof(address), "%s%s/%d%s",
                 AOO_MSG_DOMAIN, AOO_MSG_SINK, id, AOO_MSG_FORMAT);

        msg << osc::BeginMessage(address);
    } else {
        msg << osc::BeginMessage(AOO_MSG_DOMAIN AOO_MSG_SINK AOO_MSG_WILDCARD AOO_MSG_FORMAT);
    }

    msg << src << (int32_t)make_version(AOO_PROTOCOL_FLAG_COMPACT_DATA) << salt << f.nchannels << f.samplerate << f.blocksize
    << f.codec << osc::Blob(options, size);

    if (userformat && ufsize > 0) {
        msg << osc::Blob(userformat, ufsize);
    }

    msg << osc::EndMessage;

    send(msg.Data(), (int32_t)msg.Size());
}

// /aoo/sink/<id>/ping <src> <time>

void endpoint::send_ping(int32_t src, time_tag t) const {
    // call without lock!
    LOG_DEBUG("send ping to " << id);

    char buf[AOO_MAXPACKETSIZE];
    osc::OutboundPacketStream msg(buf, sizeof(buf));

    if (id != AOO_ID_WILDCARD){
        const int32_t max_addr_size = AOO_MSG_DOMAIN_LEN
                + AOO_MSG_SINK_LEN + 16 + AOO_MSG_PING_LEN;
        char address[max_addr_size];
        snprintf(address, sizeof(address), "%s%s/%d%s",
                 AOO_MSG_DOMAIN, AOO_MSG_SINK, id, AOO_MSG_PING);

        msg << osc::BeginMessage(address);
    } else {
        msg << osc::BeginMessage(AOO_MSG_DOMAIN AOO_MSG_SINK AOO_MSG_WILDCARD AOO_MSG_PING);
    }

    msg << src << osc::TimeTag(t.to_uint64()) << osc::EndMessage;

    send(msg.Data(), (int32_t)msg.Size());
}

/*///////////////////////// source ////////////////////////////////*/

sink_desc * source::find_sink(void *endpoint, int32_t id){
    for (auto& sink : sinks_){
        if ((sink.user == endpoint) &&
            (sink.id == AOO_ID_WILDCARD || sink.id == id))
        {
            return &sink;
        }
    }
    return nullptr;
}

int32_t source::set_format(aoo_format &f){
    unique_lock lock(update_mutex_); // writer lock!
    if (!encoder_ || strcmp(encoder_->name(), f.codec)){
        auto codec = aoo::find_codec(f.codec);
        if (codec){
            encoder_ = codec->create_encoder();
        } else {
            LOG_ERROR("codec '" << f.codec << "' not supported!");
            return 0;
        }
        if (!encoder_){
            LOG_ERROR("couldn't create encoder!");
            return 0;
        }
    }
    encoder_->set_format(f);

    update();

    return 1;
}

int32_t source::make_salt(){
    thread_local std::random_device dev;
    thread_local std::mt19937 mt(dev());
    std::uniform_int_distribution<int32_t> dist;
    return dist(mt);
}

int32_t source::set_userformat(void * ptr, int32_t size){
    unique_lock lock(update_mutex_); // writer lock!

    if (size <= 0) {
        // clear any user format
        userformat_.clear();
    }
    else {
        auto cptr = static_cast<char*>(ptr);
        userformat_.assign(cptr, cptr+size);
    }
    update(); // to force format change message

    return 1;
}


// always called with update_mutex_ locked!
void source::update(){
    if (!encoder_){
        return;
    }
    assert(encoder_->blocksize() > 0 && encoder_->samplerate() > 0);

    if (blocksize_ > 0){
        assert(samplerate_ > 0 && nchannels_ > 0);
        // setup audio buffer
        auto nsamples = encoder_->blocksize() * nchannels_;
        double bufsize = (double)buffersize_ * encoder_->samplerate() * 0.001;
        bufsize = std::max(bufsize, (double)blocksize_); // needs to be at least one processing blocksize_ worth!
        auto d = div(bufsize, encoder_->blocksize());
        int32_t nbuffers = d.quot + (d.rem != 0); // round up
        nbuffers = std::max<int32_t>(nbuffers, 1); // need at least 1 buffer!
        audioqueue_.resize(nbuffers * nsamples, nsamples);
        srqueue_.resize(nbuffers, 1);
        LOG_DEBUG("aoo::source::update: id: " << id_ << " nbuffers = " << nbuffers << " dquot: " << d.quot << " drem: " << d.rem <<  " bufsize: " << bufsize << " bs: " << encoder_->blocksize() << " reqbufms: " << buffersize_);

        // resampler
       // if (blocksize_ != encoder_->blocksize() || samplerate_ != encoder_->samplerate()){
            resampler_.setup(blocksize_, encoder_->blocksize(),
                             samplerate_, encoder_->samplerate(), nchannels_);
            resampler_.update(samplerate_, encoder_->samplerate());
        //} else {
        //    resampler_.clear();
        //}

        // history buffer
        update_historybuffer();
        
        // reset encoder state to avoid old garbage
        encoder_->reset();
        
        // reset time DLL to be on the safe side
        timer_.reset();
        lastpingtime_ = -1000; // force first ping
        lastplay_ = false;
        
        // Start new sequence and resend format.
        // We naturally want to do this when setting the format,
        // but it's good to also do it in setup() to eliminate
        // any timing gaps.
        salt_ = make_salt();
        sequence_ = 0;
        dropped_ = 0;
        {
            shared_lock lock2(sink_mutex_);
            for (auto& sink : sinks_){
                sink.format_changed = true;
            }
            // notify send_format()
            format_changed_ = true;
        }
    }
}

void source::update_historybuffer(){
    if (samplerate_ > 0 && encoder_){
        double bufsize = (double)resend_buffersize_ * 0.001 * samplerate_;
        auto d = div(bufsize, encoder_->blocksize());
        int32_t nbuffers = d.quot + (d.rem != 0); // round up
        history_.resize(nbuffers);
    }
}

bool source::send_format(){
    bool format_changed = format_changed_.exchange(false);
    bool format_requested = formatrequestqueue_.read_available();

    if (!format_changed && !format_requested){
        return false;
    }

    shared_lock updatelock(update_mutex_); // reader lock!

    if (!encoder_){
        return false;
    }

    int32_t salt = salt_;

    aoo_format fmt;
    char settings[AOO_CODEC_MAXSETTINGSIZE];
    auto size = encoder_->write_format(fmt, settings, sizeof(settings));

    updatelock.unlock();

    if (size < 0){
        return false;
    }

    auto userfmt = !userformat_.empty() ? &*userformat_.begin() : nullptr;
    int32_t userfmtsize = (int32_t) userformat_.size();

    if (format_changed){
        // only copy sinks which require a format update!
        shared_lock sinklock(sink_mutex_);
        auto sinks = (aoo::sink_desc *)alloca((sinks_.size() + 1) * sizeof(aoo::endpoint)); // avoid alloca(0)
        int numsinks = 0;
        for (auto& sink : sinks_){
            if (sink.format_changed.exchange(false)){
                new (sinks + numsinks) aoo::endpoint (sink.user, sink.fn, sink.id);
                numsinks++;
            }
        }
        sinklock.unlock();
        // now we don't hold any lock!

        for (int i = 0; i < numsinks; ++i){
            sinks[i].send_format(id(), salt, fmt, settings, size, userfmt, userfmtsize);
        }
    }

    if (format_requested){
        while (formatrequestqueue_.read_available()){
            endpoint ep;
            formatrequestqueue_.read(ep);
            ep.send_format(id(), salt, fmt, settings, size, userfmt, userfmtsize);
        }
    }

    return true;
}

bool source::resend_data(){
    shared_lock updatelock(update_mutex_); // reader lock!
    if (!history_.capacity()){
        return false;
    }

    bool didsomething = false;

    while (datarequestqueue_.read_available()){
        data_request request;
        datarequestqueue_.read(request);

        auto salt = salt_;
        if (salt != request.salt){
            // outdated request
            continue;
        }

        auto block = history_.find(request.sequence);
        if (block){
            aoo::data_packet d;
            d.sequence = block->sequence;
            d.samplerate = block->samplerate;
            d.channel = block->channel;
            d.totalsize = block->size();
            d.nframes = block->num_frames();
            // We use a buffer on the heap because blocks and even frames
            // can be quite large and we don't want them to sit on the stack.
            if (request.frame < 0){
                // Copy whole block and save frame pointers.
                sendbuffer_.resize(d.totalsize);
                char *buf = sendbuffer_.data();
                char *frameptr[256];
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
                    d.framenum = i;
                    d.data = frameptr[i];
                    d.size = framesize[i];
                    request.send_data(id(), salt, d);
                }
            } else {
                // Copy a single frame
                if (request.frame >= 0 && request.frame < d.nframes){
                    int32_t size = block->frame_size(request.frame);
                    sendbuffer_.resize(size);
                    block->get_frame(request.frame, sendbuffer_.data(), size);
                    // unlock before sending
                    updatelock.unlock();

                    // send frame to sink
                    d.framenum = request.frame;
                    d.data = sendbuffer_.data();
                    d.size = size;
                    request.send_data(id(), salt, d);
                } else {
                    LOG_ERROR("frame number " << request.frame << " out of range!");
                }
            }
            // lock again
            updatelock.lock();

            didsomething = true;
        } else {
            LOG_VERBOSE("couldn't find block " << request.sequence);
        }
    }

    return didsomething;
}

bool source::send_data(){
    shared_lock updatelock(update_mutex_); // reader lock!
    if (!encoder_){
        return 0;
    }

    data_packet d;
    int32_t salt = salt_;

    // *first* check for dropped blocks
    // NOTE: there's no ABA problem because the variable will only be decremented in this method.
    if (dropped_ > 0){
        // send empty block
        d.sequence = sequence_++;
        d.samplerate = encoder_->samplerate(); // use nominal samplerate
        d.totalsize = 0;
        d.nframes = 0;
        d.framenum = 0;
        d.data = nullptr;
        d.size = 0;
        // now we can unlock
        updatelock.unlock();

        // make local copy of sink descriptors
        shared_lock listlock(sink_mutex_);
        int32_t numsinks = (int32_t) sinks_.size();
        auto sinks = (sink_desc *)alloca((numsinks + 1) * sizeof(sink_desc)); // avoid alloca(0)
        std::copy(sinks_.begin(), sinks_.end(), sinks);

        // unlock before sending!
        listlock.unlock();

        // send block to sinks
        for (int i = 0; i < numsinks; ++i){
            sinks[i].send_data(id(), salt, d);            
        }
        --dropped_;
    } else if (audioqueue_.read_available() && srqueue_.read_available()){
        // make local copy of sink descriptors
        shared_lock listlock(sink_mutex_);
        int32_t numsinks = (int32_t) sinks_.size();
        auto sinks = (sink_desc *)alloca((numsinks + 1) * sizeof(sink_desc)); // avoid alloca(0)
        std::copy(sinks_.begin(), sinks_.end(), sinks);

        // unlock before sending!
        listlock.unlock();

        d.sequence = sequence_++;
        srqueue_.read(d.samplerate); // always read samplerate from ringbuffer

        // for compact data sending purposes... only send rate when necessary
        bool sendrate = false;
        if (abs(d.samplerate - prev_sent_samplerate_) > 0.1) {
            sendrate = true;
            prev_sent_samplerate_ = d.samplerate;
        }
        
        if (numsinks){
            // copy and convert audio samples to blob data
            auto nchannels = encoder_->nchannels();
            auto blocksize = encoder_->blocksize();
            sendbuffer_.resize(sizeof(double) * nchannels * blocksize); // overallocate

            d.totalsize = encoder_->encode(audioqueue_.read_data(), audioqueue_.blocksize(),
                                           sendbuffer_.data(), (int32_t) sendbuffer_.size());
            audioqueue_.read_commit();

            if (d.totalsize > 0){
                // calculate number of frames
                auto maxpacketsize = packetsize_ - AOO_DATA_HEADERSIZE;
                auto dv = div(d.totalsize, maxpacketsize);
                d.nframes = dv.quot + (dv.rem != 0);

                // save block
                history_.push(d.sequence, d.samplerate, sendbuffer_.data(),
                              d.totalsize, d.nframes, maxpacketsize);

                // unlock before sending!
                updatelock.unlock();

                // from here on we don't hold any lock!

                // send a single frame to all sinks
                // /AoO/<sink>/data <src> <salt> <seq> <sr> <channel_onset> <totalsize> <numpackets> <packetnum> <data>
                auto dosend = [&](int32_t frame, const char* data, auto n){
                    d.framenum = frame;
                    d.data = data;
                    d.size = n;
                    for (int i = 0; i < numsinks; ++i){
                        d.channel = sinks[i].channel;
                        // if the protocol_flags allow using the compact data message, use it if appropriate
                        if (d.nframes == 1 && d.channel == 0 && sinks[i].protocol_flags & AOO_PROTOCOL_FLAG_COMPACT_DATA) {
                            sinks[i].send_data_compact(id(), salt, d, sendrate);                
                        } else {
                            sinks[i].send_data(id(), salt, d);
                        }
                    }
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
            } else {
                LOG_WARNING("aoo_source: couldn't encode audio data!");
            }
        } else {
            // drain buffer anyway
            audioqueue_.read_commit();
        }
    } else {
        // LOG_DEBUG("couldn't send");       
        if (!play_.load() && flushingout_.load() ) {
            LOG_VERBOSE("finished flushing out");
            activeplay_ = 0;
            flushingout_ = 0;
        }
        return 0;
    }

    // handle overflow (with 64 samples @ 44.1 kHz this happens every 36 days)
    // for now just force a reset by changing the salt, LATER think how to handle this better
    if (d.sequence == INT32_MAX){
        unique_lock lock2(update_mutex_); // take writer lock
        salt_ = make_salt();
    }

    return 1;
}

bool source::send_ping(){
    // if stream is stopped, the timer won't increment anyway
    auto elapsed = timer_.get_elapsed();
    auto pingtime = lastpingtime_.load();
    auto interval = ping_interval_.load(); // 0: no ping
    if (interval > 0 && (elapsed - pingtime) >= interval){
        // only copy sinks which require a format update!
        shared_lock sinklock(sink_mutex_);
        int32_t numsinks = (int32_t) sinks_.size();
        auto sinks = (aoo::sink_desc *)alloca((numsinks + 1) * sizeof(aoo::sink_desc)); // avoid alloca(0)
        std::copy(sinks_.begin(), sinks_.end(), sinks);
        sinklock.unlock();

        auto tt = timer_.get_absolute();

        for (int i = 0; i < numsinks; ++i){
            sinks[i].send_ping(id(), tt);
        }

        lastpingtime_ = elapsed;
        return true;
    } else {
        return false;
    }
}

void source::handle_format_request(void *endpoint, aoo_replyfn fn,
                                   const osc::ReceivedMessage& msg)
{
    LOG_DEBUG("handle format request");

    auto it = msg.ArgumentsBegin();

    auto id = (it++)->AsInt32();
    auto version = (it++)->AsInt32();

    // LATER handle this in the sink_desc (e.g. not sending data)
    if (!check_version(version)){
        LOG_ERROR("aoo_source: sink version not supported");
        return;
    }

    // check if sink exists (not strictly necessary, but might help catch errors)
    shared_lock lock(sink_mutex_); // reader lock!
    auto sink = find_sink(endpoint, id);
    lock.unlock();

    if (sink){
        sink->protocol_flags = version & 0xFF;
        if (formatrequestqueue_.write_available()){
            formatrequestqueue_.write(aoo::endpoint { endpoint, fn, id });
        }
    } else {
        LOG_WARNING("ignoring '" << AOO_MSG_FORMAT << "' message: sink not found");
    }
}

void source::handle_data_request(void *endpoint, aoo_replyfn fn,
                                 const osc::ReceivedMessage& msg)
{
    auto it = msg.ArgumentsBegin();
    auto id = (it++)->AsInt32();
    auto salt = (it++)->AsInt32();

    LOG_DEBUG("handle data request");

    // check if sink exists (not strictly necessary, but might help catch errors)
    shared_lock lock(sink_mutex_); // reader lock!
    auto sink = find_sink(endpoint, id);
    lock.unlock();

    if (sink){
        // get pairs of [seq, frame]
        int npairs = (msg.ArgumentCount() - 2) / 2;
        while (npairs--){
            auto seq = (it++)->AsInt32();
            auto frame = (it++)->AsInt32();
            if (datarequestqueue_.write_available()){
                datarequestqueue_.write(data_request{ endpoint, fn, id, salt, seq, frame });
            }
        }
    } else {
        LOG_WARNING("ignoring '" << AOO_MSG_DATA << "' message: sink not found");
    }
}

void source::handle_invite(void *endpoint, aoo_replyfn fn,
                           const osc::ReceivedMessage& msg)
{
    auto it = msg.ArgumentsBegin();
    auto id = (it++)->AsInt32();
    auto flags = it == msg.ArgumentsEnd() ? 0 : it->AsInt32();

    LOG_DEBUG("handle invite");

    // check if sink exists (not strictly necessary, but might help catch errors)
    shared_lock lock(sink_mutex_); // reader lock!
    auto sink = find_sink(endpoint, id);
    lock.unlock();

    if (!sink){
        // push "invite" event
        if (eventqueue_.write_available()){
            event e;
            e.type = AOO_INVITE_EVENT;
            e.sink.endpoint = endpoint;
            // Use 'id' because we want the individual sink! ('sink.id' might be a wildcard)
            e.sink.id = id;
            e.sink.flags = flags;
            eventqueue_.write(e);
        }
    } else {
        LOG_VERBOSE("ignoring '" << AOO_MSG_INVITE << "' message: sink already added");
    }
}

void source::handle_uninvite(void *endpoint, aoo_replyfn fn,
                             const osc::ReceivedMessage& msg)
{
    auto id = msg.ArgumentsBegin()->AsInt32();

    LOG_DEBUG("handle uninvite");

    // check if sink exists (not strictly necessary, but might help catch errors)
    shared_lock lock(sink_mutex_); // reader lock!
    auto sink = find_sink(endpoint, id);
    lock.unlock();

    if (sink){
        // push "uninvite" event
        if (eventqueue_.write_available()){
            event e;
            e.type = AOO_UNINVITE_EVENT;
            e.sink.endpoint = endpoint;
            // Use 'id' because we want the individual sink! ('sink.id' might be a wildcard)
            e.sink.id = id;
            eventqueue_.write(e);
        }
    } else {
        LOG_VERBOSE("ignoring '" << AOO_MSG_UNINVITE << "' message: sink not found");
    }
}

void source::handle_ping(void *endpoint, aoo_replyfn fn,
                         const osc::ReceivedMessage& msg)
{
    auto it = msg.ArgumentsBegin();
    auto id = (it++)->AsInt32();
    time_tag tt1 = (it++)->AsTimeTag();
    time_tag tt2 = (it++)->AsTimeTag();
    int32_t lost_blocks = (it++)->AsInt32();

    LOG_DEBUG("handle ping");

    // check if sink exists (not strictly necessary, but might help catch errors)
    shared_lock lock(sink_mutex_); // reader lock!
    auto sink = find_sink(endpoint, id);
    lock.unlock();

    if (sink){
        // push "ping" event
        if (eventqueue_.write_available()){
            event e;
            e.type = AOO_PING_EVENT;
            e.sink.endpoint = endpoint;
            // Use 'id' because we want the individual sink! ('sink.id' might be a wildcard)
            e.sink.id = id;
            e.ping.tt1 = tt1.to_uint64();
            e.ping.tt2 = tt2.to_uint64();
            e.ping.lost_blocks = lost_blocks;
        #if 0
            e.ping.tt3 = timer_.get_absolute().to_uint64(); // use last stream time
        #else
            e.ping.tt3 = aoo_osctime_get(); // use real system time
        #endif
            eventqueue_.write(e);
        }
    } else {
        LOG_VERBOSE("ignoring '" << AOO_MSG_PING << "' message: sink not found");
    }
}

// /aoo/src/<id>/codecchange <sink> <numchannels> <samplerate> <blocksize> <codec> <options...>

void source::handle_codec_change(void *endpoint, aoo_replyfn fn,
                                 const osc::ReceivedMessage& msg)
{
    if (!respect_codec_change_req_) {
        LOG_WARNING("Not allowing codec change request");
        return;
    }
    
    auto it = msg.ArgumentsBegin();

    int32_t id = (it++)->AsInt32();

    // get requested codec and format options from arguments
    // use our existing format for the other params
    aoo_format f;
    auto chans = (it++)->AsInt32();
    auto srate = (it++)->AsInt32();
    auto bsize = (it++)->AsInt32();
    f.codec = (it++)->AsString();

    f.nchannels = nchannels_; // use existing for now
    f.samplerate = samplerate_; // use existing for now
    f.blocksize = bsize;
    const void *settings;
    osc::osc_bundle_element_size_t size;
    (it++)->AsBlob(settings, size);

    if (id < 0){
        LOG_WARNING("bad ID for " << AOO_MSG_FORMAT << " message");
        return;
    }
    
   
    // check if sink exists (not strictly necessary, but might help catch errors)
    shared_lock lock(sink_mutex_); // reader lock!
    auto sink = find_sink(endpoint, id);
    lock.unlock();

    if (sink){
        { // only if the requesting sink exists we will respect this request
            LOG_DEBUG("handle codec change");
            unique_lock lock(update_mutex_); // writer lock!
            
            if (!encoder_ || strcmp(encoder_->name(), f.codec)){
                auto codec = aoo::find_codec(f.codec);
                if (codec){
                    encoder_ = codec->create_encoder();
                } else {
                    LOG_ERROR("codec '" << f.codec << "' not supported!");
                    return;
                }
                if (!encoder_){
                    LOG_ERROR("couldn't create encoder!");
                    return;
                }
            }
        
            encoder_->read_format(f, (const char *)settings, size);
            update();
        }
               
        
        // push "codec change" event
        if (eventqueue_.write_available()){
            event e;
            e.type = AOO_CHANGECODEC_EVENT;
            e.sink.endpoint = endpoint;
            // Use 'id' because we want the individual sink! ('sink.id' might be a wildcard)
            e.sink.id = id;
            eventqueue_.write(e);
        }
    } else {
        LOG_VERBOSE("ignoring '" << AOO_CHANGECODEC_EVENT << "' message: sink not found");
    }
}



} // aoo
