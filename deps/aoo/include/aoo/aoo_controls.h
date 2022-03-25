/* Copyright (c) 2021 Christof Ressi
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/** \file
 * \brief AOO controls
 */

#pragma once

#include "aoo_defines.h"

#define AOO_ARG(x) ((void *)&x), sizeof(x)

/*------------------------------------------------------*/
/*               AOO source/sink controls               */
/*------------------------------------------------------*/

/** \brief AOO source/sink control constants
 *
 * These are passed to AooSource_control() resp. AooSink_control()
 * internally by helper functions and shouldn't be used directly.
 */
enum AooControls
{
    kAooCtlSetId = 0,
    kAooCtlGetId,
    kAooCtlSetFormat,
    kAooCtlGetFormat,
    kAooCtlReset,
    kAooCtlActivate,
    kAooCtlIsActive,
    kAooCtlSetBufferSize,
    kAooCtlGetBufferSize,
    kAooCtlSetDynamicResampling,
    kAooCtlGetDynamicResampling,
    kAooCtlGetRealSampleRate,
    kAooCtlSetDllBandwidth,
    kAooCtlGetDllBandwidth,
    kAooCtlSetXRunDetection,
    kAooCtlGetXRunDetection,
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
    kAooCtlSetStreamMetadataSize,
    kAooCtlGetStreamMetadataSize,
#if USE_AOO_NET
    kAooCtlSetPassword = 1000,
    kAooCtlSetTcpHost,
    kAooCtlSetRelayHost,
    kAooCtlSetServerRelay,
    kAooCtlGetServerRelay,
    kAooCtlSetGroupAutoCreate,
    kAooCtlGetGroupAutoCreate,
#endif
    kAooCtlSentinel
};

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
enum AooPrivateControls
{
    kAooCtlSetClient = -1000,
    kAooCtlNeedRelay,
    kAooCtlGetRelayAddress
};
