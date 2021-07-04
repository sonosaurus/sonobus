#pragma once

#include "aoo/aoo.h"

#include "imp.hpp"

#include <stdint.h>
#include <vector>
#include <bitset>

namespace aoo {

struct data_packet {
    int32_t sequence;
    int32_t channel;
    int32_t totalsize;
    int32_t nframes;
    int32_t frame;
    int32_t size;
    const char *data;
    double samplerate;
};

/*///////////////////// history_buffer /////////////////////////*/

class sent_block {
public:
    // methods
    void set(int32_t seq, double sr,
             const char *data, int32_t nbytes,
             int32_t nframes, int32_t framesize);

    const char* data() const { return buffer_.data(); }
    int32_t size() const { return buffer_.size(); }

    int32_t num_frames() const { return numframes_; }
    int32_t frame_size(int32_t which) const;
    int32_t get_frame(int32_t which, char * data, int32_t n);

    // data
    int32_t sequence = -1;
    double samplerate = 0;
protected:
    std::vector<char, aoo::allocator<char>> buffer_;
    int32_t numframes_ = 0;
    int32_t framesize_ = 0;
};

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
    using block_buffer = std::vector<sent_block, aoo::allocator<sent_block>>;
    block_buffer buffer_;
    int32_t head_ = 0;
    int32_t size_ = 0;
};

/*///////////////// jitter_buffer ///////////////////////*/

class received_block {
public:
    void reserve(int32_t size);

    void init(int32_t seq, bool dropped);
    void init(int32_t seq, double sr, int32_t chn,
              int32_t nbytes, int32_t nframes);

    const char* data() const { return buffer_.data(); }
    int32_t size() const { return buffer_.size(); }

    int32_t num_frames() const { return numframes_; }
    bool has_frame(int32_t which) const;
    int32_t count_frames() const;
    void add_frame(int32_t which, const char *data, int32_t n);

    int32_t resend_count() const;
    bool dropped() const;
    bool complete() const;
    bool update(double time, double interval);

    // data
    int32_t sequence = -1;
    int32_t channel = 0;
    double samplerate = 0;
protected:
    std::vector<char, aoo::allocator<char>> buffer_;
    int32_t numframes_ = 0;
    int32_t framesize_ = 0;
    std::bitset<256> frames_ = 0;
    double timestamp_ = 0;
    int32_t numtries_ = 0;
    bool dropped_ = false;
};

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
    received_block* push_back(int32_t seq);
    void pop_front();

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
    std::vector<received_block, aoo::allocator<received_block>> data_;
    int32_t size_ = 0;
    int32_t head_ = 0;
    int32_t tail_ = 0;
    int32_t last_pushed_ = -1;
    int32_t last_popped_ = -1;
};

} // aoo
