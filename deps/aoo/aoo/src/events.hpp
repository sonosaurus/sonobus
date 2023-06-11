#pragma once

#include "detail.hpp"

#include "aoo/aoo_events.h"

#include "common/net_utils.hpp"

#include "memory.hpp"

#define AOO_DEBUG_EVENT_MEMORY 0

#if AOO_DEBUG_EVENT_MEMORY
# define LOG_EVENT(x) LOG_DEBUG(x)
#else
# define LOG_EVENT(x)
#endif

namespace aoo {

#define RT_CLASS(x)                     \
    void * operator new (size_t size) { \
        LOG_EVENT("allocate " #x);      \
        return rt_allocate(size);       \
    }                                   \
                                        \
    void operator delete (void *ptr) {  \
        LOG_EVENT("deallocate " #x);    \
        rt_deallocate(ptr, sizeof(x));  \
    }                                   \


struct ievent {
    virtual ~ievent() {}

    AooEvent& cast();
};

using event_ptr = std::unique_ptr<ievent>;

template<typename T, typename... Args>
event_ptr make_event(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T>
struct base_event : ievent, T {
    virtual ~base_event() {}

    base_event(AooEventType type_, size_t size) {
        this->type = type_;
        this->structSize = size;
    }

    base_event(const base_event&) = delete;
    base_event& operator=(const base_event&) = delete;
};

#define BASE_EVENT(name, field) \
    k##name, AOO_STRUCT_SIZE(name, field)

// only for casting
struct cast_event : base_event<AooEventBase> {
    cast_event() = delete;
};

inline AooEvent& ievent::cast() {
    auto ptr = static_cast<AooEventBase *>(static_cast<cast_event *>(this));
    return *reinterpret_cast<AooEvent *>(ptr);
}

template<typename T>
struct endpoint_event : base_event<T> {
    RT_CLASS(endpoint_event)

    endpoint_event(AooEventType type, size_t size, const aoo::endpoint& ep)
        : endpoint_event(type, size, ep.address, ep.id) {}

    endpoint_event(AooEventType type_, size_t size, const ip_address& addr, AooId id)
        : base_event<T>(type_, size) {
        memcpy(&addr_, addr.address(), addr.length());
        this->endpoint.address = &addr_;
        this->endpoint.addrlen = addr.length();
        this->endpoint.id = id;
    }
private:
    char addr_[ip_address::max_length];
};

struct source_event : endpoint_event<AooEventEndpoint> {
    source_event(AooEventType type, const aoo::endpoint& ep)
        : endpoint_event(type, AOO_STRUCT_SIZE(AooEventEndpoint, endpoint), ep) {}
};

struct sink_add_event : endpoint_event<AooEventEndpoint> {
    sink_add_event(const aoo::ip_address& addr, AooId id)
        : endpoint_event(BASE_EVENT(AooEventSinkAdd, endpoint), addr, id) {}
};

struct invite_event : endpoint_event<AooEventInvite> {
    RT_CLASS(invite_event)

    invite_event(const ip_address& addr, AooId id, AooId token, const AooData *md)
        : endpoint_event(BASE_EVENT(AooEventInvite, metadata), addr, id) {
        this->token = token;
        if (md) {
            auto size = flat_metadata_size(*md);
            auto metadata = (AooData *)rt_allocate(size);
            flat_metadata_copy(*md, *metadata);
            this->metadata = metadata;
        } else {
            this->metadata = nullptr;
        }
    }

    ~invite_event() {
        if (this->metadata) {
            auto size = flat_metadata_size(*this->metadata);
            rt_deallocate((void *)this->metadata, size);
        }
    }
};

struct uninvite_event : endpoint_event<AooEventUninvite> {
    RT_CLASS(uninvite_event)

    uninvite_event(const ip_address& addr, AooId id, AooId token)
        : endpoint_event(BASE_EVENT(AooEventUninvite, token), addr, id) {
        this->token = token;
    }
};

struct source_ping_event : endpoint_event<AooEventSourcePing> {
    RT_CLASS(source_ping_event)

    source_ping_event(const aoo::endpoint& ep,
                      aoo::time_tag tt1, aoo::time_tag tt2,
                      aoo::time_tag tt3)
        : endpoint_event(BASE_EVENT(AooEventSourcePing, t3), ep) {
        this->t1 = tt1;
        this->t2 = tt2;
        this->t3 = tt3;
    }
};

struct sink_ping_event : endpoint_event<AooEventSinkPing> {
    RT_CLASS(source_ping_event)

    sink_ping_event(const aoo::endpoint& ep,
                    aoo::time_tag tt1, aoo::time_tag tt2,
                    aoo::time_tag tt3, float packetloss)
        : endpoint_event(BASE_EVENT(AooEventSinkPing, packetLoss), ep) {
        this->t1 = tt1;
        this->t2 = tt2;
        this->t3 = tt3;
        this->packetLoss = packetloss;
    }
};

struct format_change_event : endpoint_event<AooEventFormatChange> {
    RT_CLASS(format_change_event)

    format_change_event(const aoo::endpoint& ep, const AooFormat& fmt)
        : endpoint_event(BASE_EVENT(AooEventFormatChange, format), ep) {
        auto fp = (AooFormat *)rt_allocate(fmt.structSize);
        memcpy(fp, &fmt, fmt.structSize);
        this->format = fp;
    }

    ~format_change_event() {
        rt_deallocate((void *)this->format, this->format->structSize);
    }
};

struct stream_start_event : endpoint_event<AooEventStreamStart> {
    RT_CLASS(stream_start_event)

    stream_start_event(const aoo::endpoint& ep, const AooData *md)
        : endpoint_event(BASE_EVENT(AooEventStreamStart, metadata), ep) {
        this->metadata = md; // metadata is moved!
    }

    ~stream_start_event() {
        if (this->metadata) {
            auto size = flat_metadata_size(*this->metadata);
            rt_deallocate((void *)this->metadata, size);
        }
    }
};

struct stream_stop_event : source_event {
    RT_CLASS(stream_stop_event)

    stream_stop_event(const aoo::endpoint& ep)
        : source_event(kAooEventStreamStop, ep) {}
};

struct stream_state_event : endpoint_event<AooEventStreamState> {
    RT_CLASS(stream_state_event)

    stream_state_event(const aoo::endpoint& ep, AooStreamState state, int32_t offset)
        : endpoint_event(BASE_EVENT(AooEventStreamState, sampleOffset), ep) {
        this->state = state;
        this->sampleOffset = offset;
    }
};

struct block_event : endpoint_event<AooEventBlock> {
    RT_CLASS(block_event)

    block_event(AooEventType type, const aoo::endpoint& ep, int32_t count)
        : endpoint_event(type, AOO_STRUCT_SIZE(AooEventBlock, count), ep) {
        this->count = count;
    }
};

struct block_drop_event : block_event {
    block_drop_event(const aoo::endpoint& ep, int32_t count)
        : block_event(kAooEventBlockDrop, ep, count) {}
};

struct block_resend_event : block_event {
    block_resend_event(const aoo::endpoint& ep, int32_t count)
        : block_event(kAooEventBlockResend, ep, count) {}
};

struct block_xrun_event : block_event {
    block_xrun_event(const aoo::endpoint& ep, int32_t count)
        : block_event(kAooEventBlockXRun, ep, count) {}
};

} // namespace aoo
