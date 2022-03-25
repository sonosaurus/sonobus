/* Copyright (c) 2021 Christof Ressi
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/** \file
 * \brief main library API
 *
 * This file contains default values and important library routines.
 * It also describes the public OSC interface. */

#pragma once

#include "aoo_defines.h"

/*------------- compile time settings -------------*/

/** \brief clip audio output between -1 and 1 */
#ifndef AOO_CLIP_OUTPUT
# define AOO_CLIP_OUTPUT 0
#endif

/** \brief debug the state of the DLL filter */
#ifndef AOO_DEBUG_DLL
# define AOO_DEBUG_DLL 0
#endif

/** \brief debug incoming/outgoing data */
#ifndef AOO_DEBUG_DATA
# define AOO_DEBUG_DATA 0
#endif

/** \brief debug the state of the xrun detector */
#ifndef AOO_DEBUG_TIMER
# define AOO_DEBUG_TIMER 0
#endif

/** \brief debug the state of the dynamic resampler */
#ifndef AOO_DEBUG_RESAMPLING
# define AOO_DEBUG_RESAMPLING 0
#endif

/** \brief debug the state of the audio buffer */
#ifndef AOO_DEBUG_AUDIO_BUFFER
# define AOO_DEBUG_AUDIO_BUFFER 0
#endif

/** \brief debug the state of the jitter buffer */
#ifndef AOO_DEBUG_JITTER_BUFFER
# define AOO_DEBUG_JITTER_BUFFER 0
#endif

/*---------------- default values --------------*/

/** \brief default source buffer size in seconds */
#ifndef AOO_SOURCE_BUFFER_SIZE
 #define AOO_SOURCE_BUFFER_SIZE 0.025
#endif

/** \brief default sink buffer size in seconds */
#ifndef AOO_SINK_BUFFER_SIZE
 #define AOO_SINK_BUFFER_SIZE 0.05
#endif

/** \brief default sink buffer size in seconds */
#ifndef AOO_STREAM_METADATA_SIZE
 #define AOO_STREAM_METADATA_SIZE 256
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

/** \brief default enable/disable xrun detection by default */
#ifndef AOO_XRUN_DETECTIOIN
 #define AOO_XRUN_DETECTION 1
#endif

/** \brief timer tolerance
 *
 * This is the tolerance for deviations from the nominal block
 * period in (fractional) blocks used by the xrun detection algorithm. */
#ifndef AOO_TIMER_TOLERANCE
 #define AOO_TIMER_TOLERANCE 0.25
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

/*---------------- public OSC interface ---------------*/

/* OSC address patterns */
#define kAooMsgDomain "/aoo"
#define kAooMsgDomainLen 4
#define kAooMsgSource "/src"
#define kAooMsgSourceLen 4
#define kAooMsgSink "/sink"
#define kAooMsgSinkLen 5
#define kAooMsgStart "/start"
#define kAooMsgStartLen 6
#define kAooMsgStop "/stop"
#define kAooMsgStopLen 5
#define kAooMsgData "/data"
#define kAooMsgDataLen 5
#define kAooMsgPing "/ping"
#define kAooMsgPingLen 5
#define kAooMsgInvite "/invite"
#define kAooMsgInviteLen 7
#define kAooMsgUninvite "/uninvite"
#define kAooMsgUninviteLen 9

/*------------------- binary message ---------------------*/

/* domain bit + type (uint8), size bit + cmd (uint8)
 * a) sink ID (uint8), source ID (uint8)
 * b) padding (uint16), sink ID (int32), source ID (int32)
 */

#define kAooBinMsgHeaderSize 4
#define kAooBinMsgLargeHeaderSize 12
#define kAooBinMsgDomainBit 0x80
#define kAooBinMsgSizeBit 0x80

#define kAooBinMsgCmdData 0
#define kAooBinMsgCmdRelayIPv4 0
#define kAooBinMsgCmdRelayIPv6 1

enum AooBinMsgDataFlags
{
    kAooBinMsgDataSampleRate = 0x01,
    kAooBinMsgDataFrames = 0x02
};

/*------------------ library functions --------------------*/

/** \brief settings for aoo_initializeEx() */
typedef struct AooSettings
{
    AooSize size; /** `sizeof(AooSettings)` */
    AooAllocFunc allocFunc; /** custom allocator function, or `NULL` */
    AooLogFunc logFunc; /** custom log function, or `NULL` */
} AooSettings;

/** \brief default initialization for AooSettings struct */
AOO_INLINE void AooSettings_init(AooSettings *settings)
{
    settings->size = sizeof(AooSettings);
    settings->allocFunc = NULL;
    settings->logFunc = NULL;
}

/**
 * \brief initialize AOO library settings
 *
 * \note Call before using any AOO functions!
 * \param settings (optional) settings struct
 */
AOO_API AooError AOO_CALL aoo_initialize(const AooSettings *settings);

/**
 * \brief terminate AOO library
 *
 * \note Call before program exit.
 */
AOO_API void AOO_CALL aoo_terminate(void);

/**
 * \brief get the AOO version number
 *
 * \param[out] major major version
 * \param[out] minor minor version
 * \param[out] patch bugfix version
 * \param[out] test test or pre-release version
 */
AOO_API void AOO_CALL aoo_getVersion(
        AooInt32 *major, AooInt32 *minor, AooInt32 *patch, AooInt32 *test);

/**
 * \brief get the AOO version string
 *
 * Format: `<major>[.<minor>][.<patch>][-test<test>]`
 * \return the version as a C string
 */
AOO_API const AooChar * AOO_CALL aoo_getVersionString(void);

/**
 * \brief get a textual description for an error code
 *
 * \param err the error code
 * \return a C string describing the error
 */
AOO_API const AooChar * AOO_CALL aoo_strerror(AooError err);

/**
 * \brief get the current NTP time
 *
 * \return NTP time stamp
 */
AOO_API AooNtpTime AOO_CALL aoo_getCurrentNtpTime(void);

/**
 * \brief convert NTP time to seconds
 *
 * \param t NTP time stamp
 * \return seconds
 */
AOO_API AooSeconds AOO_CALL aoo_ntpTimeToSeconds(AooNtpTime t);

/**
 * \brief convert seconds to NTP time
 *
 * \param s seconds
 * \return NTP time stamp
 */
AOO_API AooNtpTime AOO_CALL aoo_ntpTimeFromSeconds(AooSeconds s);

/**
 * \brief get time difference in seconds between two NTP time stamps
 *
 * \param t1 the first time stamp
 * \param t2 the second time stamp
 * \return the time difference in seconds
 */
AOO_API AooSeconds AOO_CALL aoo_ntpTimeDuration(AooNtpTime t1, AooNtpTime t2);

/**
 * \brief parse an AOO message
 *
 * Tries to obtain the AOO message type and ID from the address pattern,
 * like in `/aoo/src/<id>/data`.
 *
 * \param msg the OSC message data
 * \param size the OSC message size
 * \param[out] type the AOO message type
 * \param[out] id the source/sink ID
 * \param[out] offset pointer to the start of the remaining address pattern
 * \return error code
 */
AOO_API AooError AOO_CALL aoo_parsePattern(
        const AooByte *msg, AooInt32 size, AooMsgType *type, AooId *id, AooInt32 *offset);

