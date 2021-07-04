#include "aoo/aoo.h"
#if USE_AOO_NET
#include "aoo/aoo_net.h"
#include "common/net_utils.hpp"
#endif

#include "imp.hpp"
#include "codec.hpp"

#include "common/sync.hpp"
#include "common/time.hpp"
#include "common/utils.hpp"

#include "aoo/codec/aoo_pcm.h"
#if USE_CODEC_OPUS
#include "aoo/codec/aoo_opus.h"
#endif

#include <iostream>
#include <sstream>
#include <atomic>
#include <unordered_map>

namespace aoo {

/*//////////////////// helper /////////////////////////*/

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

} // aoo

/*//////////////////// allocator /////////////////////*/

#if AOO_CUSTOM_ALLOCATOR || AOO_DEBUG_MEMORY

namespace aoo {

#if AOO_DEBUG_MEMORY
std::atomic<int64_t> total_memory{0};
#endif

static aoo_allocator g_allocator {
    [](size_t n, void *){
    #if AOO_DEBUG_MEMORY
        auto total = total_memory.fetch_add(n, std::memory_order_relaxed) + n;
        fprintf(stderr, "allocate %d bytes (total: %d)\n", n, total);
        fflush(stderr);
    #endif
        return operator new(n);
    },
    nullptr,
    [](void *ptr, size_t n, void *){
    #if AOO_DEBUG_MEMORY
        auto total = total_memory.fetch_sub(n, std::memory_order_relaxed) - n;
        fprintf(stderr, "deallocate %d bytes (total: %d)\n", n, total);
        fflush(stderr);
    #endif
        operator delete(ptr);
    },
    nullptr
};

void * allocate(size_t size){
    return g_allocator.alloc(size, g_allocator.context);
}

void deallocate(void *ptr, size_t size){
    g_allocator.free(ptr, size, g_allocator.context);
}

} // aoo

#endif

#if AOO_CUSTOM_ALLOCATOR
void aoo_set_allocator(const aoo_allocator *alloc){
    aoo::g_allocator = *alloc;
}
#endif

/*//////////////////// Log ////////////////////////////*/

#define LOG_MUTEX 1

#if LOG_MUTEX
static aoo::sync::mutex g_log_mutex;
#endif

static void cerr_logfunction(const char *msg, int32_t level, void *ctx){
#if LOG_MUTEX
    aoo::sync::scoped_lock<aoo::sync::mutex> lock(g_log_mutex);
#endif
    std::cerr << msg;
    std::flush(std::cerr);
}

static aoo_logfunction g_logfunction = cerr_logfunction;
static void *g_logcontext = nullptr;

void aoo_set_logfunction(aoo_logfunction f, void *context){
    g_logfunction = f;
    g_logcontext = context;
}

static const char *errmsg[] = {
    // TODO
    "undefined"
};

const char *aoo_error_string(aoo_error e){
    if (e == AOO_OK){
        return "no error";
    } else {
        return "unspecified error"; // TODO
    }
}

namespace aoo {

Log::~Log(){
    stream_ << "\n";
    std::string msg = stream_.str();
    g_logfunction(msg.c_str(), level_, g_logcontext);
}

}

/*//////////////////// OSC ////////////////////////////*/

aoo_error aoo_parse_pattern(const char *msg, int32_t n,
                            aoo_type *type, aoo_id *id, int32_t *offset)
{
    int32_t count = 0;
    if (n >= AOO_BIN_MSG_HEADER_SIZE &&
        !memcmp(msg, AOO_BIN_MSG_DOMAIN, AOO_BIN_MSG_DOMAIN_SIZE))
    {
        // domain (int32), type (int16), cmd (int16), id (int32) ...
        *type = aoo::from_bytes<int16_t>(msg + 4);
        // cmd = aoo::from_bytes<int16_t>(msg + 6);
        *id = aoo::from_bytes<int32_t>(msg + 8);
        *offset = 12;

        return AOO_OK;
    } else if (n >= AOO_MSG_DOMAIN_LEN
        && !memcmp(msg, AOO_MSG_DOMAIN, AOO_MSG_DOMAIN_LEN))
    {
        count += AOO_MSG_DOMAIN_LEN;
        if (n >= (count + AOO_MSG_SOURCE_LEN)
            && !memcmp(msg + count, AOO_MSG_SOURCE, AOO_MSG_SOURCE_LEN))
        {
            *type = AOO_TYPE_SOURCE;
            count += AOO_MSG_SOURCE_LEN;
        } else if (n >= (count + AOO_MSG_SINK_LEN)
            && !memcmp(msg + count, AOO_MSG_SINK, AOO_MSG_SINK_LEN))
        {
            *type = AOO_TYPE_SINK;
            count += AOO_MSG_SINK_LEN;
        } else {
        #if USE_AOO_NET
            if (n >= (count + AOO_NET_MSG_CLIENT_LEN)
                && !memcmp(msg + count, AOO_NET_MSG_CLIENT, AOO_NET_MSG_CLIENT_LEN))
            {
                *type = AOO_TYPE_CLIENT;
                count += AOO_NET_MSG_CLIENT_LEN;
            } else if (n >= (count + AOO_NET_MSG_SERVER_LEN)
                && !memcmp(msg + count, AOO_NET_MSG_SERVER, AOO_NET_MSG_SERVER_LEN))
            {
                *type = AOO_TYPE_SERVER;
                count += AOO_NET_MSG_SERVER_LEN;
            } else if (n >= (count + AOO_NET_MSG_PEER_LEN)
                && !memcmp(msg + count, AOO_NET_MSG_PEER, AOO_NET_MSG_PEER_LEN))
            {
                *type = AOO_TYPE_PEER;
                count += AOO_NET_MSG_PEER_LEN;
            } else if (n >= (count + AOO_NET_MSG_RELAY_LEN)
                && !memcmp(msg + count, AOO_NET_MSG_RELAY, AOO_NET_MSG_RELAY_LEN))
            {
                *type = AOO_TYPE_RELAY;
                count += AOO_NET_MSG_RELAY_LEN;
            } else {
                return AOO_ERROR_UNSPECIFIED;
            }

            if (offset){
                *offset = count;
            }
        #endif // USE_AOO_NET

            return AOO_OK;
        }

        // /aoo/source or /aoo/sink
        if (id){
            int32_t skip = 0;
            if (sscanf(msg + count, "/%d%n", id, &skip) > 0){
                count += skip;
            } else {
                // TODO only print relevant part of OSC address string
                LOG_ERROR("aoo_parse_pattern: bad ID " << (msg + count));
                return AOO_ERROR_UNSPECIFIED;
            }
        } else {
            return AOO_ERROR_UNSPECIFIED;
        }

        if (offset){
            *offset = count;
        }
        return AOO_OK;
    } else {
        return AOO_ERROR_UNSPECIFIED; // not an AoO message
    }
}

// OSC time stamp (NTP time)
uint64_t aoo_osctime_now(void){
    return aoo::time_tag::now();
}

double aoo_osctime_to_seconds(uint64_t t){
    return aoo::time_tag(t).to_seconds();
}

uint64_t aoo_osctime_from_seconds(double s){
    return aoo::time_tag::from_seconds(s);
}

double aoo_osctime_duration(uint64_t t1, uint64_t t2){
    return aoo::time_tag::duration(t1, t2);
}

/*/////////////// version ////////////////////*/

void aoo_version(int32_t *major, int32_t *minor,
                 int32_t *patch, int32_t *pre){
    if (major) *major = AOO_VERSION_MAJOR;
    if (minor) *minor = AOO_VERSION_MINOR;
    if (patch) *patch = AOO_VERSION_PATCH;
    if (pre) *pre = AOO_VERSION_PRERELEASE;
}

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

const char *aoo_version_string(){
    return STR(AOO_VERSION_MAJOR) "." STR(AOO_VERSION_MINOR)
    #if AOO_VERSION_PATCH > 0
        "." STR(AOO_VERSION_PATCH)
    #endif
    #if AOO_VERSION_PRERELEASE > 0
       "-pre" STR(AOO_VERSION_PRERELEASE)
    #endif
        ;
}

namespace aoo {

bool check_version(uint32_t version){
    auto major = (version >> 24) & 255;
    auto minor = (version >> 16) & 255;
    auto bugfix = (version >> 8) & 255;

    if (major != AOO_VERSION_MAJOR){
        return false;
    }

    return true;
}

uint32_t make_version(){
    // make version: major, minor, bugfix, [protocol]
    return ((uint32_t)AOO_VERSION_MAJOR << 24) | ((uint32_t)AOO_VERSION_MINOR << 16)
            | ((uint32_t)AOO_VERSION_PATCH << 8);
}

/*//////////////////// memory /////////////////*/

memory_block * memory_block::allocate(size_t size){
    auto fullsize = sizeof(memory_block::header) + size;
    auto mem = (memory_block *)aoo::allocate(fullsize);
    mem->header.next = nullptr;
    mem->header.size = size;
#if DEBUG_MEMORY
    fprintf(stderr, "allocate memory block (%d bytes)\n", size);
    fflush(stderr);
#endif
    return mem;
}

void memory_block::free(memory_block *mem){
#if DEBUG_MEMORY
    fprintf(stderr, "deallocate memory block (%d bytes)\n", mem->size());
    fflush(stderr);
#endif
    aoo::deallocate(mem, mem->full_size());
}

memory_list::~memory_list(){
    // free memory blocks
    auto mem = memlist_.load(std::memory_order_relaxed);
    while (mem){
        auto next = mem->header.next;
        memory_block::free(mem);
        mem = next;
    }
}

memory_block* memory_list::alloc(size_t size) {
    for (;;){
        // try to pop existing block
        auto head = memlist_.load(std::memory_order_relaxed);
        if (head){
            auto next = head->header.next;
            if (memlist_.compare_exchange_weak(head, next, std::memory_order_acq_rel)){
                if (head->header.size >= size){
                #if DEBUG_MEMORY
                    fprintf(stderr, "reuse memory block (%d bytes)\n", head->header.size);
                    fflush(stderr);
                #endif
                    return head;
                } else {
                    // free block
                    memory_block::free(head);
                }
            } else {
                // try again
                continue;
            }
        }
        // allocate new block
        return memory_block::allocate(size);
    }
}
void memory_list::free(memory_block* b) {
    b->header.next = memlist_.load(std::memory_order_relaxed);
    // check if the head has changed and update it atomically.
    // (if the CAS fails, 'next' is updated to the current head)
    while (!memlist_.compare_exchange_weak(b->header.next, b, std::memory_order_acq_rel)) ;
#if DEBUG_MEMORY
    fprintf(stderr, "return memory block (%d bytes)\n", b->header.size);
    fflush(stderr);
#endif
}

} // aoo

/*//////////////////// codec //////////////////*/


namespace aoo {

static std::unordered_map<std::string, std::unique_ptr<aoo::codec>> g_codec_dict;

const aoo::codec * find_codec(const char * name){
    auto it = g_codec_dict.find(name);
    if (it != g_codec_dict.end()){
        return it->second.get();
    } else {
        return nullptr;
    }
}

} // aoo

aoo_error aoo_register_codec(const char *name, const aoo_codec *codec){
    if (aoo::g_codec_dict.count(name) != 0){
        LOG_WARNING("aoo: codec " << name << " already registered!");
        return AOO_ERROR_UNSPECIFIED;
    }
    aoo::g_codec_dict[name] = std::make_unique<aoo::codec>(codec);
    LOG_VERBOSE("aoo: registered codec '" << name << "'");
    return AOO_OK;
}

/*/////////////// (de)initialize //////////////////*/

void aoo_codec_pcm_setup(aoo_codec_registerfn fn, const aoo_allocator *alloc);
#if USE_CODEC_OPUS
void aoo_codec_opus_setup(aoo_codec_registerfn fn, const aoo_allocator *alloc);
#endif

#if AOO_CUSTOM_ALLOCATOR || AOO_DEBUG_MEMORY
#define ALLOCATOR &aoo::g_allocator
#else
#define ALLOCATOR nullptr
#endif

void aoo_initialize(){
    static bool initialized = false;
    if (!initialized){
    #if USE_AOO_NET
        aoo::socket_init();
    #endif

        // register codecs
        aoo_codec_pcm_setup(aoo_register_codec, ALLOCATOR);

    #if USE_CODEC_OPUS
        aoo_codec_opus_setup(aoo_register_codec, ALLOCATOR);
    #endif

        initialized = true;
    }
}

void aoo_terminate() {}


