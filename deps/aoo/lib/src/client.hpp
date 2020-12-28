/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#pragma once

#include "aoo/aoo_net.hpp"
#include "aoo/aoo_utils.hpp"

#include "sync.hpp"
#include "time.hpp"
#include "lockfree.hpp"
#include "net_utils.hpp"
#include "SLIP.hpp"

#include "oscpack/osc/OscOutboundPacketStream.h"
#include "oscpack/osc/OscReceivedElements.h"

#define AOO_NET_CLIENT_PING_INTERVAL 10000
#define AOO_NET_CLIENT_REQUEST_INTERVAL 100
#define AOO_NET_CLIENT_REQUEST_TIMEOUT 5000

namespace aoo {
namespace net {

class client;

class peer {
public:
    peer(client& client, const std::string& group, const std::string& user,
         const ip_address& public_addr, const ip_address& local_addr, int64_t token=0);

    ~peer();

    bool match(const ip_address& addr) const;

    bool match(const std::string& group, const std::string& user);

    bool match_token(int64_t token) const;
    
    void set_public_address(const ip_address & addr);
    
    const std::string& group() const { return group_; }

    const std::string& user() const { return user_; }

    bool has_real_address() const {
        auto addr = address_.load();
        return addr != 0;
    }
    
    const ip_address& address() const {
        auto addr = address_.load();
        if (addr){
            return *addr;
        } else {
            return public_address_;
        }
    }

    void send(time_tag now);

    void handle_message(const osc::ReceivedMessage& msg, int onset,
                        const ip_address& addr);

    friend std::ostream& operator << (std::ostream& os, const peer& p);
private:
    client *client_;
    std::string group_;
    std::string user_;
    ip_address public_address_;
    ip_address local_address_;
    int64_t token_;
    std::atomic<ip_address *> address_{nullptr};
    time_tag start_time_;
    double last_pingtime_ = 0;
    bool timeout_ = false;
};

enum class client_state {
    disconnected,
    connecting,
    handshake,
    login,
    connected
};

enum class command_reason {
    none,
    user,
    timeout,
    error
};

class client final : public iclient {
public:
    struct icommand {
        virtual ~icommand(){}
        virtual void perform(client&) = 0;
    };

    struct ievent {
        virtual ~ievent(){}

        union {
            aoo_event event_;
            aoonet_client_event client_event_;
            aoonet_client_group_event group_event_;
            aoonet_client_peer_event peer_event_;
        };
    };

    client(void *udpsocket, aoo_sendfn fn, int port);
    ~client();

    int32_t run() override;

    int32_t quit() override;

    int32_t connect(const char *host, int port,
                    const char *username, const char *pwd) override;

    int32_t disconnect() override;

    int32_t group_join(const char *group, const char *pwd, bool is_public) override;

    int32_t group_leave(const char *group) override;

    int32_t group_watch_public(bool watch) override;


    int32_t handle_message(const char *data, int32_t n, void *addr) override;

    int32_t send() override;

    int32_t events_available() override;

    int32_t handle_events(aoo_eventhandler fn, void *user) override;

    void do_connect(const std::string& host, int port);

    int try_connect(const std::string& host, int port);

    void do_disconnect(command_reason reason = command_reason::none, int error = 0);

    void do_login();

    void do_group_join(const std::string& group, const std::string& pwd, bool is_public);

    void do_group_leave(const std::string& group);

    void do_group_watch_public(bool watch);

    double ping_interval() const { return ping_interval_.load(); }

    double request_interval() const { return request_interval_.load(); }

    double request_timeout() const { return request_timeout_.load(); }

    void send_message_udp(const char *data, int32_t size, const ip_address& addr);

    void push_event(std::unique_ptr<ievent> e);
    
    int64_t get_token() const { return token_; }
private:
    void *udpsocket_;
    aoo_sendfn sendfn_;
    int udpport_;
    int tcpsocket_ = -1;
    ip_address remote_addr_;
    ip_address public_addr_;
    ip_address local_addr_;
    SLIP sendbuffer_;
    std::vector<uint8_t> pending_send_data_;
    SLIP recvbuffer_;
    shared_mutex clientlock_;
    // peers
    std::vector<std::shared_ptr<peer>> peers_;
    aoo::shared_mutex peerlock_;
    // user
    std::string username_;
    std::string password_;
    // time
    time_tag start_time_;
    double last_tcp_ping_time_ = 0;
    // handshake
    std::atomic<client_state> state_{client_state::disconnected};
    double last_udp_ping_time_ = 0;
    double first_udp_ping_time_ = 0;
    int64_t token_ = 0;
    
    // commands
    lockfree::queue<std::unique_ptr<icommand>> commands_;
    spinlock command_lock_;
    void push_command(std::unique_ptr<icommand>&& cmd){
        scoped_lock<spinlock> lock(command_lock_);
        if (commands_.write_available()){
            commands_.write(std::move(cmd));
        }
    }
    // events
    lockfree::queue<std::unique_ptr<ievent>> events_;
    spinlock event_lock_;
    // signal
    std::atomic<bool> quit_{false};
#ifdef _WIN32
    HANDLE waitevent_ = 0;
    HANDLE sockevent_ = 0;
#else
    int waitpipe_[2];
#endif
    // options
    std::atomic<float> ping_interval_{AOO_NET_CLIENT_PING_INTERVAL * 0.001};
    std::atomic<float> request_interval_{AOO_NET_CLIENT_REQUEST_INTERVAL * 0.001};
    std::atomic<float> request_timeout_{AOO_NET_CLIENT_REQUEST_TIMEOUT * 0.001};

    void send_ping_tcp();

    void send_ping_udp();

    void wait_for_event(float timeout);

    void receive_data();

    void send_server_message_tcp(const char *data, int32_t size);

    void send_server_message_udp(const char *data, int32_t size);

    void handle_server_message_tcp(const osc::ReceivedMessage& msg);

    void handle_server_message_udp(const osc::ReceivedMessage& msg, int onset);

    void handle_login(const osc::ReceivedMessage& msg);

    void handle_group_join(const osc::ReceivedMessage& msg);

    void handle_group_leave(const osc::ReceivedMessage& msg);

    void handle_public_group_add(const osc::ReceivedMessage& msg);

    void handle_public_group_del(const osc::ReceivedMessage& msg);


    void handle_peer_add(const osc::ReceivedMessage& msg);

    void handle_peer_remove(const osc::ReceivedMessage& msg);

    void signal();

    /*////////////////////// events /////////////////////*/
public:
    struct event : ievent
    {
        event(int32_t type, int32_t result,
              const char * errmsg = 0);
        ~event();
    };

    struct group_event : ievent
    {
        group_event(int32_t type, const char *name,
                   int32_t result, const char * errmsg = 0);
        ~group_event();
    };

    struct peer_event : ievent
    {
        peer_event(int32_t type,
                   const char *group, const char *user,
                   const void *address, int32_t length);
        ~peer_event();
    };

    /*////////////////////// commands ///////////////////*/
private:
    struct connect_cmd : icommand
    {
        connect_cmd(const std::string& _host, int _port)
            : host(_host), port(_port){}

        void perform(client &obj) override {
            obj.do_connect(host, port);
        }
        std::string host;
        int port;
    };

    struct disconnect_cmd : icommand
    {
        disconnect_cmd(command_reason _reason, int _error = 0)
            : reason(_reason), error(_error){}

        void perform(client &obj) override {
            obj.do_disconnect(reason, error);
        }
        command_reason reason;
        int error;
    };

    struct login_cmd : icommand
    {
        void perform(client& obj) override {
            obj.do_login();
        }
    };

    struct group_join_cmd : icommand
    {
        group_join_cmd(const std::string& _group, const std::string& _pwd, bool _is_public=false)
            : group(_group), password(_pwd), is_public(_is_public){}

        void perform(client &obj) override {
            obj.do_group_join(group, password, is_public);
        }
        std::string group;
        std::string password;
        bool is_public;
    };

    struct group_leave_cmd : icommand
    {
        group_leave_cmd(const std::string& _group)
            : group(_group){}

        void perform(client &obj) override {
            obj.do_group_leave(group);
        }
        std::string group;
    };

    struct group_watchpublic_cmd : icommand
    {
        group_watchpublic_cmd(bool _watch)
            : watch(_watch){}

        void perform(client &obj) override {
            obj.do_group_watch_public(watch);
        }
        bool watch;
    };
};

} // net
} // aoo
