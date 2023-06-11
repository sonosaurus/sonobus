/* Copyright (c) 2021 Christof Ressi
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/** \file
 * \brief C++ interface for AOO source
 */

#pragma once

#include "aoo_config.h"
#include "aoo_controls.h"
#include "aoo_defines.h"
#include "aoo_events.h"
#include "aoo_types.h"

#if AOO_HAVE_CXX11
# include <memory>
#endif

typedef struct AooSource AooSource;

/** \brief create a new AOO source instance
 *
 * \param id the ID
 * \param[out] err (optional) error code on failure
 * \return new AooSource instance on success; `NULL` on failure
 */
AOO_API AooSource * AOO_CALL AooSource_new(AooId id, AooError *err);

/** \brief destroy the AOO source instance */
AOO_API void AOO_CALL AooSource_free(AooSource *source);

/*------------------------------------------------------------*/

/** \brief AOO source interface */
struct AooSource {
public:
#if AOO_HAVE_CXX11
    /** \brief custom deleter for AooSource */
    class Deleter {
    public:
        void operator()(AooSource *obj){
            AooSource_free(obj);
        }
    };

    /** \brief smart pointer for AOO source instance */
    using Ptr = std::unique_ptr<AooSource, Deleter>;

    /** \brief create a new managed AOO source instance
     *
     * \copydetails AooSource_new()
     */
    static Ptr create(AooId id, AooError *err) {
        return Ptr(AooSource_new(id, err));
    }
#endif

    /*------------------- methods -----------------------*/

    /** \brief setup AOO source
     *
     * \attention Not threadsafe - needs to be synchronized with other method calls!
     *
     * \param numChannels the max. number of channels
     * \param sampleRate the sample rate
     * \param maxBlockSize the max. number of samples per block
     * \param flags optional flags (currently always 0)
     */
    virtual AooError AOO_CALL setup(
            AooInt32 numChannels, AooSampleRate sampleRate,
            AooInt32 maxBlockSize, AooSetupFlags flags) = 0;

    /** \brief handle sink messages
     *
     * \note Threadsafe; call on the network thread
     *
     * \param data the message data
     * \param size the message size in bytes
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
     * \param fn the send function
     * \param user the user data (passed to the send function)
     */
    virtual AooError AOO_CALL send(AooSendFunc fn, void *user) = 0;

    /** \brief add a stream message
     *
     * \note Threadsafe and RT-safe; call on the audio thread
     *
     * Call this function to add messages for the \em next process block.
     *
     * \note if the sample offset is larger than the next process block,
     *       the message will be scheduled for later.
     *
     * \param message the message
     */
    virtual AooError AOO_CALL addStreamMessage(const AooStreamMessage& message) = 0;

    /** \brief process audio
     *
     * \note Threadsafe and RT-safe; call on the audio thread
     *
     * \param data an array of audio channels; the number of
     *        channels must match the number in #AooSource_setup.
     * \param numSamples the number of samples per channel
     * \param t current NTP time; \see aoo_getCurrentNtpTime
     */
    virtual AooError AOO_CALL process(
            AooSample **data, AooInt32 numSamples, AooNtpTime t) = 0;

    /** \brief set event handler function and event handling mode
     *
     * \attention Not threadsafe - only call in the beginning! */
    virtual AooError AOO_CALL setEventHandler(
            AooEventHandler fn, void *user, AooEventMode mode) = 0;

    /** \brief check for pending events
     *
     * \note Threadsafe and RT-safe */
    virtual AooBool AOO_CALL eventsAvailable() = 0;

    /** \brief poll events
     *
     * \note Threadsafe and RT-safe, but not reentrant.
     *
     * This function will call the registered event handler one or more times.
     * \attention The event handler must have been registered with #kAooEventModePoll.
     */
    virtual AooError AOO_CALL pollEvents() = 0;

    /** \brief Start a new stream
     *
     * \note Threadsafe, RT-safe and reentrant
     *
     * You can pass an optional AooData structure which will be sent as
     * additional stream metadata. For example, it could contain information
     * about the channel layout, the musical content, etc.
     */
    virtual AooError AOO_CALL startStream(
            const AooData *metadata) = 0;

    /** \brief Stop the current stream */
    virtual AooError AOO_CALL stopStream() = 0;

    /** \brief add sink
     *
     * If `active` is true, the sink will start active, otherwise
     * it has to be activated manually with AooSource_activateSink().
     */
    virtual AooError AOO_CALL addSink(
            const AooEndpoint& sink, AooBool active) = 0;

    /** \brief remove sink */
    virtual AooError AOO_CALL removeSink(const AooEndpoint& sink) = 0;

    /** \brief remove all sinks */
    virtual AooError AOO_CALL removeAll() = 0;

    /** \brief accept/decline an invitation
     *
     * When you receive an #kAooEventInvite event, you can decide to
     * accept or decline the invitation.
     *
     * \param sink The sink that has sent the invitation
     * \param token The invitation token (from the `AooEventInvite` event)
     * \param accept `kAooTrue`: accept invitation (activates the sink)
     *               `kAooFalse`: declines the invitation
     */
    virtual AooError AOO_CALL handleInvite(
            const AooEndpoint& sink, AooId token, AooBool accept) = 0;

    /** \brief accept/decline an uninvitation
     *
     * When you receive an #kAooEventUninvite event, you can decide to
     * accept or decline the uninvitation.
     *
     * \param sink The sink that has sent the uninvitation
     * \param token The uninvitation token (from the `AooEventUninvite` event)
     * \param accept `kAooTrue`: accept invitation (deactivates the sink)
     *               `kAooFalse`: declines the uninvitation
     */
    virtual AooError AOO_CALL handleUninvite(
            const AooEndpoint& sink, AooId token, AooBool accept) = 0;

    /** \brief control interface
     *
     * Not to be used directly. */
    virtual AooError AOO_CALL control(
            AooCtl ctl, AooIntPtr index, void *data, AooSize size) = 0;

    /** \brief codec control interface
     *
     * Not to be used directly. */
    virtual AooError AOO_CALL codecControl(
            AooCtl ctl, AooIntPtr index, void *data, AooSize size) = 0;

    /*--------------------------------------------*/
    /*         type-safe control functions        */
    /*--------------------------------------------*/

    /** \brief (De)activate the given sink */
    AooError activate(const AooEndpoint& sink, AooBool active) {
        return control(kAooCtlActivate, (AooIntPtr)&sink, AOO_ARG(active));
    }

    /** \brief Check whether the given sink is active */
    AooError isActive(const AooEndpoint& sink, AooBool& active) {
        return control(kAooCtlIsActive, (AooIntPtr)&sink, AOO_ARG(active));
    }

    /** \brief reset the source */
    AooError reset() {
        return control(kAooCtlReset, 0, nullptr, 0);
    }

    /** \brief Set the stream format
     *
     * \param[in,out] format Pointer to the format header.
     * The format struct is validated and updated on success!
     *
     * This will change the streaming format and consequently start a new stream.
     * The sink(s) will receive a `kAooEventFormatChange` event.
     */
    AooError setFormat(AooFormat& format) {
        return control(kAooCtlSetFormat, 0, AOO_ARG(format));
    }

    /** \brief Get the stream format
     *
     * \param[out] format Pointer to an instance of `AooFormatStorage`
     */
    AooError getFormat(AooFormatStorage& format) {
        return control(kAooCtlGetFormat, 0, AOO_ARG(format));
    }

    /** \brief Set AOO source ID
     * \param id The new ID
     */
    AooError setId(AooId id) {
        return control(kAooCtlSetId, 0, AOO_ARG(id));
    }

    /** \brief Get AOO source ID */
    AooError getId(AooId &id) {
        return control(kAooCtlGetId, 0, AOO_ARG(id));
    }

    /** \brief Set the buffer size in seconds (in seconds)
     *
     * This is the size of the ring buffer between the audio and network thread.
     * The value can be rather small, as you only have to compensate for the time
     * it takes to wake up the network thread.
     */
    AooError setBufferSize(AooSeconds s) {
        return control(kAooCtlSetBufferSize, 0, AOO_ARG(s));
    }

    /** \brief Get the current buffer size (in seconds) */
    AooError getBufferSize(AooSeconds& s) {
        return control(kAooCtlGetBufferSize, 0, AOO_ARG(s));
    }

    /** \brief Report xruns
     *
     * If your audio backend notifies you about xruns, you may report
     * them to AooSink. "Missing" samples will be substituted with empty
     * blocks and the dynamic resampler (if enabled) will be reset.
     */
    AooError reportXRun(AooInt32 numSamples) {
        return control(kAooCtlReportXRun, 0, AOO_ARG(numSamples));
    }

    /** \brief Enable/disable dynamic resampling
     *
     * Dynamic resampling attempts to mitigate timing differences
     * between different machines caused by internal clock drift.
     *
     * A DLL filter estimates the effective sample rate on both sides
     * and the audio data is resampled accordingly. The behavior can be
     * fine-tuned with AooSource::setDllBandWidth().
     *
     * See the paper "Using a DLL to filter time" by Fons Adriaensen.
     */
    AooError setDynamicResampling(AooBool b) {
        return control(kAooCtlSetDynamicResampling, 0, AOO_ARG(b));
    }

    /** \brief Check if dynamic resampling is enabled. */
    AooError getDynamicResampling(AooBool& b) {
        return control(kAooCtlGetDynamicResampling, 0, AOO_ARG(b));
    }

    /** \brief Get the "real" samplerate as measured by the DLL filter */
    AooError getRealSampleRate(AooSampleRate& sr) {
        return control(kAooCtlGetRealSampleRate, 0, AOO_ARG(sr));
    }

    /** \brief Set DLL filter bandwidth
     *
     * Used for dynamic resampling, see AooSource::setDynamicResampling().
     */
    AooError setDllBandwidth(double q) {
        return control(kAooCtlSetDllBandwidth, 0, AOO_ARG(q));
    }

    /** \brief get DLL filter bandwidth */
    AooError getDllBandwidth(double& q) {
        return control(kAooCtlGetDllBandwidth, 0, AOO_ARG(q));
    }

    /** \brief Reset the time DLL filter
     *
     * Use this if the audio thread experiences timing irregularities,
     * but you cannot use AooSource::reportXRun(). This function only
     * has an effect if dynamic resampling is enabled.
     */
    AooError resetDll() {
        return control(kAooCtlResetDll, 0, nullptr, 0);
    }

    /** \brief Set the max. UDP packet size in bytes
     *
     * The default value should be fine for most networks (including the internet),
     * but you might want to increase this value for local networks because larger
     * packet sizes have less overhead. If a audio block exceeds the max. UDP packet size,
     * it will be automatically broken up into several "frames" and then reassembled in the sink.
     */
    AooError setPacketSize(AooInt32 n) {
        return control(kAooCtlSetPacketSize, 0, AOO_ARG(n));
    }

    /** \brief Get the max. UDP packet size */
    AooError getPacketSize(AooInt32& n) {
        return control(kAooCtlGetPacketSize, 0, AOO_ARG(n));
    }

    /** \brief Set the ping interval (in seconds) */
    AooError setPingInterval(AooSeconds s) {
        return control(kAooCtlSetPingInterval, 0, AOO_ARG(s));
    }

    /** \brief Get the ping interval (in seconds) */
    AooError getPingInterval(AooSeconds& s) {
        return control(kAooCtlGetPingInterval, 0, AOO_ARG(s));
    }

    /** \brief Set the resend buffer size (in seconds)
     *
     * The source keeps the last N seconds of audio in a buffer, so it can resend
     * parts of it if requested by the sink (because of packet loss).
     *
     * If set to 0.0, resending is disabled.
     */
    AooError setResendBufferSize(AooSeconds s) {
        return control(kAooCtlSetResendBufferSize, 0, AOO_ARG(s));
    }

    /** \brief Get the resend buffer size (in seconds) */
    AooError getResendBufferSize(AooSeconds& s) {
        return control(kAooCtlGetResendBufferSize, 0, AOO_ARG(s));
    }

    /** \brief Set redundancy
     *
     * The number of times each frames is sent (default = 1). This is a primitive
     * strategy to cope with possible packet loss, but it can be counterproductive:
     * packet loss is often the result of network contention and sending more data
     * would only make it worse.
     */
    AooError setRedundancy(AooInt32 n) {
        return control(kAooCtlSetRedundancy, 0, AOO_ARG(n));
    }

    /** \brief Get redundancy */
    AooError getRedundancy(AooInt32& n) {
        return control(kAooCtlGetRedundancy, 0, AOO_ARG(n));
    }

    /** \brief Enable/disable binary data messages
     *
     * Use a more compact (and faster) binary format for the audio data message
     */
    AooError setBinaryDataMsg(AooBool b) {
        return control(kAooCtlSetBinaryDataMsg, 0, AOO_ARG(b));
    }

    /** \brief Check if binary data messages are enabled */
    AooError getBinaryDataMsg(AooBool& b) {
        return control(kAooCtlGetBinaryDataMsg, 0, AOO_ARG(b));
    }

    /** \brief Set the sink channel onset
     *
     * Set channel onset of the given sink where the source signal should be received.
     * For example, if the channel onset is 5, a 2-channel source signal will be summed
     * into sink channels 5 and 6. The default is 0 (= the first channel).
     */
    AooError setSinkChannelOnset(const AooEndpoint& sink, AooInt32 onset) {
        return control(kAooCtlSetChannelOnset, (AooIntPtr)&sink, AOO_ARG(onset));
    }

    /** \brief Get the sink channel onset for the given sink */
    AooError getSinkChannelOnset(const AooEndpoint& sink, AooInt32& onset) {
        return control(kAooCtlSetChannelOnset, (AooIntPtr)&sink, AOO_ARG(onset));
    }
protected:
    ~AooSource(){} // non-virtual!
};
