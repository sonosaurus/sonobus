/* Copyright (c) 2021 Christof Ressi
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/** \file
 * \brief C++ interface for AOO sink
 */

#pragma once

#include "aoo_sink.h"

#include <memory>

/** \brief AOO sink interface */
struct AooSink {
public:
    /** \brief custom deleter for AooSink */
    class Deleter {
    public:
        void operator()(AooSink *obj){
            AooSink_free(obj);
        }
    };

    /** \brief smart pointer for AOO sink instance */
    using Ptr = std::unique_ptr<AooSink, Deleter>;

    /** \brief create a new managed AOO sink instance
     *
     * \copydetails AooSink_new()
     */
    static Ptr create(AooId id, AooFlag flags, AooError *err) {
        return Ptr(AooSink_new(id, flags, err));
    }

    /*-------------------- methods -----------------------------*/

    /** \brief setup AOO sink
     *
     * \attention Not threadsafe - needs to be synchronized with other method calls!
     *
     * \param sampleRate the sample rate
     * \param blockSize the max. blocksize
     * \param numChannels the max. number of channels
     */
    virtual AooError AOO_CALL setup(
            AooSampleRate sampleRate, AooInt32 blockSize, AooInt32 numChannels) = 0;

    /** \brief handle source messages
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

    /** \brief process audio
     *
     * \note Threadsafe and RT-safe; call on the audio thread
     *
     * \param data an array of audio channels; the number of
     *        channels must match the number in #AooSource_setup.
     * \param numSamples the number of samples per channel
     * \param t current NTP time; \see aoo_getCurrentNtpTime
     */
    virtual AooError AOO_CALL process(AooSample **data, AooInt32 numSamples,
                                      AooNtpTime t) = 0;

    /** \brief set event handler function and event handling mode
     *
     * \attention Not threadsafe - only call in the beginning!
     */
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

    /** \brief invite source
     *
     * This will continuously send invitation requests to the source
     * The source can either accept the invitation request and start a
     * stream or it can ignore it, upon which the sink will eventually
     * receive an AooEventInviteTimeout event.
     * If you call this function while you are already receiving a stream,
     * it will force a new stream. For example, you might want to request
     * different format parameters or even ask for different musical content.
     *
     * \param source the AOO source to be invited
     * \param metadata optional metadata that the source can interpret
     *        before accepting the invitation
     */
    virtual AooError AOO_CALL inviteSource(
            const AooEndpoint& source, const AooDataView *metadata) = 0;

    /** \brief uninvite source
     *
     * This will continuously send uninvitation requests to the source.
     * The source can either accept the uninvitation request and stop the
     * stream, or it can ignore and continue sending, upon which the sink
     * will eventually receive an #kAooEventUninviteTimeout event.
     *
     * \param source the AOO source to be uninvited
     */
    virtual AooError AOO_CALL uninviteSource(const AooEndpoint& source) = 0;

    /** \brief uninvite all sources */
    virtual AooError AOO_CALL uninviteAll() = 0;

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

    /** \brief Reset the sink */
    AooError reset() {
        return control(kAooCtlReset, 0, nullptr, 0);
    }

    /** \brief Set the buffer size in seconds (in seconds)
     *
     * This is the size of the ring buffer between the audio and network thread.
     *
     * A larger buffer size helps to deal with network jitter, packet reordering
     * and packet loss.
     *
     * For local networks small buffersizes between 10-50ms should work;
     * for unreliable/unpredictable networks you might need to increase it
     * significantly if you want to avoid dropouts.
     *
     * Some codecs, like Opus, offer packet loss concealment, so you can use
     * very low buffer sizes and still get acceptable results.
     */
    AooError setBufferSize(AooSeconds s) {
        return control(kAooCtlSetBufferSize, 0, AOO_ARG(s));
    }

    /** \brief Get the current buffer size (in seconds) */
    AooError getBufferSize(AooSeconds& s) {
        return control(kAooCtlGetBufferSize, 0, AOO_ARG(s));
    }

    /** \brief Enable/disable xrun detection
     *
     * xrun detection helps to catch timing problems, e.g. when the host accidentally
     * blocks the audio callback, which would confuse the time DLL filter.
     * Also, timing gaps are handled by dropping blocks at the sink.
     * \attention: only takes effect after calling AooSink::setup()!
     */
    AooError setXRunDetection(AooBool b) {
        return control(kAooCtlSetXRunDetection, 0, AOO_ARG(b));
    }

    /** \brief Check if xrun detection is enabled */
    AooError getXRunDetection(AooBool b) {
        return control(kAooCtlGetXRunDetection, 0, AOO_ARG(b));
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
    AooError getDynamicResampling(AooBool b) {
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

    /** \brief Set the max. UDP packet size in bytes
     *
     * The default value should be fine for most networks (including the internet),
     * but you might want to increase this value for local networks because larger
     * packet sizes have less overhead. This is mostly relevant for resend requests.
     */
    AooError setPacketSize(AooInt32 n) {
        return control(kAooCtlSetPacketSize, 0, AOO_ARG(n));
    }

    /** \brief Get the max. UDP packet size */
    AooError getPacketSize(AooInt32& n) {
        return control(kAooCtlGetPacketSize, 0, AOO_ARG(n));
    }

    /** \brief Enable/disable data resending */
    AooError setResendData(AooBool b) {
        return control(kAooCtlSetResendData, 0, AOO_ARG(b));
    }

    /** \brief Check if data resending is enabled */
    AooError getResendData(AooBool& b) {
        return control(kAooCtlGetResendData, 0, AOO_ARG(b));
    }

    /** \brief Set resend interval (in seconds)
     *
     * This is the interval between individual resend attempts for a specific frame.
     * Since there is always a certain roundtrip delay between source and sink,
     * it makes sense to wait between resend attempts to not spam the network
     * with redundant messages.
     */
    AooError setResendInterval(AooSeconds s) {
        return control(kAooCtlSetResendInterval, 0, AOO_ARG(s));
    }

    /** \brief Get resend interval (in seconds) */
    AooError getResendInterval(AooSeconds& s) {
        return control(kAooCtlGetResendInterval, 0, AOO_ARG(s));
    }

    /** \brief Set the frame resend limit
     *
     * This is the max. number of frames to request in a single process call.
     */
    AooError setResendLimit(AooInt32 n) {
        return control(kAooCtlSetResendLimit, 0, AOO_ARG(n));
    }

    /** \brief Get the frame resend limit */
    AooError getResendLimit(AooInt32& n) {
        return control(kAooCtlGetResendLimit, 0, AOO_ARG(n));
    }

    /** \brief Set source timeout (in seconds)
     *
     * The time to wait before removing inactive sources
     */
    AooError setSourceTimeout(AooSeconds s) {
        return control(kAooCtlSetSourceTimeout, 0, AOO_ARG(s));
    }

    /** \brief Get source timeout (in seconds) */
    AooError getSourceTimeout(AooSeconds& s) {
        return control(kAooCtlGetSourceTimeout, 0, AOO_ARG(s));
    }

    /** \brief Set (un)invite timeout (in seconds)
     *
     * Time to wait before stopping the (un)invite process.
     */
    AooError setInviteTimeout(AooSeconds s) {
        return control(kAooCtlSetInviteTimeout, 0, AOO_ARG(s));
    }

    /** \brief Get (un)invite timeout (in seconds) */
    AooError getInviteTimeout(AooSeconds& s) {
        return control(kAooCtlGetInviteTimeout, 0, AOO_ARG(s));
    }

    /** \brief Reset a specific source */
    AooError resetSource(const AooEndpoint& source) {
        return control(kAooCtlReset, (AooIntPtr)&source, nullptr, 0);
    }

    /** \brief Get the source stream format
     *
     * \param source The source endpoint.
     * \param[out] format Pointer to an instance of `AooFormatStorage` or a similar struct
     * that is large enough to hold any codec format. The `size` member in the format header
     * should contain the storage size; on success it is updated to the actual format size.
     */
    AooError getSourceFormat(const AooEndpoint& source, AooFormatStorage& format) {
        return control(kAooCtlGetFormat, (AooIntPtr)&source, AOO_ARG(format));
    }

    /** \brief Get the current buffer fill ratio
     *
     * \param source The source endpoint.
     * \param[out] ratio The current fill ratio (0.0: empty, 1.0: full)
     */
    AooError getBufferFillRatio(const AooEndpoint& source, double& ratio) {
        return control(kAooCtlGetBufferFillRatio, (AooIntPtr)&source, AOO_ARG(ratio));
    }
protected:
    ~AooSink(){} // non-virtual!
};
