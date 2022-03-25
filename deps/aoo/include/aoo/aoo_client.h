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
 * \param udpSocket bound UDP socket handle
 * \param flags optional flags
 * \param[out] err error code on failure
 * \return new AooClient instance on success; `NULL` on failure
 */
AOO_API AooClient * AOO_CALL AooClient_new(
        AooSocket udpSocket, AooFlag flags, AooError *err);

/** \brief destroy AOO client */
AOO_API void AOO_CALL AooClient_free(AooClient *client);

/** \copydoc AooClient::run() */
AOO_API AooError AOO_CALL AooClient_run(
        AooClient *client, AooBool nonBlocking);

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

/** \copydoc AooClient::connect() */
AOO_API AooError AooClient_connect(
        AooClient *client,
        const AooChar *hostName, AooInt32 port, const AooChar *password,
        const AooDataView *metadata, AooNetCallback cb, void *context);

/** \copydoc AooClient::disconnect() */
AOO_API AooError AooClient_disconnect(
        AooClient *client, AooNetCallback cb, void *context);

/** \copydoc AooClient::joinGroup() */
AOO_API AooError AooClient_joinGroup(
        AooClient *client,
        const AooChar *groupName, const AooChar *groupPwd,
        const AooDataView *groupMetadata,
        const AooChar *userName, const AooChar *userPwd,
        const AooDataView *userMetadata,
        const AooIpEndpoint *relayAddress,
        AooNetCallback cb, void *context);

/** \copydoc AooClient::leaveGroup() */
AOO_API AooError AooClient_leaveGroup(
        AooClient *client, AooId group,
        AooNetCallback cb, void *context);

/** \copydoc AooClient::getPeerByName() */
AOO_API AooError AOO_CALL AooClient_getPeerByName(
        AooClient *client, const AooChar *group, const AooChar *user,
        void *address, AooAddrSize *addrlen);

/** \copydoc AooClient::getPeerById() */
AOO_API AooError AOO_CALL AooClient_getPeerById(
        AooClient *client, AooId group, AooId user,
        void *address, AooAddrSize *addrlen);

/** \copydoc AooClient::getPeerByAddress() */
AOO_API AooError AOO_CALL AooClient_getPeerByAddress(
        AooClient *client,
        const void *address, AooAddrSize addrlen,
        AooId *group, AooId *user,
        AooChar *groupNameBuf, AooSize *groupNameSize,
        AooChar *userNameBuf, AooSize *userNameSize);

/** \copydoc AooClient::sendMessage() */
AOO_API AooError AOO_CALL AooClient_sendMessage(
        AooClient *client, AooId group, AooId user,
        const AooDataView *msg, AooNtpTime timeStamp, AooFlag flags);

/** \copydoc AooClient::handleMessage() */
AOO_API AooError AOO_CALL AooClient_handleMessage(
        AooClient *client, const AooByte *data, AooInt32 size,
        const void *address, AooAddrSize addrlen);

/** \copydoc AooClient::send() */
AOO_API AooError AOO_CALL AooClient_send(
        AooClient *client, AooSendFunc fn, void *user);

/** \copydoc AooClient::setEventHandler() */
AOO_API AooError AOO_CALL AooClient_setEventHandler(
        AooClient *client, AooEventHandler fn, void *user, AooEventMode mode);

/** \copydoc AooClient::eventsAvailable() */
AOO_API AooBool AOO_CALL AooClient_eventsAvailable(AooClient *client);

/** \copydoc AooClient::pollEvents() */
AOO_API AooError AOO_CALL AooClient_pollEvents(AooClient *client);

/** \copydoc AooClient::sendRequest() */
AOO_API AooError AOO_CALL AooClient_sendRequest(
        AooClient *client, const AooNetRequest *request,
        AooNetCallback callback, void *user, AooFlag flags);

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

/** \copydoc AooClient::sendCustomRequest() */
AOO_INLINE AooError AooClient_sendCustomRequest(
        AooClient *client, const AooDataView *data,
        AooNetCallback cb, void *context)
{
    AooNetRequestCustom request;
    request.type = kAooNetRequestConnect;
    request.flags = 0;
    request.data.type = data->type;
    request.data.data = data->data;
    request.data.size = data->size;
    return AooClient_sendRequest(
                client, (AooNetRequest *)&request, cb, context, 0);
}
