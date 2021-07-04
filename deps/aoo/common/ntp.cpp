#ifdef _WIN32
#include <windows.h>
#endif

#include <stdexcept>
#include <string>
#include <cstring>

namespace aoo {

/*////////////////////// NTP server ////////////////////*/

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
