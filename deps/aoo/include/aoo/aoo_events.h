/* Copyright (c) 2021 Christof Ressi
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/** \file
 * \brief AOO events
 */

#pragma once

#include "aoo_config.h"
#include "aoo_defines.h"
#include "aoo_types.h"

AOO_PACK_BEGIN

/*--------------------------------------------*/

/** \brief AOO source/sink event types */
AOO_ENUM(AooEventType)
{
    /** generic error event */
    kAooEventError = 0,
    /*----------------------------------*/
    /*     AooSource/AooSink events     */
    /*----------------------------------*/
    /** AooSource: received ping from sink */
    kAooEventSinkPing,
    /** AooSink: received ping from source */
    kAooEventSourcePing,
    /** AooSource: invited by sink */
    kAooEventInvite,
    /** AooSource: uninvited by sink */
    kAooEventUninvite,
    /** AooSource: sink added */
    kAooEventSinkAdd,
    /** AooSource: sink removed */
    kAooEventSinkRemove,
    /** AooSink: source added */
    kAooEventSourceAdd,
    /** AooSink: source removed */
    kAooEventSourceRemove,
    /** AooSink: stream started */
    kAooEventStreamStart,
    /** AooSink: stream stopped */
    kAooEventStreamStop,
    /** AooSink: stream changed state */
    kAooEventStreamState,
    /** AooSink: source format changed */
    kAooEventFormatChange,
    /** AooSink: invitation has been declined */
    kAooEventInviteDecline,
    /** AooSink: invitation timed out */
    kAooEventInviteTimeout,
    /** AooSink: uninvitation timed out */
    kAooEventUninviteTimeout,
    /** AooSink: buffer overrun */
    kAooEventBufferOverrun,
    /** AooSink: buffer underrun */
    kAooEventBufferUnderrun,
    /** AooSink: blocks had to be skipped/dropped */
    kAooEventBlockDrop,
    /** AooSink: blocks have been resent */
    kAooEventBlockResend,
    /** AooSink: empty blocks caused by source xrun */
    kAooEventBlockXRun,
    /*--------------------------------------------*/
    /*         AooClient/AooServer events         */
    /*--------------------------------------------*/
    /** AooClient: disconnected from server */
    kAooEventDisconnect = 1000,
    /** AooClient: received a server notification */
    kAooEventNotification,
    /** AooClient: ejected from a group */
    kAooEventGroupEject,
    /** AooClient: received ping (reply) from peer */
    kAooEventPeerPing,
    /** AooClient: peer handshake has started */
    kAooEventPeerHandshake,
    /** AooClient: peer handshake has timed out */
    kAooEventPeerTimeout,
    /** AooClient: peer has joined the group */
    kAooEventPeerJoin,
    /** AooClient: peer has left the group */
    kAooEventPeerLeave,
    /** AooClient: received message from peer */
    kAooEventPeerMessage,
    /** AooClient: peer has been updated */
    kAooEventPeerUpdate,
    /** AooClient: a group has been updated by a peer or by the server
     *  AooServer: a group has been updated by a user */
    kAooEventGroupUpdate,
    /** AooClient: our user has been updated by the server
     *  AooServer: a user has updated itself */
    kAooEventUserUpdate,
    /** AooServer: client logged in successfully or failed */
    kAooEventClientLogin,
    /** AooServer: a new group has been added (automatically) */
    kAooEventGroupAdd,
    /** AooServer: a group has been removed (automatically) */
    kAooEventGroupRemove,
    /** AooServer: a user has joined a group */
    kAooEventGroupJoin,
    /** AooServer: a user has left a group */
    kAooEventGroupLeave,
    /** start of user defined events (for custom AOO versions) */
    kAooEventCustom = 10000
};

/*--------------------------------------------*/

/** \brief common header of all event structs */
#define AOO_EVENT_HEADER \
    AooEventType type; \
    AooUInt32 structSize;

/** \brief base event */
typedef struct AooEventBase
{
    AOO_EVENT_HEADER
} AooEventBase;

/** \brief error event */
typedef struct AooEventError
{
    AOO_EVENT_HEADER
    /** platform specific error code for system errors */
    AooInt32 errorCode;
    const AooChar *errorMessage;
} AooEventError;

/*-------------------------------------------------*/
/*              AOO source/sink events             */
/*-------------------------------------------------*/

/** \brief generic source/sink event */
typedef struct AooEventEndpoint
{
    AOO_EVENT_HEADER
    AooEndpoint endpoint;
} AooEventEndpoint;

/** \brief received ping (reply) from source */
typedef struct AooEventSourcePing
{
    AOO_EVENT_HEADER
    AooEndpoint endpoint;
    AooNtpTime t1; /** send time */
    AooNtpTime t2; /** remote time */
    AooNtpTime t3; /** receive time */
} AooEventSourcePing;

/** \brief received ping (reply) from sink */
typedef struct AooEventSinkPing
{
    AOO_EVENT_HEADER
    AooEndpoint endpoint;
    AooNtpTime t1; /** send time */
    AooNtpTime t2; /** remote time */
    AooNtpTime t3; /** receive time */
    float packetLoss; /** packet loss percentage (0.0 - 1.0) */
} AooEventSinkPing;

/** \brief a new source has been added */
typedef AooEventEndpoint AooEventSourceAdd;

/** \brief a source has been removed */
typedef AooEventEndpoint AooEventSourceRemove;

/** \brief a sink has been added */
typedef AooEventEndpoint AooEventSinkAdd;

/** \brief a sink has been removed */
typedef AooEventEndpoint AooEventSinkRemove;

/** \brief buffer overrun occurred */
typedef AooEventEndpoint AooEventBufferOverrun;

/** \brief buffer underrun occurred */
typedef AooEventEndpoint AooEventBufferUnderrun;

/** \brief a new stream has started */
typedef struct AooEventStreamStart
{
    AOO_EVENT_HEADER
    AooEndpoint endpoint;
    const AooData * metadata;
} AooEventStreamStart;

/** \brief a stream has stopped */
typedef AooEventEndpoint AooEventStreamStop;

/** \brief received invitation by sink */
typedef struct AooEventInvite
{
    AOO_EVENT_HEADER
    AooEndpoint endpoint;
    AooId token;
    const AooData *metadata;
} AooEventInvite;

/** \brief received uninvitation by sink */
typedef struct AooEventUninvite
{
    AOO_EVENT_HEADER
    AooEndpoint endpoint;
    AooId token;
} AooEventUninvite;

/** \brief invitation has been declined */
typedef AooEventEndpoint AooEventInviteDecline;

/** \brief invitation has timed out */
typedef AooEventEndpoint AooEventInviteTimeout;

/** \brief uninvitation has timed out */
typedef AooEventEndpoint AooEventUninviteTimeout;

/** \brief stream states */
AOO_ENUM(AooStreamState)
{
    /** stream is (temporarily) inactive */
    kAooStreamStateInactive = 0,
    /** stream is active */
    kAooStreamStateActive = 1,
    /** stream is buffering */
    kAooStreamStateBuffering = 2
};

/** \brief the stream state has changed */
typedef struct AooEventStreamState
{
    AOO_EVENT_HEADER
    AooEndpoint endpoint;
    AooStreamState state;
    AooInt32 sampleOffset;
} AooEventStreamState;

/** \brief generic stream diagnostic event */
typedef struct AooEventBlock
{
    AOO_EVENT_HEADER
    AooEndpoint endpoint;
    AooInt32 count;
} AooEventBlock;

/** \brief blocks had to be skipped/dropped */
typedef AooEventBlock AooEventBlockDrop;

/** \brief blocks have been resent */
typedef AooEventBlock AooEventBlockResend;

/** \brief empty blocks caused by source xrun */
typedef AooEventBlock AooEventBlockXRun;

/** \brief the source stream format has changed */
typedef struct AooEventFormatChange
{
    AOO_EVENT_HEADER
    AooEndpoint endpoint;
    const AooFormat *format;
} AooEventFormatChange;

/*-------------------------------------------------*/
/*            AOO server/client events             */
/*-------------------------------------------------*/

/* client events */

/** \brief client has been disconnected from server */
typedef AooEventError AooEventDisconnect;

/** \brief client received server notification */
typedef struct AooEventNotification
{
    AOO_EVENT_HEADER
    AooData message;
} AooEventNotification;

/** \brief we have been ejected from a group */
typedef struct AooEventGroupEject
{
    AOO_EVENT_HEADER
    AooId groupId;
} AooEventGroupEject;

/** \brief group metadata has been updated */
typedef struct AooEventGroupUpdate
{
    AOO_EVENT_HEADER
    AooId groupId;
    /** user who updated the group;
     * `kAooIdInvalid` if updated by the server */
    AooId userId;
    AooData groupMetadata;
} AooEventGroupUpdate;

/** \brief user metadata has been updated by the server */
typedef struct AooEventUserUpdate
{
    AOO_EVENT_HEADER
    AooId groupId;
    AooId userId;
    AooData userMetadata;
} AooEventUserUpdate;

/* peer events */

/** \brief generic peer event */
typedef struct AooEventPeer
{
    AOO_EVENT_HEADER
    AooId groupId;
    AooId userId;
    const AooChar *groupName;
    const AooChar *userName;
    AooSockAddr address;
    AooPeerFlags flags;
    const AooChar *version;
    /** See AooResponseGroupJoin::userMetadata */
    const AooData *metadata;
#if 0
    /** relay address provided by this peer,
     * see AooClient::joinGroup() */
    const AooIpEndpoint *relayAddress;
#endif
} AooEventPeer;

/** \brief peer handshake has started */
typedef AooEventPeer AooEventPeerHandshake;

/** \brief peer handshake has timed out */
typedef AooEventPeer AooEventPeerTimeout;

/** \brief peer has joined a group */
typedef AooEventPeer AooEventPeerJoin;

/** \brief peer has left a group */
#if 1
typedef AooEventPeer AooEventPeerLeave;
#else
typedef struct AooEventPeerLeave
{
    AOO_EVENT_HEADER
    AooId group;
    AooId user;
} AooEventPeerLeave;
#endif

/** \brief received ping (reply) from peer */
typedef struct AooEventPeerPing
{
    AOO_EVENT_HEADER
    AooId group;
    AooId user;
    AooNtpTime t1; /** send time */
    AooNtpTime t2; /** remote time */
    AooNtpTime t3; /** receive time */
} AooEventPeerPing;

/** \brief received peer message */
typedef struct AooEventPeerMessage
{
    AOO_EVENT_HEADER
    AooId groupId;
    AooId userId;
    AooNtpTime timeStamp;
    AooData data;
} AooEventPeerMessage;

/** \brief peer metadata has been updated */
typedef struct AooEventPeerUpdate
{
    AOO_EVENT_HEADER
    AooId groupId;
    AooId userId;
    AooData userMetadata;
} AooEventPeerUpdate;

/* server events */

/** \brief client logged in */
typedef struct AooEventClientLogin
{
    AOO_EVENT_HEADER
    AooId id;
    AooError error;
    const AooChar *version;
    const AooData *metadata;
} AooEventClientLogin;

/** \brief group added */
typedef struct AooEventGroupAdd
{
    AOO_EVENT_HEADER
    AooId id;
    AooGroupFlags flags;
    const AooChar *name;
    const AooData *metadata;
#if 0
    const AooIpEndpoint *relayAddress;
#endif
} AooEventGroupAdd;

/** \brief group removed */
typedef struct AooEventGroupRemove
{
    AOO_EVENT_HEADER
    AooId id;
#if 1
    const AooChar *name;
#endif
} AooEventGroupRemove;

/** \brief user joined group */
typedef struct AooEventGroupJoin
{
    AOO_EVENT_HEADER
    AooId groupId;
    AooId userId;
#if 1
    const AooChar *groupName;
#endif
    const AooChar *userName;
    AooId clientId;
    AooUserFlags userFlags;
    const AooData *userMetadata;
#if 0
    const AooIpEndpoint *relayAddress;
#endif
} AooEventGroupJoin;

/** \brief user left group */
typedef struct AooEventGroupLeave
{
    AOO_EVENT_HEADER
    AooId groupId;
    AooId userId;
#if 1
    const AooChar *groupName;
    const AooChar *userName;
#endif
} AooEventGroupLeave;

/*----------------------------------------------------*/

/** \brief union holding all possible events */
union AooEvent
{
    AooEventType type; /** the event type */
    AooEventBase base;
    AooEventError error;
    /* AOO source/sink events */
    AooEventEndpoint endpoint;
    AooEventSourcePing sourcePing;
    AooEventSinkPing sinkPing;
    AooEventInvite invite;
    AooEventUninvite uninvite;
    AooEventSinkAdd sinkAdd;
    AooEventSinkRemove sinkRemove;
    AooEventSourceAdd sourceAdd;
    AooEventSourceRemove sourceRemove;
    AooEventStreamStart streamStart;
    AooEventStreamStop streamStop;
    AooEventStreamState streamState;
    AooEventFormatChange formatChange;
    AooEventInviteDecline inviteDecline;
    AooEventInviteTimeout inviteTimeout;
    AooEventUninviteTimeout uninviteTimeout;
    AooEventBufferOverrun bufferOverrrun;
    AooEventBufferUnderrun bufferUnderrun;
    AooEventBlockDrop blockDrop;
    AooEventBlockResend blockResend;
    AooEventBlockXRun blockXRun;
    /* AooClient/AooServer events */
    AooEventDisconnect disconnect;
    AooEventNotification notification;
    AooEventGroupEject groupEject;
    AooEventPeer peer;
    AooEventPeerPing peerPing;
    AooEventPeerHandshake peerHandshake;
    AooEventPeerTimeout peerTimeout;
    AooEventPeerJoin peerJoin;
    AooEventPeerLeave peerLeave;
    AooEventPeerMessage peerMessage;
    AooEventPeerUpdate peerUpdate;
    AooEventGroupUpdate groupUpdate;
    AooEventUserUpdate userUpdate;
    AooEventClientLogin clientLogin;
    AooEventGroupAdd groupAdd;
    AooEventGroupRemove groupRemove;
    AooEventGroupJoin groupJoin;
    AooEventGroupLeave groupLeave;
};

/*-------------------------------------------------*/

AOO_PACK_END
