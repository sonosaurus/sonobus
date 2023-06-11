#pragma once

#include "event.hpp"
#include "client_endpoint.hpp"

namespace aoo {
namespace net {

struct client_login_event : ievent
{
    client_login_event(const client_endpoint& c, AooError error, aoo::metadata md = aoo::metadata{})
        : id_(c.id()), error_(error), version_(c.version()), metadata_(std::move(md)) {}

    void dispatch(const event_handler& fn) const override {
        AooEventClientLogin e;
        AOO_EVENT_INIT(&e, AooEventClientLogin, metadata);
        e.id = id_;
        e.error = error_;
        e.version = version_.c_str();
        AooData md;
        if (metadata_.type() != kAooDataUnspecified && metadata_.size() > 0) {
            md.type = metadata_.type();
            md.data = metadata_.data();
            md.size = metadata_.size();
            e.metadata = &md;
        } else {
            e.metadata = nullptr;
        }

        fn(e);
    }

    AooId id_;
    AooError error_;
    std::string version_;
    aoo::metadata metadata_;
};

struct group_add_event : ievent
{
    group_add_event(const group& grp)
        : id_(grp.id()), flags_(grp.flags()), name_(grp.name()),
          metadata_(grp.metadata()) {}

    void dispatch(const event_handler& fn) const override {
        AooEventGroupAdd e;
        AOO_EVENT_INIT(&e, AooEventGroupAdd, metadata);
        e.id = id_;
        e.flags = flags_;
        e.name = name_.c_str();
        AooData md { metadata_.type(), metadata_.data(), metadata_.size() };
        e.metadata = md.size > 0 ? &md : nullptr;

        fn(e);
    }

    AooId id_;
    AooFlag flags_;
    std::string name_;
    aoo::metadata metadata_;
};

struct group_remove_event : ievent
{
   group_remove_event(const group& grp)
        : id_(grp.id()), name_(grp.name()) {}

    void dispatch(const event_handler& fn) const override {
        AooEventGroupRemove e;
        AOO_EVENT_INIT(&e, AooEventGroupRemove, name);
        e.id = id_;
    #if 1
        e.name = name_.c_str();
    #endif

        fn(e);
    }

    AooId id_;
    std::string name_;
};

struct group_join_event : ievent
{
    group_join_event(const group& grp, const user& usr)
        : group_id_(grp.id()), user_id_(usr.id()),
          group_name_(grp.name()), user_name_(usr.name()),
          metadata_(usr.metadata()), client_id_(usr.client()),
          user_flags_(usr.flags()) {}

    void dispatch(const event_handler& fn) const override {
        AooEventGroupJoin e;
        AOO_EVENT_INIT(&e, AooEventGroupJoin, userMetadata);
        e.groupId = group_id_;
        e.userId = user_id_;
        e.groupName = group_name_.c_str();
        e.userName = user_name_.c_str();
        e.clientId = client_id_;
        e.userFlags = user_flags_;
        AooData md { metadata_.type(), metadata_.data(), metadata_.size() };
        e.userMetadata = md.size > 0 ? &md : nullptr;

        fn(e);
    }

    AooId group_id_;
    AooId user_id_;
    std::string group_name_;
    std::string user_name_;
    aoo::metadata metadata_;
    AooId client_id_;
    AooFlag user_flags_;
};

struct group_leave_event : ievent
{
    group_leave_event(const group& grp, const user& usr)
#if 1
        : group_id_(grp.id()), user_id_(usr.id()),
          group_name_(grp.name()), user_name_(usr.name()){}
#else
        : group_id_(grp.id()), user_id_(usr.id()) {}
#endif

    void dispatch(const event_handler& fn) const override {
        AooEventGroupLeave e;
        AOO_EVENT_INIT(&e, AooEventGroupLeave, userName);
        e.groupId = group_id_;
        e.userId = user_id_;
    #if 1
        e.groupName = group_name_.c_str();
        e.userName = user_name_.c_str();
    #endif

        fn(e);
    }

    AooId group_id_;
    AooId user_id_;
#if 1
    std::string group_name_;
    std::string user_name_;
#endif
};

struct group_update_event : ievent
{
    group_update_event(const group& grp, const user& usr)
        : group_(grp.id()), user_(usr.id()), md_(grp.metadata()) {}

    void dispatch(const event_handler& fn) const override {
        AooEventGroupUpdate e;
        AOO_EVENT_INIT(&e, AooEventGroupUpdate, groupMetadata);
        e.groupId = group_;
        e.userId = user_;
        e.groupMetadata.type = md_.type();
        e.groupMetadata.data = md_.data();
        e.groupMetadata.size = md_.size();

        fn(e);
    }

    AooId group_;
    AooId user_;
    aoo::metadata md_;
};

struct user_update_event : ievent
{
    user_update_event(const user& usr)
        : group_(usr.group()), user_(usr.id()),
          md_(usr.metadata()) {}

    void dispatch(const event_handler& fn) const override {
        AooEventUserUpdate e;
        AOO_EVENT_INIT(&e, AooEventUserUpdate, userMetadata);
        e.groupId = group_;
        e.userId = user_;
        e.userMetadata.type = md_.type();
        e.userMetadata.data = md_.data();
        e.userMetadata.size = md_.size();

        fn(e);
    }

    AooId group_;
    AooId user_;
    aoo::metadata md_;
};

} // namespace net
} // namespace aoo
