#include "aoo/aoo.h"
#if USE_AOO_NET
# include "aoo/aoo_net.h"
# include "common/net_utils.hpp"
# include "md5/md5.h"
#endif
#include "aoo/aoo_codec.h"

#include "binmsg.hpp"
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

//--------------------- interface table -------------------//

void * AOO_CALL def_allocator(void *ptr, AooSize oldsize, AooSize newsize);

#ifndef _MSC_VER
void __attribute__((format(printf, 2, 3 )))
#else
void
#endif
    def_logfunc(AooLogLevel, const char *, ...);

// populate interface table with default implementations
static AooCodecHostInterface g_interface = {
    sizeof(AooCodecHostInterface),
    aoo_registerCodec,
    def_allocator,
    def_logfunc
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

#if USE_AOO_NET
namespace net {

std::string encrypt(const std::string& input) {
    // pass empty password unchanged
    if (input.empty()) {
        return "";
    }

    uint8_t result[16];
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, (uint8_t *)input.data(), input.size());
    MD5_Final(result, &ctx);

    char output[33];
    for (int i = 0; i < 16; ++i){
        snprintf(&output[i * 2], 3, "%02X", result[i]);
    }

    return output;
}

// optimized version of aoo_parse_pattern() for client/server
AooError parse_pattern(const AooByte *msg, int32_t n, AooMsgType& type, int32_t& offset)
{
    int32_t count = 0;
    if (aoo::binmsg_check(msg, n))
    {
        type = aoo::binmsg_type(msg, n);
        offset = aoo::binmsg_headersize(msg, n);
        return kAooOk;
    } else if (n >= kAooMsgDomainLen
            && !memcmp(msg, kAooMsgDomain, kAooMsgDomainLen))
    {
        count += kAooMsgDomainLen;
        if (n >= (count + kAooNetMsgServerLen)
            && !memcmp(msg + count, kAooNetMsgServer, kAooNetMsgServerLen))
        {
            type = kAooTypeServer;
            count += kAooNetMsgServerLen;
        }
        else if (n >= (count + kAooNetMsgClientLen)
            && !memcmp(msg + count, kAooNetMsgClient, kAooNetMsgClientLen))
        {
            type = kAooTypeClient;
            count += kAooNetMsgClientLen;
        }
        else if (n >= (count + kAooNetMsgPeerLen)
            && !memcmp(msg + count, kAooNetMsgPeer, kAooNetMsgPeerLen))
        {
            type = kAooTypePeer;
            count += kAooNetMsgPeerLen;
        }
        else if (n >= (count + kAooNetMsgRelayLen)
            && !memcmp(msg + count, kAooNetMsgRelay, kAooNetMsgRelayLen))
        {
            type = kAooTypeRelay;
            count += kAooNetMsgRelayLen;
        } else {
            return kAooErrorUnknown;
        }

        offset = count;

        return kAooOk;
    } else {
        return kAooErrorUnknown; // not an AOO message
    }
}

AooSize write_relay_message(AooByte *buffer, AooSize bufsize,
                            const AooByte *msg, AooSize msgsize,
                            const ip_address& addr) {
    if (aoo::binmsg_check(msg, msgsize)) {
    #if AOO_DEBUG_RELAY && 0
        LOG_DEBUG("write binary relay message");
    #endif
        auto onset = aoo::binmsg_write_relay(buffer, bufsize, addr);
        auto total = msgsize + onset;
        // NB: we do not write the message size itself because it is implicit
        if (bufsize >= total) {
            memcpy(buffer + onset, msg, msgsize);
            return total;
        } else {
            return 0;
        }
    } else {
    #if AOO_DEBUG_RELAY && 0
        LOG_DEBUG("write OSC relay message " << msg);
    #endif
        osc::OutboundPacketStream msg2((char *)buffer, bufsize);
        msg2 << osc::BeginMessage(kAooMsgDomain kAooNetMsgRelay)
             << addr << osc::Blob(msg, msgsize)
             << osc::EndMessage;
        return msg2.Size();
    }
}

} // net
#endif // USE_AOO_NET

//------------------------------ OSC utilities ---------------------------------//

#if USE_AOO_NET
// see comment in "imp.hpp"
namespace net {
osc::OutboundPacketStream& operator<<(osc::OutboundPacketStream& msg, const AooDataView *md) {
    if (md) {
        msg << md->type << osc::Blob(md->data, md->size);
    } else {
        msg << "" << osc::Blob(msg.Data(), 0); // HACK: do not use nullptr because of memcpy()
    }
    return msg;
}
} // net
#endif // USE_AOO_NET

osc::OutboundPacketStream& operator<<(osc::OutboundPacketStream& msg, const aoo::metadata& md) {
    if (md.size() > 0) {
        msg << md.type() << osc::Blob(md.data(), md.size());
    } else {
        msg << "" << osc::Blob(msg.Data(), 0); // HACK: do not use nullptr because of memcpy()
    }
    return msg;
}

AooDataView osc_read_metadata(osc::ReceivedMessageArgumentIterator& it) {
    auto type = (it++)->AsString();
    const void *blobdata;
    osc::osc_bundle_element_size_t blobsize;
    (it++)->AsBlob(blobdata, blobsize);
    if (blobsize) {
        return AooDataView { type, (const AooByte *)blobdata, (AooSize)blobsize };
    } else {
        return AooDataView { type, nullptr, 0 };
    }
}

osc::OutboundPacketStream& operator<<(osc::OutboundPacketStream& msg, const ip_address& addr) {
    // send *unmapped* addresses in case the client is IPv4 only
    if (addr.valid()) {
        msg << addr.name_unmapped() << (int32_t)addr.port();
    } else {
        msg << "" << (int32_t)0;
    }
    return msg;
}

ip_address osc_read_address(osc::ReceivedMessageArgumentIterator& it, ip_address::ip_type type) {
    auto host = (it++)->AsString();
    auto port = (it++)->AsInt32();
    return ip_address(host, port, type);
}

#if USE_AOO_NET
namespace net {

osc::OutboundPacketStream& operator<<(osc::OutboundPacketStream& msg, const ip_host& addr) {
    msg << addr.name.c_str() << addr.port;
    return msg;
}

ip_host osc_read_host(osc::ReceivedMessageArgumentIterator& it) {
    auto host = (it++)->AsString();
    auto port = (it++)->AsInt32();
    return net::ip_host { host, port };
}

} // net
#endif // USE_AOO_NET

//------------------- allocator ------------------//

#if AOO_DEBUG_MEMORY
std::atomic<ptrdiff_t> total_memory{0};
#endif

void * AOO_CALL def_allocator(void *ptr, AooSize oldsize, AooSize newsize) {
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
    auto result = g_interface.allocFunc(nullptr, 0, size);
    if (!result && size > 0) {
        throw std::bad_alloc{};
    }
    return result;
}

void deallocate(void *ptr, size_t size){
    g_interface.allocFunc(ptr, size, 0);
}

#endif


//----------------------- logging --------------------------//


#if CERR_LOG_FUNCTION

#if CERR_LOG_MUTEX
static sync::mutex g_log_mutex;
#endif

#ifndef _MSC_VER
void __attribute__((format(printf, 2, 3 )))
#else
void
#endif
    def_logfunc(AooLogLevel level, const char *fmt, ...)
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

#else // CERR_LOG_FUNCTION

#ifndef _MSC_VER
void __attribute__((format(printf, 2, 3 )))
#else
void
#endif
    def_logfunc(AooLogLevel, const char *, ...) {}

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
    if (aoo::g_interface.logFunc) {
        buffer_[pos_] = '\0';
        aoo::g_interface.logFunc(level_, buffer_);
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
    if (aoo::binmsg_check(msg, size))
    {
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

const AooCodecHostInterface * aoo_getCodecHostInterface(void)
{
    return &aoo::g_interface;
}

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

void aoo_pcmLoad(const AooCodecHostInterface *);
void aoo_pcmUnload();
#if USE_CODEC_OPUS
void aoo_opusLoad(const AooCodecHostInterface *);
void aoo_opusUnload();
#endif

#define HAVE_SETTING(settings, field) \
    (settings && (settings->size >= (offsetof(AooSettings, field) + sizeof(settings->field))))

AooError AOO_CALL aoo_initialize(const AooSettings *settings) {
    static bool initialized = false;
    if (!initialized) {
    #if USE_AOO_NET
        aoo::socket_init();
    #endif
        // optional settings
        if (HAVE_SETTING(settings, logFunc) && settings->logFunc) {
            aoo::g_interface.logFunc = settings->logFunc;
        }
        if (HAVE_SETTING(settings, allocFunc) && settings->allocFunc) {
    #if AOO_CUSTOM_ALLOCATOR
            aoo::g_interface.allocFunc = settings->allocFunc;
    #else
            LOG_WARNING("aoo_initializeEx: custom allocator not supported");
    #endif
        }

        // register codecs
        aoo_pcmLoad(&aoo::g_interface);

    #if USE_CODEC_OPUS
        aoo_opusLoad(&aoo::g_interface);
    #endif

        initialized = true;
    }
    return kAooOk;
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
