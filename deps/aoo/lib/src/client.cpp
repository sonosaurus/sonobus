/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "client.hpp"

#include <cstring>
#include <functional>
#include <algorithm>
#include <sstream>
#include <random>

#include "md5/md5.h"

#define AOONET_MSG_SERVER_PING \
    AOO_MSG_DOMAIN AOONET_MSG_SERVER AOONET_MSG_PING

#define AOONET_MSG_PEER_PING \
    AOO_MSG_DOMAIN AOONET_MSG_PEER AOONET_MSG_PING

#define AOONET_MSG_SERVER_LOGIN \
    AOO_MSG_DOMAIN AOONET_MSG_SERVER AOONET_MSG_LOGIN

#define AOONET_MSG_SERVER_REQUEST \
    AOO_MSG_DOMAIN AOONET_MSG_SERVER AOONET_MSG_REQUEST

#define AOONET_MSG_SERVER_GROUP_JOIN \
    AOO_MSG_DOMAIN AOONET_MSG_SERVER AOONET_MSG_GROUP AOONET_MSG_JOIN

#define AOONET_MSG_SERVER_GROUP_LEAVE \
    AOO_MSG_DOMAIN AOONET_MSG_SERVER AOONET_MSG_GROUP AOONET_MSG_LEAVE

#define AOONET_MSG_SERVER_GROUP_PUBLIC_ADD \
    AOO_MSG_DOMAIN AOONET_MSG_SERVER AOONET_MSG_GROUP AOONET_MSG_PUBLIC AOONET_MSG_ADD

#define AOONET_MSG_SERVER_GROUP_PUBLIC_DEL \
    AOO_MSG_DOMAIN AOONET_MSG_SERVER AOONET_MSG_GROUP AOONET_MSG_PUBLIC AOONET_MSG_DEL

#define AOONET_MSG_SERVER_GROUP_PUBLIC \
    AOO_MSG_DOMAIN AOONET_MSG_SERVER AOONET_MSG_GROUP AOONET_MSG_PUBLIC


#define AOONET_MSG_GROUP_JOIN \
    AOONET_MSG_GROUP AOONET_MSG_JOIN

#define AOONET_MSG_GROUP_LEAVE \
    AOONET_MSG_GROUP AOONET_MSG_LEAVE

#define AOONET_MSG_PEER_JOIN \
    AOONET_MSG_PEER AOONET_MSG_JOIN

#define AOONET_MSG_PEER_LEAVE \
    AOONET_MSG_PEER AOONET_MSG_LEAVE

#define AOONET_MSG_GROUP_PUBLIC_ADD \
    AOONET_MSG_GROUP AOONET_MSG_PUBLIC AOONET_MSG_ADD

#define AOONET_MSG_GROUP_PUBLIC_DEL \
    AOONET_MSG_GROUP AOONET_MSG_PUBLIC AOONET_MSG_DEL

#define AOONET_MSG_GROUP_PUBLIC \
    AOONET_MSG_GROUP AOONET_MSG_PUBLIC



namespace aoo {
namespace net {

std::string encrypt(const std::string& input){
    uint8_t result[16];
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, (uint8_t *)input.data(), input.size());
    MD5_Final(result, &ctx);

    char output[33];
    for (int i = 0; i < 16; ++i){
        sprintf(&output[i * 2], "%02X", result[i]);
    }

    return output;
}

char * copy_string(const char * s){
    if (s){
        auto len = strlen(s);
        auto result = new char[len + 1];
        memcpy(result, s, len + 1);
        return result;
    } else {
        return nullptr;
    }
}

void * copy_sockaddr(const void * sa){
    if (sa){
        if (static_cast<const sockaddr *>(sa)->sa_family == AF_INET){
            auto result = new char[sizeof(sockaddr_in)];
            memcpy(result, sa, sizeof(sockaddr_in));
            return result;
        } else {
            // LATER IPv6
            return nullptr;
        }
    } else {
        return nullptr;
    }
}

void free_sockaddr(const void * sa){
    delete static_cast<const sockaddr *>(sa);
}

} // net
} // aoo

/*//////////////////// OSC ////////////////////////////*/

int32_t aoonet_parse_pattern(const char *msg, int32_t n, int32_t *type)
{
    int32_t offset = 0;
    if (n >= AOO_MSG_DOMAIN_LEN
            && !memcmp(msg, AOO_MSG_DOMAIN, AOO_MSG_DOMAIN_LEN))
    {
        offset += AOO_MSG_DOMAIN_LEN;
        if (n >= (offset + AOONET_MSG_SERVER_LEN)
            && !memcmp(msg + offset, AOONET_MSG_SERVER, AOONET_MSG_SERVER_LEN))
        {
            *type = AOO_TYPE_SERVER;
            return offset + AOONET_MSG_SERVER_LEN;
        }
        else if (n >= (offset + AOONET_MSG_CLIENT_LEN)
            && !memcmp(msg + offset, AOONET_MSG_CLIENT, AOONET_MSG_CLIENT_LEN))
        {
            *type = AOO_TYPE_CLIENT;
            return offset + AOONET_MSG_CLIENT_LEN;
        }
        else if (n >= (offset + AOONET_MSG_PEER_LEN)
            && !memcmp(msg + offset, AOONET_MSG_PEER, AOONET_MSG_PEER_LEN))
        {
            *type = AOO_TYPE_PEER;
            return offset + AOONET_MSG_PEER_LEN;
        } else {
            return 0;
        }
    } else {
        return 0; // not an AoO message
    }
}

/*//////////////////// AoO client /////////////////////*/

aoonet_client * aoonet_client_new(void *udpsocket, aoo_sendfn fn, int port) {
    return new aoo::net::client(udpsocket, fn, port);
}

aoo::net::client::client(void *udpsocket, aoo_sendfn fn, int port)
    : udpsocket_(udpsocket), sendfn_(fn), udpport_(port)
{
#ifdef _WIN32
    sockevent_ = CreateEvent(NULL, FALSE, FALSE, NULL);
    waitevent_ = CreateEvent(NULL, FALSE, FALSE, NULL);
#else
    if (pipe(waitpipe_) != 0){
        // TODO handle error
    }
#endif
    commands_.resize(256, 1);
    events_.resize(256, 1);
    sendbuffer_.setup(65536);
    recvbuffer_.setup(65536);
    
    // generate random token
    std::random_device randdev;
    std::default_random_engine reng(randdev());
    std::uniform_int_distribution<int64_t> uniform_dist(1); // minimum of 1, max of maxint
    token_ = uniform_dist(reng);
}

void aoonet_client_free(aoonet_client *client){
    // cast to correct type because base class
    // has no virtual destructor!
    delete static_cast<aoo::net::client *>(client);
}

aoo::net::client::~client() {
    do_disconnect();

#ifdef _WIN32
    CloseHandle(sockevent_);
    CloseHandle(waitevent_);
#else
    close(waitpipe_[0]);
    close(waitpipe_[1]);
#endif
}

int32_t aoonet_client_run(aoonet_client *client){
    return client->run();
}

int32_t aoo::net::client::run(){
    start_time_ = time_tag::now();

    while (!quit_.load()){
        double timeout = 0;

        time_tag now = time_tag::now();
        auto elapsed_time = time_tag::duration(start_time_, now);

        if (tcpsocket_ >= 0 && state_.load() == client_state::connected){
            auto delta = elapsed_time - last_tcp_ping_time_;
            auto ping_interval = ping_interval_.load();
            if (delta >= ping_interval){
                // send ping
                if (tcpsocket_ >= 0){
                    char buf[64];
                    osc::OutboundPacketStream msg(buf, sizeof(buf));
                    msg << osc::BeginMessage(AOONET_MSG_SERVER_PING)
                        << osc::EndMessage;

                    send_server_message_tcp(msg.Data(), (int32_t) msg.Size());
                } else {
                    LOG_ERROR("aoo_client: bug send_ping()");
                }

                last_tcp_ping_time_ = elapsed_time;
                timeout = ping_interval;
            } else {
                timeout = ping_interval - delta;
            }
        } else {
            timeout = -1;
        }

        wait_for_event(timeout);

        // handle commands
        while (commands_.read_available()){
            std::unique_ptr<icommand> cmd;
            commands_.read(cmd);
            cmd->perform(*this);
        }
    }
    return 1;
}

int32_t aoonet_client_quit(aoonet_client *client){
    return client->quit();
}

int32_t aoo::net::client::quit(){
    quit_.store(true);
    signal();
    return 1;
}

int32_t aoonet_client_connect(aoonet_client *client, const char *host, int port,
                           const char *username, const char *pwd)
{
    return client->connect(host, port, username, pwd);
}

int32_t aoo::net::client::connect(const char *host, int port,
                             const char *username, const char *pwd)
{
    auto state = state_.load();
    if (state != client_state::disconnected){
        if (state == client_state::connected){
            LOG_ERROR("aoo_client: already connected!");
        } else {
            LOG_ERROR("aoo_client: already connecting!");
        }
        return 0;
    }

    username_ = username;
    password_ = encrypt(pwd);

    state_ = client_state::connecting;

    push_command(std::make_unique<connect_cmd>(host, port));

    signal();

    return 1;
}

int32_t aoonet_client_disconnect(aoonet_client *client){
    return client->disconnect();
}

int32_t aoo::net::client::disconnect(){
    auto state = state_.load();
    if (state != client_state::connected){
        LOG_WARNING("aoo_client: not connected");
        return 0;
    }

    push_command(std::make_unique<disconnect_cmd>(command_reason::user));

    signal();

    return 1;
}

int32_t aoonet_client_group_join(aoonet_client *client, const char *group, const char *pwd){
    return client->group_join(group, pwd);
}

int32_t aoonet_client_group_join_public(aoonet_client *client, const char *group, const char *pwd){
    return client->group_join(group, pwd, true);
}

int32_t aoo::net::client::group_join(const char *group, const char *pwd, bool is_public){
    push_command(std::make_unique<group_join_cmd>(group, encrypt(pwd), is_public));

    signal();

    return 1;
}

int32_t aoonet_client_group_leave(aoonet_client *client, const char *group){
    return client->group_leave(group);
}

int32_t aoo::net::client::group_leave(const char *group){
    push_command(std::make_unique<group_leave_cmd>(group));

    signal();

    return 1;
}

int32_t aoonet_client_group_watch_public(aoonet_client *client, bool watch){
    return client->group_watch_public(watch);
}

int32_t aoo::net::client::group_watch_public(bool watch) {
    push_command(std::make_unique<group_watchpublic_cmd>(watch));

    signal();

    return 1;
}

int32_t aoonet_client_handle_message(aoonet_client *client, const char *data,
                                     int32_t n, void *addr)
{
    return client->handle_message(data, n, addr);
}

int32_t aoo::net::client::handle_message(const char *data, int32_t n, void *addr){
    if (static_cast<struct sockaddr *>(addr)->sa_family != AF_INET){
        return 0;
    }
    try {
        osc::ReceivedPacket packet(data, n);
        osc::ReceivedMessage msg(packet);

        int32_t type;
        auto onset = aoonet_parse_pattern(data, n, &type);
        if (!onset){
            LOG_WARNING("aoo_client: not an AOO NET message!");
            return 0;
        }

        ip_address address((struct sockaddr *)addr, sizeof(sockaddr_in)); // FIXME

        LOG_DEBUG("aoo_client: handle UDP message " << msg.AddressPattern()
            << " from " << address.name() << ":" << address.port());

        if (address == remote_addr_){
            // server message
            if (type != AOO_TYPE_CLIENT){
                LOG_WARNING("aoo_client: not a server message!");
                return 0;
            }
            handle_server_message_udp(msg, onset);
            return 1;
        } else {
            // peer message
            if (type != AOO_TYPE_PEER){
                LOG_WARNING("aoo_client: not a peer message!");
                return 0;
            }
            bool success = false;
            {
                shared_lock lock(peerlock_);
                // NOTE: we have to loop over *all* peers because there can
                // be more than 1 peer on a given IP endpoint, because a single
                // user can join multiple groups.
                // LATER make this more efficient, e.g. by associating IP endpoints
                // with peers instead of having them all in a single vector.

                auto pattern = msg.AddressPattern() + onset;
                int64_t token = 0;
                if (!strcmp(pattern, AOONET_MSG_PING) && msg.ArgumentCount() > 0){
                    token = msg.ArgumentsBegin()->AsInt64();
                }
                        
                for (auto& p : peers_){
                    if (p->match(address)){
                        p->handle_message(msg, onset, address);
                        success = true;
                    } else if (!p->has_real_address() && token > 0 && p->match_token(token)) {
                        // this message doesn't match one of the addresses given by the server for this peer
                        // but it DOES match the random token for the peer, which means we might be dealing
                        // with a symmetric NAT for that peer. so we will assign the address here as the *real* address

                        LOG_VERBOSE("aoo_client: found matching token, changing public address for endpoint "
                                    << p->address().name() << ":" << p->address().port() << " TO "
                                    << address.name() << ":" << address.port());

                        p->set_public_address(address);
                        p->handle_message(msg, onset, address);
                        success = true;
                    }
                }
            }
            // NOTE: during the handshake process it is expected that
            // we receive UDP messages which we have to ignore:
            // a) pings from a peer which we haven't had the chance to add yet
            // b) pings sent to the other endpoint address
            if (!success){
                LOG_VERBOSE("aoo_client: ignoring UDP message "
                            << msg.AddressPattern() << " from endpoint "
                            << address.name() << ":" << address.port());
            }
        }
    } catch (const osc::Exception& e){
        LOG_ERROR("aoo_client: exception in handle_message: " << e.what());
    }

    return 0;
}

int32_t aoonet_client_send(aoonet_client *client){
    return client->send();
}

int32_t aoo::net::client::send(){
    auto state = state_.load();
    if (state != client_state::disconnected){
        time_tag now = time_tag::now();
        auto elapsed_time = time_tag::duration(start_time_, now);
        auto delta = elapsed_time - last_udp_ping_time_;

        if (state == client_state::handshake){
            // check for time out
            if (first_udp_ping_time_ == 0){
                first_udp_ping_time_ = elapsed_time;
            } else if ((elapsed_time - first_udp_ping_time_) > request_timeout()){
                // request has timed out!
                first_udp_ping_time_ = 0;

                push_command(std::make_unique<disconnect_cmd>(command_reason::timeout));

                signal();

                return 1; // ?
            }
            // send handshakes in fast succession
            if (delta >= request_interval()){
                char buf[64];
                osc::OutboundPacketStream msg(buf, sizeof(buf));
                msg << osc::BeginMessage(AOONET_MSG_SERVER_REQUEST) << osc::EndMessage;

                send_server_message_udp(msg.Data(), (int32_t) msg.Size());
                last_udp_ping_time_ = elapsed_time;
            }
        } else if (state == client_state::connected){
            // send regular pings
            if (delta >= ping_interval()){
                char buf[64];
                osc::OutboundPacketStream msg(buf, sizeof(buf));
                msg << osc::BeginMessage(AOONET_MSG_SERVER_PING)
                    << osc::EndMessage;

                send_server_message_udp(msg.Data(), (int32_t) msg.Size());
                last_udp_ping_time_ = elapsed_time;
            }
        } else {
            // ignore
            return 1;
        }

        // update peers
        shared_lock lock(peerlock_);
        for (auto& p : peers_){
            p->send(now);
        }
    }
    return 1;
}

int32_t aoonet_client_events_available(aoonet_server *client){
    return client->events_available();
}

int32_t aoo::net::client::events_available(){
    return 1;
}

int32_t aoonet_client_handle_events(aoonet_client *client, aoo_eventhandler fn, void *user){
    return client->handle_events(fn, user);
}

int32_t aoo::net::client::handle_events(aoo_eventhandler fn, void *user){
    // always thread-safe
    auto n = events_.read_available();
    if (n > 0){
        // copy events
        auto events = (ievent **)alloca(sizeof(ievent *) * n);
        auto vec = (const aoo_event **)alloca(sizeof(aoo_event *) * n); // adjusted pointers
        for (int i = 0; i < n; ++i){
            std::unique_ptr<ievent> ptr;
            events_.read(ptr);
            events[i] = ptr.release(); // get raw pointer
            vec[i] = &events[i]->event_; // adjust pointer
        }
        // send events
        fn(user, vec, n);
        // manually free events
        for (int i = 0; i < n; ++i){
            delete events[i];
        }
    }
    return n;
}

namespace aoo {
namespace net {

void client::do_connect(const std::string &host, int port)
{
    if (tcpsocket_ >= 0){
        LOG_ERROR("aoo_client: bug client::do_connect()");
        return;
    }

    int err = try_connect(host, port);
    if (err != 0){
        // event
        std::string errmsg = socket_strerror(err);

        auto e = std::make_unique<event>(
            AOONET_CLIENT_CONNECT_EVENT, 0, errmsg.c_str());
        push_event(std::move(e));

        do_disconnect();
        return;
    }

    first_udp_ping_time_ = 0;
    state_ = client_state::handshake;

}

void client::do_disconnect(command_reason reason, int error){
    if (tcpsocket_ >= 0){
    #ifdef _WIN32
        // unregister event from socket.
        // actually, I think this happens automatically when closing the socket.
        WSAEventSelect(tcpsocket_, sockevent_, 0);
    #endif
        socket_close(tcpsocket_);
        tcpsocket_ = -1;
        LOG_VERBOSE("aoo_client: disconnected");
    }

    {
        unique_lock lock(peerlock_);
        peers_.clear();
    }

    // event
    if (reason != command_reason::none){
        if (reason == command_reason::user){
            auto e = std::make_unique<event>(
                AOONET_CLIENT_DISCONNECT_EVENT, 1);
            push_event(std::move(e));
        } else {
            std::string errmsg;
            if (reason == command_reason::timeout) {
                errmsg = "timed out";
            } else {
                if (error == 0){
                    errmsg = "disconnected from server";
                } else {
                    errmsg = socket_strerror(error);
                }
            }
            auto e = std::make_unique<event>(
                AOONET_CLIENT_DISCONNECT_EVENT, 0, errmsg.c_str());
            push_event(std::move(e));
        }
    }

    state_ = client_state::disconnected;
}

int client::try_connect(const std::string &host, int port){
    tcpsocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpsocket_ < 0){
        int err = socket_errno();
        LOG_ERROR("aoo_client: couldn't create socket (" << err << ")");
        return err;
    }
    // resolve host name
    struct hostent *he = gethostbyname(host.c_str());
    if (!he){
        int err = socket_errno();
        LOG_ERROR("aoo_client: couldn't connect (" << err << ")");
        return err;
    }

    // copy IP address
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    memcpy(&sa.sin_addr, he->h_addr_list[0], he->h_length);

    remote_addr_ = ip_address((struct sockaddr *)&sa, sizeof(sa));

    // set TCP_NODELAY
    int val = 1;
    if (setsockopt(tcpsocket_, IPPROTO_TCP, TCP_NODELAY, (char *)&val, sizeof(val)) < 0){
        LOG_WARNING("aoo_client: couldn't set TCP_NODELAY");
        // ignore
    }

    // try to connect (LATER make timeout configurable)
    if (socket_connect(tcpsocket_, remote_addr_, 5) < 0){
        int err = socket_errno();
        LOG_ERROR("aoo_client: couldn't connect (" << err << ")");
        return err;
    }

    // get local network interface
    ip_address tmp;
    if (getsockname(tcpsocket_,
                    (struct sockaddr *)&tmp.address, &tmp.length) < 0) {
        int err = socket_errno();
        LOG_ERROR("aoo_client: couldn't get socket name (" << err << ")");
        return err;
    }
    local_addr_ = ip_address(tmp.name(), udpport_);

#ifdef _WIN32
    // register event with socket
    WSAEventSelect(tcpsocket_, sockevent_, FD_READ | FD_WRITE | FD_CLOSE);
#else
    // set non-blocking
    // (this is not necessary on Windows, since WSAEventSelect will do it automatically)
    val = 1;
    if (ioctl(tcpsocket_, FIONBIO, (char *)&val) < 0){
        int err = socket_errno();
        LOG_ERROR("aoo_client: couldn't set socket to non-blocking (" << err << ")");
        return err;
    }
#endif

    LOG_VERBOSE("aoo_client: successfully connected to "
                << remote_addr_.name() << " on port " << remote_addr_.port());
    LOG_VERBOSE("aoo_client: local address: " << local_addr_.name());

    return 0;
}

void client::do_login(){
    char buf[AOO_MAXPACKETSIZE];
    osc::OutboundPacketStream msg(buf, sizeof(buf));
    msg << osc::BeginMessage(AOONET_MSG_SERVER_LOGIN)
        << username_.c_str() << password_.c_str()
        << public_addr_.name().c_str() << public_addr_.port()
        << local_addr_.name().c_str() << local_addr_.port()
        << token_
        << osc::EndMessage;

    send_server_message_tcp(msg.Data(), (int32_t) msg.Size());
}

void client::do_group_join(const std::string &group, const std::string &pwd, bool is_public){
    char buf[AOO_MAXPACKETSIZE];
    osc::OutboundPacketStream msg(buf, sizeof(buf));
    msg << osc::BeginMessage(AOONET_MSG_SERVER_GROUP_JOIN)
        << group.c_str() << pwd.c_str() << is_public << osc::EndMessage;

    send_server_message_tcp(msg.Data(), (int32_t) msg.Size());
}

void client::do_group_leave(const std::string &group){
    char buf[AOO_MAXPACKETSIZE];
    osc::OutboundPacketStream msg(buf, sizeof(buf));
    msg << osc::BeginMessage(AOONET_MSG_SERVER_GROUP_LEAVE)
        << group.c_str() << osc::EndMessage;

    send_server_message_tcp(msg.Data(), (int32_t) msg.Size());
}

void client::do_group_watch_public(bool watch){
    char buf[AOO_MAXPACKETSIZE];
    osc::OutboundPacketStream msg(buf, sizeof(buf));
    msg << osc::BeginMessage(AOONET_MSG_SERVER_GROUP_PUBLIC)
        << watch << osc::EndMessage;

    send_server_message_tcp(msg.Data(), (int32_t) msg.Size());
}

void client::send_message_udp(const char *data, int32_t size, const ip_address& addr)
{
    sendfn_(udpsocket_, data, size, (void *)&addr.address);
}

void client::push_event(std::unique_ptr<ievent> e)
{
    scoped_lock<spinlock> lock(event_lock_);
    if (events_.write_available()){
        events_.write(std::move(e));
    }
}

void client::wait_for_event(float timeout){
    LOG_DEBUG("aoo_client: wait " << timeout << " seconds");
#ifdef _WIN32
    HANDLE events[2];
    int numevents;
    events[0] = waitevent_;
    if (tcpsocket_ >= 0){
        events[1] = sockevent_;
        numevents = 2;
    } else {
        numevents = 1;
    }

    DWORD time = timeout < 0 ? INFINITE : (timeout * 1000 + 0.5); // round up to 1 ms!
    DWORD result = WaitForMultipleObjects(numevents, events, FALSE, time);
    if (result == WAIT_TIMEOUT){
        LOG_DEBUG("aoo_server: timed out");
        return;
    }
    // only the second event is a socket
    if (result - WAIT_OBJECT_0 == 1){
        WSANETWORKEVENTS ne;
        memset(&ne, 0, sizeof(ne));
        WSAEnumNetworkEvents(tcpsocket_, sockevent_, &ne);

        if (ne.lNetworkEvents & FD_READ){
            // ready to receive data from server
            receive_data();
        } else if (ne.lNetworkEvents & FD_CLOSE){
            // connection was closed
            int err = ne.iErrorCode[FD_CLOSE_BIT];
            LOG_WARNING("aoo_client: connection was closed (" << err << ")");

            do_disconnect(command_reason::error, err);
        } else {
            // ignore FD_WRITE event
        }
    }
#else
#if 1 // poll() version
    struct pollfd fds[2];
    fds[0].fd = waitpipe_[0];
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    fds[1].fd = tcpsocket_;
    fds[1].events = POLLIN;
    fds[1].revents = 0;

    // round up to 1 ms! -1: block indefinitely
    // NOTE: macOS requires the negative timeout to exactly -1!
    int result = poll(fds, 2, timeout < 0 ? -1 : timeout * 1000.0 + 0.5);
    if (result < 0){
        int err = errno;
        if (err == EINTR){
            // ?
        } else {
            LOG_ERROR("aoo_client: poll failed (" << err << ")");
            // what to do?
        }
        return;
    }

    if (fds[0].revents & POLLIN){
        // clear pipe
        char c;
        read(waitpipe_[0], &c, 1);
    }

    if (fds[1].revents & POLLIN){
        receive_data();
    }
#else // select() version
    fd_set rdset;
    FD_ZERO(&rdset);
    int fdmax = std::max<int>(waitpipe_[0], tcpsocket_); // tcpsocket might be -1
    FD_SET(waitpipe_[0], &rdset);
    if (tcpsocket_ >= 0){
        FD_SET(tcpsocket_, &rdset);
    }

    struct timeval time;
    time.tv_sec = (time_t)timeout;
    time.tv_usec = (timeout - (double)time.tv_sec) * 1000000;

    if (select(fdmax + 1, &rdset, 0, 0, timeout < 0 ? nullptr : &time) < 0){
        int err = errno;
        if (err == EINTR){
            // ?
        } else {
            LOG_ERROR("aoo_client: select failed (" << err << ")");
            // what to do?
        }
        return;
    }

    if (FD_ISSET(waitpipe_[0], &rdset)){
        // clear pipe
        char c;
        read(waitpipe_[0], &c, 1);
    }

    if (FD_ISSET(tcpsocket_, &rdset)){
        receive_data();
    }
#endif
#endif
}

void client::receive_data(){
    // read as much data as possible until recv() would block
    while (true){
        char buffer[AOO_MAXPACKETSIZE];
        auto result = recv(tcpsocket_, buffer, sizeof(buffer), 0);
        if (result > 0){
            recvbuffer_.write_bytes((uint8_t *)buffer, (int32_t) result);

            // handle packets
            uint8_t buf[AOO_MAXPACKETSIZE];
            while (true){
                auto size = recvbuffer_.read_packet(buf, sizeof(buf));
                if (size > 0){
                    try {
                        osc::ReceivedPacket packet((char *)buf, size);

                        std::function<void(const osc::ReceivedBundle&)> dispatchBundle
                                = [&](const osc::ReceivedBundle& bundle){
                            auto it = bundle.ElementsBegin();
                            while (it != bundle.ElementsEnd()){
                                if (it->IsMessage()){
                                    osc::ReceivedMessage msg(*it);
                                    handle_server_message_tcp(msg);
                                } else if (it->IsBundle()){
                                    osc::ReceivedBundle bundle2(*it);
                                    dispatchBundle(bundle2);
                                } else {
                                    // ignore
                                }
                                ++it;
                            }
                        };

                        if (packet.IsMessage()){
                            osc::ReceivedMessage msg(packet);
                            handle_server_message_tcp(msg);
                        } else if (packet.IsBundle()){
                            osc::ReceivedBundle bundle(packet);
                            dispatchBundle(bundle);
                        } else {
                            // ignore
                        }
                    } catch (const osc::Exception& e){
                        LOG_ERROR("aoo_client: exception in receive_data: " << e.what());
                    }
                } else {
                    break;
                }
            }
        } else if (result == 0){
            // connection closed by the remote server
            do_disconnect(command_reason::error, 0);
        } else {
            int err = socket_errno();
        #ifdef _WIN32
            if (err == WSAEWOULDBLOCK)
        #else
            if (err == EWOULDBLOCK)
        #endif
            {
            #if 0
                LOG_VERBOSE("aoo_client: recv() would block");
            #endif
            } else {
                LOG_ERROR("aoo_client: recv() failed (" << err << ")");
                do_disconnect(command_reason::error, err);
            }
            return;
        }
    }
}

void client::send_server_message_tcp(const char *data, int32_t size){
    if (tcpsocket_ >= 0){
        if (sendbuffer_.write_packet((const uint8_t *)data, size)){
            // try to send as much as possible until send() would block
            while (true){
                uint8_t buf[1024];
                int32_t total = 0;
                // first try to send pending data
                if (!pending_send_data_.empty()){
                     std::copy(pending_send_data_.begin(), pending_send_data_.end(), buf);
                     total = (int32_t) pending_send_data_.size();
                     pending_send_data_.clear();
                } else if (sendbuffer_.read_available()){
                     total = sendbuffer_.read_bytes(buf, sizeof(buf));
                } else {
                    break;
                }

                int32_t nbytes = 0;
                while (nbytes < total){
                    auto res = ::send(tcpsocket_, (char *)buf + nbytes, total - nbytes, 0);
                    if (res >= 0){
                        nbytes += res;
                    #if 0
                        LOG_VERBOSE("aoo_client: sent " << res << " bytes");
                    #endif
                    } else {
                        auto err = socket_errno();
                    #ifdef _WIN32
                        if (err == WSAEWOULDBLOCK)
                    #else
                        if (err == EWOULDBLOCK)
                    #endif
                        {
                            // store in pending buffer
                            pending_send_data_.assign(buf + nbytes, buf + total);
                        #if 1
                            LOG_VERBOSE("aoo_client: send() would block");
                        #endif
                        }
                        else
                        {
                            do_disconnect(command_reason::error, err);
                            LOG_ERROR("aoo_client: send() failed (" << err << ")");
                        }
                        return;
                    }
                }
            }
            LOG_DEBUG("aoo_client: sent " << data << " to server");
        } else {
            LOG_ERROR("aoo_client: couldn't send " << data << " to server");
        }
    } else {
        LOG_ERROR("aoo_client: can't send server message - socket closed!");
    }
}

void client::send_server_message_udp(const char *data, int32_t size)
{
    send_message_udp(data, size, remote_addr_);
}

void client::handle_server_message_tcp(const osc::ReceivedMessage& msg){
    // first check main pattern
    int32_t len = (int32_t) strlen(msg.AddressPattern());
    int32_t onset = AOO_MSG_DOMAIN_LEN + AOONET_MSG_CLIENT_LEN;

    if ((len < onset) ||
        memcmp(msg.AddressPattern(), AOO_MSG_DOMAIN AOONET_MSG_CLIENT, onset))
    {
        LOG_ERROR("aoo_client: received bad message " << msg.AddressPattern()
                  << " from server");
        return;
    }

    // now compare subpattern
    auto pattern = msg.AddressPattern() + onset;
    LOG_DEBUG("aoo_client: got message " << pattern << " from server");

    try {
        if (!strcmp(pattern, AOONET_MSG_PING)){
            LOG_DEBUG("aoo_client: got TCP ping from server");
        } else if (!strcmp(pattern, AOONET_MSG_LOGIN)){
            handle_login(msg);
        } else if (!strcmp(pattern, AOONET_MSG_GROUP_JOIN)){
            handle_group_join(msg);
        } else if (!strcmp(pattern, AOONET_MSG_GROUP_LEAVE)){
            handle_group_leave(msg);
        } else if (!strcmp(pattern, AOONET_MSG_GROUP_PUBLIC)){
            //
        } else if (!strcmp(pattern, AOONET_MSG_GROUP_PUBLIC_ADD)){
            handle_public_group_add(msg);
        } else if (!strcmp(pattern, AOONET_MSG_GROUP_PUBLIC_DEL)){
            handle_public_group_del(msg);
        } else if (!strcmp(pattern, AOONET_MSG_PEER_JOIN)){
            handle_peer_add(msg);
        } else if (!strcmp(pattern, AOONET_MSG_PEER_LEAVE)){
            handle_peer_remove(msg);
        } else {
            LOG_ERROR("aoo_client: unknown server message " << pattern);
        }
    } catch (const osc::Exception& e){
        LOG_ERROR("aoo_client: exception on handling " << pattern
                  << " message: " << e.what());
    }
}

void client::handle_login(const osc::ReceivedMessage& msg){
    if (state_.load() == client_state::login){
        auto it = msg.ArgumentsBegin();
        int32_t status = (it++)->AsInt32();
        if (status > 0){
            // connected!
            state_ = client_state::connected;
            LOG_VERBOSE("aoo_client: successfully logged in");
            // event
            auto e = std::make_unique<event>(
                AOONET_CLIENT_CONNECT_EVENT, 1);
            push_event(std::move(e));
        } else {
            std::string errmsg;
            if (msg.ArgumentCount() > 1){
                errmsg = (it++)->AsString();
            } else {
                errmsg = "unknown error";
            }
            LOG_WARNING("aoo_client: login failed: " << errmsg);

            // event
            auto e = std::make_unique<event>(
                AOONET_CLIENT_CONNECT_EVENT, status, errmsg.c_str());
            push_event(std::move(e));

            do_disconnect();
        }
    }
}

void client::handle_group_join(const osc::ReceivedMessage& msg){
    auto it = msg.ArgumentsBegin();
    std::string group = (it++)->AsString();
    int32_t status = (it++)->AsInt32();
    if (status > 0){
        LOG_VERBOSE("aoo_client: successfully joined group " << group);
        auto e = std::make_unique<group_event>(
            AOONET_CLIENT_GROUP_JOIN_EVENT, group.c_str(), 1);
        push_event(std::move(e));
    } else {
        std::string errmsg;
        if (msg.ArgumentCount() > 2){
            errmsg = (it++)->AsString();
            LOG_WARNING("aoo_client: couldn't join group "
                        << group << ": " << errmsg);
        } else {
            errmsg = "unknown error";
        }
        // event
        auto e = std::make_unique<group_event>(
            AOONET_CLIENT_GROUP_JOIN_EVENT, group.c_str(), status, errmsg.c_str());
        push_event(std::move(e));
    }
}

void client::handle_group_leave(const osc::ReceivedMessage& msg){
    auto it = msg.ArgumentsBegin();
    std::string group = (it++)->AsString();
    int32_t status = (it++)->AsInt32();
    if (status > 0){
        LOG_VERBOSE("aoo_client: successfully left group " << group);

        // remove all peers from this group
        unique_lock lock(peerlock_); // writer lock!
        auto result = std::remove_if(peers_.begin(), peers_.end(),
                                     [&](auto& p){ return p->group() == group; });
        peers_.erase(result, peers_.end());

        auto e = std::make_unique<group_event>(
            AOONET_CLIENT_GROUP_LEAVE_EVENT, group.c_str(), 1);
        push_event(std::move(e));
    } else {
        std::string errmsg;
        if (msg.ArgumentCount() > 2){
            errmsg = (it++)->AsString();
            LOG_WARNING("aoo_client: couldn't leave group "
                        << group << ": " << errmsg);
        } else {
            errmsg = "unknown error";
        }
        // event
        auto e = std::make_unique<group_event>(
            AOONET_CLIENT_GROUP_LEAVE_EVENT, group.c_str(), status, errmsg.c_str());
        push_event(std::move(e));
    }
}

void client::handle_public_group_add(const osc::ReceivedMessage& msg){
    auto it = msg.ArgumentsBegin();
    std::string group = (it++)->AsString();
    int32_t usercnt = (it++)->AsInt32();

    LOG_VERBOSE("aoo_client: public group add/changed " << group << " users: " << usercnt);

    auto e = std::make_unique<group_event>(AOONET_CLIENT_GROUP_PUBLIC_ADD_EVENT, group.c_str(), usercnt);
    push_event(std::move(e));
}

void client::handle_public_group_del(const osc::ReceivedMessage& msg){
    auto it = msg.ArgumentsBegin();
    std::string group = (it++)->AsString();

    LOG_VERBOSE("aoo_client: public group deleted " << group);

    auto e = std::make_unique<group_event>(AOONET_CLIENT_GROUP_PUBLIC_DEL_EVENT, group.c_str(), 0);
    push_event(std::move(e));
}


void client::handle_peer_add(const osc::ReceivedMessage& msg){
    auto it = msg.ArgumentsBegin();
    std::string group = (it++)->AsString();
    std::string user = (it++)->AsString();
    std::string public_ip = (it++)->AsString();
    int32_t public_port = (it++)->AsInt32();
    std::string local_ip = (it++)->AsString();
    int32_t local_port = (it++)->AsInt32();
    int64_t token = msg.ArgumentCount() > 6 ? (it++)->AsInt64() : 0;
    
    ip_address public_addr(public_ip, public_port);
    ip_address local_addr(local_ip, local_port);

    unique_lock lock(peerlock_); // writer lock!

    // check if peer already exists (shouldn't happen)
    for (auto& p: peers_){
        if (p->match(group, user)){
            LOG_ERROR("aoo_client: peer " << *p << " already added");
            return;
        }
    }
    peers_.push_back(std::make_unique<peer>(*this, group, user,
                                            public_addr, local_addr, token));

    // push prejoin event, real join event will be sent after handshake and real address is discovered
    
    auto e = std::make_unique<client::peer_event>(
                AOONET_CLIENT_PEER_PREJOIN_EVENT,
                group.c_str(), user.c_str(), nullptr, 0);
    push_event(std::move(e));

    
    LOG_VERBOSE("aoo_client: new peer " << *peers_.back()
                << " (" << public_ip << ":" << public_port << ") token: " << token);
}

void client::handle_peer_remove(const osc::ReceivedMessage& msg){
    auto it = msg.ArgumentsBegin();
    std::string group = (it++)->AsString();
    std::string user = (it++)->AsString();

    unique_lock lock(peerlock_); // writer lock!

    auto result = std::find_if(peers_.begin(), peers_.end(),
                               [&](auto& p){ return p->match(group, user); });
    if (result == peers_.end()){
        LOG_ERROR("aoo_client: couldn't remove " << group << "|" << user);
        return;
    }

    ip_address addr = (*result)->address();

    peers_.erase(result);

    auto e = std::make_unique<peer_event>(
                AOONET_CLIENT_PEER_LEAVE_EVENT,
                group.c_str(), user.c_str(), &addr.address, addr.length);
    push_event(std::move(e));

    LOG_VERBOSE("aoo_client: peer " << group << "|" << user << " left");
}

void client::handle_server_message_udp(const osc::ReceivedMessage &msg, int onset){
    auto pattern = msg.AddressPattern() + onset;
    try {
        if (!strcmp(pattern, AOONET_MSG_PING)){
            LOG_DEBUG("aoo_client: got UDP ping from server");
        } else if (!strcmp(pattern, AOONET_MSG_REPLY)){
            client_state expected = client_state::handshake;
            if (state_.compare_exchange_strong(expected, client_state::login)){
                // retrieve public IP + port
                auto it = msg.ArgumentsBegin();
                std::string ip = (it++)->AsString();
                int port = (it++)->AsInt32();
                public_addr_ = ip_address(ip, port);
                LOG_VERBOSE("aoo_client: public endpoint is "
                            << public_addr_.name() << " " << public_addr_.port());

                // now we can try to login
                push_command(std::make_unique<login_cmd>());

                signal();
            }
        } else {
            LOG_WARNING("aoo_client: received unknown UDP message "
                        << pattern << " from server");
        }
    } catch (const osc::Exception& e){
        LOG_ERROR("aoo_client: exception on handling " << pattern
                  << " message: " << e.what());
    }
}

void client::signal(){
#ifdef _WIN32
    SetEvent(waitevent_);
#else
    write(waitpipe_[1], "\0", 1);
#endif
}


/*///////////////////// events ////////////////////////*/

client::event::event(int32_t type, int32_t result,
                     const char * errmsg)
{
    client_event_.type = type;
    client_event_.result = result;
    client_event_.errormsg = copy_string(errmsg);
}

client::event::~event()
{
    delete client_event_.errormsg;
}

client::group_event::group_event(int32_t type, const char *name,
                                 int32_t result, const char *errmsg)
{
    group_event_.type = type;
    group_event_.result = result;
    group_event_.errormsg = copy_string(errmsg);
    group_event_.name = copy_string(name);
}

client::group_event::~group_event()
{
    delete group_event_.errormsg;
    delete group_event_.name;
}

client::peer_event::peer_event(int32_t type,
                               const char *group, const char *user,
                               const void *address, int32_t length)
{
    peer_event_.type = type;
    peer_event_.result = 1;
    peer_event_.errormsg = nullptr;
    peer_event_.group = copy_string(group);
    peer_event_.user = copy_string(user);
    peer_event_.address = copy_sockaddr(address);
    peer_event_.length = length;
}

client::peer_event::~peer_event()
{
    delete peer_event_.user;
    delete peer_event_.group;
    if (peer_event_.address) {
        free_sockaddr(peer_event_.address);
    }
}

/*///////////////////// peer //////////////////////////*/

peer::peer(client& client,
           const std::string& group, const std::string& user,
           const ip_address& public_addr, const ip_address& local_addr, int64_t token)
    : client_(&client), group_(group), user_(user),
      public_address_(public_addr), local_address_(local_addr), token_(token)
{
    start_time_ = time_tag::now();

    LOG_VERBOSE("create peer " << *this);
}

peer::~peer(){
    LOG_VERBOSE("destroy peer " << *this);
}

bool peer::match(const ip_address &addr) const {
    auto real_addr = address_.load();
    if (real_addr){
        return *real_addr == addr;
    } else {
        return public_address_ == addr || local_address_ == addr;
    }
}

bool peer::match(const std::string& group, const std::string& user)
{
    return group_ == group && user_ == user;
}

bool peer::match_token(int64_t token) const
{
    return token_ == token;
}

void peer::set_public_address(const ip_address & addr)
{
    public_address_ = addr;
}


std::ostream& operator << (std::ostream& os, const peer& p)
{
    os << p.group_ << "|" << p.user_;
    return os;
}

void peer::send(time_tag now){
    auto elapsed_time = time_tag::duration(start_time_, now);
    auto delta = elapsed_time - last_pingtime_;

    auto real_addr = address_.load();
    if (real_addr){
        // send regular ping if it is time, or if we have never sent one (handles race condition on initial peer join)
        if (delta >= client_->ping_interval() || last_pingtime_ <= 0){
            char buf[64];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            msg << osc::BeginMessage(AOONET_MSG_PEER_PING) << osc::EndMessage;

            client_->send_message_udp(msg.Data(), (int32_t) msg.Size(), *real_addr);
            LOG_DEBUG("send regular ping to " << *this);

            last_pingtime_ = elapsed_time;
        }
    } else if (!timeout_) {
        // try to establish UDP connection with peer
        if (elapsed_time > client_->request_timeout()){
            // couldn't establish peer connection!
            LOG_ERROR("aoo_client: couldn't establish UDP connection to "
                      << *this << "; timed out after "
                      << client_->request_timeout() << " seconds");
            timeout_ = true;


            // this at least lets us present to the user that a particular user@group failed to establish
            auto e = std::make_unique<client::peer_event>(
                        AOONET_CLIENT_PEER_JOINFAIL_EVENT,
                        group().c_str(), user().c_str(), nullptr, 0);
            client_->push_event(std::move(e));
           
            return;
        }
        // send handshakes in fast succession to *both* addresses
        // until we get a reply from one of them (see handle_message())
        if (delta >= client_->request_interval()){
            char buf[80];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            msg << osc::BeginMessage(AOONET_MSG_PEER_PING) << client_->get_token() << osc::EndMessage;

            client_->send_message_udp(msg.Data(), (int32_t) msg.Size(), local_address_);
            client_->send_message_udp(msg.Data(), (int32_t) msg.Size(), public_address_);

            LOG_DEBUG("send ping to " << *this);

            last_pingtime_ = elapsed_time;
        }
    }
}

void peer::handle_message(const osc::ReceivedMessage &msg, int onset,
                          const ip_address& addr)
{
    auto pattern = msg.AddressPattern() + onset;
    try {
        if (!strcmp(pattern, AOONET_MSG_PING)){
            if (!address_.load()){
                // this is the first ping!
                if (addr == public_address_){
                    address_.store(&public_address_);
                } else if (addr == local_address_){
                    address_.store(&local_address_);
                } else {
                    LOG_ERROR("aoo_client: bug in peer::handle_message");
                    return;
                }

                // push event
                auto e = std::make_unique<client::peer_event>(
                            AOONET_CLIENT_PEER_JOIN_EVENT,
                            group().c_str(), user().c_str(), &addr.address, addr.length);
                client_->push_event(std::move(e));

                LOG_VERBOSE("aoo_client: successfully established connection with " << *this);
                
                // force last_pingtime_ to zero to make sure we ping them back immediately, avoiding race condition
                last_pingtime_ = 0;
                
            } else {
                LOG_DEBUG("aoo_client: got ping from " << *this);
                // maybe handle ping?
            }
        } else {
            LOG_WARNING("aoo_client: received unknown message "
                        << pattern << " from " << *this);
        }
    } catch (const osc::Exception& e){
        LOG_ERROR("aoo_client: " << *this << ": exception on handling "
                  << pattern << " message: " << e.what());
    }
}

} // net
} // aoo
