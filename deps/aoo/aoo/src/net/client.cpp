/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "client.hpp"

#include <cstring>
#include <functional>
#include <algorithm>
#include <sstream>

#include "md5/md5.h"

#ifndef _WIN32
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>
#endif

#define AOO_NET_MSG_SERVER_PING \
    AOO_MSG_DOMAIN AOO_NET_MSG_SERVER AOO_NET_MSG_PING

#define AOO_NET_MSG_PEER_PING \
    AOO_MSG_DOMAIN AOO_NET_MSG_PEER AOO_NET_MSG_PING

#define AOO_NET_MSG_PEER_REPLY \
    AOO_MSG_DOMAIN AOO_NET_MSG_PEER AOO_NET_MSG_REPLY

#define AOO_NET_MSG_PEER_MESSAGE \
    AOO_MSG_DOMAIN AOO_NET_MSG_PEER AOO_NET_MSG_MESSAGE

#define AOO_NET_MSG_SERVER_LOGIN \
    AOO_MSG_DOMAIN AOO_NET_MSG_SERVER AOO_NET_MSG_LOGIN

#define AOO_NET_MSG_SERVER_REQUEST \
    AOO_MSG_DOMAIN AOO_NET_MSG_SERVER AOO_NET_MSG_REQUEST

#define AOO_NET_MSG_SERVER_GROUP_JOIN \
    AOO_MSG_DOMAIN AOO_NET_MSG_SERVER AOO_NET_MSG_GROUP AOO_NET_MSG_JOIN

#define AOO_NET_MSG_SERVER_GROUP_LEAVE \
    AOO_MSG_DOMAIN AOO_NET_MSG_SERVER AOO_NET_MSG_GROUP AOO_NET_MSG_LEAVE

#define AOO_NET_MSG_GROUP_JOIN \
    AOO_NET_MSG_GROUP AOO_NET_MSG_JOIN

#define AOO_NET_MSG_GROUP_LEAVE \
    AOO_NET_MSG_GROUP AOO_NET_MSG_LEAVE

#define AOO_NET_MSG_PEER_JOIN \
    AOO_NET_MSG_PEER AOO_NET_MSG_JOIN

#define AOO_NET_MSG_PEER_LEAVE \
    AOO_NET_MSG_PEER AOO_NET_MSG_LEAVE

// debugging

#define FORCE_RELAY 0

#define DEBUG_RELAY 0

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

void copy_string(const std::string& src, char *dst, int32_t size){
    const auto limit = size - 1; // leave space for '\0'
    auto n = (src.size() > limit) ? limit : src.size();
    memcpy(dst, src.data(), n);
    dst[n] = '\0';
}

/*//////////////////// OSC ////////////////////////////*/

// optimized version of aoo_parse_pattern() for client/server
aoo_error parse_pattern(const char *msg, int32_t n, aoo_type& type, int32_t& offset)
{
    int32_t count = 0;
    if (n >= AOO_BIN_MSG_HEADER_SIZE &&
        !memcmp(msg, AOO_BIN_MSG_DOMAIN, AOO_BIN_MSG_DOMAIN_SIZE))
    {
        // domain (int32), type (int16), cmd (int16), id (int32) ...
        type = aoo::from_bytes<int16_t>(msg + 4);
        offset = 12;

        return AOO_OK;
    } else if (n >= AOO_MSG_DOMAIN_LEN
            && !memcmp(msg, AOO_MSG_DOMAIN, AOO_MSG_DOMAIN_LEN))
    {
        count += AOO_MSG_DOMAIN_LEN;
        if (n >= (count + AOO_NET_MSG_SERVER_LEN)
            && !memcmp(msg + count, AOO_NET_MSG_SERVER, AOO_NET_MSG_SERVER_LEN))
        {
            type = AOO_TYPE_SERVER;
            count += AOO_NET_MSG_SERVER_LEN;
        }
        else if (n >= (count + AOO_NET_MSG_CLIENT_LEN)
            && !memcmp(msg + count, AOO_NET_MSG_CLIENT, AOO_NET_MSG_CLIENT_LEN))
        {
            type = AOO_TYPE_CLIENT;
            count += AOO_NET_MSG_CLIENT_LEN;
        }
        else if (n >= (count + AOO_NET_MSG_PEER_LEN)
            && !memcmp(msg + count, AOO_NET_MSG_PEER, AOO_NET_MSG_PEER_LEN))
        {
            type = AOO_TYPE_PEER;
            count += AOO_NET_MSG_PEER_LEN;
        }
        else if (n >= (count + AOO_NET_MSG_RELAY_LEN)
            && !memcmp(msg + count, AOO_NET_MSG_RELAY, AOO_NET_MSG_RELAY_LEN))
        {
            type = AOO_TYPE_RELAY;
            count += AOO_NET_MSG_RELAY_LEN;
        } else {
            return AOO_ERROR_UNSPECIFIED;
        }

        offset = count;

        return AOO_OK;
    } else {
        return AOO_ERROR_UNSPECIFIED; // not an AoO message
    }
}

} // net
} // aoo

/*//////////////////// AoO client /////////////////////*/

aoo_net_client * aoo_net_client_new(const void *address, int32_t addrlen,
                                    uint32_t flags) {
    return aoo::construct<aoo::net::client_imp>(address, addrlen, flags);
}

aoo::net::client_imp::client_imp(const void *address, int32_t addrlen, uint32_t flags)
{
    ip_address addr((const sockaddr *)address, addrlen);
    udp_client_ = std::make_unique<udp_client>(*this, addr.port(), flags);
    type_ = addr.type();

    eventsocket_ = socket_udp(0);
    if (eventsocket_ < 0){
        // TODO handle error
        socket_error_print("socket_udp");
    }

    sendbuffer_.setup(65536);
    recvbuffer_.setup(65536);

    // commands_.reserve(256);
    // messages_.reserve(256);
    // events_.reserve(256);
}

void aoo_net_client_free(aoo_net_client *client){
    // cast to correct type because base class
    // has no virtual destructor!
    aoo::destroy(static_cast<aoo::net::client_imp *>(client));
}

aoo::net::client_imp::~client_imp() {
    if (socket_ >= 0){
        socket_close(socket_);
    }
}

aoo_error aoo_net_client_run(aoo_net_client *client){
    return client->run();
}

aoo_error aoo::net::client_imp::run(){
    start_time_ = time_tag::now();

    while (!quit_.load()){
        double timeout = 0;

        time_tag now = time_tag::now();
        auto elapsed_time = time_tag::duration(start_time_, now);

        if (state_.load() == client_state::connected){
            auto delta = elapsed_time - last_ping_time_;
            auto interval = ping_interval();
            if (delta >= interval){
                // send ping
                if (socket_ >= 0){
                    char buf[64];
                    osc::OutboundPacketStream msg(buf, sizeof(buf));
                    msg << osc::BeginMessage(AOO_NET_MSG_SERVER_PING)
                        << osc::EndMessage;

                    send_server_message(msg.Data(), msg.Size());
                } else {
                    LOG_ERROR("aoo_client: bug send_ping()");
                }

                last_ping_time_ = elapsed_time;
                timeout = interval;
            } else {
                timeout = interval - delta;
            }
        } else {
            timeout = -1;
        }

        if (!wait_for_event(timeout)){
            break;
        }

        // handle commands
        std::unique_ptr<icommand> cmd;
        while (commands_.try_pop(cmd)){
            cmd->perform(*this);
        }

        // handle messages
        std::unique_ptr<imessage> msg;
        while (tcp_messages_.try_pop(msg)){
            // call with dummy reply function
            msg->perform(*this, sendfn{});
        }

        if (!peers_.try_free()){
            LOG_DEBUG("aoo::client: try_free() would block");
        }
    }
    return AOO_OK;
}

aoo_error aoo_net_client_quit(aoo_net_client *client){
    return client->quit();
}

aoo_error aoo::net::client_imp::quit(){
    quit_.store(true);
    if (!signal()){
        // force wakeup by closing the socket.
        // this is not nice and probably undefined behavior,
        // the MSDN docs explicitly forbid it!
        socket_close(eventsocket_);
    }
    return AOO_OK;
}

aoo_error aoo_net_client_add_source(aoo_net_client *client,
                                    aoo_source *src, aoo_id id)
{
    return client->add_source(src, id);
}

aoo_error aoo::net::client_imp::add_source(source *src, aoo_id id)
{
#if 1
    for (auto& s : sources_){
        if (s.source == src){
            LOG_ERROR("aoo_client: source already added");
            return AOO_ERROR_UNSPECIFIED;
        } else if (s.id == id){
            LOG_WARNING("aoo_client: source with id " << id
                        << " already added!");
            return AOO_ERROR_UNSPECIFIED;
        }
    }
#endif
    sources_.push_back({ src, id });
    return AOO_OK;
}

aoo_error aoo_net_client_remove_source(aoo_net_client *client,
                                       aoo_source *src)
{
    return client->remove_source(src);
}

aoo_error aoo::net::client_imp::remove_source(source *src)
{
    for (auto it = sources_.begin(); it != sources_.end(); ++it){
        if (it->source == src){
            sources_.erase(it);
            return AOO_OK;
        }
    }
    LOG_ERROR("aoo_client: source not found");
    return AOO_ERROR_UNSPECIFIED;
}

aoo_error aoo_net_client_add_sink(aoo_net_client *client,
                                  aoo_sink *sink, aoo_id id)
{
    return client->add_sink(sink, id);
}

aoo_error aoo::net::client_imp::add_sink(sink *sink, aoo_id id)
{
#if 1
    for (auto& s : sinks_){
        if (s.sink == sink){
            LOG_ERROR("aoo_client: sink already added");
            return AOO_OK;
        } else if (s.id == id){
            LOG_WARNING("aoo_client: sink with id " << id
                        << " already added!");
            return AOO_ERROR_UNSPECIFIED;
        }
    }
#endif
    sinks_.push_back({ sink, id });
    return AOO_OK;
}

aoo_error aoo_net_client_remove_sink(aoo_net_client *client,
                                     aoo_sink *sink)
{
    return client->remove_sink(sink);
}

aoo_error aoo::net::client_imp::remove_sink(sink *sink)
{
    for (auto it = sinks_.begin(); it != sinks_.end(); ++it){
        if (it->sink == sink){
            sinks_.erase(it);
            return AOO_OK;
        }
    }
    LOG_ERROR("aoo_client: sink not found");
    return AOO_ERROR_UNSPECIFIED;
}

aoo_error aoo_net_client_get_peer_address(aoo_net_client *client,
                                          const char *group, const char *user,
                                          void *address, int32_t *addrlen, uint32_t *flags)
{
    return client->get_peer_address(group, user, address, addrlen, flags);
}

aoo_error aoo::net::client_imp::get_peer_address(const char *group, const char *user,
                                                 void *address, int32_t *addrlen, uint32_t *flags)
{
    peer_lock lock(peers_);
    for (auto& p : peers_){
        // we can only access the address if the peer is connected!
        if (p.match(group, user) && p.connected()){
            if (address){
                auto& addr = p.address();
                if (*addrlen < addr.length()){
                    return AOO_ERROR_UNSPECIFIED;
                }
                memcpy(address, addr.address(), addr.length());
                *addrlen = addr.length();
            }
            if (flags){
                *flags = p.flags();
            }
            return AOO_OK;
        }
    }
    return AOO_ERROR_UNSPECIFIED;
}

aoo_error aoo_net_client_get_peer_info(aoo_net_client *client,
                                       const void *address, int32_t addrlen,
                                       aoo_net_peer_info *info)
{
    return client->get_peer_info(address, addrlen, info);
}

aoo_error aoo::net::client_imp::get_peer_info(const void *address, int32_t addrlen,
                                              aoo_net_peer_info *info)
{
    peer_lock lock(peers_);
    for (auto& p : peers_){
        ip_address addr((const sockaddr *)address, addrlen);
        if (p.match(addr)){
            aoo::net::copy_string(p.group(), info->group_name, sizeof(info->group_name));
            aoo::net::copy_string(p.user(), info->user_name, sizeof(info->user_name));
            info->user_id = p.id();
            info->flags = p.flags();

            return AOO_OK;
        }
    }
    return AOO_ERROR_UNSPECIFIED;
}

aoo_error aoo_net_client_request(aoo_net_client *client,
                                 aoo_net_request_type request, void *data,
                                 aoo_net_callback callback, void *user) {
    return client->send_request(request, data, callback, user);
}

aoo_error aoo::net::client_imp::send_request(aoo_net_request_type request, void *data,
                                             aoo_net_callback callback, void *user){
    switch (request){
    case AOO_NET_CONNECT_REQUEST:
    {
        auto d = (aoo_net_connect_request *)data;
        do_connect(d->host, d->port, d->user_name, d->user_pwd, callback, user);
        break;
    }
    case AOO_NET_DISCONNECT_REQUEST:
        do_disconnect(callback, user);
        break;
    case AOO_NET_GROUP_JOIN_REQUEST:
    {
        auto d = (aoo_net_group_request *)data;
        do_join_group(d->group_name, d->group_pwd, callback, user);
        break;
    }
    case AOO_NET_GROUP_LEAVE_REQUEST:
    {
        auto d = (aoo_net_group_request *)data;
        do_leave_group(d->group_name, callback, user);
        break;
    }
    default:
        LOG_ERROR("aoo client: unknown request " << request);
        return AOO_ERROR_UNSPECIFIED;
    }
    return AOO_OK;
}

aoo_error aoo_net_client_send_message(aoo_net_client *client, const char *data, int32_t size,
                                      const void *addr, int32_t len, int32_t flags)
{
    return client->send_message(data, size, addr, len, flags);
}

aoo_error aoo::net::client_imp::send_message(const char *data, int32_t size,
                                             const void *addr, int32_t len, int32_t flags)
{
    // for now, we simply achieve 'reliable' messages by relaying over TCP
    // LATER implement ack mechanism over UDP.
    bool reliable = flags & AOO_NET_MESSAGE_RELIABLE;

    std::unique_ptr<imessage> msg;
    if (addr){
        if (len > 0){
            // peer message
            msg = std::make_unique<peer_message>(
                        data, size, (const sockaddr *)addr, len, flags);
        } else {
            // group message
            msg = std::make_unique<group_message>(
                        data, size, (const char *)addr, flags);
        }
    } else {
        msg = std::make_unique<message>(data, size, flags);
    }

    if (reliable){
        // add to TCP message queue
        tcp_messages_.push(std::move(msg));
        signal();
    } else {
        // add to UDP message queue
        udp_messages_.push(std::move(msg));
    }
    return AOO_OK;
}

aoo_error aoo_net_client_handle_message(aoo_net_client *client, const char *data,
                                        int32_t n, const void *addr, int32_t len)
{
    return client->handle_message(data, n, addr, len);
}

aoo_error aoo::net::client_imp::handle_message(const char *data, int32_t n,
                                               const void *addr, int32_t len){
    int32_t type;
    aoo_id id;
    int32_t onset;
    auto err = aoo_parse_pattern(data, n, &type, &id, &onset);
    if (err != AOO_OK){
        LOG_VERBOSE("aoo_client: not an AOO NET message!");
        return AOO_ERROR_UNSPECIFIED;
    }

    if (type == AOO_TYPE_SOURCE){
        // forward to matching source
        for (auto& s : sources_){
            if (s.id == id){
                return s.source->handle_message(data, n, addr, len);
            }
        }
        LOG_WARNING("aoo_client: handle_message(): source not found");
    } else if (type == AOO_TYPE_SINK){
        // forward to matching sink
        for (auto& s : sinks_){
            if (s.id == id){
                return s.sink->handle_message(data, n, addr, len);
            }
        }
        LOG_WARNING("aoo_client: handle_message(): sink not found");
    } else if (udp_client_){
        // forward to UDP client
        ip_address address((const sockaddr *)addr, len);
        return udp_client_->handle_message(data, n, address, type, onset);
    }

    return AOO_ERROR_UNSPECIFIED;
}

aoo_error aoo_net_client_send(aoo_net_client *client, aoo_sendfn fn, void *user){
    return client->send(fn, user);
}

aoo_error aoo::net::client_imp::send(aoo_sendfn fn, void *user){
    struct proxy_data {
        client_imp *self;
        aoo_sendfn fn;
        void *user;
    };

    auto proxy = [](void *x, const char *data, int32_t n,
            const void *address, int32_t addrlen, uint32_t flags){
        auto y = static_cast<proxy_data *>(x);

        bool relay = flags & AOO_ENDPOINT_RELAY;
        if (relay){
            // relay via server
            ip_address addr((const sockaddr *)address, addrlen);

            sendfn reply(y->fn, y->user);

            y->self->udp().send_peer_message(data, n, addr, reply, true);

            return (int32_t)0;
        } else {
            // just forward
            return y->fn(y->user, data, n, address, addrlen, flags);
        }
    };

    proxy_data data {this, fn, user};

    // send sources and sinks
    for (auto& s : sources_){
        s.source->send(proxy, &data);
    }
    for (auto& s : sinks_){
        s.sink->send(proxy, &data);
    }
    // send server messages
    if (state_.load() != client_state::disconnected){
        time_tag now = time_tag::now();
        sendfn reply(fn, user);

        if (udp_client_){
            udp_client_->update(reply, now);
        }

        // send outgoing peer/group messages
        std::unique_ptr<imessage> msg;
        while (udp_messages_.try_pop(msg)){
            msg->perform(*this, reply);
        }

        // update peers
        peer_lock lock(peers_);
        for (auto& p : peers_){
            p.send(reply, now);
        }
    }
    return AOO_OK;
}

aoo_error aoo_net_client_set_eventhandler(aoo_net_client *sink, aoo_eventhandler fn,
                                          void *user, int32_t mode)
{
    return sink->set_eventhandler(fn, user, mode);
}

aoo_error aoo::net::client_imp::set_eventhandler(aoo_eventhandler fn, void *user,
                                                 int32_t mode)
{
    eventhandler_ = fn;
    eventcontext_ = user;
    eventmode_ = (aoo_event_mode)mode;
    return AOO_OK;
}

aoo_bool aoo_net_client_events_available(aoo_net_server *client){
    return client->events_available();
}

aoo_bool aoo::net::client_imp::events_available(){
    return !events_.empty();
}

aoo_error aoo_net_client_poll_events(aoo_net_client *client){
    return client->poll_events();
}

aoo_error aoo::net::client_imp::poll_events(){
    // always thread-safe
    std::unique_ptr<ievent> e;
    while (events_.try_pop(e)){
        eventhandler_(eventcontext_, &e->event_, AOO_THREAD_UNKNOWN);
    }
    return AOO_OK;
}

aoo_error aoo_net_client_ctl(aoo_net_client *client, int32_t ctl,
                             intptr_t index, void *p, size_t size)
{
    return client->control(ctl, index, p, size);
}

aoo_error aoo::net::client_imp::control(int32_t ctl, intptr_t index,
                                        void *ptr, size_t size)
{
    LOG_WARNING("aoo_client: unsupported control " << ctl);
    return AOO_ERROR_UNSPECIFIED;
}

namespace aoo {
namespace net {

bool client_imp::handle_peer_message(const osc::ReceivedMessage& msg, int onset,
                                     const ip_address& addr)
{
    bool success = false;
    // NOTE: we have to loop over *all* peers because there can
    // be more than 1 peer on a given IP endpoint, since a single
    // user can join multiple groups.
    // LATER make this more efficient, e.g. by associating IP endpoints
    // with peers instead of having them all in a single list.
    peer_lock lock(peers_);
    for (auto& p : peers_){
        // forward to matching or unconnected peers!
        if (p.match(addr, true)){
            p.handle_message(msg, onset, addr);
            success = true;
        }
    }
    return success;
}

template<typename T>
void client_imp::perform_send_message(const char *data, int32_t size, int32_t flags,
                                      const sendfn& fn, T&& filter)
{
    bool reliable = flags & AOO_NET_MESSAGE_RELIABLE;
    // embed inside an OSC message:
    // /aoo/peer/msg' (b)<message>
    try {
        char buf[AOO_MAXPACKETSIZE];
        osc::OutboundPacketStream msg(buf, sizeof(buf));
        msg << osc::BeginMessage(AOO_NET_MSG_PEER_MESSAGE)
            << osc::Blob(data, size) << osc::EndMessage;

        peer_lock lock(peers_);
        for (auto& peer : peers_){
            if (filter(peer)){
                auto& addr = peer.address();
                LOG_DEBUG("aoo_client: send message " << data
                          << " to " << addr.name() << ":" << addr.port());
                // Note: reliable messages are dispatched in the TCP network thread,
                // unreliable messages are dispatched in the UDP network thread.
                if (reliable){
                    send_peer_message(msg.Data(), msg.Size(), addr);
                } else if (udp_client_){
                    udp_client_->send_peer_message(msg.Data(), msg.Size(),
                                                   addr, fn, peer.relay());
                }
            }
        }
    } catch (const osc::Exception& e){
        LOG_ERROR("aoo_client: error sending OSC message: " << e.what());
    }
}

void client_imp::do_connect(const char *host, int port,
                            const char *name, const char *pwd,
                            aoo_net_callback cb, void *user)
{
    auto cmd = std::make_unique<connect_cmd>(cb, user, host, port,
                                             name, encrypt(pwd));

    push_command(std::move(cmd));
}

void client_imp::perform_connect(const std::string& host, int port,
                                 const std::string& name, const std::string& pwd,
                                 aoo_net_callback cb, void *user)
{
    auto state = state_.load();
    if (state != client_state::disconnected){
        aoo_net_error_reply reply;
        reply.error_code = 0;
        if (state == client_state::connected){
            reply.error_message = "already connected";
        } else {
            reply.error_message = "already connecting";
        }

        if (cb) cb(user, AOO_ERROR_UNSPECIFIED, &reply);

        return;
    }

    state_.store(client_state::connecting);

    int err = try_connect(host, port);
    if (err != 0){
        // event
        std::string errmsg = socket_strerror(err);

        close();

        aoo_net_error_reply reply;
        reply.error_code = err;
        reply.error_message = errmsg.c_str();

        if (cb) cb(user, AOO_ERROR_UNSPECIFIED, &reply);

        return;
    }

    username_ = name;
    password_ = pwd;
    callback_ = cb;
    userdata_ = user;

    state_.store(client_state::handshake);
}

int client_imp::try_connect(const std::string &host, int port){
    socket_ = socket_tcp(0);
    if (socket_ < 0){
        int err = socket_errno();
        LOG_ERROR("aoo_client: couldn't create socket (" << err << ")");
        return err;
    }
    // resolve host name
    auto result = ip_address::resolve(host, port, type_);
    if (result.empty()){
        int err = socket_errno();
        // LATER think about best way for error handling. Maybe exception?
        LOG_ERROR("aoo_client: couldn't resolve hostname (" << socket_errno() << ")");
        return err;
    }

    LOG_DEBUG("aoo_client: server address list:");
    for (auto& addr : result){
        LOG_DEBUG("\t" << addr.name() << " " << addr.port());
    }

    // for actual TCP connection, just pick the first result
    auto& remote = result.front();
    LOG_VERBOSE("try to connect to " << remote.name() << "/" << port);

    // try to connect (LATER make timeout configurable)
    if (socket_connect(socket_, remote, 5) < 0){
        int err = socket_errno();
        LOG_ERROR("aoo_client: couldn't connect (" << err << ")");
        return err;
    }

    // get local network interface
    ip_address temp;
    if (socket_address(socket_, temp) < 0){
        int err = socket_errno();
        LOG_ERROR("aoo_client: couldn't get socket name (" << err << ")");
        return err;
    }
    ip_address local(temp.name(), udp_client_->port(), type_);

    LOG_VERBOSE("aoo_client: successfully connected to "
                << remote.name() << " on port " << remote.port());
    LOG_VERBOSE("aoo_client: local address: " << local.name());

    udp_client_->start_handshake(local, std::move(result));

    return 0;
}

void client_imp::do_disconnect(aoo_net_callback cb, void *user){
    auto cmd = std::make_unique<disconnect_cmd>(cb, user);

    push_command(std::move(cmd));
}

void client_imp::perform_disconnect(aoo_net_callback cb, void *user){
    auto state = state_.load();
    if (state != client_state::connected){
        aoo_net_error_reply reply;
        reply.error_message = (state == client_state::disconnected)
                ? "not connected" : "still connecting";
        reply.error_code = 0;

        if (cb) cb(user, AOO_ERROR_UNSPECIFIED, &reply);

        return;
    }

    close(true);

    if (cb) cb(user, AOO_OK, nullptr); // always succeeds
}

void client_imp::perform_login(const ip_address_list& addrlist){
    state_.store(client_state::login);

    char buf[AOO_MAXPACKETSIZE];

    osc::OutboundPacketStream msg(buf, sizeof(buf));
    msg << osc::BeginMessage(AOO_NET_MSG_SERVER_LOGIN)
        << (int32_t)make_version()
        << username_.c_str() << password_.c_str();
    for (auto& addr : addrlist){
        // unmap IPv4 addresses for IPv4 only servers
        msg << addr.name_unmapped() << addr.port();
    }
    msg << osc::EndMessage;
    send_server_message(msg.Data(), msg.Size());

    // send legacy one second, just in case we are talking to a legacy server
    osc::OutboundPacketStream msg2(buf, sizeof(buf));
    msg2 << osc::BeginMessage(AOO_NET_MSG_SERVER_LOGIN)
         << username_.c_str() << password_.c_str();
    for (auto& addr : addrlist){
        // unmap IPv4 addresses for IPv4 only servers
        msg2 << addr.name_unmapped() << addr.port();
    }
    msg2 << osc::EndMessage;
    send_server_message(msg2.Data(), msg2.Size());
}

void client_imp::perform_timeout(){
    aoo_net_error_reply reply;
    reply.error_code = 0;
    reply.error_message = "UDP handshake time out";

    callback_(userdata_, AOO_ERROR_UNSPECIFIED, &reply);

    if (state_.load() == client_state::handshake){
        close();
    }
}

void client_imp::do_join_group(const char *group, const char *pwd,
                               aoo_net_callback cb, void *user){
    auto cmd = std::make_unique<group_join_cmd>(cb, user, group, encrypt(pwd));

    push_command(std::move(cmd));
}

void client_imp::perform_join_group(const std::string &group, const std::string &pwd,
                                    aoo_net_callback cb, void *user)
{

    auto request = [group, cb, user](
            const char *pattern,
            const osc::ReceivedMessage& msg)
    {
        if (!strcmp(pattern, AOO_NET_MSG_GROUP_JOIN)){
            auto it = msg.ArgumentsBegin();
            std::string g = (it++)->AsString();
            if (g == group){
                int32_t status = (it++)->AsInt32();
                if (status > 0){
                    LOG_VERBOSE("aoo_client: successfully joined group " << group);
                    if (cb) cb(user, AOO_OK, nullptr);
                } else {
                    std::string errmsg;
                    if (msg.ArgumentCount() > 2){
                        errmsg = (it++)->AsString();
                        LOG_WARNING("aoo_client: couldn't join group "
                                    << group << ": " << errmsg);
                    } else {
                        errmsg = "unknown error";
                    }
                    // reply
                    aoo_net_error_reply reply;
                    reply.error_code = 0;
                    reply.error_message = errmsg.c_str();

                    if (cb) cb(user, AOO_ERROR_UNSPECIFIED, &reply);
                }

                return true;
            }
        }
        return false;
    };
    pending_requests_.push_back(request);

    char buf[AOO_MAXPACKETSIZE];
    osc::OutboundPacketStream msg(buf, sizeof(buf));
    msg << osc::BeginMessage(AOO_NET_MSG_SERVER_GROUP_JOIN)
        << group.c_str() << pwd.c_str() << osc::EndMessage;

    send_server_message(msg.Data(), msg.Size());
}

void client_imp::do_leave_group(const char *group,
                                aoo_net_callback cb, void *user){
    auto cmd = std::make_unique<group_leave_cmd>(cb, user, group);

    push_command(std::move(cmd));
}

void client_imp::perform_leave_group(const std::string &group,
                                     aoo_net_callback cb, void *user)
{
    auto request = [this, group, cb, user](
            const char *pattern,
            const osc::ReceivedMessage& msg)
    {
        if (!strcmp(pattern, AOO_NET_MSG_GROUP_LEAVE)){
            auto it = msg.ArgumentsBegin();
            std::string g = (it++)->AsString();
            if (g == group){
                int32_t status = (it++)->AsInt32();
                if (status > 0){
                    LOG_VERBOSE("aoo_client: successfully left group " << group);

                    // remove all peers from this group
                    peer_lock lock(peers_);
                    for (auto it = peers_.begin(); it != peers_.end(); ){
                        if (it->match(group)){
                            it = peers_.erase(it);
                        } else {
                            ++it;
                        }
                    }
                    lock.unlock();

                    if (cb) cb(user, AOO_OK, nullptr);
                } else {
                    std::string errmsg;
                    if (msg.ArgumentCount() > 2){
                        errmsg = (it++)->AsString();
                        LOG_WARNING("aoo_client: couldn't leave group "
                                    << group << ": " << errmsg);
                    } else {
                        errmsg = "unknown error";
                    }
                    // reply
                    aoo_net_error_reply reply;
                    reply.error_code = 0;
                    reply.error_message = errmsg.c_str();

                    if (cb) cb(user, AOO_ERROR_UNSPECIFIED, &reply);
                }

                return true;
            }
        }
        return false;
    };
    pending_requests_.push_back(request);

    char buf[AOO_MAXPACKETSIZE];
    osc::OutboundPacketStream msg(buf, sizeof(buf));
    msg << osc::BeginMessage(AOO_NET_MSG_SERVER_GROUP_LEAVE)
        << group.c_str() << osc::EndMessage;

    send_server_message(msg.Data(), msg.Size());
}

void client_imp::send_event(std::unique_ptr<ievent> e)
{
    switch (eventmode_){
    case AOO_EVENT_POLL:
        events_.push(std::move(e));
        break;
    case AOO_EVENT_CALLBACK:
        // client only has network threads
        eventhandler_(eventcontext_, &e->event_, AOO_THREAD_NETWORK);
        break;
    default:
        break;
    }
}

void client_imp::push_command(std::unique_ptr<icommand>&& cmd){
    commands_.push(std::move(cmd));

    signal();
}

bool client_imp::wait_for_event(float timeout){
    // LOG_DEBUG("aoo_client: wait " << timeout << " seconds");

    struct pollfd fds[2];
    fds[0].fd = eventsocket_;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    fds[1].fd = socket_;
    fds[1].events = POLLIN;
    fds[1].revents = 0;

    // round up to 1 ms! -1: block indefinitely
    // NOTE: macOS requires the negative timeout to exactly -1!
#ifdef _WIN32
    int result = WSAPoll(fds, 2, timeout < 0 ? -1 : timeout * 1000.0 + 0.5);
#else
    int result = poll(fds, 2, timeout < 0 ? -1 : timeout * 1000.0 + 0.5);
#endif
    if (result < 0){
        int err = socket_errno();
        if (err == EINTR){
            return true; // ?
        } else {
            LOG_ERROR("aoo_client: poll failed (" << err << ")");
            socket_error_print("poll");
            return false;
        }
    }

    // event socket
    if (fds[0].revents){
        // read empty packet
        char buf[64];
        recv(eventsocket_, buf, sizeof(buf), 0);
        // LOG_DEBUG("aoo_client: got signalled");
    }

    // tcp socket
    if (socket_ >= 0 && fds[1].revents){
        receive_data();
    }

    return true;
}

void client_imp::receive_data(){
    char buffer[AOO_MAXPACKETSIZE];
    auto result = recv(socket_, buffer, sizeof(buffer), 0);
    if (result > 0){
        recvbuffer_.write_bytes((uint8_t *)buffer, result);

        // handle packets
        uint8_t buf[AOO_MAXPACKETSIZE];
        while (true){
            auto size = recvbuffer_.read_packet(buf, sizeof(buf));
            if (size > 0){
                try {
                    osc::ReceivedPacket packet((const char *)buf, size);
                    if (packet.IsBundle()){
                        osc::ReceivedBundle bundle(packet);
                        handle_server_bundle(bundle);
                    } else {
                        handle_server_message(packet.Contents(), packet.Size());
                    }
                } catch (const osc::Exception& e){
                    LOG_ERROR("aoo_client: exception in receive_data: " << e.what());
                    on_exception("server TCP message", e);
                }
            } else {
                break;
            }
        }
    } else if (result == 0){
        // connection closed by the remote server
        on_socket_error(0);
    } else {
        int err = socket_errno();
        LOG_ERROR("aoo_client: recv() failed (" << err << ")");
        on_socket_error(err);
    }
}

void client_imp::send_server_message(const char *data, int32_t size){
    if (sendbuffer_.write_packet((const uint8_t *)data, size)){
        while (sendbuffer_.read_available()){
            uint8_t buf[1024];
            int32_t total = sendbuffer_.read_bytes(buf, sizeof(buf));

            int32_t nbytes = 0;
            while (nbytes < total){
                auto res = ::send(socket_, (char *)buf + nbytes, total - nbytes, 0);
                if (res >= 0){
                    nbytes += res;
                #if 0
                    LOG_DEBUG("aoo_client: sent " << res << " bytes");
                #endif
                } else {
                    auto err = socket_errno();

                    LOG_ERROR("aoo_client: send() failed (" << err << ")");
                    on_socket_error(err);
                    return;
                }
            }
        }
        LOG_DEBUG("aoo_client: sent " << data << " to server");
    } else {
        LOG_ERROR("aoo_client: couldn't send " << data << " to server");
    }
}

void client_imp::send_peer_message(const char *data, int32_t size,
                                   const ip_address& addr) {
    // /aoo/relay <ip> <port> <msg>
    char buf[AOO_MAXPACKETSIZE];
    osc::OutboundPacketStream msg(buf, sizeof(buf));
    // send unmapped IP address in case peer is IPv4 only!
    msg << osc::BeginMessage(AOO_MSG_DOMAIN AOO_NET_MSG_RELAY)
        << addr.name_unmapped() << addr.port() << osc::Blob(data, size)
        << osc::EndMessage;

    send_server_message(msg.Data(), msg.Size());
}

void client_imp::handle_server_message(const char *data, int32_t n){
    osc::ReceivedPacket packet(data, n);
    osc::ReceivedMessage msg(packet);

    aoo_type type;
    int32_t onset;
    auto err = parse_pattern(data, n, type, onset);
    if (err != AOO_OK){
        LOG_VERBOSE("aoo_client: not an AOO NET message!");
        return;
    }

    try {
        if (type == AOO_TYPE_CLIENT){
            // now compare subpattern
            auto pattern = msg.AddressPattern() + onset;
            LOG_DEBUG("aoo_client: got message " << pattern << " from server");

            if (!strcmp(pattern, AOO_NET_MSG_PING)){
                LOG_DEBUG("aoo_client: got TCP ping from server");
            } else if (!strcmp(pattern, AOO_NET_MSG_PEER_JOIN)){
                handle_peer_add(msg);
            } else if (!strcmp(pattern, AOO_NET_MSG_PEER_LEAVE)){
                handle_peer_remove(msg);
            } else if (!strcmp(pattern, AOO_NET_MSG_LOGIN)){
                handle_login(msg);
            } else {
                // handle reply
                for (auto it = pending_requests_.begin(); it != pending_requests_.end();){
                    if ((*it)(pattern, msg)){
                        it = pending_requests_.erase(it);
                        return;
                    } else {
                        ++it;
                    }
                }
                LOG_ERROR("aoo_client: couldn't handle reply " << pattern);
            }
        } else if (type == AOO_TYPE_RELAY){
            handle_relay_message(msg);
        } else {
            LOG_WARNING("aoo_client: got unsupported message " << msg.AddressPattern());
        }
    } catch (const osc::Exception& e){
        LOG_ERROR("aoo_client: exception on handling " << msg.AddressPattern()
                  << " message: " << e.what());
        on_exception("server TCP message", e, msg.AddressPattern());
    }
}

void client_imp::handle_server_bundle(const osc::ReceivedBundle &bundle){
    auto it = bundle.ElementsBegin();
    while (it != bundle.ElementsEnd()){
        if (it->IsBundle()){
            osc::ReceivedBundle b(*it);
            handle_server_bundle(b);
        } else {
            handle_server_message(it->Contents(), it->Size());
        }
        ++it;
    }
}

void client_imp::handle_login(const osc::ReceivedMessage& msg){
    // make sure that state hasn't changed
    if (state_.load() == client_state::login){
        auto it = msg.ArgumentsBegin();
        int32_t status = (it++)->AsInt32();

        if (status > 0){
            int32_t id = -1;
            uint32_t flags = 0;
            if (msg.ArgumentCount() > 2){
                id = (it++)->AsInt32();
                flags = (it != msg.ArgumentsEnd()) ? (uint32_t)(it++)->AsInt32() : 0;
            } else {
                // legacy client login only send status + errmsg
                flags |= AOO_NET_SERVER_LEGACY;
            }
            // connected!
            state_.store(client_state::connected);
            LOG_VERBOSE("aoo_client: successfully logged in (user ID: "
                        << id << " )");
            // notify
            aoo_net_connect_reply reply;
            reply.user_id = id;
            reply.server_flags = flags;
            server_flags_ = flags;

            callback_(userdata_, AOO_OK, &reply);
        } else {
            std::string errmsg;
            if (msg.ArgumentCount() > 1){
                errmsg = (it++)->AsString();
            } else {
                errmsg = "unknown error";
            }
            LOG_WARNING("aoo_client: login failed: " << errmsg);

            // cache callback and userdata
            auto cb = callback_;
            auto ud = userdata_;

            close();

            // notify
            aoo_net_error_reply reply;
            reply.error_code = 0;
            reply.error_message = errmsg.c_str();

            cb(ud, AOO_ERROR_UNSPECIFIED, &reply);
        }
    }
}

static void unwrap_message(const osc::ReceivedMessage& msg,
    ip_address& addr, ip_address::ip_type type, const char * & retdata, std::size_t & retsize)
{
    auto it = msg.ArgumentsBegin();

    auto ip = (it++)->AsString();
    auto port = (it++)->AsInt32();

    addr = ip_address(ip, port, type);

    const void *blobData;
    osc::osc_bundle_element_size_t blobSize;
    (it++)->AsBlob(blobData, blobSize);

    retdata = (const char *)blobData;
    retsize = blobSize;
    //    return osc::ReceivedPacket((const char *)blobData, blobSize);
}

void client_imp::handle_relay_message(const osc::ReceivedMessage &msg){
    ip_address addr;
    const char * retmsg = 0;
    std::size_t retsize = 0;
    unwrap_message(msg, addr, type(), retmsg, retsize);

    osc::ReceivedPacket packet (retmsg, retsize);
    osc::ReceivedMessage relayMsg(packet);

    // for now, we only handle peer OSC messages
    if (!strcmp(relayMsg.AddressPattern(), AOO_NET_MSG_PEER_MESSAGE)){
        // get embedded OSC message
        const void *data;
        osc::osc_bundle_element_size_t size;
        relayMsg.ArgumentsBegin()->AsBlob(data, size);

        LOG_DEBUG("aoo_client: got relayed peer message " << (const char *)data
                  << " from " << addr.name() << ":" << addr.port());

        auto e = std::make_unique<client_imp::message_event>(
                    (const char *)data, size, addr);

        send_event(std::move(e));
    } else {
        LOG_WARNING("aoo_client: got unexpected relay message " << relayMsg.AddressPattern());
    }
}

void client_imp::handle_peer_add(const osc::ReceivedMessage& msg){
    auto count = msg.ArgumentCount();
    auto it = msg.ArgumentsBegin();
    std::string group = (it++)->AsString();
    std::string user = (it++)->AsString();
    int32_t id = 0;
    count -= 2;
    int32_t flags = 0;
    bool legacy = false;

    // LEGACY: older sonobus did not have this id field
    if (msg.TypeTags()[2] == 'i') {
        id = (it++)->AsInt32();
        count -= 1;
    }
    else {
        legacy = true;
        server_flags_ |= AOO_NET_SERVER_LEGACY;
    }

    ip_address_list addrlist;
    while (count >= 2){
        std::string ip = (it++)->AsString();
        int32_t port = (it++)->AsInt32();
        ip_address addr(ip, port, type_);
        if (addr.valid()){
            addrlist.push_back(addr);
        }
        count -= 2;
    }

    peer_lock lock(peers_);
    // check if peer already exists (shouldn't happen)
    for (auto& p: peers_){
        if (p.match(group, id) || (legacy && p.match(group, user))){
            LOG_ERROR("aoo_client: peer " << p << " already added");
            return;
        }
    }
    peers_.emplace_front(*this, id, group, user, std::move(addrlist), flags);

    auto e = std::make_unique<peer_event>(
                AOO_NET_PEER_HANDSHAKE_EVENT,
                group.c_str(), user.c_str(), id);
    send_event(std::move(e));

    LOG_VERBOSE("aoo_client: new peer " << peers_.front());
}

void client_imp::handle_peer_remove(const osc::ReceivedMessage& msg){
    auto it = msg.ArgumentsBegin();
    std::string group = (it++)->AsString();
    std::string user = (it++)->AsString();
    int32_t id =  msg.ArgumentCount() > 2 ? (it++)->AsInt32() : -1;

    peer_lock lock(peers_);
    auto result = std::find_if(peers_.begin(), peers_.end(),
        [&](auto& p){ return (id >= 0) ? p.match(group, id) : p.match(group,user); });
    if (result == peers_.end()){
        LOG_ERROR("aoo_client: couldn't remove " << group << "|" << user << "|" << id);
        return;
    }

    // only send event if we're connected, which means
    // that an AOO_NET_PEER_JOIN_EVENT has been sent.
    if (result->connected()){
        ip_address addr = result->address();
        uint32_t flags = result->flags();

        auto e = std::make_unique<peer_event>(
            AOO_NET_PEER_LEAVE_EVENT, addr,
            group.c_str(), user.c_str(), id, flags);
        send_event(std::move(e));
    }

    peers_.erase(result);

    LOG_VERBOSE("aoo_client: peer " << group << "|" << user << " left");
}

bool client_imp::signal(){
    // LOG_DEBUG("aoo_client signal");
    return socket_signal(eventsocket_);
}

void client_imp::close(bool manual){
    if (socket_ >= 0){
        socket_close(socket_);
        socket_ = -1;
        LOG_VERBOSE("aoo_client: connection closed");
    }

    username_.clear();
    password_.clear();
    callback_ = nullptr;
    userdata_ = nullptr;

    sendbuffer_.reset();
    recvbuffer_.reset();

    // remove all peers
    peer_lock lock(peers_);
    peers_.clear();

    // clear pending request
    // LATER call them all with some dummy input
    // to avoid memleak if clients pass heap
    // allocated request data, which is supposed
    // to be freed in the callback.
    pending_requests_.clear();

    if (!manual && state_.load() == client_state::connected){
        auto e = std::make_unique<event>(AOO_NET_DISCONNECT_EVENT);
        send_event(std::move(e));
    }
    state_.store(client_state::disconnected);
}

void client_imp::on_socket_error(int err){
    char msg[256];
    if (err != 0) {
        socket_strerror(err, msg, sizeof(msg));
    } else {
        snprintf(msg, sizeof(msg), "connection closed by server");
    }
    auto e = std::make_unique<error_event>(err, msg);

    send_event(std::move(e));

    close();
}

void client_imp::on_exception(const char *what, const osc::Exception &err,
                              const char *pattern){
    char msg[256];
    if (pattern){
        snprintf(msg, sizeof(msg), "exception in %s (%s): %s",
                 what, pattern, err.what());
    } else {
        snprintf(msg, sizeof(msg), "exception in %s: %s",
                 what, err.what());
    }

    auto e = std::make_unique<error_event>(0, msg);

    send_event(std::move(e));

    close();
}

/*///////////////////// events ////////////////////////*/

client_imp::error_event::error_event(int32_t code, const char *msg)
{
    error_event_.type = AOO_NET_ERROR_EVENT;
    error_event_.error_code = code;
    error_event_.error_message = aoo::copy_string(msg);
}

client_imp::error_event::~error_event()
{
    free_string((char *)error_event_.error_message);
}

client_imp::ping_event::ping_event(int32_t type, const ip_address& addr,
                                   uint64_t tt1, uint64_t tt2, uint64_t tt3)
{
    ping_event_.type = type;
    ping_event_.address = copy_sockaddr(addr.address(), addr.length());
    ping_event_.addrlen = addr.length();
    ping_event_.tt1 = tt1;
    ping_event_.tt2 = tt2;
    ping_event_.tt3 = tt3;
}

client_imp::ping_event::~ping_event()
{
    free_sockaddr((void *)ping_event_.address, ping_event_.addrlen);
}

client_imp::peer_event::peer_event(int32_t type, const ip_address& addr,
                                   const char *group, const char *user,
                                   int32_t id, uint32_t flags)
{
    peer_event_.type = type;
    peer_event_.address = copy_sockaddr(addr.address(), addr.length());
    peer_event_.addrlen = addr.length();
    peer_event_.group_name = aoo::copy_string(group);
    peer_event_.user_name = aoo::copy_string(user);
    peer_event_.user_id = id;
    peer_event_.flags = flags;
}

client_imp::peer_event::peer_event(int32_t type, const char *group,
                                   const char *user, int32_t id)
{
    peer_event_.type = type;
    peer_event_.address = nullptr;
    peer_event_.addrlen = 0;
    peer_event_.group_name = aoo::copy_string(group);
    peer_event_.user_name = aoo::copy_string(user);
    peer_event_.user_id = id;
    peer_event_.flags = 0;
}


client_imp::peer_event::~peer_event()
{
    free_string((char *)peer_event_.user_name);
    free_string((char *)peer_event_.group_name);
    free_sockaddr((void *)peer_event_.address, peer_event_.addrlen);
}

client_imp::message_event::message_event(const char *data, int32_t size,
                                         const ip_address& addr)
{
    message_event_.type = AOO_NET_PEER_MESSAGE_EVENT;
    message_event_.address = copy_sockaddr(addr.address(), addr.length());
    message_event_.addrlen = addr.length();
    auto msg = (char *)aoo::allocate(size);
    memcpy(msg, data, size);
    message_event_.data = msg;
    message_event_.size = size;
}

client_imp::message_event::~message_event()
{
    aoo::deallocate((char *)message_event_.data, message_event_.size);
    free_sockaddr((void *)message_event_.address, message_event_.addrlen);
}

/*///////////////////// udp_client ////////////////////*/

void udp_client::update(const sendfn& reply, time_tag now){
    auto elapsed_time = client_->elapsed_time_since(now);
    auto delta = elapsed_time - last_ping_time_;

    auto state = client_->current_state();

    if (state == client_state::handshake){
        // check for time out
        // "first_ping_time_" is guaranteed to be set to 0
        // before the state changes to "handshake"
        auto start = first_ping_time_.load();
        if (start == 0){
            first_ping_time_.store(elapsed_time);
        } else if ((elapsed_time - start) > client_->request_timeout()){
            // handshake has timed out!
            auto cmd = std::make_unique<client_imp::timeout_cmd>();

            client_->push_command(std::move(cmd));

            return;
        }
        // send handshakes in fast succession
        if (delta >= client_->request_interval()){
            char buf[64];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            msg << osc::BeginMessage(AOO_NET_MSG_SERVER_REQUEST) << osc::EndMessage;

            scoped_shared_lock lock(mutex_);
            for (auto& addr : server_addrlist_){
                reply(msg.Data(), msg.Size(), addr, 0);
            }
            last_ping_time_ = elapsed_time;
        }
    } else if (state == client_state::connected){
        // send regular pings
        if (delta >= client_->ping_interval()){
            char buf[64];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            msg << osc::BeginMessage(AOO_NET_MSG_SERVER_PING)
                << osc::EndMessage;

            send_server_message(msg.Data(), msg.Size(), reply);
            last_ping_time_ = elapsed_time;
        }
    }
}

aoo_error udp_client::handle_message(const char *data, int32_t n,
                                     const ip_address &addr,
                                     aoo_type type, int32_t onset){
    try {
        osc::ReceivedPacket packet(data, n);
        osc::ReceivedMessage msg(packet);

        LOG_DEBUG("aoo_client: handle UDP message " << msg.AddressPattern()
            << " from " << addr.name() << ":" << addr.port());

        if (type == AOO_TYPE_PEER){
            // peer message
            //
            // NOTE: during the handshake process it is expected that
            // we receive UDP messages which we have to ignore:
            // a) pings from a peer which we haven't had the chance to add yet
            // b) pings sent to alternative endpoint addresses
            if (!client_->handle_peer_message(msg, onset, addr)){
                LOG_VERBOSE("aoo_client: ignoring UDP message "
                            << msg.AddressPattern() << " from endpoint "
                            << addr.name() << ":" << addr.port());
            }
        } else if (type == AOO_TYPE_CLIENT){
            // server message
            if (is_server_address(addr)){
                handle_server_message(msg, onset);
            } else {
                LOG_WARNING("aoo_client: got message from unknown server " << addr.name());
            }
        } else if (type == AOO_TYPE_RELAY){
            handle_relay_message(msg);
        } else {
            LOG_WARNING("aoo_client: got unexpected message " << msg.AddressPattern());
            return AOO_ERROR_UNSPECIFIED;
        }

        return AOO_OK;
    } catch (const osc::Exception& e){
        LOG_ERROR("aoo_client: exception in handle_message: " << e.what());
    #if 0
        on_exception("UDP message", e);
    #endif
        return AOO_ERROR_UNSPECIFIED;
    }
}

void udp_client::handle_relay_message(const osc::ReceivedMessage &msg){
    ip_address addr;
    const char * retmsg = 0;
    std::size_t retsize = 0;
    unwrap_message(msg, addr, client_->type(), retmsg, retsize);

#if DEBUG_RELAY
    LOG_DEBUG("aoo_client: got relayed message " << packet.Contents());
#endif

    client_->handle_message(retmsg, retsize,
                            addr.address(), addr.length());
}

void udp_client::send_peer_message(const char *data, int32_t size,
                                   const ip_address& addr,
                                   const sendfn& fn, bool relay)
{
    if (relay){
        char buf[AOO_MAXPACKETSIZE];
        osc::OutboundPacketStream msg(buf, sizeof(buf));
        // send unmapped address in case the peer is IPv4 only!
        msg << osc::BeginMessage(AOO_MSG_DOMAIN AOO_NET_MSG_RELAY)
            << addr.name_unmapped() << addr.port() << osc::Blob(data, size)
            << osc::EndMessage;
        send_server_message(msg.Data(), msg.Size(), fn);
    } else {
        fn(data, size, addr, 0);
    }
}

void udp_client::start_handshake(const ip_address& local,
                                 ip_address_list&& remote)
{
    scoped_lock lock(mutex_); // to be really safe
    first_ping_time_ = 0;
    local_address_ = local;
    public_addrlist_.clear();
    server_addrlist_ = std::move(remote);
}

void udp_client::send_server_message(const char *data, int32_t size, const sendfn& fn)
{
    scoped_shared_lock lock(mutex_);
    if (!server_addrlist_.empty()){
        auto& remote = server_addrlist_.front();
        if (remote.valid()){
            fn(data, size, remote, 0);
        }
    }
}

void udp_client::handle_server_message(const osc::ReceivedMessage& msg, int onset){
    auto pattern = msg.AddressPattern() + onset;
    try {
        if (!strcmp(pattern, AOO_NET_MSG_PING)){
            LOG_DEBUG("aoo_client: got UDP ping from server");
        } else if (!strcmp(pattern, AOO_NET_MSG_REPLY)){
            if (client_->current_state() == client_state::handshake){
                // retrieve public IP + port
                auto it = msg.ArgumentsBegin();
                std::string ip = (it++)->AsString();
                int port = (it++)->AsInt32();

                ip_address addr(ip, port, client_->type());

                scoped_lock lock(mutex_);
                for (auto& a : public_addrlist_){
                    if (a == addr){
                        LOG_DEBUG("aoo_client: public address " << addr.name()
                                  << " already received");
                        return; // already received
                    }
                }
                public_addrlist_.push_back(addr);
                LOG_VERBOSE("aoo_client: got public address "
                            << addr.name() << " " << addr.port());

                // check if we got all public addresses
                // LATER improve this
                if (public_addrlist_.size() == server_addrlist_.size()){
                    // now we can try to login
                    ip_address_list addrlist;
                    addrlist.reserve(public_addrlist_.size() + 1);

                    // local address first (for backwards compatibility with older versions)
                    addrlist.push_back(local_address_);
                    addrlist.insert(addrlist.end(),
                        public_addrlist_.begin(), public_addrlist_.end());

                    auto cmd = std::make_unique<client_imp::login_cmd>(std::move(addrlist));

                    client_->push_command(std::move(cmd));
                }
            }
        } else {
            LOG_WARNING("aoo_client: received unexpected UDP message "
                        << pattern << " from server");
        }
    } catch (const osc::Exception& e){
        LOG_ERROR("aoo_client: exception on handling " << pattern
                  << " message: " << e.what());
    #if 0
        on_exception("server UDP message", e, pattern);
    #endif
    }
}

bool udp_client::is_server_address(const ip_address& addr){
    // server message
    scoped_shared_lock lock(mutex_);
    for (auto& remote : server_addrlist_){
        if (remote == addr){
            return true;
        }
    }
    return false;
}

/*///////////////////// peer //////////////////////////*/

peer::peer(client_imp& client, int32_t id, const std::string& group,
           const std::string& user, ip_address_list&& addrlist, int32_t flags)
    : client_(&client), id_(id), flags_(flags), group_(group), user_(user),
      addresses_(std::move(addrlist))
{
    start_time_ = time_tag::now();

    LOG_DEBUG("create peer " << *this);
}

peer::~peer(){
    LOG_DEBUG("destroy peer " << *this);
}

bool peer::match(const ip_address& addr, bool unconnected) const {
    if (connected()){
        return real_address_ == addr;
    } else {
        return unconnected;
    }
}

bool peer::match(const std::string& group) const {
    return group_ == group; // immutable!
}

bool peer::match(const std::string& group, const std::string& user) const {
    return group_ == group && user_ == user; // immutable!
}

bool peer::match(const std::string& group, int32_t id)
{
    return id_ == id && group_ == group; // immutable!
}

std::ostream& operator << (std::ostream& os, const peer& p)
{
    os << p.group_ << "|" << p.user_ << "|" << p.id_;
    return os;
}

void peer::send(const sendfn& reply, time_tag now){
    auto elapsed_time = time_tag::duration(start_time_, now);
    auto delta = elapsed_time - last_pingtime_;

    if (connected()){
        // send regular ping
        if (delta >= client_->ping_interval()){
            char buf[64];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            msg << osc::BeginMessage(AOO_NET_MSG_PEER_PING) << osc::EndMessage;

            client_->udp().send_peer_message(msg.Data(), msg.Size(),
                                             real_address_, reply, relay());

            last_pingtime_ = elapsed_time;
        }
        // reply to /ping message
        if (got_ping_.exchange(false)){
            char buf[64];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            msg << osc::BeginMessage(AOO_NET_MSG_PEER_REPLY) << osc::EndMessage;

            client_->udp().send_peer_message(msg.Data(), msg.Size(),
                                             real_address_, reply, relay());
        }
    } else if (!timeout_) {
        // try to establish UDP connection with peer
        if (elapsed_time > client_->request_timeout()){
            // time out
            bool can_relay = client_->have_server_flag(AOO_NET_SERVER_RELAY);
            if (can_relay && !relay()){
                LOG_VERBOSE("aoo_client: try to relay " << *this);
                // try to relay traffic over server
                start_time_ = now; // reset timer
                last_pingtime_ = 0;
                flags_ |= AOO_ENDPOINT_RELAY;
                flags_ &= ~(AOO_ENDPOINT_LEGACY);
                return;
            }

            // couldn't establish relay connection!
            const char *what = relay() ? "relay" : "peer-to-peer";
            LOG_ERROR("aoo_client: couldn't establish UDP " << what
                      << " connection to " << *this << "; timed out after "
                      << client_->request_timeout() << " seconds");

            std::stringstream ss;
            ss << "couldn't establish connection with peer " << *this;

            auto e1 = std::make_unique<client_imp::error_event>(0, ss.str().c_str());
            client_->send_event(std::move(e1));

            auto e2 = std::make_unique<client_imp::peer_event>(
                        AOO_NET_PEER_TIMEOUT_EVENT,
                        group().c_str(), user().c_str(), id());
            client_->send_event(std::move(e2));

            timeout_ = true;

            return;
        }
        // send handshakes in fast succession to all addresses
        // until we get a reply from one of them (see handle_message())
        if (delta >= client_->request_interval()){
            char buf[64];
            char buf2[68];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            osc::OutboundPacketStream msg2(buf2, sizeof(buf2));

            // Include user ID, so peers can identify us even
            // if we're behind a symmetric NAT.
            // NOTE: This trick doesn't work if both parties are
            // behind a symmetrict NAT; in that case, UDP hole punching
            // simply doesn't work.



            msg << osc::BeginMessage(AOO_NET_MSG_PEER_PING)
                << (int32_t)id_ << osc::EndMessage;

            // legacy sonobus used 64 bit token field, so for backwards compat, send this too
            msg2 << osc::BeginMessage(AOO_NET_MSG_PEER_PING)
            << (int64_t)id_ << osc::EndMessage;

            for (auto& addr : addresses_){
                LOG_DEBUG("aoo_client: send handshake peer ping to: " << *this << " = " <<  addr.name() << ":" << addr.port());
                client_->udp().send_peer_message(msg.Data(), msg.Size(),
                                                 addr, reply, relay());

                client_->udp().send_peer_message(msg2.Data(), msg2.Size(),
                                                 addr, reply, relay());
            }

            // LOG_DEBUG("send ping to " << *this);

            last_pingtime_ = elapsed_time;
        }
    }
}

bool peer::handle_first_message(const osc::ReceivedMessage &msg, int onset,
                                const ip_address &addr)
{
#if FORCE_RELAY
    // force relay
    if (!relay()){
        return false;
    }
#endif
    // Try to find matching address
    for (auto& a : addresses_){
        if (a == addr){
            real_address_ = addr;
            connected_.store(true, std::memory_order_release);
            return true;
        }
    }

    // We might get a message from a peer behind a symmetric NAT.
    // To be sure, check user ID, but only if provided
    // (for backwards compatibility with older AOO clients)
    auto pattern = msg.AddressPattern() + onset;
    if (!strcmp(pattern, AOO_NET_MSG_PING)){
        if (msg.ArgumentCount() > 0 && msg.TypeTags()[0] == 'i'){
            try {
                auto it = msg.ArgumentsBegin();
                int32_t id = it->AsInt32();
                if (id == id_){
                    real_address_ = addr;
                    connected_.store(true);
                    LOG_WARNING("aoo_client: peer " << *this
                                << " is located behind a symmetric NAT!");
                    return true;
                }
            } catch (const osc::Exception& e){
                LOG_ERROR("aoo_client: got bad " << pattern
                          << " message from peer: " << e.what());
            }
        }
        else if (msg.ArgumentCount() > 0 && msg.TypeTags()[0] == 'h'){
            // LEGACY: old sonobus pings use a 64bit "token" here
            LOG_VERBOSE("aoo_client: legacy ping detected from peer " << *this);
            flags_ |= AOO_ENDPOINT_LEGACY;
        } else {
            // ignore silently!
        }
    } else {
        LOG_ERROR("aoo_client: got " << pattern
                  << " message from unknown peer");
    }
    return false;
}

void peer::handle_message(const osc::ReceivedMessage &msg, int onset,
                          const ip_address& addr)
{
    auto pattern = msg.AddressPattern() + onset;

    if (!connected()){
        if (!handle_first_message(msg, onset, addr)){
            return;
        }

        if (!strcmp(pattern, AOO_NET_MSG_PING)){
            if (msg.ArgumentCount() > 0 && msg.TypeTags()[0] == 'h'){
                // LEGACY: old sonobus pings use a 64bit "token" here
                LOG_VERBOSE("aoo_client: legacy ping detected from peer " << *this);
                flags_ |= AOO_ENDPOINT_LEGACY;
            }
        }

        // push event
        auto e = std::make_unique<client_imp::peer_event>(
                    AOO_NET_PEER_JOIN_EVENT, addr,
                    group().c_str(), user().c_str(), id(), flags());

        client_->send_event(std::move(e));

        LOG_VERBOSE("aoo_client: successfully established connection with "
                  << *this << " [" << addr.name() << "]:" << addr.port()
                    << (relay() ? " (relayed)" : ""));
    }

    try {
        if (!strcmp(pattern, AOO_NET_MSG_PING)){
            LOG_DEBUG("aoo_client: got ping from " << *this);
            got_ping_.store(true);
        } else if (!strcmp(pattern, AOO_NET_MSG_REPLY)){
            LOG_DEBUG("aoo_client: got reply from " << *this);
        } else if (!strcmp(pattern, AOO_NET_MSG_MESSAGE)){
            // get embedded OSC message
            const void *data;
            osc::osc_bundle_element_size_t size;
            msg.ArgumentsBegin()->AsBlob(data, size);

            LOG_DEBUG("aoo_client: got message " << (const char *)data
                      << " from " << addr.name() << ":" << addr.port());

            auto e = std::make_unique<client_imp::message_event>(
                        (const char *)data, size, addr);

            client_->send_event(std::move(e));
        } else {
            LOG_WARNING("aoo_client: received unknown message "
                        << pattern << " from " << *this);
        }
    } catch (const osc::Exception& e){
        // peer exceptions are not fatal!
        LOG_ERROR("aoo_client: " << *this << ": exception on handling "
                  << pattern << " message: " << e.what());
    }
}

} // net
} // aoo
