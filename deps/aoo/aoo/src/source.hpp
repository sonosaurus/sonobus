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

#include "codec.hpp"
#include "buffer.hpp"
#include "imp.hpp"
#include "resampler.hpp"
#include "timer.hpp"
#include "time_dll.hpp"

#include "oscpack/osc/OscOutboundPacketStream.h"
#include "oscpack/osc/OscReceivedElements.h"

#include <list>

namespace aoo {

class source;

struct endpoint {
    endpoint() = default;
    endpoint(const ip_address& _address, int32_t _id, uint32_t _flags)
        : address(_address), id(_id), flags(_flags) {}

    // data
    ip_address address;
    aoo_id id = 0;
    uint32_t flags = 0;
};

struct data_request {
    int32_t sequence;
    int32_t frame;
};

struct sink_desc : endpoint {
    sink_desc(const ip_address& _addr, int32_t _id, uint32_t _flags)
        : endpoint(_addr, _id, _flags), channel(0) {}
    sink_desc(const sink_desc& other)
        : endpoint(other.address, other.id, other.flags),
          channel(other.channel.load()), needformat_(other.needformat_.load()) {}
    sink_desc& operator=(const sink_desc& other){
        address = other.address;
        id = other.id;
        flags = other.flags;
        channel = other.channel.load();
        needformat_ = other.needformat_.load();
        return *this;
    }

    // data
    std::atomic<int16_t> channel;
    lockfree::unbounded_mpsc_queue<data_request, aoo::allocator<data_request>> data_requests;

    void request_format(){
        needformat_.store(true, std::memory_order_release);
    }

    bool need_format() {
        return needformat_.exchange(false, std::memory_order_acquire);
    }

    void reset(){
        data_requests.clear();
    }
private:
    std::atomic<bool> needformat_{true}; // !
};

class source_imp final : public source {
 public:
    struct event {
        event() = default;

        event(aoo_event_type type) : type_(type){}

        event(aoo_event_type type, const ip_address& addr, aoo_id id){
            memcpy(&addr_, addr.address(), addr.length());
            sink.type = type;
            sink.ep.address = &addr_;
            sink.ep.addrlen = addr.length();
            sink.ep.id = id;
        }

        event(const event& other){
            memcpy(this, &other, sizeof(event)); // ugh
            if (type_ != AOO_XRUN_EVENT){
                sink.ep.address = &addr_;
            }
        }

        event& operator=(const event& other){
            memcpy(this, &other, sizeof(event)); // ugh
            if (type_ != AOO_XRUN_EVENT){
                sink.ep.address = &addr_;
            }
            return *this;
        }

        union
        {
            aoo_event_type type_;
            aoo_event event_;
            aoo_sink_event sink;
            aoo_ping_event ping;
            aoo_xrun_event xrun;
            aoo_format_event format;
        };
    private:
        char addr_[ip_address::max_length];
    };

    source_imp(aoo_id id, uint32_t flags);

    ~source_imp();

    aoo_id id() const { return id_.load(std::memory_order_relaxed); }

    aoo_error setup(int32_t samplerate, int32_t blocksize, int32_t nchannels) override;

    aoo_error handle_message(const char *data, int32_t n,
                             const void *address, int32_t addrlen) override;

    aoo_error send(aoo_sendfn fn, void *user) override;

    aoo_error process(const aoo_sample **data, int32_t n, uint64_t t) override;

    aoo_error set_eventhandler(aoo_eventhandler fn, void *user, int32_t mode) override;

    aoo_bool events_available() override;

    aoo_error poll_events() override;

    aoo_error control(int32_t ctl, intptr_t index, void *ptr, size_t size) override;
 private:
    using shared_lock = sync::shared_lock<sync::shared_mutex>;
    using unique_lock = sync::unique_lock<sync::shared_mutex>;
    using scoped_lock = sync::scoped_lock<sync::shared_mutex>;
    using scoped_shared_lock = sync::scoped_shared_lock<sync::shared_mutex>;

    // settings
    std::atomic<aoo_id> id_;
    int32_t salt_ = 0;
    int32_t nchannels_ = 0;
    int32_t blocksize_ = 0;
    int32_t samplerate_ = 0;
    // audio encoder
    std::unique_ptr<encoder> encoder_;
    // state
    int32_t sequence_ = 0;
    std::atomic<float> xrun_{0};
    std::atomic<float> lastpingtime_{0};
    std::atomic<bool> needformat_{false};
    enum class stream_state {
        stop,
        start,
        play
    };
    std::atomic<stream_state> state_{stream_state::stop};
    // timing
    std::atomic<double> realsr_{0};
    time_dll dll_;
    timer timer_;
    // buffers and queues
    std::vector<char, aoo::allocator<char>> sendbuffer_;
    dynamic_resampler resampler_;
    struct block_data {
        double sr;
        aoo_sample data[1];
    };
    lockfree::spsc_queue<char, aoo::allocator<char>> audioqueue_;
    history_buffer history_;
    // events
    lockfree::unbounded_mpsc_queue<event, aoo::allocator<event>> eventqueue_;
    aoo_eventhandler eventhandler_ = nullptr;
    void *eventcontext_ = nullptr;
    aoo_event_mode eventmode_ = AOO_EVENT_NONE;
    // sinks
    using sink_list = lockfree::simple_list<sink_desc, aoo::allocator<sink_desc>>;
    using sink_lock = std::unique_lock<sink_list>;
    sink_list sinks_;
    // memory
    memory_list memory_;
    // thread synchronization
    sync::shared_mutex update_mutex_;
    // options
    std::atomic<int32_t> buffersize_{ AOO_SOURCE_BUFFERSIZE };
    std::atomic<int32_t> packetsize_{ AOO_PACKETSIZE };
    std::atomic<int32_t> resend_buffersize_{ AOO_RESEND_BUFFERSIZE };
    std::atomic<int32_t> redundancy_{ AOO_SEND_REDUNDANCY };
    std::atomic<float> dll_bandwidth_{ AOO_DLL_BANDWIDTH };
    std::atomic<float> ping_interval_{ AOO_PING_INTERVAL * 0.001 };
    std::atomic<bool> dynamic_resampling_{ AOO_DYNAMIC_RESAMPLING };
    std::atomic<bool> timer_check_{ AOO_TIMER_CHECK };
    std::atomic<bool> binary_{ AOO_BINARY_DATA_MSG };

    // helper methods
    aoo_error add_sink(const aoo_endpoint& ep, uint32_t flags);

    aoo_error remove_sink(const aoo_endpoint& ep);

    aoo_error set_format(aoo_format& f);

    sink_desc * find_sink(const ip_address& addr, aoo_id id);

    sink_desc *get_sink_arg(intptr_t index);

    static int32_t make_salt();

    void send_event(const event& e, aoo_thread_level level);

    bool need_resampling() const;

    void start_new_stream();

    void add_xrun(float n);

    void update_audioqueue();

    void update_resampler();

    void update_historybuffer();

    void send_format(const sendfn& fn);

    void send_data(const sendfn& fn);

    void resend_data(const sendfn& fn);

    void send_packet(const sendfn& fn, int32_t salt,
                     data_packet& d, bool binary);

    void send_packet_osc(const sendfn& fn, const endpoint& ep,
                         int32_t salt, const data_packet& d) const;

    void send_packet_bin(const sendfn& fn, const endpoint& ep,
                         int32_t salt, const data_packet& d) const;

    void write_bin_data(const endpoint* ep, int32_t salt,
                        const data_packet& d, char *buf, size_t& size) const;

    void send_ping(const sendfn& fn);

    void handle_format_request(const osc::ReceivedMessage& msg,
                               const ip_address& addr);

    void handle_data_request(const osc::ReceivedMessage& msg,
                             const ip_address& addr);

    void handle_data_request(const char * msg, int32_t n,
                             const ip_address& addr);

    void handle_ping(const osc::ReceivedMessage& msg,
                     const ip_address& addr);

    void handle_invite(const osc::ReceivedMessage& msg,
                       const ip_address& addr);

    void handle_uninvite(const osc::ReceivedMessage& msg,
                         const ip_address& addr);
};

} // aoo
