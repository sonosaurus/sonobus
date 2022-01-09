/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#pragma once

#include <inttypes.h>
#include <ostream>

#include "utils.hpp"

namespace aoo {

//---------------- OSC time tag -------------------//

struct time_tag {
    static time_tag now();

    static time_tag immediate() {
        return time_tag{(uint64_t)1};
    }

    static time_tag from_seconds(double seconds) {
        uint64_t high = seconds;
        double fract = seconds - (double)high;
        uint64_t low = fract * 4294967296.0;
        return time_tag((high << 32) | low);
    }

    static double duration(time_tag t1, time_tag t2){
        if (t2 >= t1){
            return (t2 - t1).to_seconds();
        } else {
            LOG_DEBUG("t2 is smaller than t1!");
            return (t1 - t2).to_seconds() * -1.0;
        }
    }

    time_tag(uint64_t value = 0) : value_(value) {}

    time_tag(uint32_t high, uint32_t low)
        : value_(((uint64_t)high << 32) | (uint64_t)low) {}

    uint64_t value() const { return value_; }

    operator uint64_t () const {
        return value_;
    }

    void clear(){
        value_ = 0;
    }

    bool is_empty() const { return value_ == 0; }

    bool is_immediate() const { return value_ == 1; }

    double to_seconds() const {
        auto high = value_ >> 32;
        auto low = value_ & 0xFFFFFFFF;
        return (double)high + ((double)low / 4294967296.0);
    }

    time_tag& operator += (time_tag t){
        value_ += t.value_;
        return *this;
    }

    time_tag& operator -= (time_tag t){
        value_ -= t.value_;
        return *this;
    }

    friend std::ostream& operator << (std::ostream& os, time_tag t);

    friend time_tag operator+(time_tag lhs, time_tag rhs){
        return lhs.value_ + rhs.value_;
    }

    friend time_tag operator-(time_tag lhs, time_tag rhs){
        return lhs.value_ - rhs.value_;
    }

    friend bool operator==(time_tag lhs, time_tag rhs){
        return lhs.value_ == rhs.value_;
    }
    friend bool operator<(time_tag lhs, time_tag rhs){
        return lhs.value_ < rhs.value_;
    }
    friend bool operator!=(time_tag lhs, time_tag rhs){ return !operator==(lhs,rhs); }
    friend bool operator> (time_tag lhs, time_tag rhs){ return  operator< (rhs,lhs); }
    friend bool operator<=(time_tag lhs, time_tag rhs){ return !operator> (lhs,rhs); }
    friend bool operator>=(time_tag lhs, time_tag rhs){ return !operator< (lhs,rhs); }
private:
    uint64_t value_;
};

//----------------- NTP server -----------------------//

bool check_ntp_server(std::string& msg);

} // aoo
