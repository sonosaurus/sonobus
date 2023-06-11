/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "net_utils.hpp"

#include "aoo/aoo_config.h"
#include "aoo/aoo_defines.h"
#include "aoo/aoo_types.h"

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <sys/select.h>
#include <sys/poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>
#endif

#include <cassert>
#include <stdio.h>
#include <cstring>
#include <algorithm>

#define DEBUG_ADDRINFO 0

namespace aoo {

//------------------------ ip_address ------------------------//

ip_address::ip_address(){
    static_assert(sizeof(address_) == max_length,
                  "wrong max_length value");
#if AOO_USE_IPv6
    static_assert(sizeof(address_) >= sizeof(sockaddr_in6),
                  "ip_address can't hold IPv6 sockaddr");
#endif
    clear();
}

ip_address::ip_address(socklen_t size) {
    clear();
    length_ = size;
}

ip_address::ip_address(const struct sockaddr *sa, socklen_t len){
    if (sa && len > 0) {
        memcpy(&address_, sa, len);
        length_ = len;
    } else {
        clear();
    }
#if 0
    check();
#endif
}

ip_address::ip_address(const AooSockAddr &addr)
    : ip_address((const struct sockaddr *)addr.data, addr.size) {}

ip_address::ip_address(const AooByte *bytes, AooSize size,
                       uint16_t port, ip_type type) {
    switch (type) {
#if AOO_USE_IPv6
    case IPv6:
    {
        assert(size == 16);

        sockaddr_in6 sa;
        memset(&sa, 0, sizeof(sa));
        sa.sin6_family = AF_INET6;
        sa.sin6_port = htons(port);
        memcpy(&sa.sin6_addr, bytes, size);

        memcpy(&address_, &sa, sizeof(sa));
        length_ = sizeof(sa);
        break;
    }
#endif
    case IPv4:
    {
        assert(size == 4);

        sockaddr_in sa;
        memset(&sa, 0, sizeof(sa));
        sa.sin_family = AF_INET;
        sa.sin_port = htons(port);
        memcpy(&sa.sin_addr, bytes, size);

        memcpy(&address_, &sa, sizeof(sa));
        length_ = sizeof(sa);
        break;
    }
    default:
        clear();
        break;
    }
#if 0
    check();
#endif
}

void ip_address::clear(){
    memset(&address_, 0, sizeof(address_));
    address_ptr()->sa_family = AF_UNSPEC;
    length_ = 0;
}

void ip_address::check() {
    auto f = address()->sa_family;
#if AOO_USE_IPv6
    bool ok = (f == AF_INET6 || f == AF_INET || f == AF_UNSPEC);
#else
    bool ok = (f == AF_INET || f == AF_UNSPEC);
#endif
    if (!ok) {
        fprintf(stderr, "bad address family: %d\n", f);
        fflush(stderr);
    }
}

void ip_address::resize(socklen_t size) {
    length_ = size;
}

std::vector<ip_address> ip_address::resolve(const std::string &host, uint16_t port,
                                            ip_type type, bool ipv4mapped){
    std::vector<ip_address> result;

    if (host.empty()) {
    #if DEBUG_ADDRINFO
        fprintf(stderr, "don't resolve empty host\n");
        fflush(stderr);
    #endif
        return result;
    }

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));

    // if we have IPv6 support, only get IPv6 addresses
    // (IPv4 addresses will be mapped to IPv6 addresses)
    switch (type){
#if AOO_USE_IPv6
    case IPv6:
        hints.ai_family = AF_INET6;
        break;
#endif
    case IPv4:
        hints.ai_family = AF_INET;
        break;
    default:
        hints.ai_family = AF_UNSPEC;
        break;
    }

    hints.ai_flags =
        AI_ADDRCONFIG | // check if we have a matching adapter
        AI_NUMERICSERV | // we use a port number
        AI_PASSIVE;      // listen to any addr if hostname is NULL
#if AOO_USE_IPv6
    if (ipv4mapped) {
    #ifdef AI_V4MAPPED
        hints.ai_flags |= AI_V4MAPPED; // fallback to IPv4-mapped IPv6 addresses
    #endif
    #ifdef AI_ALL
        hints.ai_flags |= AI_ALL; // both IPv6 and IPv4-mapped addresses
    #endif
    }
#endif

    char portstr[10]; // largest port is 65535
    snprintf(portstr, sizeof(portstr), "%d", port);

    struct addrinfo *ailist;
    int err = getaddrinfo(!host.empty() ? host.c_str() : nullptr,
                          portstr, &hints, &ailist);
    if (err == 0){
    #if DEBUG_ADDRINFO
        fprintf(stderr, "resolved '%s' to:\n", host.empty() ? "any" : host.c_str());
    #endif
        for (auto ai = ailist; ai; ai = ai->ai_next){
        #if DEBUG_ADDRINFO
            fprintf(stderr, "\t%s\n", get_name(ai->ai_addr));
        #endif
            // avoid duplicate entries
            ip_address addr(ai->ai_addr, ai->ai_addrlen);
            if (std::find(result.begin(), result.end(), addr) == result.end()){
                result.push_back(addr);
            }
        }
    #if DEBUG_ADDRINFO
        fflush(stderr);
    #endif
    } else {
        // LATER think about how to correctly handle error. Throw exception?
    #ifdef _WIN32
        WSASetLastError(WSAHOST_NOT_FOUND);
    #else
        errno = HOST_NOT_FOUND;
    #endif
    }
    freeaddrinfo(ailist);
    return result;
}

ip_address::ip_address(uint16_t port, ip_type type) {
    // LATER optimize
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    // AI_PASSIVE: nullptr means "any" address
    hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV | AI_PASSIVE;
    switch (type){
#if AOO_USE_IPv6
    case ip_type::IPv6:
        hints.ai_family = AF_INET6;
        break;
#endif
    case ip_type::IPv4:
        hints.ai_family = AF_INET;
        break;
    default:
        hints.ai_family = AF_UNSPEC;
    }

    char portstr[10]; // largest port is 65535
    snprintf(portstr, sizeof(portstr), "%d", port);

    struct addrinfo *ailist;
    int err = getaddrinfo(nullptr, portstr, &hints, &ailist);
    if (err == 0){
        memcpy(&address_, ailist->ai_addr, ailist->ai_addrlen);
        length_ = ailist->ai_addrlen;
        freeaddrinfo(ailist);
    } else {
        // fail
        clear();
    }
#if 0
    check();
#endif
}

ip_address::ip_address(const std::string& ip, uint16_t port, ip_type type) {
    if (ip.empty() || port <= 0) {
        clear();
        return;
    }

    // LATER optimize
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    // prevent DNS lookup!
    hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;
    hints.ai_family = AF_UNSPEC;

    char portstr[10]; // largest port is 65535
    snprintf(portstr, sizeof(portstr), "%d", port);

    struct addrinfo *ailist;
    int err = getaddrinfo(ip.c_str(), portstr, &hints, &ailist);
    if (err == 0){
    #if AOO_USE_IPv6
        if (type == ip_type::IPv6 && ailist->ai_family == AF_INET){
            // manually create IPv4-mapped address.
            // this is workaround for the fact that AI_NUMERICHOST
            // doesn't seem to work with AI_V4MAPPED (at least on Windows)
            freeaddrinfo(ailist);
            std::string mapped = "::ffff:" + ip;
            err = getaddrinfo(mapped.c_str(), portstr, &hints, &ailist);
            if (err != 0){
                // fail
                clear();
                return;
            }
        }
    #endif
        // otherwise just take the first result
        memcpy(&address_, ailist->ai_addr, ailist->ai_addrlen);
        length_ = ailist->ai_addrlen;
        freeaddrinfo(ailist);
    } else {
        // fail
        clear();
    }
#if 0
    check();
#endif
}

ip_address::ip_address(const ip_address& other){
    if (other.length_ > 0) {
        memcpy(&address_, &other.address_, other.length_);
        length_ = other.length_;
    } else {
        clear();
    }
#if 0
    check();
#endif
}

ip_address& ip_address::operator=(const ip_address& other){
    if (other.length_ > 0) {
        memcpy(&address_, &other.address_, other.length_);
        length_ = other.length_;
    } else {
        clear();
    }
#if 0
    check();
#endif
    return *this;
}

bool ip_address::operator==(const ip_address& other) const {
    if (address()->sa_family == other.address()->sa_family){
        switch (address()->sa_family){
        case AF_INET:
        {
            auto a = (const struct sockaddr_in *)&address_;
            auto b = (const struct sockaddr_in *)&other.address_;
            return (a->sin_addr.s_addr == b->sin_addr.s_addr)
                    && (a->sin_port == b->sin_port);
        }
    #if AOO_USE_IPv6
        case AF_INET6:
        {
            auto a = (const struct sockaddr_in6 *)&address_;
            auto b = (const struct sockaddr_in6 *)&other.address_;
            return !memcmp(a->sin6_addr.s6_addr, b->sin6_addr.s6_addr,
                           sizeof(struct in6_addr))
                    && (a->sin6_port == b->sin6_port);
        }
    #endif
        default:
            break;
        }
    }
    return false;
}

std::ostream& operator<<(std::ostream& os, const ip_address& addr) {
    switch (addr.address()->sa_family) {
    case AF_INET6:
        os << "[" << addr.name() << "]:" << addr.port();
        break;
    case AF_INET:
        os << addr.name() << ":" << addr.port();
        break;
    case AF_UNSPEC:
        os << "[empty]";
        break;
    default:
        os << "[bad address]";
        break;
    }
    return os;
}

const char * ip_address::get_name(const sockaddr *addr){
#if AOO_USE_IPv6
    thread_local char buf[INET6_ADDRSTRLEN];
#else
    thread_local char buf[INET_ADDRSTRLEN];
#endif
    const void *na;
    auto family = addr->sa_family;
    switch (family){
#if AOO_USE_IPv6
    case AF_INET6:
        na = &reinterpret_cast<const sockaddr_in6 *>(addr)->sin6_addr;
        break;
#endif
    case AF_INET:
        na = &reinterpret_cast<const sockaddr_in *>(addr)->sin_addr;
        break;
    default:
        return "";
    }

    if (inet_ntop(family, na, buf, sizeof(buf)) != nullptr) {
        return buf;
    } else {
        return "";
    }
}

const char* ip_address::name() const {
    if (length_ > 0) {
        return get_name((const sockaddr *)&address_);
    } else {
        return "";
    }
}

// for backwards compatibility with IPv4 only servers
const char* ip_address::name_unmapped() const {
    auto str = name();
    // strip leading "::ffff:" for mapped IPv4 addresses
    if (!strncmp(str, "::ffff:", 7) ||
        !strncmp(str, "::FFFF:", 7)) {
        return str + 7;
    } else {
        return str;
    }
}

uint16_t ip_address::port() const {
    switch (address()->sa_family){
    case AF_INET:
        return ntohs(reinterpret_cast<const sockaddr_in *>(address())->sin_port);
#if AOO_USE_IPv6
    case AF_INET6:
        return ntohs(reinterpret_cast<const sockaddr_in6 *>(address())->sin6_port);
#endif
    default:
        return 0;
    }
}

const AooByte* ip_address::address_bytes() const {
    switch (address()->sa_family){
    case AF_INET:
        return (const AooByte *)&reinterpret_cast<const sockaddr_in *>(address())->sin_addr;
#if AOO_USE_IPv6
    case AF_INET6:
        return (const AooByte *)&reinterpret_cast<const sockaddr_in6 *>(address())->sin6_addr;
#endif
    default:
        return nullptr;
    }
}

size_t ip_address::address_size() const {
    switch (type()) {
    case IPv6:
        return 16;
    case IPv4:
        return 4;
    default:
        return 0;
    }
}

bool ip_address::valid() const {
    return address()->sa_family != AF_UNSPEC;
}

ip_address::ip_type ip_address::type() const {
    switch(address()->sa_family){
    case AF_INET:
        return IPv4;
#if AOO_USE_IPv6
    case AF_INET6:
        return IPv6;
#endif
    default:
        return Unspec;
    }
}

bool ip_address::is_ipv4_mapped() const {
#if AOO_USE_IPv6
    if (address()->sa_family == AF_INET6){
        auto addr = reinterpret_cast<const sockaddr_in6 *>(&address_);
        auto w = (uint16_t *)addr->sin6_addr.s6_addr;
        return (w[0] == 0) && (w[1] == 0) && (w[2] == 0) && (w[3] == 0) &&
               (w[4] == 0) && (w[5] == 0xffff);
    }
#endif
    return false;
}

ip_address ip_address::ipv4_mapped() const {
#if AOO_USE_IPv6
    if (type() == IPv4) {
        auto addr = reinterpret_cast<const sockaddr_in *>(&address_);
        uint16_t w[8] = { 0, 0, 0, 0, 0, 0xffff };
        memcpy(&w[6], &addr->sin_addr.s_addr, 4);
        return ip_address((const AooByte *)&w, 16, port(), IPv6);
    }
#endif
    return *this;
}

ip_address ip_address::unmapped() const {
#if AOO_USE_IPv6
    if (is_ipv4_mapped()){
        auto addr = reinterpret_cast<const sockaddr_in6 *>(&address_);
        auto w = (uint16_t *)addr->sin6_addr.s6_addr;
        return ip_address((const AooByte *)&w[6], 4, port(), IPv4);
    }
#endif
    return *this;
}

//------------------------ socket ----------------------------//

int socket_init()
{
    static bool initialized = false;
    if (!initialized)
    {
    #ifdef _WIN32
        short version = MAKEWORD(2, 0);
        WSADATA wsadata;
        if (WSAStartup(version, &wsadata))
            return -1;
    #endif

        initialized = true;
    }
    return 0;
}

int socket_errno()
{
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

void socket_set_errno(int err)
{
#ifdef _WIN32
    WSASetLastError(err);
#else
    errno = err;
#endif
}

int socket_error(int socket) {
    int error = 0;
    socklen_t errlen = sizeof(error);
    getsockopt(socket, SOL_SOCKET, SO_ERROR, (char *)&error, &errlen);
    return error;
}

int socket_strerror(int err, char *buf, int size)
{
#ifdef _WIN32
    wchar_t wbuf[1024];
    auto wlen = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0,
                               err, MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), wbuf,
                               sizeof(wbuf), NULL);
    if (wlen <= 0){
        return -1;
    }
    // convert to unicode
    auto len = WideCharToMultiByte(CP_UTF8, 0, wbuf, wlen, buf, size, NULL, NULL);
    if (len == 0){
        return -1;
    }
    // remove trailing newlines
    auto ptr = buf + (len - 1);
    while (*ptr == '\n' || *ptr == '\r'){
        *ptr-- = '\0';
        len--;
    }
    // add error number
    len += snprintf(buf + len, size - len, " [%d]", err);
    return len;
#else
    return snprintf(buf, size, "%s [%d]", strerror(err), err);
#endif
}

std::string socket_strerror(int err){
    char buf[1024];
    if (socket_strerror(err, buf, 1024) > 0){
        return buf;
    } else {
        return std::string {};
    }
}

void socket_error_print(const char *label)
{
    int err = socket_errno();
    if (!err){
        return;
    }

    char str[1024];
    socket_strerror(err, str, sizeof(str));
    if (label){
        fprintf(stderr, "%s: %s (%d)\n", label, str, err);
    } else {
        fprintf(stderr, "%s (%d)\n", str, err);
    }
    fflush(stderr);

    socket_set_errno(err); // restore errno!
}

int socket_udp(uint16_t port)
{
#if AOO_USE_IPv6
    // prefer IPv6, but fall back to IPv4 if disabled
    ip_address bindaddr;
    int sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock >= 0){
        bindaddr = ip_address(port, ip_address::IPv6);
        // make dual stack socket by listening to both IPv4 and IPv6 packets
        if (socket_set_int_option(sock, IPPROTO_IPV6, IPV6_V6ONLY, false) != 0){
            fprintf(stderr, "socket_udp: couldn't set IPV6_V6ONLY");
            fflush(stderr);
            // TODO: fall back to IPv4?
        }
    } else {
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        bindaddr = ip_address(port, ip_address::IPv4);
    }
#else
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    ip_address bindaddr(port, ip_address::IPv4);
#endif
    if (sock < 0) {
        socket_error_print("socket_udp");
        return -1;
    }
    // finally bind the socket
    if (bind(sock, bindaddr.address(), bindaddr.length()) != 0){
        auto err = socket_errno(); // cache errno
        socket_error_print("bind");
        socket_close(sock);
        socket_set_errno(err); // restore errno
        return -1;
    }
    return sock; // success
}

int socket_tcp(uint16_t port)
{
#if AOO_USE_IPv6
    // prefer IPv6, but fall back to IPv4 if disabled
    ip_address bindaddr;
    int sock = socket(AF_INET6, SOCK_STREAM, 0);
    if (sock >= 0) {
        bindaddr = ip_address(port, ip_address::IPv6);
        // make dual stack socket by listening to both IPv4 and IPv6 packets
        if (socket_set_int_option(sock, IPPROTO_IPV6, IPV6_V6ONLY, false)) {
            fprintf(stderr, "socket_udp: couldn't set IPV6_V6ONLY");
            fflush(stderr);
            // TODO: fall back to IPv4?
        }
    } else {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        bindaddr = ip_address(port, ip_address::IPv4);
    }
#else
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    ip_address bindaddr = ip_address(port, ip_address::IPv4);
#endif
    if (sock < 0){
        socket_error_print("socket_tcp");
        return -1;
    }
    // set SO_REUSEADDR
    if (socket_set_int_option(sock, SOL_SOCKET, SO_REUSEADDR, true) != 0) {
        fprintf(stderr, "aoo_client: couldn't set SO_REUSEADDR");
        fflush(stderr);
    }
    // disable Nagle's algorithm
    if (socket_set_int_option(sock, IPPROTO_TCP, TCP_NODELAY, true)) {
        fprintf(stderr, "aoo_client: couldn't set TCP_NODELAY");
        fflush(stderr);
    }
    // finally bind the socket
    if (bind(sock, bindaddr.address(), bindaddr.length()) != 0) {
        int err = socket_errno(); // cache errno
        socket_error_print("bind");
        socket_close(sock);
        socket_set_errno(err); // restore errno
        return -1;
    }
    return sock; // success
}


int socket_address(int socket, ip_address& addr)
{
    sockaddr_storage ss;
    socklen_t len = sizeof(ss);
    if (getsockname(socket, (sockaddr *)&ss, &len) < 0){
        return -1;
    } else {
        addr = ip_address((sockaddr *)&ss, len);
        return 0;
    }
}

int socket_peer(int socket, ip_address& addr)
{
    sockaddr_storage ss;
    socklen_t len = sizeof(ss);
    if (getpeername(socket, (sockaddr *)&ss, &len) < 0){
        return -1;
    } else {
        addr = ip_address((sockaddr *)&ss, len);
        return 0;
    }
}

int socket_port(int socket){
    ip_address addr;
    if (socket_address(socket, addr) < 0){
        socket_error_print("socket_port");
        return -1;
    } else {
        return addr.port();
    }
}

ip_address::ip_type socket_family(int socket){
    ip_address addr;
    if (socket_address(socket, addr) < 0){
        socket_error_print("socket_family");
        return ip_address::Unspec;
    } else {
        return addr.type();
    }
}

int socket_shutdown(int socket, shutdown_method how) {
    return shutdown(socket, how);
}

int socket_close(int socket)
{
#ifdef _WIN32
    return closesocket(socket);
#else
    return close(socket);
#endif
}

int socket_sendto(int socket, const void *buf, int size, const ip_address& addr)
{
    return sendto(socket, (const char *)buf, size, 0, addr.address(), addr.length());
}

int socket_receive(int socket, void *buf, int size,
                   ip_address* addr, double timeout)
{
    if (timeout >= 0){
        // non-blocking receive via poll()
        struct pollfd p;
        p.fd = socket;
        p.revents = 0;
        p.events = POLLIN;
    #ifdef _WIN32
        int result = WSAPoll(&p, 1, timeout * 1000);
    #else
        int result = poll(&p, 1, timeout * 1000);
    #endif
        if (result < 0){
            socket_error_print("poll");
            return -1; // poll failed
        }
        if (!(result > 0 && (p.revents & POLLIN))){
            return 0; // timeout
        }
    }
    if (addr) {
        addr->resize(ip_address::max_length);
        return recvfrom(socket, (char *)buf, size, 0,
                        addr->address_ptr(), addr->length_ptr());
    } else {
        return recv(socket, (char *)buf, size, 0);
    }
}

#define DEBUG_SOCKET_BUFFER 0

int socket_set_sendbufsize(int socket, int bufsize)
{
    int oldsize = 0;
    socket_get_int_option(socket, SOL_SOCKET, SO_SNDBUF, &oldsize);
#if DEBUG_SOCKET_BUFFER
    fprintf(stderr, "old recvbufsize: %d\n", oldsize);
    fflush(stderr);
#endif
    // don't set a smaller buffer size than the default
    if (bufsize < oldsize){
        return 0;
    }
    int result = socket_set_int_option(socket, SOL_SOCKET, SO_SNDBUF, bufsize);
#if DEBUG_SOCKET_BUFFER
    if (result == 0){
        int newsize = -1;
        socket_get_int_option(socket, SOL_SOCKET, SO_SNDBUF, &newsize);
        fprintf(stderr, "new recvbufsize: %d\n", newsize);
        fflush(stderr);
    }
#endif
    return result;
}

int socket_set_recvbufsize(int socket, int bufsize)
{
    int oldsize = 0;
    socket_get_int_option(socket, SOL_SOCKET, SO_RCVBUF, &oldsize);
#if DEBUG_SOCKET_BUFFER
    fprintf(stderr, "old recvbufsize: %d\n", oldsize);
    fflush(stderr);
#endif
    // don't set a smaller buffer size than the default
    if (bufsize < oldsize){
        return 0;
    }
    int result = socket_set_int_option(socket, SOL_SOCKET, SO_RCVBUF, bufsize);
#if DEBUG_SOCKET_BUFFER
    if (result == 0){
        int newsize = -1;
        socket_get_int_option(socket, SOL_SOCKET, SO_RCVBUF, &newsize);
        fprintf(stderr, "new recvbufsize: %d\n", newsize);
        fflush(stderr);
    }
#endif
    return result;
}

bool socket_signal(int socket)
{
    // wake up blocking recv() by sending an empty packet to itself
    ip_address addr;
    if (socket_address(socket, addr) < 0){
        socket_error_print("getsockname");
        return false;
    }
    if (addr.type() == ip_address::ip_type::IPv6){
        addr = ip_address("::1", addr.port(), addr.type());
    } else {
        addr = ip_address("127.0.0.1", addr.port(), addr.type());
    }

    if (sendto(socket, 0, 0, 0, addr.address(), addr.length()) < 0){
        socket_error_print("sendto");
        return false;
    } else {
        return true;
    }
}

int socket_set_int_option(int socket, int level, int option, bool value) {
    int arg = value;
    return setsockopt(socket, level, option, (const char *)&arg, sizeof(arg));
}

int socket_get_int_option(int socket, int level, int option, bool* value) {
    int arg;
    socklen_t len = sizeof(arg);
    int err = getsockopt(socket, level, option, (char *)&arg, &len);
    if (err == 0) {
        *value = arg;
    }
    return err;
}

int socket_set_int_option(int socket, int level, int option, int value) {
    return setsockopt(socket, level, option, (const char *)&value, sizeof(value));
}

int socket_get_int_option(int socket, int level, int option, int* value) {
    socklen_t len = sizeof(*value);
    return getsockopt(socket, level, option, (char *)value, &len);
}

int socket_set_nonblocking(int socket, bool nonblocking)
{
#ifdef _WIN32
    u_long modearg = nonblocking;
    if (ioctlsocket(socket, FIONBIO, &modearg) != NO_ERROR)
        return -1;
#else
    int sockflags = fcntl(socket, F_GETFL, 0);
    if (nonblocking)
        sockflags |= O_NONBLOCK;
    else
        sockflags &= ~O_NONBLOCK;
    if (fcntl(socket, F_SETFL, sockflags) < 0)
        return -1;
#endif
    return 0;
}

// kudos to https://stackoverflow.com/a/46062474/6063908
int socket_connect(int socket, const ip_address& addr, double timeout)
{
    // set nonblocking and connect
    socket_set_nonblocking(socket, true);

    if (connect(socket, addr.address(), addr.length()) < 0)
    {
        int status;
        struct timeval timeoutval;
        fd_set writefds, errfds;
    #ifdef _WIN32
        if (socket_errno() != WSAEWOULDBLOCK)
    #else
        if (socket_errno() != EINPROGRESS)
    #endif
            return -1; // break on "real" error

        // block with select using timeout
        if (timeout < 0) timeout = 0;
        timeoutval.tv_sec = (int)timeout;
        timeoutval.tv_usec = (timeout - timeoutval.tv_sec) * 1000000;
        FD_ZERO(&writefds);
        FD_SET(socket, &writefds); // socket is connected when writable
        FD_ZERO(&errfds);
        FD_SET(socket, &errfds); // catch exceptions

        status = select(socket + 1, NULL, &writefds, &errfds, &timeoutval);
        if (status < 0) // select failed
        {
            socket_error_print("select");
            return -1;
        }
        else if (status == 0) // connection timed out
        {
        #ifdef _WIN32
            WSASetLastError(WSAETIMEDOUT);
        #else
            errno = ETIMEDOUT;
        #endif
            return -1;
        }

        if (FD_ISSET(socket, &errfds)) // connection failed
        {
            int err = socket_error(socket);
            socket_set_errno(err);
            return -1;
        }
    }
    // done, set blocking again
    socket_set_nonblocking(socket, false);
    return 0;
}

} // aoo
