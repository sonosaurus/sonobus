/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others. 
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#pragma once

#include <stdint.h>
#include <cstring>
#include <iostream>

/*------------------ alloca -----------------------*/
#ifdef _WIN32
# include <malloc.h> // MSVC or mingw on windows
# ifdef _MSC_VER
#  define alloca _alloca
# endif
#elif defined(__linux__) || defined(__APPLE__)
# include <alloca.h> // linux, mac, mingw, cygwin
#else
# include <stdlib.h> // BSDs for example
#endif

// 0: error, 1: warning, 2: verbose, 3: debug
#ifndef LOGLEVEL
 #define LOGLEVEL 1
#endif

#define DO_LOG(x) do { std::cerr << x << std::endl; } while (false);

#if LOGLEVEL >= 0
 #define LOG_ERROR(x) DO_LOG(x)
#else
 #define LOG_ERROR(x)
#endif

#if LOGLEVEL >= 1
 #define LOG_WARNING(x) DO_LOG(x)
#else
 #define LOG_WARNING(x)
#endif

#if LOGLEVEL >= 2
 #define LOG_VERBOSE(x) DO_LOG(x)
#else
 #define LOG_VERBOSE(x)
#endif

#if LOGLEVEL >= 3
 #define LOG_DEBUG(x) DO_LOG(x)
#else
 #define LOG_DEBUG(x)
#endif

/*------------------ endianess -------------------*/
    // endianess check taken from Pure Data (d_osc.c)
#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__FreeBSD_kernel__) \
    || defined(__OpenBSD__)
#include <machine/endian.h>
#endif

#if defined(__linux__) || defined(__CYGWIN__) || defined(__GNU__) || \
    defined(__EMSCRIPTEN__) || \
    defined(ANDROID)
#include <endian.h>
#endif

#ifdef __MINGW32__
#include <sys/param.h>
#endif

#ifdef _MSC_VER
/* _MSVC lacks BYTE_ORDER and LITTLE_ENDIAN */
 #define LITTLE_ENDIAN 0x0001
 #define BYTE_ORDER LITTLE_ENDIAN
#endif

#if !defined(BYTE_ORDER) || !defined(LITTLE_ENDIAN)
 #error No byte order defined
#endif

namespace aoo {

template<typename T>
constexpr bool is_pow2(T i){
    return (i & (i - 1)) == 0;
}

template<typename T>
T from_bytes(const char *b){
    union {
        T t;
        char b[sizeof(T)];
    } c;
#if BYTE_ORDER == BIG_ENDIAN
    memcpy(c.b, b, sizeof(T));
#else
    for (size_t i = 0; i < sizeof(T); ++i){
        c.b[i] = b[sizeof(T) - i - 1];
    }
#endif
    return c.t;
}

template<typename T>
void to_bytes(T v, char *b){
    union {
        T t;
        char b[sizeof(T)];
    } c;
    c.t = v;
#if BYTE_ORDER == BIG_ENDIAN
    memcpy(b, c.b, sizeof(T));
#else
    for (size_t i = 0; i < sizeof(T); ++i){
        b[i] = c.b[sizeof(T) - i - 1];
    }
#endif
}

} // aoo
