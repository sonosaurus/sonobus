#include "timer.hpp"

namespace aoo {

timer::timer(timer&& other){
    last_.store(other.last_.load());
    elapsed_.store(other.elapsed_.load());
    mavg_check_ = std::move(other.mavg_check_);
}

timer& timer::operator=(timer&& other){
    last_.store(other.last_.load());
    elapsed_.store(other.elapsed_.load());
    mavg_check_ = std::move(other.mavg_check_);
    return *this;
}

void timer::setup(int32_t sr, int32_t blocksize, bool check){
    if (check){
        auto delta = (double)blocksize / (double)sr;
        mavg_check_ = std::make_unique<moving_average_check>(delta);
    } else {
        mavg_check_ = nullptr;
    }

    reset();
}

timer::state timer::update(time_tag t, double& error){
    time_tag last = last_.exchange(t);
    if (!last.is_empty()){
        auto delta = time_tag::duration(last, t);
    #if AOO_DEBUG_TIMER
        LOG_DEBUG("time delta: " << delta * 1000.0 << " ms");
    #endif
        // 'elapsed' is only ever modified in this function
        // (which is not reentrant!)
        auto elapsed = elapsed_.load() + delta;
        elapsed_.store(elapsed);

        if (mavg_check_){
            return mavg_check_->check(delta, error);
        } else {
            return state::ok;
        }
    } else {
        // reset
        elapsed_.store(0);
        if (mavg_check_){
            mavg_check_->reset();
        }

        return state::reset;
    }
}

timer::state timer::moving_average_check::check(double delta, double& error){
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

    if (average_error > delta_ * AOO_TIMER_TOLERANCE){
        LOG_WARNING("DSP tick(s) took too long!");
        LOG_VERBOSE("last period: " << (delta * 1000.0)
                    << " ms, average period: " << (average * 1000.0)
                    << " ms, error: " << (last_error * 1000.0)
                    << " ms, average error: " << (average_error * 1000.0) << " ms");
        error = std::max<double>(0, delta - delta_);
        return state::error;
    } else {
    #if AOO_DEBUG_TIMER
        LOG_ALL("average delta: " << (average * 1000.0)
                << " ms, error: " << (last_error * 1000.0)
                << ", average error: " << (average_error * 1000.0));
    #endif
        return state::ok;
    }
}

void timer::moving_average_check::reset(){
    // fill ringbuffer with nominal delta
    std::fill(buffer_.begin(), buffer_.end(), delta_);
    sum_ = delta_ * buffer_.size(); // initial sum
    head_ = 0;
}

} // aoo
