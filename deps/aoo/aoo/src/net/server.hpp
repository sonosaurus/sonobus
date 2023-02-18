/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#pragma once

#include "aoo/aoo_server.hpp"

#include "common/utils.hpp"
#include "common/lockfree.hpp"
#include "common/net_utils.hpp"

#include "client_endpoint.hpp"
#include "detail.hpp"
#include "event.hpp"

#include "oscpack/osc/OscOutboundPacketStream.h"
#include "oscpack/osc/OscReceivedElements.h"

#include <unordered_map>
#include <vector>

#define DEBUG_THREADS 0

namespace aoo {
namespace net {

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
            AooServerReplyFunc replyFn, void *user,
            AooSocket sockfd, AooId *id) override;

    AooError AOO_CALL removeClient(AooId clientId) override;

    AooError AOO_CALL handleClientMessage(
            AooId client, const AooByte *data, AooInt32 size) override;

    AooError AOO_CALL setRequestHandler(
            AooRequestHandler cb, void *user, AooFlag flags) override;

    AooError AOO_CALL acceptRequest(
            AooId client, AooId token, const AooRequest *request,
            AooResponse *response) override;

    AooError AOO_CALL declineRequest(
            AooId client, AooId token, const AooRequest *request,
            AooError errorCode, const AooChar *errorMessage) override;

    AooError AOO_CALL notifyClient(
            AooId client, const AooData *data) override;

    AooError AOO_CALL notifyGroup(
            AooId group, AooId user, const AooData *data) override;

    AooError AOO_CALL findGroup(const AooChar *name, AooId *id) override;

    AooError AOO_CALL addGroup(
            const AooChar *name, const AooChar *password, const AooData *metadata,
            const AooIpEndpoint *relayAddress, AooFlag flags, AooId *groupId) override;

    AooError AOO_CALL removeGroup(AooId group) override;

    AooError AOO_CALL findUserInGroup(
            AooId group, const AooChar *userName, AooId *userId) override;

    AooError AOO_CALL addUserToGroup(
            AooId group, const AooChar *userName, const AooChar *userPwd,
            const AooData *metadata, AooFlag flags, AooId *userId) override;

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

    void update_group(group& grp, const AooData& md);

    void update_user(const group& grp, user& usr, const AooData& md);

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

    void do_query(const AooRequestQuery& request,
                  const AooResponseQuery& response) const;

    // TCP
    void handle_ping(const client_endpoint& client, const osc::ReceivedMessage& msg);

    void handle_login(client_endpoint& client, const osc::ReceivedMessage& msg);

    AooError do_login(client_endpoint& client, AooId token,
                      const AooRequestLogin& request,
                      AooResponseLogin& response);

    void handle_group_join(client_endpoint& client, const osc::ReceivedMessage& msg);

    AooError do_group_join(client_endpoint& client, AooId token,
                           const AooRequestGroupJoin& request,
                           AooResponseGroupJoin& response);

    void handle_group_leave(client_endpoint& client, const osc::ReceivedMessage& msg);

    AooError do_group_leave(client_endpoint& client, AooId token,
                            const AooRequestGroupLeave& request,
                            AooResponseGroupLeave& response);

    void handle_group_update(client_endpoint& client, const osc::ReceivedMessage& msg);

    AooError do_group_update(client_endpoint& client, AooId token,
                             const AooRequestGroupUpdate& request,
                             AooResponseGroupUpdate& response);

    void handle_user_update(client_endpoint& client, const osc::ReceivedMessage& msg);

    AooError do_user_update(client_endpoint& client, AooId token,
                            const AooRequestUserUpdate& request,
                            AooResponseUserUpdate& response);

    void handle_custom_request(client_endpoint& client, const osc::ReceivedMessage& msg);

    AooError do_custom_request(client_endpoint& client, AooId token,
                               const AooRequestCustom& request,
                               AooResponseCustom& response);

    AooId get_next_client_id();

    AooId get_next_group_id();

    template<typename Request>
    bool handle_request(const Request& request) {
        return request_handler_ &&
                request_handler_(request_context_, kAooIdInvalid,
                                 kAooIdInvalid, (const AooRequest *)&request);
    }

    template<typename Request>
    bool handle_request(const client_endpoint& client, AooId token, const Request& request) {
        return request_handler_ &&
                request_handler_(request_context_, client.id(), token, (const AooRequest *)&request);
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
    AooRequestHandler request_handler_{nullptr};
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
    parameter<bool> allow_relay_{AOO_SERVER_RELAY};
    parameter<bool> group_auto_create_{AOO_GROUP_AUTO_CREATE};
};

} // net
} // aoo
