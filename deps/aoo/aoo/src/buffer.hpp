#pragma once

#include "aoo/aoo.h"

#include "imp.hpp"

#include <stdint.h>
#include <vector>
#include <bitset>
#include <list>

namespace aoo {

struct data_packet {
    int32_t sequence;
    int32_t channel;
    int32_t totalsize;
    int32_t nframes;
    int32_t frame;
    int32_t size;
    const AooByte *data;
    double samplerate;
};

//---------------------- sent_block ---------------------------//

class sent_block {
public:
    // methods
    void set(int32_t seq, double sr,
             const AooByte *data, int32_t nbytes,
             int32_t nframes, int32_t framesize);

    const AooByte* data() const { return buffer_.data(); }
    int32_t size() const { return buffer_.size(); }

    int32_t num_frames() const { return numframes_; }
    int32_t frame_size(int32_t which) const;
    int32_t get_frame(int32_t which, AooByte * data, int32_t n);

    // data
    int32_t sequence = -1;
    double samplerate = 0;
protected:
    aoo::vector<AooByte> buffer_;
    int32_t numframes_ = 0;
    int32_t framesize_ = 0;
};

//---------------------------- history_buffer ------------------------------//

class history_buffer {
public:
    void clear();
    bool empty() const {
        return size_ == 0;
    }
    int32_t size() const {
        return size_;
    }
    int32_t capacity() const {
        return buffer_.size();
    }
    void resize(int32_t n);
    sent_block * find(int32_t seq);
    sent_block * push();
private:
    aoo::vector<sent_block> buffer_;
    int32_t head_ = 0;
    int32_t size_ = 0;
};

//---------------------------- received_block ------------------------------//

// LATER use a (sorted) linked list of data frames (coming from the network thread)
// which are eventually written sequentially into a contiguous client-size buffer.
// This has the advantage that we don't need to preallocate memory and we can easily
// handle arbitrary number of data frames. We can still use the bitset as an optimization,
// but only up to a certain number of frames (e.g. 32); above that we do a linear search
// over the linked list.

class received_block {
public:
    void reserve(int32_t size);

    void init(int32_t seq, bool dropped);
    void init(int32_t seq, double sr, int32_t chn,
              int32_t nbytes, int32_t nframes);

    const AooByte* data() const { return buffer_.data(); }
    int32_t size() const { return buffer_.size(); }

    int32_t num_frames() const { return numframes_; }
    bool has_frame(int32_t which) const {
        return !frames_[which];
    }
    int32_t count_frames() const {
        return std::max<int32_t>(0, numframes_ - frames_.count());
    }
    void add_frame(int32_t which, const AooByte *data, int32_t n);

    int32_t resend_count() const { return numtries_; }
    bool dropped() const { return dropped_; }
    bool complete() const { return frames_.none(); }
    bool update(double time, double interval);

    // data
    int32_t sequence = -1;
    int32_t channel = 0;
    double samplerate = 0;
protected:
    aoo::vector<AooByte> buffer_;
    int32_t numframes_ = 0;
    int32_t framesize_ = 0;
    std::bitset<256> frames_ = 0;
    double timestamp_ = 0;
    int32_t numtries_ = 0;
    bool dropped_ = false;
};

//---------------------------- jitter_buffer ------------------------------//

class jitter_buffer {
public:
    template<typename T, typename U>
    class base_iterator {
        T *data_;
        U *owner_;
    public:
        base_iterator(U* owner)
            : data_(nullptr), owner_(owner){}
        base_iterator(U* owner, T* data)
            : data_(data), owner_(owner){}
        base_iterator(const base_iterator&) = default;
        base_iterator& operator=(const base_iterator&) = default;
        T& operator*() { return *data_; }
        T* operator->() { return data_; }
        base_iterator& operator++() {
            auto begin = owner_->data_.data();
            auto end = begin + owner_->data_.size();
            auto next = data_ + 1;
            if (next == end){
                next = begin;
            }
            if (next == (begin + owner_->head_)){
                next = nullptr; // sentinel
            }
            data_ = next;
            return *this;
        }
        base_iterator operator++(int) {
            base_iterator old = *this;
            operator++();
            return old;
        }
        bool operator==(const base_iterator& other){
            return data_ == other.data_;
        }
        bool operator!=(const base_iterator& other){
            return data_ != other.data_;
        }
    };

    using iterator = base_iterator<received_block, jitter_buffer>;
    using const_iterator = base_iterator<const received_block, const jitter_buffer>;

    void clear();

    void resize(int32_t n, int32_t maxblocksize);

    bool empty() const {
        return size_ == 0;
    }
    bool full() const {
        return size_ == capacity();
    }
    int32_t size() const {
        return size_;
    }
    int32_t capacity() const {
        return data_.size();
    }

    received_block* find(int32_t seq);
    received_block* push(int32_t seq);
    void pop();

    int32_t last_pushed() const {
        return last_pushed_;
    }
    int32_t last_popped() const {
        return last_popped_;
    }

    received_block& front();
    const received_block& front() const;
    received_block& back();
    const received_block& back() const;

    iterator begin();
    const_iterator begin() const;
    iterator end();
    const_iterator end() const;

    friend std::ostream& operator<<(std::ostream& os, const jitter_buffer& b);
private:
    aoo::vector<received_block> data_;
    int32_t size_ = 0;
    int32_t head_ = 0;
    int32_t tail_ = 0;
    int32_t last_pushed_ = -1;
    int32_t last_popped_ = -1;
};

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
    void set_info(const char *type, time_tag tt) {
        type_ = type;
        tt_ = tt;
    }
    // methods
    const char *type() const { return type_.c_str(); }
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
    aoo::string type_;
    aoo::vector<AooByte> buffer_;
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

} // aoo
