#pragma once

#include "detail.hpp"

#include <bitset>
#include <list>

namespace aoo {
namespace net {

//-------------------------- sent_message -----------------------------//

struct sent_message {
    sent_message(const metadata& data, aoo::time_tag tt, int32_t seq,
                 int32_t nframes, int32_t framesize, float interval)
        : data_(data), tt_(tt), sequence_(seq), nframes_(nframes),
          framesize_(framesize), interval_(interval) {
        // LATER support messages with arbitrary number of frames
        assert(nframes <= (int32_t)frames_.size());
        for (int i = 0; i < nframes; ++i){
            frames_[i] = true;
        }
    }
    // methods
    bool need_resend(double now);
    bool has_frame(int32_t which) const {
        return !frames_[which];
    }
    void get_frame(int32_t which, const AooByte *& data, int32_t& size) {
        if (nframes_ == 1) {
            // single-frame message
            data = data_.data();
            size = data_.size();
        } else {
            // multi-frame message
            if (which == (nframes_ - 1)) {
                // last frame
                auto onset = (which - 1) * framesize_;
                data = data_.data() + onset;
                size = data_.size() - onset;
            } else {
                data = data_.data() + which * framesize_;
                size = framesize_;
            }
        }
    }
    void ack_frame(int32_t which) {
        frames_[which] = false;
    }
    void ack_all() {
        frames_.reset();
    }
    bool complete() const { return frames_.none(); }
    // data
    aoo::metadata data_;
    aoo::time_tag tt_;
    int32_t sequence_;
    int32_t nframes_;
    int32_t framesize_;
    int32_t remainder_;
private:
    double time_ = 0;
    double interval_;
    std::bitset<256> frames_;
};

//-------------------------- message_send_buffer -----------------------------//

class message_send_buffer {
public:
    message_send_buffer() = default;

    using iterator = std::list<sent_message>::iterator;
    using const_iterator = std::list<sent_message>::const_iterator;

    iterator begin() {
        return data_.begin();
    }
    const_iterator begin() const {
        return data_.begin();
    }
    iterator end() {
        return data_.end();
    }
    const_iterator end() const {
        return data_.end();
    }

    sent_message& front() {
        return data_.front();
    }
    const sent_message& front() const {
        return data_.front();
    }
    sent_message& back() {
        return data_.back();
    }
    const sent_message& back() const {
        return data_.back();
    }

    bool empty() const { return data_.empty(); }
    int32_t size() const { return data_.size(); }
    sent_message& push(sent_message&& msg);
    void pop();
    sent_message* find(int32_t seq);
private:
    std::list<sent_message> data_;
};

//-------------------------- received_message -----------------------------//

class received_message {
public:
    received_message(int32_t seq = kAooIdInvalid) : sequence_(seq) {
        // so that complete() will return false
        frames_.set();
    }
    void init(int32_t seq, int32_t nframes, int32_t size) {
        sequence_ = seq;
        init(nframes, size);
    }
    void init(int32_t nframes, int32_t size) {
        buffer_.resize(size);
        nframes_ = nframes;
        frames_.reset();
        // LATER support messages with arbitrary number of frames
        assert(nframes <= frames_.size());
        for (int i = 0; i < nframes; ++i){
            frames_[i] = true;
        }
    }
    bool initialized() const {
        return nframes_ > 0;
    }
    void set_info(AooDataType type, time_tag tt) {
        type_ = type;
        tt_ = tt;
    }
    // methods
    AooDataType type() const { return type_; }
    const AooByte* data() const { return buffer_.data(); }
    int32_t size() const { return buffer_.size(); }

    int32_t num_frames() const { return nframes_; }
    bool has_frame(int32_t which) const {
        return !frames_[which];
    }
    void add_frame(int32_t which, const AooByte *data, int32_t n);
    bool complete() const { return frames_.none(); }

    // data
    int32_t sequence_ = -1;
    aoo::time_tag tt_;
protected:
    aoo::vector<AooByte> buffer_;
    AooDataType type_;
    int32_t nframes_ = 0;
    int32_t framesize_ = 0;
    std::bitset<256> frames_ = 0;
};

//-------------------------- message_receive_buffer -----------------------------//

class message_receive_buffer {
public:
    message_receive_buffer() = default;

    using iterator = std::list<received_message>::iterator;
    using const_iterator = std::list<received_message>::const_iterator;

    iterator begin() {
        return data_.begin();
    }
    const_iterator begin() const {
        return data_.begin();
    }
    iterator end() {
        return data_.end();
    }
    const_iterator end() const {
        return data_.end();
    }

    received_message& front() {
        return data_.front();
    }
    const received_message& front() const {
        return data_.front();
    }
    received_message& back() {
        return data_.back();
    }
    const received_message& back() const {
        return data_.back();
    }

    bool empty() const { return data_.empty(); }
    bool full() const { return false; }
    int32_t size() const { return data_.size(); }
    received_message& push(received_message&& msg);
    void pop();
    received_message *find(int32_t seq);

    int32_t last_pushed() const {
        return last_pushed_;
    }
    int32_t last_popped() const {
        return last_popped_;
    }
private:
    std::list<received_message> data_;
    int32_t last_pushed_ = -1;
    int32_t last_popped_ = -1;
};

} // namespace net
} // namespace aoo
