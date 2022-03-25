#pragma once

#include "aoo/aoo.h"
#if USE_AOO_NET
# include "aoo/aoo_net.h"
#endif
#include "aoo/aoo_codec.h"
#include "aoo/aoo_events.h"

#include "common/net_utils.hpp"
#include "common/lockfree.hpp"
#include "common/sync.hpp"
#include "common/time.hpp"
#include "common/utils.hpp"

#include "oscpack/osc/OscReceivedElements.h"
#include "oscpack/osc/OscOutboundPacketStream.h"

#include <stdint.h>
#include <cstring>
#include <utility>
#include <memory>
#include <atomic>
#include <vector>
#include <string>
#include <type_traits>

namespace aoo {

template<typename T>
using parameter = sync::relaxed_atomic<T>;

//------------------ OSC ---------------------------//

osc::OutboundPacketStream& operator<<(osc::OutboundPacketStream& msg, const ip_address& addr);

ip_address osc_read_address(osc::ReceivedMessageArgumentIterator& it, ip_address::ip_type type = ip_address::Unspec);

//---------------- codec ---------------------------//

const struct AooCodecInterface * find_codec(const char * name);

//--------------- helper functions ----------------//

AooId get_random_id();

uint32_t make_version();

bool check_version(uint32_t version);

#if USE_AOO_NET
namespace net {

AooError parse_pattern(const AooByte *msg, int32_t n,
                       AooMsgType& type, int32_t& offset);

AooSize write_relay_message(AooByte *buffer, AooSize bufsize,
                            const AooByte *msg, AooSize msgsize,
                            const ip_address& addr);

std::string encrypt(const std::string& input);

struct ip_host {
    ip_host() = default;
    ip_host(const std::string& _name, int _port)
        : name(_name), port(_port) {}
    ip_host(const AooIpEndpoint& ep)
        : name(ep.hostName), port(ep.port) {}

    std::string name;
    int port = 0;

    bool valid() const {
        return !name.empty() && port > 0;
    }

    bool operator==(const ip_host& other) const {
        return name == other.name && port == other.port;
    }

    bool operator!=(const ip_host& other) const {
        return !operator==(other);
    }
};

osc::OutboundPacketStream& operator<<(osc::OutboundPacketStream& msg, const ip_host& addr);

ip_host osc_read_host(osc::ReceivedMessageArgumentIterator& it);


//-------------------------- ievent --------------------------------//

struct event_handler {
    event_handler(AooEventHandler fn, void *user, AooThreadLevel level)
        : fn_(fn), user_(user), level_(level) {}

    template<typename T>
    void operator()(const T& event) const {
        fn_(user_, &reinterpret_cast<const AooEvent&>(event), level_);
    }
private:
    AooEventHandler fn_;
    void *user_;
    AooThreadLevel level_;
};

struct ievent {
    virtual ~ievent() {}

    virtual void dispatch(const event_handler& fn) const = 0;
};

using event_ptr = std::unique_ptr<ievent>;

struct net_error_event : ievent
{
    net_error_event(int32_t code, std::string msg)
        : code_(code), msg_(std::move(msg)) {}

    void dispatch(const event_handler& fn) const override {
        AooNetEventError e;
        e.type = kAooNetEventError;
        e.errorCode = code_;
        e.errorMessage = msg_.c_str();

        fn(e);
    }

    int32_t code_;
    std::string msg_;
};

} // net
#endif // USE_AOO_NET

//-------------------- sendfn-------------------------//

struct sendfn {
    sendfn(AooSendFunc fn = nullptr, void *user = nullptr)
        : fn_(fn), user_(user) {}

    void operator() (const AooByte *data, AooInt32 size,
                     const ip_address& addr, AooFlag flags = 0) const {
        fn_(user_, data, size, addr.address(), addr.length(), flags);
    }

    AooSendFunc fn() const { return fn_; }

    void * user() const { return user_; }
private:
    AooSendFunc fn_;
    void *user_;
};

//---------------- endpoint ------------------------//

struct endpoint {
    endpoint() = default;
    endpoint(const ip_address& _address, int32_t _id)
        : address(_address), id(_id) {}
#if USE_AOO_NET
    endpoint(const ip_address& _address, int32_t _id, const ip_address& _relay)
        : address(_address), relay(_relay), id(_id) {}
#endif
    // data
    ip_address address;
#if USE_AOO_NET
    ip_address relay;
#endif
    AooId id = 0;

    void send(const osc::OutboundPacketStream& msg, const sendfn& fn) const {
        send((const AooByte *)msg.Data(), msg.Size(), fn);
    }
#if USE_AOO_NET
    void send(const AooByte *data, AooSize size, const sendfn& fn) const;
#else
    void send(const AooByte *data, AooSize size, const sendfn& fn) const {
        fn(data, size, address, 0);
    }
#endif
};

inline std::ostream& operator<<(std::ostream& os, const endpoint& ep){
    os << ep.address << "|" << ep.id;
    return os;
}

#if USE_AOO_NET
inline void endpoint::send(const AooByte *data, AooSize size, const sendfn& fn) const {
    if (relay.valid()) {
    #if AOO_DEBUG_RELAY
        LOG_DEBUG("relay message to " << *this << " via " << relay);
    #endif
        AooByte buffer[AOO_MAX_PACKET_SIZE];
        auto result = net::write_relay_message(buffer, sizeof(buffer),
                                               data, size, address);
        if (result > 0) {
            fn(buffer, result, relay, 0);
        } else {
            LOG_ERROR("can't relay binary message: buffer too small");
        }
    } else {
        fn(data, size, address, 0);
    }
}
#endif

//---------------- endpoint event ------------------//

// we keep the union in a seperate base class, so that we
// can use the default copy constructor and assignment.
struct endpoint_event_union
{
    endpoint_event_union() = default;
    endpoint_event_union(AooEventType _type)
        : type(_type) {}
    union {
        AooEventType type;
        AooEvent event;
        AooEventEndpoint ep;
        AooEventEndpoint source;
        AooEventEndpoint sink;
        AooEventInvite invite;
        AooEventUninvite uninvite;
        AooEventPing ping;
        AooEventPingReply ping_reply;
        AooEventXRun xrun;
    };
};

struct endpoint_event : endpoint_event_union {
    endpoint_event() = default;

    endpoint_event(AooEventType _type) : endpoint_event_union(_type) {}

    endpoint_event(AooEventType _type, const endpoint& _ep)
        : endpoint_event(_type, _ep.address, _ep.id) {}

    endpoint_event(AooEventType _type, const ip_address& addr, AooId id)
        : endpoint_event_union(_type) {
        // only for endpoint events
        if (type != kAooEventXRun) {
            memcpy(&addr_, addr.address(), addr.length());
            ep.endpoint.address = &addr_;
            ep.endpoint.addrlen = addr.length();
            ep.endpoint.id = id;
        }
    }

    endpoint_event(const endpoint_event& other)
        : endpoint_event_union(other) {
        // only for sink events:
        if (type != kAooEventXRun) {
            memcpy(&addr_, other.addr_, sizeof(addr_));
            ep.endpoint.address = &addr_;
        }
    }

    endpoint_event& operator=(const endpoint_event& other) {
        endpoint_event_union::operator=(other);
        // only for sink events:
        if (type != kAooEventXRun) {
            memcpy(&addr_, other.addr_, sizeof(addr_));
            ep.endpoint.address = &addr_;
        }
        return *this;
    }
private:
    char addr_[ip_address::max_length];
};

//---------------- allocator -----------------------//

#if AOO_CUSTOM_ALLOCATOR || AOO_DEBUG_MEMORY

void * allocate(size_t size);

template<class T, class... U>
T * construct(U&&... args){
    auto ptr = allocate(sizeof(T));
    return new (ptr) T(std::forward<U>(args)...);
}

void deallocate(void *ptr, size_t size);

template<typename T>
void destroy(T *x){
    if (x) {
        x->~T();
        deallocate(x, sizeof(T));
    }
}

template<class T>
class allocator {
public:
    using value_type = T;

    allocator() noexcept = default;

    template<typename U>
    allocator(const allocator<U>&) noexcept {}

    template<typename U>
    allocator& operator=(const allocator<U>&) noexcept {}

    template<class U>
    struct rebind {
        typedef allocator<U> other;
    };

    value_type* allocate(size_t n) {
        return (value_type *)aoo::allocate(sizeof(T) * n);
    }

    void deallocate(value_type* p, size_t n) noexcept {
        aoo::deallocate(p, sizeof(T) * n);
    }
};

template <class T, class U>
bool operator==(allocator<T> const&, allocator<U> const&) noexcept
{
    return true;
}

template <class T, class U>
bool operator!=(allocator<T> const& x, allocator<U> const& y) noexcept
{
    return !(x == y);
}

#else

inline void * allocate(size_t size){
    return operator new(size);
}

template<class T, class... U>
T * construct(U&&... args){
    return new T(std::forward<U>(args)...);
}

inline void deallocate(void *ptr, size_t size){
    operator delete(ptr);
}

template<typename T>
void destroy(T *x){
    delete x;
}

template<typename T>
using allocator = std::allocator<T>;

#endif

//------------- common data structures ------------//

template<typename T>
using vector = std::vector<T, aoo::allocator<T>>;

using string = std::basic_string<char, std::char_traits<char>, aoo::allocator<char>>;

template<typename T>
using spsc_queue = lockfree::spsc_queue<T, aoo::allocator<T>>;

template<typename T>
using unbounded_mpsc_queue = lockfree::unbounded_mpsc_queue<T, aoo::allocator<T>>;

template<typename T>
using rcu_list = lockfree::rcu_list<T, aoo::allocator<T>>;

//------------------ memory --------------------//

class memory_list {
public:
    memory_list() = default;
    ~memory_list();
    memory_list(memory_list&& other)
        : list_(other.list_.exchange(nullptr)){}
    memory_list& operator=(memory_list&& other){
        list_.store(other.list_.exchange(nullptr));
        return *this;
    }
    void* allocate(size_t size);

    template<typename T, typename... Args>
    T* construct(Args&&... args){
        auto mem = allocate(sizeof(T));
        return new (mem) T (std::forward<Args>(args)...);
    }

    void deallocate(void* b);

    template<typename T>
    void destroy(T *obj){
        obj->~T();
        deallocate(obj);
    }
private:
    struct block {
        struct {
            block *next;
            size_t size;
        } header;
        char data[1];

        static block * alloc(size_t size);

        static void free(block *mem);

        static block * from_bytes(void *bytes){
            return (block *)((char *)bytes - sizeof(block::header));
        }
    };
    std::atomic<block *> list_{nullptr};
};

//------------------- misc -------------------------//

struct format_deleter {
    void operator() (void *x) const {
        auto f = static_cast<AooFormat *>(x);
        aoo::deallocate(x, f->size);
    }
};

struct encoder_deleter {
    void operator() (void *x) const {
        auto c = (AooCodec *)x;
        c->cls->encoderFree(c);
    }
};

struct decoder_deleter {
    void operator() (void *x) const {
        auto c = (AooCodec *)x;
        c->cls->decoderFree(c);
    }
};

struct metadata {
    metadata() = default;
    metadata(const AooDataView* md) {
        if (md) {
            type_ = md->type;
            data_.assign(md->data, md->data + md->size);
        }
    }
    const char *type() const { return type_.c_str(); }
    const AooByte *data() const { return data_.data(); }
    AooSize size() const { return data_.size(); }
private:
    std::string type_;
    std::vector<AooByte> data_;
};

// HACK: declare the AooDataView overload in "net" namespace and then import into "aoo"
// namespace to prevent the compiler from picking OutboundPacketStream::operator<<bool
namespace net {
osc::OutboundPacketStream& operator<<(osc::OutboundPacketStream& msg, const AooDataView *md);
} // net
osc::OutboundPacketStream& net::operator<<(osc::OutboundPacketStream& msg, const AooDataView *md);

osc::OutboundPacketStream& operator<<(osc::OutboundPacketStream& msg, const aoo::metadata& md);

AooDataView osc_read_metadata(osc::ReceivedMessageArgumentIterator& it);

inline AooSize flat_metadata_maxsize(int32_t size) {
    return sizeof(AooDataView) + size + kAooDataTypeMaxLen + 1;
}

inline AooSize flat_metadata_size(const AooDataView& data){
    return sizeof(data) + data.size + strlen(data.type) + 1;
}

struct flat_metadata_deleter {
    void operator() (void *x) const {
        auto md = static_cast<AooDataView *>(x);
        auto mdsize = flat_metadata_size(*md);
        aoo::deallocate(x, mdsize);
    }
};

inline void flat_metadata_copy(const AooDataView& src, AooDataView& dst) {
    auto data = (AooByte *)(&dst) + sizeof(AooDataView);
    memcpy(data, src.data, src.size);

    auto type = (AooChar *)(data + src.size);
    memcpy(type, src.type, strlen(src.type) + 1);

    dst.type = type;
    dst.data = data;
    dst.size = src.size;
}

} // aoo
