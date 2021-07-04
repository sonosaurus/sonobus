/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others. 
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#pragma once

#include "aoo_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

AOO_API const char *aoo_error_message(aoo_error e);

AOO_API void aoo_version(int32_t *major, int32_t *minor,
                         int32_t *patch, int32_t *pre);

AOO_API const char *aoo_version_string(void);

// call before aoo_initialize!
AOO_API void aoo_set_logfunction(aoo_logfunction fn, void *context);

#ifndef AOO_CUSTOM_ALLOCATOR
#define AOO_CUSTOM_ALLOCATOR 0
#endif

#if AOO_CUSTOM_ALLOCATOR
// call before aoo_initialize!
AOO_API void aoo_set_allocator(const aoo_allocator *alloc);

#endif

#ifndef AOO_DEBUG_MEMORY
 #define AOO_DEBUG_MEMORY 0
#endif

#ifndef AOO_DEBUG_DATA
 #define AOO_DEBUG_DATA 0
#endif

#ifndef AOO_DEBUG_DLL
 #define AOO_DEBUG_DLL 0
#endif

#ifndef AOO_DEBUG_TIMER
 #define AOO_DEBUG_TIMER 0
#endif

#ifndef AOO_DEBUG_RESAMPLER
 #define AOO_DEBUG_RESAMPLER 0
#endif

#ifndef AOO_DEBUG_AUDIO_BUFFER
 #define AOO_DEBUG_AUDIO_BUFFER 0
#endif

#ifndef AOO_DEBUG_JITTER_BUFFER
 #define AOO_DEBUG_JITTER_BUFFER 0
#endif

#ifndef AOO_CLIP_OUTPUT
#define AOO_CLIP_OUTPUT 0
#endif

/*////////// default values ////////////*/

// source buffer size in ms
#ifndef AOO_SOURCE_BUFFERSIZE
 #define AOO_SOURCE_BUFFERSIZE 25
#endif

// sink buffer size in ms
#ifndef AOO_SINK_BUFFERSIZE
 #define AOO_SINK_BUFFERSIZE 50
#endif

// enable/disable dynamic resampling
#ifndef AOO_BINARY_DATA_MSG
 #define AOO_BINARY_DATA_MSG 1
#endif

// enable/disable dynamic resampling
#ifndef AOO_DYNAMIC_RESAMPLING
 #define AOO_DYNAMIC_RESAMPLING 1
#endif

// time DLL filter bandwidth
#ifndef AOO_DLL_BANDWIDTH
 #define AOO_DLL_BANDWIDTH 0.012
#endif

// enable/disable timer check to
// catch timing issues (e.g. blocking audio thread)
#ifndef AOO_TIMER_CHECK
 #define AOO_TIMER_CHECK 1
#endif

// the tolerance for deviations from the nominal block period
#ifndef AOO_TIMER_TOLERANCE
 #define AOO_TIMER_TOLERANCE 0.25
#endif

// ping interval (sink to source) in ms
#ifndef AOO_PING_INTERVAL
 #define AOO_PING_INTERVAL 1000
#endif

// resend buffer size in ms
#ifndef AOO_RESEND_BUFFERSIZE
 #define AOO_RESEND_BUFFERSIZE 1000
#endif

// send redundancy
#ifndef AOO_SEND_REDUNDANCY
 #define AOO_SEND_REDUNDANCY 1
#endif

// enable/disable packet resending
#ifndef AOO_RESEND_DATA
 #define AOO_RESEND_DATA 1
#endif

// interval between resend attempts in ms
#ifndef AOO_RESEND_INTERVAL
 #define AOO_RESEND_INTERVAL 10
#endif

// max. number of frames to request per call
#ifndef AOO_RESEND_LIMIT
 #define AOO_RESEND_LIMIT 16
#endif

// max. number of ms to wait before removing source
#ifndef AOO_SOURCE_TIMEOUT
 #define AOO_SOURCE_TIMEOUT 1000
#endif

// initialize AoO library - call only once!
AOO_API void aoo_initialize(void);

// terminate AoO library - call only once!
AOO_API void aoo_terminate(void);

/*//////////////////// OSC ////////////////////////////*/

#define AOO_MSG_SOURCE "/src"
#define AOO_MSG_SOURCE_LEN 4
#define AOO_MSG_SINK "/sink"
#define AOO_MSG_SINK_LEN 5
#define AOO_MSG_FORMAT "/format"
#define AOO_MSG_FORMAT_LEN 7
#define AOO_MSG_DATA "/data"
#define AOO_MSG_DATA_LEN 5
#define AOO_MSG_PING "/ping"
#define AOO_MSG_PING_LEN 5
#define AOO_MSG_INVITE "/invite"
#define AOO_MSG_INVITE_LEN 7
#define AOO_MSG_UNINVITE "/uninvite"
#define AOO_MSG_UNINVITE_LEN 9

// get the aoo_type and ID from an AoO OSC message, e.g. in /aoo/src/<id>/data
// returns the offset on success, 0 on fail
AOO_API aoo_error aoo_parse_pattern(const char *msg, int32_t n,
                                    aoo_type *type, aoo_id *id, int32_t *offset);

// get the current NTP time
AOO_API uint64_t aoo_osctime_now(void);

// convert NTP time to seconds
AOO_API double aoo_osctime_to_seconds(uint64_t t);

// convert seconds to NTP time
AOO_API uint64_t aoo_osctime_from_seconds(double s);

// get time difference in seconds between two NTP timestamps
AOO_API double aoo_osctime_duration(uint64_t t1, uint64_t t2);

/*//////////////////// AoO events /////////////////////*/

#define AOO_EVENTQUEUESIZE 8

// event types
typedef enum aoo_event_type
{
    AOO_ERROR_EVENT = 0,
    // sink/source: received a ping from source/sink
    AOO_PING_EVENT,
    // source/sink: xruns occurred
    AOO_XRUN_EVENT,
    // source: invited by sink
    AOO_INVITE_EVENT,
    // source: uninvited by sink
    AOO_UNINVITE_EVENT,
    // source: format request by sink
    AOO_FORMAT_REQUEST_EVENT,
    // sink: source added
    AOO_SOURCE_ADD_EVENT,
    // sink: source removed
    AOO_SOURCE_REMOVE_EVENT,
    // sink: source format changed
    AOO_FORMAT_CHANGE_EVENT,
    // sink: source changed state
    AOO_STREAM_STATE_EVENT,
    // sink: buffer underrun
    AOO_BUFFER_UNDERRUN_EVENT,
    // sink: blocks have been lost
    AOO_BLOCK_LOST_EVENT,
    // sink: blocks arrived out of order
    AOO_BLOCK_REORDERED_EVENT,
    // sink: blocks have been resent
    AOO_BLOCK_RESENT_EVENT,
    // sink: large gap between blocks
    AOO_BLOCK_GAP_EVENT,
    // sink: block has been dropped by source
    AOO_BLOCK_DROPPED_EVENT,
    // sink: source invitation timed out
    AOO_INVITE_TIMEOUT_EVENT,
    // sink: format request timed out
    AOO_FORMAT_TIMEOUT_EVENT
} aoo_event_type;

typedef struct aoo_error_event
{
    int32_t type;
    int32_t error_code;
    const char *error_message;
} aoo_error_event;

typedef struct aoo_xrun_event
{
    int32_t type;
    int32_t count;
} aoo_xrun_event;

#define AOO_ENDPOINT_EVENT  \
    int32_t type;           \
    aoo_id id;              \
    const void *address;    \
    int32_t addrlen;        \

// source event
typedef struct aoo_source_event
{
    int32_t type;
    aoo_endpoint ep;
} aoo_source_event;

// sink event
typedef struct aoo_sink_event
{
    int32_t type;
    aoo_endpoint ep;
} aoo_sink_event;

// source state event
typedef enum aoo_stream_state
{
    AOO_STREAM_STATE_INIT = -1,
    AOO_STREAM_STATE_STOP = 0,
    AOO_STREAM_STATE_PLAY = 1
} aoo_stream_state;

typedef struct aoo_stream_state_event
{
    int32_t type;
    aoo_endpoint ep;
    int32_t state;
} aoo_stream_state_event;

// block events
struct _aoo_block_event
{
    int32_t type;
    aoo_endpoint ep;
    int32_t count;
};

typedef struct _aoo_block_event aoo_block_lost_event;
typedef struct _aoo_block_event aoo_block_reordered_event;
typedef struct _aoo_block_event aoo_block_resent_event;
typedef struct _aoo_block_event aoo_block_gap_event;
typedef struct _aoo_block_event aoo_block_dropped_event;

typedef aoo_sink_event aoo_invite_event;
typedef aoo_sink_event aoo_uninvite_event;
typedef aoo_source_event aoo_invite_timeout_event;
typedef aoo_source_event aoo_format_timeout_event;
typedef aoo_source_event aoo_buffer_underrun_event;

// ping event
typedef struct aoo_ping_event
{
    int32_t type;
    aoo_endpoint ep;
    uint64_t tt1;
    uint64_t tt2;
    uint64_t tt3; // only for source
    int32_t lost_blocks; // only for source
} aoo_ping_event;

// format event
typedef struct aoo_format_event
{
    int32_t type;
    aoo_endpoint ep;
    const struct aoo_format *format;
} aoo_format_event;

typedef aoo_format_event aoo_format_change_event;
typedef aoo_format_event aoo_format_request_event;

/*//////////////////// AoO options ////////////////////*/

typedef enum aoo_option
{
    // Set/get source/sink ID
    AOO_CTL_SET_ID = 0,
    AOO_CTL_GET_ID,
    // add/remove sink
    AOO_CTL_ADD_SINK,
    AOO_CTL_REMOVE_SINK,
    // invite/uninvite source
    AOO_CTL_INVITE_SOURCE,
    AOO_CTL_UNINVITE_SOURCE,
    // Set the stream format (aoo_format)
    // ---
    // Set the source format by passing a pointer to the format header.
    // The format struct is validated and updated on success!
    //
    // This will change the streaming format and consequently start a
    // new stream. The sink(s) will receive a AOO_FORMAT_CHANGE event
    AOO_CTL_SET_FORMAT,
    // Get the stream format (aoo_format)
    // ---
    // Get the format by passing a pointer to an instance of
    // 'aoo_format_storage' or a similar struct that is large enough
    // to hold any format. On success, the actual format size will be
    // contained in the 'size' member of the format header.
    AOO_CTL_GET_FORMAT,
    // Request stream format (aoo_format)
    // ---
    // A sink can request the streaming format for a specific source,
    // which can choose to accept or decline the request.
    // The source will receive one or more AOO_FORMAT_REQUEST events.
    // If the request has timed out, the sink receives a AOO_FORMAT_TIMEOUT event.
    AOO_CTL_REQUEST_FORMAT,
    // Reset the source/sink (NULL)
    AOO_CTL_RESET,
    // Start the source/sink (NULL)
    AOO_CTL_START,
    // Stop the source/sink (NULL)
    AOO_CTL_STOP,
    // Set/get buffer size in ms (int32_t)
    // ---
    // This is the size of the ring buffer
    // between the audio and network thread.
    // For the source, this can be rather small,
    // as you only have to compensate the latency
    // for thread signalling.
    // For the sink, a larger buffer size helps
    // to deal with network jitter, packet reordering
    // and packet loss. For local networks, small
    // buffersizes between 10-50ms should work,
    // but for unreliable/unpredictable networks
    // you might need to increased it significantly.
    AOO_CTL_SET_BUFFERSIZE,
    AOO_CTL_GET_BUFFERSIZE,
    // Enable/disable dynamic resampling (aoo_bool)
    // ---
    // Dynamic resampling attempts to mitigate CPU clock drift
    // between two different machines.
    // A DLL filter estimates the effective samplerate on both sides,
    // and the audio data is resampled accordingly. The behavior
    // can be fine-tuned with the AOO_CTL_DLL_BANDWIDTH option.
    // See the paper "Using a DLL to filter time" by Fons Adriaensen.
    AOO_CTL_SET_DYNAMIC_RESAMPLING,
    AOO_CTL_GET_DYNAMIC_RESAMPLING,
    // Real samplerate (double)
    // ---
    // Get effective samplerate as estimated by DLL.
    AOO_CTL_GET_REAL_SAMPLERATE,
    // Set/get DLL filter bandwidth (float)
    // ---
    // Used for dynamic resampling, see AOO_CTL_DYNAMIC_RESAMPLING.
    AOO_CTL_SET_DLL_BANDWIDTH,
    AOO_CTL_GET_DLL_BANDWIDTH,
    // Enable/disable timer check (aoo_bool)
    // ---
    // Enable to catch timing problems, e.g. when the host accidentally
    // blocks the audio callback, which would confuse the time DLL filter.
    // Also, timing gaps are handled by sending empty blocks at the source
    // resp. dropping blocks at the sink.
    // NOTE: only takes effect on source/sink setup!
    AOO_CTL_SET_TIMER_CHECK,
    AOO_CTL_GET_TIMER_CHECK,
    // Set/get sink channel onset (int32_t)
    // ---
    // The channel onset of the sink where a given source
    // should be received. For example, if the channel onset
    // is 5, a 2-channel source will be summed into sink
    // channels 5 and 6. The default is 0 (= the first channel).
    AOO_CTL_SET_CHANNEL_ONSET,
    AOO_CTL_GET_CHANNEL_ONSET,
    // Set/get max. UDP packet size in bytes (int32_t)
    // ---
    // The default value of 512 should work across most
    // networks (even the internet). You might increase
    // this value for local networks. Larger packet sizes
    // have less overhead. If a audio block exceeds
    // the max. UDP packet size, it will be automatically
    // broken up into several "frames" in reassembled
    // in the sink.
    AOO_CTL_SET_PACKETSIZE,
    AOO_CTL_GET_PACKETSIZE,
    // Set/get ping interval in ms (int32_t)
    // ---
    // The sink sends a periodic ping message to each
    // source to signify that it is actually receiving data.
    // For example, a application might choose to remove
    // a sink after the source hasn't received a ping
    // for a certain amount of time.
    AOO_CTL_SET_PING_INTERVAL,
    AOO_CTL_GET_PING_INTERVAL,
    // Enable/disable resending (aoo_bool)
    AOO_CTL_SET_RESEND_DATA,
    AOO_CTL_GET_RESEND_DATA,
    // Set/get resend buffer size in ms (int32_t).
    // ---
    // The source keeps the last N ms of audio in a buffer,
    // so it can resend parts of it, if requested, e.g. to
    // handle packet loss.
    AOO_CTL_SET_RESEND_BUFFERSIZE,
    AOO_CTL_GET_RESEND_BUFFERSIZE,
    // Set/get resend interval in ms (int32_t)
    // ---
    // This is the interval between individual resend
    // attempts for a specific frame.
    // Since there is always a certain roundtrip delay
    // between source and sink, it makes sense to wait
    // between resend attempts to not spam the network
    // with redundant /resend messages.
    AOO_CTL_SET_RESEND_INTERVAL,
    AOO_CTL_GET_RESEND_INTERVAL,
    // Set/get max. number of frames to resend (int32_t)
    // ---
    // This is the max. number of frames to request
    // in a single call to sink_handle_message().
    AOO_CTL_SET_RESEND_LIMIT,
    AOO_CTL_GET_RESEND_LIMIT,
    // Set/get redundancy (int32_t)
    // ---
    // The number of times each frames is sent (default = 1)
    AOO_CTL_SET_REDUNDANCY,
    AOO_CTL_GET_REDUNDANCY,
    // Set/get source timeout in ms (int32_t)
    // ---
    // Time to wait before removing inactive source.
    AOO_CTL_SET_SOURCE_TIMEOUT,
    AOO_CTL_GET_SOURCE_TIMEOUT,
    // Set/get buffer fill ratio (float)
    // ---
    // This is a read-only option for aoo_sink_ctl,
    // giving a ratio of how full the buffer is;
    // 0.0 is empty and 1.0 is full
    AOO_CTL_SET_BUFFER_FILL_RATIO,
    AOO_CTL_GET_BUFFER_FILL_RATIO,
    // Enable/disable binary data message
    // ---
    // Use a more compact (and faster) binary format
    // for the audio data message
    AOO_CTL_SET_BINARY_DATA_MSG,
    AOO_CTL_GET_BINARY_DATA_MSG
} aoo_option;

#define AOO_ARG(x) ((void *)&x), sizeof(x)

/*//////////////////// AoO source /////////////////////*/

typedef struct aoo_format
{
    const char *codec;
    int32_t size;
    int32_t nchannels;
    int32_t samplerate;
    int32_t blocksize;
} aoo_format;

typedef struct aoo_format_storage
{
    aoo_format header;
    char data[256];
} aoo_format_storage;

enum aoo_endpoint_flags
{
    AOO_ENDPOINT_RELAY = 1
};

// create a new AoO source instance
AOO_API aoo_source * aoo_source_new(aoo_id id, uint32_t flags);

// destroy the AoO source instance
AOO_API void aoo_source_free(aoo_source *src);

// setup the source - needs to be synchronized with other method calls!
AOO_API aoo_error aoo_source_setup(aoo_source *src, int32_t samplerate,
                                   int32_t blocksize, int32_t nchannels);

// handle messages from sinks (threadsafe, called from a network thread)
AOO_API aoo_error aoo_source_handle_message(aoo_source *src, const char *data, int32_t nbytes,
                                            const void *address, int32_t addrlen);

// update and send outgoing messages (threadsafe, called from the network thread)
AOO_API aoo_error aoo_source_send(aoo_source *src, aoo_sendfn fn, void *user);

// process audio blocks (threadsafe, called from the audio thread)
// data:        array of channel data (non-interleaved)
// nsamples:    number of samples per channel
// t:           current NTP timestamp (see aoo_osctime_get)
AOO_API aoo_error aoo_source_process(aoo_source *src, const aoo_sample **data,
                                     int32_t nsamples, uint64_t t);

// set event handler callback + mode
AOO_API aoo_error aoo_source_set_eventhandler(aoo_source *src, aoo_eventhandler fn,
                                              void *user, int32_t mode);

// check for pending events (always thread safe)
AOO_API aoo_bool aoo_source_events_available(aoo_source *src);

// poll events (threadsafe, but not reentrant)
// will call the event handler function one or more times
AOO_API aoo_error aoo_source_poll_events(aoo_source *src);

// source control (always threadsafe)
AOO_API aoo_error aoo_source_ctl(aoo_source *src, int32_t ctl,
                                 intptr_t index, void *p, size_t size);

// ------------------------------------------------------------
// type-safe convenience functions for frequently used controls

static inline aoo_error aoo_source_start(aoo_source *src) {
    return aoo_source_ctl(src, 0, AOO_CTL_START, 0, 0);
}

static inline aoo_error aoo_source_stop(aoo_source *src) {
    return aoo_source_ctl(src, 0, AOO_CTL_STOP, 0, 0);
}

static inline aoo_error aoo_source_add_sink(aoo_source *src,
                                            const aoo_endpoint *ep, uint32_t flags){
    return aoo_source_ctl(src, AOO_CTL_ADD_SINK, (intptr_t)ep, AOO_ARG(flags));
}

static inline aoo_error aoo_source_remove_sink(aoo_source *src,
                                               const aoo_endpoint *ep){
    return aoo_source_ctl(src, AOO_CTL_REMOVE_SINK, (intptr_t)ep, 0, 0);
}

static inline aoo_error aoo_source_remove_all(aoo_source *src){
    return aoo_source_ctl(src, AOO_CTL_REMOVE_SINK, 0, 0, 0);
}

static inline aoo_error aoo_source_set_id(aoo_source *src, aoo_id id) {
    return aoo_source_ctl(src, 0, AOO_CTL_SET_ID, AOO_ARG(id));
}

static inline aoo_error aoo_source_get_id(aoo_source *src, aoo_id *id) {
    return aoo_source_ctl(src, 0, AOO_CTL_GET_ID, AOO_ARG(*id));
}

static inline aoo_error aoo_source_set_format(aoo_source *src, aoo_format *f) {
    return aoo_source_ctl(src, 0, AOO_CTL_SET_FORMAT, AOO_ARG(*f));
}

static inline aoo_error aoo_source_get_format(aoo_source *src, aoo_format_storage *f) {
    return aoo_source_ctl(src, 0, AOO_CTL_GET_FORMAT, AOO_ARG(*f));
}

static inline aoo_error aoo_source_set_buffersize(aoo_source *src, int32_t n) {
    return aoo_source_ctl(src, 0, AOO_CTL_SET_BUFFERSIZE, AOO_ARG(n));
}

static inline aoo_error aoo_source_get_buffersize(aoo_source *src, int32_t *n) {
    return aoo_source_ctl(src, 0, AOO_CTL_GET_BUFFERSIZE, AOO_ARG(*n));
}

static inline aoo_error aoo_source_set_timer_check(aoo_source *src, aoo_bool b) {
    return aoo_source_ctl(src, 0, AOO_CTL_SET_TIMER_CHECK, AOO_ARG(b));
}

static inline aoo_error aoo_source_get_timer_check(aoo_source *src, aoo_bool *b) {
    return aoo_source_ctl(src, 0, AOO_CTL_GET_TIMER_CHECK, AOO_ARG(*b));
}

static inline aoo_error aoo_source_set_dynamic_resampling(aoo_source *src, aoo_bool b) {
    return aoo_source_ctl(src, 0, AOO_CTL_SET_DYNAMIC_RESAMPLING, AOO_ARG(b));
}

static inline aoo_error aoo_source_get_dynamic_resampling(aoo_source *src, aoo_bool *b) {
    return aoo_source_ctl(src, 0, AOO_CTL_GET_DYNAMIC_RESAMPLING, AOO_ARG(*b));
}

static inline aoo_error aoo_source_get_real_samplerate(aoo_source *src, double *sr) {
    return aoo_source_ctl(src, 0, AOO_CTL_GET_REAL_SAMPLERATE, AOO_ARG(*sr));
}

static inline aoo_error aoo_source_set_dll_bandwidth(aoo_source *src, float n) {
    return aoo_source_ctl(src, 0, AOO_CTL_SET_DLL_BANDWIDTH, AOO_ARG(n));
}

static inline aoo_error aoo_source_get_dll_bandwidth(aoo_source *src, float *n) {
    return aoo_source_ctl(src, 0, AOO_CTL_GET_DLL_BANDWIDTH, AOO_ARG(*n));
}

static inline aoo_error aoo_source_set_packetsize(aoo_source *src, int32_t n) {
    return aoo_source_ctl(src, 0, AOO_CTL_SET_PACKETSIZE, AOO_ARG(n));
}

static inline aoo_error aoo_source_get_packetsize(aoo_source *src, int32_t *n) {
    return aoo_source_ctl(src, 0, AOO_CTL_GET_PACKETSIZE, AOO_ARG(*n));
}

static inline aoo_error aoo_source_set_ping_interval(aoo_source *src, int32_t n) {
    return aoo_source_ctl(src, 0, AOO_CTL_SET_PING_INTERVAL, AOO_ARG(n));
}

static inline aoo_error aoo_source_get_ping_interval(aoo_source *src, int32_t *n) {
    return aoo_source_ctl(src, 0, AOO_CTL_GET_PING_INTERVAL, AOO_ARG(*n));
}

static inline aoo_error aoo_source_set_resend_buffersize(aoo_source *src, int32_t n) {
    return aoo_source_ctl(src, 0, AOO_CTL_SET_RESEND_BUFFERSIZE, AOO_ARG(n));
}

static inline aoo_error aoo_source_get_resend_buffersize(aoo_source *src, int32_t *n) {
    return aoo_source_ctl(src, 0, AOO_CTL_GET_RESEND_BUFFERSIZE, AOO_ARG(*n));
}

static inline aoo_error aoo_source_set_redundancy(aoo_source *src, int32_t n) {
    return aoo_source_ctl(src, 0, AOO_CTL_SET_REDUNDANCY, AOO_ARG(n));
}

static inline aoo_error aoo_source_get_redundancy(aoo_source *src, int32_t *n) {
    return aoo_source_ctl(src, 0, AOO_CTL_GET_REDUNDANCY, AOO_ARG(*n));
}

static inline aoo_error aoo_source_set_binary_data_msg(aoo_source *src, aoo_bool b) {
    return aoo_source_ctl(src, 0, AOO_CTL_SET_BINARY_DATA_MSG, AOO_ARG(b));
}

static inline aoo_error aoo_source_get_binary_data_msg(aoo_source *src, aoo_bool *b) {
    return aoo_source_ctl(src, 0, AOO_CTL_GET_BINARY_DATA_MSG, AOO_ARG(*b));
}

static inline aoo_error aoo_source_set_sink_channel_onset(aoo_source *src, const aoo_endpoint *ep, int32_t onset) {
    return aoo_source_ctl(src, (intptr_t)ep, AOO_CTL_SET_CHANNEL_ONSET, AOO_ARG(onset));
}

static inline aoo_error aoo_source_get_sink_channel_onset(aoo_source *src, const aoo_endpoint *ep, int32_t *onset) {
    return aoo_source_ctl(src, (intptr_t)ep, AOO_CTL_GET_CHANNEL_ONSET, AOO_ARG(*onset));
}

/*//////////////////// AoO sink /////////////////////*/

// create a new AoO sink instance
AOO_API aoo_sink * aoo_sink_new(aoo_id id, uint32_t flags);

// destroy the AoO sink instance
AOO_API void aoo_sink_free(aoo_sink *sink);

// setup the sink - needs to be synchronized with other method calls!
AOO_API aoo_error aoo_sink_setup(aoo_sink *sink, int32_t samplerate,
                                 int32_t blocksize, int32_t nchannels);

// handle messages from sources (threadsafe, called from a network thread)
AOO_API aoo_error aoo_sink_handle_message(aoo_sink *sink, const char *data, int32_t nbytes,
                                          const void *address, int32_t addrlen);

// send outgoing messages (threadsafe, called from a network thread)
AOO_API aoo_error aoo_sink_send(aoo_sink *sink, aoo_sendfn fn, void *user);

// process audio (threadsafe, but not reentrant)
// data:        array of channel data (non-interleaved)
// nsamples:    number of samples per channel
// t:           current NTP timestamp (see aoo_osctime_get)
AOO_API aoo_error aoo_sink_process(aoo_sink *sink, aoo_sample **data,
                                   int32_t nsamples, uint64_t t);

// set event handler callback + aoo_event_mode
AOO_API aoo_error aoo_sink_set_eventhandler(aoo_sink *sink, aoo_eventhandler fn,
                                            void *user, int32_t mode);

// check for pending events (always thread safe)
AOO_API aoo_bool aoo_sink_events_available(aoo_sink *sink);

// poll events (threadsafe, but not reentrant)
// will call the event handler function one or more times
AOO_API aoo_error aoo_sink_poll_events(aoo_sink *sink);

// sink control (always threadsafe)
AOO_API aoo_error aoo_sink_ctl(aoo_sink *sink, int32_t ctl,
                               intptr_t index, void *p, size_t size);

// ------------------------------------------------------------
// type-safe convenience functions for frequently used controls

static inline aoo_error aoo_sink_invite_source(aoo_sink *sink,
                                               const aoo_endpoint *ep)
{
    return aoo_sink_ctl(sink, AOO_CTL_INVITE_SOURCE, (intptr_t)ep, 0, 0);
}

static inline aoo_error aoo_sink_uninvite_source(aoo_sink *sink,
                                                 const aoo_endpoint *ep)
{
    return aoo_sink_ctl(sink, AOO_CTL_UNINVITE_SOURCE, (intptr_t)ep, 0, 0);
}

static inline aoo_error aoo_sink_uninvite_all(aoo_sink *sink){
    return aoo_sink_ctl(sink, AOO_CTL_UNINVITE_SOURCE, 0, 0, 0);
}

static inline aoo_error aoo_sink_set_id(aoo_sink *sink, aoo_id id) {
    return aoo_sink_ctl(sink, 0, AOO_CTL_SET_ID, AOO_ARG(id));
}

static inline aoo_error aoo_sink_get_id(aoo_sink *sink, aoo_id *id) {
    return aoo_sink_ctl(sink, 0, AOO_CTL_GET_ID, AOO_ARG(*id));
}

static inline aoo_error aoo_sink_reset(aoo_sink *sink) {
    return aoo_sink_ctl(sink, 0, AOO_CTL_RESET, 0, 0);
}

static inline aoo_error aoo_sink_set_buffersize(aoo_sink *sink, int32_t n) {
    return aoo_sink_ctl(sink, 0, AOO_CTL_SET_BUFFERSIZE, AOO_ARG(n));
}

static inline aoo_error aoo_sink_get_buffersize(aoo_sink *sink, int32_t *n) {
    return aoo_sink_ctl(sink, 0, AOO_CTL_GET_BUFFERSIZE, AOO_ARG(*n));
}

static inline aoo_error aoo_sink_set_timer_check(aoo_sink *sink, aoo_bool b) {
    return aoo_sink_ctl(sink, 0, AOO_CTL_SET_TIMER_CHECK, AOO_ARG(b));
}

static inline aoo_error aoo_sink_get_timer_check(aoo_sink *sink, aoo_bool *b) {
    return aoo_sink_ctl(sink, 0, AOO_CTL_GET_TIMER_CHECK, AOO_ARG(*b));
}

static inline aoo_error aoo_sink_set_dynamic_resampling(aoo_sink *sink, aoo_bool b) {
    return aoo_sink_ctl(sink, 0, AOO_CTL_SET_DYNAMIC_RESAMPLING, AOO_ARG(b));
}

static inline aoo_error aoo_sink_get_dynamic_resampling(aoo_sink *sink, aoo_bool *b) {
    return aoo_sink_ctl(sink, 0, AOO_CTL_GET_DYNAMIC_RESAMPLING, AOO_ARG(*b));
}

static inline aoo_error aoo_sink_get_real_samplerate(aoo_sink *sink, double *sr) {
    return aoo_sink_ctl(sink, 0, AOO_CTL_GET_REAL_SAMPLERATE, AOO_ARG(*sr));
}

static inline aoo_error aoo_sink_set_dll_bandwith(aoo_sink *sink, float n) {
    return aoo_sink_ctl(sink, 0, AOO_CTL_SET_DLL_BANDWIDTH, AOO_ARG(n));
}

static inline aoo_error aoo_sink_get_dll_bandwidth(aoo_sink *sink, float *n) {
    return aoo_sink_ctl(sink, 0, AOO_CTL_GET_DLL_BANDWIDTH, AOO_ARG(*n));
}

static inline aoo_error aoo_sink_set_packetsize(aoo_sink *sink, int32_t n) {
    return aoo_sink_ctl(sink, 0, AOO_CTL_SET_PACKETSIZE, AOO_ARG(n));
}

static inline aoo_error aoo_sink_get_packetsize(aoo_sink *sink, int32_t *n) {
    return aoo_sink_ctl(sink, 0, AOO_CTL_GET_PACKETSIZE, AOO_ARG(*n));
}

static inline aoo_error aoo_sink_set_resend_data(aoo_sink *sink, aoo_bool b) {
    return aoo_sink_ctl(sink, 0, AOO_CTL_SET_RESEND_DATA, AOO_ARG(b));
}

static inline aoo_error aoo_sink_get_resend_data(aoo_sink *sink, aoo_bool *b) {
    return aoo_sink_ctl(sink, 0, AOO_CTL_GET_RESEND_DATA, AOO_ARG(*b));
}

static inline aoo_error aoo_sink_set_resend_interval(aoo_sink *sink, int32_t n) {
    return aoo_sink_ctl(sink, 0, AOO_CTL_SET_RESEND_INTERVAL, AOO_ARG(n));
}

static inline aoo_error aoo_sink_get_resend_interval(aoo_sink *sink, int32_t *n) {
    return aoo_sink_ctl(sink, 0, AOO_CTL_GET_RESEND_INTERVAL, AOO_ARG(*n));
}

static inline aoo_error aoo_sink_set_resend_limit(aoo_sink *sink, int32_t n) {
    return aoo_sink_ctl(sink, 0, AOO_CTL_SET_RESEND_LIMIT, AOO_ARG(n));
}

static inline aoo_error aoo_sink_get_resend_limit(aoo_sink *sink, int32_t *n) {
    return aoo_sink_ctl(sink, 0, AOO_CTL_GET_RESEND_LIMIT, AOO_ARG(*n));
}

static inline aoo_error aoo_sink_set_source_timeout(aoo_sink *sink, int32_t n) {
    return aoo_sink_ctl(sink, 0, AOO_CTL_SET_SOURCE_TIMEOUT, AOO_ARG(n));
}

static inline aoo_error aoo_sink_get_source_timeout(aoo_sink *sink, int32_t *n) {
    return aoo_sink_ctl(sink, 0, AOO_CTL_GET_SOURCE_TIMEOUT, AOO_ARG(*n));
}

static inline aoo_error aoo_sink_reset_source(aoo_sink *sink, const aoo_endpoint *ep) {
    return aoo_sink_ctl(sink, (intptr_t)ep, AOO_CTL_RESET, 0, 0);
}

static inline aoo_error aoo_sink_request_source_format(aoo_sink *sink, const aoo_endpoint *ep,
                                                       const aoo_format *f) {
    return aoo_sink_ctl(sink, (intptr_t)ep, AOO_CTL_REQUEST_FORMAT, AOO_ARG(*f));
}

static inline aoo_error aoo_sink_get_source_format(aoo_sink *sink, const aoo_endpoint *ep,
                                                   aoo_format_storage *f) {
    return aoo_sink_ctl(sink, (intptr_t)ep, AOO_CTL_GET_FORMAT, AOO_ARG(*f));
}

static inline aoo_error aoo_sink_get_buffer_fill_ratio(aoo_sink *sink, const aoo_endpoint *ep,
                                                       float *n) {
    return aoo_sink_ctl(sink, (intptr_t)ep, AOO_CTL_GET_BUFFER_FILL_RATIO, AOO_ARG(*n));
}

/*//////////////////// Codec API //////////////////////////*/

#define AOO_CODEC_MAXSETTINGSIZE 256

typedef void* (*aoo_codec_new)(void);

typedef void (*aoo_codec_free)(void *);

// codec control
typedef aoo_error (*aoo_codec_ctlfn)
(
        void *, // the encoder/decoder instance
        int32_t, // the ctl number
        void *, // pointer to value
        int32_t // the value size
);

// encode samples to bytes
typedef aoo_error (*aoo_codec_encode)(
        void *,             // the encoder instance
        const aoo_sample *, // input samples (interleaved)
        int32_t,            // number of samples
        char *,             // output buffer
        int32_t *           // max. buffer size (updated to actual size)
);

// decode bytes to samples
typedef aoo_error (*aoo_codec_decode)(
        void *,         // the decoder instance
        const char *,   // input bytes
        int32_t,        // input size
        aoo_sample *,   // output samples (interleaved)
        int32_t *       // max. number of samples (updated to actual number)

);

// serialize format options (everything after the 'aoo_format' header)
typedef aoo_error (*aoo_codec_serialize)(
        const aoo_format *, // source format
        char *,             // option buffer
        int32_t *           // buffer size (updated to actual size)
);

// deserialize format options (everything after the 'aoo_format' header).
typedef aoo_error (*aoo_codec_deserialize)(
        const aoo_format *, // format header
        const char *,       // option buffer
        int32_t,            // buffer size
        aoo_format *,       // format buffer large enough to hold the codec format.
        int32_t             // size of the format buffer
);

// NOTE: generic codec controls are negative, so you can also pass
// codec specific controls (which are assumed to be positiv, e.g. OPUS_SET_BITRATE)
typedef enum aoo_codec_ctl
{
    // reset the codec state (NULL)
    AOO_CODEC_RESET = -1000,
    // set the codec format (aoo_format)
    // ---
    // Set the format by passing a pointer to the format header.
    // The format struct is validated and updated on success!
    AOO_CODEC_SET_FORMAT,
    // set the codec format (aoo_format)
    // ---
    // Get the format by passing a pointer to an instance of
    // 'aoo_format_storage' or a similar struct that is large enough
    // to hold any format. On success, the actual format size will be
    // contained in the 'size' member of the format header.
    AOO_CODEC_GET_FORMAT,
    // check if the format is equal (aoo_format)
    // ---
    // returns AOO_TRUE or AOO_FALSE
    AOO_CODEC_FORMAT_EQUAL
} aoo_codec_ctl;

typedef struct aoo_codec
{
    const char *name;
    // encoder
    aoo_codec_new encoder_new;
    aoo_codec_free encoder_free;
    aoo_codec_ctlfn encoder_ctl;
    aoo_codec_encode encoder_encode;
    // decoder
    aoo_codec_new decoder_new;
    aoo_codec_free decoder_free;
    aoo_codec_ctlfn decoder_ctl;
    aoo_codec_decode decoder_decode;
    // helpers
    aoo_codec_serialize serialize;
    aoo_codec_deserialize deserialize;
    void *reserved[18];
} aoo_codec;

// register an external codec plugin
AOO_API aoo_error aoo_register_codec(const char *name, const aoo_codec *codec);

// The type of 'aoo_register_codec', which gets passed to codec plugins
// to register themselves.
typedef aoo_error (*aoo_codec_registerfn)(const char *, const aoo_codec *);

// NOTE: AOO doesn't support dynamic plugin loading out of the box,
// but it is quite easy to implement on your own:
// Usually, you would put one or more codecs in a shared library
// and export a single function like the following:
//
// void aoo_setup(aoo_codec_registerfn fn, const aoo_allocator *alloc);
//
// In your host application, you would then scan directories for shared libraries,
// check if they export a function named 'aoo_setup', and if yes, called it with
// 'aoo_register_codec' and an (optional) 'aoo_allocator' instance.

#ifdef __cplusplus
} // extern "C"
#endif
