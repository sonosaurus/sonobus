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

/*--------------------------------------------------------------*/

typedef struct AooServer AooServer;

/** \brief create a new AOO source instance
 *
 * \param port the listening port (TCP + UDP)
 * \param flags optional flags
 * \param[out] err error code on failure
 * \return new AooServer instance on success; `NULL` on failure
 */
AOO_API AooServer * AOO_CALL AooServer_new(
        AooInt32 port, AooFlag flags, AooError *err);

/** \brief destroy AOO server instance */
AOO_API void AOO_CALL AooServer_free(AooServer *server);

/** \copydoc AooServer::run() */
AOO_API AooError AOO_CALL AooServer_run(AooServer *server);

/** \copydoc AooServer::quit() */
AOO_API AooError AOO_CALL AooServer_quit(AooServer *server);

/** \copydoc AooServer::setEventHandler() */
AOO_API AooError AOO_CALL AooServer_setEventHandler(
        AooServer *sink, AooEventHandler fn, void *user, AooEventMode mode);

/** \copydoc AooServer::eventsAvailable() */
AOO_API AooBool AOO_CALL AooServer_eventsAvailable(AooServer *server);

/** \copydoc AooServer::pollEvents() */
AOO_API AooError AOO_CALL AooServer_pollEvents(AooServer *server);

/** \copydoc AooServer::control() */
AOO_API AooError AOO_CALL AooServer_control(
        AooServer *server, AooCtl ctl, AooIntPtr index, void *data, AooSize size);

/*--------------------------------------------*/
/*         type-safe control functions        */
/*--------------------------------------------*/

/* (empty) */

/*----------------------------------------------------------------------*/
