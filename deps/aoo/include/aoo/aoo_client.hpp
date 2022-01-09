/* Copyright (c) 2021 Christof Ressi
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/** \file
 * \brief C++ interface for AOO client
 */

#pragma once

#include "aoo_client.h"

#include <memory>

struct AooSource;
struct AooSink;

/** \brief AOO client interface */
struct AooClient {
public:
    /** \brief custom deleter for AooClient */
    class Deleter {
    public:
        void operator()(AooClient *obj){
            AooClient_free(obj);
        }
    };

    /** \brief smart pointer for AOO client instance */
    using Ptr = std::unique_ptr<AooClient, Deleter>;

    /** \brief create a new managed AOO client instance
     *
     * \copydetails AooClient_new()
     */
    static Ptr create(const void *address, AooAddrSize addrlen,
                      AooFlag flags, AooError *err) {
        return Ptr(AooClient_new(address, addrlen, flags, err));
    }

    /*------------------ methods -------------------------------*/

    /** \brief run the AOO client
     *
     * This function blocks until AooClient_quit() is called.
     */
    virtual AooError AOO_CALL run() = 0;

    /** \brief quit the AOO client from another thread */
    virtual AooError AOO_CALL quit() = 0;

    /** \brief add AOO source
     *
     * \param source the AOO source
     * \param id the AOO source ID
     */
    virtual AooError AOO_CALL addSource(AooSource *source, AooId id) = 0;

    /** \brief remove AOO source */
    virtual AooError AOO_CALL removeSource(AooSource *source) = 0;

    /** \brief add AOO sink
     *
     * \param sink the AOO sink
     * \param id the AOO sink ID
     */
    virtual AooError AOO_CALL addSink(AooSink *sink, AooId id) = 0;

    /** \brief remove AOO sink */
    virtual AooError AOO_CALL removeSink(AooSink *sink) = 0;

    /** \brief find peer by name
     *
     * Find peer of the given user + group name and return its IP endpoint address
     *
     * \param group the group name
     * \param user the user name
     * \param address socket address storage, i.e. pointer to `sockaddr_storage` struct
     * \param addrlen socket address storage length;
     *        initialized with max. storage size, updated to actual size
     */
    virtual AooError AOO_CALL getPeerByName(
            const AooChar *group, const AooChar *user,
            void *address, AooAddrSize *addrlen) = 0;

    /** \brief send a request to the AOO server
     *
     * \note Threadsafe
     *
     * Not to be used directly.
     *
     * \param request the request type
     * \param data (optional) request data
     * \param callback function to be called back when response has arrived
     * \param user user data passed to callback function
     */
    virtual AooError AOO_CALL sendRequest(
            AooNetRequestType request, void *data,
            AooNetCallback callback, void *user) = 0;

    /** \brief send a message to one or more peers
     *
     * `address` + `addrlen` accept the following values:
     * \li `struct sockaddr *` + `socklen_t`: send to a single peer
     * \li `const AooChar *` (group name) + 0: send to all peers of a specific group
     * \li `NULL` + 0: send to all peers
     *
     * \param data the message data
     * \param size the message size in bytes
     * \param address the message target (see above)
     * \param addrlen the socket address length
     * \param flags contains one or more values from AooNetMessageFlags
     */
    virtual AooError AOO_CALL sendPeerMessage(
            const AooByte *data, AooInt32 size,
            const void *address, AooAddrSize addrlen, AooFlag flags) = 0;

    /** \brief handle messages from peers
     *
     * \note Threadsafe, but not reentrant; call on the network thread
     *
     * \param data the message data
     * \param size the message size
     * \param address the remote socket address
     * \param addrlen the socket address length
     */
    virtual AooError AOO_CALL handleMessage(
            const AooByte *data, AooInt32 size, const void *address, AooAddrSize addrlen) = 0;

    /** \brief send outgoing messages
     *
     * \note Threadsafe; call on the network thread
     *
     * \param sink the AOO sink
     * \param fn the send function
     * \param user the user data (passed to the send function)
     */
    virtual AooError AOO_CALL send(AooSendFunc fn, void *user) = 0;

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

    /* (empty) */

    /*--------------------------------------------*/
    /*         type-safe request functions        */
    /*--------------------------------------------*/

    /** \brief connect to AOO server
     *
     * \note Threadsafe and RT-safe
     *
     * \param hostName the AOO server host name
     * \param port the AOO server port
     * \param userName the user name
     * \param userPwd the user password
     * \param cb callback function for server reply
     * \param user user data passed to callback function
     */
    AooError connect(const AooChar *hostName, AooInt32 port,
                     const AooChar *userName, const AooChar *userPwd,
                     AooNetCallback cb, void *user)
    {
        AooNetRequestConnect data = { hostName, port, userName, userPwd };
        return sendRequest(kAooNetRequestConnect, &data, cb, user);
    }

    /** \brief disconnect from AOO server
     *
     * \note Threadsafe and RT-safe
     *
     * \param cb callback function for server reply
     * \param user user data passed to callback function
     */
    AooError disconnect(AooNetCallback cb, void *user)
    {
        return sendRequest(kAooNetRequestDisconnect, NULL, cb, user);
    }

    /** \brief join a group on the server
     *
     * \note Threadsafe and RT-safe
     *
     * \param groupName the group name
     * \param groupPwd the group password
     * \param cb function to be called with server reply
     * \param user user data passed to callback function
     */
    AooError joinGroup(const AooChar *groupName, const AooChar *groupPwd,
                       AooNetCallback cb, void *user)
    {
        AooNetRequestJoinGroup data = { groupName, groupPwd };
        return sendRequest(kAooNetRequestJoinGroup, &data, cb, user);
    }

    /** \brief leave a group
     *
     * \note Threadsafe and RT-safe
     *
     * \param groupName the group name
     * \param cb function to be called with server reply
     * \param user user data passed to callback function
     */
    AooError leaveGroup(const AooChar *groupName, AooNetCallback cb, void *user)
    {
        AooNetRequestLeaveGroup data = { groupName };
        return sendRequest(kAooNetRequestLeaveGroup, &data, cb, user);
    }
protected:
    ~AooClient(){} // non-virtual!
};
