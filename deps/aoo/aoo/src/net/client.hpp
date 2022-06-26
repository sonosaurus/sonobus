/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#pragma once

#include "aoo/aoo_client.hpp"

#include "osc_stream_receiver.hpp"

#include "common/utils.hpp"
#include "common/sync.hpp"
#include "common/time.hpp"
#include "common/lockfree.hpp"
#include "common/net_utils.hpp"

#include "../imp.hpp"
#include "../binmsg.hpp"
#include "../buffer.hpp"

#include "oscpack/osc/OscOutboundPacketStream.h"
#include "oscpack/osc/OscReceivedElements.h"

#ifdef _WIN32
#include <winsock2.h>
#endif

#include <vector>
#include <unordered_map>
#include <queue>

#ifndef AOO_NET_CLIENT_PING_INTERVAL
 #define AOO_NET_CLIENT_PING_INTERVAL 5000
#endif

#ifndef AOO_NET_CLIENT_QUERY_INTERVAL
 #define AOO_NET_CLIENT_QUERY_INTERVAL 100
#endif

#ifndef AOO_NET_CLIENT_QUERY_TIMEOUT
 #define AOO_NET_CLIENT_QUERY_TIMEOUT 5000
#endif

#ifndef AOO_NET_CLIENT_SIMULATE
#define AOO_NET_CLIENT_SIMULATE 0
#endif

struct AooSource;
struct AooSink;

namespace aoo {
namespace net {

class Client;

#if 0
using ip_address_list = std::vector<ip_address, aoo::allocator<ip_address>>;
#else
using ip_address_list = std::vector<ip_address>;
#endif

//---------------------- peer -----------------------------------//

struct message;

struct message_packet {
    const char *type;
    const AooByte *data;
    int32_t size;
    time_tag tt;
    int32_t sequence;
    int32_t totalsize;
    int32_t nframes;
    int32_t frame;
    bool reliable;
};

struct message_ack {
    int32_t seq;
    int32_t frame;
};

class peer {
public:
    peer(const std::string& groupname, AooId groupid,
         const std::string& username, AooId userid, AooId localid,
         ip_address_list&& addrlist, const AooDataView *metadata,
         ip_address_list&& user_relay, const ip_address_list& group_relay);

    ~peer();

    bool connected() const {
        return connected_.load(std::memory_order_acquire);
    }

    bool match(const ip_address& addr) const;

    bool match(const std::string& group) const;

    bool match(const std::string& group, const std::string& user) const;

    bool match(AooId group) const;

    bool match(AooId group, AooId user) const;

    bool match_wildcard(AooId group, AooId user) const;

    bool relay() const { return relay_; }

    const ip_address& relay_address() const { return relay_address_; }

    const ip_address_list& user_relay() const { return user_relay_;}

    const std::string& group_name() const { return group_name_; }

    AooId group_id() const { return group_id_; }

    const std::string& user_name() const { return user_name_; }

    AooId user_id() const { return user_id_; }

    AooId local_id() const { return local_id_; }

    const ip_address& address() const {
        return real_address_;
    }

    const aoo::metadata& metadata() const { return metadata_; }

    void send(Client& client, const sendfn& fn, time_tag now);

    void send_message(const message& msg, const sendfn& fn, bool binary);

    void handle_osc_message(Client& client, const char *pattern,
                            osc::ReceivedMessageArgumentIterator it,
                            const ip_address& addr);

    void handle_bin_message(Client& client, const AooByte *data,
                            AooSize size, int onset, const ip_address& addr);
private:
    void handle_ping(Client& client, osc::ReceivedMessageArgumentIterator it,
                     const ip_address& addr, bool reply);

    void handle_client_message(Client& client, osc::ReceivedMessageArgumentIterator it);

    void handle_client_message(Client& client, const AooByte *data, AooSize size);

    void do_handle_client_message(Client& client, const message_packet& p, AooFlag flags);

    void handle_ack(Client& client, osc::ReceivedMessageArgumentIterator it);

    void handle_ack(Client& client, const AooByte *data, AooSize size);

    void do_send(Client& client, const sendfn& fn, time_tag now);

    void send_packet_osc(const message_packet& frame, const sendfn& fn) const;

    void send_packet_bin(const message_packet& frame, const sendfn& fn) const;

    void send_ack(const message_ack& ack, const sendfn& fn);

    void send(const osc::OutboundPacketStream& msg, const sendfn& fn) const {
        send((const AooByte *)msg.Data(), msg.Size(), fn);
    }

    void send(const AooByte *data, AooSize size, const sendfn& fn) const;

    void send(const osc::OutboundPacketStream& msg,
              const ip_address& addr, const sendfn& fn) const {
        send((const AooByte *)msg.Data(), msg.Size(), addr, fn);
    }

    void send(const AooByte *data, AooSize size,
              const ip_address& addr, const sendfn &fn) const;

    const std::string group_name_;
    const std::string user_name_;
    const AooId group_id_;
    const AooId user_id_;
    const AooId local_id_;
    aoo::metadata metadata_;
    bool relay_ = false;
    ip_address_list addrlist_;
    ip_address_list user_relay_;
    ip_address_list group_relay_;
    ip_address real_address_;
    ip_address relay_address_;
    time_tag start_time_;
    double last_pingtime_ = 0;
    time_tag ping_tt1_;
    std::atomic<float> average_rtt_{0};
    std::atomic<bool> connected_{false};
    std::atomic<bool> got_ping_{false};
    std::atomic<bool> binary_{false};
    bool timeout_ = false;
    int32_t next_sequence_reliable_ = 0;
    int32_t next_sequence_unreliable_ = 0;
    aoo::message_send_buffer send_buffer_;
    aoo::message_receive_buffer receive_buffer_;
    received_message current_msg_;
    aoo::unbounded_mpsc_queue<message_ack> send_acks_;
    aoo::unbounded_mpsc_queue<message_ack> received_acks_;
};

inline std::ostream& operator<<(std::ostream& os, const peer& p) {
    os << "" << p.group_name() << "|" << p.user_name()
       << " (" << p.group_id() << "|" << p.user_id() << ")";
    return os;
}

//------------------------ events ----------------------------//

struct base_event : ievent {
    base_event(AooEventType type) : type_(type) {}

    void dispatch(const event_handler &fn) const override {
        AooEvent e;
        e.type = type_;
        fn(e);
    }

    AooEventType type_;
};

struct peer_event : base_event
{
    peer_event(int32_t type, const peer& p)
        : base_event(type), group_id_(p.group_id()), user_id_(p.user_id()),
          group_name_(p.group_name()), user_name_(p.user_name()),
          addr_(p.address()), metadata_(p.metadata()) {}

    void dispatch(const event_handler &fn) const override {
        AooNetEventPeer e;
        e.type = type_;
        e.flags = 0;
        e.groupId = group_id_;
        e.userId = user_id_;
        e.groupName = group_name_.c_str();
        e.userName = user_name_.c_str();
        e.address.data = addr_.valid() ? addr_.address() : nullptr;
        e.address.size = addr_.length();
        AooDataView md { metadata_.type(), metadata_.data(), metadata_.size() };
        e.metadata = md.size > 0 ? &md : nullptr;

        fn(e);
    }

    AooId group_id_;
    AooId user_id_;
    std::string group_name_;
    std::string user_name_;
    ip_address addr_;
    metadata metadata_;
};

struct peer_ping_base_event : base_event
{
    peer_ping_base_event(AooEventType type, const peer& p,
                         time_tag tt1, time_tag tt2, time_tag tt3 = time_tag{})
        : base_event(type), group_(p.group_id()), user_(p.user_id()),
          tt1_(tt1), tt2_(tt2), tt3_(tt3) {}

    void dispatch(const event_handler &fn) const override {
        // AooNetEventPeerPing and AooNetEventPeerPingReply are layout compatible
        AooNetEventPeerPingReply e;
        e.type = type_;
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

struct peer_ping_event : peer_ping_base_event
{
    peer_ping_event(const peer& p, time_tag tt1, time_tag tt2)
        : peer_ping_base_event(kAooNetEventPeerPing, p, tt1, tt2) {}
};

struct peer_ping_reply_event : peer_ping_base_event
{
    peer_ping_reply_event(const peer& p, time_tag tt1, time_tag tt2, time_tag tt3)
        : peer_ping_base_event(kAooNetEventPeerPingReply, p, tt1, tt2, tt3) {}
};

struct peer_message_event : base_event
{
    peer_message_event(AooId group, AooId user, time_tag tt, const AooDataView& msg)
        : base_event(kAooNetEventPeerMessage), group_(group), user_(user), tt_(tt), msg_(&msg) {}

    void dispatch(const event_handler &fn) const override {
        AooNetEventPeerMessage e;
        e.type = type_;
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

struct peer_update_event : base_event
{
    peer_update_event(AooId group, AooId user, const AooDataView& md)
        : base_event(kAooNetEventPeerUpdate), group_(group), user_(user), md_(&md) {}

    void dispatch(const event_handler &fn) const override {
        AooNetEventPeerUpdate e;
        e.type = type_;
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

struct user_update_event : base_event
{
    user_update_event(AooId group, AooId user, const AooDataView& md)
        : base_event(kAooNetEventClientUserUpdate), group_(group), user_(user), md_(&md) {}

    void dispatch(const event_handler &fn) const override {
        AooNetEventClientUserUpdate e;
        e.type = type_;
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

struct group_update_event : base_event
{
    group_update_event(AooId group, const AooDataView& md)
        : base_event(kAooNetEventClientGroupUpdate), group_(group), md_(&md) {}

    void dispatch(const event_handler &fn) const override {
        AooNetEventClientGroupUpdate e;
        e.type = type_;
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

struct notification_event : base_event
{
    notification_event(const AooDataView& msg)
        : base_event(kAooNetEventClientNotification), msg_(&msg) {}

    void dispatch(const event_handler &fn) const override {
        AooNetEventClientNotification e;
        e.type = type_;
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

//---------------------------- message ---------------------------------//

// peer/group messages
struct message {
    message() = default;

    message(AooId group, AooId user, time_tag tt, const AooDataView& data, bool reliable)
        : group_(group), user_(user), tt_(tt), data_(&data), reliable_(reliable) {}
    // data
    AooId group_ = kAooIdInvalid;
    AooId user_ = kAooIdInvalid;
    time_tag tt_ = time_tag::immediate();
    metadata data_;
    bool reliable_ = false;
};

//---------------------------- udp_client ---------------------------------//

class udp_client {
public:
    udp_client(int socket, int port, ip_address::ip_type type)
        : socket_(socket), port_(port), type_(type) {}

    int port() const { return port_; }

    ip_address::ip_type type() const { return type_; }

    AooError handle_osc_message(Client& client, const AooByte *data, int32_t n,
                                const ip_address& addr, int32_t type, AooMsgType onset);

    AooError handle_bin_message(Client& client, const AooByte *data, int32_t n,
                                const ip_address& addr, int32_t type, AooMsgType onset);

    void update(Client& client, const sendfn& fn, time_tag now);

    void start_handshake(ip_address_list&& remote);

    void queue_message(message&& msg);
private:
    void send_server_message(const osc::OutboundPacketStream& msg, const sendfn& fn);

    void handle_server_message(Client& client, const osc::ReceivedMessage& msg, int onset);

    bool is_server_address(const ip_address& addr);

    using scoped_lock = sync::scoped_lock<sync::shared_mutex>;
    using scoped_shared_lock = sync::scoped_shared_lock<sync::shared_mutex>;

    int socket_;
    int port_;
    ip_address::ip_type type_;

    ip_address_list server_addrlist_;
    ip_address_list tcp_addrlist_;
    ip_address_list public_addrlist_;
    ip_address local_address_;
    sync::shared_mutex mutex_;

    double last_ping_time_ = 0;
    std::atomic<double> first_ping_time_{0};

    using message_queue = aoo::unbounded_mpsc_queue<message>;
    message_queue messages_;
};

//------------------------- Client ----------------------------//

enum class client_state {
    disconnected,
    handshake,
    connecting,
    connected
};

class Client final : public AooClient
{
public:
    struct icommand {
        virtual ~icommand(){}
        virtual void perform(Client&) = 0;
    };

    using command_ptr = std::unique_ptr<icommand>;

    //----------------------------------------------------------//

    Client(int socket, const ip_address& address,
           AooFlag flags, AooError *err);

    ~Client();

    AooError AOO_CALL run(AooBool nonBlocking) override;

    AooError AOO_CALL quit() override;

    AooError AOO_CALL addSource(AooSource *src, AooId id) override;

    AooError AOO_CALL removeSource(AooSource *src) override;

    AooError AOO_CALL addSink(AooSink *sink, AooId id) override;

    AooError AOO_CALL removeSink(AooSink *sink) override;

    AooError AOO_CALL connect(
            const AooChar *hostName, AooInt32 port, const AooChar *password,
            const AooDataView *metadata, AooNetCallback cb, void *context) override;

    AooError AOO_CALL disconnect(AooNetCallback cb, void *context) override;

    AooError AOO_CALL joinGroup(
            const AooChar *groupName, const AooChar *groupPwd, const AooDataView *groupMetadata,
            const AooChar *userName, const AooChar *userPwd, const AooDataView *userMetadata,
            const AooIpEndpoint *relayAddress, AooNetCallback cb, void *context) override;

    AooError AOO_CALL leaveGroup(AooId group, AooNetCallback cb, void *context) override;

    AooError AOO_CALL updateGroup(AooId group, const AooDataView& metadata,
                                  AooNetCallback cb, void *context) override;

    AooError AOO_CALL updateUser(AooId group, AooId user, const AooDataView& metadata,
                                 AooNetCallback cb, void *context) override;

    AooError AOO_CALL customRequest(const AooDataView& data, AooFlag flags,
                                    AooNetCallback cb, void *context) override;

    AooError AOO_CALL findPeerByName(
            const AooChar *group, const AooChar *user, AooId *groupId,
            AooId *userId, void *address, AooAddrSize *addrlen) override;

    AooError AOO_CALL findPeerByAddress(const void *address, AooAddrSize addrlen,
                                        AooId *groupId, AooId *userId) override;

    AooError AOO_CALL getPeerName(AooId group, AooId user,
                                  AooChar *groupNameBuffer, AooSize *groupNameSize,
                                  AooChar *userNameBuffer, AooSize *userNameSize) override;

    AooError AOO_CALL sendMessage(
            AooId group, AooId user, const AooDataView& msg,
            AooNtpTime timeStamp, AooFlag flags) override;


    AooError AOO_CALL handleMessage(
            const AooByte *data, AooInt32 n,
            const void *addr, AooAddrSize len) override;

    AooError AOO_CALL send(AooSendFunc fn, void *user) override;

    AooError AOO_CALL setEventHandler(
            AooEventHandler fn, void *user, AooEventMode mode) override;

    AooBool AOO_CALL eventsAvailable() override;

    AooError AOO_CALL pollEvents() override;

    AooError AOO_CALL sendRequest(
            const AooNetRequest& request, AooNetCallback callback,
            void *user, AooFlag flags) override;

    AooError AOO_CALL control(
            AooCtl ctl, intptr_t index, void *ptr, size_t size) override;

    //---------------------------------------------------------------------//

    bool handle_peer_osc_message(const osc::ReceivedMessage& msg, int onset,
                                 const ip_address& addr);

    bool handle_peer_bin_message(const AooByte *data, AooSize size, int onset,
                                 const ip_address& addr);

    struct connect_cmd;
    void perform(const connect_cmd& cmd);

    int try_connect(const ip_address& remote);

    struct login_cmd;
    void perform(const login_cmd& cmd);

    struct timeout_cmd;
    void perform(const timeout_cmd& cmd);

    struct disconnect_cmd;
    void perform(const disconnect_cmd& cmd);

    struct group_join_cmd;
    void perform(const group_join_cmd& cmd);

    void handle_response(const group_join_cmd& cmd, const osc::ReceivedMessage& msg);

    struct group_leave_cmd;
    void perform(const group_leave_cmd& cmd);

    void handle_response(const group_leave_cmd& cmd, const osc::ReceivedMessage& msg);

    struct group_update_cmd;
    void perform(const group_update_cmd& cmd);

    void handle_response(const group_update_cmd& cmd, const osc::ReceivedMessage& msg);

    struct user_update_cmd;
    void perform(const user_update_cmd& cmd);

    void handle_response(const user_update_cmd& cmd, const osc::ReceivedMessage& msg);

    struct custom_request_cmd;
    void perform(const custom_request_cmd& cmd);

    void handle_response(const custom_request_cmd& cmd, const osc::ReceivedMessage& msg);

    void perform(const message& msg, const sendfn& fn);

    double ping_interval() const { return ping_interval_.load(); }

    double query_interval() const { return query_interval_.load(); }

    double query_timeout() const { return query_timeout_.load(); }

    bool binary() const { return binary_.load(); }

    void send_event(event_ptr e);

    void push_command(command_ptr cmd);

    double elapsed_time_since(time_tag now) const {
        return time_tag::duration(start_time_, now);
    }

    client_state current_state() const { return state_.load(); }
private:
    // networking
    int socket_ = -1;
    udp_client udp_client_;
    osc_stream_receiver receiver_;
    ip_address local_addr_;
    std::atomic<bool> quit_{false};
    int eventsocket_ = -1;
    bool server_relay_ = false;
    std::vector<char> sendbuffer_;
    // dependants
    struct source_desc {
        AooSource *source;
        AooId id;
    };
    aoo::vector<source_desc> sources_;
    struct sink_desc {
        AooSink *sink;
        AooId id;
    };
    aoo::vector<sink_desc> sinks_;
    // peers
    using peer_list = aoo::rcu_list<peer>;
    using peer_lock = std::unique_lock<peer_list>;
    peer_list peers_;
    // time
    time_tag start_time_;
    double last_ping_time_ = 0;
    // connect/login
    std::atomic<client_state> state_{client_state::disconnected};
    std::unique_ptr<connect_cmd> connection_;
    struct group_membership {
        std::string group_name;
        std::string user_name;
        AooId group_id;
        AooId user_id;
        ip_address_list relay_list;
    };
    std::vector<group_membership> memberships_;
    // commands
    using command_queue = aoo::unbounded_mpsc_queue<command_ptr>;
    command_queue commands_;
    // pending request
    struct callback_cmd : icommand
    {
        callback_cmd(AooNetCallback cb, void *user)
            : cb_(cb), user_(user) {}

        virtual void handle_response(Client& client, const osc::ReceivedMessage& msg) = 0;

        virtual void reply(AooError result, const AooNetResponse *response) const = 0;

        void reply_error(AooError result, const char *msg, int32_t code) const {
            AooNetResponseError response;
            response.type = kAooNetRequestError;
            response.errorCode = code;
            response.errorMessage = msg;
            reply(result, (AooNetResponse*)&response);
        }
    protected:
        void callback(const AooNetRequest& request, AooError result, const AooNetResponse* response) const {
            if (cb_) {
                cb_(user_, &request, result, response);
            }
        }
    private:
        AooNetCallback cb_;
        void *user_;
    };
    using callback_cmd_ptr = std::unique_ptr<callback_cmd>;
    using request_map = std::unordered_map<AooId, callback_cmd_ptr>;
    request_map pending_requests_;
    AooId next_token_ = 0;
    // events
    using event_queue = aoo::unbounded_mpsc_queue<event_ptr>;
    event_queue events_;
    AooEventHandler eventhandler_ = nullptr;
    void *eventcontext_ = nullptr;
    AooEventMode eventmode_ = kAooEventModeNone;
    // options
    parameter<AooSeconds> ping_interval_{AOO_NET_CLIENT_PING_INTERVAL * 0.001};
    parameter<AooSeconds> query_interval_{AOO_NET_CLIENT_QUERY_INTERVAL * 0.001};
    parameter<AooSeconds> query_timeout_{AOO_NET_CLIENT_QUERY_TIMEOUT * 0.001};
    parameter<bool> binary_{AOO_NET_CLIENT_BINARY_MSG};
#if AOO_NET_CLIENT_SIMULATE
    parameter<float> sim_packet_drop_{0};
    parameter<float> sim_packet_reorder_{0};
    parameter<bool> sim_packet_jitter_{false};

    struct netpacket {
        std::vector<AooByte> data;
        aoo::ip_address addr;
        time_tag tt;
        uint64_t sequence;

        bool operator>(const netpacket& other) const {
            // preserve FIFO ordering for packets with the same timestamp
            if (tt == other.tt) {
                return sequence > other.sequence;
            } else {
                return tt > other.tt;
            }
        }
    };
    using packet_queue = std::priority_queue<netpacket, std::vector<netpacket>, std::greater<netpacket>>;
    packet_queue packetqueue_;
    uint64_t packet_sequence_ = 0;
#endif

    // methods
    bool wait_for_event(float timeout);

    void receive_data();

    bool signal();

    osc::OutboundPacketStream start_server_message(size_t extra = 0);

    void send_server_message(const osc::OutboundPacketStream& msg);

    void handle_server_message(const osc::ReceivedMessage& msg, int32_t n);

    void handle_login(const osc::ReceivedMessage& msg);

    void handle_server_notification(const osc::ReceivedMessage& msg);

    void handle_group_changed(const osc::ReceivedMessage& msg);

    void handle_user_changed(const osc::ReceivedMessage& msg);

    void handle_peer_changed(const osc::ReceivedMessage& msg);

    void handle_peer_add(const osc::ReceivedMessage& msg);

    void handle_peer_remove(const osc::ReceivedMessage& msg);

    void on_socket_error(int err);

    void on_exception(const char *what, const osc::Exception& err,
                      const char *pattern = nullptr);

    void close(bool manual = false);

    group_membership * find_group_membership(const std::string& name);

    group_membership * find_group_membership(AooId id);

public:
    //---------------------- commands ------------------------//

    struct connect_cmd : callback_cmd
    {
        connect_cmd(const std::string& hostname, int port, const char * pwd,
                    const AooDataView *metadata, AooNetCallback cb, void *user)
            : callback_cmd(cb, user),
              host_(hostname, port), pwd_(pwd ? pwd : ""), metadata_(metadata) {}

        void perform(Client& obj) override {
            obj.perform(*this);
        }

        void handle_response(Client &client, const osc::ReceivedMessage &msg) override {
            // dummy
        }

        void reply(AooError result, const AooNetResponse *response) const override {
            AooNetRequestConnect request;
            request.type = kAooNetRequestConnect;
            request.flags = 0; // TODO
            request.address.hostName = host_.name.c_str();
            request.address.port = host_.port;
            request.password = pwd_.empty() ? nullptr : pwd_.c_str();
            AooDataView md { metadata_.type(), metadata_.data(), metadata_.size() };
            request.metadata = (md.size > 0) ? &md : nullptr;

            callback((AooNetRequest&)request, result, response);
        }
    public:
        ip_host host_;
        std::string pwd_;
        aoo::metadata metadata_;
    };

    struct disconnect_cmd : callback_cmd
    {
        disconnect_cmd(AooNetCallback cb, void *user)
            : callback_cmd(cb, user) {}

        void perform(Client& obj) override {
            obj.perform(*this);
        }

        void handle_response(Client &client, const osc::ReceivedMessage &msg) override {
            // dummy
        }

        void reply(AooError result, const AooNetResponse* response) const override {
            AooNetRequest request;
            request.type = kAooNetRequestConnect;

            callback((AooNetRequest&)request, result, response);
        }
    };

    struct login_cmd : icommand
    {
        login_cmd(ip_address_list&& server_ip, ip_address_list&& public_ip)
            : server_ip_(std::move(server_ip)),
              public_ip_(std::move(public_ip)) {}

        void perform(Client& obj) override {
            obj.perform(*this);
        }
    public:
        ip_address_list server_ip_;
        ip_address_list public_ip_;
    };

    struct timeout_cmd : icommand
    {
        void perform(Client& obj) override {
            obj.perform(*this);
        }
    };

    struct group_join_cmd : callback_cmd
    {
        group_join_cmd(const std::string& group_name, const char * group_pwd, const AooDataView *group_md,
                       const std::string& user_name, const char * user_pwd, const AooDataView *user_md,
                       const ip_host& relay, AooNetCallback cb, void *user)
            : callback_cmd(cb, user),
              group_name_(group_name), group_pwd_(group_pwd ? group_pwd : ""), group_md_(group_md),
              user_name_(user_name), user_pwd_(user_pwd ? user_pwd : ""), user_md_(user_md), relay_(relay) {}

        void perform(Client& obj) override {
            obj.perform(*this);
        }

        void handle_response(Client &client, const osc::ReceivedMessage &msg) override {
            client.handle_response(*this, msg);
        }

        void reply(AooError result, const AooNetResponse* response) const override {
            AooNetRequestGroupJoin request;
            request.type = kAooNetRequestGroupJoin;
            request.flags = 0; // TODO

            request.groupName = group_name_.c_str();
            request.groupPwd = group_pwd_.empty() ? nullptr : group_pwd_.c_str();
            AooDataView group_md { group_md_.type(), group_md_.data(), group_md_.size() };
            request.groupMetadata = (group_md.size > 0) ? &group_md : nullptr;

            request.userName = user_name_.c_str();
            request.userPwd = user_pwd_.empty() ? nullptr : user_pwd_.c_str();
            AooDataView user_md { user_md_.type(), user_md_.data(), user_md_.size() };
            request.userMetadata = (user_md.size > 0) ? &user_md : nullptr;

            AooIpEndpoint relay;
            if (relay_.valid()) {
                relay.hostName = relay_.name.c_str();
                relay.port = relay_.port;
                request.relayAddress = &relay;
            } else {
                request.relayAddress = nullptr;
            }

            callback((AooNetRequest&)request, result, response);
        }
    public:
        std::string group_name_;
        std::string group_pwd_;
        aoo::metadata group_md_;
        std::string user_name_;
        std::string user_pwd_;
        aoo::metadata user_md_;
        ip_host relay_;
    };

    struct group_leave_cmd : callback_cmd
    {
        group_leave_cmd(AooId group, AooNetCallback cb, void *user)
            : callback_cmd(cb, user), group_(group) {}

        void perform(Client& obj) override {
            obj.perform(*this);
        }

        void handle_response(Client &client, const osc::ReceivedMessage &msg) override {
            client.handle_response(*this, msg);
        }

        void reply(AooError result, const AooNetResponse* response) const override {
            AooNetRequestGroupLeave request;
            request.type = kAooNetRequestGroupLeave;
            request.group = group_;

            callback((AooNetRequest&)request, result, response);
        }
    public:
        AooId group_;
    };

    struct group_update_cmd : callback_cmd
    {
        group_update_cmd(AooId group, const AooDataView &md, AooNetCallback cb, void *context)
            : callback_cmd(cb, context),
              group_(group), md_(&md) {}

        void perform(Client& obj) override {
            obj.perform(*this);
        }

        void handle_response(Client &client, const osc::ReceivedMessage &msg) override {
            client.handle_response(*this, msg);
        }

        void reply(AooError result, const AooNetResponse* response) const override {
            AooNetRequestGroupUpdate request;
            request.type = kAooNetRequestGroupUpdate;
            request.flags = 0; // TODO
            request.groupId = group_;
            request.groupMetadata.type = md_.type();
            request.groupMetadata.data = md_.data();
            request.groupMetadata.size = md_.size();

            callback((AooNetRequest&)request, result, response);
        }
    public:
        AooId group_;
        aoo::metadata md_;
    };

    struct user_update_cmd : callback_cmd
    {
        user_update_cmd(AooId group, AooId user,
                        const AooDataView &md, AooNetCallback cb, void *context)
            : callback_cmd(cb, context),
              group_(group), user_(user), md_(&md) {}

        void perform(Client& obj) override {
            obj.perform(*this);
        }

        void handle_response(Client &client, const osc::ReceivedMessage &msg) override {
            client.handle_response(*this, msg);
        }

        void reply(AooError result, const AooNetResponse* response) const override {
            AooNetRequestUserUpdate request;
            request.type = kAooNetRequestUserUpdate;
            request.flags = 0; // TODO
            request.groupId = group_;
            request.userId = user_;
            request.userMetadata.type = md_.type();
            request.userMetadata.data = md_.data();
            request.userMetadata.size = md_.size();

            callback((AooNetRequest&)request, result, response);
        }
    public:
        AooId group_;
        AooId user_;
        aoo::metadata md_;
    };

    struct custom_request_cmd : callback_cmd
    {
        custom_request_cmd(const AooDataView& data, AooFlag flags, AooNetCallback cb, void *user)
            : callback_cmd(cb, user), data_(&data), flags_(flags) {}

        void perform(Client& obj) override {
            obj.perform(*this);
        }

        void handle_response(Client &client, const osc::ReceivedMessage &msg) override {
            auto it = msg.ArgumentsBegin();
            auto token = (it++)->AsInt32(); // skip
            auto result = (it++)->AsInt32();
            if (result == kAooOk) {
                AooNetResponseCustom response;
                response.type = kAooNetRequestCustom;
                response.flags = (AooFlag)(it++)->AsInt32();
                response.data = osc_read_metadata(it);

                reply(result, (AooNetResponse*)&response);
            } else {
                auto code = (it++)->AsInt32();
                auto msg = (it++)->AsString();
                reply_error(result, msg, code);
            }
        }

        void reply(AooError result, const AooNetResponse* response) const override {
            AooNetRequestCustom request;
            request.type = kAooNetRequestCustom;
            request.flags = flags_;
            request.data.type = data_.type();
            request.data.data = data_.data();
            request.data.size = data_.size();

            callback((AooNetRequest&)request, result, response);
        }
    public:
        aoo::metadata data_;
        AooFlag flags_;
    };
};

} // net
} // aoo
