/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others. 
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#pragma once

#include "aoo/aoo.hpp"

#include "common/lockfree.hpp"
#include "common/net_utils.hpp"
#include "common/sync.hpp"
#include "common/time.hpp"
#include "common/utils.hpp"

#include "buffer.hpp"
#include "codec.hpp"
#include "imp.hpp"
#include "resampler.hpp"
#include "timer.hpp"
#include "time_dll.hpp"

#include "oscpack/osc/OscOutboundPacketStream.h"
#include "oscpack/osc/OscReceivedElements.h"

#ifndef DEBUG_MEMORY
#define DEBUG_MEMORY 0
#endif

namespace aoo {

struct stream_state {
    int32_t lost = 0;
    int32_t reordered = 0;
    int32_t resent = 0;
    int32_t dropped = 0;
};

class source_desc;

struct event
{
    event() = default;

    event(aoo_event_type type) : type_(type) {}

    event(aoo_event_type type, const source_desc& desc);

    union {
        aoo_event_type type_;
        aoo_event event_;
        aoo_source_event source;
        aoo_format_change_event format;
        aoo_format_timeout_event format_timeout;
        aoo_ping_event ping;
        aoo_stream_state_event source_state;
        aoo_block_lost_event block_loss;
        aoo_block_reordered_event block_reorder;
        aoo_block_resent_event block_resend;
        aoo_block_dropped_event block_dropped;
        aoo_block_gap_event block_gap;
    };
};

struct sink_event {
    sink_event() = default;

    sink_event(aoo_event_type _type) : type(_type) {}
    sink_event(aoo_event_type _type, const source_desc& desc);

    aoo_event_type type;
    ip_address address;
    aoo_id id;
    int32_t count; // for xrun event
};

enum class request_type {
    unknown,
    format,
    ping_reply,
    invite,
    uninvite,
    uninvite_all
};

// used in 'source_desc'
struct request {
    request(request_type _type = request_type::unknown)
        : type(_type){}

    request_type type;
    union {
        struct {
            uint64_t tt1;
            uint64_t tt2;
        } ping;
    };
};

// used in 'sink'
struct source_request {
    source_request() = default;

    source_request(request_type _type)
        : type(_type) {}

    source_request(request_type _type, const ip_address& _addr, aoo_id _id, uint32_t _flags=0)
        : type(_type), address(_addr), id(_id), flags(_flags) {}

    request_type type;
    ip_address address;
    aoo_id id = -1;
    uint32_t flags = 0;
};

class sink_imp;

enum class source_state {
    idle,
    stream,
    invite,
    uninvite
};

struct net_packet : data_packet {
    int32_t salt;
};

class source_desc {
public:
    source_desc(const ip_address& addr, aoo_id id, double time, uint32_t flags=0);

    source_desc(const source_desc& other) = delete;
    source_desc& operator=(const source_desc& other) = delete;

    ~source_desc();

    // getters
    aoo_id id() const { return id_; }

    const ip_address& address() const { return addr_; }

    bool match(const ip_address& addr, aoo_id id) const {
        return (addr_ == addr) && (id_ == id);
    }

    bool is_active(const sink_imp& s) const;

    bool is_inviting() const {
        return state_.load() == source_state::invite;
    }

    bool has_events() const {
        return !eventqueue_.empty();
    }

    int32_t poll_events(sink_imp& s, aoo_eventhandler fn, void *user);

    aoo_error get_format(aoo_format& format, size_t size);

    // methods
    void reset(const sink_imp& s);

    aoo_error handle_format(const sink_imp& s, int32_t salt, const aoo_format& f,
                            const char *settings, int32_t size, uint32_t flags);

    aoo_error handle_data(const sink_imp& s, net_packet& d, bool binary);

    aoo_error handle_ping(const sink_imp& s, time_tag tt);

    void send(const sink_imp& s, const sendfn& fn);

    bool process(const sink_imp& s, aoo_sample **buffer, int32_t nsamples, time_tag tt);

    void invite(const sink_imp& s);

    void uninvite(const sink_imp& s);

    aoo_error request_format(const sink_imp& s, const aoo_format& f);

    float get_buffer_fill_ratio();

    void add_xrun(int32_t nsamples){
        xrunsamples_ += nsamples;
    }
private:
    using shared_lock = sync::shared_lock<sync::shared_mutex>;
    using unique_lock = sync::unique_lock<sync::shared_mutex>;
    using scoped_lock = sync::scoped_lock<sync::shared_mutex>;
    using scoped_shared_lock = sync::scoped_shared_lock<sync::shared_mutex>;

    void update(const sink_imp& s);

    void add_lost(stream_state& state, int32_t n);

    void handle_underrun(const sink_imp& s);

    bool add_packet(const sink_imp& s, const net_packet& d,
                    stream_state& state);

    void process_blocks(const sink_imp& s, stream_state& state);

    void skip_blocks(const sink_imp& s);

    void check_missing_blocks(const sink_imp& s);

    // send messages
    void send_ping_reply(const sink_imp& s, const sendfn& fn,
                         const request& r);

    void send_format_request(const sink_imp& s, const sendfn& fn,
                             bool format = false);

    void send_data_requests(const sink_imp& s, const sendfn& fn);

    void send_invitation(const sink_imp& s, const sendfn& fn);

    void send_uninvitation(const sink_imp& s, const sendfn& fn);

    // data
    const ip_address addr_;
    const aoo_id id_;
    uint32_t flags_ = 0;
    int32_t salt_ = -1; // start with invalid stream ID!

    aoo_stream_state streamstate_;
    bool underrun_{false};
    bool didupdate_{false};
    std::atomic<bool> binary_{false};

    std::atomic<source_state> state_{source_state::idle};

    std::unique_ptr<aoo_format, format_deleter> format_request_;
    double format_time_ = 0;

    std::atomic<double> state_time_{0.0};
    std::atomic<double> last_packet_time_{0};
    std::atomic<int32_t> lost_since_ping_{0};
    // audio decoder
    std::unique_ptr<aoo::decoder> decoder_;
    // state
    int32_t channel_ = 0; // recent channel onset
    int32_t skipblocks_ = 0;
    float xrun_ = 0;
    int32_t xrunsamples_ = 0;
    // resampler
    dynamic_resampler resampler_;
    // queues and buffers
    struct block_data {
        struct {
            double samplerate;
            int32_t channel;
            int32_t padding;
        } header;
        aoo_sample data[1];
    };
    lockfree::spsc_queue<char, aoo::allocator<char>> audioqueue_;
    int32_t minblocks_ = 0;
    // packet queue and jitter buffer
    lockfree::unbounded_mpsc_queue<net_packet, aoo::allocator<net_packet>> packetqueue_;
    jitter_buffer jitterbuffer_;
    // requests
    lockfree::unbounded_mpsc_queue<request, aoo::allocator<request>> requestqueue_;
    void push_request(const request& r){
        requestqueue_.push(r);
    }
    struct data_request {
        int32_t sequence;
        int32_t frame;
    };
    lockfree::unbounded_mpsc_queue<data_request, aoo::allocator<data_request>> datarequestqueue_;
    void push_data_request(const data_request& r){
        datarequestqueue_.push(r);
    }
    // events
    lockfree::unbounded_mpsc_queue<event, aoo::allocator<event>> eventqueue_;
    void send_event(const sink_imp& s, const event& e, aoo_thread_level level);
    // thread synchronization
    sync::shared_mutex mutex_; // LATER replace with a spinlock?
};

class sink_imp final : public sink {
public:
    sink_imp(aoo_id id, uint32_t flags);

    ~sink_imp();

    aoo_error setup(int32_t samplerate, int32_t blocksize, int32_t nchannels) override;

    aoo_error handle_message(const char *data, int32_t n,
                             const void *address, int32_t addrlen) override;

    aoo_error send(aoo_sendfn fn, void *user) override;

    aoo_error process(aoo_sample **data, int32_t nsamples, uint64_t t) override;

    aoo_error set_eventhandler(aoo_eventhandler fn, void *user, int32_t mode) override;

    aoo_bool events_available() override;

    aoo_error poll_events() override;

    aoo_error control(int32_t ctl, intptr_t index, void *ptr, size_t size) override;

    // getters
    aoo_id id() const { return id_.load(std::memory_order_relaxed); }

    int32_t nchannels() const { return nchannels_; }

    int32_t samplerate() const { return samplerate_; }

    double real_samplerate() const { return realsr_.load(std::memory_order_relaxed); }

    bool dynamic_resampling() const { return dynamic_resampling_.load(std::memory_order_relaxed);}

    int32_t blocksize() const { return blocksize_; }

    int32_t buffersize() const { return buffersize_.load(std::memory_order_relaxed); }

    int32_t packetsize() const { return packetsize_.load(std::memory_order_relaxed); }

    bool resend_enabled() const { return resend_.load(std::memory_order_relaxed); }

    float resend_interval() const { return resend_interval_.load(std::memory_order_relaxed); }

    int32_t resend_limit() const { return resend_limit_.load(std::memory_order_relaxed); }

    float source_timeout() const { return source_timeout_.load(std::memory_order_relaxed); }

    double elapsed_time() const { return timer_.get_elapsed(); }

    time_tag absolute_time() const { return timer_.get_absolute(); }

    aoo_event_mode event_mode() const { return eventmode_; }
private:
    // settings
    std::atomic<aoo_id> id_;
    int32_t nchannels_ = 0;
    int32_t samplerate_ = 0;
    int32_t blocksize_ = 0;
    // the sources
    using source_list = lockfree::simple_list<source_desc, aoo::allocator<source_desc>>;
    using source_lock = std::unique_lock<source_list>;
    source_list sources_;
    sync::mutex source_mutex_;
    // timing
    std::atomic<double> realsr_{0};
    time_dll dll_;
    timer timer_;
    // options
    std::atomic<int32_t> buffersize_{ AOO_SINK_BUFFERSIZE };
    std::atomic<int32_t> packetsize_{ AOO_PACKETSIZE };
    std::atomic<float> resend_interval_{ AOO_RESEND_INTERVAL * 0.001 };
    std::atomic<int32_t> resend_limit_{ AOO_RESEND_LIMIT };
    std::atomic<float> source_timeout_{ AOO_SOURCE_TIMEOUT * 0.001 };
    std::atomic<float> dll_bandwidth_{ AOO_DLL_BANDWIDTH };
    std::atomic<bool> resend_{AOO_RESEND_DATA};
    std::atomic<bool> dynamic_resampling_{ AOO_DYNAMIC_RESAMPLING };
    std::atomic<bool> timer_check_{ AOO_TIMER_CHECK };
    // events
    lockfree::unbounded_mpsc_queue<sink_event, aoo::allocator<sink_event>> eventqueue_;
    void send_event(const sink_event& e, aoo_thread_level level);
public:
    void call_event(const event& e, aoo_thread_level level) const;
private:
    aoo_eventhandler eventhandler_ = nullptr;
    void *eventcontext_ = nullptr;
    aoo_event_mode eventmode_ = AOO_EVENT_NONE;
    // requests
    lockfree::unbounded_mpsc_queue<source_request, aoo::allocator<source_request>> requestqueue_;
    void push_request(const source_request& r){
        requestqueue_.push(r);
    }
public:
    // memory
    mutable memory_list memory;
private:
    // helper methods

    source_desc *find_source(const ip_address& addr, aoo_id id);

    source_desc *get_source_arg(intptr_t index);

    source_desc *add_source(const ip_address& addr, aoo_id id, uint32_t flags=0);

    void reset_sources();

    aoo_error handle_format_message(const osc::ReceivedMessage& msg,
                                    const ip_address& addr);

    aoo_error handle_data_message(const osc::ReceivedMessage& msg,
                                  const ip_address& addr);

    aoo_error handle_data_message(const char *msg, int32_t n,
                                  const ip_address& addr);

    aoo_error handle_data_packet(net_packet& d, bool binary,
                                 const ip_address& addr, aoo_id id);

    aoo_error handle_ping_message(const osc::ReceivedMessage& msg,
                                  const ip_address& addr);
};

} // aoo
