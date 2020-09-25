/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others. 
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "aoo_common.h"

#include "string.h"
#include "stdlib.h"

#ifdef _WIN32
#include <windows.h>
#endif

void aoo_send_tilde_setup(void);
void aoo_receive_tilde_setup(void);
void aoo_pack_tilde_setup(void);
void aoo_unpack_tilde_setup(void);
void aoo_route_setup(void);
void aoo_node_setup(void);
void aoo_server_setup(void);
void aoo_client_setup(void);

static void seconds_to_string(int sec, char *buf, size_t size)
{
    if (sec <= 0){
        snprintf(buf, size, "-"); // shouldn't happen
        return;
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

    if (d){
        count += snprintf(buf, size, "%d days ", d);
    }
    snprintf(buf + count, size - count, "%02d:%02d:%02d", h, m, s);
}

#ifdef _WIN32
static HKEY reg_openkey(HKEY key, const char *subkey)
{
    HKEY result;

    DWORD ret = RegOpenKeyExA(key, subkey, 0, KEY_QUERY_VALUE, &result);
    if (ret == ERROR_SUCCESS){
        return result;
    } else {
        int err = GetLastError();
        error("Registry: couldn't open key %s (%d)", subkey, (int)ret);
        return (HKEY)0;
    }
}

static int reg_getvalue(HKEY key, const char *subkey, const char *value,
                        DWORD type, void *data, DWORD *size)
{
    DWORD rtype, ret;

    ret = RegGetValueA(key, subkey, value, RRF_RT_ANY, &rtype, data, size);
    if (ret == ERROR_SUCCESS){
        if (rtype == type){
            return 1;
        } else {
            error("Registry: wrong type for value %s", value);
        }
    } else {
        error("Registry: couldn't get value %s in %s (%d)", value, subkey, (int)ret);
    }
    return 0;
}

static int service_running(const char *service)
{
    SC_HANDLE scm = OpenSCManagerA(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
    if (scm == NULL)
        return 0;

    SC_HANDLE hService = OpenServiceA(scm, service, GENERIC_READ);
    if (hService == NULL)
    {
        CloseServiceHandle(scm);
        return 0;
    }

    int running = 0;
    SERVICE_STATUS status;
    if (QueryServiceStatus(hService, &status)){
        running = status.dwCurrentState == SERVICE_RUNNING;
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(scm);

    return running;
}

static void check_ntp(void)
{
    if (!service_running("W32Time")){
        error("WARNING: Windows time server is not running!");
        return;
    }

    HKEY w32time = reg_openkey(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\services\\W32Time");
    if (w32time){
        // first get the time server + flags from "Parameters/NtpServer"
        // e.g. time.windows.com,0x9
        char server[256];
        DWORD size = 256;

        if (reg_getvalue(w32time, "Parameters", "NtpServer", REG_SZ, server, &size)){
            char *onset = strrchr(server, ',');
            if (onset){
                int flags;
                if (sscanf(onset + 1, "%i", &flags) > 0){
                    *onset = 0;

                    if (flags & 1){
                        // special poll interval
                        HKEY tp = reg_openkey(w32time, "TimeProviders");
                        if (tp){
                            DWORD pollint;
                            size = sizeof(DWORD);

                            if (reg_getvalue(tp, "NtpClient", "SpecialPollInterval",
                                             REG_DWORD, &pollint, &size))
                            {
                                char pollintstring[64];
                                seconds_to_string(pollint, pollintstring, 64);

                                post("NTP server: %s, poll interval: %s", server, pollintstring);
                                post("NOTE: disable SpecialPollInterval "
                                     "for more accurate timing (see README)");
                            }
                            RegCloseKey(tp);
                        }
                    } else {
                        // min/max poll interval
                        DWORD maxpollint, minpollint;
                        size = sizeof(DWORD);

                        if (reg_getvalue(w32time, "Config", "MinPollInterval",
                                         REG_DWORD, &minpollint, &size) &&
                            reg_getvalue(w32time, "Config", "MaxPollInterval",
                                         REG_DWORD, &maxpollint, &size))
                        {
                            char minpollintstring[64];
                            char maxpollintstring[64];
                            // min/max poll interval are given in powers of 2
                            seconds_to_string(1 << minpollint, minpollintstring, 64);
                            seconds_to_string(1 << maxpollint, maxpollintstring, 64);

                            post("NTP server: %s, min. poll interval: %s, max. poll interval: %s",
                                 server, minpollintstring, maxpollintstring);
                        }
                    }
                } else {
                    error("couldn't extract flag from NtpServer value");
                }
            } else {
                error("value of NtpServer is not comma seperated");
            }
        }

        RegCloseKey(w32time);
    }
}

#else

static void check_ntp(void) {}

#endif

EXPORT void aoo_setup(void)
{
    char version[64];
    int offset = 0;
    offset += snprintf(version, sizeof(version), "%d.%d",
                       AOO_VERSION_MAJOR, AOO_VERSION_MINOR);
#if AOO_VERSION_BUGFIX > 0
    offset += snprintf(version + offset, sizeof(version) - offset,
                       ".%d", AOO_VERSION_BUGFIX);
#endif
#if AOO_VERSION_PRERELEASE > 0
    offset += snprintf(version + offset, sizeof(version) - offset,
                       "-pre%d", AOO_VERSION_PRERELEASE);
#endif

    post("AOO (audio over OSC) %s", version);
    post("  (c) 2020 Christof Ressi, Winfried Ritsch, et al.");

    aoo_initialize();

    check_ntp();

    post("");

    aoo_send_tilde_setup();
    aoo_receive_tilde_setup();
    aoo_pack_tilde_setup();
    aoo_unpack_tilde_setup();
    aoo_route_setup();
    aoo_node_setup();
    aoo_server_setup();
    aoo_client_setup();
}
