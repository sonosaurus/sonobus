/* Copyright (c) 2021 Christof Ressi
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/** \file
 * \brief AOO controls
 */

#pragma once

#include "aoo_config.h"

#define AOO_ARG(x) ((void *)&x), sizeof(x)

/*------------------------------------------------------*/
/*               AOO source/sink controls               */
/*------------------------------------------------------*/

/** \brief AOO source/sink control constants
 *
 * These are passed to AooSource_control() resp. AooSink_control()
 * internally by helper functions and shouldn't be used directly.
 */
enum
{
    kAooCtlSetId = 0,
    kAooCtlGetId,
    kAooCtlSetFormat,
    kAooCtlGetFormat,
    kAooCtlReset,
    kAooCtlActivate,
    kAooCtlIsActive,
    kAooCtlSetLatency,
    kAooCtlGetLatency,
    kAooCtlSetBufferSize,
    kAooCtlGetBufferSize,
    kAooCtlReportXRun,
    kAooCtlSetDynamicResampling,
    kAooCtlGetDynamicResampling,
    kAooCtlGetRealSampleRate,
    kAooCtlSetDllBandwidth,
    kAooCtlGetDllBandwidth,
    kAooCtlResetDll,
    kAooCtlSetChannelOnset,
    kAooCtlGetChannelOnset,
    kAooCtlSetPacketSize,
    kAooCtlGetPacketSize,
    kAooCtlSetPingInterval,
    kAooCtlGetPingInterval,
    kAooCtlSetResendData,
    kAooCtlGetResendData,
    kAooCtlSetResendBufferSize,
    kAooCtlGetResendBufferSize,
    kAooCtlSetResendInterval,
    kAooCtlGetResendInterval,
    kAooCtlSetResendLimit,
    kAooCtlGetResendLimit,
    kAooCtlSetRedundancy,
    kAooCtlGetRedundancy,
    kAooCtlSetSourceTimeout,
    kAooCtlGetSourceTimeout,
    kAooCtlSetInviteTimeout,
    kAooCtlGetInviteTimeout,
    kAooCtlGetBufferFillRatio,
    kAooCtlSetBinaryDataMsg,
    kAooCtlGetBinaryDataMsg,
#if AOO_NET
    kAooCtlSetPassword = 1000,
    kAooCtlSetRelayHost,
    kAooCtlSetServerRelay,
    kAooCtlGetServerRelay,
    kAooCtlSetGroupAutoCreate,
    kAooCtlGetGroupAutoCreate,
    kAooCtlSetBinaryClientMsg,
    kAooCtlGetBinaryClientMsg,
    kAooCtlAddInterfaceAddress,
    kAooCtlRemoveInterfaceAddress,
    /* server group controls */
    kAooCtlUpdateGroup,
    kAooCtlUpdateUser,
#endif
    kAooCtlSentinel
};

/* default values */

/** \brief default source send buffer size in seconds */
#ifndef AOO_SOURCE_BUFFER_SIZE
 #define AOO_SOURCE_BUFFER_SIZE 0.025
#endif

/** \brief default sink latency in seconds */
#ifndef AOO_SINK_LATENCY
 #define AOO_SINK_LATENCY 0.05
#endif

/** \brief use binary data message format by default */
#ifndef AOO_BINARY_DATA_MSG
 #define AOO_BINARY_DATA_MSG 1
#endif

/** \brief enable/disable dynamic resampling by default */
#ifndef AOO_DYNAMIC_RESAMPLING
 #define AOO_DYNAMIC_RESAMPLING 1
#endif

/** \brief default time DLL filter bandwidth */
#ifndef AOO_DLL_BANDWIDTH
 #define AOO_DLL_BANDWIDTH 0.012
#endif

/** \brief default ping interval in seconds */
#ifndef AOO_PING_INTERVAL
 #define AOO_PING_INTERVAL 1.0
#endif

/** \brief default resend buffer size in seconds */
#ifndef AOO_RESEND_BUFFER_SIZE
 #define AOO_RESEND_BUFFER_SIZE 1.0
#endif

/** \brief default send redundancy */
#ifndef AOO_SEND_REDUNDANCY
 #define AOO_SEND_REDUNDANCY 1
#endif

/** \brief enable/disable packet resending by default */
#ifndef AOO_RESEND_DATA
 #define AOO_RESEND_DATA 1
#endif

/** \brief default resend interval in seconds */
#ifndef AOO_RESEND_INTERVAL
 #define AOO_RESEND_INTERVAL 0.01
#endif

/** \brief default resend limit */
#ifndef AOO_RESEND_LIMIT
 #define AOO_RESEND_LIMIT 16
#endif

/** \brief default source timeout */
#ifndef AOO_SOURCE_TIMEOUT
 #define AOO_SOURCE_TIMEOUT 10.0
#endif

/** \brief default invite timeout */
#ifndef AOO_INVITE_TIMEOUT
 #define AOO_INVITE_TIMEOUT 1.0
#endif

/** \brief enable/disable server relay by default */
#ifndef AOO_SERVER_RELAY
 #define AOO_SERVER_RELAY 0
#endif

/** \brief enable/disable automatic group creation by default */
#ifndef AOO_GROUP_AUTO_CREATE
#define AOO_GROUP_AUTO_CREATE 1
#endif

/** \brief enable/disable automatic user creation by default */
#ifndef AOO_USER_AUTO_CREATE
#define AOO_USER_AUTO_CREATE 1
#endif

/** \brief enable/disable binary format for client messages */
#ifndef AOO_CLIENT_BINARY_MSG
 #define AOO_CLIENT_BINARY_MSG 1
#endif

/*------------------------------------------------------*/
/*               user defined controls                  */
/*------------------------------------------------------*/

/* User defined controls (for custom AOO versions)
 * must start from kAooCtlUserDefined, for example:
 *
 * enum MyAooControls
 * {
 *     kMyControl1 = kAooCtlUserDefined
 *     kMyControl2,
 *     kMyControl3,
 *     ...
 * };
 */

/** \brief offset for user defined controls */
#define kAooCtlUserDefined 10000

/*------------------------------------------------------*/
/*                 private controls                     */
/*------------------------------------------------------*/

/* Don't use! TODO: move somewhere else. */
enum
{
    kAooCtlSetClient = -1000,
    kAooCtlNeedRelay,
    kAooCtlGetRelayAddress,
    kAooCtlSetSimulatePacketLoss,
    kAooCtlSetSimulatePacketReorder,
    kAooCtlSetSimulatePacketJitter
};
