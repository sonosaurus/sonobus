/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#pragma once

#include <stdint.h>

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

#define AOO_MSG_DOMAIN "/aoo"
#define AOO_MSG_DOMAIN_LEN 4

#define AOO_MAXPACKETSIZE 4096 // ?

#ifndef AOO_SAMPLETYPE
#define AOO_SAMPLETYPE float
#endif

typedef AOO_SAMPLETYPE aoo_sample;

// send function
typedef int32_t (*aoo_sendfn)(
        void *,             // user
        const char *,       // data
        int32_t,            // numbytes
        void *              // addr
);

// reply function
typedef int32_t (*aoo_replyfn)(
        void *,         // user
        const char *,   // data
        int32_t         // number of bytes
);

// base event
typedef struct aoo_event
{
    int32_t type;
} aoo_event;

// event handler
typedef int32_t (*aoo_eventhandler)(
        void *,             // user
        const aoo_event **, // event array
        int32_t n           // number of events
);

#ifdef __cplusplus
} // extern "C"
#endif
