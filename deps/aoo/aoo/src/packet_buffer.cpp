#include "packet_buffer.hpp"

#include "common/utils.hpp"

#include <algorithm>
#include <cassert>

namespace aoo {

//-------------------------- sent_block -----------------------------//

void sent_block::set(const data_packet& d, int32_t framesize)
{
    sequence = d.sequence;
    message_size = d.msgsize;
    samplerate = d.samplerate;
    flags = d.flags;
    numframes_ = d.nframes;
    framesize_ = framesize;
    if (d.totalsize > 0) {
        buffer_.assign(d.data, d.data + d.totalsize);
    } else {
        buffer_.clear();
    }
}

int32_t sent_block::get_frame(int32_t which, AooByte *data, int32_t n) const {
    assert((framesize_ > 0) == (numframes_ > 0));
    assert(which >= 0 && which < numframes_);
    auto onset = which * framesize_;
    auto size = (which == numframes_ - 1) ? buffer_.size() - onset : framesize_;
    if (n >= size){
        auto ptr = buffer_.data() + onset;
        std::copy(ptr, ptr + size, data);
        return size;
    } else {
        LOG_ERROR("sent_block::get_frame(): buffer too small!");
        return 0;
    }
}

int32_t sent_block::frame_size(int32_t which) const {
    assert(which < numframes_);
    if (which == numframes_ - 1){ // last frame
        return size() - which * framesize_;
    } else {
        return framesize_;
    }
}

//-------------------- history_buffer ----------------------//

void history_buffer::clear(){
    head_ = 0;
    size_ = 0;
}

void history_buffer::resize(int32_t n){
    buffer_.resize(n);
#if 1
    buffer_.shrink_to_fit();
#endif
    clear();
}

sent_block * history_buffer::find(int32_t seq){
    // the code below only works if the buffer is not empty!
    if (size_ > 0){
        // check if sequence number is outdated
        // (tail always starts at buffer begin and becomes
        // equal to head once the buffer is full)
        auto head = buffer_.begin() + head_;
        auto tail = (size_ == capacity()) ? head : buffer_.begin();
        if (seq < tail->sequence){
            LOG_DEBUG("history buffer: block " << seq << " too old");
            return nullptr;
        }

    #if 0
        // linear search
        auto end = buffer_.begin() + size_;
        for (auto it = buffer_.begin(); it != end; ++it){
            if (it->sequence == seq){
                return &(*it);
            }
        }
    #else
        // binary search
        auto dofind = [&](auto begin, auto end) -> sent_block * {
            auto result = std::lower_bound(begin, end, seq, [](auto& a, auto& b){
                return a.sequence < b;
            });
            if (result != end && result->sequence == seq){
                return &(*result);
            } else {
                return nullptr;
            }
        };
        if (head != tail){
            // buffer not full, just search range [tail, head]
            auto result = dofind(tail, head);
            if (result){
                return result;
            }
        } else {
            // blocks are always pushed in chronological order,
            // so the ranges [begin, head] and [head, end] will always be sorted.
            auto result = dofind(buffer_.begin(), head);
            if (!result){
                result = dofind(head, buffer_.end());
            }
            if (result){
                return result;
            }
        }
    #endif
    }

    LOG_ERROR("history buffer: couldn't find block " << seq);
    return nullptr;
}

sent_block * history_buffer::push()
{
    assert(!buffer_.empty());
    auto old = head_++;
    if (head_ >= capacity()){
        head_ = 0;
    }
    if (size_ < capacity()){
        ++size_;
    }
    return &buffer_[old];
}

//---------------------- received_block ------------------------//

void received_block::reserve(int32_t size){
    buffer_.reserve(size);
}

void received_block::init(const data_packet& d)
{
    assert((d.totalsize > 0) == (d.nframes > 0));
    // LATER support blocks with arbitrary number of frames
    assert(d.nframes <= (int32_t)frames_.size());
    // keep timestamp and numtries if we're actually reiniting
    if (d.sequence != sequence){
        timestamp_ = 0;
        numtries_ = 0;
    }
    sequence = d.sequence;
    samplerate = d.samplerate;
    message_size = d.msgsize;
    flags = d.flags;
    channel = d.channel;
    buffer_.resize(d.totalsize);
    numframes_ = d.nframes;
    frames_.reset();
    for (int i = 0; i < d.nframes; ++i){
        frames_[i] = true;
    }
}

void received_block::init(int32_t seq)
{
    sequence = seq;
    samplerate = 0;
    message_size = 0;
    channel = 0;
    buffer_.clear();
    numframes_ = -1; // sentinel for placeholder block
    timestamp_ = 0;
    numtries_ = 0;
    frames_.set(); // has_frame() always returns false
}

void received_block::add_frame(int32_t which, const AooByte *data, int32_t n){
    assert(!buffer_.empty());
    assert(numframes_ > 0 && which >= 0 && which < numframes_);
    if (which == numframes_ - 1){
    #if AOO_DEBUG_JITTER_BUFFER
        LOG_ALL("jitter buffer: copy last frame with " << n << " bytes");
    #endif
        std::copy(data, data + n, buffer_.end() - n);
    } else {
    #if AOO_DEBUG_JITTER_BUFFER
        LOG_ALL("jitter buffer: copy frame " << which << " with " << n << " bytes");
    #endif
        std::copy(data, data + n, buffer_.data() + (which * n));
    }
    frames_[which] = false;
}

bool received_block::update(double time, double interval){
    if (timestamp_ > 0 && (time - timestamp_) < interval){
        return false;
    }
    timestamp_ = time;
    numtries_++;
#if AOO_DEBUG_JITTER_BUFFER
    LOG_ALL("jitter buffer: request block " << sequence);
#endif
    return true;
}

//----------------------- jitter_buffer ----------------------//

void jitter_buffer::clear(){
    head_ = tail_ = size_ = 0;
    last_popped_ = last_pushed_ = -1;
}

void jitter_buffer::resize(int32_t n, int32_t maxblocksize){
    data_.resize(n);
#if 1
    data_.shrink_to_fit();
#endif
    for (auto& b : data_){
        b.reserve(maxblocksize);
    }
    clear();
}

received_block* jitter_buffer::find(int32_t seq){
    // first try the end, as we most likely have to complete the most recent block
    if (empty()){
        return nullptr;
    } else if (back().sequence == seq){
        return &back();
    }
#if 0
    // linear search
    if (head_ > tail_){
        for (int32_t i = tail_; i < head_; ++i){
            if (data_[i].sequence == seq){
                return &data_[i];
            }
        }
    } else {
        for (int32_t i = 0; i < head_; ++i){
            if (data_[i].sequence == seq){
                return &data_[i];
            }
        }
        for (int32_t i = tail_; i < capacity(); ++i){
            if (data_[i].sequence == seq){
                return &data_[i];
            }
        }
    }
    return nullptr;
#else
    // binary search
    // (blocks are always pushed in chronological order)
    auto dofind = [&](auto begin, auto end) -> received_block * {
        auto result = std::lower_bound(begin, end, seq, [](auto& a, auto& b){
            return a.sequence < b;
        });
        if (result != end && result->sequence == seq){
            return &(*result);
        } else {
            return nullptr;
        }
    };

    auto begin = data_.data();
    if (head_ > tail_){
        // [tail, head]
        return dofind(begin + tail_, begin + head_);
    } else {
        // [begin, head] + [tail, end]
        auto result = dofind(begin, begin + head_);
        if (!result){
            result = dofind(begin + tail_, begin + data_.capacity());
        }
        return result;
    }
#endif
}

received_block* jitter_buffer::push(int32_t seq){
    assert(!full());
    auto old = head_;
    if (++head_ == capacity()){
        head_ = 0;
    }
    size_++;
    assert((last_pushed_ == -1) || ((seq - last_pushed_) == 1));
    last_pushed_ = seq;
    return &data_[old];
}

void jitter_buffer::pop(){
    assert(!empty());
    last_popped_ = data_[tail_].sequence;
    if (++tail_ == capacity()){
        tail_ = 0;
    }
    size_--;
}

received_block& jitter_buffer::front(){
    assert(!empty());
    return data_[tail_];
}

const received_block& jitter_buffer::front() const {
    assert(!empty());
    return data_[tail_];
}

received_block& jitter_buffer::back(){
    assert(!empty());
    auto index = head_ - 1;
    if (index < 0){
        index = capacity() - 1;
    }
    return data_[index];
}

const received_block& jitter_buffer::back() const {
    assert(!empty());
    auto index = head_ - 1;
    if (index < 0){
        index = capacity() - 1;
    }
    return data_[index];
}

jitter_buffer::iterator jitter_buffer::begin(){
    if (empty()){
        return end();
    } else {
        return iterator(this, &data_[tail_]);
    }
}

jitter_buffer::const_iterator jitter_buffer::begin() const {
    if (empty()){
        return end();
    } else {
        return const_iterator(this, &data_[tail_]);
    }
}

jitter_buffer::iterator jitter_buffer::end(){
    return iterator(this);
}

jitter_buffer::const_iterator jitter_buffer::end() const {
    return const_iterator(this);
}

std::ostream& operator<<(std::ostream& os, const jitter_buffer& jb){
    os << "jitterbuffer (" << jb.size() << " / " << jb.capacity() << "): ";
    for (auto& b : jb){
        os << "\n" << b.sequence << " " << "(" << b.received_frames() << "/" << b.num_frames() << ")";
    }
    return os;
}

} // aoo
