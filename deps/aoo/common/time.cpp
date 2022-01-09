/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "time.hpp"

#include <stdexcept>
#include <string>
#include <cstring>
#include <chrono>
#include <algorithm>
#include <cassert>

#ifdef _WIN32
# include <windows.h>
// to prevent segfault in LOG_* when initializing get_system_time
# include <iostream>
#else
# include <sys/time.h>
#endif

namespace aoo {

//------------------- time_tag -----------------------------//

#ifdef _WIN32

using system_time_fn = VOID (WINAPI *) (LPFILETIME);

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

std::ostream& operator << (std::ostream& os, time_tag t){
    auto s = t.to_seconds();
    int32_t days, hours, minutes, seconds, micros;

    auto d = lldiv(s, 86400);
    days = d.quot;
    d = lldiv(d.rem, 3600);
    hours = d.quot;
    d = lldiv(d.rem, 60);
    minutes = d.quot;
    seconds = d.rem;
    micros = (s - (uint64_t)s) * 1000000.0;

    if (days){
        os << days << "-";
    }

    char buf[256];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d.%06d",
             hours, minutes, seconds, micros);

    os << buf;

    return os;
}

//------------------- NTP server --------------------//

#ifdef _WIN32

static std::string seconds_to_string(int sec)
{
    if (sec <= 0){
        return "-"; // shouldn't happen
    }

    int d = 0, h = 0, m = 0, s = 0, count = 0;
    div_t dv = div(sec, 60);
    s = dv.rem;
    if (dv.quot){
        dv = div(dv.quot, 60);
        m = dv.rem;
        if (dv.quot){
            dv = div(dv.quot, 24);
            h = dv.rem;
            d = dv.quot;
        }
    }
    char buf[64];
    if (d){
        count += snprintf(buf, 64, "%d days ", d);
    }
    snprintf(buf + count, 64 - count, "%02d:%02d:%02d", h, m, s);

    return buf;
}

static HKEY reg_openkey(HKEY key, const char *subkey)
{
    HKEY result;
    char buf[256];

    DWORD ret = RegOpenKeyExA(key, subkey, 0, KEY_QUERY_VALUE, &result);
    if (ret == ERROR_SUCCESS){
        return result;
    } else {
        snprintf(buf, sizeof(buf), "Registry: couldn't open key %s (%d)", subkey, (int)ret);

        throw std::domain_error(buf);
    }
}

static void reg_getvalue(HKEY key, const char *subkey, const char *value,
                        DWORD type, void *data, DWORD *size)
{
    char buf[256];
    DWORD rtype;

    DWORD ret = RegGetValueA(key, subkey, value, RRF_RT_ANY, &rtype, data, size);
    if (ret == ERROR_SUCCESS){
        if (rtype == type){
            return; // success
        } else {
            snprintf(buf, sizeof(buf), "Registry: wrong type for value %s", value);
        }
    } else {
        snprintf(buf, sizeof(buf), "Registry: couldn't get value %s in %s (%d)",
                 value, subkey, (int)ret);
    }

    throw std::domain_error(buf);
}

static bool service_running(const char *service)
{
    SC_HANDLE scm = OpenSCManagerA(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
    if (scm == NULL)
        return false;

    SC_HANDLE hService = OpenServiceA(scm, service, GENERIC_READ);
    if (hService == NULL)
    {
        CloseServiceHandle(scm);
        return false;
    }

    bool running = false;
    SERVICE_STATUS status;
    if (QueryServiceStatus(hService, &status)){
        running = status.dwCurrentState == SERVICE_RUNNING;
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(scm);

    return running;
}

bool check_ntp_server(std::string& msg)
{
    char buf[256];
    HKEY w32time = nullptr;
    HKEY timeProviders = nullptr;
    bool result = false;

    try {
        if (!service_running("W32Time")){
            msg = "Windows time server is not running!";
            return false;
        }

        w32time = reg_openkey(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\services\\W32Time");
        // first get the time server + flags from "Parameters/NtpServer"
        // e.g. time.windows.com,0x9
        char server[256];
        DWORD size = 256;

        reg_getvalue(w32time, "Parameters", "NtpServer", REG_SZ, server, &size);

        char *onset = strrchr(server, ',');
        if (onset){
            int flags;
            if (sscanf(onset + 1, "%i", &flags) > 0){
                *onset = 0;

                if (flags & 1){
                    // special poll interval
                    timeProviders = reg_openkey(w32time, "TimeProviders");

                    DWORD pollint;
                    size = sizeof(DWORD);
                    reg_getvalue(timeProviders, "NtpClient", "SpecialPollInterval",
                                 REG_DWORD, &pollint, &size);
                    auto pollintstring = seconds_to_string(pollint);

                    snprintf(buf, sizeof(buf), "NTP server: %s, poll interval: %s\n"
                        "NOTE: disable SpecialPollInterval "
                        "for more accurate timing (see README)",
                        server, pollintstring.c_str());
                } else {
                    // min/max poll interval
                    DWORD maxpollint, minpollint;
                    size = sizeof(DWORD);

                    reg_getvalue(w32time, "Config", "MinPollInterval",
                                 REG_DWORD, &minpollint, &size);
                    reg_getvalue(w32time, "Config", "MaxPollInterval",
                                 REG_DWORD, &maxpollint, &size);
                    auto minpollintstring = seconds_to_string(1 << minpollint);
                    auto maxpollintstring = seconds_to_string(1 << maxpollint);
                    // min/max poll interval are given in powers of 2

                    snprintf(buf, sizeof(buf), "NTP server: %s, min. poll interval: %s, "
                             "max. poll interval: %s",
                             server, minpollintstring.c_str(), maxpollintstring.c_str());
                }
                msg = buf;
                result = true;
            } else {
                throw std::domain_error("couldn't extract flag from NtpServer value");
            }
        } else {
            throw std::domain_error("value of NtpServer is not comma seperated");
        }
    } catch (const std::exception& e){
        msg = e.what();
    }

    if (w32time){
        RegCloseKey(w32time);
    }
    if (timeProviders){
        RegCloseKey(timeProviders);
    }

    return result;
}

#else

bool check_ntp_server(std::string &msg) { return true; }

#endif

} // aoo
