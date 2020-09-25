/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others. 
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#pragma once

#ifdef _WIN32
#include <winsock2.h>
typedef int socklen_t;
#else
#include <sys/socket.h>
#endif

#include "m_pd.h"

int socket_udp(void);

int socket_close(int socket);

int socket_bind(int socket, int port);

int socket_sendto(int socket, const char *buf, int size, const struct sockaddr *addr);

int socket_receive(int socket, char *buf, int size,
                   struct sockaddr_storage *sa, socklen_t *len,
                   int nonblocking);

int socket_setsendbufsize(int socket, int bufsize);

int socket_setrecvbufsize(int socket, int bufsize);

int socket_signal(int socket, int port);

int socket_getaddr(const char *hostname, int port,
                   struct sockaddr_storage *sa, socklen_t *len);

int socket_errno();

int socket_strerror(int err, char *buf, int size);

void socket_error_print(const char *label);

int sockaddr_to_atoms(const struct sockaddr *sa, socklen_t len, t_atom *a);

// use linked list for persistent memory
typedef struct _endpoint {
    void *owner;
    struct sockaddr_storage addr;
    socklen_t addrlen;
    struct _endpoint *next;
} t_endpoint;

t_endpoint * endpoint_new(void *owner, const struct sockaddr_storage *sa, socklen_t len);

void endpoint_free(t_endpoint *e);

int endpoint_send(t_endpoint *e, const char *data, int size);

int endpoint_getaddress(const t_endpoint *e, t_symbol **hostname, int *port);

t_endpoint * endpoint_find(t_endpoint *e, const struct sockaddr_storage *sa);

int endpoint_match(t_endpoint *e, const struct sockaddr_storage *sa);
