/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

// AOO_NET is an embeddable UDP punch hole library for creating dynamic
// peer-2-peer networks over the public internet. It has been designed
// to seemlessly interoperate with the AOO streaming library.
//
// The implementation is largely based on the techniques described in the paper
// "Peer-to-Peer Communication Across Network Address Translators"
// by Ford, Srisuresh and Kegel (https://bford.info/pub/net/p2pnat/)
//
// It uses TCP over SLIP to reliable exchange meta information between peers.
//
// The UDP punch hole server runs on a public endpoint and manages the public
// and local IP endpoint addresses of all the clients.
// It can host multiple peer-2-peer networks which are organized as called groups.
//
// Each client connects to the server, logs in as a user, joins one ore more groups
// and in turn receives the public and local IP endpoint addresses from its peers.
//
// Currently, users and groups are automatically created on demand, but later
// we might add the possibility to create persistent users and groups on the server.
//
// Later we might add TCP connections between peers, so we can reliably exchange
// additional data, like chat messages or arbitrary OSC messages.
//
// Also we could support sending additional notifications from the server to all clients.

#pragma once

#include "aoo/aoo_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define AOO_NET_MAXNAMELEN 64

/*////////// default values ////////////*/

#ifndef AOO_NET_RELAY_ENABLE
 #define AOO_NET_RELAY_ENABLE 1
#endif

#ifndef AOO_NET_NOTIFY_ON_SHUTDOWN
 #define AOO_NET_NOTIFY_ON_SHUTDOWN 0
#endif

/*///////////////////////// OSC ////////////////////////////////*/

#define AOO_NET_MSG_SERVER "/server"
#define AOO_NET_MSG_SERVER_LEN 7

#define AOO_NET_MSG_CLIENT "/client"
#define AOO_NET_MSG_CLIENT_LEN 7

#define AOO_NET_MSG_PEER "/peer"
#define AOO_NET_MSG_PEER_LEN 5

#define AOO_NET_MSG_RELAY "/relay"
#define AOO_NET_MSG_RELAY_LEN 6

/*///////////////////////// requests/replies ///////////////////////////*/

typedef void (*aoo_net_callback)(void *user, aoo_error result, const void *reply);

typedef struct aoo_net_error_reply {
    const char *error_message;
    int32_t error_code;
} aoo_net_error_reply;

typedef enum aoo_net_request_type {
    AOO_NET_CONNECT_REQUEST = 0,
    AOO_NET_DISCONNECT_REQUEST,
    AOO_NET_GROUP_JOIN_REQUEST,
    AOO_NET_GROUP_LEAVE_REQUEST
} aoo_net_request_type;

typedef struct aoo_net_connect_request {
    const char *host;
    int32_t port;
    const char *user_name;
    const char *user_pwd;
} aoo_net_connect_request;

enum aoo_net_server_flags {
    AOO_NET_SERVER_RELAY = 1,
    AOO_NET_SERVER_LEGACY = 256
};

typedef struct aoo_net_connect_reply {
    aoo_id user_id;
    uint32_t server_flags;
} aoo_net_connect_reply;

typedef struct aoo_net_group_request {
    const char *group_name;
    const char *group_pwd;
} aoo_net_group_request;

/*///////////////////////// events ///////////////////////////*/

typedef enum aoo_net_event_type
{
    // generic events
    AOO_NET_ERROR_EVENT = 0,
    AOO_NET_PING_EVENT,
    // client events
    AOO_NET_DISCONNECT_EVENT,
    AOO_NET_PEER_HANDSHAKE_EVENT,
    AOO_NET_PEER_TIMEOUT_EVENT,
    AOO_NET_PEER_JOIN_EVENT,
    AOO_NET_PEER_LEAVE_EVENT,
    AOO_NET_PEER_MESSAGE_EVENT,
    // server events
    AOO_NET_USER_JOIN_EVENT,
    AOO_NET_USER_LEAVE_EVENT,
    AOO_NET_GROUP_JOIN_EVENT,
    AOO_NET_GROUP_LEAVE_EVENT
} aoo_net_event_type;

typedef struct aoo_net_error_event
{
    int32_t type;
    int32_t error_code;
    const char *error_message;
} aoo_net_error_event;

#define AOO_NET_ENDPOINT_EVENT  \
    int32_t type;               \
    int32_t addrlen;            \
    const void *address;        \

typedef struct aoo_net_ping_event
{
    AOO_NET_ENDPOINT_EVENT
    uint64_t tt1;
    uint64_t tt2;
    uint64_t tt3; // only for clients
} aoo_net_ping_event;

typedef struct aoo_net_peer_event
{
    AOO_NET_ENDPOINT_EVENT
    const char *group_name;
    const char *user_name;
    aoo_id user_id;
    uint32_t flags;
} aoo_net_peer_event;

typedef aoo_net_peer_event aoo_net_group_event;

typedef struct aoo_net_message_event {
    AOO_NET_ENDPOINT_EVENT
    const char *data;
    int32_t size;
} aoo_net_peer_message_event;

typedef struct aoo_net_user_event
{
    AOO_NET_ENDPOINT_EVENT
    const char *user_name;
    aoo_id user_id;
} aoo_net_user_event;

/*///////////////////////// AOO server /////////////////////////*/

#ifdef __cplusplus
namespace aoo {
namespace net {
    class server;
} // net
} // aoo
using aoo_net_server = aoo::net::server;
#else
typedef struct aoo_net_server aoo_net_server;
#endif

// create a new AOO server instance, listening on the given port
AOO_API aoo_net_server * aoo_net_server_new(int port, uint32_t flags, aoo_error *err);

// destroy AOO server instance
AOO_API void aoo_net_server_free(aoo_net_server *server);

// run the AOO server; this function blocks indefinitely.
AOO_API aoo_error aoo_net_server_run(aoo_net_server *server);

// quit the AOO server from another thread
AOO_API aoo_error aoo_net_server_quit(aoo_net_server *server);

// set event handler callback + mode
AOO_API aoo_error aoo_net_server_set_eventhandler(aoo_net_server *sink,
                                                  aoo_eventhandler fn,
                                                  void *user, int32_t mode);

// check for pending events (always thread safe)
AOO_API aoo_bool aoo_net_server_events_available(aoo_net_server *server);

// poll events (threadsafe, but not reentrant)
// will call the event handler function one or more times
AOO_API aoo_error aoo_net_server_poll_events(aoo_net_server *server);

AOO_API aoo_error aoo_net_server_ctl(aoo_net_server *server, int32_t ctl,
                                     intptr_t index, void *p, size_t size);

// ------------------------------------------------------------
// type-safe convenience functions for frequently used controls

// *none*


/*///////////////////////// AOO client /////////////////////////*/

// flags for aoo_net_client_send_message():
enum aoo_net_message_flags
{
    AOO_NET_MESSAGE_RELIABLE = 1
};

typedef struct aoo_net_peer_info
{
    char group_name[AOO_NET_MAXNAMELEN];
    char user_name[AOO_NET_MAXNAMELEN];
    aoo_id user_id;
    uint32_t flags;
    char reserved[56];
} aoo_net_peer_info;

#ifdef __cplusplus
namespace aoo {
namespace net {
    class client;
} // net
} // aoo
using aoo_net_client = aoo::net::client;
#else
typedef struct aoo_net_client aoo_net_client;
#endif

// create a new AOO client for the given UDP port
AOO_API aoo_net_client * aoo_net_client_new(const void *address, int32_t addrlen,
                                            uint32_t flags);

// destroy AOO client
AOO_API void aoo_net_client_free(aoo_net_client *client);

// run the AOO client; this function blocks indefinitely.
AOO_API aoo_error aoo_net_client_run(aoo_net_client *client);

// quit the AOO client from another thread
AOO_API aoo_error aoo_net_client_quit(aoo_net_client *client);

// add AOO source
AOO_API aoo_error aoo_net_client_add_source(aoo_net_client *client,
                                            aoo_source *src, aoo_id id);

// remove AOO source
AOO_API aoo_error aoo_net_client_remove_source(aoo_net_client *client,
                                               aoo_source *src);

// add AOO sink
AOO_API aoo_error aoo_net_client_add_sink(aoo_net_client *client,
                                          aoo_sink *sink, aoo_id id);

// remove AOO sink
AOO_API aoo_error aoo_net_client_remove_sink(aoo_net_client *client,
                                             aoo_sink *sink);

// find peer by name and return its IP endpoint address
// address: pointer to sockaddr_storage
// addrlen: initialized with max. storage size, updated to actual size
AOO_API aoo_error aoo_net_client_get_peer_address(aoo_net_client *client,
                                                  const char *group, const char *user,
                                                  void *address, int32_t *addrlen, uint32_t *flags);

// find peer by its IP address and return additional info
AOO_API aoo_error aoo_net_client_get_peer_info(aoo_net_client *client,
                                               const void *address, int32_t addrlen,
                                               aoo_net_peer_info *info);

// send a request to the AOO server (always thread safe)
AOO_API aoo_error aoo_net_client_request(aoo_net_client *client,
                                         aoo_net_request_type request, void *data,
                                         aoo_net_callback callback, void *user);

// send a message to one or more peers
// 'addr' + 'len' accept the following values:
// a) 'struct sockaddr *' + 'socklen_t': send to a single peer
// b) 'const char *' (group name) + 0: send to all peers of a specific group
// c) 'NULL' + 0: send to all peers
// the 'flags' parameter allows for (future) additional settings
AOO_API aoo_error aoo_net_client_send_message(aoo_net_client *client,
                                              const char *data, int32_t size,
                                              const void *addr, int32_t len, int32_t flags);

// handle messages from peers (threadsafe, called from a network thread)
// 'addr' should be sockaddr *
AOO_API aoo_error aoo_net_client_handle_message(aoo_net_client *client,
                                                const char *data, int32_t nbytes,
                                                const void *addr, int32_t addrlen);

// send outgoing messages (threadsafe, called from a network thread)
AOO_API aoo_error aoo_net_client_send(aoo_net_client *client, aoo_sendfn fn, void *user);

// set event handler callback + mode
AOO_API aoo_error aoo_net_client_set_eventhandler(aoo_net_client *sink,
                                                  aoo_eventhandler fn,
                                                  void *user, int32_t mode);

// check for pending events (always thread safe)
AOO_API aoo_bool aoo_net_client_events_available(aoo_net_client *client);

// handle events (threadsafe, but not reentrant)
// will call the event handler function one or more times
AOO_API aoo_error aoo_net_client_poll_events(aoo_net_client *client);

// client controls (always threadsafe)
AOO_API aoo_error aoo_net_client_ctl(aoo_net_server *server, int32_t ctl,
                                     intptr_t index, void *p, size_t size);

// ------------------------------------------------------------
// type-safe convenience functions for frequently used controls

// *none*

// ------------------------------------------------------------
// type-safe convenience functions for frequently used requests

// connect to AOO server (always thread safe)
static inline aoo_error aoo_net_client_connect(aoo_net_client *client,
                                               const char *host, int port,
                                               const char *name, const char *pwd,
                                               aoo_net_callback cb, void *user)
{
    aoo_net_connect_request data = { host, port, name, pwd };
    return aoo_net_client_request(client, AOO_NET_CONNECT_REQUEST, &data, cb, user);
}

// disconnect from AOO server (always thread safe)
static inline aoo_error aoo_net_client_disconnect(aoo_net_client *client,
                                                  aoo_net_callback cb, void *user)
{
    return aoo_net_client_request(client, AOO_NET_DISCONNECT_REQUEST, NULL, cb, user);
}

// join an AOO group
static inline aoo_error aoo_net_client_group_join(aoo_net_client *client,
                                                  const char *group, const char *pwd,
                                                  aoo_net_callback cb, void *user)
{
    aoo_net_group_request data = { group, pwd };
    return aoo_net_client_request(client, AOO_NET_GROUP_JOIN_REQUEST, &data, cb, user);
}

// leave an AOO group
static inline aoo_error aoo_net_client_group_leave(aoo_net_client *client, const char *group,
                                                   aoo_net_callback cb, void *user)
{
    aoo_net_group_request data = { group, NULL };
    return aoo_net_client_request(client, AOO_NET_GROUP_LEAVE_REQUEST, &data, cb, user);
}

#ifdef __cplusplus
} // extern "C"
#endif
