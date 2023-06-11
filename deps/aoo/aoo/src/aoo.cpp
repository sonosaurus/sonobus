#include "aoo/aoo.h"
#if AOO_NET
# include "aoo/aoo_requests.h"
# include "common/net_utils.hpp"
#endif
#include "aoo/aoo_codec.h"

#include "binmsg.hpp"
#include "detail.hpp"
#include "rt_memory_pool.hpp"

#include "common/sync.hpp"
#include "common/time.hpp"
#include "common/utils.hpp"

#include "aoo/codec/aoo_pcm.h"
#if AOO_USE_CODEC_OPUS
# include "aoo/codec/aoo_opus.h"
#endif

#define CERR_LOG_FUNCTION 1
#if CERR_LOG_FUNCTION
# include <cstdio>
# include <cstdarg>
# define CERR_LOG_MUTEX 0
# define CERR_LOG_LABEL 1
#endif // CERR_LOG_FUNCTION

#include <atomic>
#include <random>
#include <unordered_map>

#ifdef ESP_PLATFORM
# include "esp_system.h"
#endif

namespace aoo {

//--------------------- interface table -------------------//

void * AOO_CALL default_allocator(void *ptr, AooSize oldsize, AooSize newsize);

void AOO_CALL default_logfunc(AooLogLevel level, const char *message);

// populate interface table with default implementations
static AooCodecHostInterface g_interface = {
    sizeof(AooCodecHostInterface),
    aoo_registerCodec,
    default_allocator,
    default_logfunc
};

//--------------------- helper functions -----------------//

int32_t get_random_id(){
#if defined(ESP_PLATFORM)
    // use ESP hardware RNG
    return esp_random() & 0x7fffffff;
#else
    // software PRNG
#if defined(__i386__) || defined(_M_IX86) || \
        defined(__x86_64__) || defined(_M_X64) || \
        defined(__arm__) || defined(__aarch64__)
    // Don't use on embedded platforms because it can cause issues,
    // e.g. ESP-IDF stores thread_local variables on the stack!
    thread_local std::mt19937 mt(std::random_device{}());
#else
    // fallback for embedded platforms
    static sync::padded_spinlock spinlock;
    static std::mt19937 mt(std::random_device{}());
    sync::scoped_lock<sync::padded_spinlock> lock(spinlock);
#endif
    std::uniform_int_distribution<int32_t> dist;
    return dist(mt);
#endif
}

//----------------------- logging --------------------------//

#if CERR_LOG_FUNCTION

#if CERR_LOG_MUTEX
static sync::mutex g_log_mutex;
#endif

void AOO_CALL default_logfunc(AooLogLevel level, const char *message) {
    const char *label = nullptr;

#if CERR_LOG_LABEL
    switch (level) {
    case kAooLogLevelError:
        label = "error";
        break;
    case kAooLogLevelWarning:
        label = "warning";
        break;
    case kAooLogLevelVerbose:
        label = "verbose";
        break;
    case kAooLogLevelDebug:
        label = "debug";
        break;
    default:
        break;
    }
#endif

    const auto size = Log::buffer_size;
    char buffer[size];
    int count;
    if (label) {
        count = snprintf(buffer, size, "[aoo][%s] %s\n", label, message);
    } else {
        count = snprintf(buffer, size, "[aoo] %s\n", message);
    }

#if CERR_LOG_MUTEX
    // shouldn't be necessary since fwrite() is supposed
    // to be atomic.
    sync::scoped_lock<sync::mutex> lock(g_log_mutex);
#endif
    fwrite(buffer, count, 1, stderr);
    fflush(stderr);
}

#else // CERR_LOG_FUNCTION

#ifndef _MSC_VER
void __attribute__((format(printf, 2, 3 )))
#else
void
#endif
    default_logfunc(AooLogLevel, const char *, ...) {}

#endif // CERR_LOG_FUNCTION

Log::int_type Log::overflow(int_type c) {
    if (pos_ < buffer_size - 1) {
        buffer_[pos_++] = c;
        return 0;
    } else {
        return std::streambuf::traits_type::eof();
    }
}

std::streamsize Log::xsputn(const char_type *s, std::streamsize n) {
    auto limit = buffer_size - 1;
    if (pos_ < limit) {
        if (pos_ + n > limit) {
            n = limit - pos_;
        }
        memcpy(buffer_ + pos_, s, n);
        pos_ += n;
        return n;
    } else {
        return 0;
    }
}

Log::~Log() {
    if (aoo::g_interface.log) {
        buffer_[pos_] = '\0';
        aoo::g_interface.log(level_, buffer_);
    }
}

} // aoo

const char *aoo_strerror(AooError e){
    switch (e){
    case kAooErrorUnspecified:
        return "unspecified error";
    case kAooErrorNone:
        return "no error";
    case kAooErrorNotImplemented:
        return "not implemented";
    case kAooErrorNotPermitted:
        return "not permitted";
    case kAooErrorNotInitialized:
        return "not initialized";
    case kAooErrorBadArgument:
        return "bad argument";
    case kAooErrorBadFormat:
        return "bad format";
    case kAooErrorIdle:
        return "idle";
    case kAooErrorWouldBlock:
        return "would block";
    case kAooErrorOverflow:
        return "overflow";
    case kAooErrorOutOfMemory:
        return "out of memory";
    case kAooErrorAlreadyExists:
        return "already exists";
    case kAooErrorNotFound:
        return "not found";
    case kAooErrorInsufficientBuffer:
        return "insufficient buffer";
    case kAooErrorBadState:
        return "bad state";
    case kAooErrorSocket:
        return "socket error";
    case kAooErrorInternal:
        return "internal error";
    case kAooErrorSystem:
        return "system error";
    case kAooErrorUserDefined:
        return "user-defined error";
    case kAooErrorRequestInProgress:
        return "request in progress";
    case kAooErrorUnhandledRequest:
        return "unhandled request";
    case kAooErrorVersionNotSupported:
        return "version not supported";
    case kAooErrorUDPHandshakeTimeOut:
        return "UDP handshake time out";
    case kAooErrorWrongPassword:
        return "wrong password";
    case kAooErrorAlreadyConnected:
        return "already connected";
    case kAooErrorNotConnected:
        return "not connected";
    case kAooErrorGroupDoesNotExist:
        return "group does not exist";
    case kAooErrorCannotCreateGroup:
        return "cannot create group";
    case kAooErrorAlreadyGroupMember:
        return "already a group member";
    case kAooErrorNotGroupMember:
        return "not a group member";
    case kAooErrorUserAlreadyExists:
        return "user already exists";
    case kAooErrorUserDoesNotExist:
        return "user does not exist";
    case kAooErrorCannotCreateUser:
        return "cannot create user";
    default:
        return "unknown error code";
    }
}

//----------------------- OSC --------------------------//

AooError AOO_CALL aoo_parsePattern(
        const AooByte *msg, AooInt32 size,
        AooMsgType *type, AooId *id, AooInt32 *offset) {
    int32_t count = 0;
    if (aoo::binmsg_check(msg, size)) {
        *type = aoo::binmsg_type(msg, size);
        *id = aoo::binmsg_to(msg, size);
        auto n = aoo::binmsg_headersize(msg, size);
        if (n > 0) {
            if (offset) {
                *offset = n;
            }
            return kAooOk;
        } else {
            return kAooErrorBadArgument;
        }
    } else if (size >= kAooMsgDomainLen
        && !memcmp(msg, kAooMsgDomain, kAooMsgDomainLen)) {
        count += kAooMsgDomainLen;
        if (size >= (count + kAooMsgSourceLen)
            && !memcmp(msg + count, kAooMsgSource, kAooMsgSourceLen)) {
            *type = kAooMsgTypeSource;
            count += kAooMsgSourceLen;
        } else if (size >= (count + kAooMsgSinkLen)
            && !memcmp(msg + count, kAooMsgSink, kAooMsgSinkLen)) {
            *type = kAooMsgTypeSink;
            count += kAooMsgSinkLen;
        } else {
        #if AOO_NET
            if (size >= (count + kAooMsgClientLen)
                && !memcmp(msg + count, kAooMsgClient, kAooMsgClientLen)) {
                *type = kAooMsgTypeClient;
                count += kAooMsgClientLen;
            } else if (size >= (count + kAooMsgServerLen)
                && !memcmp(msg + count, kAooMsgServer, kAooMsgServerLen)) {
                *type = kAooMsgTypeServer;
                count += kAooMsgServerLen;
            } else if (size >= (count + kAooMsgPeerLen)
                && !memcmp(msg + count, kAooMsgPeer, kAooMsgPeerLen)) {
                *type = kAooMsgTypePeer;
                count += kAooMsgPeerLen;
            } else if (size >= (count + kAooMsgRelayLen)
                && !memcmp(msg + count, kAooMsgRelay, kAooMsgRelayLen)) {
                *type = kAooMsgTypeRelay;
                count += kAooMsgRelayLen;
            } else {
                return kAooErrorBadArgument;
            }

            if (offset) {
                *offset = count;
            }
        #endif // AOO_NET

            return kAooOk;
        }

        // /aoo/source or /aoo/sink
        if (id) {
            int32_t skip = 0;
            if (sscanf((const char *)(msg + count), "/%d%n", id, &skip) > 0) {
                count += skip;
            } else {
                // TODO only print relevant part of OSC address string
                LOG_ERROR("aoo_parse_pattern: bad ID " << (msg + count));
                return kAooErrorBadArgument;
            }
        } else {
            return kAooErrorBadArgument;
        }

        if (offset) {
            *offset = count;
        }
        return kAooOk;
    } else {
        return kAooErrorBadArgument; // not an AOO message
    }
}

//-------------------- NTP time ----------------------------//

uint64_t AOO_CALL aoo_getCurrentNtpTime(void){
    return aoo::time_tag::now();
}

double AOO_CALL aoo_osctime_to_seconds(AooNtpTime t){
    return aoo::time_tag(t).to_seconds();
}

uint64_t AOO_CALL aoo_osctime_from_seconds(AooSeconds s){
    return aoo::time_tag::from_seconds(s);
}

double AOO_CALL aoo_ntpTimeDuration(AooNtpTime t1, AooNtpTime t2){
    return aoo::time_tag::duration(t1, t2);
}

//---------------------- version -------------------------//

void AOO_CALL aoo_getVersion(AooInt32 *major, AooInt32 *minor,
                            AooInt32 *patch, AooInt32 *test){
    if (major) *major = kAooVersionMajor;
    if (minor) *minor = kAooVersionMinor;
    if (patch) *patch = kAooVersionPatch;
    if (test) *test = kAooVersionTest;
}

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

const char *aoo_getVersionString() {
    return STR(kAooVersionMajor) "." STR(kAooVersionMinor)
    #if kAooVersionPatch > 0
        "." STR(kAooVersionPatch)
    #endif
    #if kAooVersionTest > 0
       "-test" STR(kAooVersionTest)
    #endif
        ;
}

//---------------------- AooData ----------------------//

static std::unordered_map<std::string, AooDataType> g_data_type_map = {
    { "unspecified", kAooDataUnspecified },
    { "raw", kAooDataRaw },
    { "binary", kAooDataBinary },
    { "text", kAooDataText },
    { "osc", kAooDataOSC },
    { "midi", kAooDataMIDI },
    { "fudi", kAooDataFUDI },
    { "json", kAooDataJSON },
    { "xml", kAooDataXML }
};

AooDataType AOO_CALL aoo_dataTypeFromString(const AooChar *str) {
    auto it = g_data_type_map.find(str);
    if (it != g_data_type_map.end()) {
        return it->second;
    } else {
        return kAooDataUnspecified;
    }
}

const AooChar * AOO_CALL aoo_dataTypeToString(AooDataType type) {
    const AooChar *result;
    switch (type) {
    case kAooDataRaw:
        return "raw";
    case kAooDataText:
        return "text";
    case kAooDataOSC:
        return "osc";
    case kAooDataMIDI:
        return "midi";
    case kAooDataFUDI:
        return "fudi";
    case kAooDataJSON:
        return "json";
    case kAooDataXML:
        return "xml";
    default:
        if (type >= kAooDataUser) {
            return "user";
        } else {
            return nullptr;
        }
    }
}


namespace aoo {

AooError check_version(const char *version) {
    int major, minor;
    if (sscanf(version, "%d.%d", &major, &minor) != 2) {
        return kAooErrorBadFormat;
    }
    // LOG_DEBUG("major version: " << major << ", minor version: " << minor);

    if (major == kAooVersionMajor){
        return kAooOk;
    } else {
        return kAooErrorVersionNotSupported;
    }
}

//------------------- allocator ------------------//

#if AOO_DEBUG_MEMORY
std::atomic<ptrdiff_t> total_memory{0};
#endif

void * AOO_CALL default_allocator(void *ptr, AooSize oldsize, AooSize newsize) {
    if (newsize > 0) {
        // allocate new memory
        // NOTE: we never reallocate
        assert(ptr == nullptr && oldsize == 0);
    #if AOO_DEBUG_MEMORY
        auto total = total_memory.fetch_add(newsize, std::memory_order_relaxed) + (ptrdiff_t)newsize;
        LOG_ALL("allocate " << newsize << " bytes (total: " << total << ")");
    #endif
        return operator new(newsize);
    } else if (oldsize > 0) {
        // free memory
    #if AOO_DEBUG_MEMORY
        auto total = total_memory.fetch_sub(oldsize, std::memory_order_relaxed) - (ptrdiff_t)oldsize;
        LOG_ALL("deallocate " << oldsize << " bytes (total: " << total << ")");
    #endif
        operator delete(ptr);
    } else {
        // (de)allocating memory of size 0: do nothing.
        assert(ptr == nullptr);
    }
    return nullptr;
}

#if AOO_CUSTOM_ALLOCATOR || AOO_DEBUG_MEMORY

void * allocate(size_t size) {
    auto result = g_interface.alloc(nullptr, 0, size);
    if (!result && size > 0) {
        throw std::bad_alloc{};
    }
    return result;
}

void deallocate(void *ptr, size_t size){
    g_interface.alloc(ptr, size, 0);
}

#endif

//---------------------- RT memory --------------------------//

static rt_memory_pool<true, aoo::allocator<char>> g_rt_memory_pool;

void * rt_allocate(size_t size) {
    auto ptr = g_rt_memory_pool.allocate(size);
    if (!ptr && (size > 0)) {
        throw std::bad_alloc{};
    }
    return ptr;
}

void rt_deallocate(void *ptr, size_t size) {
    g_rt_memory_pool.deallocate(ptr, size);
}

sync::mutex g_rt_memory_pool_lock;
size_t g_rt_memory_pool_refcount = 0;

void rt_memory_pool_ref() {
    sync::scoped_lock<sync::mutex> l(g_rt_memory_pool_lock);
    g_rt_memory_pool_refcount++;
    // LOG_DEBUG("rt_memory_pool_ref: " << g_rt_memory_pool_refcount);
}

void rt_memory_pool_unref() {
    sync::scoped_lock<sync::mutex> l(g_rt_memory_pool_lock);
    if (--g_rt_memory_pool_refcount == 0) {
        LOG_DEBUG("total RT memory usage: " << g_rt_memory_pool.memory_usage()
                  << " / " << g_rt_memory_pool.size() << " bytes");
        g_rt_memory_pool.reset();
    }
    // LOG_DEBUG("rt_memory_pool_unref: " << g_rt_memory_pool_refcount);
}

} // aoo

//------------------------ codec ---------------------------//

namespace aoo {

// can't use std::unordered_map with custom allocator, so let's just use
// aoo::vector instead. performance might be better anyway, since the vector
// will be very small.

using codec_list = aoo::vector<std::pair<aoo::string, const AooCodecInterface *>>;
static codec_list g_codec_list;

const AooCodecInterface * find_codec(const char * name){
    for (auto& codec : g_codec_list) {
        if (codec.first == name) {
            return codec.second;
        }
    }
    return nullptr;
}

} // aoo

const AooCodecHostInterface * aoo_getCodecHostInterface(void)
{
    return &aoo::g_interface;
}

AooError AOO_CALL aoo_registerCodec(const char *name, const AooCodecInterface *codec){
    if (aoo::find_codec(name)) {
        LOG_WARNING("codec " << name << " already registered!");
        return kAooErrorAlreadyExists;
    }
    aoo::g_codec_list.emplace_back(name, codec);
    LOG_VERBOSE("registered codec '" << name << "'");
    return kAooOk;
}

//--------------------------- (de)initialize -----------------------------------//

void aoo_nullLoad(const AooCodecHostInterface *);
void aoo_nullUnload();
void aoo_pcmLoad(const AooCodecHostInterface *);
void aoo_pcmUnload();
#if AOO_USE_CODEC_OPUS
void aoo_opusLoad(const AooCodecHostInterface *);
void aoo_opusUnload();
#endif

#define CHECK_SETTING(ptr, field) \
    (ptr && AOO_CHECK_FIELD(ptr, AooSettings, field))

AooError AOO_CALL aoo_initialize(const AooSettings *settings) {
    static bool initialized = false;
    if (!initialized) {
    #if AOO_NET
        aoo::socket_init();
    #endif
        // optional settings
        if (CHECK_SETTING(settings, logFunc) && settings->logFunc) {
            aoo::g_interface.log = settings->logFunc;
        }
        if (CHECK_SETTING(settings, allocFunc) && settings->allocFunc) {
    #if AOO_CUSTOM_ALLOCATOR
            aoo::g_interface.alloc = settings->allocFunc;
    #else
            LOG_WARNING("aoo_initialize: custom allocator not supported");
    #endif
        }

        if (CHECK_SETTING(settings, memPoolSize) && settings->memPoolSize > 0) {
            aoo::g_rt_memory_pool.resize(settings->memPoolSize);
        } else {
            aoo::g_rt_memory_pool.resize(AOO_MEM_POOL_SIZE);
        }

        // register codecs
        aoo_nullLoad(&aoo::g_interface);
        aoo_pcmLoad(&aoo::g_interface);
    #if AOO_USE_CODEC_OPUS
        aoo_opusLoad(&aoo::g_interface);
    #endif

        initialized = true;
    }
    return kAooOk;
}

void AOO_CALL aoo_terminate() {
#if AOO_DEBUG_MEMORY
    aoo::g_rt_memory_pool.print();
#endif
    // unload codecs
    aoo_nullUnload();
    aoo_pcmUnload();
#if AOO_USE_CODEC_OPUS
    aoo_opusUnload();
#endif
    // free codec plugin list
    aoo::codec_list tmp;
    std::swap(tmp, aoo::g_codec_list);
}
