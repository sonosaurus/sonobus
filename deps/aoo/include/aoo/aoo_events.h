/* Copyright (c) 2021 Christof Ressi
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/** \file
 * \brief AOO events
 */

#pragma once

#include "aoo_defines.h"

AOO_PACK_BEGIN

/*-------------------------------------------------*/
/*              AOO source/sink events             */
/*-------------------------------------------------*/

/** \brief AOO source/sink event types */
enum AooEventTypes
{
    /** generic error event */
    kAooEventError = 0,
    /** source/sink: xruns occured */
    kAooEventXRun,
    /** sink: received a ping from source */
    kAooEventPing,
    /** source: received a ping reply from sink */
    kAooEventPingReply,
    /** source: invited by sink */
    kAooEventInvite,
    /** source: uninvited by sink */
    kAooEventUninvite,
    /** source: sink added */
    kAooEventSinkAdd,
    /** source: sink removed */
    kAooEventSinkRemove,
    /** sink: source added */
    kAooEventSourceAdd,
    /** sink: source removed */
    kAooEventSourceRemove,
    /** sink: stream started */
    kAooEventStreamStart,
    /** sink: stream stopped */
    kAooEventStreamStop,
    /** sink: stream changed state */
    kAooEventStreamState,
    /** sink: source format changed */
    kAooEventFormatChange,
    /** sink: invitation timed out */
    kAooEventInviteTimeout,
    /** sink: uninvitation timed out */
    kAooEventUninviteTimeout,
    /** sink: buffer underrun */
    kAooEventBufferUnderrun,
    /** sink: blocks have been lost */
    kAooEventBlockLost,
    /** sink: blocks arrived out of order */
    kAooEventBlockReordered,
    /** sink: blocks have been resent */
    kAooEventBlockResent,
    /** sink: block has been dropped by source */
    kAooEventBlockDropped
};

/** \brief error event */
typedef struct AooEventError
{
    AooEventType type;
    /** platform specific error code for system errors */
    AooInt32 errorCode;
    const AooChar *errorMessage;
} AooEventError;

/** \brief xrun occured */
typedef struct AooEventXRun
{
    AooEventType type;
    AooInt32 count;
} AooEventXRun;

/** \brief received ping from source */
typedef struct AooEventPing {
    AooEventType type;
    AooEndpoint endpoint;
    AooNtpTime t1;
    AooNtpTime t2;
} AooEventPing;

/** \brief received ping reply from sink */
typedef struct AooEventPingReply {
    AooEventType type;
    AooEndpoint endpoint;
    AooNtpTime t1;
    AooNtpTime t2;
    AooNtpTime t3;
    float packetLoss;
} AooEventPingReply;

/** \brief generic source/sink event */
typedef struct AooEventEndpoint
{
    AooEventType type;
    AooEndpoint endpoint;
} AooEventEndpoint;

/** \brief a new source has been added */
#define AooEventSourceAdd AooEventEndpoint

/** \brief a source has been removed */
#define AooEventSourceRemove AooEventEndpoint

/** \brief a sink has been added */
#define AooEventSinkAdd AooEventEndpoint

/** \brief a sink has been removed */
#define AooEventSinkRemove AooEventEndpoint

/** \brief buffer underrun occurred */
#define AooEventBufferUnderrun AooEventEndpoint

/** \brief a new stream has started */
typedef struct AooEventStreamStart
{
    AooEventType type;
    AooEndpoint endpoint;
    const AooDataView * metadata;
} AooEventStreamStart;

/** \brief a stream has stopped */
#define AooEventStreamStop AooEventEndpoint

/** \brief received invitation by sink */
typedef struct AooEventInvite
{
    AooEventType type;
    AooEndpoint endpoint;
    AooId token;
    AooInt32 reserved;
    const AooDataView * metadata;
} AooEventInvite;

/** \brief received uninvitation by sink */
typedef struct AooEventUninvite
{
    AooEventType type;
    AooEndpoint endpoint;
    AooId token;
} AooEventUninvite;

/** \brief invitation has timed out */
#define AooEventInviteTimeout AooEventEndpoint

/** \brief uninvitation has timed out */
#define AooEventUninviteTimeout AooEventEndpoint

/** \brief AOO stream state type */
typedef AooInt32 AooStreamState;

/** \brief stream is (temporarily) inactive */
#define kAooStreamStateInactive 0
/** \brief stream is active */
#define kAooStreamStateActive 1

/** \brief the stream state has changed */
typedef struct AooEventStreamState
{
    AooEventType type;
    AooEndpoint endpoint;
    AooStreamState state;
} AooEventStreamState;

/** \brief generic stream diagnostic event */
typedef struct AooEventBlock
{
    AooEventType type;
    AooEndpoint endpoint;
    AooInt32 count;
} AooEventBlock;

/** \brief blocks have been lost */
#define AooEventBlockLost AooEventBlock

/** \brief blocks arrived out of order */
#define AooEventBlockReordered AooEventBlock

/** \brief blocks have been resent */
#define AooEventBlockResent AooEventBlock

/** \brief large gap between blocks */
#define AooEventBlockGap AooEventBlock

/** \brief blocks have been dropped (e.g. because of xruns) */
#define AooEventBlockDropped AooEventBlock

/** \brief the source stream format has changed */
typedef struct AooEventFormatChange {
    AooEventType type;
    AooEndpoint endpoint;
    const AooFormat *format;
} AooEventFormatChange;

/*-------------------------------------------------*/
/*            AOO server/client events             */
/*-------------------------------------------------*/

#if USE_AOO_NET

/** \brief AOO server/client event types */
enum AooNetEventTypes
{
    /* generic events */
    /** generic error event */
    kAooNetEventError = 0,
    /* client events */
    /** client has disconnected from server */
    kAooNetEventDisconnect,
    /** received a server notification */
    kAooNetEventServerNotification,
    /** need to call AooClient_send() */
    kAooNetEventNeedSend,
    /* peer events */
    /** received ping */
    kAooNetEventPeerPing,
    /** received ping reply */
    kAooNetEventPeerPingReply,
    /** peer handshake has started */
    kAooNetEventPeerHandshake,
    /** peer handshake has timed out */
    kAooNetEventPeerTimeout,
    /** peer has joined the group */
    kAooNetEventPeerJoin,
    /** peer has left the group */
    kAooNetEventPeerLeave,
    /** received peer message */
    kAooNetEventPeerMessage,
    /* server events */
    /** client logged in successfully */
    kAooNetEventClientLogin,
    /** client has been removed */
    kAooNetEventClientRemove,
    /** a new group has been added (automatically) */
    kAooNetEventGroupAdd,
    /** a group has been removed (automatically) */
    kAooNetEventGroupRemove,
    /** a user has joined a group */
    kAooNetEventGroupJoin,
    /** a user has left a group */
    kAooNetEventGroupLeave
};

/* generic events */

/** \brief generic error event */
typedef struct AooNetEventError
{
    AooEventType type;
    AooInt32 errorCode; /* platform/user specific error code */
    const AooChar *errorMessage;
} AooNetEventError;

/* client events */

/** \brief client has disconnected from server */
#define AooNetEventDisconnect AooNetEventError

/** \brief server notification */
typedef struct AooNetEventServerNotification
{
    AooEventType type;
    AooFlag flags;
    AooDataView message;
} AooNetEventServerNotification;

/* peer events */

/** \brief generic peer event */
typedef struct AooNetEventPeer
{
    AooEventType type;
    AooFlag flags;
    AooId groupId;
    AooId userId;
    const AooChar *groupName;
    const AooChar *userName;
    AooSockAddr address;
    /** See AooNetResponseGroupJoin::userMetadata */
    const AooDataView *metadata;
#if 0
    /** relay address provided by this peer,
     * see AooClient::joinGroup() */
    const AooIpEndpoint *relayAddress;
#endif
} AooNetEventPeer;

/** \brief peer handshake has started */
#define AooNetEventPeerHandshake AooNetEventPeer

/** \brief peer handshake has timed out */
#define AooNetEventPeerTimeout AooNetEventPeer

/** \brief peer has joined a group */
#define AooNetEventPeerJoin AooNetEventPeer

#if 0
/** \brief peer has left a group */
typedef struct AooNetEventLeave
{
    AooEventType type;
    AooFlag reserved;
    AooId group;
    AooId user;
} AooNetEventPeerLeave;
#endif

/** \brief received ping from peer */
typedef struct AooNetEventPeerPing
{
    AooEventType type;
    AooFlag flags;
    AooId group;
    AooId user;
    AooNtpTime tt1;
    AooNtpTime tt2;
} AooNetEventPeerPing;

/** \brief received ping reply */
typedef struct AooNetEventPeerPingReply
{
    AooEventType type;
    AooFlag flags;
    AooId group;
    AooId user;
    AooNtpTime tt1;
    AooNtpTime tt2;
    AooNtpTime tt3;
} AooNetEventPeerPingReply;

/** \brief received peer message */
typedef struct AooNetEventPeerMessage
{
    AooEventType type;
    AooFlag flags;
    AooId groupId;
    AooId userId;
    AooNtpTime timeStamp;
    AooDataView data;
} AooNetEventPeerMessage;

/* server events */

/** \brief client logged in */
typedef struct AooNetEventClientLogin
{
    AooEventType type;
    AooFlag flags;
    AooId id;
    AooSocket sockfd;
} AooNetEventClientLogin;

/** \brief client has been removed */
typedef struct AooNetEventClientRemove
{
    AooEventType type;
    AooId id;
} AooNetEventClientRemove;

/** \brief group added */
typedef struct AooNetEventGroupAdd
{
    AooEventType type;
    AooId id;
    const AooChar *name;
    const AooDataView *metadata;
#if 0
    const AooIpEndpoint *relayAddress;
#endif
    AooFlag flags;
} AooNetEventGroupAdd;

/** \brief group removed */
typedef struct AooNetEventGroupRemove
{
    AooEventType type;
    AooId id;
#if 1
    const AooChar *name;
#endif
} AooNetEventGroupRemove;

/** \brief user joined group */
typedef struct AooNetEventGroupJoin
{
    AooEventType type;
    AooFlag flags;
    AooId groupId;
    AooId userId;
#if 1
    const AooChar *groupName;
#endif
    const AooChar *userName;
    AooId clientId;
    AooFlag userFlags;
    const AooDataView *userMetadata;
#if 0
    const AooIpEndpoint *relayAddress;
#endif
} AooNetEventGroupJoin;

/** \brief user left group */
typedef struct AooNetEventGroupLeave
{
    AooEventType type;
    AooFlag flags;
    AooId groupId;
    AooId userId;
#if 1
    const AooChar *groupName;
    const AooChar *userName;
#endif
} AooNetEventGroupLeave;

#endif /* USE_AOO_NET */

/*--------------------------------------------------*/
/*             user defined events                  */
/*--------------------------------------------------*/

/* User defined event types (for custom AOO versions)
 * must start from kAooEventUserDefined, for example:
 *
 * enum MyAooEventTypes
 * {
 *     kMyEvent1 = kAooEventUserDefined
 *     kMyEvent2,
 *     kMyEvent3,
 *     ...
 * };
 */

/** \brief offset for user defined events */
#define kAooEventUserDefined 10000

/*-------------------------------------------------*/

AOO_PACK_END
