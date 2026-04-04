/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "time.hpp"

#include "aoo/aoo_utils.hpp"

#include <chrono>
#include <algorithm>
#include <cassert>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

namespace aoo {

/*////////////////////////// time tag //////////////////////////*/

#ifdef _WIN32

using system_time_fn = void (__stdcall *)(LPFILETIME);

// GetSystemTimePreciseAsFileTime is only available in Windows 8 and above.
// For older versions of Windows we fall back to GetSystemTimeAsFileTime.
system_time_fn get_system_time = [](){
    auto module = LoadLibraryA("Kernel32.dll");
    if (module){
        auto fn = (void *)GetProcAddress(module, "GetSystemTimePreciseAsFileTime");
        if (fn){
            return reinterpret_cast<system_time_fn>(fn);
            LOG_VERBOSE("using GetSystemTimePreciseAsFileTime");
        } else {
            LOG_VERBOSE("using GetSystemTimeAsFileTime");
        }
    } else {
        LOG_ERROR("couldn't open kernel32.dll!");
    }

    return &GetSystemTimeAsFileTime;
}();

#endif

// OSC time stamp (NTP time)
time_tag time_tag::now(){
#if 1
#if defined(_WIN32)
    // make sure to get the highest precision
    // LATER try to use GetSystemTimePreciseAsFileTime
    // (only available on Windows 8 and above)
    FILETIME ft;
    get_system_time(&ft);
    // GetSystemTimeAsFileTime returns the number of
    // 100-nanosecond ticks since Jan 1, 1601.
    LARGE_INTEGER date;
    date.HighPart = ft.dwHighDateTime;
    date.LowPart = ft.dwLowDateTime;
    auto d = lldiv(date.QuadPart, 10000000);
    auto seconds = d.quot;
    auto nanos = d.rem * 100;
    // Kudos to https://www.frenk.com/2009/12/convert-filetime-to-unix-timestamp/
    // Between Jan 1, 1601 and Jan 1, 1970 there are 11644473600 seconds
    seconds -= 11644473600;
#else
    struct timeval v;
    gettimeofday(&v, nullptr);
    auto seconds = v.tv_sec;
    auto nanos = v.tv_usec * 1000;
#endif // _WIN32
#else
    // use system clock (1970 epoch)
    auto epoch = std::chrono::system_clock::now().time_since_epoch();
    auto s = std::chrono::duration_cast<std::chrono::seconds>(epoch);
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(epoch - s);
    auto seconds = s.count();
    auto nanos = ns.count();
    // LOG_DEBUG("seconds: " << seconds << ", nanos: " << nanos);
#endif
    // add number of seconds between 1900 and 1970 (including leap years!)
    uint32_t high = seconds + 2208988800UL;
    // fractional part in nanoseconds mapped to the range of uint32_t
    uint32_t low = nanos * 4.294967296; // 2^32 / 1e9
    return time_tag(high, low);
}

double time_tag::duration(time_tag t1, time_tag t2){
    if (t2 >= t1){
        return (t2 - t1).to_double();
    } else {
        LOG_DEBUG("t2 is smaller than t1!");
        return (t1 - t2).to_double() * -1.0;
    }
}

std::ostream& operator << (std::ostream& os, time_tag t){
    int32_t hours, minutes, seconds;
    auto d = lldiv((int64_t)t.high, 3600);
    hours = d.quot;
    d = lldiv(d.rem, 60);
    minutes = d.quot;
    seconds = d.rem;

    int32_t micros = t.low / 4294967296.0 * 1000000.0;

    os << "time_tag (" << hours << ":" << minutes << ":"
       << seconds << "." << micros << ")";

    return os;
}

} // aoo
