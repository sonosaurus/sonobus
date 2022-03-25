/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others. 
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#pragma once

#include "aoo/aoo_source.hpp"
#if USE_AOO_NET
# include "aoo/aoo_client.hpp"
#endif

#include "common/lockfree.hpp"
#include "common/net_utils.hpp"
#include "common/sync.hpp"
#include "common/time.hpp"
#include "common/utils.hpp"

#include "binmsg.hpp"
#include "buffer.hpp"
#include "imp.hpp"
#include "resampler.hpp"
#include "timer.hpp"
#include "time_dll.hpp"

#include "oscpack/osc/OscOutboundPacketStream.h"
#include "oscpack/osc/OscReceivedElements.h"

#include <list>

namespace aoo {

struct Source;

struct data_request {
    int32_t sequence;
    int32_t frame;
};

enum class request_type {
    none,
    stop
};

struct sink_request {
    sink_request() = default;

    sink_request(request_type _type)
        : type(_type) {}

    sink_request(request_type _type, const endpoint& _ep)
        : type(_type), ep(_ep) {}

    request_type type;
    endpoint ep;
    union {
        struct {
            int32_t stream;
        } stop;
    };
};

// NOTE: the stream ID can change anytime, it only
// has to be synchronized with any format change.
struct sink_desc {
#if USE_AOO_NET
    sink_desc(const ip_address& addr, int32_t id,
              AooId stream_id, const ip_address& relay)
        : ep(addr, id, relay), stream_id_(stream_id) {}
#else
    sink_desc(const ip_address& addr, int32_t id, AooId stream_id)
        : ep(addr, id), stream_id_(stream_id) {}
#endif // USE_AOO_NEt
    sink_desc(const sink_desc& other) = delete;
    sink_desc& operator=(const sink_desc& other) = delete;

    const endpoint ep;

    AooId stream_id() const {
        return stream_id_.load(std::memory_order_acquire);
    }

    void set_channel(int32_t chn){
        channel_.store(chn, std::memory_order_relaxed);
    }

    int32_t channel() const {
        return channel_.load(std::memory_order_relaxed);
    }

    // called while locked
    void start();

    void stop(Source& s);

    void activate(Source& s, bool b);

    bool is_active() const {
        return stream_id_.load(std::memory_order_acquire) != kAooIdInvalid;
    }

    bool need_invite(AooId token);

    void accept_invitation(Source& s, AooId token);

    bool need_uninvite(AooId stream_id);

    void accept_uninvitation(Source& s, AooId token);

    void notify_start() {
        needstart_.exchange(true, std::memory_order_release);
    }

    bool need_start() {
        return needstart_.exchange(false, std::memory_order_acquire);
    }

    void add_data_request(int32_t sequence, int32_t frame){
        data_requests_.push(sequence, frame);
    }

    bool get_data_request(data_request& r){
        return data_requests_.try_pop(r);
    }
private:
    std::atomic<int32_t> channel_{0};
    std::atomic<int32_t> stream_id_ {kAooIdInvalid};
    int32_t invite_token_{kAooIdInvalid};
    int32_t uninvite_token_{kAooIdInvalid};
    std::atomic<bool> needstart_{false};
    aoo::unbounded_mpsc_queue<data_request> data_requests_;
};

struct cached_sink {
    cached_sink(const sink_desc& s)
        : ep(s.ep), stream_id(s.stream_id()), channel(s.channel()) {}

    endpoint ep;
    AooId stream_id;
    int32_t channel;
};

class Source final : public AooSource {
 public:
    Source(AooId id, AooFlag flags, AooError *err);

    ~Source();

    //--------------------- interface methods --------------------------//

    AooError AOO_CALL setup(AooSampleRate sampleRate,
                            AooInt32 blockSize, AooInt32 numChannels) override;

    AooError AOO_CALL handleMessage(const AooByte *data, AooInt32 n,
                                    const void *address, AooAddrSize addrlen) override;

    AooError AOO_CALL send(AooSendFunc fn, void *user) override;

    AooError AOO_CALL process(AooSample **data, AooInt32 n,
                              AooNtpTime t) override;

    AooError AOO_CALL setEventHandler(AooEventHandler fn, void *user,
                                      AooEventMode mode) override;

    AooBool AOO_CALL eventsAvailable() override;

    AooError AOO_CALL pollEvents() override;

    AooError AOO_CALL startStream(const AooDataView *metadata) override;

    AooError AOO_CALL stopStream() override;

    AooError AOO_CALL addSink(const AooEndpoint& sink, AooFlag flags) override;

    AooError AOO_CALL removeSink(const AooEndpoint& sink) override;

    AooError AOO_CALL removeAll() override;

    AooError AOO_CALL acceptInvitation(const AooEndpoint& sink, AooId token) override;

    AooError AOO_CALL acceptUninvitation(const AooEndpoint& sink, AooId token) override;

    AooError AOO_CALL control(AooCtl ctl, AooIntPtr index,
                              void *ptr, AooSize size) override;

    AooError AOO_CALL codecControl(AooCtl ctl, AooIntPtr index,
                                   void *ptr, AooSize size) override;

    //----------------------- semi-public methods -------------------//

    AooId id() const { return id_.load(); }

    bool is_running() const {
        return state_.load(std::memory_order_acquire) == stream_state::run;
    }

    void notify_start();

    void push_request(const sink_request& r);
 private:
    using shared_lock = sync::shared_lock<sync::shared_mutex>;
    using unique_lock = sync::unique_lock<sync::shared_mutex>;
    using scoped_lock = sync::scoped_lock<sync::shared_mutex>;
    using scoped_shared_lock = sync::scoped_shared_lock<sync::shared_mutex>;
    using scoped_spinlock = sync::scoped_lock<sync::spinlock>;

    // settings
    parameter<AooId> id_;
    int32_t nchannels_ = 0;
    int32_t blocksize_ = 0;
    int32_t samplerate_ = 0;
#if USE_AOO_NET
    AooClient *client_ = nullptr;
#endif
    // audio encoder
    std::unique_ptr<AooFormat, format_deleter> format_;
    std::unique_ptr<AooCodec, encoder_deleter> encoder_;
    AooId format_id_ {kAooIdInvalid};
    // state
    int32_t sequence_ = 0;
    std::atomic<float> xrun_{0};
    std::atomic<float> lastpingtime_{0};
    std::atomic<bool> needstart_{false};
    enum class stream_state {
        stop,
        start,
        run,
        idle
    };
    std::atomic<stream_state> state_{stream_state::idle};
    // metadata
    AooDataView *metadata_{nullptr};
    std::atomic<int32_t> metadata_size_{ AOO_STREAM_METADATA_SIZE };
    bool metadata_accepted_{false};
    sync::spinlock metadata_lock_;
    // timing
    parameter<AooSampleRate> realsr_{0};
    time_dll dll_;
    timer timer_;
    // buffers and queues
    aoo::vector<AooByte> sendbuffer_;
    dynamic_resampler resampler_;
    struct block_data {
        double sr;
        AooSample data[1];
    };
    aoo::spsc_queue<char> audioqueue_;
    history_buffer history_;
    // events
    aoo::unbounded_mpsc_queue<endpoint_event> eventqueue_;
    AooEventHandler eventhandler_ = nullptr;
    void *eventcontext_ = nullptr;
    AooEventMode eventmode_ = kAooEventModeNone;
    // requests
    aoo::unbounded_mpsc_queue<sink_request> requests_;
    // sinks
    using sink_list = aoo::rcu_list<sink_desc>;
    using sink_lock = std::unique_lock<sink_list>;
    sink_list sinks_;
    sync::mutex sink_mutex_;
    aoo::vector<cached_sink> cached_sinks_; // only for the send thread
    // memory
    memory_list memory_;
    // thread synchronization
    sync::shared_mutex update_mutex_;
    // options
    parameter<float> buffersize_{ AOO_SOURCE_BUFFER_SIZE };
    parameter<float> resend_buffersize_{ AOO_RESEND_BUFFER_SIZE };
    parameter<int32_t> packetsize_{ AOO_PACKET_SIZE };
    parameter<int32_t> redundancy_{ AOO_SEND_REDUNDANCY };
    parameter<float> ping_interval_{ AOO_PING_INTERVAL };
    parameter<float> dll_bandwidth_{ AOO_DLL_BANDWIDTH };
    parameter<bool> dynamic_resampling_{ AOO_DYNAMIC_RESAMPLING };
    parameter<bool> timer_check_{ AOO_XRUN_DETECTION };
    parameter<bool> binary_{ AOO_BINARY_DATA_MSG };

    // helper methods
    sink_desc * do_add_sink(const ip_address& addr, AooId id, AooId stream_id);

    bool do_remove_sink(const ip_address& addr, AooId id);

    AooError set_format(AooFormat& fmt);

    AooError get_format(AooFormat& fmt);

    sink_desc * find_sink(const ip_address& addr, AooId id);

    sink_desc *get_sink_arg(intptr_t index);

    void send_event(const endpoint_event& e, AooThreadLevel level);

    void free_event(const endpoint_event& e);

    bool need_resampling() const;

    void make_new_stream();

    void allocate_metadata(int32_t size);

    void add_xrun(float n);

    void update_audioqueue();

    void update_resampler();

    void update_historybuffer();

    void dispatch_requests(const sendfn& fn);

    void send_start(const sendfn& fn);

    void send_data(const sendfn& fn);

    void resend_data(const sendfn& fn);

    void send_ping(const sendfn& fn);

    void handle_start_request(const osc::ReceivedMessage& msg,
                              const ip_address& addr);

    void handle_data_request(const osc::ReceivedMessage& msg,
                             const ip_address& addr);

    void handle_data_request(const AooByte * msg, int32_t n,
                             AooId id, const ip_address& addr);

    void handle_ping(const osc::ReceivedMessage& msg,
                     const ip_address& addr);

    void handle_invite(const osc::ReceivedMessage& msg,
                       const ip_address& addr);

    void handle_uninvite(const osc::ReceivedMessage& msg,
                         const ip_address& addr);
};

} // aoo
