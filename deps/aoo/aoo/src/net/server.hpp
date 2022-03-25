/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#pragma once

#include "aoo/aoo_server.hpp"

#include "osc_stream_receiver.hpp"

#include "common/sync.hpp"
#include "common/utils.hpp"
#include "common/lockfree.hpp"
#include "common/net_utils.hpp"

#include "../binmsg.hpp"
#include "../imp.hpp"

#include "oscpack/osc/OscOutboundPacketStream.h"
#include "oscpack/osc/OscReceivedElements.h"

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/poll.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>
#endif

#include <memory.h>
#include <unordered_map>
#include <list>
#include <vector>
#include <thread>

#define DEBUG_THREADS 0

namespace aoo {
namespace net {

class Server;
class group;
class user;
class client_endpoint;

using ip_address_list = std::vector<ip_address>;

//--------------------------- user -----------------------------//

struct user {
    user(const std::string& name, const std::string& pwd, AooId id,
         AooId group, AooId client, const AooDataView *md,
         const ip_host& relay, bool persistent)
        : name_(name), pwd_(pwd), id_(id), group_(group),
          client_(client), md_(md), relay_(relay), persistent_(persistent) {}
    ~user() {}

    const std::string& name() const { return name_; }

    const std::string& pwd() const { return pwd_; }

    bool check_pwd(const char *pwd) const {
        return pwd_.empty() || (pwd && (pwd == pwd_));
    }

    AooId id() const { return id_; }

    const aoo::metadata& metadata() const { return md_; }

    AooId group() const { return group_; }

    bool active() const { return client_ != kAooIdInvalid; }

    void set_client(AooId client) {
        client_ = client;
    }

    void unset() { client_ = kAooIdInvalid; }

    AooId client() const {return client_; }

    void set_relay(const ip_host& relay) {
        relay_ = relay;
    }

    const ip_host& relay_addr() const { return relay_; }

    bool persistent() const { return persistent_; }
private:
    std::string name_;
    std::string pwd_;
    AooId id_;
    AooId group_;
    AooId client_;
    aoo::metadata md_;
    ip_host relay_;
    bool persistent_;
};

inline std::ostream& operator<<(std::ostream& os, const user& u) {
    os << "'" << u.name() << "'(" << u.id() << ")[" << u.client() << "]";
    return os;
}

//--------------------------- group -----------------------------//

using user_list = std::vector<user>;

struct group {
    group(const std::string& name, const std::string& pwd, AooId id,
         const AooDataView *md, const ip_host& relay, bool persistent)
        : name_(name), pwd_(pwd), id_(id), md_(md), relay_(relay), persistent_(persistent) {}

    const std::string& name() const { return name_; }

    const std::string& pwd() const { return pwd_; }

    bool check_pwd(const char *pwd) const {
        return pwd_.empty() || (pwd && (pwd == pwd_));
    }

    AooId id() const { return id_; }

    const aoo::metadata& metadata() const { return md_; }

    const ip_host& relay_addr() const { return relay_; }

    bool persistent() const { return persistent_; }

    bool user_auto_create() const { return user_auto_create_; }

    user* add_user(user&& u);

    user* find_user(const std::string& name);

    user* find_user(AooId id);

    user* find_user(const client_endpoint& client);

    bool remove_user(AooId id);

    int32_t user_count() const { return users_.size(); }

    const user_list& users() const { return users_; }

    AooId get_next_user_id();
private:
    std::string name_;
    std::string pwd_;
    AooId id_;
    aoo::metadata md_;
    ip_host relay_;
    bool persistent_;
    bool user_auto_create_ = AOO_NET_USER_AUTO_CREATE; // TODO
    user_list users_;
    uint32_t next_user_id_{0};
};

inline std::ostream& operator<<(std::ostream& os, const group& g) {
    os << "'" << g.name() << "'(" << g.id() << ")";
    return os;
}

//--------------------- client_endpoint --------------------------//

class client_endpoint {
public:
    client_endpoint(int sockfd, AooId id, AooNetReplyFunc replyfn, void *context)
        : sockfd_(sockfd), id_(id), replyfn_(replyfn), context_(context) {}

    ~client_endpoint() {}

    client_endpoint(client_endpoint&&) = default;
    client_endpoint& operator=(client_endpoint&&) = default;

    AooId id() const { return id_; }

    AooSocket sockfd() const { return sockfd_; }

    void add_public_address(const ip_address& addr) {
        public_addresses_.push_back(addr);
    }

    const ip_address_list& public_addresses() const {
        return public_addresses_;
    }

    bool match(const ip_address& addr) const;

    void send_message(const osc::OutboundPacketStream& msg) const;

    void send_error(Server& server, AooId token, AooNetRequestType type,
                    AooError result, int32_t errcode, const char *errmsg);

    void send_notification(Server& server, const AooDataView& data) const;

    void send_peer_add(Server& server, const group& grp, const user& usr,
                       const client_endpoint& client) const;

    void send_peer_remove(Server& server, const group& grp, const user& usr) const;

    void on_group_join(const group& grp, const user& usr);

    void on_group_leave(const group& grp, const user& usr, bool force);

    void on_close(Server& server);

    void handle_message(Server& server, const AooByte *data, int32_t n);
private:
    AooId id_;
    int sockfd_; // LATER use this to get information about the client (e.g. IP protocol)
    AooNetReplyFunc replyfn_;
    void *context_;
    osc_stream_receiver receiver_;
    ip_address_list public_addresses_;
    struct group_user {
        AooId group;
        AooId user;
    };
    std::vector<group_user> group_users_;
};

//--------------------------- events -----------------------------//

struct client_login_event : ievent
{
    client_login_event(const client_endpoint& c)
        : id_(c.id()), sockfd_(c.sockfd()) {}

    void dispatch(const event_handler& fn) const override {
        AooNetEventClientLogin e;
        e.type = kAooNetEventClientLogin;
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
        AooNetEventClientRemove e;
        e.type = kAooNetEventClientRemove;
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
        AooNetEventGroupAdd e;
        e.type = kAooNetEventGroupAdd;
        e.id = id_;
        e.name = name_.c_str();
        AooDataView md { metadata_.type(), metadata_.data(), metadata_.size() };
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
        AooNetEventGroupRemove e;
        e.type = kAooNetEventGroupRemove;
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
        AooNetEventGroupJoin e;
        e.type = kAooNetEventGroupJoin;
        e.flags = 0;
        e.groupId = group_id_;
        e.userId = user_id_;
        e.groupName = group_name_.c_str();
        e.userName = user_name_.c_str();
        e.clientId = client_id_;
        e.userFlags = 0;
        AooDataView md { metadata_.type(), metadata_.data(), metadata_.size() };
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
        AooNetEventGroupLeave e;
        e.type = kAooNetEventGroupLeave;
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

//------------------------- Server -------------------------------//

class Server final : public AooServer {
public:
    Server();

    ~Server();

    AooError AOO_CALL handleUdpMessage(
            const AooByte *data, AooInt32 size,
            const void *address, AooAddrSize addrlen,
            AooSendFunc replyFn, void *user) override;

    AooError AOO_CALL addClient(
            AooNetReplyFunc replyFn, void *user,
            AooSocket sockfd, AooId *id) override;

    AooError AOO_CALL removeClient(AooId clientId) override;

    AooError AOO_CALL handleClientMessage(
            AooId client, const AooByte *data, AooInt32 size) override;

    AooError AOO_CALL setRequestHandler(
            AooNetRequestHandler cb, void *user, AooFlag flags) override;

    AooError AOO_CALL acceptRequest(
            AooId client, AooId token, const AooNetRequest *request,
            AooNetResponse *response) override;

    AooError AOO_CALL declineRequest(
            AooId client, AooId token, const AooNetRequest *request,
            AooError errorCode, const AooChar *errorMessage) override;

    AooError AOO_CALL notifyClient(
            AooId client, const AooDataView *data) override;

    AooError AOO_CALL notifyGroup(
            AooId group, AooId user, const AooDataView *data) override;

    AooError AOO_CALL findGroup(const AooChar *name, AooId *id) override;

    AooError AOO_CALL addGroup(
            const AooChar *name, const AooChar *password, const AooDataView *metadata,
            const AooIpEndpoint *relayAddress, AooFlag flags, AooId *groupId) override;

    AooError AOO_CALL removeGroup(AooId group) override;

    AooError AOO_CALL findUserInGroup(
            AooId group, const AooChar *userName, AooId *userId) override;

    AooError AOO_CALL addUserToGroup(
            AooId group, const AooChar *userName, const AooChar *userPwd,
            const AooDataView *metadata, AooFlag flags, AooId *userId) override;

    AooError AOO_CALL removeUserFromGroup(
            AooId group, AooId user) override;

    AooError AOO_CALL groupControl(
            AooId group, AooCtl ctl, AooIntPtr index,
            void *data, AooSize size) override;

    AooError AOO_CALL update() override;

    AooError AOO_CALL setEventHandler(
            AooEventHandler fn, void *user, AooEventMode mode) override;

    AooBool AOO_CALL eventsAvailable() override;

    AooError AOO_CALL pollEvents() override;

    AooError AOO_CALL control(
            AooCtl ctl, AooIntPtr index, void *data, AooSize size) override;

    //-----------------------------------------------------------------//

#if 0
    ip_address::ip_type type() const {
        return ip_address::ip_type::Unspec; // TODO
    }
#endif

    client_endpoint * find_client(AooId id);

    client_endpoint * find_client(const user& usr) {
        return find_client(usr.client());
    }

    client_endpoint * find_client(const ip_address& addr);

    bool remove_client(AooId id);

    group* find_group(AooId id);

    group* find_group(const std::string& name);

    group* add_group(group&& g);

    bool remove_group(AooId id);

    void on_user_joined_group(const group& grp, const user& usr,
                              const client_endpoint& client);

    void on_user_left_group(const group& grp, const user& usr);

    void do_remove_user_from_group(group& grp, user& usr);

    osc::OutboundPacketStream start_message(size_t extra_size = 0);

    void handle_message(client_endpoint& client, const osc::ReceivedMessage& msg, int32_t size);
private:
    // UDP
    AooError handle_udp_message(const AooByte *data, AooSize size, int onset,
                            const ip_address& addr, const sendfn& fn);

    AooError handle_relay(const AooByte *data, AooSize size,
                          const aoo::ip_address& addr, const aoo::sendfn& fn) const;

    void handle_ping(const osc::ReceivedMessage& msg,
                     const ip_address& addr, const sendfn& fn) const;

    void handle_query(const osc::ReceivedMessage& msg,
                      const ip_address& addr, const sendfn& fn);

    void do_query(const AooNetRequestQuery& request,
                  const AooNetResponseQuery& response) const;

    // TCP
    void handle_ping(const client_endpoint& client, const osc::ReceivedMessage& msg);

    void handle_login(client_endpoint& client, const osc::ReceivedMessage& msg);

    AooError do_login(client_endpoint& client, AooId token,
                      const AooNetRequestLogin& request,
                      AooNetResponseLogin& response);

    void handle_group_join(client_endpoint& client, const osc::ReceivedMessage& msg);

    AooError do_group_join(client_endpoint& client, AooId token,
                           const AooNetRequestGroupJoin& request,
                           AooNetResponseGroupJoin& response);

    void handle_group_leave(client_endpoint& client, const osc::ReceivedMessage& msg);

    AooError do_group_leave(client_endpoint& client, AooId token,
                            const AooNetRequestGroupLeave& request,
                            AooNetResponseGroupLeave& response);

    void handle_custom_request(client_endpoint& client, const osc::ReceivedMessage& msg);

    AooError do_custom_request(client_endpoint& client, AooId token,
                               const AooNetRequestCustom& request,
                               AooNetResponseCustom& response);

    AooId get_next_client_id();

    AooId get_next_group_id();

    template<typename Request>
    bool handle_request(const Request& request) {
        return request_handler_ &&
                request_handler_(request_context_, kAooIdInvalid,
                                 kAooIdInvalid, (const AooNetRequest *)&request);
    }

    template<typename Request>
    bool handle_request(const client_endpoint& client, AooId token, const Request& request) {
        return request_handler_ &&
                request_handler_(request_context_, client.id(), token, (const AooNetRequest *)&request);
    }

    void send_event(event_ptr event);

    //----------------------------------------------------------------//

    // clients
    using client_map = std::unordered_map<AooId, client_endpoint>;
    client_map clients_;
    AooId next_client_id_{0};
    // groups
    using group_map = std::unordered_map<AooId, group>;
    group_map groups_;
    AooId next_group_id_{0};
    // network
    std::vector<char> sendbuffer_;
    // request handler
    AooNetRequestHandler request_handler_{nullptr};
    void *request_context_{nullptr};
    // event handler
    using event_queue = aoo::unbounded_mpsc_queue<event_ptr>;
    event_queue events_;
    AooEventHandler eventhandler_ = nullptr;
    void *eventcontext_ = nullptr;
    AooEventMode eventmode_ = kAooEventModeNone;
    // options
    ip_host tcp_addr_;
    ip_host relay_addr_;
    std::string password_;
    parameter<bool> allow_relay_{AOO_NET_SERVER_RELAY};
    parameter<bool> group_auto_create_{AOO_NET_GROUP_AUTO_CREATE};
};

} // net
} // aoo
