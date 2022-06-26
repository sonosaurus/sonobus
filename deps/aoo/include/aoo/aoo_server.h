/* Copyright (c) 2021 Christof Ressi
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/** \file
 * \brief C interface for AOO server
 */

#pragma once

#include "aoo_net.h"
#include "aoo_events.h"
#include "aoo_controls.h"

/** \brief server reply function
 *
 * \attention This function must send the entire message!
 * Partial writes are not allowed.
 * \param user user data
 * \param clientId client ID
 * \param data message data
 * \param size message size
 * \param flags send flags
 * \return number of bytes written, or -1 on error
 */
typedef AooInt32 (AOO_CALL *AooNetReplyFunc)(
        /** the user data */
        void *user,
        /** the client ID */
        AooId clientId,
        /** the message content */
        const AooByte *data,
        /** the message size in bytes */
        AooSize size
);

/*--------------------------------------------------------------*/

typedef struct AooServer AooServer;

/** \brief create a new AOO source instance
 *
 * \param flags optional flags
 * \param[out] err error code on failure
 * \return new AooServer instance on success; `NULL` on failure
 */
AOO_API AooServer * AOO_CALL AooServer_new(
        AooFlag flags, AooError *err);

/** \brief destroy AOO server instance */
AOO_API void AOO_CALL AooServer_free(AooServer *server);

/** \copydoc AooServer::update() */
AOO_API AooError AOO_CALL AooServer_update(AooServer *server);

/* UDP echo server */

/** \copydoc AooServer::handleUdpMessage() */
AOO_API AooError AOO_CALL AooServer_handleUdpMessage(
        AooServer *server,
        const AooByte *data, AooInt32 size,
        const void *address, AooAddrSize addrlen,
        AooSendFunc replyFn, void *user);

/* TCP server */

/* client management */

/** \copydoc AooServer::addClient() */
AOO_API AooError AOO_CALL AooServer_addClient(
        AooServer *server, AooNetReplyFunc replyFn,
        void *user, AooSocket sockfd, AooId *id);

/** \copydoc AooServer::removeClient() */
AOO_API AooError AOO_CALL AooServer_removeClient(
        AooServer *server, AooId clientId);

/** \copydoc AooServer::handleClientMessage() */
AOO_API AooError AOO_CALL AooServer_handleClientMessage(
        AooServer *server, AooId client,
        const AooByte *data, AooInt32 size);

/** \copydoc AooServer::setRequestHandler() */
AOO_API AooError AOO_CALL AooServer_setRequestHandler(
        AooServer *server, AooNetRequestHandler cb,
        void *user, AooFlag flags);

/** \copydoc AooServer::acceptRequest */
AOO_API AooError AOO_CALL acceptRequest(
        AooServer *server,
        AooId client, AooId token, const AooNetRequest *request,
        const AooNetResponse *response);

/** \copydoc AooServer::declineRequest */
AOO_API AooError AOO_CALL declineRequest(
        AooServer *server,
        AooId client, AooId token, const AooNetRequest *request,
        AooError errorCode, const AooChar *errorMessage);

/* push notifications */

/** \copydoc AooServer::notifyClient() */
AOO_API AooError AOO_CALL AooServer_notifyClient(
        AooServer *server, AooId client,
        const AooDataView *data);

/** \copydoc AooServer::notifyGroup() */
AOO_API AooError AOO_CALL AooServer_notifyGroup(
        AooServer *server, AooId group, AooId user,
        const AooDataView *data);

/* group management */

/** \copydoc AooServer::findGroup() */
AOO_API AooError AOO_CALL AooServer_findGroup(
        AooServer *server, const AooChar *name, AooId *id);

/** \copydoc AooServer::addGroup() */
AOO_API AooError AOO_CALL AooServer_addGroup(
        AooServer *server, const AooChar *name, const AooChar *password,
        const AooDataView *metadata, const AooIpEndpoint *relayAddress,
        AooFlag flags, AooId *groupId);

/** \copydoc AooServer::removeGroup() */
AOO_API AooError AOO_CALL AooServer_removeGroup(
        AooServer *server, AooId group);

/** \copydoc AooServer::findUserInGroup() */
AOO_API AooError AOO_CALL AooServer_findUserInGroup(
        AooServer *server, AooId group,
        const AooChar *userName, AooId *userId);

/** \copydoc AooServer::addUserToGroup() */
AOO_API AooError AOO_CALL AooServer_addUserToGroup(
        AooServer *server, AooId group,
        const AooChar *userName, const AooChar *userPwd,
        const AooDataView *metadata, AooFlag flags, AooId *userId);

/** \copydoc AooServer::removeUserFromGroup() */
AOO_API AooError AOO_CALL AooServer_removeUserFromGroup(
        AooServer *server, AooId group, AooId user);

/** \copydoc AooServer::groupControl() */
AOO_API AooError AOO_CALL AooServer_groupControl(
        AooServer *server, AooId group, AooCtl ctl,
        AooIntPtr index, void *data, AooSize size);

/* other methods */

/** \copydoc AooServer::setEventHandler() */
AOO_API AooError AOO_CALL AooServer_setEventHandler(
        AooServer *server, AooEventHandler fn, void *user, AooEventMode mode);

/** \copydoc AooServer::eventsAvailable() */
AOO_API AooBool AOO_CALL AooServer_eventsAvailable(AooServer *server);

/** \copydoc AooServer::pollEvents() */
AOO_API AooError AOO_CALL AooServer_pollEvents(AooServer *server);

/** \copydoc AooServer::control() */
AOO_API AooError AOO_CALL AooServer_control(
        AooServer *server, AooCtl ctl, AooIntPtr index,
        void *data, AooSize size);

/*--------------------------------------------*/
/*         type-safe control functions        */
/*--------------------------------------------*/

AOO_INLINE AooError AooServer_setPassword(AooServer *server, const AooChar *pwd)
{
    return AooServer_control(server, kAooCtlSetPassword, 0, AOO_ARG(pwd));
}

AOO_INLINE AooError AooServer_setTcpHost(AooServer *server, const AooIpEndpoint *ep)
{
    return AooServer_control(server, kAooCtlSetTcpHost, 0, AOO_ARG(ep));
}

AOO_INLINE AooError AooServer_setRelayHost(
        AooServer *server, const AooIpEndpoint *ep)
{
    return AooServer_control(server, kAooCtlSetRelayHost, 0, AOO_ARG(ep));
}

AOO_INLINE AooError AooServer_setServerRelay(AooServer *server, AooBool b)
{
    return AooServer_control(server, kAooCtlSetServerRelay, 0, AOO_ARG(b));
}

AOO_INLINE AooError AooServer_getServerRelay(AooServer *server, AooBool* b)
{
    return AooServer_control(server, kAooCtlGetServerRelay, 0, AOO_ARG(*b));
}

AOO_INLINE AooError AooServer_setGroupAutoCreate(AooServer *server, AooBool b)
{
    return AooServer_control(server, kAooCtlSetGroupAutoCreate, 0, AOO_ARG(b));
}

AOO_INLINE AooError AooServer_getGroupAutoCreate(AooServer *server, AooBool* b)
{
    return AooServer_control(server, kAooCtlGetGroupAutoCreate, 0, AOO_ARG(*b));
}

/*--------------------------------------------------*/
/*         type-safe group control functions        */
/*--------------------------------------------------*/

AOO_INLINE AooError AooServer_updateGroup(
        AooServer *server, AooId group, const AooDataView *metadata)
{
    return AooServer_groupControl(server, group, kAooCtlUpdateGroup, 0, AOO_ARG(metadata));
}

AOO_INLINE AooError AooServer_updateUser(
        AooServer *server, AooId group, AooId user, const AooDataView *metadata)
{
    return AooServer_groupControl(server, group, kAooCtlUpdateUser, user, AOO_ARG(metadata));
}

/*----------------------------------------------------------------------*/
