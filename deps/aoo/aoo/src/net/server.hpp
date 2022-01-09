/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#pragma once

#include "aoo/aoo_server.hpp"

#include "common/sync.hpp"
#include "common/utils.hpp"
#include "common/lockfree.hpp"
#include "common/net_utils.hpp"

#include "../imp.hpp"
#include "SLIP.hpp"

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

using ip_address_list = aoo::vector<ip_address>;

struct user;
using user_ptr = std::shared_ptr<user>;
using user_list = aoo::vector<user_ptr>;

struct group;
using group_ptr = std::shared_ptr<group>;
using group_list = aoo::vector<group_ptr>;


class client_endpoint {
public:
    client_endpoint(Server& s, int socket, const ip_address& addr);

    ~client_endpoint();

    client_endpoint(const client_endpoint&) = delete;
    client_endpoint(client_endpoint&&) = delete;

    const ip_address& local_address() const { return addr_; }

    const ip_address_list& public_addresses() const {
        return public_addresses_;
    }

    bool match(const ip_address& addr) const;

    int socket() const { return socket_; }

    void close(bool notify = true);

    bool active() const { return socket_ >= 0; }

    void send_message(const char *msg, int32_t);

    bool receive_data();
private:
    Server *server_;
    int socket_;
    ip_address_list public_addresses_;
    std::shared_ptr<user> user_;
    ip_address addr_;

    SLIP<aoo::allocator<uint8_t>> sendbuffer_;
    SLIP<aoo::allocator<uint8_t>> recvbuffer_;

    bool handle_message(const AooByte *data, int32_t n);

    bool handle_bundle(const osc::ReceivedBundle& bundle);

    void handle_ping(const osc::ReceivedMessage& msg);

    void handle_login(const osc::ReceivedMessage& msg);

    void handle_group_join(const osc::ReceivedMessage& msg);

    void handle_group_leave(const osc::ReceivedMessage& msg);
};

struct user {
    user(const std::string& _name, const std::string& _pwd,
         int32_t _id, uint32_t _version)
        : name(_name), password(_pwd), id(_id), version(_version) {}

    ~user() { LOG_VERBOSE("removed user " << name); }

    bool active() const { return endpoint_ != nullptr; }

    void on_close(Server& s);

    bool add_group(std::shared_ptr<group> grp);

    bool remove_group(const group& grp);

    int32_t num_groups() const { return groups_.size(); }

    const group_list& groups() { return groups_; }

    client_endpoint * endpoint() {
        return endpoint_;
    }

    void set_endpoint(client_endpoint *ep){
        endpoint_ = ep;
    }

    // data
    const std::string name;
    const std::string password;
    const int32_t id;
    const uint32_t version;
private:
    group_list groups_;
    client_endpoint *endpoint_ = nullptr;
};

struct group {
    group(const std::string& _name, const std::string& _pwd)
        : name(_name), password(_pwd){}
    ~group() { LOG_VERBOSE("removed group " << name); }

    bool add_user(std::shared_ptr<user> usr);

    bool remove_user(const user& usr);

    int32_t num_users() const { return users_.size(); }

    const user_list& users() { return users_; }

    // data
    const std::string name;
    const std::string password;
private:
    user_list users_;
};

class Server;

class udp_server {
public:
    udp_server(int socket);
    ~udp_server();
private:
    int socket_;
    ip_address::ip_type type_;

    std::thread receivethread_;
    std::thread workerthread_;

    struct udp_packet {
        aoo::vector<AooByte> data;
        ip_address address;
    };
    using packet_queue = aoo::unbounded_mpsc_queue<udp_packet>;
    packet_queue recvbuffer_;
#if DEBUG_THREADS
    std::atomic<int32_t> recvbufferfill_{0};
#endif
    std::atomic<bool> quit_{false};
    sync::event event_;

    void receive_packets();

    void handle_packets();

    void handle_packet(const AooByte *data, int32_t n, const ip_address& addr);

    void handle_message(const osc::ReceivedMessage& msg, int onset, const ip_address& addr);

    void handle_relay_message(const osc::ReceivedMessage& msg, const ip_address& src);

    void send_message(const AooByte *data, int32_t n, const ip_address& addr);
};

class Server final : public AooServer {
public:
    enum class error {
        none,
        wrong_password,
        permission_denied,
        access_denied
    };

    static std::string error_to_string(error e);

    struct ievent {
        virtual ~ievent(){}

        union {
            AooEvent event_;
            AooNetEventError error_event_;
            AooNetEventUser user_event_;
            AooNetEventUserGroup user_group_event_;
        };
    };

    Server(int tcpsocket, int udpsocket);

    ~Server();

    ip_address::ip_type type() const { return type_; }

    AooError AOO_CALL run() override;

    AooError AOO_CALL quit() override;

    AooError AOO_CALL setEventHandler(AooEventHandler fn, void *user,
                                      AooEventMode mode) override;

    AooBool AOO_CALL eventsAvailable() override;

    AooError AOO_CALL pollEvents() override;

    AooError AOO_CALL control(AooCtl ctl, intptr_t index,
                              void *ptr, size_t size) override;

    std::shared_ptr<user> get_user(const std::string& name,
                                   const std::string& pwd,
                                   uint32_t version, error& e);

    std::shared_ptr<user> find_user(const std::string& name);

    std::shared_ptr<group> get_group(const std::string& name,
                                     const std::string& pwd, error& e);

    std::shared_ptr<group> find_group(const std::string& name);

    void on_user_joined(user& usr);

    void on_user_left(user& usr);

    void on_user_joined_group(user& usr, group& grp);

    void on_user_left_group(user& usr, group& grp);

    void handle_relay_message(const osc::ReceivedMessage& msg,
                              const ip_address& src);

    uint32_t flags() const;
private:
    int tcpsocket_;
    int eventsocket_;
    ip_address::ip_type type_;
    aoo::vector<pollfd> pollarray_;
    udp_server udpserver_;
    // clients
    using client_list = std::list<client_endpoint, aoo::allocator<client_endpoint>>;
    client_list clients_;
    // users/groups
    int32_t next_user_id_ = 0;
    user_list users_;
    group_list groups_;
    // events
    using ievent_ptr = std::unique_ptr<ievent>;
    using event_queue = aoo::unbounded_mpsc_queue<ievent_ptr>;
    event_queue events_;
    AooEventHandler eventhandler_ = nullptr;
    void *eventcontext_ = nullptr;
    AooEventMode eventmode_ = kAooEventModeNone;

    void send_event(std::unique_ptr<ievent> e);

    int32_t get_next_user_id();

    // signal
    std::atomic<bool> quit_{false};

    // options
    parameter<bool> allow_relay_{AOO_NET_RELAY_ENABLE};
    parameter<bool> notify_on_shutdown_{AOO_NET_NOTIFY_ON_SHUTDOWN};

    bool receive();

    void update();

    //-------------------------- events ---------------------------//

    struct error_event : ievent
    {
        error_event(int32_t type, int32_t code, const char * msg = 0);
        ~error_event();
    };

    struct user_event : ievent
    {
        user_event(int32_t type, const char *name, int32_t id,
                   const ip_address& address);
        ~user_event();
    };

    struct user_group_event : ievent
    {
        user_group_event(int32_t type, const char *group,
                         const char *user, int32_t id);
        ~user_group_event();
    };
};

} // net
} // aoo
