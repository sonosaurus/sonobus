#include "aoo/aoo.h"
#if USE_AOO_NET
# include "aoo/aoo_net.h"
# include "common/net_utils.hpp"
#endif
#include "aoo/aoo_codec.h"

#include "imp.hpp"

#include "common/sync.hpp"
#include "common/time.hpp"
#include "common/utils.hpp"

#include "aoo/codec/aoo_pcm.h"
#if USE_CODEC_OPUS
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

//--------------------- helper functions -----------------//

char * copy_string(const char * s){
    if (s){
        auto len = strlen(s);
        auto result = aoo::allocate(len + 1);
        memcpy(result, s, len + 1);
        return (char *)result;
    } else {
        return nullptr;
    }
}

void free_string(char *s){
    if (s){
        auto len = strlen(s);
        aoo::deallocate(s, len + 1);
    }
}

void * copy_sockaddr(const void *sa, int32_t len){
    if (sa){
        auto result = aoo::allocate(len);
        memcpy(result, sa, len);
        return result;
    } else {
        return nullptr;
    }
}

void free_sockaddr(void *sa, int32_t len){
    if (sa){
        aoo::deallocate(sa, len);
    }
}

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
    thread_local std::random_device dev;
    thread_local std::mt19937 mt(dev());
#else
    // fallback for embedded platforms
    static sync::padded_spinlock spinlock;
    static std::random_device dev;
    static std::mt19937 mt(dev());
    sync::scoped_lock<sync::padded_spinlock> lock(spinlock);
#endif
    std::uniform_int_distribution<int32_t> dist;
    return dist(mt);
#endif
}

} // aoo

//------------------- allocator ------------------//

#if AOO_CUSTOM_ALLOCATOR || AOO_DEBUG_MEMORY

namespace aoo {

#if AOO_DEBUG_MEMORY
std::atomic<ptrdiff_t> total_memory{0};
#endif

static AooAllocFunc g_allocator =
        [](void *ptr, AooSize oldsize, AooSize newsize) -> void*
{
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
};

void * allocate(size_t size){
    auto result = g_allocator(nullptr, 0, size);
    if (!result && size > 0) {
        throw std::bad_alloc{};
    }
    return result;
}

void deallocate(void *ptr, size_t size){
    g_allocator(ptr, size, 0);
}

} // aoo

#endif

//----------------------- logging --------------------------//

namespace aoo {

#if CERR_LOG_FUNCTION

#if CERR_LOG_MUTEX
static sync::mutex g_log_mutex;
#endif

static void
#ifndef _MSC_VER
    __attribute__((format(printf, 2, 3 )))
#endif
        cerr_logfunction(AooLogLevel level, const char *fmt, ...)
{
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
    auto size = Log::buffer_size;
    char buffer[size];
    int count = 0;
    if (label) {
        count += snprintf(buffer, size, "[aoo][%s] ", label);
    } else {
        count += snprintf(buffer, size, "[aoo] ");
    }

    va_list args;
    va_start (args, fmt);
    count += vsnprintf(buffer + count, size - count, fmt, args);
    va_end (args);

    // force newline
    count = std::min(count, size - 2);
    buffer[count++] = '\n';
    buffer[count++] = '\0';

#if CERR_LOG_MUTEX
    // shouldn't be necessary since fwrite() is supposed
    // to be atomic.
    sync::scoped_lock<sync::mutex> lock(g_log_mutex);
#endif
    fwrite(buffer, count, 1, stderr);
    fflush(stderr);
}

static AooLogFunc g_logfunction = cerr_logfunction;

#else // CERR_LOG_FUNCTION

static AooLogFunc g_logfunction = nullptr;

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
    if (g_logfunction) {
        buffer_[pos_] = '\0';
        g_logfunction(level_, buffer_);
    }
}

} // aoo

const char *aoo_strerror(AooError e){
    switch (e){
    case kAooErrorUnknown:
        return "unspecified error";
    case kAooErrorNone:
        return "no error";
    case kAooErrorNotImplemented:
        return "not implemented";
    case kAooErrorIdle:
        return "idle";
    case kAooErrorOutOfMemory:
        return "out of memory";
    default:
        return "unknown error code";
    }
}

//----------------------- OSC --------------------------//

AooError AOO_CALL aoo_parsePattern(
        const AooByte *msg, AooInt32 size,
        AooMsgType *type, AooId *id, AooInt32 *offset)
{
    int32_t count = 0;
    if (size >= kAooBinMsgHeaderSize &&
        !memcmp(msg, kAooBinMsgDomain, kAooBinMsgDomainSize))
    {
        // domain (int32), type (int16), cmd (int16), id (int32) ...
        *type = aoo::from_bytes<int16_t>(msg + 4);
        // cmd = aoo::from_bytes<int16_t>(msg + 6);
        *id = aoo::from_bytes<int32_t>(msg + 8);
        *offset = 12;

        return kAooOk;
    } else if (size >= kAooMsgDomainLen
        && !memcmp(msg, kAooMsgDomain, kAooMsgDomainLen))
    {
        count += kAooMsgDomainLen;
        if (size >= (count + kAooMsgSourceLen)
            && !memcmp(msg + count, kAooMsgSource, kAooMsgSourceLen))
        {
            *type = kAooTypeSource;
            count += kAooMsgSourceLen;
        } else if (size >= (count + kAooMsgSinkLen)
            && !memcmp(msg + count, kAooMsgSink, kAooMsgSinkLen))
        {
            *type = kAooTypeSink;
            count += kAooMsgSinkLen;
        } else {
        #if USE_AOO_NET
            if (size >= (count + kAooNetMsgClientLen)
                && !memcmp(msg + count, kAooNetMsgClient, kAooNetMsgClientLen))
            {
                *type = kAooTypeClient;
                count += kAooNetMsgClientLen;
            } else if (size >= (count + kAooNetMsgServerLen)
                && !memcmp(msg + count, kAooNetMsgServer, kAooNetMsgServerLen))
            {
                *type = kAooTypeServer;
                count += kAooNetMsgServerLen;
            } else if (size >= (count + kAooNetMsgPeerLen)
                && !memcmp(msg + count, kAooNetMsgPeer, kAooNetMsgPeerLen))
            {
                *type = kAooTypePeer;
                count += kAooNetMsgPeerLen;
            } else if (size >= (count + kAooNetMsgRelayLen)
                && !memcmp(msg + count, kAooNetMsgRelay, kAooNetMsgRelayLen))
            {
                *type = kAooTypeRelay;
                count += kAooNetMsgRelayLen;
            } else {
                return kAooErrorUnknown;
            }

            if (offset){
                *offset = count;
            }
        #endif // USE_AOO_NET

            return kAooOk;
        }

        // /aoo/source or /aoo/sink
        if (id){
            int32_t skip = 0;
            if (sscanf((const char *)(msg + count), "/%d%n", id, &skip) > 0){
                count += skip;
            } else {
                // TODO only print relevant part of OSC address string
                LOG_ERROR("aoo_parse_pattern: bad ID " << (msg + count));
                return kAooErrorUnknown;
            }
        } else {
            return kAooErrorUnknown;
        }

        if (offset){
            *offset = count;
        }
        return kAooOk;
    } else {
        return kAooErrorUnknown; // not an AOO message
    }
}

//-------------------- NTP time ----------------------------//

uint64_t AOO_CALL aoo_getCurrentNtpTime(void){
    return aoo::time_tag::now();
}

double AOO_CALL aoo_osctime_to_seconds(uint64_t t){
    return aoo::time_tag(t).to_seconds();
}

uint64_t AOO_CALL aoo_osctime_from_seconds(double s){
    return aoo::time_tag::from_seconds(s);
}

double AOO_CALL aoo_ntpTimeDuration(uint64_t t1, uint64_t t2){
    return aoo::time_tag::duration(t1, t2);
}

//---------------------- version -------------------------//

void aoo_getVersion(int32_t *major, int32_t *minor,
                    int32_t *patch, int32_t *test){
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

namespace aoo {

bool check_version(uint32_t version){
    auto major = (version >> 24) & 255;
#if 0
    auto minor = (version >> 16) & 255;
    auto bugfix = (version >> 8) & 255;
#endif

    if (major != kAooVersionMajor){
        return false;
    }

    return true;
}

uint32_t make_version(){
    // make version: major, minor, bugfix, [protocol]
    return ((uint32_t)kAooVersionMajor << 24)
            | ((uint32_t)kAooVersionMinor << 16)
            | ((uint32_t)kAooVersionPatch << 8);
}

//---------------------- memory -----------------------------//

#define DEBUG_MEMORY_LIST 0

memory_list::block * memory_list::block::alloc(size_t size){
    auto fullsize = sizeof(block::header) + size;
    auto b = (block *)aoo::allocate(fullsize);
    b->header.next = nullptr;
    b->header.size = size;
#if DEBUG_MEMORY_LIST
    LOG_ALL("allocate memory block (" << size << " bytes)");
#endif
    return b;
}

void memory_list::block::free(memory_list::block *b){
#if DEBUG_MEMORY_LIST
    LOG_ALL("deallocate memory block (" << b->header.size << " bytes)");
#endif
    auto fullsize = sizeof(block::header) + b->header.size;
    aoo::deallocate(b, fullsize);
}

memory_list::~memory_list(){
    // free memory blocks
    auto b = list_.load(std::memory_order_relaxed);
    while (b){
        auto next = b->header.next;
        block::free(b);
        b = next;
    }
}

void* memory_list::allocate(size_t size) {
    for (;;){
        // try to pop existing block
        auto head = list_.load(std::memory_order_relaxed);
        if (head){
            auto next = head->header.next;
            if (list_.compare_exchange_weak(head, next, std::memory_order_acq_rel)){
                if (head->header.size >= size){
                #if DEBUG_MEMORY_LIST
                    LOG_ALL("reuse memory block (" << head->header.size << " bytes)");
                #endif
                    return head->data;
                } else {
                    // free block
                    block::free(head);
                }
            } else {
                // try again
                continue;
            }
        }
        // allocate new block
        return block::alloc(size)->data;
    }
}
void memory_list::deallocate(void* ptr) {
    auto b = block::from_bytes(ptr);
    b->header.next = list_.load(std::memory_order_relaxed);
    // check if the head has changed and update it atomically.
    // (if the CAS fails, 'next' is updated to the current head)
    while (!list_.compare_exchange_weak(b->header.next, b, std::memory_order_acq_rel)) ;
#if DEBUG_MEMORY_LIST
    LOG_ALL("return memory block (" << b->header.size << " bytes)");
#endif
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

AooError AOO_CALL aoo_registerCodec(const char *name, const AooCodecInterface *codec){
    if (aoo::find_codec(name)) {
        LOG_WARNING("codec " << name << " already registered!");
        return kAooErrorUnknown;
    }
    aoo::g_codec_list.emplace_back(name, codec);
    LOG_VERBOSE("registered codec '" << name << "'");
    return kAooOk;
}

//--------------------------- (de)initialize -----------------------------------//

void aoo_pcmLoad(AooCodecRegisterFunc fn, AooLogFunc log, AooAllocFunc alloc);
void aoo_pcmUnload();
#if USE_CODEC_OPUS
void aoo_opusLoad(AooCodecRegisterFunc fn, AooLogFunc log, AooAllocFunc alloc);
void aoo_opusUnload();
#endif

#if AOO_CUSTOM_ALLOCATOR || AOO_DEBUG_MEMORY
#define ALLOCATOR aoo::g_allocator
#else
#define ALLOCATOR nullptr
#endif

void AOO_CALL aoo_initialize(){
    static bool initialized = false;
    if (!initialized){
    #if USE_AOO_NET
        aoo::socket_init();
    #endif

        // register codecs
        aoo_pcmLoad(aoo_registerCodec, aoo::g_logfunction, ALLOCATOR);

    #if USE_CODEC_OPUS
        aoo_opusLoad(aoo_registerCodec, aoo::g_logfunction, ALLOCATOR);
    #endif

        initialized = true;
    }
}

void AOO_CALL aoo_initializeEx(AooLogFunc log, AooAllocFunc alloc) {
    if (log) {
        aoo::g_logfunction = log;
    }
    if (alloc) {
#if AOO_CUSTOM_ALLOCATOR
        aoo::g_allocator = *alloc;
#else
        LOG_WARNING("aoo_initializeEx: custom allocator not supported");
#endif
    }
    aoo_initialize();
}

void AOO_CALL aoo_terminate() {
    // unload codecs
    aoo_pcmUnload();
#if USE_CODEC_OPUS
    aoo_opusUnload();
#endif
    // free codec pluginlist
    aoo::codec_list tmp;
    std::swap(tmp, aoo::g_codec_list);
}
