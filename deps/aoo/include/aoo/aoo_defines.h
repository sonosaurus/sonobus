/* Copyright (c) 2021 Christof Ressi
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/** \file
 * \brief types and constants
 *
 * contains typedefs, constants, enumerations and struct declarations
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

#include <inttypes.h>

/* check for C++11
 * NB: MSVC does not correctly set __cplusplus by default! */
#if defined(__cplusplus) && (__cplusplus >= 201103L || ((defined(_MSC_VER) && _MSC_VER >= 1900)))
    #define AOO_HAVE_CXX11 1
#else
    #define AOO_HAVE_CXX11 0
#endif


#if defined(__cplusplus)
# define AOO_INLINE inline
#else
# if (__STDC_VERSION__ >= 199901L)
#  define AOO_INLINE static inline
# else
#  define AOO_INLINE static
# endif
#endif

#ifndef AOO_CALL
# ifdef _WIN32
#  define AOO_CALL __cdecl
# else
#  define AOO_CALL
# endif
#endif

#ifndef AOO_EXPORT
# ifndef AOO_STATIC
#  if defined(_WIN32) // Windows
#   if defined(AOO_BUILD)
#      if defined(DLL_EXPORT)
#        define AOO_EXPORT __declspec(dllexport)
#      else
#        define AOO_EXPORT
#      endif
#   else
#    define AOO_EXPORT __declspec(dllimport)
#   endif
#  elif defined(__GNUC__) && defined(AOO_BUILD) // GNU C
#   define AOO_EXPORT __attribute__ ((visibility ("default")))
#  else /* Other */
#   define AOO_EXPORT
#  endif
# else /* AOO_STATIC */
#  define AOO_EXPORT
# endif
#endif

#ifdef __cplusplus
# define AOO_API extern "C" AOO_EXPORT
#else
# define AOO_API AOO_EXPORT
#endif

/*---------- struct packing -----------------*/

#if defined(__GNUC__)
# define AOO_PACK_BEGIN _Pragma("pack(push,8)")
# define AOO_PACK_END _Pragma("pack(pop)")
# elif defined(_MSC_VER)
# define AOO_PACK_BEGIN __pragma(pack(push,8))
# define AOO_PACK_END __pragma(pack(pop))
#else
# define AOO_PACK_BEGIN
# define AOO_PACK_END
#endif

/* begin struct packing */
AOO_PACK_BEGIN

/*---------- global compile time settings ----------*/

/** \brief AOO_NET support */
#ifndef USE_AOO_NET
# define USE_AOO_NET 1
#endif

/** \brief custom allocator support  */
#ifndef AOO_CUSTOM_ALLOCATOR
# define AOO_CUSTOM_ALLOCATOR 0
#endif

/** \brief default UDP packet size */
#ifndef AOO_PACKET_SIZE
# define AOO_PACKET_SIZE 512
#endif

/** \brief max. UDP packet size */
#ifndef AOO_MAX_PACKET_SIZE
# define AOO_MAX_PACKET_SIZE 4096
#endif

/** \brief debug memory usage */
#ifndef AOO_DEBUG_MEMORY
# define AOO_DEBUG_MEMORY 0
#endif

/*---------------- versioning ----------------*/

/** \brief the AOO major version */
#define kAooVersionMajor 2
/** \brief the AOO minor version */
#define kAooVersionMinor 0
/** \brief the AOO bugfix version */
#define kAooVersionPatch 0
/** \brief the AOO test version (0: stable release) */
#define kAooVersionTest 3

/*------------ general data types ----------------*/

/** \brief boolean type */
typedef int32_t AooBool;

/** \brief 'true' boolean constant */
#define kAooTrue 1
/** \brief 'false' boolean constant */
#define kAooFalse 0

/** \brief character type */
typedef char AooChar;

/** \brief byte type */
typedef uint8_t AooByte;

/** \brief 16-bit signed integer */
typedef int16_t AooInt16;
/** \brief 16-bit unsigned integer */
typedef uint16_t AooUInt16;

/** \brief 32-bit signed integer */
typedef int32_t AooInt32;
/** \brief 32-bit unsigned integer */
typedef uint32_t AooUInt32;

/** \brief 64-bit signed integer */
typedef int64_t AooInt64;
/** \brief 64-bit unsigned integer */
typedef uint64_t AooUInt64;

/** \brief size type */
typedef size_t AooSize;

/** \brief pointer-sized signed integer */
typedef intptr_t AooIntPtr;
/** \brief pointer-sized unsigned integer */
typedef uintptr_t AooUIntPtr;

/*------------ semantic data types --------------*/

/** \brief audio sample size in bits */
#ifndef AOO_SAMPLE_SIZE
# define AOO_SAMPLE_SIZE 32
#endif

#if AOO_SAMPLE_SIZE == 32
/** \brief audio sample type */
typedef float AooSample;
#elif AOO_SAMPLE_SIZE == 64
/** \brief audio sample type */
typedef double AooSample
#else
# error "unsupported value for AOO_SAMPLE_SIZE"
#endif

/** \brief generic ID type */
typedef AooInt32 AooId;

/** \brief invalid AooId constant */
#define kAooIdInvalid -1
/** \brief smallest valid AooId */
#define kAooIdMin 0
/** \brief largest valid AooId */
#define kAooIdMax INT32_MAX

/** \brief flag/bit map type */
typedef AooUInt32 AooFlag;

/** \brief NTP time stamp */
typedef AooUInt64 AooNtpTime;

/** \brief constant representing the current time */
#define kAooNtpTimeNow 1

/** \brief time point/interval in seconds */
typedef double AooSeconds;

/** \brief sample rate type */
typedef double AooSampleRate;

/** \brief AOO control type */
typedef AooInt32 AooCtl;

/** \brief AOO socket handle type */
typedef AooInt32 AooSocket;

/*---------------- error codes ----------------*/

/** \brief error code type */
typedef AooInt32 AooError;

/** \brief list of available error codes */
enum AooErrorCodes
{
    /** unknown/unspecified error */
    kAooErrorUnknown = -1,
    /** no error (= success) */
    kAooErrorNone = 0,
    /** operation/control not implemented */
    kAooErrorNotImplemented,
    /** bad argument for function/method call */
    kAooErrorBadArgument,
    /** AOO source/sink is idle;
     * no need to call `send()` resp. notify the send thread */
    kAooErrorIdle,
    /** out of memory */
    kAooErrorOutOfMemory,
    /** resource not found */
    kAooErrorNotFound,
    /** insufficient buffer size */
    kAooErrorInsufficientBuffer
};

/** \brief alias for success result */
#define kAooOk kAooErrorNone

/*---------------- AOO events --------------------*/

/** \brief  AOO event type */
typedef AooInt32 AooEventType;

/** \brief generic AOO event */
typedef struct AooEvent
{
    /** the event type; \see AooEventTypes */
    AooEventType type;
} AooEvent;

/** \brief thread level type */
typedef AooInt32 AooThreadLevel;

/** \brief unknown thread level */
#define kAooThreadLevelUnknown 0
/** \brief audio thread */
#define kAooThreadLevelAudio 1
/** \brief network thread(s) */
#define kAooThreadLevelNetwork 2

/** \brief event mode type */
typedef AooInt32 AooEventMode;

/** \brief no events */
#define kAooEventModeNone 0
/** \brief use event callback */
#define kAooEventModeCallback 1
/** \brief poll for events */
#define kAooEventModePoll 2

/** \brief event handler function
 *
 * The callback function type that is passed to
 * AooSource, AooSink or AooClient to receive events.
 * If the callback is registered with #kAooEventModeCallback,
 * the function is called immediately when an event occurs.
 * The event should be handled appropriately regarding the
 * current thread level.
 * If the callback is registered with #kAooEventModePoll,
 * the user has to regularly poll for pending events.
 * Polling itself is realtime safe and can be done from
 * any thread.
 */
typedef void (AOO_CALL *AooEventHandler)(
        /** the user data */
        void *user,
        /** the event */
        const AooEvent *event,
        /** the current thread level
         * (only releveant for #kAooEventModeCallback) */
        AooThreadLevel level
);

/*---------------- AOO endpoint ------------------*/

/** \brief socket address size type */
typedef AooUInt32 AooAddrSize;

typedef struct AooSockAddr
{
    const void *data;
    AooAddrSize size;
} AooSockAddr;

/** \brief AOO endpoint */
typedef struct AooEndpoint
{
    /** socket address */
    const void *address;
    /** socket address length */
    AooAddrSize addrlen;
    /** source/sink ID */
    AooId id;
} AooEndpoint;

/* TODO do we need this? */
enum AooEndpointFlags
{
    kAooEndpointRelay = 0x01
};

typedef struct AooIpEndpoint
{
    const AooChar *hostName;
    AooInt32 port;
} AooIpEndpoint;

/** \brief send function
 *
 * The function type that is passed to #AooSource,
 * #AooSink or #AooClient for sending outgoing network packets.
 */
typedef AooInt32 (AOO_CALL *AooSendFunc)(
        /** the user data */
        void *user,
        /** the packet content */
        const AooByte *data,
        /** the packet size in bytes */
        AooInt32 size,
        /** the socket address */
        const void *address,
        /** the socket address length */
        AooAddrSize addrlen,
        /** optional flags */
        /* TODO do we need this? */
        AooFlag flags
);

/*------------- AOO message types ----------------*/

/** \brief type for AOO message destination types */
typedef AooInt32 AooMsgType;

/** \brief AOO message destination types */
enum AooMsgTypes
{
    /** AOO source */
    kAooTypeSource = 0,
    /** AOO sink */
    kAooTypeSink,
#if USE_AOO_NET
    /** AOO server */
    kAooTypeServer,
    /** AOO client */
    kAooTypeClient,
    /** AOO peer */
    kAooTypePeer,
    /** relayed message */
    kAooTypeRelay,
#endif
    kAooTypeSentinel
};

/*------------- AOO data view ---------------*/

/** \brief view on arbitrary structured data */
typedef struct AooDataView
{
    /** C string describing the data format */
    const AooChar *type;
    /** the data content */
    const AooByte *data;
    /** the data size in bytes */
    AooSize size;
} AooDataView;

/** \brief max. length of data type strings (excluding the trailing `\0`) */
#define kAooDataTypeMaxLen 63

/* pre-defined data formats */

/** \brief plain text (UTF-8 encoded) */
#define kAooDataTypeText "text"
/** \brief JSON (UTF-8 encoded) */
#define kAooDataTypeJSON "json"
/** \brief XML (UTF-8 encoded) */
#define kAooDataTypeXML "xml"
/** \brief OSC message (Open Sound Control) */
#define kAooDataTypeOSC "osc"
/** \brief FUDI (Pure Data) */
#define kAooDataTypeFUDI "fudi"
/** \brief invalid data type */
#define kAooDataTypeInvalid ""

/* Users can define their own data formats.
 * Make sure to use a unique type name!
 * HINT: you can use an ASCII encoded UUID. */

/*---------------- AOO format --------------------*/

/** \brief max. size of codec names */
#define kAooCodecNameMaxLen 16

/** \brief common header for all AOO format structs */
typedef struct AooFormat
{
    /** the codec name */
    AooChar codec[kAooCodecNameMaxLen];
    /** the format structure size (including the header) */
    AooInt32 size;
    /** the number of channels */
    AooInt32 numChannels;
    /** the sample rate */
    AooInt32 sampleRate;
    /** the max. block size */
    AooInt32 blockSize;
} AooFormat;

/** \brief the max. size of AOO format header extensions */
#define kAooFormatExtMaxSize 64

/** \brief helper struct large enough to hold any AOO format struct */
typedef struct AooFormatStorage
{
    AooFormat header;
    AooByte data[kAooFormatExtMaxSize];
} AooFormatStorage;

/*------------ memory allocation --------------*/

/** \brief custom allocator function
 * \param ptr pointer to memory block;
 *    `NULL` if `oldsize` is 0.
 * \param oldsize original size of memory block;
 *    0 for allocating new memory
 * \param newsize size of the memory block;
 *    0 for freeing existing memory.
 * \return pointer to the new memory block;
 *    `NULL` if `newsize` is 0 or the allocation failed.
 *
 * If `oldsize` is 0 and `newsize` is not 0, the function
 * shall behave like `malloc`.
 * If `oldsize` is not 0 and `newsize` is 0, the function
 * shall behave like `free`.
 * If both `oldsize`and `newsize` are not 0, the function
 * shall behave like `realloc`.
 * If both `oldsize` and `newsize` are 0, the function
 * shall have no effect.
 */
typedef void * (AOO_CALL *AooAllocFunc)
    (void *ptr, AooSize oldsize, AooSize newsize);

/*---------------- logging ----------------*/

/** \brief log level type */
typedef AooInt32 AooLogLevel;

/** \brief no logging */
#define kAooLogLevelNone 0
/** \brief only errors */
#define kAooLogLevelError 1
/** \brief only errors and warnings */
#define kAooLogLevelWarning 2
/** \brief errors, warnings and notifications */
#define kAooLogLevelVerbose 3
/** \brief errors, warnings, notifications and debug messages */
#define kAooLogLevelDebug 4

/** \brief compile time log level */
#ifndef AOO_LOG_LEVEL
#define AOO_LOG_LEVEL kAooLogLevelWarning
#endif

/** \brief custom log function type
 * \param level the log level
 * \param fmt the format string
 * \param ... arguments
 */
typedef void
/** \cond */
#ifndef _MSC_VER
    __attribute__((format(printf, 2, 3 )))
#endif
/** \endcond */
    (AOO_CALL *AooLogFunc)
        (AooLogLevel level, const AooChar *fmt, ...);

/*--------------------------------------*/

AOO_PACK_END
