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
#ifndef AOO_SERVER_RELAY
 #define AOO_SERVER_RELAY 0
#endif

/** \brief enable/disable automatic group creation by default */
#ifndef AOO_GROUP_AUTO_CREATE
#define AOO_GROUP_AUTO_CREATE 1
#endif

/** \brief enable/disable automatic user creation by default */
#ifndef AOO_USER_AUTO_CREATE
#define AOO_USER_AUTO_CREATE 1
#endif

/** \brief enable/disable binary format for client messages */
#ifndef AOO_CLIENT_BINARY_MSG
 #define AOO_CLIENT_BINARY_MSG 1
#endif

/*-------------- public OSC interface ---------------*/

#define kAooMsgServer "/server"
#define kAooMsgServerLen 7

#define kAooMsgClient "/client"
#define kAooMsgClientLen 7

#define kAooMsgPeer "/peer"
#define kAooMsgPeerLen 5

#define kAooMsgRelay "/relay"
#define kAooMsgRelayLen 6

#define kAooMsgPong "/pong"
#define kAooMsgReplyLen 5

#define kAooMsgMessage "/msg"
#define kAooMsgMessageLen 4

#define kAooMsgAck "/ack"
#define kAooMsgAckLen 4

#define kAooMsgLogin "/login"
#define kAooMsgLoginLen 6

#define kAooMsgQuery "/query"
#define kAooMsgQueryLen 6

#define kAooMsgGroup "/group"
#define kAooMsgGroupLen 6

#define kAooMsgUser "/user"
#define kAooMsgUserLen 5

#define kAooMsgJoin "/join"
#define kAooMsgJoinLen 5

#define kAooMsgLeave "/leave"
#define kAooMsgLeaveLen 6

#define kAooMsgUpdate "/update"
#define kAooMsgUpdateLen 7

#define kAooMsgChanged "/changed"
#define kAooMsgChangedLen 8

#define kAooMsgRequest "/request"
#define kAooMsgRequestLen 8

/*------------- defines --------------------------*/

/** \brief used in AooClient_sendMessage / AooClient::sendMessage */
enum AooMessageFlags
{
    kAooMessageReliable = 0x01
};

/** \brief server flags */
enum AooServerFlags
{
    kAooServerRelay = 0x01
};

/*------------- requests/replies ----------------*/

/** \brief type for client requests */
typedef AooInt32 AooRequestType;

/** \brief client request type constants */
enum AooRequestTypes
{
    /** error response */
    kAooRequestError = 0,
    /** connect to server */
    kAooRequestConnect,
    /** query public IP + server IP */
    kAooRequestQuery,
    /** login to server */
    kAooRequestLogin,
    /** disconnect from server */
    kAooRequestDisconnect,
    /** join group */
    kAooRequestGroupJoin,
    /** leave group */
    kAooRequestGroupLeave,
    /** update group */
    kAooRequestGroupUpdate,
    /** update user */
    kAooRequestUserUpdate,
    /** custom request */
    kAooRequestCustom
};

typedef union AooRequest AooRequest;

/** \brief request handler (to intercept client requests)
 * \param user user data
 * \param client client ID
 * \param token request token
 * \param request the client request
 * \return #kAooTrue: handled manually; #kAooFalse: handle automatically.
 */
typedef AooBool (AOO_CALL *AooRequestHandler)(
        void *user,
        AooId client,
        AooId token,
        const AooRequest *request
);

typedef union AooResponse AooResponse;

/** \brief callback for client requests */
typedef void (AOO_CALL *AooResponseHandler)(
        /** the user data */
        void *user,
        /** the original request */
        const AooRequest *request,
        /** the result */
        AooError result,
        /** the response data */
        const AooResponse *response
);

/* generic request/response */

typedef struct AooRequestBase
{
    AooRequestType type;
    AooFlag flags;
} AooRequestBase;

typedef AooRequestBase AooResponseBase;

/* error */

/** \brief error response */
typedef struct AooResponseError
{
    AooRequestType type;
    AooFlag flags;
    /** platform- or user-specific error code */
    AooInt32 errorCode;
    /** discriptive error message */
    const AooChar *errorMessage;
} AooResponseError;

/* connect (client-side) */

/** \brief connect request */
typedef struct AooRequestConnect
{
    AooRequestType type;
    AooFlag flags;
    AooIpEndpoint address;
    const AooChar *password;
    const AooData *metadata;
} AooRequestConnect;

/** \brief connect response */
typedef struct AooResponseConnect
{
    AooRequestType type;
    AooFlag flags;
    AooId clientId; /* client-only */
    const AooData *metadata;
} AooResponseConnect;

/* disconnect (client-side) */
typedef AooRequestBase AooRequestDisconnect;
typedef AooResponseBase AooResponseDisconnect;

/* query (server-side) */

/** \brief query request */
typedef struct AooRequestQuery
{
    AooRequestType type;
    AooFlag flags;
    AooSockAddr replyAddr;
    AooSendFunc replyFunc;
    void *replyContext;
} AooRequestQuery;

/** \brief query response */
typedef struct AooResponseQuery
{
    AooRequestType type;
    AooFlag flags;
    AooIpEndpoint serverAddress;
} AooResponseQuery;

/* login (server-side) */

/** \brief login request */
typedef struct AooRequestLogin
{
    AooRequestType type;
    AooFlag flags;
    const AooChar *password;
    const AooData *metadata;
} AooRequestLogin;

/** \brief login response */
typedef struct AooResponseLogin
{
    AooRequestType type;
    AooFlag flags;
    const AooData *metadata;
} AooResponseLogin;

/* join group (server/client) */

/** \brief request for joining a group */
typedef struct AooRequestGroupJoin
{
    AooRequestType type;
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
     * AooResponseGroupJoin::groupMetadata. */
    const AooData *groupMetadata;
    /* user */
    const AooChar *userName;
    const AooChar *userPwd;
    AooId userId; /* kAooIdInvalid if user does not exist (yet) */
    AooFlag userFlags;
    /** Each client may provide user metadata in AooClient::joinGroup().
     * By default, the server just stores the metadata and sends it to all
     * peers via AooEventPeer::metadata. However, it may also intercept
     * the request and validate/modify the metadata, or provide any kind of
     * metadata it wants, by setting AooResponseGroupJoin::userMetadata. */
    const AooData *userMetadata;
    /* other */
    /** (optional) Relay address provided by the user/client. The server will
     * forward it to all peers. */
    const AooIpEndpoint *relayAddress;
} AooRequestGroupJoin;

/** \brief response for joining a group */
typedef struct AooResponseGroupJoin
{
    AooRequestType type;
    AooFlag flags;
    /* group */
    /** group ID generated by the server */
    AooId groupId;
    AooFlag groupFlags;
    /** (optional) group metadata validated/modified by the server. */
    const AooData *groupMetadata;
    /* user */
    /** user Id generated by the server */
    AooId userId;
    AooFlag userFlags;
    /** (optional) user metadata validated/modified by the server. */
    const AooData *userMetadata;
    /* other */
    /** (optional) private metadata that is only sent to the client.
     * For example, this can be used for state synchronization. */
    const AooData *privateMetadata;
    /** (optional) relay address provided by the server.
      * For example, the AOO server might provide a group with a
      * dedicated UDP relay server. */
    const AooIpEndpoint *relayAddress;
} AooResponseGroupJoin;

/* leave group (server/client) */

/** \brief request for leaving a group */
typedef struct AooRequestGroupLeave
{
    AooRequestType type;
    AooFlag flags;
    AooId group;
} AooRequestGroupLeave;

/** \brief response for leaving a group */
typedef AooResponseBase AooResponseGroupLeave;

/* update group metadata */

/** \brief request for updating a group */
typedef struct AooRequestGroupUpdate
{
    AooRequestType type;
    AooFlag flags;
    AooId groupId;
    AooData groupMetadata;
} AooRequestGroupUpdate;

/** \brief response for updating a group */
typedef struct AooResponseGroupUpdate
{
    AooRequestType type;
    AooFlag flags;
    AooData groupMetadata;
} AooResponseGroupUpdate;

/* update user metadata */

/** \brief request for updating a user */
typedef struct AooRequestUserUpdate
{
    AooRequestType type;
    AooFlag flags;
    AooId groupId;
    AooId userId;
    AooData userMetadata;
} AooRequestUserUpdate;

/** \brief response for updating a user */
typedef struct AooResponseUserUpdate
{
    AooRequestType type;
    AooFlag flags;
    AooData userMetadata;
} AooResponseUserUpdate;

/* custom request (server/client) */

/** \brief custom client request */
typedef struct AooRequestCustom
{
    AooRequestType type;
    AooFlag flags;
    AooData data;
} AooRequestCustom;

/** \brief custom server response */
typedef struct AooResponseCustom
{
    AooRequestType type;
    AooFlag flags;
    AooData data;
} AooResponseCustom;

/*--------------------------------------------*/

/** \brief union of all client requests */
union AooRequest
{
    AooRequestType type; /** request type */
    AooRequestConnect connect;
    AooRequestDisconnect disconnect;
    AooRequestQuery query;
    AooRequestLogin login;
    AooRequestGroupJoin groupJoin;
    AooRequestGroupLeave groupLeave;
    AooRequestGroupUpdate groupUpdate;
    AooRequestUserUpdate userUpdate;
    AooRequestCustom custom;
};

/*--------------------------------------------*/

/** \brief union of all client requests */
union AooResponse
{
    AooRequestType type; /** request type */
    AooResponseError error;
    AooResponseConnect connect;
    AooResponseDisconnect disconnect;
    AooResponseQuery query;
    AooResponseLogin login;
    AooResponseGroupJoin groupJoin;
    AooResponseGroupLeave groupLeave;
    AooResponseGroupUpdate groupUpdate;
    AooResponseUserUpdate userUpdate;
    AooResponseCustom custom;
};

/*--------------------------------------------*/

AOO_PACK_END
