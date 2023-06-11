/* Copyright (c) 2021 Christof Ressi
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/** \file
 * \brief C++ interface for AOO server
 *
 * 1. UDP: query -> get public IP + TCP server ID
 * 2. TCP: connect to TCP server -> get user ID
 * 3. TCP: join group -> get peer ID (start from 0) + relay server address
 */

#pragma once

#include "aoo_config.h"
#include "aoo_controls.h"
#include "aoo_defines.h"
#include "aoo_events.h"
#include "aoo_requests.h"
#include "aoo_types.h"

#if AOO_HAVE_CXX11
# include <memory>
#endif

typedef struct AooServer AooServer;

/** \brief create a new AOO source instance
 *
 * \param[out] err (optional) error code on failure
 * \return new AooServer instance on success; `NULL` on failure
 */
AOO_API AooServer * AOO_CALL AooServer_new(AooError *err);

/** \brief destroy AOO server instance */
AOO_API void AOO_CALL AooServer_free(AooServer *server);

/*-----------------------------------------------------------*/

/** \brief AOO server interface */
struct AooServer {
public:
#if AOO_HAVE_CXX11
    /** \brief custom deleter for AooServer */
    class Deleter {
    public:
        void operator()(AooServer *obj){
            AooServer_free(obj);
        }
    };

    /** \brief smart pointer for AOO server instance */
    using Ptr = std::unique_ptr<AooServer, Deleter>;

    /** \brief create a new managed AOO server instance
     *
     * \copydetails AooServer_new()
     */
    static Ptr create(AooError *err) {
        return Ptr(AooServer_new(err));
    }
#endif

    /*---------------------- methods ---------------------------*/

    /** \brief setup the server object
     *
     * \attention If `flags` is 0, we assume that the server is IPv4-only.
     */
    virtual AooError AOO_CALL setup(AooUInt16 port, AooSocketFlags flags) = 0;

    /* UDP echo server */

    /** \brief handle UDP message
     * for querying and relaying
     */
    virtual AooError AOO_CALL handleUdpMessage(
            const AooByte *data, AooInt32 size,
            const void *address, AooAddrSize addrlen,
            AooSendFunc replyFn, void *user) = 0;

    /* TCP server */

    /* client management */

    /** \brief add a new client */
    virtual AooError AOO_CALL addClient(
            AooServerReplyFunc replyFn, void *user, AooId *id) = 0;

    /** \brief remove a client */
    virtual AooError AOO_CALL removeClient(AooId clientId) = 0;

    /** \brief handle client message */
    virtual AooError AOO_CALL handleClientMessage(
            AooId client, const AooByte *data, AooInt32 size) = 0;

    /* request handling */

    /** \brief set request handler (to intercept client requests) */
    virtual AooError AOO_CALL setRequestHandler(
            AooRequestHandler cb, void *user, AooFlag flags) = 0;

    /** \brief handle request
     *
     * If `result` is `kAooErrorNone`, the request has been handled successfully
     * and `response` points to the corresponding response structure.
     *
     * Otherwise the request has failed or been denied; in this case `response`
     * is either `NULL` or points to an `AooResponseError` structure for more detailed
     * information about the error. For example, in the case of `kAooErrorSystem`,
     * the response may contain an OS-specific error code and error message.
     *
     * \attention The response must be properly initialized with `AOO_RESPONSE_INIT`.
     */
    virtual AooError AOO_CALL handleRequest(
            AooId client, AooId token, const AooRequest *request,
            AooError result, AooResponse *response) = 0;

    /* push notifications */

    /** \brief send custom push notification to client */
    virtual AooError AOO_CALL notifyClient(
            AooId client, const AooData &data) = 0;

    /** \brief send custom push notification to group member(s);
        if `user` is `kAooIdInvalid`, all group members are notified. */
    virtual AooError AOO_CALL notifyGroup(
            AooId group, AooId user, const AooData &data) = 0;

    /* group management */

    /** \brief find a group by name */
    virtual AooError AOO_CALL findGroup(
            const AooChar *name, AooId *id) = 0;

    /** \brief add a group
     * By default, the metadata is passed to clients
     * via AooResponseGroupJoin::groupMetadata. */
    virtual AooError AOO_CALL addGroup(
            const AooChar *name, const AooChar *password,
            const AooData *metadata, const AooIpEndpoint *relayAddress,
            AooFlag flags, AooId *groupId) = 0;

    /** \brief remove a group */
    virtual AooError AOO_CALL removeGroup(
            AooId group) = 0;

    /** \brief find a user in a group by name */
    virtual AooError AOO_CALL findUserInGroup(
            AooId group, const AooChar *userName, AooId *userId) = 0;

    /** \brief add user to group
     * By default, the metadata is passed to peers via AooEventPeer::metadata. */
    virtual AooError AOO_CALL addUserToGroup(
            AooId group, const AooChar *userName, const AooChar *userPwd,
            const AooData *metadata, AooFlag flags, AooId *userId) = 0;

    /** \brief remove user from group */
    virtual AooError AOO_CALL removeUserFromGroup(
            AooId group, AooId user) = 0;

    /** \brief group control interface
     *
     * Not to be used directly.
     */
    virtual AooError AOO_CALL groupControl(
            AooId group, AooCtl ctl, AooIntPtr index,
            void *data, AooSize size) = 0;

    /* other methods */

    /** \brief update the Server
     * should be called regularly; used by the Server
     * to update internal timers, for example.
     */
    virtual AooError AOO_CALL update() = 0;

    /** \brief set event handler function and event handling mode
     *
     * \attention Not threadsafe - only call in the beginning!
     */
    virtual AooError AOO_CALL setEventHandler(
            AooEventHandler fn, void *user, AooEventMode mode) = 0;

    /** \brief check for pending events
     *
     * \note Threadsafe and RT-safe
     */
    virtual AooBool AOO_CALL eventsAvailable() = 0;

    /** \brief poll events
     *
     * \note Threadsafe and RT-safe, but not reentrant.
     *
     * This function will call the registered event handler one or more times.
     * \attention The event handler must have been registered with #kAooEventModePoll.
     */
    virtual AooError AOO_CALL pollEvents() = 0;

    /** \brief control interface
     *
     * Not to be used directly.
     */
    virtual AooError AOO_CALL control(
            AooCtl ctl, AooIntPtr index, void *data, AooSize size) = 0;

    /*--------------------------------------------*/
    /*         type-safe control functions        */
    /*--------------------------------------------*/

    /** \brief set the server password */
    AooError setPassword(const AooChar *pwd)
    {
        return control(kAooCtlSetPassword, 0, AOO_ARG(pwd));
    }

    /** \brief get the server password */
    AooError setRelayHost(const AooIpEndpoint *ep)
    {
        return control(kAooCtlSetRelayHost, 0, AOO_ARG(ep));
    }

    /** \brief enabled/disable server relay */
    AooError setServerRelay(AooBool b)
    {
        return control(kAooCtlSetServerRelay, 0, AOO_ARG(b));
    }

    /** \brief check if server relay is enabled */
    AooError getServerRelay(AooBool& b)
    {
        return control(kAooCtlGetServerRelay, 0, AOO_ARG(b));
    }

    /** \brief enable/disable automatic group creation */
    AooError setGroupAutoCreate(AooBool b)
    {
        return control(kAooCtlSetGroupAutoCreate, 0, AOO_ARG(b));
    }

    /** \brief check if automatic group creation is enabled */
    AooError getGroupAutoCreate(AooBool& b)
    {
        return control(kAooCtlGetGroupAutoCreate, 0, AOO_ARG(b));
    }

    /*--------------------------------------------------*/
    /*         type-safe group control functions        */
    /*--------------------------------------------------*/

    /** \brief update group metadata */
    AooError updateGroup(AooId group, const AooData *metadata)
    {
        return groupControl(group, kAooCtlUpdateGroup, 0, AOO_ARG(metadata));
    }

    /** \brief update user metadata */
    AooError updateUser(AooId group, AooId user, const AooData *metadata)
    {
        return groupControl(group, kAooCtlUpdateUser, user, AOO_ARG(metadata));
    }
protected:
    ~AooServer(){} // non-virtual!
};
