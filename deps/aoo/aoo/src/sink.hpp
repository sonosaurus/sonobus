/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#pragma once

#include "aoo/aoo_sink.hpp"
#if AOO_NET
# include "aoo/aoo_client.hpp"
#endif

#include "common/lockfree.hpp"
#include "common/net_utils.hpp"
#include "common/sync.hpp"
#include "common/time.hpp"
#include "common/utils.hpp"

#include "binmsg.hpp"
#include "packet_buffer.hpp"
#include "detail.hpp"
#include "events.hpp"
#include "resampler.hpp"
#include "timer.hpp"
#include "time_dll.hpp"

#include "oscpack/osc/OscOutboundPacketStream.h"
#include "oscpack/osc/OscReceivedElements.h"

namespace aoo {

struct stream_stats {
    int32_t lost = 0;
    int32_t reordered = 0;
    int32_t resent = 0;
    int32_t dropped = 0;
};

enum class request_type {
    none,
    start,
    ping_reply,
    invite,
    uninvite,
    uninvite_all,
    // format
};

// used in 'source_desc'
struct request {
    request(request_type _type = request_type::none)
        : type(_type){}

    request_type type;
    union {
        struct {
            AooNtpTime tt1;
            AooNtpTime tt2;
        } ping;
        struct {
            AooId token;
        } uninvite;
    };
};

// used in 'sink'
struct source_request {
    source_request() = default;

    source_request(request_type _type)
        : type(_type) {}

    // NOTE: can't use aoo::endpoint here
    source_request(request_type _type, const ip_address& _addr, AooId _id)
        : type(_type), id(_id), address(_addr) {}

    request_type type;
    AooId id = kAooIdInvalid;
    ip_address address;
    union {
        struct {
            AooId token;
            AooData *metadata;
        } invite;
    };
};

enum class source_state {
    idle,
    run,
    stop,
    start,
    invite,
    uninvite,
    timeout
};

struct net_packet : data_packet {
    int32_t stream_id;
};

struct stream_message_header {
    stream_message_header *next;
    double time;
    AooDataType type;
    AooSize size;
};

struct flat_stream_message {
    stream_message_header header;
    char data[1];
};

class Sink;

class source_desc {
public:
#if AOO_NET
    source_desc(const ip_address& addr, AooId id,
                const ip_address& relay, double time);
#else
    source_desc(const ip_address& addr, AooId id, double time);
#endif

    source_desc(const source_desc& other) = delete;
    source_desc& operator=(const source_desc& other) = delete;

    ~source_desc();

    bool match(const ip_address& addr, AooId id) const {
        return (ep.address == addr) && (ep.id == id);
    }

    bool check_active(const Sink& s);

    bool has_events() const {
        return !eventqueue_.empty();
    }

    int32_t poll_events(Sink& s, AooEventHandler fn, void *user);

    AooError get_format(AooFormat& format);

    AooError codec_control(AooCtl ctl, void *data, AooSize size);

    // methods
    void reset(const Sink& s);

    AooError handle_start(const Sink& s, int32_t stream, uint32_t flags, int32_t format_id,
                          const AooFormat& f, const AooByte *settings, int32_t size, const AooData& md);

    AooError handle_stop(const Sink& s, int32_t stream);

    AooError handle_data(const Sink& s, net_packet& d, bool binary);

    AooError handle_ping(const Sink& s, time_tag tt);

    void send(const Sink& s, const sendfn& fn);

    bool process(const Sink& s, AooSample **buffer, int32_t nsamples,
                 AooStreamMessageHandler handler, void *user);

    void invite(const Sink& s, AooId token, AooData *metadata);

    void uninvite(const Sink& s);

    float get_buffer_fill_ratio();

    void add_xrun(double nblocks);
private:
    using shared_lock = sync::shared_lock<sync::shared_mutex>;
    using unique_lock = sync::unique_lock<sync::shared_mutex>;
    using scoped_lock = sync::scoped_lock<sync::shared_mutex>;
    using scoped_shared_lock = sync::scoped_shared_lock<sync::shared_mutex>;

    void update(const Sink& s);

    void handle_underrun(const Sink& s);

    bool add_packet(const Sink& s, const net_packet& d,
                    stream_stats& stats);

    bool try_decode_block(const Sink& s, stream_stats& stats);

    void check_missing_blocks(const Sink& s);

    // send messages
    void send_ping_reply(const Sink& s, AooNtpTime tt1, AooNtpTime tt2,
                         const sendfn& fn);

    void send_start_request(const Sink& s, const sendfn& fn);

    void send_data_requests(const Sink& s, const sendfn& fn);

    void send_invitations(const Sink& s, const sendfn& fn);
public:
    // data
    const endpoint ep;
private:
    AooId stream_id_ = kAooIdInvalid;
    AooId format_id_ = kAooIdInvalid;

    int32_t channel_ = 0; // recent channel onset
    AooStreamState streamstate_{kAooStreamStateInactive};
    bool underrun_{false};
    bool didupdate_{false};
    bool wait_for_buffer_{false};
    std::atomic<bool> binary_{false};
    double xrunblocks_ = 0;

    std::atomic<source_state> state_{source_state::idle};
    rt_metadata_ptr metadata_;
    rt_metadata_ptr invite_metadata_;
    std::atomic<int32_t> invite_token_{kAooIdInvalid};

    // timing
    std::atomic<float> invite_start_time_{0};
    std::atomic<float> last_invite_time_{0};
    std::atomic<float> last_packet_time_{0};
    // statistics
    std::atomic<int32_t> lost_blocks_{0};
    time_tag last_ping_reply_time_;
    // audio decoder
    std::unique_ptr<AooFormat, format_deleter> format_;
    std::unique_ptr<AooCodec, decoder_deleter> decoder_;
    // resampler
    dynamic_resampler resampler_;
    // packet queue and jitter buffer
    aoo::unbounded_mpsc_queue<net_packet> packetqueue_;
    jitter_buffer jitterbuffer_;
    int32_t wait_blocks_ = 0;
    int32_t latency_blocks_ = 0;
    // stream messages
    stream_message_header *stream_messages_ = nullptr;
    double stream_samples_ = 0;
    uint64_t process_samples_ = 0;
    void reset_stream_messages();
    // requests
    aoo::unbounded_mpsc_queue<request> requestqueue_;
    void push_request(const request& r){
        requestqueue_.push(r);
    }
    struct data_request {
        int32_t sequence;
        int32_t frame;
    };
    aoo::unbounded_mpsc_queue<data_request> datarequestqueue_;
    void push_data_request(const data_request& r){
        datarequestqueue_.push(r);
    }
    // events
    using event_queue = lockfree::unbounded_mpsc_queue<event_ptr, aoo::rt_allocator<event_ptr>>;
    event_queue eventqueue_;
    void send_event(const Sink& s, event_ptr e, AooThreadLevel level);
    // memory
    aoo::memory_list memory_;
    // thread synchronization
    sync::shared_mutex mutex_; // LATER replace with a spinlock?
};

class Sink final : public AooSink, rt_memory_pool_client {
public:
    Sink(AooId id, AooFlag flags, AooError *err);

    ~Sink();

    AooError AOO_CALL setup(AooSampleRate samplerate,
                            AooInt32 blocksize, AooInt32 nchannels) override;

    AooError AOO_CALL handleMessage(const AooByte *data, AooInt32 n,
                                    const void *address, AooAddrSize addrlen) override;

    AooError AOO_CALL send(AooSendFunc fn, void *user) override;

    AooError AOO_CALL process(AooSample **data, AooInt32 nsamples, AooNtpTime t,
                              AooStreamMessageHandler messageHandler, void *user) override;

    AooError AOO_CALL setEventHandler(AooEventHandler fn, void *user,
                                      AooEventMode mode) override;

    AooBool AOO_CALL eventsAvailable() override;

    AooError AOO_CALL pollEvents() override;

    AooError AOO_CALL inviteSource(
            const AooEndpoint& source, const AooData *metadata) override;

    AooError AOO_CALL uninviteSource(const AooEndpoint& source) override;

    AooError AOO_CALL uninviteAll() override;

    AooError AOO_CALL control(AooCtl ctl, AooIntPtr index,
                              void *ptr, AooSize size) override;

    AooError AOO_CALL codecControl(
            AooCtl ctl, AooIntPtr index, void *data, AooSize size) override;

    // getters
    AooId id() const { return id_.load(); }

    int32_t nchannels() const { return nchannels_; }

    int32_t samplerate() const { return samplerate_; }

    AooSampleRate real_samplerate() const { return realsr_.load(); }

    bool dynamic_resampling() const { return dynamic_resampling_.load();}

    int32_t blocksize() const { return blocksize_; }

    AooSeconds latency() const { return latency_.load(); }

    AooSeconds buffersize() const { return buffersize_.load(); }

    int32_t packetsize() const { return packetsize_.load(); }

    bool resend_enabled() const { return resend_.load(); }

    AooSeconds resend_interval() const { return resend_interval_.load(); }

    int32_t resend_limit() const { return resend_limit_.load(); }

    AooSeconds source_timeout() const { return source_timeout_.load(); }

    AooSeconds invite_timeout() const { return invite_timeout_.load(); }

    AooSeconds elapsed_time() const { return timer_.get_elapsed(); }

    time_tag absolute_time() const { return timer_.get_absolute(); }

    AooEventMode event_mode() const { return eventmode_; }

    void send_event(event_ptr e, AooThreadLevel level) const;

    void call_event(event_ptr e, AooThreadLevel level) const;
private:
    // settings
    parameter<AooId> id_;
    int32_t nchannels_ = 0;
    int32_t samplerate_ = 0;
    int32_t blocksize_ = 0;
#if AOO_NET
    AooClient *client_ = nullptr;
#endif
    // the sources
    using source_list = aoo::rcu_list<source_desc>;
    using source_lock = std::unique_lock<source_list>;
    source_list sources_;
    sync::mutex source_mutex_;
    // timing
    parameter<AooSampleRate> realsr_{0};
    time_dll dll_;
    timer timer_;
    // options
    parameter<float> latency_{ AOO_SINK_LATENCY };
    parameter<float> buffersize_{ 0 };
    parameter<float> resend_interval_{ AOO_RESEND_INTERVAL };
    parameter<int32_t> packetsize_{ AOO_PACKET_SIZE };
    parameter<int32_t> resend_limit_{ AOO_RESEND_LIMIT };
    parameter<float> source_timeout_{ AOO_SOURCE_TIMEOUT };
    parameter<float> invite_timeout_{ AOO_INVITE_TIMEOUT };
    parameter<float> dll_bandwidth_{ AOO_DLL_BANDWIDTH };
    parameter<bool> resend_{AOO_RESEND_DATA};
    parameter<bool> dynamic_resampling_{ AOO_DYNAMIC_RESAMPLING };
    parameter<bool> timer_check_{ AOO_XRUN_DETECTION };
    // events
    using event_queue = lockfree::unbounded_mpsc_queue<event_ptr, aoo::rt_allocator<event_ptr>>;
    mutable event_queue eventqueue_;
    AooEventHandler eventhandler_ = nullptr;
    void *eventcontext_ = nullptr;
    AooEventMode eventmode_ = kAooEventModeNone;
    // requests
    aoo::unbounded_mpsc_queue<source_request> requestqueue_;
    void push_request(const source_request& r){
        requestqueue_.push(r);
    }
    void dispatch_requests();

    // helper method

    source_desc *find_source(const ip_address& addr, AooId id);

    source_desc *get_source_arg(intptr_t index);

    source_desc *add_source(const ip_address& addr, AooId id);

    void reset_sources();

    AooError handle_start_message(const osc::ReceivedMessage& msg,
                                  const ip_address& addr);

    AooError handle_stop_message(const osc::ReceivedMessage& msg,
                                 const ip_address& addr);

    AooError handle_data_message(const osc::ReceivedMessage& msg,
                                 const ip_address& addr);

    AooError handle_data_message(const AooByte *msg, int32_t n,
                                 AooId id, const ip_address& addr);

    AooError handle_data_packet(net_packet& d, bool binary,
                                const ip_address& addr, AooId id);

    AooError handle_ping_message(const osc::ReceivedMessage& msg,
                                 const ip_address& addr);
};

} // aoo
