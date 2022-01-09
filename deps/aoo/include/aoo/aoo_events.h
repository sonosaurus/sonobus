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
    /** ping has been received */
    kAooNetEventPing,
    /* client events */
    /** client has disconnected from server */
    kAooNetEventDisconnect,
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
    /** user has been added */
    kAooNetEventUserAdd,
    /** user has been removed */
    kAooNetEventUserRemove,
    /** user has joined the server */
    kAooNetEventUserJoin,
    /** user has left the server */
    kAooNetEventUserLeave,
    /** group has been added */
    kAooNetEventGroupAdd,
    /** group has been removed */
    kAooNetEventGroupRemove,
    /** user has joined a group */
    kAooNetEventUserGroupJoin,
    /** user has left a group */
    kAooNetEventUserGroupLeave
};

/* generic events */

/** \brief generic error event */
typedef struct AooNetEventError
{
    AooEventType type;
    /* platform specific error code for system/socket errors */
    AooInt32 errorCode;
    const AooChar *errorMessage;
} AooNetEventError;

/** \brief received ping */
typedef struct AooNetEventPing
{
    AooEventType type;
    AooAddrSize addrlen;
    const void *address;
    AooNtpTime tt1;
    AooNtpTime tt2;
    AooNtpTime tt3; /* only for clients */
} AooNetEventPing;

/* client events */

/** \brief client has disconnected from server */
#define AooNetEventDisconnect AooNetEventError

/** \brief generic peer event */
typedef struct AooNetEventPeer
{
    AooEventType type;
    AooAddrSize addrlen;
    const void *address;
    const AooChar *groupName;
    const AooChar *userName;
    AooId userId;
} AooNetEventPeer;

/** \brief peer handshake has started */
#define AooNetEventPeerHandshake AooNetEventPeer

/** \brief peer handshake has timed out */
#define AooNetEventPeerTimeout AooNetEventPeer

/** \brief peer has joined a group */
#define AooNetEventPeerJoin AooNetEventPeer

/** \brief peer has left a group */
#define AooNetEventPeerLeave AooNetEventPeer

/** \brief received peer message */
typedef struct AooNetEventPeerMessage
{
    AooEventType type;
    AooAddrSize addrlen;
    const void *address;
    const AooByte *data;
    AooInt32 size;
} AooNetEventPeerMessage;

/* server events */

/** \brief generic user event */
typedef struct AooNetEventUser
{
    AooEventType type;
    AooAddrSize addrlen;
    const void *address;
    const AooChar *userName;
    AooId userId;
} AooNetEventUser;

/** \brief user has been added */
#define AooNetEventUserAdd AooNetEventUser

/** \brief user has been removed */
#define AooNetEventUserRemove AooNetEventUser

/** \brief user has joined the server */
#define AooNetEventUserJoin AooNetEventUser

/** \brief user has left the server */
#define AooNetEventUserLeave AooNetEventUser

/** \brief generic group event */
typedef struct AooNetEventGroup
{
    AooEventType type;
    const AooChar *groupName;
} AooNetEventGroup;

/** \brief group has been added */
#define AooNetEventGroupAdd AooNetEventGroup

/** \brief group has been removed */
#define AooNetEventGroupRemove AooNetEventGroup

/** \brief generic user/group event */
typedef struct AooNetEventUserGroup
{
    AooEventType type;
    AooId userId;
    const AooChar *userName;
    const AooChar *groupName;
} AooNetEventUserGroup;

/** \brief user has joined a group */
#define AooNetEventUserGroupJoin AooNetEventUserGroup

/** \brief user has left a group */
#define AooNetEventUserGroupLeave AooNetEventUserGroup

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
