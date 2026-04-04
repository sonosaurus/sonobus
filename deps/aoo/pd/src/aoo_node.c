/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "aoo_common.h"
#include "aoo/aoo_net.h"

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#ifndef AOO_NODE_POLL
 #define AOO_NODE_POLL 0
#endif

#if AOO_NODE_POLL
 #ifdef _WIN32
  #include <winsock2.h>
 #else
  #include <sys/poll.h>
 #endif
#endif // AOO_NODE_POLL

// aoo_receive

extern t_class *aoo_receive_class;

typedef struct _aoo_receive t_aoo_receive;

void aoo_receive_send(t_aoo_receive *x);

void aoo_receive_handle_message(t_aoo_receive *x, const char * data,
                                int32_t n, void *endpoint, aoo_replyfn fn);

// aoo_send

extern t_class *aoo_send_class;

typedef struct _aoo_send t_aoo_send;

void aoo_send_send(t_aoo_send *x);

void aoo_send_handle_message(t_aoo_send *x, const char * data,
                             int32_t n, void *endpoint, aoo_replyfn fn);

// aoo_client

extern t_class *aoo_client_class;

typedef struct _aoo_client t_aoo_client;

void aoo_client_send(t_aoo_client *x);

void aoo_client_handle_message(t_aoo_client *x, const char * data,
                               int32_t n, void *endpoint, aoo_replyfn fn);

static void lower_thread_priority(void)
{
#ifdef _WIN32
    // lower thread priority only for high priority or real time processes
    DWORD cls = GetPriorityClass(GetCurrentProcess());
    if (cls == HIGH_PRIORITY_CLASS || cls == REALTIME_PRIORITY_CLASS){
        int priority = GetThreadPriority(GetCurrentThread());
        SetThreadPriority(GetCurrentThread(), priority - 2);
    }
#else

#endif
}

/*////////////////////// aoo node //////////////////*/

static t_class *aoo_node_class;

typedef struct _client
{
    t_pd *c_obj;
    int32_t c_id;
} t_client;

typedef struct _peer
{
    t_symbol *group;
    t_symbol *user;
    t_endpoint *endpoint;
} t_peer;

typedef struct _aoo_node
{
    t_pd x_pd;
    t_symbol *x_sym;
    // dependants
    t_client *x_clients;
    int x_numclients; // doubles as refcount
    aoo_lock x_clientlock;
    // peers
    t_peer *x_peers;
    int x_numpeers;
    // socket
    int x_socket;
    int x_port;
    t_endpoint *x_endpoints;
    pthread_mutex_t x_endpointlock;
    // threading
#if AOO_NODE_POLL
    pthread_t x_thread;
#else
    pthread_t x_sendthread;
    pthread_t x_receivethread;
    pthread_mutex_t x_mutex;
    pthread_cond_t x_condition;
#endif
    int x_quit; // should be atomic, but works anyway
} t_aoo_node;

t_endpoint * aoo_node_endpoint(t_aoo_node *x,
                               const struct sockaddr_storage *sa, socklen_t len)
{
    pthread_mutex_lock(&x->x_endpointlock);
    t_endpoint *ep = endpoint_find(x->x_endpoints, sa);
    if (!ep){
        // add endpoint
        ep = endpoint_new(&x->x_socket, sa, len);
        ep->next = x->x_endpoints;
        x->x_endpoints = ep;
    }
    pthread_mutex_unlock(&x->x_endpointlock);
    return ep;
}

static t_peer * aoo_node_dofind_peer(t_aoo_node *x, t_symbol *group, t_symbol *user)
{
    for (int i = 0; i < x->x_numpeers; ++i){
        t_peer *p = &x->x_peers[i];
        if (p->group == group && p->user == user){
            return p;
        }
    }
    return 0;
}

t_endpoint * aoo_node_find_peer(t_aoo_node *x, t_symbol *group, t_symbol *user)
{
    t_peer *p = aoo_node_dofind_peer(x, group, user);
    return p ? p->endpoint : 0;
}

void aoo_node_add_peer(t_aoo_node *x, t_symbol *group, t_symbol *user,
                       const struct sockaddr *sa, socklen_t len)
{
    if (aoo_node_dofind_peer(x, group, user)){
        bug("aoo_node_add_peer");
        return;
    }

    t_endpoint *e = aoo_node_endpoint(x, (const struct sockaddr_storage *)sa, len);

    if (x->x_peers){
        x->x_peers = resizebytes(x->x_peers, x->x_numpeers * sizeof(t_peer),
                                 (x->x_numpeers + 1) * sizeof(t_peer));
    } else {
        x->x_peers = getbytes(sizeof(t_peer));
    }
    t_peer *p = &x->x_peers[x->x_numpeers++];
    p->group = group;
    p->user = user;
    p->endpoint = e;
}

void aoo_node_remove_peer(t_aoo_node *x, t_symbol *group, t_symbol *user)
{
    t_peer *p = aoo_node_dofind_peer(x, group, user);
    if (!p){
        bug("aoo_node_remove_peer");
        return;
    }
    if (x->x_numpeers > 1){
        int index = p - x->x_peers;
        memmove(p, p + 1, (x->x_numpeers - index - 1) * sizeof(t_peer));
        x->x_peers = resizebytes(x->x_peers, x->x_numpeers * sizeof(t_peer),
                                 (x->x_numpeers - 1) * sizeof(t_peer));
    } else {
        freebytes(x->x_peers, sizeof(t_peer));
        x->x_peers = 0;
    }
    x->x_numpeers--;
}

void aoo_node_remove_group(t_aoo_node *x, t_symbol *group)
{
    if (x->x_peers){
        // remove all sinks matching endpoint
        int n = x->x_numpeers;
        t_peer *end = x->x_peers + n;
        for (t_peer *p = x->x_peers; p != end; ){
            if (p->group == group){
                memmove(p, p + 1, (end - p - 1) * sizeof(t_peer));
                end--;
            } else {
                p++;
            }
        }
        int newsize = end - x->x_peers;
        if (newsize > 0){
            x->x_peers = (t_peer *)resizebytes(x->x_peers,
                n * sizeof(t_peer), newsize * sizeof(t_peer));
        } else {
            freebytes(x->x_peers, n * sizeof(t_peer));
            x->x_peers = 0;
        }
        x->x_numpeers = newsize;
    }
}

void aoo_node_remove_all_peers(t_aoo_node *x)
{
    if (x->x_peers){
        freebytes(x->x_peers, x->x_numpeers * sizeof(t_peer));
        x->x_peers = 0;
        x->x_numpeers = 0;
    }
}

int aoo_node_socket(t_aoo_node *x)
{
    return x->x_socket;
}

int aoo_node_port(t_aoo_node *x)
{
    return x->x_port;
}

void aoo_node_notify(t_aoo_node *x)
{
#if !AOO_NODE_POLL
    pthread_cond_signal(&x->x_condition);
#endif
}

int32_t aoo_node_sendto(t_aoo_node *x, const char *buf, int32_t size,
                        const struct sockaddr *addr)
{
    int result = socket_sendto(x->x_socket, buf, size, addr);
    return result;
}

void aoo_node_dosend(t_aoo_node *x)
{
    aoo_lock_lock_shared(&x->x_clientlock);

    for (int i = 0; i < x->x_numclients; ++i){
        t_client *c = &x->x_clients[i];
        if (pd_class(c->c_obj) == aoo_receive_class){
            aoo_receive_send((t_aoo_receive *)c->c_obj);
        } else if (pd_class(c->c_obj) == aoo_send_class){
            aoo_send_send((t_aoo_send *)c->c_obj);
        } else if (pd_class(c->c_obj) == aoo_client_class){
            aoo_client_send((t_aoo_client *)c->c_obj);
        } else {
            fprintf(stderr, "bug: aoo_node_send\n");
            fflush(stderr);
        }
    }

    aoo_lock_unlock_shared(&x->x_clientlock);
}

void aoo_node_doreceive(t_aoo_node *x)
{
    struct sockaddr_storage sa;
    socklen_t len;
    char buf[AOO_MAXPACKETSIZE];
    int nbytes = socket_receive(x->x_socket, buf, AOO_MAXPACKETSIZE, &sa, &len, 0);
    if (nbytes > 0){
        // try to find endpoint
        pthread_mutex_lock(&x->x_endpointlock);
        t_endpoint *ep = endpoint_find(x->x_endpoints, &sa);
        if (!ep){
            // add endpoint
            ep = endpoint_new(&x->x_socket, &sa, len);
            ep->next = x->x_endpoints;
            x->x_endpoints = ep;
        }
        pthread_mutex_unlock(&x->x_endpointlock);
        // get sink ID
        int32_t type, id;
        if ((aoo_parse_pattern(buf, nbytes, &type, &id) > 0)
            || (aoonet_parse_pattern(buf, nbytes, &type) > 0))
        {
            aoo_lock_lock_shared(&x->x_clientlock);
            if (type == AOO_TYPE_SINK){
                // forward OSC packet to matching receiver(s)
                for (int i = 0; i < x->x_numclients; ++i){
                    if ((pd_class(x->x_clients[i].c_obj) == aoo_receive_class) &&
                        ((id == AOO_ID_WILDCARD) || (id == x->x_clients[i].c_id)))
                    {
                        t_aoo_receive *rcv = (t_aoo_receive *)x->x_clients[i].c_obj;
                        aoo_receive_handle_message(rcv, buf, nbytes,
                            ep, (aoo_replyfn)endpoint_send);
                        if (id != AOO_ID_WILDCARD)
                            break;
                    }
                }
            } else if (type == AOO_TYPE_SOURCE){
                // forward OSC packet to matching senders(s)
                for (int i = 0; i < x->x_numclients; ++i){
                    if ((pd_class(x->x_clients[i].c_obj) == aoo_send_class) &&
                        ((id == AOO_ID_WILDCARD) || (id == x->x_clients[i].c_id)))
                    {
                        t_aoo_send *snd = (t_aoo_send *)x->x_clients[i].c_obj;
                        aoo_send_handle_message(snd, buf, nbytes,
                            ep, (aoo_replyfn)endpoint_send);
                        if (id != AOO_ID_WILDCARD)
                            break;
                    }
                }
            } else if (type == AOO_TYPE_CLIENT || type == AOO_TYPE_PEER){
                // forward OSC packet to matching client
                for (int i = 0; i < x->x_numclients; ++i){
                    if (pd_class(x->x_clients[i].c_obj) == aoo_client_class)
                    {
                        t_aoo_client *c = (t_aoo_client *)x->x_clients[i].c_obj;
                        aoo_client_handle_message(c, buf, nbytes,
                            ep, (aoo_replyfn)endpoint_send);
                        break;
                    }
                }
            } else if (type == AOO_TYPE_SERVER){
                // ignore
            } else {
                fprintf(stderr, "bug: unknown aoo type\n");
                fflush(stderr);
            }
            aoo_lock_unlock_shared(&x->x_clientlock);
        #if !AOO_NODE_POLL
            // notify send thread
            pthread_cond_signal(&x->x_condition);
        #endif
        } else {
            // not a valid AoO OSC message
            fprintf(stderr, "aoo_node: not a valid AOO message!\n");
            fflush(stderr);
        }
    } else if (nbytes < 0){
        // ignore errors when quitting
        if (!x->x_quit){
            socket_error_print("recv");
        }
    }
}

#if AOO_NODE_POLL

static void* aoo_node_thread(void *y)
{
    t_aoo_node *x = (t_aoo_node *)y;

    lower_thread_priority();

    while (!x->x_quit){
        struct pollfd p;
        p.fd = x->x_socket;
        p.revents = 0;
        p.events = POLLIN;

        int result = poll(&p, 1, 1); // 1 ms timeout
        if (result < 0){
            int err = errno;
            fprintf(stderr, "poll() failed: %s\n", strerror(err));
            break;
        }
        if (result > 0 && (p.revents & POLLIN)){
            aoo_node_doreceive(x);
        }
        aoo_node_dosend(x);
    }

    return 0;
}

#else
static void* aoo_node_send(void *y)
{
    t_aoo_node *x = (t_aoo_node *)y;

    lower_thread_priority();

    pthread_mutex_lock(&x->x_mutex);
    while (!x->x_quit){
        pthread_cond_wait(&x->x_condition, &x->x_mutex);

        aoo_node_dosend(x);
    }
    pthread_mutex_unlock(&x->x_mutex);

    return 0;
}

static void* aoo_node_receive(void *y)
{
    t_aoo_node *x = (t_aoo_node *)y;

    lower_thread_priority();

    while (!x->x_quit){
        aoo_node_doreceive(x);
    }

    return 0;
}
#endif // AOO_NODE_POLL

t_aoo_node* aoo_node_add(int port, t_pd *obj, int32_t id)
{
    // make bind symbol for port number
    char buf[64];
    snprintf(buf, sizeof(buf), "aoo_node %d", port);
    t_symbol *s = gensym(buf);
    t_client client = { obj, id };
    t_aoo_node *x = (t_aoo_node *)pd_findbyclass(s, aoo_node_class);
    if (x){
        // check receiver and add to list
        aoo_lock_lock(&x->x_clientlock);
    #if 1
        for (int i = 0; i < x->x_numclients; ++i){
            if (pd_class(obj) == pd_class(x->x_clients[i].c_obj)
                && id == x->x_clients[i].c_id)
            {
                if (obj == x->x_clients[i].c_obj){
                    bug("aoo_node_add: client already added!");
                } else {
                    if (pd_class(obj) == aoo_client_class){
                        pd_error(obj, "%s on port %d already exists!",
                                 classname(obj), port);
                    } else {
                        pd_error(obj, "%s with ID %d on port %d already exists!",
                                 classname(obj), id, port);
                    }
                }
                aoo_lock_unlock(&x->x_clientlock);
                return 0;
            }
        }
    #endif
        x->x_clients = (t_client *)resizebytes(x->x_clients, sizeof(t_client) * x->x_numclients,
                                                sizeof(t_client) * (x->x_numclients + 1));
        x->x_clients[x->x_numclients] = client;
        x->x_numclients++;
        aoo_lock_unlock(&x->x_clientlock);
    } else {
        // make new aoo node

        // first create socket
        int sock = socket_udp();
        if (sock < 0){
            socket_error_print("socket");
            return 0;
        }

        // bind socket to given port
        if (socket_bind(sock, port) < 0){
            pd_error(obj, "%s: couldn't bind to port %d", classname(obj), port);
            socket_close(sock);
            return 0;
        }

        // increase send buffer size to 65 kB
        socket_setsendbufsize(sock, 2 << 15);
        // increase receive buffer size to 2 MB
        socket_setrecvbufsize(sock, 2 << 20);

        // now create aoo node instance
        x = (t_aoo_node *)getbytes(sizeof(t_aoo_node));
        x->x_pd = aoo_node_class;
        x->x_sym = s;
        pd_bind(&x->x_pd, s);

        // add receiver
        x->x_clients = (t_client *)getbytes(sizeof(t_client));
        x->x_clients[0] = client;
        x->x_numclients = 1;

        x->x_peers = 0;
        x->x_numpeers = 0;

        x->x_socket = sock;
        x->x_port = port;
        x->x_endpoints = 0;

        // start threads
        x->x_quit = 0;
        aoo_lock_init(&x->x_clientlock);
    #if AOO_NODE_POLL
        pthread_create(&x->x_thread, 0, aoo_node_thread, x);
    #else
        pthread_mutex_init(&x->x_mutex, 0);
        pthread_cond_init(&x->x_condition, 0);

        pthread_create(&x->x_sendthread, 0, aoo_node_send, x);
        pthread_create(&x->x_receivethread, 0, aoo_node_receive, x);
    #endif

        verbose(0, "new aoo node on port %d", x->x_port);
    }
    return x;
}

void aoo_node_release(t_aoo_node *x, t_pd *obj, int32_t id)
{
    if (x->x_numclients > 1){
        // just remove receiver from list
        aoo_lock_lock(&x->x_clientlock);
        int n = x->x_numclients;
        for (int i = 0; i < n; ++i){
            if (obj == x->x_clients[i].c_obj){
                if (id != x->x_clients[i].c_id){
                    bug("aoo_node_remove: wrong ID!");
                    aoo_lock_unlock(&x->x_clientlock);
                    return;
                }
                memmove(&x->x_clients[i], &x->x_clients[i + 1], (n - i - 1) * sizeof(t_client));
                x->x_clients = (t_client *)resizebytes(x->x_clients, n * sizeof(t_client),
                                                        (n - 1) * sizeof(t_client));
                x->x_numclients--;
                aoo_lock_unlock(&x->x_clientlock);
                return;
            }
        }
        bug("aoo_node_release: %s not found!", classname(obj));
        aoo_lock_unlock(&x->x_clientlock);
    } else if (x->x_numclients == 1){
        // last instance
        pd_unbind(&x->x_pd, x->x_sym);

        // tell the threads that we're done
    #if AOO_NODE_POLL
        // don't bother waking up the thread...
        // just set the flag and wait
        x->x_quit = 1;
        pthread_join(x->x_thread, 0);

        socket_close(x->x_socket);
    #else
        pthread_mutex_lock(&x->x_mutex);
        x->x_quit = 1;
        pthread_mutex_unlock(&x->x_mutex);

        // notify send thread
        pthread_cond_signal(&x->x_condition);

        // try to wake up receive thread
        aoo_lock_lock(&x->x_clientlock);
        int didit = socket_signal(x->x_socket, x->x_port);
        if (!didit){
            // force wakeup by closing the socket.
            // this is not nice and probably undefined behavior,
            // the MSDN docs explicitly forbid it!
            socket_close(x->x_socket);
        }
        aoo_lock_unlock(&x->x_clientlock);

        // wait for threads
        pthread_join(x->x_sendthread, 0);
        pthread_join(x->x_receivethread, 0);

        if (didit){
            socket_close(x->x_socket);
        }
    #endif
        // free memory
        t_endpoint *e = x->x_endpoints;
        while (e){
            t_endpoint *next = e->next;
            endpoint_free(e);
            e = next;
        }
        if (x->x_clients)
            freebytes(x->x_clients, sizeof(t_client) * x->x_numclients);
        if (x->x_peers)
            freebytes(x->x_peers, sizeof(t_peer) * x->x_numpeers);

        aoo_lock_destroy(&x->x_clientlock);
    #if !AOO_NODE_POLL
        pthread_mutex_destroy(&x->x_mutex);
        pthread_cond_destroy(&x->x_condition);
    #endif
        verbose(0, "released aoo node on port %d", x->x_port);

        freebytes(x, sizeof(*x));
    } else {
        bug("aoo_node_release: negative refcount!");
    }
}

void aoo_node_setup(void)
{
    aoo_node_class = class_new(gensym("aoo socket receiver"), 0, 0,
                                  sizeof(t_aoo_node), CLASS_PD, A_NULL);
}
