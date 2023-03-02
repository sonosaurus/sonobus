#pragma once

#include "event.hpp"
#include "peer.hpp"

namespace aoo {
namespace net {

struct peer_event : ievent
{
    peer_event(int32_t type, const peer& p)
        : type_(type), group_id_(p.group_id()), user_id_(p.user_id()),
          group_name_(p.group_name()), user_name_(p.user_name()),
          addr_(p.address()), metadata_(p.metadata()) {}

    void dispatch(const event_handler &fn) const override {
        AooEventPeer e;
        e.type = type_;
        e.flags = 0;
        e.groupId = group_id_;
        e.userId = user_id_;
        e.groupName = group_name_.c_str();
        e.userName = user_name_.c_str();
        e.address.data = addr_.valid() ? addr_.address() : nullptr;
        e.address.size = addr_.length();
        AooData md { metadata_.type(), metadata_.data(), metadata_.size() };
        e.metadata = md.size > 0 ? &md : nullptr;

        fn(e);
    }

    AooEventType type_;
    AooId group_id_;
    AooId user_id_;
    std::string group_name_;
    std::string user_name_;
    ip_address addr_;
    metadata metadata_;
};

struct peer_ping_event : ievent
{
    peer_ping_event(const peer& p, time_tag tt1, time_tag tt2, time_tag tt3)
        : group_(p.group_id()), user_(p.user_id()),
          tt1_(tt1), tt2_(tt2), tt3_(tt3) {}

    void dispatch(const event_handler &fn) const override {
        AooEventPeerPing e;
        e.type = kAooEventPeerPing;
        e.flags = 0;
        e.group = group_;
        e.user = user_;
        e.tt1 = tt1_;
        e.tt2 = tt2_;
        e.tt3 = tt3_;

        fn(e);
    }

    AooId group_;
    AooId user_;
    time_tag tt1_;
    time_tag tt2_;
    time_tag tt3_;
};

struct peer_message_event : ievent
{
    peer_message_event(AooId group, AooId user, time_tag tt, const AooData& msg)
        : group_(group), user_(user), tt_(tt), msg_(&msg) {}

    void dispatch(const event_handler &fn) const override {
        AooEventPeerMessage e;
        e.type = kAooEventPeerMessage;
        e.flags = 0; // TODO
        e.groupId = group_;
        e.userId = user_;
        e.timeStamp = tt_;
        e.data.type = msg_.type();
        e.data.data = msg_.data();
        e.data.size = msg_.size();

        fn(e);
    }

    AooId group_;
    AooId user_;
    time_tag tt_;
    metadata msg_;
};

struct peer_update_event : ievent
{
    peer_update_event(AooId group, AooId user, const AooData& md)
        : group_(group), user_(user), md_(&md) {}

    void dispatch(const event_handler &fn) const override {
        AooEventPeerUpdate e;
        e.type = kAooEventPeerUpdate;
        e.flags = 0; // TODO
        e.groupId = group_;
        e.userId = user_;
        e.userMetadata.type = md_.type();
        e.userMetadata.data = md_.data();
        e.userMetadata.size = md_.size();

        fn(e);
    }

    AooId group_;
    AooId user_;
    metadata md_;
};

struct user_update_event : ievent
{
    user_update_event(AooId group, AooId user, const AooData& md)
        : group_(group), user_(user), md_(&md) {}

    void dispatch(const event_handler &fn) const override {
        AooEventClientUserUpdate e;
        e.type = kAooEventClientUserUpdate;
        e.flags = 0; // TODO
        e.groupId = group_;
        e.userId = user_;
        e.userMetadata.type = md_.type();
        e.userMetadata.data = md_.data();
        e.userMetadata.size = md_.size();

        fn(e);
    }

    AooId group_;
    AooId user_;
    metadata md_;
};

struct group_update_event : ievent
{
    group_update_event(AooId group, const AooData& md)
        : group_(group), md_(&md) {}

    void dispatch(const event_handler &fn) const override {
        AooEventClientGroupUpdate e;
        e.type = kAooEventClientGroupUpdate;
        e.flags = 0; // TODO
        e.groupId = group_;
        e.groupMetadata.type = md_.type();
        e.groupMetadata.data = md_.data();
        e.groupMetadata.size = md_.size();

        fn(e);
    }

    AooId group_;
    metadata md_;
};

struct notification_event : ievent
{
    notification_event(const AooData& msg)
        : msg_(&msg) {}

    void dispatch(const event_handler &fn) const override {
        AooEventClientNotification e;
        e.type = kAooEventClientNotification;
        e.flags = 0; // TODO
        e.message.type = msg_.type();
        e.message.data = msg_.data();
        e.message.size = msg_.size();

        fn(e);
    }

    AooId group_;
    AooId user_;
    time_tag tt_;
    metadata msg_;
};

struct disconnect_event : ievent
{
    disconnect_event(int error, std::string msg)
        : error_(error), msg_(std::move(msg)) {}

    void dispatch(const event_handler &fn) const override {
        AooEventClientDisconnect e;
        e.type = kAooEventClientDisconnect;
        e.errorCode = error_;
        e.errorMessage = msg_.c_str();
        fn(e);
    }

    int error_;
    std::string msg_;
};

} // namespace net
} // namespace aoo
