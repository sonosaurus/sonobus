#pragma once

#include "aoo/aoo.h"

#include "common/sync.hpp"
#include "common/time.hpp"

#include <atomic>
#include <array>

namespace aoo {

class timer {
public:
    enum class state {
        reset,
        ok,
        error
    };
    timer() = default;
    timer(timer&& other);
    timer& operator=(timer&& other);

    void setup(int32_t sr, int32_t blocksize, bool check);
    void reset();
    double get_elapsed() const {
        return elapsed_.load(std::memory_order_relaxed);
    }
    time_tag get_absolute() const{
        return last_.load(std::memory_order_relaxed);
    }
    state update(time_tag t, double& error);
private:
    std::atomic<uint64_t> last_;
    std::atomic<double> elapsed_{0};

    // moving average filter to detect timing issues
    struct moving_average_check {
        static const size_t buffersize = 64;

        moving_average_check(double delta)
            : delta_(delta) {
            static_assert(is_pow2(buffersize),
                          "buffer size must be power of 2!");
        }

        state check(double delta, double& error);
        void reset();

        double delta_ = 0;
        double sum_ = 0;
        std::array<double, buffersize> buffer_;
        int32_t head_ = 0;
    };

    std::unique_ptr<moving_average_check> mavg_check_;
};

}
