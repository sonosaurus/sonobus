#pragma once

#include "aoo/aoo_config.h"
#include "aoo/aoo_defines.h"
#include "aoo/aoo_types.h"

#include <ostream>
#include <string>
#include <vector>

#ifdef _WIN32
typedef int socklen_t;
struct sockaddr;
#else
#include <sys/socket.h>
#include <errno.h>
#endif

#ifndef AOO_USE_IPv6
#define AOO_USE_IPv6 1
#endif

namespace aoo {

//-------------- ip_address --------------//

class ip_address {
public:
    enum ip_type {
        Unspec,
        IPv4,
        IPv6
    };

    static std::vector<ip_address> resolve(const std::string& host, uint16_t port,
                                           ip_type type, bool ipv4mapped = false);

    ip_address();
    ip_address(socklen_t size);
    ip_address(const struct sockaddr *sa, socklen_t len);
    ip_address(const AooSockAddr& addr);
    ip_address(uint16_t port, ip_type type); // "any" address
    ip_address(const std::string& ip, uint16_t port, ip_type type = ip_type::Unspec);
    ip_address(const AooByte *bytes, AooSize size, uint16_t port, ip_type type);

    ip_address(const ip_address& other);
    ip_address& operator=(const ip_address& other);

    void clear();

    void resize(socklen_t size);

    bool operator==(const ip_address& other) const;

    bool operator !=(const ip_address& other) const {
        return !(*this == other);
    }

    const char* name() const;

    const char* name_unmapped() const;

    uint16_t port() const;

    const AooByte* address_bytes() const;

    size_t address_size() const;

    bool valid() const;

    ip_type type() const;

    bool is_ipv4_mapped() const;

    ip_address ipv4_mapped() const;

    ip_address unmapped() const;

    const struct sockaddr *address() const {
        return (const struct sockaddr *)&address_;
    }

    struct sockaddr *address_ptr() {
        return (struct sockaddr *)&address_;
    }

    socklen_t length() const {
        return length_;
    }

    socklen_t *length_ptr() {
        return &length_;
    }

    friend std::ostream& operator<<(std::ostream& os, const ip_address& addr);

    static const socklen_t max_length = 32;
private:
    static const char *get_name(const struct sockaddr *addr);
    // large enough to hold both sockaddr_in
    // and sockaddr_in6 (max. 32 bytes)
    struct {
        int16_t __ss_family;
        char __ss_pad1[6];
        int64_t __ss_align;
        char __ss_pad2[16];
    } address_;
    socklen_t length_;

    void check();
};

//-------------------- socket --------------------//

int socket_init();

int socket_udp(uint16_t port);

int socket_tcp(uint16_t port);

enum shutdown_method {
    shutdown_receive = 0,
    shutdown_send = 1,
    shutdown_both = 2
};

int socket_shutdown(int socket, shutdown_method how);

int socket_close(int socket);

int socket_connect(int socket, const ip_address& addr, double timeout);

int socket_address(int socket, ip_address& addr);

int socket_peer(int socket, ip_address& addr);

int socket_port(int socket);

ip_address::ip_type socket_family(int socket);

int socket_sendto(int socket, const void *buf, int size,
                  const ip_address& address);

int socket_receive(int socket, void *buf, int size,
                   ip_address* addr, double timeout);

int socket_set_sendbufsize(int socket, int bufsize);

int socket_set_recvbufsize(int socket, int bufsize);

int socket_set_int_option(int socket, int level, int opt, int value);

int socket_get_int_option(int socket, int level, int opt, int* value);

int socket_set_nonblocking(int socket, bool nonblocking);

bool socket_signal(int socket);

int socket_errno();

void socket_set_errno(int err);

int socket_error(int socket);

int socket_strerror(int err, char *buf, int size);

std::string socket_strerror(int err);

void socket_error_print(const char *label = nullptr);

} // aoo
