#pragma once

#include <inttypes.h>

#ifdef _WIN32
#include <winsock2.h>
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <sys/select.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>
#endif

#include <cstring>
#include <string>

namespace aoo {
namespace net {

struct ip_address {
    ip_address(){
        memset(&address, 0, sizeof(address));
        length = sizeof(address);
    }
    ip_address(const struct sockaddr *sa, socklen_t len){
        memcpy(&address, sa, len);
        length = len;
    }
    ip_address(uint32_t ipv4, int port){
        struct sockaddr_in sa;
        memset(&sa, 0, sizeof(sa));
        sa.sin_family = AF_INET;
        sa.sin_addr.s_addr = htonl(ipv4);
        sa.sin_port = htons(port);
        memcpy(&address, &sa, sizeof(sa));
        length = sizeof(sa);
    }
    ip_address(const std::string& host, int port){
        struct sockaddr_in sa;
        memset(&sa, 0, sizeof(sa));
        sa.sin_family = AF_INET;
        sa.sin_addr.s_addr = inet_addr(host.c_str());
        sa.sin_port = htons(port);
        memcpy(&address, &sa, sizeof(sa));
        length = sizeof(sa);
    }

    ip_address(const ip_address& other){
        memcpy(&address, &other.address, other.length);
        length = other.length;
    }
    ip_address& operator=(const ip_address& other){
        memcpy(&address, &other.address, other.length);
        length = other.length;
        return *this;
    }

    bool operator==(const ip_address& other) const {
        if (address.ss_family == other.address.ss_family){
        #if 1
            if (address.ss_family == AF_INET){
                auto a = (const struct sockaddr_in *)&address;
                auto b = (const struct sockaddr_in *)&other.address;
                return (a->sin_addr.s_addr == b->sin_addr.s_addr)
                        && (a->sin_port == b->sin_port);
            } else  {
                // IPv6 not supported yet
                return false;
            }
        #else
            // doesn't work reliable on BSDs if sin_len is not set
            return !memcmp(&address, &other.address, length);
        #endif
        } else {
            return false;
        }
    }

    std::string name() const {
        if (address.ss_family == AF_INET){
            return inet_ntoa(reinterpret_cast<const struct sockaddr_in *>(&address)->sin_addr);
        } else {
            return "";
        }
    }

    int port() const {
        if (address.ss_family == AF_INET){
            return ntohs(reinterpret_cast<const struct sockaddr_in *>(&address)->sin_port);
        } else {
            return -1;
        }
    }

    struct sockaddr_storage address;
    socklen_t length;
};

void socket_close(int sock);

std::string socket_strerror(int err);

int socket_errno();

int socket_set_nonblocking(int socket, int nonblocking);

int socket_connect(int socket, const ip_address& addr, float timeout);

} // net
} // aoo
