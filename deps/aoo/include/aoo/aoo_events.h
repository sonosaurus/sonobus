/* Copyright (c) 2021 Christof Ressi
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/** \file
 * \brief AOO events
 */

#pragma once

#include "aoo_defines.h"

AOO_PACK_BEGIN

/** \brief AOO source/sink event types */
enum AooEventTypes
{
    /** generic error event */
    kAooEventError = 0,
    /*----------------------------------*/
    /*     AooSource/AooSink events     */
    /*----------------------------------*/
    /** source/sink: xruns occured */
    kAooEventXRun,
    /** source/sink: received a ping */
    kAooEventPing,
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
    /** sink: invitation has been declined */
    kAooEventInviteDecline,
    /** sink: invitation timed out */
    kAooEventInviteTimeout,
    /** sink: uninvitation timed out */
    kAooEventUninviteTimeout,
    /** sink: buffer overrun */
    kAooEventBufferOverrun,
    /** sink: buffer underrun */
    kAooEventBufferUnderrun,
    /** sink: blocks have been lost */
    kAooEventBlockLost,
    /** sink: blocks arrived out of order */
    kAooEventBlockReordered,
    /** sink: blocks have been resent */
    kAooEventBlockResent,
    /** sink: block has been dropped by source */
    kAooEventBlockDropped,
#if AOO_NET
    /*----------------------------------*/
    /*         AooClient events         */
    /*----------------------------------*/
    /** client has disconnected from server */
    kAooEventClientDisconnect = 1000,
    /** received a server notification */
    kAooEventClientNotification,
    /** need to call AooClient_send() */
    kAooEventClientNeedSend,
    /** a group has been updated (by a peer or by the server) */
    kAooEventClientGroupUpdate,
    /** our user has been updated (by the server) */
    kAooEventClientUserUpdate,
    /** received ping (reply) from peer */
    kAooEventPeerPing,
    /** peer handshake has started */
    kAooEventPeerHandshake,
    /** peer handshake has timed out */
    kAooEventPeerTimeout,
    /** peer has joined the group */
    kAooEventPeerJoin,
    /** peer has left the group */
    kAooEventPeerLeave,
    /** received message from peer */
    kAooEventPeerMessage,
    /** peer has been updated */
    kAooEventPeerUpdate,
    /*----------------------------------*/
    /*         AooServer events         */
    /*----------------------------------*/
    /** client logged in successfully */
    kAooEventServerClientLogin = 2000,
    /** client has been removed */
    kAooEventServerClientRemove,
    /** a new group has been added (automatically) */
    kAooEventServerGroupAdd,
    /** a group has been removed (automatically) */
    kAooEventServerGroupRemove,
    /** a user has joined a group */
    kAooEventServerGroupJoin,
    /** a user has left a group */
    kAooEventServerGroupLeave,
    /** a group has been updated (by a client) */
    kAooEventServerGroupUpdate,
    /** a user has been updated (by the client) */
    kAooEventServerUserUpdate,
#endif
    /** start of user defined events (for custom AOO versions) */
    kAooEventCustom = 10000
};

/** \brief base event */
typedef struct AooEventBase
{
    AooEventType type;
} AooEventBase;

/** \brief error event */
typedef struct AooEventError
{
    AooEventType type;
    /** platform specific error code for system errors */
    AooInt32 errorCode;
    const AooChar *errorMessage;
} AooEventError;

/*-------------------------------------------------*/
/*              AOO source/sink events             */
/*-------------------------------------------------*/

/** \brief xrun occured */
typedef struct AooEventXRun
{
    AooEventType type;
    AooInt32 count;
} AooEventXRun;

/** \brief generic source/sink event */
typedef struct AooEventEndpoint
{
    AooEventType type;
    AooEndpoint endpoint;
} AooEventEndpoint;

/** \brief ping info for AooSource */
typedef struct AooSourcePingInfo
{
    float packetLoss; /** current packet loss in percent (0.0 - 1.0) */
} AooSourcePingInfo;

/** \brief ping info union */
typedef union AooPingInfo
{
    AooSourcePingInfo source; /** for AooSource */
    void *sink; /** for AooSink (placeholder) */
} AooPingInfo;

/** \brief received ping (reply) */
typedef struct AooEventPing
{
    AooEventType type;
    AooEndpoint endpoint;
    AooNtpTime t1; /** send time */
    AooNtpTime t2; /** remote time */
    AooNtpTime t3; /** receive time */
    AooSize size; /** size of `info` union member */
    AooPingInfo info; /** additional information */
} AooEventPing;

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
    AooEventType type;
    AooEndpoint endpoint;
    const AooData * metadata;
} AooEventStreamStart;

/** \brief a stream has stopped */
typedef AooEventEndpoint AooEventStreamStop;

/** \brief received invitation by sink */
typedef struct AooEventInvite
{
    AooEventType type;
    AooEndpoint endpoint;
    AooId token;
    AooInt32 reserved;
    const AooData * metadata;
} AooEventInvite;

/** \brief received uninvitation by sink */
typedef struct AooEventUninvite
{
    AooEventType type;
    AooEndpoint endpoint;
    AooId token;
} AooEventUninvite;

/** \brief invitation has been declined */
typedef AooEventEndpoint AooEventInviteDecline;

/** \brief invitation has timed out */
typedef AooEventEndpoint AooEventInviteTimeout;

/** \brief uninvitation has timed out */
typedef AooEventEndpoint AooEventUninviteTimeout;

/** \brief AOO stream state type */
typedef AooInt32 AooStreamState;

/** \brief possible stream states */
enum AooStreamStates
{
    /** \brief stream is (temporarily) inactive */
    kAooStreamStateInactive = 0,
    /** \brief stream is active */
    kAooStreamStateActive = 1,
    /** \brief stream is buffering */
    kAooStreamStateBuffering = 2
};

/** \brief the stream state has changed */
typedef struct AooEventStreamState
{
    AooEventType type;
    AooEndpoint endpoint;
    AooStreamState state;
    AooInt32 sampleOffset;
} AooEventStreamState;

/** \brief generic stream diagnostic event */
typedef struct AooEventBlock
{
    AooEventType type;
    AooEndpoint endpoint;
    AooInt32 count;
} AooEventBlock;

/** \brief blocks have been lost */
typedef AooEventBlock AooEventBlockLost;

/** \brief blocks arrived out of order */
typedef AooEventBlock AooEventBlockReordered;

/** \brief blocks have been resent */
typedef AooEventBlock AooEventBlockResent;

/** \brief large gap between blocks */
typedef AooEventBlock AooEventBlockGap;

/** \brief blocks have been dropped (e.g. because of xruns) */
typedef AooEventBlock AooEventBlockDropped;

/** \brief the source stream format has changed */
typedef struct AooEventFormatChange
{
    AooEventType type;
    AooEndpoint endpoint;
    const AooFormat *format;
} AooEventFormatChange;

/*-------------------------------------------------*/
/*            AOO server/client events             */
/*-------------------------------------------------*/

#if AOO_NET

/* client events */

/** \brief client has been disconnected from server */
typedef AooEventError AooEventClientDisconnect;

/** \brief client received server notification */
typedef struct AooEventClientNotification
{
    AooEventType type;
    AooFlag flags;
    AooData message;
} AooEventClientNotification;

/** \brief group metadata has been updated */
typedef struct AooEventClientGroupUpdate
{
    AooEventType type;
    AooFlag flags;
    AooId groupId;
    AooData groupMetadata;
} AooEventClientGroupUpdate;

/** \brief user metadata has been updated */
typedef struct AooEventClientUserUpdate
{
    AooEventType type;
    AooFlag flags;
    AooId groupId;
    AooId userId;
    AooData userMetadata;
} AooEventClientUserUpdate;

/* peer events */

/** \brief generic peer event */
typedef struct AooEventPeer
{
    AooEventType type;
    AooFlag flags;
    AooId groupId;
    AooId userId;
    const AooChar *groupName;
    const AooChar *userName;
    AooSockAddr address;
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
    AooEventType type;
    AooFlag reserved;
    AooId group;
    AooId user;
} AooEventPeerLeave;
#endif

/** \brief received ping (reply) from peer */
typedef struct AooEventPeerPing
{
    AooEventType type;
    AooFlag flags;
    AooId group;
    AooId user;
    AooNtpTime tt1; /** send time */
    AooNtpTime tt2; /** remote time */
    AooNtpTime tt3; /** receive time */
    AooSize reserved;
} AooEventPeerPing;

/** \brief received peer message */
typedef struct AooEventPeerMessage
{
    AooEventType type;
    AooFlag flags;
    AooId groupId;
    AooId userId;
    AooNtpTime timeStamp;
    AooData data;
} AooEventPeerMessage;

/** \brief peer metadata has been updated */
typedef struct AooEventPeerUpdate
{
    AooEventType type;
    AooFlag flags;
    AooId groupId;
    AooId userId;
    AooData userMetadata;
} AooEventPeerUpdate;

/* server events */

/** \brief client logged in */
typedef struct AooEventServerClientLogin
{
    AooEventType type;
    AooFlag flags;
    AooId id;
    AooSocket sockfd;
} AooEventServerClientLogin;

/** \brief client has been removed */
typedef struct AooEventServerClientRemove
{
    AooEventType type;
    AooId id;
} AooEventServerClientRemove;

/** \brief group added */
typedef struct AooEventServerGroupAdd
{
    AooEventType type;
    AooId id;
    const AooChar *name;
    const AooData *metadata;
#if 0
    const AooIpEndpoint *relayAddress;
#endif
    AooFlag flags;
} AooEventServerGroupAdd;

/** \brief group removed */
typedef struct AooEventServerGroupRemove
{
    AooEventType type;
    AooId id;
#if 1
    const AooChar *name;
#endif
} AooEventServerGroupRemove;

/** \brief user joined group */
typedef struct AooEventServerGroupJoin
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
    const AooData *userMetadata;
#if 0
    const AooIpEndpoint *relayAddress;
#endif
} AooEventServerGroupJoin;

/** \brief user left group */
typedef struct AooEventServerGroupLeave
{
    AooEventType type;
    AooFlag flags;
    AooId groupId;
    AooId userId;
#if 1
    const AooChar *groupName;
    const AooChar *userName;
#endif
} AooEventServerGroupLeave;

/** \brief a group has been updated */
typedef struct AooEventServerGroupUpdate
{
    AooEventType type;
    AooFlag flags;
    AooId groupId;
    AooData groupMetadata;
} AooEventServerGroupUpdate;

/** \brief a user has been updated */
typedef struct AooEventServerUserUpdate
{
    AooEventType type;
    AooFlag flags;
    AooId groupId;
    AooId userId;
    AooData userMetadata;
} AooEventServerUserUpdate;

#endif /* AOO_NET */

/*----------------------------------------------------*/

/** \brief union holding all possible events */
union AooEvent
{
    AooEventType type; /** the event type */
    AooEventBase base;
    AooEventError error;
    /* AOO source/sink events */
    AooEventXRun xrun;
    AooEventEndpoint endpoint;
    AooEventPing ping;
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
    AooEventBlockLost blockLost;
    AooEventBlockReordered blockReordered;
    AooEventBlockResent blockResent;
    AooEventBlockDropped blockDropped;
    /* Aoo client/server events */
    AooEventClientDisconnect clientDisconnect;
    AooEventClientNotification clientNotification;
#if 0
    AooEventClientNeedSend clientNeedSend;
#endif
    AooEventClientGroupUpdate clientGroupUpdate;
    AooEventClientUserUpdate clientUserUpdate;
    AooEventPeer peer;
    AooEventPeerPing peerPing;
    AooEventPeerHandshake peerHandshake;
    AooEventPeerTimeout peerTimeout;
    AooEventPeerJoin peerJoin;
    AooEventPeerLeave peerLeave;
    AooEventPeerMessage peerMessage;
    AooEventPeerUpdate peerUpdate;
    AooEventServerClientLogin serverClientLogin;
    AooEventServerClientRemove serverClientRemove;
    AooEventServerGroupAdd serverGroupAdd;
    AooEventServerGroupRemove serverGroupRemove;
    AooEventServerGroupJoin serverGroupJoin;
    AooEventServerGroupLeave serverGroupLeave;
    AooEventServerGroupUpdate serverGroupUpdate;
    AooEventServerUserUpdate serverUserUpdate;
};

/*-------------------------------------------------*/

AOO_PACK_END
