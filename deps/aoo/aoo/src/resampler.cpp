#include "resampler.hpp"

#include <algorithm>

namespace aoo {

// extra space for samplerate fluctuations and non-pow-of-2 blocksizes.
// must be larger than 2!
#define AOO_RESAMPLER_SPACE 2.5

void dynamic_resampler::setup(int32_t nfrom, int32_t nto,
                              int32_t srfrom, int32_t srto,
                              int32_t nchannels){
    reset();
    nchannels_ = nchannels;
    ideal_ratio_ = (double)srto / (double)srfrom;
    int32_t blocksize;
    if (ideal_ratio_ < 1.0){
        // downsampling
        blocksize = std::max<int32_t>(nfrom, (double)nto / ideal_ratio_ + 0.5);
    } else {
        blocksize = std::max<int32_t>(nfrom, nto);
    }
    blocksize *= AOO_RESAMPLER_SPACE;
#if AOO_DEBUG_RESAMLER
    DO_LOG_DEBUG("resampler setup: nfrom: " << nfrom << ", srfrom: " << srfrom
              << ", nto: " << nto << ", srto: " << srto << ", capacity: " << blocksize);
#endif
    buffer_.resize(blocksize * nchannels);
#if 1
    buffer_.shrink_to_fit();
#endif
    update(srfrom, srto);
}

void dynamic_resampler::reset(){
    // don't touch ratio_!
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
#if AOO_DEBUG_RESAMLER
    DO_LOG_DEBUG("srfrom: " << srfrom << ", srto: " << srto << ", ratio: " << ratio_);
    DO_LOG_DEBUG("balance: " << balance_ << ", capacity: " << buffer_.size());
#endif
}

bool dynamic_resampler::write(const AooSample *data, int32_t n){
    if ((buffer_.size() - balance_) < n){
        return false;
    }
    auto buf = buffer_.data();
    auto size = (int32_t)buffer_.size();
    auto endpos = wrpos_ + n;
    if (endpos > size){
        auto split = size - wrpos_;
        std::copy(data, data + split, buf + wrpos_);
        std::copy(data + split, data + n, buf);
    } else {
        std::copy(data, data + n, buf + wrpos_);
    }
    wrpos_ = endpos;
    if (wrpos_ >= size){
        wrpos_ -= size;
    }
    balance_ += n;
    return true;
}

bool dynamic_resampler::write(const AooSample **data,
                              int32_t nchannels, int32_t nsamples) {
    // use nominal channel count! excess channels will be ignored,
    // missing channels are filled with zeros.
    auto n = nchannels_ * nsamples;
    if ((buffer_.size() - balance_) < n){
        return false;
    }
    auto buf = buffer_.data();
    auto size = (int32_t)buffer_.size();
    auto endpos = wrpos_ + n;
    if (endpos > size){
        auto k = (size - wrpos_) / nchannels_;
        for (int i = 0; i < k; ++i) {
            auto dest = &buf[wrpos_ + i * nchannels_];
            for (int j = 0; j < nchannels_; ++j) {
                if (j < nchannels) {
                    dest[j] = data[j][i];
                } else {
                    dest[j] = 0;
                }
            }
        }
        for (int i = k; i < nsamples; ++i) {
            auto dest = &buf[i * nchannels_];
            for (int j = 0; j < nchannels_; ++j) {
                if (j < nchannels) {
                    dest[j] = data[j][i];
                } else {
                    dest[j] = 0;
                }
            }
        }
    } else {
        for (int i = 0; i < nsamples; ++i) {
            auto dest = &buf[wrpos_ + i * nchannels_];
            for (int j = 0; j < nchannels_; ++j) {
                if (j < nchannels) {
                    dest[j] = data[j][i];
                } else {
                    dest[j] = 0;
                }
            }
        }
    }
    wrpos_ = endpos;
    if (wrpos_ >= size){
        wrpos_ -= size;
    }
    balance_ += n;
    return true;
}

bool dynamic_resampler::read(AooSample *data, int32_t n){
    auto size = (int32_t)buffer_.size();
    auto limit = size / nchannels_;
    int32_t intpos = (int32_t)rdpos_;
    double advance = 1.0 / ratio_;
    int32_t intadvance = (int32_t)advance;
    if ((advance - intadvance) == 0.0 && (rdpos_ - intpos) == 0.0){
        // non-interpolating (faster) versions
        if ((int32_t)balance_ < n * intadvance){
            return false;
        }
        if (intadvance == 1){
            // just copy samples
            int32_t pos = intpos * nchannels_;
            int32_t end = pos + n;
            auto buf = buffer_.data();
            if (end > size){
                auto n1 = size - pos;
                auto n2 = end - size;
                std::copy(buf + pos, buf + size, data);
                std::copy(buf, buf + n2, data + n1);
            } else {
                std::copy(buf + pos, buf + end, data);
            }
            pos += n;
            if (pos >= size){
                pos -= size;
            }
            rdpos_ = pos / nchannels_;
            balance_ -= n;
        } else {
            // skip samples
            int32_t pos = rdpos_;
            for (int i = 0; i < n; i += nchannels_){
                for (int j = 0; j < nchannels_; ++j){
                    int32_t index = pos * nchannels_ + j;
                    data[i + j] = buffer_[index];
                }
                pos += intadvance;
                if (pos >= limit){
                    pos -= limit;
                }
            }
            rdpos_ = pos;
            balance_ -= n * intadvance;
        }
     } else {
        // interpolating version
        if (static_cast<int32_t>(balance_ * ratio_ / nchannels_) * nchannels_ <= n){
            return false;
        }
        double pos = rdpos_;
        for (int i = 0; i < n; i += nchannels_){
            int32_t index = (int32_t)pos;
            double fract = pos - (double)index;
            for (int j = 0; j < nchannels_; ++j){
                int32_t idx1 = index * nchannels_ + j;
                int32_t idx2 = (index + 1) * nchannels_ + j;
                if (idx2 >= size){
                    idx2 -= size;
                }
                double a = buffer_[idx1];
                double b = buffer_[idx2];
                data[i + j] = a + (b - a) * fract;
            }
            pos += advance;
            if (pos >= limit){
                pos -= limit;
            }
        }
        rdpos_ = pos;
        balance_ -= n * advance;
    }
    return true;
}

} // aoo
