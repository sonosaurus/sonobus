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

    base_event(AooEventType type_) {
        this->type = type_;
    }

    base_event(const base_event&) = delete;
    base_event& operator=(const base_event&) = delete;
};

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

    endpoint_event(AooEventType type, const aoo::endpoint& ep)
        : endpoint_event(type, ep.address, ep.id) {}

    endpoint_event(AooEventType type_, const ip_address& addr, AooId id)
        : base_event<T>(type_) {
        memcpy(&addr_, addr.address(), addr.length());
        this->endpoint.address = &addr_;
        this->endpoint.addrlen = addr.length();
        this->endpoint.id = id;
    }
private:
    char addr_[ip_address::max_length];
};

using source_event = endpoint_event<AooEventEndpoint>;
using sink_event = endpoint_event<AooEventEndpoint>;

struct invite_event : endpoint_event<AooEventInvite> {
    RT_CLASS(invite_event)

    invite_event(const ip_address& addr, AooId id, AooId token, const AooData *md)
        : endpoint_event(kAooEventInvite, addr, id) {
        this->token = token;
        this->reserved = 0;
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
        : endpoint_event(kAooEventUninvite, addr, id) {
        this->token = token;
    }
};

struct ping_event : endpoint_event<AooEventPing> {
    RT_CLASS(ping_event)

    // sink ping
    ping_event(const aoo::endpoint& ep,
               aoo::time_tag tt1, aoo::time_tag tt2,
               aoo::time_tag tt3)
        : endpoint_event(kAooEventPing, ep) {
        this->t1 = tt1;
        this->t2 = tt2;
        this->t3 = tt3;
        this->size = 0;
        this->info.sink = nullptr; /* dummy */
    }

    // source ping
    ping_event(const aoo::endpoint& ep,
               aoo::time_tag tt1, aoo::time_tag tt2,
               aoo::time_tag tt3, float packet_loss)
        : ping_event(ep, tt1, tt2, tt3) {
        this->size = sizeof(AooSourcePingInfo);
        this->info.source.packetLoss = packet_loss;
    }
};

struct xrun_event : base_event<AooEventXRun> {
    RT_CLASS(xrun_event)

    xrun_event(int32_t count)
        : base_event(kAooEventXRun) {
        this->count = count;
    }
};

struct format_change_event : endpoint_event<AooEventFormatChange> {
    RT_CLASS(format_change_event)

    format_change_event(const aoo::endpoint& ep, const AooFormat& fmt)
        : endpoint_event(kAooEventFormatChange, ep) {
        auto fp = (AooFormat *)rt_allocate(fmt.size);
        memcpy(fp, &fmt, fmt.size);
        this->format = fp;
    }

    ~format_change_event() {
        rt_deallocate((void *)this->format, this->format->size);
    }
};

struct stream_start_event : endpoint_event<AooEventStreamStart> {
    RT_CLASS(stream_start_event)

    stream_start_event(const aoo::endpoint& ep, const AooData *md)
        : endpoint_event(kAooEventStreamStart, ep) {
        this->metadata = md; // metadata is moved!
    }

    ~stream_start_event() {
        if (this->metadata) {
            auto size = flat_metadata_size(*this->metadata);
            rt_deallocate((void *)this->metadata, size);
        }
    }
};

struct stream_stop_event : endpoint_event<AooEventStreamStop> {
    RT_CLASS(stream_stop_event)

    stream_stop_event(const aoo::endpoint& ep)
        : endpoint_event(kAooEventStreamStop, ep) {}
};

struct stream_state_event : endpoint_event<AooEventStreamState> {
    RT_CLASS(stream_state_event)

    stream_state_event(const aoo::endpoint& ep, AooStreamState state, int32_t offset)
        : endpoint_event(kAooEventStreamState, ep) {
        this->state = state;
        this->sampleOffset = offset;
    }
};

struct block_event : endpoint_event<AooEventBlock> {
    RT_CLASS(block_event)

    block_event(AooEventType type, const aoo::endpoint& ep, int32_t count)
        : endpoint_event(type, ep) {
        this->count = count;
    }
};

struct block_lost_event : block_event {
    block_lost_event(const aoo::endpoint& ep, int32_t count)
        : block_event(kAooEventBlockLost, ep, count) {}
};

struct block_reordered_event : block_event {
    block_reordered_event(const aoo::endpoint& ep, int32_t count)
        : block_event(kAooEventBlockReordered, ep, count) {}
};

struct block_resent_event : block_event {
    block_resent_event(const aoo::endpoint& ep, int32_t count)
        : block_event(kAooEventBlockResent, ep, count) {}
};

struct block_dropped_event : block_event {
    block_dropped_event(const aoo::endpoint& ep, int32_t count)
        : block_event(kAooEventBlockDropped, ep, count) {}
};

} // namespace aoo
