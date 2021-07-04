#pragma once

#include "aoo/aoo_types.h"

#include "imp.hpp"

#include <vector>

namespace aoo {

class dynamic_resampler {
public:
    void setup(int32_t nfrom, int32_t nto,
               int32_t srfrom, int32_t srto,
               int32_t nchannels);
    void reset();
    void update(double srfrom, double srto);

    bool write(const aoo_sample* data, int32_t n);
    bool read(aoo_sample* data, int32_t n);

    int32_t size() const { return balance_; }
    int32_t capacity() const { return buffer_.size(); }
    double ratio() const { return ideal_ratio_; }
private:
    std::vector<aoo_sample, aoo::allocator<aoo_sample>> buffer_;
    int32_t nchannels_ = 0;
    double rdpos_ = 0;
    int32_t wrpos_ = 0;
    double balance_ = 0;
    double ratio_ = 1.0;
    double ideal_ratio_ = 1.0;
};

}
