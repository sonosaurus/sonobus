/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others. 
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "aoo/aoo_utils.hpp"
#include "aoo/aoo_pcm.h"
#if USE_CODEC_OPUS
#include "aoo/aoo_opus.h"
#endif
#include "common.hpp"

#include <unordered_map>
#include <algorithm>
#include <cassert>
#include <cstring>

/*/////////////// version ////////////////////*/

namespace aoo {

bool check_version(uint32_t version){
    auto major = (version >> 24) & 255;
    auto minor = (version >> 16) & 255;
    auto bugfix = (version >> 8) & 255;

    if (major != AOO_VERSION_MAJOR){
        return false;
    }

    return true;
}

uint32_t make_version(uint8_t protocolflags){
    // make version: major, minor, bugfix, [protocol]
    return ((uint32_t)AOO_VERSION_MAJOR << 24) | ((uint32_t)AOO_VERSION_MINOR << 16)
            | ((uint32_t)AOO_VERSION_BUGFIX << 8) | ((uint32_t) protocolflags);
}

}

/*////////////// codec plugins ///////////////*/

namespace aoo {

static std::unordered_map<std::string, std::unique_ptr<aoo::codec>> codec_dict;

const aoo::codec * find_codec(const std::string& name){
    auto it = codec_dict.find(name);
    if (it != codec_dict.end()){
        return it->second.get();
    } else {
        return nullptr;
    }
}

} // aoo

int32_t aoo_register_codec(const char *name, const aoo_codec *codec){
    if (aoo::codec_dict.count(name) != 0){
        LOG_WARNING("aoo: codec " << name << " already registered!");
        return 0;
    }
    aoo::codec_dict[name] = std::make_unique<aoo::codec>(codec);
    LOG_VERBOSE("aoo: registered codec '" << name << "'");
    return 1;
}

/*//////////////////// OSC ////////////////////////////*/

int32_t aoo_parse_pattern(const char *msg, int32_t n,
                         int32_t *type, int32_t *id)
{
    int32_t offset = 0;
    // special case the compact data message which doesn't use the aoo domain
    if (n >= AOO_MSG_COMPACT_DATA_LEN
        && !memcmp(msg, AOO_MSG_COMPACT_DATA, AOO_MSG_COMPACT_DATA_LEN)) 
    {
        *type = AOO_TYPE_SINK;
        offset += AOO_MSG_COMPACT_DATA_LEN;
        *id = AOO_ID_NONE; // will be looked up later
        return offset;
    }
    else if (n >= AOO_MSG_DOMAIN_LEN
        && !memcmp(msg, AOO_MSG_DOMAIN, AOO_MSG_DOMAIN_LEN))
    {
        offset += AOO_MSG_DOMAIN_LEN;
        if (n >= (offset + AOO_MSG_SOURCE_LEN)
            && !memcmp(msg + offset, AOO_MSG_SOURCE, AOO_MSG_SOURCE_LEN))
        {
            *type = AOO_TYPE_SOURCE;
            offset += AOO_MSG_SOURCE_LEN;
        } else if (n >= (offset + AOO_MSG_SINK_LEN)
            && !memcmp(msg + offset, AOO_MSG_SINK, AOO_MSG_SINK_LEN))
        {
            *type = AOO_TYPE_SINK;
            offset += AOO_MSG_SINK_LEN;
        } else {
            return 0;
        }

        if (!memcmp(msg + offset, "/*", 2)){
            *id = AOO_ID_WILDCARD; // wildcard
            return offset + 2;
        }
        int32_t skip = 0;
        if (sscanf(msg + offset, "/%d%n", id, &skip) > 0){
            return offset + skip;
        } else {
            // TODO only print relevant part of OSC address string
            LOG_ERROR("aoo_parsepattern: bad ID " << msg + offset);
            return 0;
        }
    } else {
        return 0; // not an AoO message
    }
}

// OSC time stamp (NTP time)
uint64_t aoo_osctime_get(void){
    return aoo::time_tag::now().to_uint64();
}

double aoo_osctime_toseconds(uint64_t t){
    return aoo::time_tag(t).to_double();
}

uint64_t aoo_osctime_fromseconds(double s){
    return aoo::time_tag(s).to_uint64();
}

double aoo_osctime_duration(uint64_t t1, uint64_t t2){
    return aoo::time_tag::duration(t1, t2);
}

namespace aoo {

/*////////////////////////// codec /////////////////////////////*/

bool encoder::set_format(aoo_format& fmt){
    auto result = codec_->encoder_setformat(obj_, &fmt);
    if (result > 0){
        // assign after validation!
        nchannels_ = fmt.nchannels;
        samplerate_ = fmt.samplerate;
        blocksize_ = fmt.blocksize;
        return true;
    } else {
        return false;
    }
}

int32_t encoder::read_format(const aoo_format& fmt, const char *buf, int32_t size){
    aoo_format_storage nfmt;
    memcpy(&nfmt.header, &fmt, sizeof(aoo_format));
    auto result = codec_->encoder_readformat(obj_, &nfmt.header, buf, size);
    if (result > 0) {
        // assign after validation!
        nchannels_ = nfmt.header.nchannels;
        samplerate_ = nfmt.header.samplerate;
        blocksize_ = nfmt.header.blocksize;
    }
    return result;
}

bool decoder::set_format(aoo_format& fmt){
    auto result = codec_->decoder_setformat(obj_, &fmt);
    if (result > 0){
        // assign after validation!
        nchannels_ = fmt.nchannels;
        samplerate_ = fmt.samplerate;
        blocksize_ = fmt.blocksize;
        return true;
    } else {
        return false;
    }
}

int32_t decoder::read_format(const aoo_format& fmt, const char *opt, int32_t size){
    aoo_format_storage nfmt;
    memcpy(&nfmt.header, &fmt, sizeof(aoo_format));
    auto result = codec_->decoder_readformat(obj_, &nfmt.header, opt, size);
    if (result >= 0){
        nchannels_ = nfmt.header.nchannels;
        samplerate_ = nfmt.header.samplerate;
        blocksize_ = nfmt.header.blocksize;
    }
    return result;
}

std::unique_ptr<encoder> codec::create_encoder() const {
    auto obj = codec_->encoder_new();
    if (obj){
        return std::make_unique<encoder>(codec_, obj);
    } else {
        return nullptr;
    }
}
std::unique_ptr<decoder> codec::create_decoder() const {
    auto obj = codec_->decoder_new();
    if (obj){
        return std::make_unique<decoder>(codec_, obj);
    } else {
        return nullptr;
    }
}

/*////////////////////////// block /////////////////////////////*/

void block::set(int32_t seq, double sr, int32_t chn,
             int32_t nbytes, int32_t nframes)
{
    sequence = seq;
    samplerate = sr;
    channel = chn;
    numframes_ = nframes;
    framesize_ = 0;
    assert(nbytes > 0);
    buffer_.resize(nbytes);
    // set missing frame bits to 1
    frames_ = 0;
    for (int i = 0; i < nframes; ++i){
        frames_ |= ((uint64_t)1 << i);
    }
    // LOG_DEBUG("initial frames: " << (unsigned)frames);
}

void block::set(int32_t seq, double sr, int32_t chn,
                const char *data, int32_t nbytes,
                int32_t nframes, int32_t framesize)
{
    sequence = seq;
    samplerate = sr;
    channel = chn;
    numframes_ = nframes;
    framesize_ = framesize;
    frames_ = 0; // no frames missing
    buffer_.assign(data, data + nbytes);
}

bool block::complete() const {
    if (buffer_.data() == nullptr){
        LOG_ERROR("buffer is 0!");
    }
    assert(buffer_.data() != nullptr);
    assert(sequence >= 0);
    return frames_ == 0;
}

void block::add_frame(int32_t which, const char *data, int32_t n){
    assert(data != nullptr);
    assert(buffer_.data() != nullptr);
    if (which == numframes_ - 1){
        LOG_DEBUG("copy last frame with " << n << " bytes");
        std::copy(data, data + n, buffer_.end() - n);
    } else {
        LOG_DEBUG("copy frame " << which << " with " << n << " bytes");
        std::copy(data, data + n, buffer_.begin() + which * n);
        framesize_ = n; // LATER allow varying framesizes
    }
    frames_ &= ~((uint64_t)1 << which);
    // LOG_DEBUG("frames: " << frames_);
}

int32_t block::get_frame(int32_t which, char *data, int32_t n){
    assert(framesize_ > 0 && numframes_ > 0);
    if (which >= 0 && which < numframes_){
        auto onset = which * framesize_;
        auto minsize = (which == numframes_ - 1) ? size() - onset : framesize_;
        if (n >= minsize){
            int32_t nbytes;
            if (which == numframes_ - 1){ // last frame
                nbytes = size() - onset;
            } else {
                nbytes = framesize_;
            }
            auto ptr = buffer_.data() + onset;
            std::copy(ptr, ptr + n, data);
            return nbytes;
        } else {
            LOG_ERROR("buffer too small! got " << n << ", need " << minsize);
        }
    } else {
        LOG_ERROR("frame number " << which << " out of range!");
    }
    return 0;
}

int32_t block::frame_size(int32_t which) const {
    assert(which < numframes_);
    if (which == numframes_ - 1){ // last frame
        return size() - which * framesize_;
    } else {
        return framesize_;
    }
}

bool block::has_frame(int32_t which) const {
    assert(which < numframes_);
    return ((frames_ >> which) & 1) == 0;
}

/*////////////////////////// block_ack /////////////////////////////*/

block_ack::block_ack()
    : sequence(EMPTY), count_(0), timestamp_(-1e009){}

block_ack::block_ack(int32_t seq, int32_t limit)
    : sequence(seq) {
    count_ = limit;
    timestamp_ = -1e009;
}

bool block_ack::update(double time, double interval){
    if (count_ > 0){
        auto diff = time - timestamp_;
        if (diff >= interval){
            timestamp_ = time;
            count_--;
            LOG_DEBUG("request block " << sequence);
            return true;
        } else {
//            LOG_DEBUG("don't resend block "
//                        << sequence << ": need to wait");
        }
    } else {
//        LOG_DEBUG("don't resend block "
//                    << sequence << ": tried too many times");
    }
    return false;
}

/*////////////////////////// block_ack_list ///////////////////////////*/

#if BLOCK_ACK_LIST_HASHTABLE

block_ack_list::block_ack_list(){
    static_assert(is_pow2(initial_size_), "initial size must be a power of 2!");
    data_.resize(initial_size_);
    size_ = 0;
    deleted_ = 0;
    oldest_ = INT32_MAX;
}

void block_ack_list::set_limit(int32_t limit){
    limit_ = limit;
}

void block_ack_list::clear(){
    for (auto& b : data_){
        b.sequence = block_ack::EMPTY;
    }
    size_ = 0;
    deleted_ = 0;
    oldest_ = INT32_MAX;
}

int32_t block_ack_list::size() const {
    return size_;
}

bool block_ack_list::empty() const {
    return size_ == 0;
}

block_ack * block_ack_list::find(int32_t seq){
    int32_t mask = data_.size() - 1;
    auto index = seq & mask;
    while (data_[index].sequence != seq){
        // terminate on empty bucket, but skip deleted buckets
        if (data_[index].sequence == block_ack::EMPTY){
            return nullptr;
        }
        index = (index + 1) & mask;
    }
    assert(data_[index].sequence >= 0);
    assert(seq >= oldest_);
    return &data_[index];
}

block_ack& block_ack_list::get(int32_t seq){
    // try to find item
    block_ack *deleted = nullptr;
    int32_t mask = data_.size() - 1;
    auto index = seq & mask;
    while (data_[index].sequence != seq){
        if (data_[index].sequence == block_ack::DELETED){
            // save for reuse
            deleted = &data_[index];
        } else if (data_[index].sequence == block_ack::EMPTY){
            // empty bucket -> not found -> insert item
            // update oldest
            if (seq < oldest_){
                oldest_ = seq;
            }
            // try to reclaim deleted bucket
            if (deleted){
                *deleted = block_ack { seq, limit_ };
                deleted_--;
                size_++;
                // load factor doesn't change, no need to rehash
                return *deleted;
            }
            // put in empty bucket
            data_[index] = block_ack { seq, limit_ };
            size_++;
            // rehash if the table is more than 50% full
            if ((size_ + deleted_) > (int32_t)(data_.size() >> 1)){
                rehash();
                auto b = find(seq);
                assert(b != nullptr);
                return *b;
            } else {
                return data_[index];
            }
        }
        index = (index + 1) & mask;
    }
    // return existing item
    assert(data_[index].sequence >= 0);
    return data_[index];
}

bool block_ack_list::remove(int32_t seq){
    // first find the key
    auto b = find(seq);
    if (b){
        b->sequence = block_ack::DELETED; // mark as deleted
        deleted_++;
        size_--;
        // this won't give the "true" oldest value, but a closer one
        if (seq == oldest_){
            oldest_++;
        }
        return true;
    } else {
        return false;
    }
}

int32_t block_ack_list::remove_before(int32_t seq){
    if (empty() || seq <= oldest_){
        return 0;
    }
    LOG_DEBUG("block_ack_list: oldest before = " << oldest_);
    int count = 0;
    for (auto& d : data_){
        if (d.sequence >= 0 && d.sequence < seq){
            d.sequence = block_ack::DELETED; // mark as deleted
            count++;
            size_--;
            deleted_++;
        }
    }
    oldest_ = seq;
    LOG_DEBUG("block_ack_list: oldest after = " << oldest_);
    assert(size_ >= 0);
    return count;
}

void block_ack_list::rehash(){
    auto newsize = data_.size() << 1; // double the size
    auto mask = newsize - 1;
    std::vector<block_ack> temp(newsize);
    // use this chance to find oldest item
    oldest_ = INT32_MAX;
    // we skip all deleted items; 'size_' stays the same
    deleted_ = 0;
    // reinsert items
    for (auto& b : data_){
        if (b.sequence >= 0){
            auto index = b.sequence & mask;
            // find free slot
            while (temp[index].sequence >= 0){
                index = (index + 1) & mask;
            }
            // insert item
            temp[index] = block_ack { b.sequence, limit_ };
            // update oldest
            if (b.sequence < oldest_){
                oldest_ = b.sequence;
            }
        }
    }
    data_ = std::move(temp);
}

std::ostream& operator<<(std::ostream& os, const block_ack_list& b){
    os << "acklist (" << b.size() << " / " << b.data_.size() << "): ";
    for (auto& d : b.data_){
        if (d.sequence >= 0){
            os << d.sequence << " ";
        }
    }
    return os;
}

#else

block_ack_list::block_ack_list(){}

void block_ack_list::setup(int32_t limit){
    limit_ = limit;
}

void block_ack_list::clear(){
    data_.clear();
}

int32_t block_ack_list::size() const {
    return data_.size();
}

bool block_ack_list::empty() const {
    return data_.empty();
}

block_ack * block_ack_list::find(int32_t seq){
#if BLOCK_ACK_LIST_SORTED
    // binary search
    auto it = lower_bound(seq);
    if (it != data_.end() && it->sequence == seq){
        return &*it;
    }
#else
    // linear search
    for (auto& b : data_){
        if (b.sequence == seq){
            return &b;
        }
    }
#endif
    return nullptr;
}

block_ack& block_ack_list::get(int32_t seq){
#if BLOCK_ACK_LIST_SORTED
    auto it = lower_bound(seq);
    // insert if needed
    if (it == data_.end() || it->sequence != seq){
        it = data_.emplace(it, seq, limit_);
    }
    return *it;
#else
    auto b = find(seq);
    if (b){
        return *b;
    } else {
        data_.emplace_back(seq, limit_);
        return data_.back();
    }
#endif
}

bool block_ack_list::remove(int32_t seq){
#if BLOCK_ACK_LIST_SORTED
    auto it = lower_bound(seq);
    if (it != data_.end() && it->sequence == seq){
        data_.erase(it);
        return true;
    }
#else
    for (auto it = data_.begin(); it != data_.end(); ++it){
        if (it->sequence == seq){
            data_.erase(it);
            return true;
        }
    }
#endif
    return false;
}

int32_t block_ack_list::remove_before(int32_t seq){
    if (empty()){
        return 0;
    }
    int count = 0;
#if BLOCK_ACK_LIST_SORTED
    auto begin = data_.begin();
    auto end = lower_bound(seq);
    count = end - begin;
    data_.erase(begin, end);
#else
    for (auto it = data_.begin(); it != data_.end(); ){
        if (it->sequence < seq){
            it = data_.erase(it);
            count++;
        } else {
            ++it;
        }
    }
#endif
    return count;
}

#if BLOCK_ACK_LIST_SORTED
std::vector<block_ack>::iterator block_ack_list::lower_bound(int32_t seq){
    return std::lower_bound(data_.begin(), data_.end(), seq, [](auto& a, auto& b){
        return a.sequence < b;
    });
}
#endif

std::ostream& operator<<(std::ostream& os, const block_ack_list& b){
    os << "acklist (" << b.size() << "): ";
    for (auto& d : b.data_){
        os << d.sequence << " ";
    }
    return os;
}

#endif

/*////////////////////////// history_buffer ///////////////////////////*/

void history_buffer::clear(){
    head_ = 0;
    oldest_ = -1;
    for (auto& block : buffer_){
        block.sequence = -1;
    }
}

int32_t history_buffer::capacity() const {
    return buffer_.size();
}

void history_buffer::resize(int32_t n){
    buffer_.resize(n);
    clear();
}

block * history_buffer::find(int32_t seq){
    if (seq >= oldest_){
    #if 0
        // linear search
        for (auto& block : buffer_){
            if (block.sequence == seq){
                return &block;
            }
        }
    #else
        // binary search
        // blocks are always pushed in chronological order,
        // so the ranges [begin, head] and [head, end] will always be sorted.
        auto dofind = [&](auto begin, auto end) -> block * {
            auto result = std::lower_bound(begin, end, seq, [](auto& a, auto& b){
                return a.sequence < b;
            });
            if (result != end && result->sequence == seq){
                return &*result;
            } else {
                return nullptr;
            }
        };
        auto result = dofind(buffer_.begin() + head_, buffer_.end());
        if (!result){
            result = dofind(buffer_.begin(), buffer_.begin() + head_);
        }
        return result;
    #endif
    } else {
        LOG_VERBOSE("couldn't find block " << seq << " - too old");
    }
    return nullptr;
}

void history_buffer::push(int32_t seq, double sr,
                          const char *data, int32_t nbytes,
                          int32_t nframes, int32_t framesize)
{
    if (buffer_.empty()){
        return;
    }
    assert(data != nullptr && nbytes > 0);
    // check if we're going to overwrite an existing block
    if (buffer_[head_].sequence >= 0){
        oldest_ = buffer_[head_].sequence;
    }
    buffer_[head_].set(seq, sr, 0, data, nbytes, nframes, framesize);
    if (++head_ >= (int32_t)buffer_.size()){
        head_ = 0;
    }
}

/*////////////////////////// block_queue /////////////////////////////*/

void block_queue::clear(){
    size_ = 0;
}

void block_queue::resize(int32_t n){
    blocks_.resize(n);
    size_ = 0;
}

bool block_queue::empty() const {
    return size_ == 0;
}

bool block_queue::full() const {
    return size_ == capacity();
}

int32_t block_queue::size() const {
    return size_;
}

int32_t block_queue::capacity() const {
    return blocks_.size();
}

block* block_queue::insert(int32_t seq, double sr, int32_t chn,
              int32_t nbytes, int32_t nframes){
    assert(capacity() > 0);
    // find pos to insert
    block * it;
    // first try the end, as it is the most likely position
    // (blocks usually arrive in sequential order)
    if (empty() || seq > back().sequence){
        it = end();
    } else {
    #if 0
        // linear search
        it = begin();
        for (; it != end(); ++it){
            assert(it->sequence != seq);
            if (it->sequence > seq){
                break;
            }
        }
    #else
        // binary search
        it = std::lower_bound(begin(), end(), seq, [](auto& a, auto& b){
            return a.sequence < b;
        });
        assert(!(it != end() && it->sequence == seq));
    #endif
    }
    // move items if needed
    if (full()){
        if (it > begin()){
            LOG_DEBUG("insert block at pos " << (it - begin()) << " and pop old block");
            // save first block
            block temp = std::move(front());
            // move blocks before 'it' to the left
            std::move(begin() + 1, it, begin());
            // adjust iterator and re-insert removed block
            *(--it) = std::move(temp);
        } else {
            // simply replace first block
            LOG_DEBUG("replace oldest block");
        }
    } else {
        if (it != end()){
            LOG_DEBUG("insert block at pos " << (it - begin()));
            // save block past the end
            block temp = std::move(*end());
            // move blocks to the right
            std::move_backward(it, end(), end() + 1);
            // re-insert removed block at free slot (first moved item)
            *it = std::move(temp);
        } else {
            // simply replace block past the end
            LOG_DEBUG("append block");
        }
        size_++;
    }
    // replace data
    it->set(seq, sr, chn, nbytes, nframes);
    return it;
}

block* block_queue::find(int32_t seq){
    // first try the end, as we most likely have to complete the most recent block
    if (empty()){
        return nullptr;
    } else if (back().sequence == seq){
        return &back();
    }
#if 0
    // linear search
    for (int32_t i = 0; i < size_; ++i){
        if (blocks_[i].sequence == seq){
            return &blocks_[i];
        }
    }
#else
    // binary search
    auto result = std::lower_bound(begin(), end(), seq, [](auto& a, auto& b){
        return a.sequence < b;
    });
    if (result != end() && result->sequence == seq){
        return result;
    }
#endif
    return nullptr;
}

void block_queue::pop_front(){
    assert(!empty());
    if (size_ > 1){
        // temporarily remove first block
        block temp = std::move(front());
        // move remaining blocks to the left
        std::move(begin() + 1, end(), begin());
        // re-insert removed block at free slot
        back() = std::move(temp);
    }
    size_--;
}

void block_queue::pop_back(){
    assert(!empty());
    size_--;
}

block& block_queue::front(){
    assert(!empty());
    return blocks_.front();
}

block& block_queue::back(){
    assert(!empty());
    return blocks_[size_ - 1];
}

block* block_queue::begin(){
    return blocks_.data();
}

block* block_queue::end(){
    return blocks_.data() + size_;
}

block& block_queue::operator[](int32_t i){
    return blocks_[i];
}

std::ostream& operator<<(std::ostream& os, const block_queue& b){
    os << "blockqueue (" << b.size() << " / " << b.capacity() << "): ";
    for (int i = 0; i < b.size(); ++i){
        os << b.blocks_[i].sequence << " ";
    }
    return os;
}

/*////////////////////////// dynamic_resampler /////////////////////////////*/

#define AOO_RESAMPLER_SPACE 2.5 // was 3 // jlc was 8

void dynamic_resampler::setup(int32_t nfrom, int32_t nto, int32_t srfrom, int32_t srto, int32_t nchannels){
    nchannels_ = nchannels;
    auto blocksize = std::max<int32_t>(nfrom, nto);
#if 0
    // this doesn't work as expected...
    auto ratio = srfrom > srto ? (double)srfrom / (double)srto : (double)srto / (double)srfrom;
    buffer_.resize(blocksize * nchannels_ * ratio * AOO_RESAMPLER_SPACE); // extra space for fluctuations
#else
    buffer_.resize(blocksize * nchannels_ * AOO_RESAMPLER_SPACE); // extra space for fluctuations
#endif
    clear();
}

void dynamic_resampler::clear(){
    ratio_ = 1;
    rdpos_ = 0;
    wrpos_ = 0;
    balance_ = 0;
}

void dynamic_resampler::update(double srfrom, double srto){
    if (srfrom == srto){
        ratio_ = 1;
    } else {
        ratio_ = srto / srfrom;
    }
#if AOO_DEBUG_RESAMPLING
    static int counter = 0;
    if (counter == 100){
        DO_LOG("srfrom: " << srfrom << ", srto: " << srto);
        DO_LOG("resample factor: " << ratio_);
        DO_LOG("balance: " << balance_ << ", size: " << buffer_.size());
        counter = 0;
    } else {
        counter++;
    }
#endif
}

int32_t dynamic_resampler::write_available(){
    return (double)buffer_.size() - balance_; // !
}

void dynamic_resampler::write(const aoo_sample *data, int32_t n){
    auto size = (int32_t)buffer_.size();
    auto end = wrpos_ + n;
    int32_t split;
    if (end > size){
        split = size - wrpos_;
    } else {
        split = n;
    }
    std::copy(data, data + split, &buffer_[wrpos_]);
    std::copy(data + split, data + n, &buffer_[0]);
    wrpos_ += n;
    if (wrpos_ >= size){
        wrpos_ -= size;
    }
    balance_ += n;
}

int32_t dynamic_resampler::read_available(){
    return balance_ * ratio_;
}

void dynamic_resampler::read(aoo_sample *data, int32_t n){
    auto size = (int32_t)buffer_.size();
    auto limit = size / nchannels_;
    int32_t intpos = (int32_t)rdpos_;
    if (ratio_ != 1.0 || (rdpos_ - intpos) != 0.0){
        // interpolating version
        double incr = 1. / ratio_;
        assert(incr > 0);
        for (int i = 0; i < n; i += nchannels_){
            int32_t index = (int32_t)rdpos_;
            double fract = rdpos_ - (double)index;
            for (int j = 0; j < nchannels_; ++j){
                double a = buffer_[index * nchannels_ + j];
                double b = buffer_[((index + 1) * nchannels_ + j) % size];
                data[i + j] = a + (b - a) * fract;
            }
            rdpos_ += incr;
            if (rdpos_ >= limit){
                rdpos_ -= limit;
            }
        }
        balance_ -= n * incr;
    } else {
        // non-interpolating (faster) version
        int32_t pos = intpos * nchannels_;
        int32_t end = pos + n;
        int n1, n2;
        if (end > size){
            n1 = size - pos;
            n2 = end - size;
            memcpy(data, &buffer_[pos], n1 * sizeof(aoo_sample));
            memcpy(data + n1, &buffer_[0], n2 * sizeof(aoo_sample));
        } else {
            n1 = n;
            n2 = 0;
            memcpy(data, &buffer_[pos], n1 * sizeof(aoo_sample));
        }

        //std::copy(&buffer_[pos], &buffer_[pos + n1], data);
        //std::copy(&buffer_[0], &buffer_[n2], data + n1);
        rdpos_ += n / nchannels_;
        if (rdpos_ >= limit){
            rdpos_ -= limit;
        }
        balance_ -= n;
    }
}

/*//////////////////////// timer //////////////////////*/

timer::timer(const timer& other){
    last_ = other.last_.load();
    elapsed_ = other.elapsed_.load();
#if AOO_TIMEFILTER_CHECK
    static_assert(is_pow2(buffersize_), "buffer size must be power of 2!");
    delta_ = other.delta_;
    sum_ = other.sum_;
    buffer_ = other.buffer_;
    head_ = other.head_;
#endif
}

timer& timer::operator=(const timer& other){
    last_ = other.last_.load();
    elapsed_ = other.elapsed_.load();
#if AOO_TIMEFILTER_CHECK
    static_assert(is_pow2(buffersize_), "buffer size must be power of 2!");
    delta_ = other.delta_;
    sum_ = other.sum_;
    buffer_ = other.buffer_;
    head_ = other.head_;
#endif
    return *this;
}

void timer::setup(int32_t sr, int32_t blocksize){
#if AOO_TIMEFILTER_CHECK
    delta_ = (double)blocksize / (double)sr; // shouldn't tear
#endif
    reset();
}

void timer::reset(){
    scoped_lock<spinlock> l(lock_);
    last_ = 0;
    elapsed_ = 0;
#if AOO_TIMEFILTER_CHECK
    // fill ringbuffer with nominal delta
    std::fill(buffer_.begin(), buffer_.end(), delta_);
    sum_ = delta_ * buffer_.size(); // initial sum
    head_ = 0;
#endif
}

double timer::get_elapsed() const {
    return elapsed_.load();
}

time_tag timer::get_absolute() const {
    return last_.load();
}

timer::state timer::update(time_tag t, double& error){
    std::unique_lock<spinlock> l(lock_);
    time_tag last = last_.load();
    if (!last.empty()){
        last_ = t.to_uint64(); // first!

        auto delta = time_tag::duration(last, t);
        elapsed_ = elapsed_ + delta;

    #if AOO_TIMEFILTER_CHECK
        // check delta and return error

        // if we're in a callback scheduler,
        // there shouldn't be any delta larger than
        // the nominal delta +- tolerance

        // If we're in a ringbuffer scheduler and we have a
        // DSP blocksize of N and a hardware buffer size of M,
        // there will be M / N blocks calculated in a row, so we
        // usually see one large delta and (M / N) - 1 short deltas.
        // The arithmetic mean should still be the nominal delta +- tolerance.
        // If it is larger than that, we assume that one or more DSP ticks
        // took too long, so we reset the timer and output the error.
        // Note that this also happens when we start the timer
        // in the middle of the ringbuffer scheduling sequence
        // (i.e. we didn't get all short deltas before the long delta),
        // so resetting the timer makes sure that the next time we start
        // at the beginning.
        // Since the relation between hardware buffersize and DSP blocksize
        // is a power of 2, our ringbuffer size also has to be a power of 2!

        // recursive moving average filter
        head_ = (head_ + 1) & (buffer_.size() - 1);
        sum_ += delta - buffer_[head_];
        buffer_[head_] = delta;

        auto average = sum_ / buffer_.size();
        auto average_error = average - delta_;
        auto last_error = delta - delta_;

        l.unlock();

        if (average_error > delta_ * AOO_TIMEFILTER_TOLERANCE){
            LOG_WARNING("DSP tick(s) took too long!");
            LOG_VERBOSE("last period: " << (delta * 1000.0)
                        << " ms, average period: " << (average * 1000.0)
                        << " ms, error: " << (last_error * 1000.0)
                        << " ms, average error: " << (average_error * 1000.0) << " ms");
            error = std::max<double>(0, delta - delta_);
            return state::error;
        } else {
        #if 0
            DO_LOG("delta : " << (delta * 1000.0)
                      << ", average delta: " << (average * 1000.0)
                      << ", error: " << (last_error * 1000.0)
                      << ", average error: " << (average_error * 1000.0));
        #endif
        }
    #endif

        return state::ok;
    } else {
        last_ = t.to_uint64();
        return state::reset;
    }
}

} // aoo

void aoo_initialize(){
    static bool initialized = false;
    if (!initialized){
        // register codecs
        aoo_codec_pcm_setup(aoo_register_codec);

    #if USE_CODEC_OPUS
        aoo_codec_opus_setup(aoo_register_codec);
    #endif

        initialized = true;
    }
}

void aoo_terminate() {}
