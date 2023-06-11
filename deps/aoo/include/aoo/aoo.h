/* Copyright (c) 2021 Christof Ressi
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/** \file
 * \brief main library API
 *
 * This file contains default values and important library routines.
 * It also describes the public OSC interface. */

#pragma once

#include "aoo_config.h"
#include "aoo_defines.h"
#include "aoo_types.h"

AOO_PACK_BEGIN

/*------------------------------------------------------*/

/** \brief settings for aoo_initialize()
 *
 * \attention Always call AooSettings_init()!
 */
typedef struct AooSettings
{
    AooSize structSize;
    AooAllocFunc allocFunc; /** custom allocator function, or `NULL` */
    AooLogFunc logFunc; /** custom log function, or `NULL` */
    AooSize memPoolSize; /** size of RT memory pool */
} AooSettings;

/** \brief default initialization for AooSettings struct */
AOO_INLINE void AooSettings_init(AooSettings *settings)
{
    AOO_STRUCT_INIT(settings, AooSettings, memPoolSize);
    settings->allocFunc = NULL;
    settings->logFunc = NULL;
    settings->memPoolSize = AOO_MEM_POOL_SIZE;
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

/**
 * \brief get AooData type from string representation
 *
 * \param str the string
 * \return error the data type on success, kAooDataUnspecified on failure
 */
AOO_API AooDataType AOO_CALL aoo_dataTypeFromString(const AooChar *str);

/**
 * \brief convert AooData type to string representation
 * \param type the data type
 * \return a C string on success, NULL if the data type is not valid
 */
AOO_API const AooChar * AOO_CALL aoo_dataTypeToString(AooDataType type);

/*------------------------------------------------------*/

AOO_PACK_END
