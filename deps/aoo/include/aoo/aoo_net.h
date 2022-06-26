/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/** \file
 * \brief AOO NET
 *
 * AOO NET is an embeddable UDP punch hole library for creating dynamic
 * peer-2-peer networks over the public internet. It has been designed
 * to seemlessly interoperate with the AOO streaming library.
 *
 * The implementation is largely based on the techniques described in the paper
 * "Peer-to-Peer Communication Across Network Address Translators"
 * by Ford, Srisuresh and Kegel (https://bford.info/pub/net/p2pnat/)
 *
 * It uses TCP over SLIP to reliable exchange meta information between peers.
 *
 * The UDP punch hole server runs on a public endpoint and manages the public
 * and local IP endpoint addresses of all the clients.
 * It can host multiple peer-2-peer networks which are organized as groups.
 *
 * Each client connects to the server, logs in as a user, joins one ore more groups
 * and in turn receives the public and local IP endpoint addresses from its peers.
 *
 * Currently, users and groups are automatically created on demand, but later
 * we might add the possibility to create persistent users and groups on the server.
 *
 * Later we might add TCP connections between peers, so we can reliably exchange
 * additional data, like chat messages or arbitrary OSC messages.
 *
 * Also we could support sending additional notifications from the server to all clients.
 */

#pragma once

#include "aoo_defines.h"

AOO_PACK_BEGIN

/*------------- compile time settings ---------------*/

/** \brief debug relay messages */
#ifndef AOO_DEBUG_RELAY
#define AOO_DEBUG_RELAY 0
#endif

/** \brief debug client messages */
#ifndef AOO_DEBUG_CLIENT_MESSAGE
#define AOO_DEBUG_CLIENT_MESSAGE 0
#endif

/*-------------- default values ---------------------*/

/** \brief enable/disable server relay by default */
#ifndef AOO_NET_SERVER_RELAY
 #define AOO_NET_SERVER_RELAY 0
#endif

/** \brief enable/disable automatic group creation by default */
#ifndef AOO_NET_GROUP_AUTO_CREATE
#define AOO_NET_GROUP_AUTO_CREATE 1
#endif

/** \brief enable/disable automatic user creation by default */
#ifndef AOO_NET_USER_AUTO_CREATE
#define AOO_NET_USER_AUTO_CREATE 1
#endif

/** \brief enable/disable binary format for client messages */
#ifndef AOO_NET_CLIENT_BINARY_MSG
 #define AOO_NET_CLIENT_BINARY_MSG 1
#endif

/*-------------- public OSC interface ---------------*/

#define kAooNetMsgServer "/server"
#define kAooNetMsgServerLen 7

#define kAooNetMsgClient "/client"
#define kAooNetMsgClientLen 7

#define kAooNetMsgPeer "/peer"
#define kAooNetMsgPeerLen 5

#define kAooNetMsgRelay "/relay"
#define kAooNetMsgRelayLen 6

#define kAooNetMsgPing "/ping"
#define kAooNetMsgPingLen 5

#define kAooNetMsgReply "/r"
#define kAooNetMsgReplyLen 2

#define kAooNetMsgMessage "/msg"
#define kAooNetMsgMessageLen 4

#define kAooNetMsgAck "/ack"
#define kAooNetMsgAckLen 4

#define kAooNetMsgLogin "/login"
#define kAooNetMsgLoginLen 6

#define kAooNetMsgQuery "/query"
#define kAooNetMsgQueryLen 6

#define kAooNetMsgGroup "/group"
#define kAooNetMsgGroupLen 6

#define kAooNetMsgUser "/user"
#define kAooNetMsgUserLen 5

#define kAooNetMsgJoin "/join"
#define kAooNetMsgJoinLen 5

#define kAooNetMsgLeave "/leave"
#define kAooNetMsgLeaveLen 6

#define kAooNetMsgUpdate "/update"
#define kAooNetMsgUpdateLen 7

#define kAooNetMsgChanged "/changed"
#define kAooNetMsgChangedLen 8

#define kAooNetMsgRequest "/request"
#define kAooNetMsgRequestLen 8

/*------------- defines --------------------------*/

/** \brief used in AooClient_sendMessage / AooClient::sendMessage */
enum AooNetMessageFlags
{
    kAooNetMessageReliable = 0x01
};

/** \brief server flags */
enum AooNetServerFlags
{
    kAooNetServerRelay = 0x01
};

/*------------- requests/replies ----------------*/

/** \brief type for client requests */
typedef AooInt32 AooNetRequestType;

/** \brief client request constants */
enum AooNetRequestTypes
{
    /** error response */
    kAooNetRequestError = 0,
    /** connect to server */
    kAooNetRequestConnect,
    /** query public IP + server IP */
    kAooNetRequestQuery,
    /** login to server */
    kAooNetRequestLogin,
    /** disconnect from server */
    kAooNetRequestDisconnect,
    /** join group */
    kAooNetRequestGroupJoin,
    /** leave group */
    kAooNetRequestGroupLeave,
    /** update group */
    kAooNetRequestGroupUpdate,
    /** update user */
    kAooNetRequestUserUpdate,
    /** custom request */
    kAooNetRequestCustom
};

/** \brief generic request */
typedef struct AooNetRequest
{
    /** the request type */
    AooNetRequestType type;
} AooNetRequest;

/** \brief request handler (to intercept client requests)
 * \param user user data
 * \param client client ID
 * \param token request token
 * \param request the client request
 * \return #kAooTrue: handled manually; #kAooFalse: handle automatically.
 */
typedef AooBool (AOO_CALL *AooNetRequestHandler)(
        void *user,
        AooId client,
        AooId token,
        const AooNetRequest *request
);

/** \brief generic response */
typedef struct AooNetResponse
{
    /** the response type */
    AooNetRequestType type;
    AooFlag flags;
} AooNetResponse;

/** \brief callback for client requests */
typedef void (AOO_CALL *AooNetCallback)(
        /** the user data */
        void *user,
        /** the original request */
        const AooNetRequest *request,
        /** the result */
        AooError result,
        /** the response data */
        const AooNetResponse *response
);

/* error */

/** \brief error response */
typedef struct AooNetResponseError
{
    AooNetRequestType type;
    AooFlag flags;
    /** discriptive error message */
    const AooChar *errorMessage;
    /** platform- or user-specific error code */
    AooInt32 errorCode;
} AooNetResponseError;

/* connect (client-side) */

/** \brief login request */
typedef struct AooNetRequestConnect
{
    AooNetRequestType type;
    AooFlag flags;
    AooIpEndpoint address;
    const AooChar *password;
    const AooDataView *metadata;
} AooNetRequestConnect;

/** \brief login response */
typedef struct AooNetResponseConnect
{
    AooNetRequestType type;
    AooFlag flags;
    AooId clientId; /* client-only */
    const AooDataView *metadata;
} AooNetResponseConnect;

/* query (server-side) */

/** \brief query request */
typedef struct AooNetRequestQuery
{
    AooNetRequestType type;
    AooFlag flags;
    AooSockAddr replyAddr;
    AooSendFunc replyFunc;
    void *replyContext;
} AooNetRequestQuery;

/** \brief query response */
typedef struct AooNetResponseQuery
{
    AooNetRequestType type;
    AooFlag flags;
    AooIpEndpoint serverAddress;
} AooNetResponseQuery;

/* login (server-side) */

/** \brief login request */
typedef struct AooNetRequestLogin
{
    AooNetRequestType type;
    AooFlag flags;
    const AooChar *password;
    const AooDataView *metadata;
} AooNetRequestLogin;

/** \brief login response */
typedef struct AooNetResponseLogin
{
    AooNetRequestType type;
    AooFlag flags;
    const AooDataView *metadata;
} AooNetResponseLogin;

/* join group (server/client) */

/** \brief request for joining a group */
typedef struct AooNetRequestGroupJoin
{
    AooNetRequestType type;
    AooFlag flags;
    /* group */
    const AooChar *groupName;
    const AooChar *groupPwd;
    AooId groupId; /* kAooIdInvalid if group does not exist (yet) */
    AooFlag groupFlags;
    /** The client who creates the group may provide group metadata
     * in AooClient::joinGroup(). By default, the server just stores
     * the metadata and sends it to all subsequent users via this field.
     * However, it may also intercept the request and validate/modify the
     * metadata, or provide any kind of metadata it wants, by setting
     * AooNetResponseGroupJoin::groupMetadata. */
    const AooDataView *groupMetadata;
    /* user */
    const AooChar *userName;
    const AooChar *userPwd;
    AooId userId; /* kAooIdInvalid if user does not exist (yet) */
    AooFlag userFlags;
    /** Each client may provide user metadata in AooClient::joinGroup().
     * By default, the server just stores the metadata and sends it to all
     * peers via AooNetEventPeer::metadata. However, it may also intercept
     * the request and validate/modify the metadata, or provide any kind of
     * metadata it wants, by setting AooNetResponseGroupJoin::userMetadata. */
    const AooDataView *userMetadata;
    /* other */
    /** (optional) Relay address provided by the user/client. The server will
     * forward it to all peers. */
    const AooIpEndpoint *relayAddress;
} AooNetRequestGroupJoin;

/** \brief response for joining a group */
typedef struct AooNetResponseGroupJoin
{
    AooNetRequestType type;
    AooFlag flags;
    /* group */
    /** group ID generated by the server */
    AooId groupId;
    AooFlag groupFlags;
    /** (optional) group metadata validated/modified by the server. */
    const AooDataView *groupMetadata;
    /* user */
    /** user Id generated by the server */
    AooId userId;
    AooFlag userFlags;
    /** (optional) user metadata validated/modified by the server. */
    const AooDataView *userMetadata;
    /* other */
    /** (optional) private metadata that is only sent to the client.
     * For example, this can be used for state synchronization. */
    const AooDataView *privateMetadata;
    /** (optional) relay address provided by the server.
      * For example, the AOO server might provide a group with a
      * dedicated UDP relay server. */
    const AooIpEndpoint *relayAddress;
} AooNetResponseGroupJoin;

/* leave group (server/client) */

/** \brief request for leaving a group */
typedef struct AooNetRequestGroupLeave
{
    AooNetRequestType type;
    AooFlag flags;
    AooId group;
} AooNetRequestGroupLeave;

#define AooNetResponseGroupLeave AooNetResponse

/* update group metadata */

/** \brief request for updating a group */
typedef struct AooNetRequestGroupUpdate
{
    AooNetRequestType type;
    AooFlag flags;
    AooId groupId;
    AooDataView groupMetadata;
} AooNetRequestGroupUpdate;

/** \brief response for updating a group */
typedef struct AooNetResponseGroupUpdate
{
    AooNetRequestType type;
    AooFlag flags;
    AooDataView groupMetadata;
} AooNetResponseGroupUpdate;

/* update user metadata */

/** \brief request for updating a user */
typedef struct AooNetRequestUserUpdate
{
    AooNetRequestType type;
    AooFlag flags;
    AooId groupId;
    AooId userId;
    AooDataView userMetadata;
} AooNetRequestUserUpdate;

/** \brief response for updating a user */
typedef struct AooNetResponseUserUpdate
{
    AooNetRequestType type;
    AooFlag flags;
    AooDataView userMetadata;
} AooNetResponseUserUpdate;

/* custom request (server/client) */

/** \brief custom client request */
typedef struct AooNetRequestCustom
{
    AooNetRequestType type;
    AooFlag flags;
    AooDataView data;
} AooNetRequestCustom;

/** \brief custom server response */
typedef struct AooNetResponseCustom
{
    AooNetRequestType type;
    AooFlag flags;
    AooDataView data;
} AooNetResponseCustom;

/*--------------------------------------------*/

AOO_PACK_END
