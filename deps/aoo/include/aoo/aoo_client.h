/* Copyright (c) 2021 Christof Ressi
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/** \file
 * \brief C interface for AOO client
 */

#pragma once

#include "aoo_net.h"
#include "aoo_events.h"
#include "aoo_controls.h"

typedef struct AooClient AooClient;

/** \brief create a new AOO source instance
 *
 * \param address local UDP socket address
 * \param addrlen socket address length
 * \param flags optional flags
 * \param[out] err error code on failure
 * \return new AooClient instance on success; `NULL` on failure
 */
AOO_API AooClient * AOO_CALL AooClient_new(
        const void *address, AooAddrSize addrlen,
        AooFlag flags, AooError *err);

/** \brief destroy AOO client */
AOO_API void AOO_CALL AooClient_free(AooClient *client);

/** \copydoc AooClient::run() */
AOO_API AooError AOO_CALL AooClient_run(AooClient *client);

/** \copydoc AooClient::quit() */
AOO_API AooError AOO_CALL AooClient_quit(AooClient *client);

/** \copydoc AooClient::addSource() */
AOO_API AooError AOO_CALL AooClient_addSource(
        AooClient *client, struct AooSource *source, AooId id);

/** \copydoc AooClient::removeSource() */
AOO_API AooError AOO_CALL AooClient_removeSource(
        AooClient *client, struct AooSource *source);

/** \copydoc AooClient::addSink() */
AOO_API AooError AOO_CALL AooClient_addSink(
        AooClient *client, struct AooSink *sink, AooId id);

/** \copydoc AooClient::removeSink() */
AOO_API AooError AOO_CALL AooClient_removeSink(
        AooClient *client, struct AooSink *sink);

/** \copydoc AooClient::getPeerByName() */
AOO_API AooError AOO_CALL AooClient_getPeerByName(
        AooClient *client, const AooChar *group, const AooChar *user,
        void *address, AooAddrSize *addrlen);

/** \copydoc AooClient::sendRequest() */
AOO_API AooError AOO_CALL AooClient_sendRequest(
        AooClient *client, AooNetRequestType request, void *data,
        AooNetCallback callback, void *user);

/** \copydoc AooClient::sendPeerMessage() */
AOO_API AooError AOO_CALL AooClient_sendPeerMessage(
        AooClient *client, const AooByte *data, AooInt32 size,
        const void *address, AooAddrSize addrlen, AooFlag flags);

/** \copydoc AooClient::handleMessage() */
AOO_API AooError AOO_CALL AooClient_handleMessage(
        AooClient *client, const AooByte *data, AooInt32 size,
        const void *address, AooAddrSize addrlen);

/** \copydoc AooClient::send() */
AOO_API AooError AOO_CALL AooClient_send(
        AooClient *client, AooSendFunc fn, void *user);

/** \copydoc AooClient::setEventHandler() */
AOO_API AooError AOO_CALL AooClient_setEventHandler(
        AooClient *sink, AooEventHandler fn, void *user, AooEventMode mode);

/** \copydoc AooClient::eventsAvailable() */
AOO_API AooBool AOO_CALL AooClient_eventsAvailable(AooClient *client);

/** \copydoc AooClient::pollEvents() */
AOO_API AooError AOO_CALL AooClient_pollEvents(AooClient *client);

/** \copydoc AooClient::control */
AOO_API AooError AOO_CALL AooClient_control(
        AooClient *client, AooCtl ctl, AooIntPtr index, void *data, AooSize size);

/*--------------------------------------------*/
/*         type-safe control functions        */
/*--------------------------------------------*/

/* (empty) */

/*--------------------------------------------*/
/*         type-safe request functions        */
/*--------------------------------------------*/

/** \copydoc AooClient::connect() */
AOO_INLINE AooError AooClient_connect(
        AooClient *client, const AooChar *hostName, AooInt32 port,
        const AooChar *userName, const AooChar *userPwd, AooNetCallback cb, void *user)
{
    AooNetRequestConnect data;
    data.hostName = hostName;
    data.port = port;
    data.userName = userName;
    data.userPwd = userPwd;
    data.flags = 0;
    return AooClient_sendRequest(client, kAooNetRequestConnect, &data, cb, user);
}

/** \copydoc AooClient::disconnect() */
AOO_INLINE AooError AooClient_disconnect(
        AooClient *client, AooNetCallback cb, void *user)
{
    return AooClient_sendRequest(client, kAooNetRequestDisconnect, NULL, cb, user);
}

/** \copydoc AooClient::joinGroup() */
AOO_INLINE AooError AooClient_joinGroup(
        AooClient *client, const AooChar *groupName, const AooChar *groupPwd,
        AooNetCallback cb, void *user)
{
    AooNetRequestJoinGroup data;
    data.groupName = groupName;
    data.groupPwd = groupPwd;
    data.flags = 0;
    return AooClient_sendRequest(client, kAooNetRequestJoinGroup, &data, cb, user);
}

/** \copydoc AooClient::leaveGroup() */
AOO_INLINE AooError AooClient_leaveGroup(
        AooClient *client, const AooChar *groupName, AooNetCallback cb, void *user)
{
    AooNetRequestLeaveGroup data;
    data.groupName = groupName;
    data.groupPwd = NULL;
    data.flags = 0;
    return AooClient_sendRequest(client, kAooNetRequestLeaveGroup, &data, cb, user);
}
