/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others. 
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#pragma once

#include "aoo/aoo.hpp"
#include "aoo/aoo_utils.hpp"

#include "time.hpp"
#include "sync.hpp"
#include "common.hpp"
#include "lockfree.hpp"
#include "time_dll.hpp"

#include "oscpack/osc/OscOutboundPacketStream.h"
#include "oscpack/osc/OscReceivedElements.h"

namespace aoo {

struct stream_state {
    stream_state() = default;
    stream_state(stream_state&& other) = delete;
    stream_state& operator=(stream_state&& other) = delete;

    void reset(){
        lost_ = 0;
        reordered_ = 0;
        resent_ = 0;
        gap_ = 0;
        state_ = AOO_SOURCE_STATE_STOP;
        underrun_ = false;
        recover_ = false;
        format_ = false;
        invite_ = NONE;
        pingtime1_ = 0;
        pingtime2_ = 0;
        codecchange_ = false;
    }

    void add_lost(int32_t n) { lost_ += n; lost_since_ping_ += n; }
    int32_t get_lost() { return lost_.exchange(0); }
    int32_t get_lost_since_ping() { return lost_since_ping_.exchange(0); }

    void add_reordered(int32_t n) { reordered_ += n; }
    int32_t get_reordered() { return reordered_.exchange(0); }

    void add_resent(int32_t n) { resent_ += n; }
    int32_t get_resent() { return resent_.exchange(0); }

    void add_gap(int32_t n) { gap_ += n; }
    int32_t get_gap() { return gap_.exchange(0); }

    bool update_state(aoo_source_state state){
        auto last = state_.exchange(state);
        return state != last;
    }
    aoo_source_state get_state(){
        return state_;
    }

    void set_ping(time_tag t1, time_tag t2){
        pingtime1_ = t1.to_uint64();
        pingtime2_ = t2.to_uint64();
    }

    bool need_ping(time_tag& t1, time_tag& t2){
        // check pingtime2 because it ensures that pingtime1 has been set
        auto pingtime2 = pingtime2_.exchange(0);
        if (pingtime2){
            t1 = pingtime1_.load();
            t2 = pingtime2;
            return true;
        } else {
            return false;
        }
    }

    void set_underrun() { underrun_ = true; }
    bool have_underrun() { return underrun_.exchange(false); }

    void request_recover() { recover_ = true; }
    bool need_recover() { return recover_.exchange(false); }

    void request_format() { format_ = true; }
    bool need_format() { return format_.exchange(false); }

    void request_codec_change(const aoo_format& f, const char *options, int32_t size) { 
        memcpy(&codecchange_format_.header, &f, sizeof(aoo_format));
        memcpy(codecchange_format_.data, options, size);
        codecchange_datasize_ = size;
        codecchange_ = true;           
    }
    bool need_codec_change() { return codecchange_.exchange(false); }

    aoo_format_storage &  get_codec_change_format(int32_t & datasize) { datasize = codecchange_datasize_; return codecchange_format_; }
    
    enum invitation_state {
        NONE = 0,
        INVITE = 1,
        UNINVITE = 2,
    };

    void request_invitation(invitation_state state) { invite_ = state; }
    invitation_state get_invitation_state() { return invite_.exchange(NONE); }
private:
    std::atomic<int32_t> lost_since_ping_{0};
    std::atomic<int32_t> lost_{0};
    std::atomic<int32_t> reordered_{0};
    std::atomic<int32_t> resent_{0};
    std::atomic<int32_t> gap_{0};
    std::atomic<aoo_source_state> state_{AOO_SOURCE_STATE_STOP};
    std::atomic<invitation_state> invite_{NONE};
    std::atomic<bool> underrun_{false};
    std::atomic<bool> recover_{false};
    std::atomic<bool> format_{false};
    std::atomic<bool> codecchange_{false};
    std::atomic<uint64_t> pingtime1_;
    std::atomic<uint64_t> pingtime2_;
    
    aoo_format_storage codecchange_format_;
    int32_t codecchange_datasize_ = 0;
};

struct block_info {
    double sr;
    int32_t channel;
};

class sink;

class source_desc {
public:
    typedef union event
    {
        aoo_event_type type;
        aoo_source_event source;
        aoo_ping_event ping;
        aoo_source_state_event source_state;
        aoo_block_lost_event block_loss;
        aoo_block_reordered_event block_reorder;
        aoo_block_resent_event block_resend;
        aoo_block_gap_event block_gap;
    } event;

    source_desc(void *endpoint, aoo_replyfn fn, int32_t id, int32_t salt);
    source_desc(const source_desc& other) = delete;
    source_desc& operator=(const source_desc& other) = delete;

    // getters
    int32_t id() const { return id_; }

    void *endpoint() const { return endpoint_; }

    bool has_events() const { return  eventqueue_.read_available() > 0; }

    int32_t get_format(aoo_format_storage& format);
    
    int32_t get_buffer_fill_ratio(float &ratio);

    int32_t get_userformat(char * buf, int32_t size);

    int32_t get_current_salt() const { return salt_; }
    
    void set_protocol_flags(int32_t flags) { protocol_flags_ = flags; }
    
    // methods
    void update(const sink& s);

    int32_t handle_format(const sink& s, int32_t salt, const aoo_format& f,
                          const char *settings, int32_t size, int32_t version, const char *userformat=nullptr, int32_t ufsize=0);

    int32_t handle_data(const sink& s, int32_t salt,
                                     const aoo::data_packet& d);

    int32_t handle_ping(const sink& s, time_tag tt);

    int32_t handle_events(aoo_eventhandler fn, void *user);

    bool send(const sink& s);

    bool process(const sink& s, aoo_sample *buffer, int32_t stride, int32_t numsampleframes);

    void request_recover(){ streamstate_.request_recover(); }

    void request_format(){ streamstate_.request_format(); }

    void request_codec_change(aoo_format & f);

    void request_invite(){ streamstate_.request_invitation(stream_state::INVITE); }

    void request_uninvite(){ streamstate_.request_invitation(stream_state::UNINVITE); }
private:
    struct data_request {
        int32_t sequence;
        int32_t frame;
    };
    void do_update(const sink& s);
    // handle messages
    bool check_packet(const data_packet& d);

    bool add_packet(const data_packet& d);

    void process_blocks();

    void check_outdated_blocks();

    void check_missing_blocks(const sink& s);
    // send messages
    bool send_format_request(const sink& s);
    bool send_codec_change_request(const sink& s);

    int32_t send_data_request(const sink& s);

    bool send_notifications(const sink& s);

    void dosend(const char *data, int32_t n){
        fn_(endpoint_, data, n);
    }
    // data
    void * const endpoint_;
    const aoo_replyfn fn_;
    const int32_t id_;
    int32_t salt_;
    // audio decoder
    std::unique_ptr<aoo::decoder> decoder_;
    // state
    int32_t newest_ = 0; // sequence number of most recent incoming block
    int32_t next_ = 0; // next outgoing block
    int32_t nextneedsfadein_ = -1; // sequence number that needs fadein
    int32_t channel_ = 0; // recent channel onset
    double samplerate_ = 0; // recent samplerate
    int32_t protocol_flags_ = 0; // protocol flags sent from the remote source
    stream_state streamstate_;
    std::vector<char> userformat_;
    // queues and buffers
    block_queue blockqueue_;
    block_ack_list ack_list_;
    lockfree::queue<aoo_sample> audioqueue_;
    lockfree::queue<block_info> infoqueue_;
    lockfree::queue<data_request> resendqueue_;
    lockfree::queue<event> eventqueue_;
    spinlock eventqueuelock_;
    void push_event(const event& e){
        scoped_lock<spinlock> l(eventqueuelock_);
        if (eventqueue_.write_available()){
            eventqueue_.write(e);
        }
    }
    dynamic_resampler resampler_;
    // thread synchronization
    aoo::shared_mutex mutex_; // LATER replace with a spinlock?
};

class sink final : public isink {
public:
    sink(int32_t id)
        : id_(id) {}

    ~sink(){}

    int32_t setup(int32_t samplerate, int32_t blocksize, int32_t nchannels) override;

    int32_t invite_source(void *endpoint, int32_t id, aoo_replyfn fn) override;

    int32_t uninvite_source(void *endpoint, int32_t id, aoo_replyfn fn) override;
                             
    int32_t uninvite_all() override;

    int32_t handle_message(const char *data, int32_t n,
                           void *endpoint, aoo_replyfn fn) override;

    int32_t send() override;

    int32_t process(aoo_sample **data, int32_t nsampframes, uint64_t t) override;

    int32_t events_available() override;

    int32_t handle_events(aoo_eventhandler fn, void *user) override;

    int32_t set_option(int32_t opt, void *ptr, int32_t size) override;

    int32_t get_option(int32_t opt, void *ptr, int32_t size) override;

    int32_t set_sourceoption(void *endpoint, int32_t id,
                             int32_t opt, void *ptr, int32_t size) override;

    int32_t get_sourceoption(void *endpoint, int32_t id,
                             int32_t opt, void *ptr, int32_t size) override;
                             
    int32_t request_source_codec_change(void *endpoint, int32_t id, aoo_format & f) override;
                             

    // getters
    int32_t id() const { return id_.load(std::memory_order_relaxed); }

    int32_t nchannels() const { return nchannels_; }

    int32_t samplerate() const { return samplerate_; }

    double real_samplerate() const { return ignore_dll_ ? samplerate_ : dll_.samplerate(); }

    int32_t blocksize() const { return blocksize_; }

    int32_t buffersize() const { return buffersize_; }

    int32_t packetsize() const { return packetsize_; }

    float resend_interval() const { return resend_interval_; }

    int32_t resend_limit() const { return resend_limit_; }

    int32_t resend_maxnumframes() const { return resend_maxnumframes_; }

    double elapsed_time() const { return timer_.get_elapsed(); }

    time_tag absolute_time() const { return timer_.get_absolute(); }

    int32_t protocol_flags() const { return protocol_flags_; }

private:
    // settings
    std::atomic<int32_t> id_;
    int32_t nchannels_ = 0;
    int32_t samplerate_ = 0;
    int32_t blocksize_ = 0;
    // buffer for summing source audio output
    std::vector<aoo_sample> buffer_;
    // options
    std::atomic<int32_t> buffersize_{ AOO_SINK_BUFSIZE };
    std::atomic<int32_t> packetsize_{ AOO_PACKETSIZE };
    std::atomic<int32_t> resend_limit_{ AOO_RESEND_LIMIT };
    std::atomic<float> resend_interval_{ AOO_RESEND_INTERVAL * 0.001 };
    std::atomic<int32_t> resend_maxnumframes_{ AOO_RESEND_MAXNUMFRAMES };
    std::atomic<int32_t> protocol_flags_{ 0 };
    // the sources
    lockfree::list<source_desc> sources_;
    // timing
    std::atomic<int32_t> dynamic_resampling_{ 1 };
    std::atomic<float> bandwidth_{ AOO_TIMEFILTER_BANDWIDTH };
    time_dll dll_;
    bool ignore_dll_ = false;
    timer timer_;
    // helper methods
    source_desc *find_source(void *endpoint, int32_t id);
    source_desc *find_source_by_salt(void *endpoint, int32_t salt);

    void update_sources();

    int32_t handle_format_message(void *endpoint, aoo_replyfn fn,
                                  const osc::ReceivedMessage& msg);

    int32_t handle_data_message(void *endpoint, aoo_replyfn fn,
                                const osc::ReceivedMessage& msg);

    int32_t handle_compact_data_message(void *endpoint, aoo_replyfn fn,
                                        const osc::ReceivedMessage& msg);

    int32_t handle_ping_message(void *endpoint, aoo_replyfn fn,
                                const osc::ReceivedMessage& msg);
};

} // aoo
