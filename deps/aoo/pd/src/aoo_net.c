/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others. 
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "aoo_net.h"

#ifdef _WIN32
#include <winsock2.h>
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#endif

#include <stdio.h>
#include <string.h>

/*///////////////////////// socket /////////////////////////////////*/

int socket_errno()
{
#ifdef _WIN32
    int err = WSAGetLastError();
    if (err == WSAECONNRESET){
        return 0; // ignore
    }
#else
    int err = errno;
#endif
    return err;
}

int socket_strerror(int err, char *buf, int size)
{
#ifdef _WIN32
    buf[0] = 0;
    return FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0,
                          err, MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), buf,
                          size, NULL);
#else
    return snprintf(buf, size, "%s", strerror(err));
#endif
}

void socket_error_print(const char *label)
{
    char str[1024];

    int err = socket_errno();
    if (!err){
        return;
    }

    socket_strerror(err, str, sizeof(str));
    if (label){
        fprintf(stderr, "%s: %s (%d)\n", label, str, err);
    } else {
        fprintf(stderr, "%s (%d)\n", str, err);
    }
    fflush(stderr);
}

int socket_udp(void)
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock >= 0){
        int val = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char *)&val, sizeof(val))){
            fprintf(stderr, "socket_udp: couldn't set SO_BROADCAST");
            fflush(stderr);
        }
    } else {
        socket_error_print("socket_udp");
    }
    return sock;
}

int socket_bind(int socket, int port)
{
    // bind to 'any' address
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = INADDR_ANY;
    sa.sin_port = htons(port);
    return bind(socket, (const struct sockaddr *)&sa, sizeof(sa));
}

int socket_close(int socket)
{
#ifdef _WIN32
    return closesocket(socket);
#else
    return close(socket);
#endif
}

int socket_sendto(int socket, const char *buf, int size, const struct sockaddr *addr)
{
    if (addr->sa_family == AF_INET){
        return sendto(socket, buf, size, 0, addr, sizeof(struct sockaddr_in));
    } else {
        // not supported yet
        return -1;
    }
}

int socket_receive(int socket, char *buf, int size,
                   struct sockaddr_storage *sa, socklen_t *len,
                   int nonblocking)
{
    if (nonblocking){
        // non-blocking receive via select()
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        fd_set rdset;
        FD_ZERO(&rdset);
        FD_SET(socket, &rdset);
        if (select(socket + 1, &rdset, 0, 0, &tv) > 0){
            if (!FD_ISSET(socket, &rdset)){
                return 0;
            }
        } else {
            return 0;
        }
    }
    if (sa && len){
        *len = sizeof(struct sockaddr_storage); // initialize len!
        return recvfrom(socket, buf, size, 0, (struct sockaddr *)sa, len);
    } else {
        return recv(socket, buf, size, 0);
    }
}

int socket_setsendbufsize(int socket, int bufsize)
{
    int val = 0;
    socklen_t len;
    len = sizeof(val);
    getsockopt(socket, SOL_SOCKET, SO_SNDBUF, (void *)&val, &len);
#if 0
    fprintf(stderr, "old recvbufsize: %d\n", val);
    fflush(stderr);
#endif
    if (val > bufsize){
        return 0;
    }
    val = bufsize;
    int result = setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (void *)&val, sizeof(val));
#if 0
    if (result == 0){
        len = sizeof(val);
        getsockopt(socket, SOL_SOCKET, SO_SNDBUF, (void *)&val, &len);
        fprintf(stderr, "new recvbufsize: %d\n", val);
        fflush(stderr);
    }
#endif
    return result;
}

int socket_setrecvbufsize(int socket, int bufsize)
{
    int val = 0;
    socklen_t len;
    len = sizeof(val);
    getsockopt(socket, SOL_SOCKET, SO_RCVBUF, (void *)&val, &len);
#if 0
    fprintf(stderr, "old recvbufsize: %d\n", val);
    fflush(stderr);
#endif
    if (val > bufsize){
        return 0;
    }
    val = bufsize;
    int result = setsockopt(socket, SOL_SOCKET, SO_RCVBUF, (void *)&val, sizeof(val));
#if 0
    if (result == 0){
        len = sizeof(val);
        getsockopt(socket, SOL_SOCKET, SO_RCVBUF, (void *)&val, &len);
        fprintf(stderr, "new recvbufsize: %d\n", val);
        fflush(stderr);
    }
#endif
    return result;
}

int socket_signal(int socket, int port)
{
    // wake up blocking recv() by sending an empty packet
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(0x7f000001); // localhost
    sa.sin_port = htons(port);
    if (sendto(socket, 0, 0, 0, (struct sockaddr *)&sa, sizeof(sa)) < 0){
        socket_error_print("sendto");
        return 0;
    } else {
        return 1;
    }
}

int socket_getaddr(const char *hostname, int port,
                   struct sockaddr_storage *sa, socklen_t *len)
{
    struct hostent *he = gethostbyname(hostname);
    if (he){
        struct sockaddr_in *addr = (struct sockaddr_in *)sa;
        // zero out to make sure that memcmp() works! see socket_match()
        memset(addr, 0, sizeof(*addr));
        addr->sin_family = AF_INET;
        addr->sin_port = htons(port);
        memcpy(&addr->sin_addr, he->h_addr_list[0], he->h_length);
        *len = sizeof(struct sockaddr_in);
        return 1;
    } else {
        return 0;
    }
}

int sockaddr_to_atoms(const struct sockaddr *sa, socklen_t len, t_atom *a)
{
    // LATER add IPv6 support
    struct sockaddr_in *addr = (struct sockaddr_in *)sa;
    const char *host = inet_ntoa(addr->sin_addr);
    if (!host){
        fprintf(stderr, "inet_ntoa failed!\n");
        return 0;
    }
    SETSYMBOL(a, gensym(host));
    SETFLOAT(a + 1, ntohs(addr->sin_port));
    return 1;
}

/*//////////////////// endpoint ///////////////////////*/

t_endpoint * endpoint_new(void *owner, const struct sockaddr_storage *sa, socklen_t len)
{
    t_endpoint *e = (t_endpoint *)getbytes(sizeof(t_endpoint));
    if (e){
        e->owner = owner;
        memcpy(&e->addr, sa, len);
        e->addrlen = len;
        e->next = 0;
    }
    return e;
}

void endpoint_free(t_endpoint *e)
{
    freebytes(e, sizeof(t_endpoint));
}

int endpoint_send(t_endpoint *e, const char *data, int size)
{
    int socket = *((int *)e->owner);
    int result = sendto(socket, data, size, 0,
                       (const struct sockaddr *)&e->addr, e->addrlen);
    if (result < 0){
        socket_error_print("sendto");
    }
    return result;
}

int endpoint_getaddress(const t_endpoint *e, t_symbol **hostname, int *port)
{
    struct sockaddr_in *addr = (struct sockaddr_in *)&e->addr;
    const char *host = inet_ntoa(addr->sin_addr);
    if (!host){
        fprintf(stderr, "inet_ntoa failed!\n");
        return 0;
    }
    *hostname = gensym(host);
    *port = ntohs(addr->sin_port);
    return 1;
}

int endpoint_match(t_endpoint *e, const struct sockaddr_storage *sa)
{
    if (sa->ss_family == e->addr.ss_family){
    #if 1
        if (sa->ss_family == AF_INET){
            const struct sockaddr_in *a = (const struct sockaddr_in *)sa;
            const struct sockaddr_in *b = (const struct sockaddr_in *)&e->addr;
            return (a->sin_addr.s_addr == b->sin_addr.s_addr)
                    && (a->sin_port == b->sin_port);
        } else  {
            return 0;
        }
    #else
        // doesn't work reliable on BSDs if sin_len is not set
        return !memcmp(&address, &other.address, length);
    #endif
    } else {
        return 0;
    }
}

t_endpoint * endpoint_find(t_endpoint *e, const struct sockaddr_storage *sa)
{
    for (t_endpoint *ep = e; ep; ep = ep->next){
        if (endpoint_match(ep, sa)){
            return ep;
        }
    }
    return 0;
}
