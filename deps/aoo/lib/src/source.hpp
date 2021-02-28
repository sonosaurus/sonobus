/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others. 
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#pragma once

#include "aoo/aoo.hpp"

#include "time.hpp"
#include "sync.hpp"
#include "common.hpp"
#include "lockfree.hpp"
#include "time_dll.hpp"

#include "oscpack/osc/OscOutboundPacketStream.h"
#include "oscpack/osc/OscReceivedElements.h"

namespace aoo {

struct endpoint {
    endpoint() = default;
    endpoint(void *_user, aoo_replyfn _fn, int32_t _id)
      : user(_user), fn(_fn), id(_id){}

    // data
    void *user = nullptr;
    aoo_replyfn fn = nullptr;
    int32_t id = 0;
    
    // methods
    void send_data(int32_t src, int32_t salt, const data_packet& data) const;
    void send_data_compact(int32_t src, int32_t salt, const data_packet& data, bool sendrate=false);

    void send_format(int32_t src, int32_t salt, const aoo_format& f,
                     const char *options, int32_t size, const char * userformat = nullptr, int32_t ufsize=0) const;

    void send_ping(int32_t src, time_tag t) const;

    void send(const char *data, int32_t n) const {
        fn(user, data, n);
    }
    

};

struct data_request : endpoint {
    data_request() = default;
    data_request(void *_user, aoo_replyfn _fn, int32_t _id,
                 int32_t _salt, int32_t _sequence, int32_t _frame)
        : endpoint(_user, _fn, _id),
          salt(_salt), sequence(_sequence), frame(_frame){}
    int32_t salt = 0;
    int32_t sequence = 0;
    int32_t frame = 0;
};

struct invite_request : endpoint {
    enum type {
        INVITE,
        UNINVITE
    };

    invite_request() = default;
    invite_request(void *_user, aoo_replyfn _fn, int32_t _id, int32_t _type)
        : endpoint(_user, _fn, _id), type(_type){}
    int32_t type = 0;
};

struct sink_desc : endpoint {
    sink_desc(void *_user, aoo_replyfn _fn, int32_t _id)
        : endpoint(_user, _fn, _id), channel(0), format_changed(true), protocol_flags(0) {}
    sink_desc(const sink_desc& other)
        : endpoint(other.user, other.fn, other.id),
          channel(other.channel.load()),
          format_changed(other.format_changed.load()),
          protocol_flags(other.protocol_flags.load()){}
    sink_desc& operator=(const sink_desc& other){
        user = other.user;
        fn = other.fn;
        id = other.id;
        channel = other.channel.load();
        format_changed = other.format_changed.load();
        protocol_flags = other.protocol_flags.load();
        return *this;
    }

    // data
    std::atomic<int16_t> channel;
    std::atomic<bool> format_changed;
    std::atomic<int8_t> protocol_flags;

};

class source final : public isource {
 public:
    typedef union event
    {
        aoo_event_type type;
        aoo_sink_event sink;
        aoo_ping_event ping;
    } event;

    source(int32_t id);
    ~source();

    int32_t id() const { return id_.load(std::memory_order_relaxed); }

    int32_t setup(int32_t samplerate, int32_t blocksize, int32_t nchannels) override;

    int32_t add_sink(void *sink, int32_t id, aoo_replyfn fn) override;

    int32_t remove_sink(void *sink, int32_t id) override;

    void remove_all() override;

    int32_t handle_message(const char *data, int32_t n, void *endpoint, aoo_replyfn fn) override;

    int32_t send() override;

    int32_t process(const aoo_sample **data, int32_t n, uint64_t t) override;

    int32_t events_available() override;

    int32_t handle_events(aoo_eventhandler fn, void *user) override;

    int32_t set_option(int32_t opt, void *ptr, int32_t size) override;

    int32_t get_option(int32_t opt, void *ptr, int32_t size) override;

    int32_t set_sinkoption(void *endpoint, int32_t id,
                           int32_t opt, void *ptr, int32_t size) override;

    int32_t get_sinkoption(void *endpoint, int32_t id,
                           int32_t opt, void *ptr, int32_t size) override;
    
    int32_t protocol_flags() const { return protocol_flags_; }

 private:
    // settings
    std::atomic<int32_t> id_;
    int32_t salt_ = 0;
    int32_t nchannels_ = 0;
    int32_t blocksize_ = 0;
    int32_t maxblocksize_ = 0;
    int32_t samplerate_ = 0;
    // audio encoder
    std::unique_ptr<encoder> encoder_;
    // state
    int32_t sequence_ = 0;
    std::atomic<int32_t> dropped_{0};
    std::atomic<float> lastpingtime_{0};
    std::atomic<bool> format_changed_{false};
    std::atomic<bool> play_{false};
    // timing
    time_dll dll_;
    timer timer_;
    // buffers and queues
    std::vector<char> sendbuffer_;
    dynamic_resampler resampler_;
    lockfree::queue<aoo_sample> audioqueue_;
    lockfree::queue<double> srqueue_;
    lockfree::queue<event> eventqueue_;
    lockfree::queue<endpoint> formatrequestqueue_;
    lockfree::queue<data_request> datarequestqueue_;
    history_buffer history_;
    // sinks
    std::vector<sink_desc> sinks_;
    // thread synchronization
    aoo::shared_mutex update_mutex_;
    aoo::shared_mutex sink_mutex_;
    // options
    std::atomic<int32_t> buffersize_{ AOO_SOURCE_BUFSIZE };
    std::atomic<int32_t> packetsize_{ AOO_PACKETSIZE };
    std::atomic<int32_t> resend_buffersize_{ AOO_RESEND_BUFSIZE };
    std::atomic<int32_t> redundancy_{ AOO_SEND_REDUNDANCY };
    std::atomic<int32_t> dynamic_resampling_{ 1 };
    std::atomic<float> bandwidth_{ AOO_TIMEFILTER_BANDWIDTH };
    std::atomic<float> ping_interval_{ AOO_PING_INTERVAL * 0.001 };
    std::atomic<int32_t> protocol_flags_{ 0 };
    std::atomic<int32_t> respect_codec_change_req_{ 0 };
    std::vector<char> userformat_;
    // runtime
    double prev_sent_samplerate_ = 0.0;
    std::atomic<int32_t> activeplay_ { 0 };
    std::atomic<int32_t> flushingout_ { 0 };
    bool lastplay_ = false;
    int32_t pushing_silent_frames_ = 0;
    
    // helper methods
    sink_desc * find_sink(void *endpoint, int32_t id);

    int32_t set_format(aoo_format& f);
    int32_t set_userformat(void * ptr, int32_t size);

    int32_t make_salt();

    void update();

    void update_historybuffer();

    bool send_format();

    bool send_data();

    bool resend_data();

    bool send_ping();

    void handle_format_request(void *endpoint, aoo_replyfn fn,
                               const osc::ReceivedMessage& msg);

    void handle_data_request(void *endpoint, aoo_replyfn fn,
                             const osc::ReceivedMessage& msg);

    void handle_ping(void *endpoint, aoo_replyfn fn,
                     const osc::ReceivedMessage& msg);

    void handle_invite(void *endpoint, aoo_replyfn fn,
                       const osc::ReceivedMessage& msg);

    void handle_uninvite(void *endpoint, aoo_replyfn fn,
                         const osc::ReceivedMessage& msg);
    
    void handle_codec_change(void *endpoint, aoo_replyfn fn,
                               const osc::ReceivedMessage& msg);
};

} // aoo
