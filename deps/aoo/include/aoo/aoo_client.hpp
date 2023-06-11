/* Copyright (c) 2021 Christof Ressi
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/** \file
 * \brief C++ interface for AOO client
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

struct AooSource;
struct AooSink;
typedef struct AooClient AooClient;

/** \brief create a new AOO source instance
 *
 * \param[out] err (optional) error code on failure
 * \return new AooClient instance on success; `NULL` on failure
 */
AOO_API AooClient * AOO_CALL AooClient_new(AooError *err);

/** \brief destroy AOO client */
AOO_API void AOO_CALL AooClient_free(AooClient *client);

/*-----------------------------------------------------------*/

/** \brief AOO client interface */
struct AooClient {
public:
#if AOO_HAVE_CXX11
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
    static Ptr create(AooError *err) {
        return Ptr(AooClient_new(err));
    }
#endif

    /*------------------ methods -------------------------------*/

    /** \brief setup the client object (before calling run())
     *
     * \param port the listening port of the UDP server
     * \param flags optional flags.
     *
     * \attention If `flags` is 0, we assume that the UDP server is IPv4-only.
     */
    virtual AooError AOO_CALL setup(AooUInt16 port, AooSocketFlags flags) = 0;

    /** \brief run the internal TCP client
     *
     * \param nonBlocking
     *   - #kAooTrue: returns immediately, either with #kAooOk (= did something)
     *     or with #kAooErrorWouldBlock (= nothing to do).
     *   - #kAooFalse: blocks until until quit() is called.
     */
    virtual AooError AOO_CALL run(AooBool nonBlocking) = 0;

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

    /** \brief connect to AOO server
     *
     * \note Threadsafe and RT-safe
     *
     * \param hostName the AOO server host name
     * \param port the AOO server port
     * \param password (optional) password
     * \param metadata (optional) metadata
     * \param cb callback function for server reply
     * \param user user data passed to callback function
     */
    virtual AooError AOO_CALL connect(
            const AooChar *hostName, AooInt32 port,
            const AooChar *password, const AooData *metadata,
            AooResponseHandler cb, void *context) = 0;

    /** \brief disconnect from AOO server
     *
     * \note Threadsafe and RT-safe
     *
     * \param cb callback function for server reply
     * \param context user data passed to callback function
     */
    virtual AooError AOO_CALL disconnect(AooResponseHandler cb, void *context) = 0;

    /** \brief join a group on the server
     *
     * \note Threadsafe and RT-safe
     *
     * \param groupName the group name
     * \param groupPwd (optional) group password
     * \param groupMetadata (optional) group metadata
     *        See AooResponseGroupJoin::groupMetadata.
     * \param userName your user name
     * \param userPwd (optional) user password
     * \param userMetadata (optional) user metadata
     *        See AooResponseGroupJoin::userMetadata resp.
     *        AooEventPeer::metadata.
     * \param relayAddress relay address
     * \param cb function to be called with server reply
     * \param context user data passed to callback function
     */
    virtual AooError AOO_CALL joinGroup(
            const AooChar *groupName, const AooChar *groupPwd,
            const AooData *groupMetadata,
            const AooChar *userName, const AooChar *userPwd,
            const AooData *userMetadata,
            const AooIpEndpoint *relayAddress,
            AooResponseHandler cb, void *context) = 0;

    /** \brief leave a group
     *
     * \note Threadsafe and RT-safe
     *
     * \param group the group
     * \param cb function to be called with server reply
     * \param context user data passed to callback function
     */
    virtual AooError AOO_CALL leaveGroup(
            AooId group, AooResponseHandler cb, void *context) = 0;

    /** \brief update group metadata
     *
     * \note Threadsafe and RT-safe
     *
     * \param groupId the group ID
     * \param groupMetadata the new group metadata
     * \param cb function to be called with server reply
     * \param context user data passed to callback function
     */
    virtual AooError AOO_CALL updateGroup(
            AooId groupId, const AooData &groupMetadata,
            AooResponseHandler cb, void *context) = 0;

    /** \brief update user metadata
     *
     * \note Threadsafe and RT-safe
     *
     * \param groupId the group ID
     * \param userMetadata the new user metadata
     * \param cb function to be called with server reply
     * \param context user data passed to callback function
     */
    virtual AooError AOO_CALL updateUser(
            AooId groupId, const AooData &userMetadata,
            AooResponseHandler cb, void *context) = 0;

    /** \brief send custom request
     *
     * \note Threadsafe and RT-safe
     *
     * \param data custom request data
     * \param flags (optional) flags
     * \param cb function to be called with server reply
     * \param context user data passed to callback function
     */
    virtual AooError AOO_CALL customRequest(
            const AooData& data, AooFlag flags,
            AooResponseHandler cb, void *context) = 0;

    // TODO: findGroupByName() and getGroupName()?

    /** \brief find peer by name
     *
     * \note Threadsafe
     *
     * Find peer by its group/user name and return its group/user ID and/or its IP address
     *
     * \param groupName the group name
     * \param userName the user name
     * \param[out] groupId (optional) group ID
     * \param[out] userId (optional) user ID
     * \param[out] address (optional) pointer to sockaddr storage
     * \param[in][out] addrlen (optional) sockaddr storage size, updated to actual size
     */
    virtual AooError AOO_CALL findPeerByName(
            const AooChar *groupName, const AooChar *userName,
            AooId *groupId, AooId *userId, void *address, AooAddrSize *addrlen) = 0;

    /** \brief find peer by IP address
     *
     * \note Threadsafe
     *
     * Find peer by its IP address and return the group ID and user ID
     *
     * \param address the sockaddr
     * \param addrlen the sockaddr size
     * \param[out] groupId group ID
     * \param[out] userId user ID
     */
    virtual AooError AOO_CALL findPeerByAddress(
            const void *address, AooAddrSize addrlen, AooId *groupId, AooId *userId) = 0;

    /** \brief get a peer's group and user name
     *
     * \note Threadsafe
     *
     * \param group the group ID
     * \param user the user ID
     * \param[out] groupNameBuf group name buffer
     * \param[in,out] groupNameSize the group name buffer size;
     *        updated to the actual size (including the 0 terminator)
     * \param[out] userNameBuf user name buffer
     * \param[in,out] userNameSize user name buffer size
     *        updated to the actual size (including 0 terminator)
     */
    virtual AooError AOO_CALL getPeerName(
            AooId group, AooId user,
            AooChar *groupNameBuffer, AooSize *groupNameSize,
            AooChar *userNameBuffer, AooSize *userNameSize) = 0;

    /** \brief send a message to a peer or group
     *
     * \param group the target group (#kAooIdInvalid for all groups)
     * \param user the target user (#kAooIdInvalid for all group members)
     * \param msg the message
     * \param timeStamp future NTP time stamp or #kAooNtpTimeNow
     * \param flags contains one or more values from AooMessageFlags
     */
    virtual AooError AOO_CALL sendMessage(
            AooId group, AooId user, const AooData &msg,
            AooNtpTime timeStamp, AooMessageFlags flags) = 0;

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
            const AooByte *data, AooInt32 size,
            const void *address, AooAddrSize addrlen) = 0;

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

    /** \brief send a request to the AOO server
     *
     * \note Threadsafe
     *
     * Not to be used directly.
     *
     * \param request request structure
     * \param callback function to be called on response
     * \param user user data passed to callback function
     * \param flags additional flags
     */
    virtual AooError AOO_CALL sendRequest(
            const AooRequest& request, AooResponseHandler callback,
            void *user, AooFlag flags) = 0;

    /** \brief control interface
     *
     * Not to be used directly.
     */
    virtual AooError AOO_CALL control(
            AooCtl ctl, AooIntPtr index, void *data, AooSize size) = 0;

    /*--------------------------------------------*/
    /*         type-safe control functions        */
    /*--------------------------------------------*/

    /** \brief Enable/disable binary messages
     *
     * Use a more compact (and faster) binary format for peer messages
     */
    AooError setBinaryMsg(AooBool b) {
        return control(kAooCtlSetBinaryClientMsg, 0, AOO_ARG(b));
    }

    /** \brief Check if binary messages are enabled */
    AooError getBinaryMsg(AooBool& b) {
        return control(kAooCtlGetBinaryClientMsg, 0, AOO_ARG(b));
    }

    /** \brief add interface address */
    AooError addInterfaceAddress(const AooChar *address) {
        return control(kAooCtlAddInterfaceAddress, (AooIntPtr)address, NULL, 0);
    }

    /** \brief remove interface address */
    AooError removeInterfaceAddress(const AooChar *address) {
        return control(kAooCtlRemoveInterfaceAddress, (AooIntPtr)address, NULL, 0);
    }

    /** \brief clear interface addresses */
    AooError clearInterfaceAddresses() {
        return control(kAooCtlRemoveInterfaceAddress, 0, NULL, 0);
    }

    /*--------------------------------------------*/
    /*         type-safe request functions        */
    /*--------------------------------------------*/

    /* (empty) */
protected:
    ~AooClient(){} // non-virtual!
};
