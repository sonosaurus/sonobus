/* Copyright (c) 2021 Christof Ressi
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/** \file
 * \brief C++ interface for AOO server
 */

#pragma once

#include "aoo_server.h"

#include <memory>

/** \brief AOO server interface */
struct AooServer {
public:
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
    static Ptr create(int32_t port, AooFlag flags, AooError *err) {
        return Ptr(AooServer_new(port, flags, err));
    }

    /*---------------------- methods ---------------------------*/

    /** \brief run the AOO server
     *
     * This function blocks until AooServer_quit() is called.
     */
    virtual AooError AOO_CALL run() = 0;

    /** \brief quit the AOO server from another thread */
    virtual AooError AOO_CALL quit() = 0;

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
protected:
    ~AooServer(){} // non-virtual!
};
