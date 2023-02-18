#pragma once

#include "event.hpp"
#include "client_endpoint.hpp"

namespace aoo {
namespace net {

struct client_login_event : ievent
{
    client_login_event(const client_endpoint& c)
        : id_(c.id()), sockfd_(c.sockfd()) {}

    void dispatch(const event_handler& fn) const override {
        AooEventServerClientLogin e;
        e.type = kAooEventServerClientLogin;
        e.flags = 0;
        e.id = id_;
        e.sockfd = sockfd_;

        fn(e);
    }

    AooId id_;
    AooSocket sockfd_;
};

struct client_remove_event : ievent
{
    client_remove_event(AooId id)
        : id_(id) {}

    void dispatch(const event_handler& fn) const override {
        AooEventServerClientRemove e;
        e.type = kAooEventServerClientRemove;
        e.id = id_;

        fn(e);
    }

    AooId id_;
};

struct group_add_event : ievent
{
    group_add_event(const group& grp)
        : id_(grp.id()), name_(grp.name()), metadata_(grp.metadata()) {}

    void dispatch(const event_handler& fn) const override {
        AooEventServerGroupAdd e;
        e.type = kAooEventServerGroupAdd;
        e.id = id_;
        e.name = name_.c_str();
        AooData md { metadata_.type(), metadata_.data(), metadata_.size() };
        e.metadata = md.size > 0 ? &md : nullptr;
        e.flags = 0;

        fn(e);
    }

    AooId id_;
    std::string name_;
    aoo::metadata metadata_;
};

struct group_remove_event : ievent
{
   group_remove_event(const group& grp)
        : id_(grp.id()), name_(grp.name()) {}

    void dispatch(const event_handler& fn) const override {
        AooEventServerGroupRemove e;
        e.type = kAooEventServerGroupRemove;
        e.id = id_;
        e.name = name_.c_str();

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
          metadata_(usr.metadata()), client_id_(usr.client()) {}

    void dispatch(const event_handler& fn) const override {
        AooEventServerGroupJoin e;
        e.type = kAooEventServerGroupJoin;
        e.flags = 0;
        e.groupId = group_id_;
        e.userId = user_id_;
        e.groupName = group_name_.c_str();
        e.userName = user_name_.c_str();
        e.clientId = client_id_;
        e.userFlags = 0;
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
};

struct group_leave_event : ievent
{
    group_leave_event(const group& grp, const user& usr)
        : group_id_(grp.id()), user_id_(usr.id()),
          group_name_(grp.name()), user_name_(usr.name()) {}

    void dispatch(const event_handler& fn) const override {
        AooEventServerGroupLeave e;
        e.type = kAooEventServerGroupLeave;
        e.flags = 0;
        e.groupId = group_id_;
        e.userId = user_id_;
        e.groupName = group_name_.c_str();
        e.userName = user_name_.c_str();

        fn(e);
    }

    AooId group_id_;
    AooId user_id_;
    std::string group_name_;
    std::string user_name_;
};

struct group_update_event : ievent
{
    group_update_event(const group& grp)
        : group_(grp.id()), md_(grp.metadata()) {}

    void dispatch(const event_handler& fn) const override {
        AooEventServerGroupUpdate e;
        e.type = kAooEventServerGroupUpdate;
        e.flags = 0;
        e.groupId = group_;
        e.groupMetadata.type = md_.type();
        e.groupMetadata.data = md_.data();
        e.groupMetadata.size = md_.size();

        fn(e);
    }

    AooId group_;
    aoo::metadata md_;
};

struct user_update_event : ievent
{
    user_update_event(const user& usr)
        : group_(usr.group()), user_(usr.id()),
          md_(usr.metadata()) {}

    void dispatch(const event_handler& fn) const override {
        AooEventServerUserUpdate e;
        e.type = kAooEventServerUserUpdate;
        e.flags = 0;
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
