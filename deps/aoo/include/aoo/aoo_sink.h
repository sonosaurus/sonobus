/* Copyright (c) 2021 Christof Ressi
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/** \file
 * \brief C interface for AOO sink
 */

#pragma once

#include "aoo.h"
#include "aoo_events.h"
#include "aoo_controls.h"

typedef struct AooSink AooSink;

/** \brief create a new AOO sink instance
 *
 * \param id the ID
 * \param flags optional flags
 * \param[out] err error code on failure
 * \return new AooSink instance on success; `NULL` on failure
 */
AOO_API AooSink * AOO_CALL AooSink_new(
        AooId id, AooFlag flags, AooError *err);

/** \brief destroy the AOO sink instance */
AOO_API void AOO_CALL AooSink_free(AooSink *sink);

/** \copydoc AooSink::setup() */
AOO_API AooError AOO_CALL AooSink_setup(
        AooSink *sink, AooSampleRate sampleRate,
        AooInt32 blockSize, AooInt32 numChannels);

/** \copydoc AooSink::handleMessage() */
AOO_API AooError AOO_CALL AooSink_handleMessage(
        AooSink *sink, const AooByte *data, AooInt32 size,
        const void *address, AooAddrSize addrlen);

/** \copydoc AooSink::send() */
AOO_API AooError AOO_CALL AooSink_send(
        AooSink *sink, AooSendFunc fn, void *user);

/** \copydoc AooSink::process() */
AOO_API AooError AOO_CALL AooSink_process(
        AooSink *sink, AooSample **data, AooInt32 numSamples, AooNtpTime t);

/** \copydoc AooSink::setEventHandler() */
AOO_API AooError AOO_CALL AooSink_setEventHandler(
        AooSink *sink, AooEventHandler fn, void *user, AooEventMode mode);

/** \copydoc AooSink::eventsAvailable() */
AOO_API AooBool AOO_CALL AooSink_eventsAvailable(AooSink *sink);

/** \copydoc AooSink::pollEvents() */
AOO_API AooError AOO_CALL AooSink_pollEvents(AooSink *sink);

/** \copydoc AooSink::inviteSource() */
AOO_API AooError AOO_CALL AooSink_inviteSource(
        AooSink *sink, const AooEndpoint *source,
        const AooDataView *metadata);

/** \copydoc AooSink::uninviteSource() */
AOO_API AooError AOO_CALL AooSink_uninviteSource(
        AooSink *sink, const AooEndpoint *source);

/** \copydoc AooSink::uninviteAll() */
AOO_API AooError AOO_CALL AooSink_uninviteAll(AooSink *sink);

/** \copydoc AooSink::control() */
AOO_API AooError AOO_CALL AooSink_control(
        AooSink *sink, AooCtl ctl, AooIntPtr index,
        void *data, AooSize size);

/** \copydoc AooSink::codecControl() */
AOO_API AooError AOO_CALL AooSink_codecControl(
        AooSink *sink, AooCtl ctl, AooIntPtr index,
        void *data, AooSize size);

/*--------------------------------------------*/
/*         type-safe control functions        */
/*--------------------------------------------*/

/** \copydoc AooSink::setId() */
AOO_INLINE AooError AooSink_setId(AooSink *sink, AooId id)
{
    return AooSink_control(sink, kAooCtlSetId, 0, AOO_ARG(id));
}

/** \copydoc AooSink::getId() */
AOO_INLINE AooError AooSink_getId(AooSink *sink, AooId *id)
{
    return AooSink_control(sink, kAooCtlGetId, 0, AOO_ARG(*id));
}

/** \copydoc AooSink::reset() */
AOO_INLINE AooError AooSink_reset(AooSink *sink)
{
    return AooSink_control(sink, kAooCtlReset, 0, 0, 0);
}

/** \copydoc AooSink::setBufferSize() */
AOO_INLINE AooError AooSink_setBufferSize(AooSink *sink, AooSeconds s)
{
    return AooSink_control(sink, kAooCtlSetBufferSize, 0, AOO_ARG(s));
}

/** \copydoc AooSink::getBufferSize() */
AOO_INLINE AooError AooSink_getBufferSize(AooSink *sink, AooSeconds *s)
{
    return AooSink_control(sink, kAooCtlGetBufferSize, 0, AOO_ARG(*s));
}

/** \copydoc AooSink::setXRunDetection() */
AOO_INLINE AooError AooSink_setXRunDetection(AooSink *sink, AooBool b)
{
    return AooSink_control(sink, kAooCtlSetXRunDetection, 0, AOO_ARG(b));
}

/** \copydoc AooSink::getXRunDetection() */
AOO_INLINE AooError AooSink_getXRunDetection(AooSink *sink, AooBool *b)
{
    return AooSink_control(sink, kAooCtlGetXRunDetection, 0, AOO_ARG(*b));
}

/** \copydoc AooSink::setDynamicResampling() */
AOO_INLINE AooError AooSink_setDynamicResampling(AooSink *sink, AooBool b)
{
    return AooSink_control(sink, kAooCtlSetDynamicResampling, 0, AOO_ARG(b));
}

/** \copydoc AooSink::getDynamicResampling() */
AOO_INLINE AooError AooSink_getDynamicResampling(AooSink *sink, AooBool *b)
{
    return AooSink_control(sink, kAooCtlGetDynamicResampling, 0, AOO_ARG(*b));
}

/** \copydoc AooSink::getRealSampleRate() */
AOO_INLINE AooError AooSink_getRealSampleRate(AooSink *sink, AooSampleRate *sr)
{
    return AooSink_control(sink, kAooCtlGetRealSampleRate, 0, AOO_ARG(*sr));
}

/** \copydoc AooSink::setDllBandwidth() */
AOO_INLINE AooError AooSink_setDllBandwidth(AooSink *sink, double q)
{
    return AooSink_control(sink, kAooCtlSetDllBandwidth, 0, AOO_ARG(q));
}

/** \copydoc AooSink::getDllBandwidth() */
AOO_INLINE AooError AooSink_getDllBandwidth(AooSink *sink, double *q)
{
    return AooSink_control(sink, kAooCtlGetDllBandwidth, 0, AOO_ARG(*q));
}

/** \copydoc AooSink::setPacketSize() */
AOO_INLINE AooError AooSink_setPacketSize(AooSink *sink, AooInt32 n)
{
    return AooSink_control(sink, kAooCtlSetPacketSize, 0, AOO_ARG(n));
}

/** \copydoc AooSink::getPacketSize() */
AOO_INLINE AooError AooSink_getPacketSize(AooSink *sink, AooInt32 *n)
{
    return AooSink_control(sink, kAooCtlGetPacketSize, 0, AOO_ARG(*n));
}

/** \copydoc AooSink::setResendData() */
AOO_INLINE AooError AooSink_setResendData(AooSink *sink, AooBool b)
{
    return AooSink_control(sink, kAooCtlSetResendData, 0, AOO_ARG(b));
}

/** \copydoc AooSink::getResendData() */
AOO_INLINE AooError AooSink_getResendData(AooSink *sink, AooBool *b)
{
    return AooSink_control(sink, kAooCtlGetResendData, 0, AOO_ARG(*b));
}

/** \copydoc AooSink::setResendInterval() */
AOO_INLINE AooError AooSink_setResendInterval(AooSink *sink, AooSeconds s)
{
    return AooSink_control(sink, kAooCtlSetResendInterval, 0, AOO_ARG(s));
}

/** \copydoc AooSink::getResendInterval() */
AOO_INLINE AooError AooSink_getResendInterval(AooSink *sink, AooSeconds *s)
{
    return AooSink_control(sink, kAooCtlGetResendInterval, 0, AOO_ARG(*s));
}

/** \copydoc AooSink::setResendLimit() */
AOO_INLINE AooError AooSink_setResendLimit(AooSink *sink, AooInt32 n)
{
    return AooSink_control(sink, kAooCtlSetResendLimit, 0, AOO_ARG(n));
}

/** \copydoc AooSink::getResendLimit() */
AOO_INLINE AooError AooSink_getResendLimit(AooSink *sink, AooInt32 *n)
{
    return AooSink_control(sink, kAooCtlGetResendLimit, 0, AOO_ARG(*n));
}

/** \copydoc AooSink::setSourceTimeout() */
AOO_INLINE AooError AooSink_setSourceTimeout(AooSink *sink, AooSeconds s)
{
    return AooSink_control(sink, kAooCtlSetSourceTimeout, 0, AOO_ARG(s));
}

/** \copydoc AooSink::getSourceTimeout() */
AOO_INLINE AooError AooSink_getSourceTimeout(AooSink *sink, AooSeconds *s)
{
    return AooSink_control(sink, kAooCtlGetSourceTimeout, 0, AOO_ARG(*s));
}

/** \copydoc AooSink::setInviteTimeout() */
AOO_INLINE AooError AooSink_setInviteTimeout(AooSink *sink, AooSeconds s)
{
    return AooSink_control(sink, kAooCtlSetInviteTimeout, 0, AOO_ARG(s));
}

/** \copydoc AooSink::getInviteTimeout() */
AOO_INLINE AooError AooSink_getInviteTimeout(AooSink *sink, AooSeconds *s)
{
    return AooSink_control(sink, kAooCtlGetInviteTimeout, 0, AOO_ARG(*s));
}

/** \copydoc AooSink::resetSource() */
AOO_INLINE AooError AooSink_resetSource(AooSink *sink, const AooEndpoint *source)
{
    return AooSink_control(sink, kAooCtlReset, (AooIntPtr)source, 0, 0);
}

/** \copydoc AooSink::getSourceFormat() */
AOO_INLINE AooError AooSink_getSourceFormat(
        AooSink *sink, const AooEndpoint *source, AooFormat *format)
{
    return AooSink_control(sink, kAooCtlGetFormat, (AooIntPtr)source, AOO_ARG(*format));
}

/** \copydoc AooSink::getBufferFillRatio() */
AOO_INLINE AooError AooSink_getBufferFillRatio(
        AooSink *sink, const AooEndpoint *source, double *ratio)
{
    return AooSink_control(sink, kAooCtlGetBufferFillRatio, (AooIntPtr)source, AOO_ARG(*ratio));
}
