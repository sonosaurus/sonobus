/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#pragma once

#include "aoo.h"

#include <memory>

namespace aoo {

// NOTE: aoo::server and aoo::client don't have public virtual
// destructors and have to be freed with the destroy() method.
// We provide a custom deleter and shared pointer to simplify this task.
//
// By following certain COM conventions (no virtual destructors,
// no method overloading, only POD parameters and return types),
// we get ABI independent C++ interfaces on Windows and all other
// platforms where the vtable layout for such classes is stable
// (generally true for Linux and macOS, in my experience).
//
// In practice this means you only have to build 'aoo' once as a
// shared library and can then use its C++ interface in applications
// built with different compilers resp. compiler versions.
//
// If you want to be on the safe safe, use the C interface :-)

/*//////////////////////// AoO source ///////////////////////*/

class source {
public:
    class deleter {
    public:
        void operator()(source *x){
            destroy(x);
        }
    };
    // smart pointer for AoO source instance
    using pointer = std::unique_ptr<source, deleter>;

    // create a new AoO source instance
    static source * create(aoo_id id, uint32_t flags);

    // destroy the AoO source instance
    static void destroy(source *src);

    // setup the source - needs to be synchronized with other method calls!
    virtual aoo_error setup(int32_t samplerate, int32_t blocksize, int32_t nchannels) = 0;

    // handle messages from sinks (threadsafe, called from a network thread)
    virtual aoo_error handle_message(const char *data, int32_t n,
                                     const void *address, int32_t addrlen) = 0;

    // send outgoing messages (threadsafe, called from a network thread)
    virtual aoo_error send(aoo_sendfn fn, void *user) = 0;

    // process audio blocks (threadsafe, called from the audio thread)
    // data:        array of channel data (non-interleaved)
    // nsamples:    number of samples per channel
    // t:           current NTP timestamp (see aoo_osctime_get)
    virtual aoo_error process(const aoo_sample **data,
                            int32_t nsamples, uint64_t t) = 0;

    // set event handler callback + mode
    virtual aoo_error set_eventhandler(aoo_eventhandler fn,
                                       void *user, int32_t mode) = 0;

    // check for pending events (always thread safe)
    virtual aoo_bool events_available() = 0;

    // poll events (threadsafe, but not reentrant)
    // will call the event handler function one or more times
    virtual aoo_error poll_events() = 0;

    virtual aoo_error control(int32_t ctl, intptr_t index, void *ptr, size_t size) = 0;

    // ----------------------------------------------------------
    // type-safe convenience methods for frequently used controls

    aoo_error start(){
        return control(AOO_CTL_START, 0, nullptr, 0);
    }

    aoo_error stop(){
        return control(AOO_CTL_STOP, 0, nullptr, 0);
    }

    aoo_error add_sink(const aoo_endpoint& ep, uint32_t flags = 0){
        return control(AOO_CTL_ADD_SINK, (intptr_t)&ep, AOO_ARG(flags));
    }

    aoo_error remove_sink(const aoo_endpoint& ep){
        return control(AOO_CTL_REMOVE_SINK, (intptr_t)&ep, nullptr, 0);
    }

    aoo_error remove_all(){
        return control(AOO_CTL_REMOVE_SINK, 0, nullptr, 0);
    }

    aoo_error set_format(aoo_format& f){
        return control(AOO_CTL_SET_FORMAT, 0, AOO_ARG(f));
    }

    aoo_error get_format(aoo_format_storage& f){
        return control(AOO_CTL_GET_FORMAT, 0, AOO_ARG(f));
    }

    aoo_error set_id(aoo_id id){
        return control(AOO_CTL_SET_ID, 0, AOO_ARG(id));
    }

    aoo_error get_id(aoo_id &id){
        return control(AOO_CTL_GET_ID, 0, AOO_ARG(id));
    }

    aoo_error set_sink_channel_onset(const aoo_endpoint& ep, int32_t onset){
        return control(AOO_CTL_SET_CHANNEL_ONSET, (intptr_t)&ep, AOO_ARG(onset));
    }

    aoo_error get_sink_channel_onset(const aoo_endpoint& ep, int32_t& onset){
        return control(AOO_CTL_GET_CHANNEL_ONSET, (intptr_t)&ep, AOO_ARG(onset));
    }

    aoo_error set_buffersize(int32_t n){
        return control(AOO_CTL_SET_BUFFERSIZE, 0, AOO_ARG(n));
    }

    aoo_error get_buffersize(int32_t& n){
        return control(AOO_CTL_GET_BUFFERSIZE, 0, AOO_ARG(n));
    }

    aoo_error set_timer_check(aoo_bool b){
        return control(AOO_CTL_SET_TIMER_CHECK, 0, AOO_ARG(b));
    }

    aoo_error get_timer_check(aoo_bool b){
        return control(AOO_CTL_GET_TIMER_CHECK, 0, AOO_ARG(b));
    }

    aoo_error set_dynamic_resampling(aoo_bool b){
        return control(AOO_CTL_SET_DYNAMIC_RESAMPLING, 0, AOO_ARG(b));
    }

    aoo_error get_dynamic_resampling(aoo_bool b){
        return control(AOO_CTL_GET_DYNAMIC_RESAMPLING, 0, AOO_ARG(b));
    }

    aoo_error get_real_samplerate(double& sr){
        return control(AOO_CTL_GET_REAL_SAMPLERATE, 0, AOO_ARG(sr));
    }

    aoo_error set_dll_bandwidth(float f){
        return control(AOO_CTL_SET_DLL_BANDWIDTH, 0, AOO_ARG(f));
    }

    aoo_error get_dll_bandwidth(float& f){
        return control(AOO_CTL_GET_DLL_BANDWIDTH, 0, AOO_ARG(f));
    }

    aoo_error set_packetsize(int32_t n){
        return control(AOO_CTL_SET_PACKETSIZE, 0, AOO_ARG(n));
    }

    aoo_error get_packetsize(int32_t& n){
        return control(AOO_CTL_GET_PACKETSIZE, 0, AOO_ARG(n));
    }

    aoo_error set_resend_buffersize(int32_t n){
        return control(AOO_CTL_SET_RESEND_BUFFERSIZE, 0, AOO_ARG(n));
    }

    aoo_error get_resend_buffersize(int32_t& n){
        return control(AOO_CTL_GET_RESEND_BUFFERSIZE, 0, AOO_ARG(n));
    }

    aoo_error set_redundancy(int32_t n){
        return control(AOO_CTL_SET_REDUNDANCY, 0, AOO_ARG(n));
    }

    aoo_error get_redundancy(int32_t& n){
        return control(AOO_CTL_GET_REDUNDANCY, 0, AOO_ARG(n));
    }

    aoo_error set_ping_interval(int32_t ms){
        return control(AOO_CTL_SET_PING_INTERVAL, 0, AOO_ARG(ms));
    }

    aoo_error get_ping_interval(int32_t& ms){
        return control(AOO_CTL_GET_PING_INTERVAL, 0, AOO_ARG(ms));
    }

    aoo_error set_binary_data_msg(aoo_bool b) {
        return control(AOO_CTL_SET_BINARY_DATA_MSG, 0, AOO_ARG(b));
    }

    aoo_error get_binary_data_msg(aoo_bool& b) {
        return control(AOO_CTL_GET_BINARY_DATA_MSG, 0, AOO_ARG(b));
    }
protected:
    ~source(){} // non-virtual!
};

inline source * source::create(aoo_id id, uint32_t flags){
    return aoo_source_new(id, flags);
}

inline void source::destroy(source *src){
    aoo_source_free(src);
}

/*//////////////////////// AoO sink ///////////////////////*/

class sink {
public:
    class deleter {
    public:
        void operator()(sink *x){
            destroy(x);
        }
    };
    // smart pointer for AoO sink instance
    using pointer = std::unique_ptr<sink, deleter>;

    // create a new AoO sink instance
    static sink * create(aoo_id id, uint32_t flags);

    // destroy the AoO sink instance
    static void destroy(sink *sink);

    // setup the sink - needs to be synchronized with other method calls!
    virtual aoo_error setup(int32_t samplerate, int32_t blocksize, int32_t nchannels) = 0;

    // handle messages from sources (threadsafe, called from a network thread)
    virtual aoo_error handle_message(const char *data, int32_t n,
                                     const void *address, int32_t addrlen) = 0;

    // send outgoing messages (threadsafe, called from a network thread)
    virtual aoo_error send(aoo_sendfn fn, void *user) = 0;

    // process audio (threadsafe, but not reentrant)
    virtual aoo_error process(aoo_sample **data, int32_t nsamples, uint64_t t) = 0;

    // set event handler callback + mode
    virtual aoo_error set_eventhandler(aoo_eventhandler fn,
                                       void *user, int32_t mode) = 0;

    // check for pending events (always thread safe)
    virtual aoo_bool events_available() = 0;

    // poll events (threadsafe, but not reentrant)
    // will call the event handler function one or more times
    virtual aoo_error poll_events() = 0;

    virtual aoo_error control(int32_t ctl, intptr_t index, void *ptr, size_t size) = 0;

    // ----------------------------------------------------------
    // type-safe convenience methods for frequently used controls

    aoo_error invite_source(const aoo_endpoint& ep, uint32_t flags=0) {
        return control(AOO_CTL_INVITE_SOURCE, (intptr_t)&ep, &flags, sizeof(uint32_t));
    }

    aoo_error uninvite_source(const aoo_endpoint& ep){
        return control(AOO_CTL_UNINVITE_SOURCE, (intptr_t)&ep, nullptr, 0);
    }

    aoo_error uninvite_all(){
        return control(AOO_CTL_UNINVITE_SOURCE, 0, nullptr, 0);
    }

    aoo_error reset(){
        return control(AOO_CTL_RESET, 0, nullptr, 0);
    }

    aoo_error reset_source(const aoo_endpoint& ep) {
        return control(AOO_CTL_RESET, (intptr_t)&ep, nullptr, 0);
    }

    aoo_error request_source_format(const aoo_endpoint& ep, const aoo_format& f) {
        return control(AOO_CTL_REQUEST_FORMAT, (intptr_t)&ep, AOO_ARG(f));
    }

    aoo_error get_source_format(const aoo_endpoint& ep, aoo_format_storage& f) {
        return control(AOO_CTL_GET_FORMAT, (intptr_t)&ep, AOO_ARG(f));
    }

    aoo_error get_buffer_fill_ratio(const aoo_endpoint& ep, float& ratio){
        return control(AOO_CTL_GET_BUFFER_FILL_RATIO, (intptr_t)&ep, AOO_ARG(ratio));
    }

    aoo_error set_id(aoo_id id){
        return control(AOO_CTL_SET_ID, 0, AOO_ARG(id));
    }

    aoo_error get_id(aoo_id &id){
        return control(AOO_CTL_GET_ID, 0, AOO_ARG(id));
    }

    aoo_error set_buffersize(int32_t n){
        return control(AOO_CTL_SET_BUFFERSIZE, 0, AOO_ARG(n));
    }

    aoo_error get_buffersize(int32_t& n){
        return control(AOO_CTL_GET_BUFFERSIZE, 0, AOO_ARG(n));
    }

    aoo_error set_timer_check(aoo_bool b){
        return control(AOO_CTL_SET_TIMER_CHECK, 0, AOO_ARG(b));
    }

    aoo_error get_timer_check(aoo_bool b){
        return control(AOO_CTL_GET_TIMER_CHECK, 0, AOO_ARG(b));
    }

    aoo_error set_dynamic_resampling(aoo_bool b){
        return control(AOO_CTL_SET_DYNAMIC_RESAMPLING, 0, AOO_ARG(b));
    }

    aoo_error get_dynamic_resampling(aoo_bool b){
        return control(AOO_CTL_GET_DYNAMIC_RESAMPLING, 0, AOO_ARG(b));
    }

    aoo_error get_real_samplerate(double& sr){
        return control(AOO_CTL_GET_REAL_SAMPLERATE, 0, AOO_ARG(sr));
    }

    aoo_error set_dll_bandwidth(float f){
        return control(AOO_CTL_SET_DLL_BANDWIDTH, 0, AOO_ARG(f));
    }

    aoo_error get_dll_bandwidth(float& f){
        return control(AOO_CTL_GET_DLL_BANDWIDTH, 0, AOO_ARG(f));
    }

    aoo_error set_packetsize(int32_t n){
        return control(AOO_CTL_SET_PACKETSIZE, 0, AOO_ARG(n));
    }

    aoo_error get_packetsize(int32_t& n){
        return control(AOO_CTL_GET_PACKETSIZE, 0, AOO_ARG(n));
    }

    aoo_error set_resend_data(aoo_bool b){
        return control(AOO_CTL_SET_RESEND_DATA, 0, AOO_ARG(b));
    }

    aoo_error get_resend_data(aoo_bool& b){
        return control(AOO_CTL_GET_RESEND_DATA, 0, AOO_ARG(b));
    }

    aoo_error set_resend_interval(int32_t n){
        return control(AOO_CTL_SET_RESEND_INTERVAL, 0, AOO_ARG(n));
    }

    aoo_error get_resend_interval(int32_t& n){
        return control(AOO_CTL_GET_RESEND_INTERVAL, 0, AOO_ARG(n));
    }

    aoo_error set_resend_limit(int32_t n){
        return control(AOO_CTL_SET_RESEND_LIMIT, 0, AOO_ARG(n));
    }

    aoo_error get_resend_maxnumframes(int32_t& n){
        return control(AOO_CTL_GET_RESEND_LIMIT, 0, AOO_ARG(n));
    }

    aoo_error set_source_timeout(int32_t n){
        return control(AOO_CTL_SET_SOURCE_TIMEOUT, 0, AOO_ARG(n));
    }

    aoo_error get_source_timeout(int32_t& n){
        return control(AOO_CTL_GET_SOURCE_TIMEOUT, 0, AOO_ARG(n));
    }
protected:
    ~sink(){} // non-virtual!
};

inline sink * sink::create(aoo_id id, uint32_t flags){
    return aoo_sink_new(id, flags);
}

inline void sink::destroy(sink *sink){
    aoo_sink_free(sink);
}

} // aoo
