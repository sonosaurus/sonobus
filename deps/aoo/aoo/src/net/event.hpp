#pragma once

#include "aoo/aoo_events.h"

#include <memory>
#include <string>

#define AOO_EVENT_INIT(ptr, name, field) \
    (ptr)->type = k##name; \
    (ptr)->structSize = AOO_STRUCT_SIZE(name, field);

namespace aoo {
namespace net {

struct event_handler {
    event_handler(AooEventHandler fn, void *user, AooThreadLevel level)
        : fn_(fn), user_(user), level_(level) {}

    template<typename T>
    void operator()(const T& event) const {
        fn_(user_, reinterpret_cast<const AooEvent *>(&event), level_);
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

struct error_event : ievent
{
    error_event(int32_t code, std::string msg)
        : code_(code), msg_(std::move(msg)) {}

    void dispatch(const event_handler& fn) const override {
        AooEventError e;
        AOO_EVENT_INIT(&e, AooEventError, errorMessage);
        e.errorCode = code_;
        e.errorMessage = msg_.c_str();

        fn(e);
    }

    int32_t code_;
    std::string msg_;
};

} // namespace net
} // namespace aoo
