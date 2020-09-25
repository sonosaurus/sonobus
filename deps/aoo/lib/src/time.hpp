/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#pragma once

#include <inttypes.h>
#include <ostream>

namespace aoo {

/*////////////////////////// time tag //////////////////////////*/

struct time_tag {
    static time_tag now();

    static double duration(time_tag t1, time_tag t2);

    time_tag() = default;
    time_tag(uint32_t _high, uint32_t _low)
        : high(_high), low(_low){}
    time_tag(uint64_t ui){
        high = ui >> 32;
        low = (uint32_t)ui;
    }
    time_tag(double s){
        high = (uint64_t)s;
        double fract = s - (double)high;
        low = fract * 4294967296.0;
    }

    uint32_t high = 0;
    uint32_t low = 0;

    void clear(){
        high = 0;
        low = 0;
    }

    bool empty() const { return (high + low) == 0; }

    double to_double() const {
        return (double)high + (double)low / 4294967296.0;
    }
    uint64_t to_uint64() const {
        return (uint64_t)high << 32 | (uint64_t)low;
    }

    friend std::ostream& operator << (std::ostream& os, time_tag t);
};

inline time_tag operator+(time_tag lhs, time_tag rhs){
    time_tag result;
    uint64_t ns = lhs.low + rhs.low;
    result.low = ns & 0xFFFFFFFF;
    result.high = lhs.high + rhs.high + (ns >> 32);
    return result;
}

inline time_tag operator-(time_tag lhs, time_tag rhs){
    time_tag result;
    uint64_t ns = ((uint64_t)1 << 32) + lhs.low - rhs.low;
    result.low = ns & 0xFFFFFFFF;
    result.high = lhs.high - rhs.high - !(ns >> 32);
    return result;
}

inline bool operator==(time_tag lhs, time_tag rhs){
    return lhs.high == rhs.high && lhs.low == rhs.low;
}
inline bool operator<(time_tag lhs, time_tag rhs){
    if (lhs.high < rhs.high){
        return true;
    } else {
        return lhs.high == rhs.high && lhs.low < rhs.low;
    }
}
inline bool operator!=(time_tag lhs, time_tag rhs){ return !operator==(lhs,rhs); }
inline bool operator> (time_tag lhs, time_tag rhs){ return  operator< (rhs,lhs); }
inline bool operator<=(time_tag lhs, time_tag rhs){ return !operator> (lhs,rhs); }
inline bool operator>=(time_tag lhs, time_tag rhs){ return !operator< (lhs,rhs); }

} // aoo
