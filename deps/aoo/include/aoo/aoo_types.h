/* Copyright (c) 2021 Christof Ressi
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#pragma once

#include "aoo_config.h"
#include "aoo_defines.h"

#include <stdint.h>
#include <stddef.h>

AOO_PACK_BEGIN

/*------------------------------------------------------------------*/

/** \brief boolean type */
typedef int32_t AooBool;

/** \brief 'true' boolean constant */
#define kAooTrue ((AooBool)1)
/** \brief 'false' boolean constant */
#define kAooFalse ((AooBool)0)

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

/*------------------------------------------------------------------*/

/** \brief struct size type */
typedef AooUInt32 AooStructSize;

/** \brief generic ID type */
typedef AooInt32 AooId;

/** \brief invalid AooId constant */
#define kAooIdInvalid -1
/** \brief smallest valid AooId */
#define kAooIdMin 0
/** \brief largest valid AooId */
#define kAooIdMax INT32_MAX

/** \brief fixed-width enum type */
typedef AooInt32 AooEnum;

/** \brief define a fixed-width named enum */
#define AOO_ENUM(name) \
    typedef AooEnum name; \
    enum

/** \brief flag/bitmap type */
typedef AooUInt32 AooFlag;

/** \brief define fixed-width named flags */
#define AOO_FLAG(name) \
    typedef AooFlag name; \
    enum

/*------------------------------------------------------------------*/

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

/** \brief NTP time point */
typedef AooUInt64 AooNtpTime;

/** \brief constant representing the current time */
#define kAooNtpTimeNow 1

/** \brief time point/interval in seconds */
typedef double AooSeconds;

/** \brief sample rate type */
typedef double AooSampleRate;

/** \brief AOO control type */
typedef AooInt32 AooCtl;

/*------------------------------------------------------------------*/

/** \brief AOO socket handle type */
typedef AooInt32 AooSocket;

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

/** \brief AOO IP endpoint */
typedef struct AooIpEndpoint
{
    const AooChar *hostName;
    AooUInt16 port;
} AooIpEndpoint;

/*------------------------------------------------------------------*/

/** \brief AOO message types */
AOO_ENUM(AooMsgType)
{
    /** AOO source */
    kAooMsgTypeSource = 0,
    /** AOO sink */
    kAooMsgTypeSink,
    /** AOO server */
    kAooMsgTypeServer,
    /** AOO client */
    kAooMsgTypeClient,
    /** AOO peer */
    kAooMsgTypePeer,
    /** relayed message */
    kAooMsgTypeRelay
};

/*------------------------------------------------------------------*/

/** \brief error codes */
AOO_ENUM(AooError)
{
    /** unknown/unspecified error */
    kAooErrorUnspecified = -1,
    /** no error (= success) */
    kAooErrorNone = 0,
    /*------------- generic errors -------------*/
    /** operation/control not implemented */
    kAooErrorNotImplemented,
    /** operation/control not permitted */
    kAooErrorNotPermitted,
    /** not initialized */
    kAooErrorNotInitialized,
    /** bad argument for function/method call */
    kAooErrorBadArgument,
    /** bad message/structure format */
    kAooErrorBadFormat,
    /** AOO source/sink is idle;
     * no need to call `send()` resp. notify the send thread */
    kAooErrorIdle,
    /** operation would block */
    kAooErrorWouldBlock,
    /** operation would overflow */
    kAooErrorOverflow,
    /** out of memory */
    kAooErrorOutOfMemory,
    /** resource already exists */
    kAooErrorAlreadyExists,
    /** resource not found */
    kAooErrorNotFound,
    /** insufficient buffer size */
    kAooErrorInsufficientBuffer,
    /** bad state */
    kAooErrorBadState,
    /** socket error
     * use errno resp. WSAGetLastError for more details */
    kAooErrorSocket,
    /** codec error */
    kAooErrorCodec,
    /** internal error */
    kAooErrorInternal,
    /** system error
     * use errno resp. GetLastError for more details */
    kAooErrorSystem,
    /** user-defined error */
    kAooErrorUserDefined,
    /*------------- server errors -------------*/
    /** request is already in progress */
    kAooErrorRequestInProgress = 1000,
    /** request has not been handled */
    kAooErrorUnhandledRequest,
    /** version not supported */
    kAooErrorVersionNotSupported,
    /** UDP handshake time out */
    kAooErrorUDPHandshakeTimeOut,
    /** wrong passoword */
    kAooErrorWrongPassword,
    /** already connected to server */
    kAooErrorAlreadyConnected,
    /** not connected to server */
    kAooErrorNotConnected,
    /** group does not exist */
    kAooErrorGroupDoesNotExist,
    /** cannot create group on the server */
    kAooErrorCannotCreateGroup,
    /** already a group member */
    kAooErrorAlreadyGroupMember,
    /** not a group member */
    kAooErrorNotGroupMember,
    /** user already exists */
    kAooErrorUserAlreadyExists,
    /** user does not exist */
    kAooErrorUserDoesNotExist,
    /** cannot create user */
    kAooErrorCannotCreateUser,
    /** start of user defined error codes (for custom AOO versions) */
    kAooErrorCustom = 10000
};

/** \brief alias for success result */
#define kAooOk kAooErrorNone

/*------------------------------------------------------------------*/

/** \brief flags for AooSource/AooSink setup */
AOO_FLAG(AooSetupFlags)
{
    /** the block size will not vary between process() calls */
    kAooFixedBlockSize = 0x01
};

/*------------------------------------------------------------------*/

/** \brief flags for AooClient_setup / AooClient::setup and
 *         AooServer_setup / AooServer::setup
 *
 * Possible combinations:
 *   - `kAooSocketIPv4` IPv4 only
 *   - `kAooSocketIPv6` IPv6 only
 *   - `kAooSocketIPv4 | kAooSocketIPv6` dedicated IPv4 and IPv6 sockets
 *   - `kAooSocketIP6 | kAooSocketIPv4Mapped` IPv6 dual-stack socket
 */
AOO_FLAG(AooSocketFlags)
{
    /** client uses IPv4 addresses */
    kAooSocketIPv4 = 0x01,
    /** client uses IPv6 addresses */
    kAooSocketIPv6 = 0x02,
    /** client uses IPv4-mapped addresses */
    kAooSocketIPv4Mapped = 0x04
};

#define kAooSocketDualStack (kAooSocketIPv6 | kAooSocketIPv4Mapped)

/*------------------------------------------------------------------*/

/** \brief see AooResponseConnect */
AOO_FLAG(AooServerFlags)
{
    /** clients may use the server as relay */
    kAooServerRelay = 0x01
};

/*------------------------------------------------------------------*/

/** \brief see AooEventPeer */
AOO_FLAG(AooPeerFlags)
{
    /** peer-to-peer not possible, need relay */
    kAooPeerNeedRelay = 0x01,
    /** peer persists between sessions */
    kAooPeerPersistent = 0x02,
    /** peer has created the group */
    kAooPeerGroupCreator = 0x04
};

/*------------------------------------------------------------------*/

/** \brief see AooEventGroupAdd and AooResponseGroupJoin */
AOO_FLAG(AooGroupFlags)
{
    /** group persists between sessions */
    kAooGroupPersistent = 0x01
};

/*------------------------------------------------------------------*/

/** \brief see AooEventGroupJoin and AooResponseGroupJoin */
AOO_FLAG(AooUserFlags)
{
    /** user persists between sessions */
    kAooUserPersistent = 0x01,
    /** user has created the group */
    kAooUserGroupCreator = 0x02
};

/*------------------------------------------------------------------*/

/** \brief flags for AooClient_sendMessage / AooClient::sendMessage */
AOO_FLAG(AooMessageFlags)
{
    /** message should be delivered reliable */
    kAooMessageReliable = 0x01
};

/*------------------------------------------------------------------*/

/** \brief thread levels (`AooThreadLevel`) */
AOO_ENUM(AooThreadLevel)
{
    /** unknown thread level */
    kAooThreadLevelUnknown = 0,
    /** audio thread */
    kAooThreadLevelAudio = 1,
    /** network thread(s) */
    kAooThreadLevelNetwork = 2
};

/*------------------------------------------------------------------*/

/** \brief event modes */
AOO_ENUM(AooEventMode)
{
    /** no events */
    kAooEventModeNone = 0,
    /** use event callback */
    kAooEventModeCallback = 1,
    /** poll for events */
    kAooEventModePoll = 2
};

/*------------------------------------------------------------------*/

typedef union AooEvent AooEvent;

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

/*------------------------------------------------------------------*/

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

/*------------------------------------------------------------------*/

/** \brief server reply function
 *
 * \attention This function must send the entire message!
 * Partial writes are not allowed.
 * \param user user data
 * \param clientId client ID
 * \param data message data
 * \param size message size
 * \param flags send flags
 * \return number of bytes written, or -1 on error
 */
typedef AooInt32 (AOO_CALL *AooServerReplyFunc)(
        /** the user data */
        void *user,
        /** the client ID */
        AooId clientId,
        /** the message content */
        const AooByte *data,
        /** the message size in bytes */
        AooSize size
);

/*------------------------------------------------------------------*/

/** \brief AOO data types */
AOO_ENUM(AooDataType)
{
    /** unspecified data type */
    kAooDataUnspecified = -1,
    /** raw or binary data */
    kAooDataRaw = 0,
    kAooDataBinary = 0,
    /** plain text (UTF-8 encoded) */
    kAooDataText,
    /** OSC message (Open Sound Control) */
    kAooDataOSC,
    /** MIDI */
    kAooDataMIDI,
    /** FUDI (Pure Data) */
    kAooDataFUDI,
    /** JSON (UTF-8 encoded) */
    kAooDataJSON,
    /** XML (UTF-8 encoded) */
    kAooDataXML,
    /** start of user specified types */
    kAooDataUser = 1000
};

/*------------------------------------------------------------------*/

/** \brief view on arbitrary structured data */
typedef struct AooData
{
    /** the data type */
    AooDataType type;
    /** the data content */
    const AooByte *data;
    /** the data size in bytes */
    AooSize size;
} AooData;

/*------------------------------------------------------------------*/

/** \brief AOO stream message */
typedef struct AooStreamMessage
{
    /** sample offset */
    AooInt32 sampleOffset;
    /** the message type */
    AooDataType type;
    /** the data content */
    const AooByte *data;
    /** the data size in bytes */
    AooSize size;
} AooStreamMessage;

/*------------------------------------------------------------------*/

/** \brief stream message handler
 *
 * The type of function that is passed to #AooSink::process
 * for handling stream messages.
 *
 * \warning Do not call any AOO functions inside the handler function!
 */
typedef void (AOO_CALL *AooStreamMessageHandler)(
        /** the user data */
        void *user,
        /** the stream message */
        const AooStreamMessage *message,
        /** the AOO source that sent the message */
        const AooEndpoint *source
);

/*------------------------------------------------------------------*/

/** \brief max. size of codec names (including \0) */
#define kAooCodecNameMaxSize 32

/** \brief common header for all AOO format structs */
typedef struct AooFormat
{
    /** the format structure size (including the header) */
    AooInt32 structSize;
    /** the number of channels */
    AooInt32 numChannels;
    /** the sample rate */
    AooInt32 sampleRate;
    /** the max. block size */
    AooInt32 blockSize;
    /** the codec name */
    AooChar codecName[kAooCodecNameMaxSize];
} AooFormat;

/*------------------------------------------------------------------*/

/** \brief the max. size of an AOO format struct */
#define kAooFormatMaxSize 128
/** \brief the max. size of AOO format header extensions */
#define kAooFormatExtMaxSize (kAooFormatMaxSize - sizeof(AooFormat))

/** \brief helper struct large enough to hold any AOO format struct */
typedef struct AooFormatStorage
{
    AooFormat header;
    AooByte data[kAooFormatExtMaxSize];
} AooFormatStorage;

/*------------------------------------------------------------------*/

typedef union AooRequest AooRequest;

/** \brief server request handler (to intercept client requests)
 * \param user user data
 * \param client client ID
 * \param token request token
 * \param request the client request
 * \return #kAooTrue: handled manually; #kAooFalse: handle automatically.
 */
typedef AooBool (AOO_CALL *AooRequestHandler)(
        void *user,
        AooId client,
        AooId token,
        const AooRequest *request
);

/*------------------------------------------------------------------*/

typedef union AooResponse AooResponse;

/** \brief client response handler
 *
 * If `result` is `kAooErrorNone`, you can safely access the corresponding
 * response member, e.g. `response->connect` for a connection request.
 * Otherwise the request has failed. If the error code is `kAooErrorSystem`
 * or `kAooErrorUserDefined`, you may access `response->error` for more details.
 */
typedef void (AOO_CALL *AooResponseHandler)(
        /** the user data */
        void *user,
        /** the original request */
        const AooRequest *request,
        /** the result error code */
        AooError result,
        /** the response */
        const AooResponse *response
);

/*------------------------------------------------------------------*/

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

/*------------------------------------------------------------------*/

/** \brief log levels */
typedef AooInt32 AooLogLevel;

/* NB: log level constants must be macros, otherwise they cannot
 * be used in #if clause, as they would expand to zero! */

/** no logging */
#define kAooLogLevelNone 0
/** only errors */
#define kAooLogLevelError 1
/** only errors and warnings */
#define kAooLogLevelWarning 2
/** errors, warnings and notifications */
#define kAooLogLevelVerbose 3
/** errors, warnings, notifications and debug messages */
#define kAooLogLevelDebug 4

/** \brief custom log function type
 * \param level the log level
 * \param message the message
 */
typedef void (AOO_CALL *AooLogFunc)
        (AooLogLevel level, const AooChar *message);

/*------------------------------------------------------------------*/

AOO_PACK_END
