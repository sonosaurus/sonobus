/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef AOO_API
# ifndef AOO_STATIC
#  if defined(_WIN32) // Windows
#   if defined(AOO_BUILD)
#      if defined(DLL_EXPORT)
#        define AOO_API __declspec(dllexport)
#      else
#        define AOO_API
#      endif
#   else
#    define AOO_API __declspec(dllimport)
#   endif
#  elif defined(__GNUC__) && defined(AOO_BUILD) // GNU C
#   define AOO_API __attribute__ ((visibility ("default")))
#  else // Other
#   define AOO_API
#  endif
# else // AOO_STATIC
#  define AOO_API
# endif
#endif

#ifndef AOO_STRICT
#define AOO_STRICT 0
#endif

#define AOO_VERSION_MAJOR 2
#define AOO_VERSION_MINOR 0
#define AOO_VERSION_PATCH 0
#define AOO_VERSION_PRERELEASE 3 // 0: no pre-release

typedef int32_t aoo_id;
#define AOO_ID_NONE INT32_MIN

// default UDP packet size
#ifndef AOO_PACKETSIZE
 #define AOO_PACKETSIZE 512
#endif

// max. UDP packet size
#define AOO_MAXPACKETSIZE 4096 // ?

#ifndef AOO_SAMPLETYPE
#define AOO_SAMPLETYPE float
#endif

typedef AOO_SAMPLETYPE aoo_sample;

#ifndef USE_AOO_NET
#define USE_AOO_NET 1
#endif

typedef int32_t aoo_type;

enum aoo_types
{
    AOO_TYPE_SOURCE = 0,
    AOO_TYPE_SINK,
#if USE_AOO_NET
    AOO_TYPE_SERVER = 1000,
    AOO_TYPE_CLIENT,
    AOO_TYPE_PEER,
    AOO_TYPE_RELAY
#endif
};

typedef int32_t aoo_bool;

#define AOO_TRUE 1
#define AOO_FALSE 0

typedef int32_t aoo_error;

enum aoo_error_codes
{
    AOO_OK = 0,
    AOO_ERROR_UNSPECIFIED,
    AOO_ERROR_IDLE,
    AOO_ERROR_WOULDBLOCK
};

// OSC messages:
#define AOO_MSG_DOMAIN "/aoo"
#define AOO_MSG_DOMAIN_LEN 4

// binary message: domain (int32), aoo_type (int16), msg_type (int16), aoo_id(int32)
#define AOO_BIN_MSG_HEADER_SIZE 12

#define AOO_BIN_MSG_DOMAIN "\0aoo"
#define AOO_BIN_MSG_DOMAIN_SIZE 4

#define AOO_BIN_MSG_CMD_DATA 0

enum aoo_bin_msg_data_flags
{
    AOO_BIN_MSG_DATA_SAMPLERATE = 1 << 0,
    AOO_BIN_MSG_DATA_FRAMES = 1 << 1
};

#ifdef __cplusplus
namespace aoo {
    class source;
    class sink;
}
using aoo_source = aoo::source;
using aoo_sink = aoo::sink;
#else
typedef struct aoo_source aoo_source;
typedef struct aoo_sink aoo_sink;
#endif

typedef struct aoo_allocator
{
    // args: size, context
    void* (*alloc)(size_t, void *);
    // args: ptr, old size, new size, context
    void* (*realloc)(void *, size_t, size_t, void *);
    // args: ptr, size, context
    void (*free)(void *, size_t, void *);
    // context passed to functions
    void* context;
} aoo_allocator;

// logging
#define AOO_LOGLEVEL_NONE 0
#define AOO_LOGLEVEL_ERROR 1
#define AOO_LOGLEVEL_WARNING 2
#define AOO_LOGLEVEL_VERBOSE 3
#define AOO_LOGLEVEL_DEBUG 4

typedef void (*aoo_logfunction)(const char *msg, int32_t level, void *context);

// endpoint
typedef struct aoo_endpoint
{
    const void *address;
    int32_t addrlen;
    aoo_id id;
} aoo_endpoint;

// reply function
typedef int32_t (*aoo_sendfn)(
        void *,         // user
        const char *,   // data
        int32_t,        // number of bytes
        const void *,   // address
        int32_t,        // address size
        uint32_t        // flags
);

// base event
typedef struct aoo_event
{
    int32_t type;
} aoo_event;

enum aoo_thread_level
{
    AOO_THREAD_UNKNOWN = 0,
    AOO_THREAD_AUDIO,
    AOO_THREAD_NETWORK
};

enum aoo_event_mode
{
    AOO_EVENT_NONE = 0,
    AOO_EVENT_CALLBACK,
    AOO_EVENT_POLL
};

// event handler
typedef void (*aoo_eventhandler)(
        void *user,          // user
        const aoo_event *e,  // event
        int32_t level        // aoo_thread_level
);

#ifdef __cplusplus
} // extern "C"
#endif
