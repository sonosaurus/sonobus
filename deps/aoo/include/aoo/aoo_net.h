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

/*-------------- default values ---------------------*/

/** \brief enable/disable server relay by default */
#ifndef AOO_NET_RELAY_ENABLE
 #define AOO_NET_RELAY_ENABLE 1
#endif

/** \brief enable/disable notify on shutdown by default */
#ifndef AOO_NET_NOTIFY_ON_SHUTDOWN
 #define AOO_NET_NOTIFY_ON_SHUTDOWN 0
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

#define kAooNetMsgReply "/reply"
#define kAooNetMsgReplyLen 6

#define kAooNetMsgMessage "/msg"
#define kAooNetMsgMessageLen 4

#define kAooNetMsgLogin "/login"
#define kAooNetMsgLoginLen 6

#define kAooNetMsgRequest "/request"
#define kAooNetMsgRequestLen 8

#define kAooNetMsgGroup "/group"
#define kAooNetMsgGroupLen 6

#define kAooNetMsgJoin "/join"
#define kAooNetMsgJoinLen 5

#define kAooNetMsgLeave "/leave"
#define kAooNetMsgLeaveLen 6

/*------------- requests/replies ----------------*/

/** \brief type for client requests */
typedef AooInt32 AooNetRequestType;

/** \brief client request constants */
enum AooNetRequestTypes
{
    /** connect to server */
    kAooNetRequestConnect = 0,
    /** disconnect from server */
    kAooNetRequestDisconnect,
    /** join group */
    kAooNetRequestJoinGroup,
    /** leave group */
    kAooNetRequestLeaveGroup
};

/** \brief callback for client requests */
typedef void (AOO_CALL *AooNetCallback)(
        /** the user data */
        void *user,
        /** the result of the request (success/failure) */
        AooError code,
        /** the reply data */
        const void *data
);

/** \brief error reply */
typedef struct AooNetReplyError
{
    /** discriptive error message */
    const AooChar *errorMessage;
    /** platform-specific error code for socket/system errors */
    AooInt32 errorCode;
} AooNetReplyError;

/** \brief connection request */
typedef struct AooNetRequestConnect
{
    const AooChar *hostName;
    AooInt32 port;
    const AooChar *userName;
    const AooChar *userPwd;
    AooFlag flags;
} AooNetRequestConnect;

/** \brief server flags (= capabilities) */
enum AooNetServerFlags
{
    kAooNetServerRelay = 0x01
};

/** \brief reply data for connection request */
typedef struct AooNetReplyConnect
{
    AooId userId;
    AooFlag serverFlags;
} AooNetReplyConnect;

/** \brief generic group request */
typedef struct AooNetRequestGroup
{
    const AooChar *groupName;
    const AooChar *groupPwd;
    AooFlag flags;
} AooNetRequestGroup;

/** \brief request to join group */
#define AooNetRequestJoinGroup AooNetRequestGroup

/** \brief request to leave group */
#define AooNetRequestLeaveGroup AooNetRequestGroup

/*-------------------- misc -------------------------*/

/** \brief used in AooClient_sendMessage / AooClient::sendMessage */
enum AooNetMessageFlags
{
    kAooNetMessageReliable = 0x01
};

/*-------------------------------------------------*/

AOO_PACK_END
