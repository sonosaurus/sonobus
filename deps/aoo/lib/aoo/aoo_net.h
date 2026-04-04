/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

// AOONET is an embeddable UDP punch hole library for creating dynamic
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

#include "aoo_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*///////////////////////// OSC ////////////////////////////////*/

#define AOONET_MSG_SERVER "/server"
#define AOONET_MSG_SERVER_LEN 7

#define AOONET_MSG_CLIENT "/client"
#define AOONET_MSG_CLIENT_LEN 7

#define AOONET_MSG_PEER "/peer"
#define AOONET_MSG_PEER_LEN 5

#define AOONET_MSG_PING "/ping"
#define AOONET_MSG_PING_LEN 5

#define AOONET_MSG_LOGIN "/login"
#define AOONET_MSG_LOGIN_LEN 6

#define AOONET_MSG_REQUEST "/request"
#define AOONET_MSG_REQUEST_LEN 8

#define AOONET_MSG_REPLY "/reply"
#define AOONET_MSG_REPLY_LEN 6

#define AOONET_MSG_GROUP "/group"
#define AOONET_MSG_GROUP_LEN 6

#define AOONET_MSG_PUBLIC "/public"
#define AOONET_MSG_PUBLIC_LEN 7

#define AOONET_MSG_ADD "/add"
#define AOONET_MSG_ADD_LEN 4

#define AOONET_MSG_DEL "/del"
#define AOONET_MSG_DEL_LEN 4

#define AOONET_MSG_JOIN "/join"
#define AOONET_MSG_JOIN_LEN 5

#define AOONET_MSG_LEAVE "/leave"
#define AOONET_MSG_LEAVE_LEN 6

typedef enum aoonet_type
{
    AOO_TYPE_SERVER = 1000,
    AOO_TYPE_CLIENT,
    AOO_TYPE_PEER
} aoonet_type;

// get the aoonet_type and ID from an AoO OSC message, e.g. in /aoo/server/ping
// returns 1 on success, 0 on fail
AOO_API int32_t aoonet_parse_pattern(const char *msg, int32_t n, int32_t *type);

/*///////////////////////// AOO events///////////////////////////*/

typedef enum aoonet_event_type
{
    // client events
    AOONET_CLIENT_ERROR_EVENT = 0,
    AOONET_CLIENT_PING_EVENT,
    AOONET_CLIENT_CONNECT_EVENT,
    AOONET_CLIENT_DISCONNECT_EVENT,
    AOONET_CLIENT_GROUP_JOIN_EVENT,
    AOONET_CLIENT_GROUP_LEAVE_EVENT,
    AOONET_CLIENT_GROUP_PUBLIC_ADD_EVENT,
    AOONET_CLIENT_GROUP_PUBLIC_DEL_EVENT,
    AOONET_CLIENT_PEER_PREJOIN_EVENT,
    AOONET_CLIENT_PEER_JOIN_EVENT,
    AOONET_CLIENT_PEER_JOINFAIL_EVENT,
    AOONET_CLIENT_PEER_LEAVE_EVENT,
    // server events
    AOONET_SERVER_ERROR_EVENT = 1000,
    AOONET_SERVER_PING_EVENT,
    AOONET_SERVER_USER_ADD_EVENT,
    AOONET_SERVER_USER_REMOVE_EVENT,
    AOONET_SERVER_USER_JOIN_EVENT,
    AOONET_SERVER_USER_LEAVE_EVENT,
    AOONET_SERVER_GROUP_ADD_EVENT,
    AOONET_SERVER_GROUP_REMOVE_EVENT,
    AOONET_SERVER_GROUP_JOIN_EVENT,
    AOONET_SERVER_GROUP_LEAVE_EVENT
} aoonet_event_type;

#define AOONET_REPLY_EVENT  \
    int32_t type;           \
    int32_t result;         \
    const char *errormsg;   \

typedef struct aoonet_reply_event
{
    AOONET_REPLY_EVENT
} aoonet_reply_event;

#define aoonet_server_event aoonet_reply_event

typedef struct aoonet_server_user_event
{
    int32_t type;
    const char *name;
} aoonet_server_user_event;

typedef struct aoonet_server_group_event
{
    int32_t type;
    const char *group;
    const char *user;
} aoonet_server_group_event;

#define aoonet_client_event aoonet_reply_event

typedef struct aoonet_client_group_event
{
    AOONET_REPLY_EVENT
    const char *name;
} aoonet_client_group_event;

typedef struct aoonet_client_peer_event
{
    AOONET_REPLY_EVENT
    const char *group;
    const char *user;
    void *address;
    int32_t length;
} aoonet_client_peer_event;


/*///////////////////////// AOO server /////////////////////////*/

#ifdef __cplusplus
namespace aoo {
namespace net {
    class iserver;
} // net
} // aoo
using aoonet_server = aoo::net::iserver;
#else
typedef struct aoonet_server aoonet_server;
#endif

// create a new AOO server instance, listening on the given port
AOO_API aoonet_server * aoonet_server_new(int port, int32_t *err);

// destroy AOO server instance
AOO_API void aoonet_server_free(aoonet_server *server);

// run the AOO server; this function blocks indefinitely.
AOO_API int32_t aoonet_server_run(aoonet_server *server);

// quit the AOO server from another thread
AOO_API int32_t aoonet_server_quit(aoonet_server *server);

// get number of pending events (always thread safe)
AOO_API int32_t aoonet_server_events_available(aoonet_server *server);

// handle events (threadsafe, but not reentrant)
// will call the event handler function one or more times
AOO_API int32_t aoonet_server_handle_events(aoonet_server *server,
                                            aoo_eventhandler fn, void *user);

// LATER add methods to add/remove users and groups
// and set/get server options, group options and user options

/*///////////////////////// AOO client /////////////////////////*/

#ifdef __cplusplus
namespace aoo {
namespace net {
    class iclient;
} // net
} // aoo
using aoonet_client = aoo::net::iclient;
#else
typedef struct aoonet_client aoonet_client;
#endif

// create a new AOO client for the given UDP socket
AOO_API aoonet_client * aoonet_client_new(void *udpsocket, aoo_sendfn fn, int port);

// destroy AOO client
AOO_API void aoonet_client_free(aoonet_client *client);

// run the AOO client; this function blocks indefinitely.
AOO_API int32_t aoonet_client_run(aoonet_client *server);

// quit the AOO client from another thread
AOO_API int32_t aoonet_client_quit(aoonet_client *server);

// connect AOO client to a AOO server (always thread safe)
AOO_API int32_t aoonet_client_connect(aoonet_client *client, const char *host, int port,
                                      const char *username, const char *pwd);

// disconnect AOO client from AOO server (always thread safe)
AOO_API int32_t aoonet_client_disconnect(aoonet_client *client);

// join an AOO group
AOO_API int32_t aoonet_client_group_join(aoonet_client *client, const char *group, const char *pwd);

// join/create an AOO public group
AOO_API int32_t aoonet_client_group_join_public(aoonet_client *client, const char *group, const char *pwd);

// leave an AOO group
AOO_API int32_t aoonet_client_group_leave(aoonet_client *client, const char *group);

// leave an AOO group
AOO_API int32_t aoonet_client_group_watch_public(aoonet_client *client, bool watch);

// handle messages from peers (threadsafe, but not reentrant)
// 'addr' should be sockaddr *
AOO_API int32_t aoonet_client_handle_message(aoonet_client *client,
                                             const char *data, int32_t n, void *addr);

// send outgoing messages to peers (threadsafe, but not reentrant)
AOO_API int32_t aoonet_client_send(aoonet_client *client);

// get number of pending events (always thread safe)
AOO_API int32_t aoonet_client_events_available(aoonet_client *client);

// handle events (threadsafe, but not reentrant)
// will call the event handler function one or more times
AOO_API int32_t aoonet_client_handle_events(aoonet_client *client,
                                            aoo_eventhandler fn, void *user);

// LATER add API functions to set options and do additional peer communication (chat, OSC messages, etc.)

#ifdef __cplusplus
} // extern "C"
#endif
