/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "client.hpp"
#include "client_events.hpp"

#include "aoo/aoo_source.hpp"
#include "aoo/aoo_sink.hpp"

#include "../binmsg.hpp"

#include <cstring>
#include <functional>
#include <algorithm>
#include <sstream>

#if AOO_CLIENT_SIMULATE
#include <random>
#endif

#ifdef _WIN32
# include <winsock2.h>
#else
# include <sys/poll.h>
# include <sys/select.h>
# include <sys/time.h>
# include <unistd.h>
# include <netdb.h>
# include <netinet/in.h>
# include <netinet/tcp.h>
# include <arpa/inet.h>
# include <errno.h>
#endif

namespace aoo {

std::string response_error_message(AooError result, int code, const char *msg) {
    if (code != 0 || *msg) {
        char buf[256];
        snprintf(buf, sizeof(buf), "%s: %s (%d)", aoo_strerror(result), msg, code);
        return buf;
    } else {
        return aoo_strerror(result);
    }
}

}

//--------------------- AooClient -----------------------------//

AOO_API AooClient * AOO_CALL AooClient_new(AooError *err) {
    try {
        if (err) {
            *err = kAooErrorNone;
        }
        return aoo::construct<aoo::net::Client>();
    } catch (const std::bad_alloc&) {
        if (err) {
            *err = kAooErrorOutOfMemory;
        }
        return nullptr;
    }
}

aoo::net::Client::Client() {
    eventsocket_ = socket_udp(0);
    if (eventsocket_ < 0){
        // TODO handle error
        socket_error_print("socket_udp");
    }

    sendbuffer_.resize(AOO_MAX_PACKET_SIZE);
}

AOO_API void AOO_CALL AooClient_free(AooClient *client){
    // cast to correct type because base class
    // has no virtual destructor!
    aoo::destroy(static_cast<aoo::net::Client *>(client));
}

aoo::net::Client::~Client() {
    if (socket_ >= 0){
        socket_close(socket_);
    }
}

AOO_API AooError AOO_CALL AooClient_setup(
    AooClient *client, AooUInt16 port, AooSocketFlags flags)
{
    return client->setup(port, flags);
}

AooError AOO_CALL aoo::net::Client::setup(AooUInt16 port, AooSocketFlags flags) {
    if (auto err = udp_client_.setup(port, flags); err != kAooOk) {
        return err;
    }

    // get private/global network interfaces
    auto get_address = [](int sock, const char *host, int port,
                          ip_address::ip_type family, bool ipv4mapped) {
        auto addrlist = ip_address::resolve(host, port, family, ipv4mapped);
        if (!addrlist.empty()) {
            if (socket_connect(sock, addrlist.front(), 0) == 0) {
                ip_address result;
                if (socket_address(sock, result) == 0) {
                    return result;
                } else {
                    throw std::runtime_error("getsockname() failed: " + socket_strerror(socket_errno()));
                }
            } else {
                throw std::runtime_error("connect() failed: " + socket_strerror(socket_errno()));
            }
        } else {
            throw std::runtime_error(std::string(ipv4mapped ? "IPv4" : "IPv6") + " networking not available");
        }
    };

    local_addr_.clear();
    int sock = socket_udp(0);

    // try to get global IPv6 address
    try {
        auto ipv6_addr = get_address(sock, "2001:4860:4860::8888", 80, ip_address::IPv6, false);
        local_addr_.emplace_back(ipv6_addr.name(), udp_client_.port());
        LOG_DEBUG("AooClient: global IPv6 address: " << local_addr_.back());
    } catch (const std::exception& e) {
        LOG_VERBOSE("AooClient: could not get global IPv6 address");
        LOG_DEBUG(e.what());
    }

    // try to get private IPv4 address
    try {
        auto ipv4_addr = get_address(sock, "8.8.8.8", 80, socket_family(sock), true);
        local_addr_.emplace_back(ipv4_addr.name_unmapped(), udp_client_.port()); // unmapped!
        LOG_DEBUG("AooClient: private IPv4 address: " << local_addr_.back());
    } catch (const std::exception& e) {
        LOG_VERBOSE("AooClient: could not get private IPv4 address");
        LOG_DEBUG(e.what());
    }

    return kAooOk;
}

AOO_API AooError AOO_CALL AooClient_run(AooClient *client, AooBool nonBlocking){
    return client->run(nonBlocking);
}

AooError AOO_CALL aoo::net::Client::run(AooBool nonBlocking){
    start_time_ = time_tag::now();

    try {
        while (!quit_.load()){
            double timeout = -1;

            time_tag now = time_tag::now();
            auto elapsed_time = time_tag::duration(start_time_, now);

            if (state_.load() == client_state::connected){
                auto delta = elapsed_time - last_ping_time_;
                auto interval = ping_interval();
                if (delta >= interval) {
                    // send ping
                    if (socket_ >= 0){
                        auto msg = start_server_message();

                        msg << osc::BeginMessage(kAooMsgServerPing)
                            << osc::EndMessage;

                        send_server_message(msg);
                    } else {
                        LOG_ERROR("AooClient: bug send_ping()");
                    }

                    last_ping_time_ = elapsed_time;
                    timeout = interval;
                } else {
                    timeout = interval - delta;
                }
            }

            auto didsomething = wait_for_event(nonBlocking ? 0 : timeout);

            // handle commands
            std::unique_ptr<icommand> cmd;
            while (commands_.try_pop(cmd)){
                cmd->perform(*this);
            }

            if (!peers_.update()){
                // LOG_DEBUG("AooClient: update() would block");
            }

            if (nonBlocking) {
                return didsomething ? kAooOk : kAooErrorWouldBlock;
            }
        }
    } catch (const net::error& e) {
        return e.code();
    }

    return kAooOk;
}

AOO_API AooError AOO_CALL AooClient_quit(AooClient *client){
    return client->quit();
}

AooError AOO_CALL aoo::net::Client::quit(){
    quit_.store(true);
    if (!signal()){
        // force wakeup by closing the socket.
        // this is not nice and probably undefined behavior,
        // the MSDN docs explicitly forbid it!
        socket_close(eventsocket_);
    }
    return kAooOk;
}

AOO_API AooError AOO_CALL AooClient_addSource(
        AooClient *client, AooSource *src, AooId id)
{
    return client->addSource(src, id);
}

AooError AOO_CALL aoo::net::Client::addSource(
        AooSource *src, AooId id)
{
#if 1
    for (auto& s : sources_){
        if (s.source == src){
            LOG_ERROR("AooClient: source already added");
            return kAooErrorAlreadyExists;
        } else if (s.id == id){
            LOG_WARNING("AooClient: source with id " << id
                        << " already added!");
            return kAooErrorAlreadyExists;
        }
    }
#endif
    sources_.push_back({ src, id });
    src->control(kAooCtlSetClient,
                 reinterpret_cast<intptr_t>(this), nullptr, 0);
    return kAooOk;
}

AOO_API AooError AOO_CALL AooClient_removeSource(
        AooClient *client, AooSource *src)
{
    return client->removeSource(src);
}

AooError AOO_CALL aoo::net::Client::removeSource(
        AooSource *src)
{
    for (auto it = sources_.begin(); it != sources_.end(); ++it){
        if (it->source == src){
            sources_.erase(it);
            src->control(kAooCtlSetClient, 0, nullptr, 0);
            return kAooOk;
        }
    }
    LOG_ERROR("AooClient: source not found");
    return kAooErrorNotFound;
}

AOO_API AooError AOO_CALL AooClient_addSink(
        AooClient *client, AooSink *sink, AooId id)
{
    return client->addSink(sink, id);
}

AooError AOO_CALL aoo::net::Client::addSink(
        AooSink *sink, AooId id)
{
#if 1
    for (auto& s : sinks_){
        if (s.sink == sink){
            LOG_ERROR("AooClient: sink already added");
            return kAooErrorAlreadyExists;
        } else if (s.id == id){
            LOG_WARNING("AooClient: sink with id " << id
                        << " already added!");
            return kAooErrorAlreadyExists;
        }
    }
#endif
    sinks_.push_back({ sink, id });
    sink->control(kAooCtlSetClient,
                  reinterpret_cast<intptr_t>(this), nullptr, 0);
    return kAooOk;
}

AOO_API AooError AOO_CALL AooClient_removeSink(
        AooClient *client, AooSink *sink)
{
    return client->removeSink(sink);
}

AooError AOO_CALL aoo::net::Client::removeSink(
        AooSink *sink)
{
    for (auto it = sinks_.begin(); it != sinks_.end(); ++it){
        if (it->sink == sink){
            sinks_.erase(it);
            sink->control(kAooCtlSetClient, 0, nullptr, 0);
            return kAooOk;
        }
    }
    LOG_ERROR("AooClient: sink not found");
    return kAooErrorNotFound;
}

AOO_API AooError AOO_CALL AooClient_connect(
        AooClient *client, const AooChar *hostName, AooInt32 port, const AooChar *password,
        const AooData *metadata, AooResponseHandler cb, void *context) {
    return client->connect(hostName, port, password, metadata, cb, context);
}

AooError AOO_CALL aoo::net::Client::connect(
        const AooChar *hostName, AooInt32 port, const AooChar *password,
        const AooData *metadata, AooResponseHandler cb, void *context) {
    auto cmd = std::make_unique<connect_cmd>(hostName, port, password, metadata, cb, context);
    push_command(std::move(cmd));
    return kAooOk;
}

AOO_API AooError AOO_CALL AooClient_disconnect(
        AooClient *client, AooResponseHandler cb, void *context) {
    return client->disconnect(cb, context);
}

AooError AOO_CALL aoo::net::Client::disconnect(AooResponseHandler cb, void *context) {
    auto cmd = std::make_unique<disconnect_cmd>(cb, context);
    push_command(std::move(cmd));
    return kAooOk;
}

AOO_API AooError AOO_CALL AooClient_joinGroup(
        AooClient *client,
        const AooChar *groupName, const AooChar *groupPwd, const AooData *groupMetadata,
        const AooChar *userName, const AooChar *userPwd, const AooData *userMetadata,
        const AooIpEndpoint *relayAddress, AooResponseHandler cb, void *context) {
    return client->joinGroup(groupName, groupPwd, groupMetadata, userName, userPwd,
                             userMetadata, relayAddress, cb, context);
}

AooError AOO_CALL aoo::net::Client::joinGroup(
        const AooChar *groupName, const AooChar *groupPwd, const AooData *groupMetadata,
        const AooChar *userName, const AooChar *userPwd, const AooData *userMetadata,
        const AooIpEndpoint *relayAddress, AooResponseHandler cb, void *context) {
    auto cmd = std::make_unique<group_join_cmd>(groupName, groupPwd, groupMetadata,
                                                userName, userPwd, userMetadata,
                                                (relayAddress ? ip_host(*relayAddress) : ip_host{}),
                                                cb, context);
    push_command(std::move(cmd));
    return kAooOk;
}

AOO_API AooError AOO_CALL AooClient_leaveGroup(
        AooClient *client, AooId group, AooResponseHandler cb, void *context) {
    return client->leaveGroup(group, cb, context);
}

AooError AOO_CALL aoo::net::Client::leaveGroup(
        AooId group, AooResponseHandler cb, void *context) {
    auto cmd = std::make_unique<group_leave_cmd>(group, cb, context);
    push_command(std::move(cmd));
    return kAooOk;
}

AOO_API AooError AOO_CALL AooClient_updateGroup(
        AooClient *client, AooId groupId, const AooData *groupMetadata,
        AooResponseHandler cb, void *context) {
    if (!groupMetadata) {
        return kAooErrorBadArgument;
    }
    return client->updateGroup(groupId, *groupMetadata, cb, context);
}

AooError AOO_CALL aoo::net::Client::updateGroup(
        AooId groupId, const AooData& groupMetadata,
        AooResponseHandler cb, void *context) {
    auto cmd = std::make_unique<group_update_cmd>(groupId, groupMetadata, cb, context);
    push_command(std::move(cmd));
    return kAooOk;
}

AOO_API AooError AOO_CALL AooClient_updateUser(
        AooClient *client, AooId groupId, const AooData *userMetadata,
        AooResponseHandler cb, void *context) {
    if (!userMetadata) {
        return kAooErrorBadArgument;
    }
    return client->updateUser(groupId, *userMetadata, cb, context);
}

AooError AOO_CALL aoo::net::Client::updateUser(
        AooId groupId, const AooData& userMetadata,
        AooResponseHandler cb, void *context) {
    auto cmd = std::make_unique<user_update_cmd>(groupId, userMetadata, cb, context);
    push_command(std::move(cmd));
    return kAooOk;
}

AOO_API AooError AOO_CALL AooClient_customRequest(
        AooClient *client, const AooData *data, AooFlag flags,
        AooResponseHandler cb, void *context) {
    if (!data) {
        return kAooErrorBadArgument;
    }
    return client->customRequest(*data, flags, cb, context);
}

AooError AOO_CALL aoo::net::Client::customRequest(
        const AooData& data, AooFlag flags, AooResponseHandler cb, void *context) {
    auto cmd = std::make_unique<custom_request_cmd>(data, flags, cb, context);
    push_command(std::move(cmd));
    return kAooOk;
}

AOO_API AooError AOO_CALL AooClient_findPeerByName(
        AooClient *client, const AooChar *group, const AooChar *user,
        AooId *groupId, AooId *userId, void *address, AooAddrSize *addrlen)
{
    return client->findPeerByName(group, user, groupId, userId, address, addrlen);
}

AooError AOO_CALL aoo::net::Client::findPeerByName(
        const char *group, const char *user, AooId *groupId, AooId *userId,
        void *address, AooAddrSize *addrlen)
{
    peer_lock lock(peers_);
    for (auto& p : peers_){
        if (p.match(group, user)){
            if (groupId) {
                *groupId = p.group_id();
            }
            if (userId) {
                *userId = p.user_id();
            }
            if (address && addrlen) {
                auto addr = p.address();
                if (addr.valid()) {
                    if (*addrlen >= addr.length()) {
                        memcpy(address, addr.address(), addr.length());
                        *addrlen = addr.length();
                    } else {
                        return kAooErrorInsufficientBuffer;
                    }
                } else {
                    // TODO: maybe return kAooErrorNotInitialized?
                    *addrlen = 0;
                }
            }
            return kAooOk;
        }
    }
    return kAooErrorNotFound;
}

AOO_API AooError AOO_CALL AooClient_findPeerByAddress(
        AooClient *client, const void *address, AooAddrSize addrlen,
        AooId *groupId, AooId *userId) {
    return client->findPeerByAddress(address, addrlen, groupId, userId);
}

AooError AOO_CALL aoo::net::Client::findPeerByAddress(
        const void *address, AooAddrSize addrlen, AooId *groupId, AooId *userId)
{
    ip_address addr((const struct sockaddr *)address, addrlen);
    peer_lock lock(peers_);
    for (auto& p : peers_){
        if (p.match(addr)) {
            if (groupId) {
                *groupId = p.group_id();
            }
            if (userId) {
                *userId = p.user_id();
            }
            return kAooOk;
        }
    }
    return kAooErrorNotFound;
}

AOO_API AooError AOO_CALL AooClient_getPeerName(
        AooClient *client, AooId group, AooId user,
        AooChar *groupNameBuffer, AooSize *groupNameSize,
        AooChar *userNameBuffer, AooSize *userNameSize) {
    return client->getPeerName(group, user, groupNameBuffer, groupNameSize,
                               userNameBuffer, userNameSize);
}

AooError AOO_CALL aoo::net::Client::getPeerName(
        AooId group, AooId user,
        AooChar *groupNameBuffer, AooSize *groupNameSize,
        AooChar *userNameBuffer, AooSize *userNameSize)
{
    peer_lock lock(peers_);
    for (auto& p : peers_){
        if (p.match(group, user)) {
            if (groupNameBuffer && groupNameSize) {
                auto len = p.group_name().size() + 1;
                if (*groupNameSize >= len) {
                    memcpy(groupNameBuffer, p.group_name().c_str(), len);
                    *groupNameSize = len;
                } else {
                    return kAooErrorInsufficientBuffer;
                }
            }
            if (userNameBuffer && userNameSize) {
                auto len = p.user_name().size() + 1;
                if (*userNameSize >= len) {
                    memcpy(userNameBuffer, p.user_name().c_str(), len);
                    *userNameSize = len;
                } else {
                    return kAooErrorInsufficientBuffer;
                }
            }
            return kAooOk;
        }
    }
    return kAooErrorNotFound;
}

AOO_API AooError AOO_CALL AooClient_sendMessage(
        AooClient *client, AooId group, AooId user,
        const AooData *message, AooNtpTime timeStamp, AooFlag flags)
{
    if (!message) {
        return kAooErrorBadArgument;
    }
    return client->sendMessage(group, user, *message, timeStamp, flags);
}

AooError AOO_CALL aoo::net::Client::sendMessage(
        AooId group, AooId user, const AooData& msg,
        AooNtpTime timeStamp, AooFlag flags)
{
    // TODO implement ack mechanism over UDP.
    bool reliable = flags & kAooMessageReliable;
    message m(group, user, timeStamp, msg, reliable);
    udp_client_.queue_message(std::move(m));
    return kAooOk;
}

AOO_API AooError AOO_CALL AooClient_handleMessage(
        AooClient *client, const AooByte *data,
        AooInt32 size, const void *addr, AooAddrSize len)
{
    return client->handleMessage(data, size, addr, len);
}

AooError AOO_CALL aoo::net::Client::handleMessage(
        const AooByte *data, AooInt32 size,
        const void *addr, AooAddrSize len)
{
    AooMsgType type;
    AooId id;
    AooInt32 onset;
    auto err = aoo_parsePattern(data, size, &type, &id, &onset);
    if (err != kAooOk){
        LOG_WARNING("AooClient: not an AOO NET message!");
        return kAooErrorBadFormat;
    }

    if (type == kAooMsgTypeSource){
        // forward to matching source
        for (auto& s : sources_){
            if (s.id == id){
                return s.source->handleMessage(data, size, addr, len);
            }
        }
        LOG_WARNING("AooClient: handle_message(): source not found");
        return kAooErrorNotFound;
    } else if (type == kAooMsgTypeSink){
        // forward to matching sink
        for (auto& s : sinks_){
            if (s.id == id){
                return s.sink->handleMessage(data, size, addr, len);
            }
        }
        LOG_WARNING("AooClient: handle_message(): sink not found");
        return kAooErrorNotFound;
    } else {
        // forward to UDP client
        ip_address address((const sockaddr *)addr, len);
        if (binmsg_check(data, size)) {
            return udp_client_.handle_bin_message(*this, data, size, address, type, onset);
        } else {
            return udp_client_.handle_osc_message(*this, data, size, address, type, onset);
        }
    }
}

AOO_API AooError AOO_CALL AooClient_send(
        AooClient *client, AooSendFunc fn, void *user){
    return client->send(fn, user);
}

AooError AOO_CALL aoo::net::Client::send(
        AooSendFunc fn, void *user)
{
    auto now = time_tag::now();
    sendfn reply(fn, user);

#if AOO_CLIENT_SIMULATE
    auto drop = sim_packet_loss_.load();
    auto reorder = sim_packet_reorder_.load();
    auto jitter = sim_packet_jitter_.load();

    // dispatch delayed packets - *before* replacing the send function!
    // - unless we want to simulate jitter
    if (!jitter) {
        while (!packetqueue_.empty()) {
            auto& p = packetqueue_.top();
            if (p.tt <= now) {
                reply(p.data.data(), p.data.size(), p.addr);
                packetqueue_.pop();
            } else {
                break;
            }
        }
    }

    struct wrap_state {
        Client *client;
        sendfn reply;
        time_tag now;
        float drop;
        float reorder;
        bool jitter;
    } state;

    auto wrapfn = [](void *user, const AooByte *data, AooInt32 size,
            const void *address, AooAddrSize addrlen, AooFlag flag) -> AooInt32 {
        auto state = (wrap_state *)user;

        thread_local std::default_random_engine gen(std::random_device{}());
        std::uniform_real_distribution dist;

        if (state->drop > 0) {
            if (dist(gen) <= state->drop) {
                // LOG_DEBUG("AooClient: drop packet");
                return 0; // drop packet
            }
        }

        aoo::ip_address addr((const struct sockaddr *)address, addrlen);

        if (state->jitter || (state->reorder > 0)) {
            // queue for later
            netpacket p;
            p.data.assign(data, data + size);
            p.addr = addr;
            p.tt = state->now;
            if (state->reorder > 0) {
                // add random delay
                auto delay = dist(gen) * state->reorder;
                p.tt += time_tag::from_seconds(delay);
            }
            p.sequence = state->client->packet_sequence_++;
            // LOG_DEBUG("AooClient: delay packet (tt: " << p.tt << ")");
            state->client->packetqueue_.push(std::move(p));
        } else {
            // send immediately
            state->reply(data, size, addr);
        }

        return 0;
    };

    if (drop > 0 || reorder > 0 || jitter) {
        // wrap send function
        state.client = this;
        state.reply = reply;
        state.now = now;
        state.drop = drop;
        state.reorder = reorder;
        state.jitter = jitter;

        // replace
        reply = sendfn(wrapfn, &state);
        fn = wrapfn;
        user = &state;
    }
#endif

    // send sources and sinks
    for (auto& s : sources_){
        s.source->send(fn, user);
    }
    for (auto& s : sinks_){
        s.sink->send(fn, user);
    }
    // send server/peer messages
    if (state_.load() != client_state::disconnected) {
        udp_client_.update(*this, reply, now);

        // update peers
        peer_lock lock(peers_);
        for (auto& p : peers_){
            p.send(*this, reply, now);
        }
    }

    return kAooOk;
}

AOO_API AooError AOO_CALL AooClient_setEventHandler(
        AooClient *sink, AooEventHandler fn, void *user, AooEventMode mode)
{
    return sink->setEventHandler(fn, user, mode);
}

AooError AOO_CALL aoo::net::Client::setEventHandler(
        AooEventHandler fn, void *user, AooEventMode mode)
{
    eventhandler_ = fn;
    eventcontext_ = user;
    eventmode_ = (AooEventMode)mode;
    return kAooOk;
}

AOO_API AooBool AOO_CALL AooClient_eventsAvailable(AooClient *client){
    return client->eventsAvailable();
}

AooBool AOO_CALL aoo::net::Client::eventsAvailable(){
    return !events_.empty();
}

AOO_API AooError AOO_CALL AooClient_pollEvents(AooClient *client){
    return client->pollEvents();
}

AooError AOO_CALL aoo::net::Client::pollEvents(){
    // always thread-safe
    event_handler fn(eventhandler_, eventcontext_, kAooThreadLevelUnknown);
    event_ptr e;
    while (events_.try_pop(e)){
        e->dispatch(fn);
    }
    return kAooOk;
}

AOO_API AooError AOO_CALL AooClient_sendRequest(
        AooClient *client, const AooRequest *request,
        AooResponseHandler callback, void *user, AooFlag flags) {
    if (!request) {
        return kAooErrorBadArgument;
    }
    return client->sendRequest(*request, callback, user, flags);
}

AooError AOO_CALL aoo::net::Client::sendRequest(
        const AooRequest& request, AooResponseHandler callback, void *user, AooFlag flags)
{
    LOG_ERROR("AooClient: unknown request " << request.type);
    return kAooErrorNotImplemented;
}

AOO_API AooError AOO_CALL AooClient_control(
        AooClient *client, AooCtl ctl, AooIntPtr index, void *ptr, AooSize size)
{
    return client->control(ctl, index, ptr, size);
}

template<typename T>
T& as(void *p){
    return *reinterpret_cast<T *>(p);
}

#define CHECKARG(type) assert(size == sizeof(type))

AooError AOO_CALL aoo::net::Client::control(
        AooCtl ctl, AooIntPtr index, void *ptr, AooSize size)
{
    switch(ctl){
    case kAooCtlSetBinaryClientMsg:
        CHECKARG(AooBool);
        binary_.store(as<AooBool>(ptr));
        break;
    case kAooCtlGetBinaryClientMsg:
        CHECKARG(AooBool);
        as<AooBool>(ptr) = binary_.load();
        break;
    case kAooCtlAddInterfaceAddress:
    {
        auto ifaddr = (const AooChar *)index;
        if (ifaddr == nullptr) {
            return kAooErrorBadArgument;
        }
        // test address
        ip_address addr(ifaddr, 0);
        if (!addr.valid() || addr.is_ipv4_mapped()) {
            return kAooErrorBadFormat;
        }
        sync::scoped_lock<sync::shared_mutex> lock(mutex_);
        if (std::find(interfaces_.begin(), interfaces_.end(), ifaddr)
                == interfaces_.end()) {
            interfaces_.push_back(ifaddr);
        } else {
            return kAooErrorAlreadyExists;
        }
        break;
    }
    case kAooCtlRemoveInterfaceAddress:
    {
        sync::scoped_lock<sync::shared_mutex> lock(mutex_);
        auto ifaddr = (const AooChar *)index;
        if (ifaddr != NULL) {
            if (auto it = std::find(interfaces_.begin(), interfaces_.end(), ifaddr);
                    it != interfaces_.end()) {
                interfaces_.erase(it);
            } else {
                return kAooErrorNotFound;
            }
        } else {
            interfaces_.clear();
        }
        break;
    }
    case kAooCtlNeedRelay:
    {
        CHECKARG(AooBool);
        auto ep = reinterpret_cast<const AooEndpoint *>(index);
        ip_address addr((sockaddr *)ep->address, ep->addrlen);
        peer_lock lock(peers_);
        for (auto& peer : peers_){
            if (peer.match(addr)){
                as<AooBool>(ptr) = peer.need_relay();
                return kAooOk;
            }
        }
        return kAooErrorNotFound;
    }
    case kAooCtlGetRelayAddress:
    {
        CHECKARG(ip_address);
        auto ep = reinterpret_cast<const AooEndpoint *>(index);
        ip_address addr((sockaddr *)ep->address, ep->addrlen);
        peer_lock lock(peers_);
        for (auto& peer : peers_){
            if (peer.match(addr)){
                as<ip_address>(ptr) = peer.relay_address();
                return kAooOk;
            }
        }
        return kAooErrorNotFound;
    }
#if AOO_CLIENT_SIMULATE
    case kAooCtlSetSimulatePacketReorder:
        CHECKARG(AooSeconds);
        sim_packet_reorder_.store(as<AooSeconds>(ptr));
        break;
    case kAooCtlSetSimulatePacketLoss:
        CHECKARG(float);
        sim_packet_loss_.store(as<float>(ptr));
        break;
    case kAooCtlSetSimulatePacketJitter:
        CHECKARG(AooBool);
        sim_packet_jitter_.store(as<AooBool>(ptr));
        break;
#endif
    default:
        LOG_WARNING("AooClient: unsupported control " << ctl);
        return kAooErrorNotImplemented;
    }
    return kAooOk;
}

namespace aoo {
namespace net {

bool Client::handle_peer_osc_message(const osc::ReceivedMessage& msg, int onset,
                                     const ip_address& addr)
{
    // all peer messages start with group ID and user ID, so we can easily
    // forward them to the corresponding peer.
    auto pattern = msg.AddressPattern() + onset;
    try {
        auto it = msg.ArgumentsBegin();
        auto group = (it++)->AsInt32();
        auto user = (it++)->AsInt32();
        auto remaining = msg.ArgumentCount() - 2;
        // forward to matching peer
        peer_lock lock(peers_);
        for (auto& p : peers_) {
            if (p.match(group, user)) {
                p.handle_osc_message(*this, pattern, it, remaining, addr);
                return true;
            }
        }
        LOG_WARNING("AooClient: got " << pattern << " message from unknown peer "
                    << group << "|" << user << " " << addr);
        return false;
    } catch (const osc::Exception& e){
        LOG_ERROR("AooClient: got bad " << pattern
                  << " message from peer " << addr << ": " << e.what());
        return false;
    }
}

bool Client::handle_peer_bin_message(const AooByte *data, AooSize size, int onset,
                                     const ip_address& addr) {
    // all peer messages start with group ID and user ID, so we can easily
    // forward them to the corresponding peer.
    auto group = binmsg_group(data, size);
    auto user = binmsg_user(data, size);
    // forward to matching peer
    peer_lock lock(peers_);
    for (auto& p : peers_) {
        if (p.match(group, user)) {
            p.handle_bin_message(*this, data, size, onset, addr);
            return true;
        }
    }
    LOG_WARNING("AooClient: got binary message from unknown peer "
                << group << "|" << user << " " << addr);
    return false;
}
//------------------ connect/login -----------------------//

void Client::perform(const connect_cmd& cmd)
{
    auto state = state_.load();
    if (state != client_state::disconnected){
        auto code = (state == client_state::connected) ?
            kAooErrorAlreadyConnected : kAooErrorRequestInProgress;

        cmd.reply_error(code);

        return;
    }

    auto addrlist = ip_address::resolve(cmd.host_.name, cmd.host_.port,
                                        udp_client_.address_family(),
                                        udp_client_.use_ipv4_mapped());
    if (addrlist.empty()){
        int err = socket_errno();
        auto errmsg = socket_strerror(err);
        // LATER think about best way for error handling. Maybe exception?
        LOG_ERROR("AooClient: could not resolve hostname: " << errmsg);
        cmd.reply_error(kAooErrorSystem, err, errmsg.c_str());
        return;
    }

    assert(connection_ == nullptr);
    assert(memberships_.empty());
    connection_ = std::make_unique<connect_cmd>(cmd);

    LOG_DEBUG("AooClient: server address list:");
    for (auto& addr : addrlist){
        LOG_DEBUG("\t" << addr);
    }

    // prefer IPv4(-mapped) server address.
    // Typically, we need to contact the UDP server to obtain our public
    // IPv4(-mapped) address. In case the server address is IPv6-only,
    // we ping it nevertheless, e.g. in case we could not obtain our
    // global IPv6 address (for whatever reason).
    std::sort(addrlist.begin(), addrlist.end(), [](auto& a, auto& b) {
        return ((a.type() == ip_address::IPv4) || (a.is_ipv4_mapped()))
               && b.type() == ip_address::IPv6;
    });
    state_.store(client_state::handshake);
    udp_client_.start_handshake(addrlist.front());
}

int Client::try_connect(const ip_host& server){
    socket_ = socket_tcp(0);
    if (socket_ < 0){
        int err = socket_errno();
        LOG_ERROR("AooClient: couldn't create socket: " << socket_strerror(err));
        return err;
    }

    auto type = socket_family(socket_);
    auto addrlist = ip_address::resolve(server.name, server.port, type, true);
    if (addrlist.empty()) {
        int err = socket_errno();
        LOG_ERROR("AooClient: couldn't resolve host name: " << socket_strerror(err));
        return err;
    }
    // sort IPv4(-mapped) first because it is more likely for an AOO server to be IP4-only than
    // to be IPv6-only
    std::sort(addrlist.begin(), addrlist.end(), [](auto& a, auto& b) {
        return ((a.type() == ip_address::IPv4) || (a.is_ipv4_mapped()))
               && b.type() == ip_address::IPv6;
    });

    LOG_VERBOSE("AooClient: try to connect to " << server.name << " on port " << server.port);
    // try to connect to both addresses (just because the hostname resolves to IPv4
    // and IPv6 addresses does not mean that the AOO server actually supports both).
    for (auto& addr : addrlist) {
        LOG_DEBUG("AooClient: try to connect to " << addr);
        // try to connect (LATER make timeout configurable)
        if (socket_connect(socket_, addr, 5.0) == 0) {
            LOG_VERBOSE("AooClient: successfully connected to " << addr);
            return 0;
        }
    }
    int err = socket_errno();
    LOG_ERROR("AooClient: couldn't connect to " << server.name << " on port "
               << server.port << ": " << socket_strerror(err));
    return err;
}

void Client::perform(const login_cmd& cmd) {
    assert(connection_ != nullptr);
    assert(memberships_.empty());

    state_.store(client_state::connecting);

    int err = try_connect(connection_->host_);
    if (err != 0){
        auto msg = socket_strerror(err);
        connection_->reply_error(kAooErrorSocket, err, msg.c_str());
        close();
        return;
    }

    // send login request
    auto token = next_token_++;
    // create address list; start with local/global addresses
    ip_address_list addrlist = local_addr_;
    // add public IP address
    if (cmd.public_ip_.valid()) {
        addrlist.push_back(cmd.public_ip_);
    }
    // add user provided interface addresses
    {
        sync::scoped_shared_lock<sync::shared_mutex> lock(mutex_);
        for (auto& ifaddr : interfaces_) {
            ip_address addr(ifaddr, udp_client_.port());
            if (addr.valid()) {
                addrlist.push_back(addr);
            } else {
                LOG_ERROR("AooClient: ignore invalid interface address " << ifaddr);
            }
        }
    }

    LOG_DEBUG("AooClient: address list:");
    for (auto& addr : addrlist) {
        LOG_DEBUG("\t" << addr);
    }

    auto msg = start_server_message(connection_->metadata_.size());

    msg << osc::BeginMessage(kAooMsgServerLogin)
        << token << aoo_getVersionString()
        << encrypt(connection_->pwd_).c_str()
        << (int32_t)addrlist.size();
    for (auto& addr : addrlist){
        msg << addr;
    }
    msg << connection_->metadata_
        << osc::EndMessage;

    send_server_message(msg);
}

void Client::perform(const timeout_cmd& cmd) {
    if (connection_ && state_.load() == client_state::handshake) {
        connection_->reply_error(kAooErrorUDPHandshakeTimeOut);
        close();
    }
}

//------------------ disconnect -----------------------//

void Client::perform(const disconnect_cmd& cmd) {
    auto state = state_.load();
    if (state != client_state::connected) {
        auto code = (state == client_state::disconnected) ?
                kAooErrorNotConnected : kAooErrorAlreadyConnected;

        cmd.reply_error(code);

        return;
    }

    close(true); // do not send disconnect event!

    AooResponseDisconnect response;
    AOO_RESPONSE_INIT(&response, Disconnect, structSize);

    cmd.reply((AooResponse&)response); // always succeeds
}

//------------------ group_join -----------------------//

void Client::perform(const group_join_cmd& cmd)
{
    if (state_.load() != client_state::connected) {
        cmd.reply_error(kAooErrorNotConnected);
        return;
    }
    // check if we're already a group member
    for (auto& m : memberships_) {
        if (m.group_name == cmd.group_name_) {
            cmd.reply_error(kAooErrorAlreadyGroupMember);
            return;
        }
    }
    auto token = next_token_++;
    pending_requests_.emplace(token, std::make_unique<group_join_cmd>(cmd));

    auto msg = start_server_message(cmd.group_md_.size() + cmd.user_md_.size());

    msg << osc::BeginMessage(kAooMsgServerGroupJoin) << token
        << cmd.group_name_.c_str() << encrypt(cmd.group_pwd_).c_str()
        << cmd.user_name_.c_str() << encrypt(cmd.user_pwd_).c_str()
        << cmd.group_md_ << cmd.user_md_ << cmd.relay_
        << osc::EndMessage;

    send_server_message(msg);
}

void Client::handle_response(const group_join_cmd& cmd, const osc::ReceivedMessage& msg) {
    auto it = msg.ArgumentsBegin();
    (it++)->AsInt32(); // skip token
    auto result = (it++)->AsInt32();
    if (result == kAooErrorNone) {
        auto group_id = (it++)->AsInt32();
        auto group_flags = (AooFlag)(it++)->AsInt32();
        auto user_id = (it++)->AsInt32();
        auto user_flags = (AooFlag)(it++)->AsInt32();
        auto group_md = osc_read_metadata(it); // optional
        auto user_md = osc_read_metadata(it); // optional
        auto private_md = osc_read_metadata(it); // optional
        auto relay = osc_read_host(it); // optional

        // add group membership
        if (!find_group_membership(cmd.group_name_)) {
            group_membership m { cmd.group_name_, cmd.user_name_, group_id, user_id, {} };

            // add relay servers (in descending priority)
            auto family = udp_client_.address_family();
            auto ipv4mapped = udp_client_.use_ipv4_mapped();
            // 1) our own relay
            if (cmd.relay_.valid()) {
                auto addrlist = ip_address::resolve(cmd.relay_.name, cmd.relay_.port,
                                                    family, ipv4mapped);
                m.relay_list.insert(m.relay_list.end(), addrlist.begin(), addrlist.end());
            }
            // 2) server group relay
            if (relay.port > 0) {
                auto addrlist = ip_address::resolve(cmd.relay_.name, cmd.relay_.port,
                                                    family, ipv4mapped);
                m.relay_list.insert(m.relay_list.end(), addrlist.begin(), addrlist.end());
            }
            // 3) use UDP server as relay
            if (server_relay_) {
                auto& host = connection_->host_;
                auto addrlist = ip_address::resolve(host.name, host.port, family, ipv4mapped);
                m.relay_list.insert(m.relay_list.end(), addrlist.begin(), addrlist.end());
            }
            memberships_.push_back(std::move(m));
        } else {
            // shouldn't happen...
            LOG_ERROR("AooClient: group join response: already a member of group " << cmd.group_name_);
            cmd.reply_error(kAooErrorAlreadyGroupMember);
            return;
        }

        AooResponseGroupJoin response;
        AOO_RESPONSE_INIT(&response, GroupJoin, relayAddress);
        response.groupId = group_id;
        response.groupFlags = group_flags;
        response.userId = user_id;
        response.userFlags = user_flags;
        response.groupMetadata = group_md.size ? &group_md : nullptr;
        response.userMetadata = user_md.size ? &user_md : nullptr;
        response.privateMetadata = private_md.size ? &private_md : nullptr;
        response.relayAddress = relay.port > 0 ? &relay : nullptr;

        cmd.reply((AooResponse&)response);
        LOG_VERBOSE("AooClient: successfully joined group " << cmd.group_name_);
    } else {
        auto code = (it++)->AsInt32();
        auto msg = (it++)->AsString();
        cmd.reply_error(result, code, msg);
        LOG_WARNING("AooClient: couldn't join group " << cmd.group_name_ << ": "
                    << response_error_message(result, code, msg));
    }
}

//------------------ group_leave -----------------------//

void Client::perform(const group_leave_cmd& cmd)
{
    // first check for group membership
    if (find_group_membership(cmd.group_) == nullptr) {
        LOG_WARNING("AooClient: couldn't leave group " << cmd.group_ << ": not a group member");
        cmd.reply_error(kAooErrorNotGroupMember);
        return;
    }

    auto token = next_token_++;
    pending_requests_.emplace(token, std::make_unique<group_leave_cmd>(cmd));

    auto msg = start_server_message();

    msg << osc::BeginMessage(kAooMsgServerGroupLeave)
        << token << cmd.group_ << osc::EndMessage;

    send_server_message(msg);
}

void Client::handle_response(const group_leave_cmd& cmd, const osc::ReceivedMessage& msg) {
    auto it = msg.ArgumentsBegin();
    (it++)->AsInt32(); // skip token
    auto result = (it++)->AsInt32();
    if (result == kAooErrorNone) {
        // remove all peers from this group
        peer_lock lock(peers_);
        for (auto it = peers_.begin(); it != peers_.end(); ){
            if (it->match(cmd.group_)){
                it = peers_.erase(it);
            } else {
                ++it;
            }
        }
        lock.unlock();

        // remove group membership
        auto mit = std::find_if(memberships_.begin(), memberships_.end(),
                                [&](auto& m) { return m.group_id == cmd.group_; });
        if (mit != memberships_.end()) {
            memberships_.erase(mit);
        } else {
            LOG_ERROR("AooClient: group leave response: not a member of group " << cmd.group_);
        }

        AooResponseGroupLeave response;
        AOO_RESPONSE_INIT(&response, GroupLeave, structSize);

        cmd.reply((AooResponse&)response);
        LOG_VERBOSE("AooClient: successfully left group " << cmd.group_);
    } else {
        auto code = (it++)->AsInt32();
        auto msg = (it++)->AsString();
        cmd.reply_error(result, code, msg);
        LOG_WARNING("AooClient: couldn't leave group " << cmd.group_ << ": "
                    << response_error_message(result, code, msg));
    }
}

//------------------ group_update -----------------------//

void Client::perform(const group_update_cmd& cmd) {
    // first check for group membership
    if (find_group_membership(cmd.group_) == nullptr) {
        LOG_WARNING("AooClient: couldn't update group " << cmd.group_ << ": not a group member");
        cmd.reply_error(kAooErrorNotGroupMember);
        return;
    }

    auto token = next_token_++;
    pending_requests_.emplace(token, std::make_unique<group_update_cmd>(cmd));

    auto msg = start_server_message(cmd.md_.size());

    msg << osc::BeginMessage(kAooMsgServerGroupUpdate)
        << token << cmd.group_ << cmd.md_ << osc::EndMessage;

    send_server_message(msg);
}

void Client::handle_response(const group_update_cmd& cmd, const osc::ReceivedMessage& msg) {
    auto it = msg.ArgumentsBegin();
    (it++)->AsInt32(); // skip token
    auto result = (it++)->AsInt32();
    if (result == kAooErrorNone) {
        AooResponseGroupUpdate response;
        AOO_RESPONSE_INIT(&response, GroupUpdate, groupMetadata);
        response.groupMetadata.type = cmd.md_.type();
        response.groupMetadata.data = cmd.md_.data();
        response.groupMetadata.size = cmd.md_.size();

        cmd.reply((AooResponse&)response);
        LOG_VERBOSE("AooClient: successfully updated group " << cmd.group_);
    } else {
        auto code = (it++)->AsInt32();
        auto msg = (it++)->AsString();
        cmd.reply_error(result, code, msg);
        LOG_WARNING("AooClient: could not update group " << cmd.group_ << ": "
                    << response_error_message(result, code, msg));
    }
}

//------------------ user_update -----------------------//

void Client::perform(const user_update_cmd& cmd) {
    // first check for group membership
    auto group = find_group_membership(cmd.group_);
    if (group == nullptr) {
        LOG_WARNING("AooClient: couldn't update user in group "
                    << cmd.group_ << ": not a group member");
        cmd.reply_error(kAooErrorNotGroupMember);
        return;
    }
    auto token = next_token_++;
    pending_requests_.emplace(token, std::make_unique<user_update_cmd>(cmd));

    auto msg = start_server_message(cmd.md_.size());

    msg << osc::BeginMessage(kAooMsgServerUserUpdate)
        << token << cmd.group_ << cmd.md_ << osc::EndMessage;

    send_server_message(msg);
}

void Client::handle_response(const user_update_cmd& cmd, const osc::ReceivedMessage& msg) {
    auto it = msg.ArgumentsBegin();
    (it++)->AsInt32(); // skip token
    auto result = (it++)->AsInt32();
    if (result == kAooErrorNone) {
        AooResponseUserUpdate response;
        AOO_RESPONSE_INIT(&response, UserUpdate, userMetadata);
        response.userMetadata.type = cmd.md_.type();
        response.userMetadata.data = cmd.md_.data();
        response.userMetadata.size = cmd.md_.size();

        cmd.reply((AooResponse&)response);
        LOG_VERBOSE("AooClient: successfully updated user in group " << cmd.group_);
    } else {
        auto code = (it++)->AsInt32();
        auto msg = (it++)->AsString();
        cmd.reply_error(result, code, msg);
        LOG_WARNING("AooClient: could not update user in group " << cmd.group_ << ": "
                    << response_error_message(result, code, msg));
    }
}

//------------------ custom_request -----------------------//

void Client::perform(const custom_request_cmd& cmd) {
    if (state_.load() != client_state::connected) {
        cmd.reply_error(kAooErrorNotConnected);
        return;
    }

    auto token = next_token_++;
    pending_requests_.emplace(token, std::make_unique<custom_request_cmd>(cmd));

    auto msg = start_server_message(cmd.data_.size());

    msg << osc::BeginMessage(kAooMsgServerRequest)
        << token << (int32_t)cmd.flags_ << cmd.data_
        << osc::EndMessage;

    send_server_message(msg);
}

void Client::perform(const message& m, const sendfn& fn) {
    bool binary = binary_.load();
    // LATER optimize this by overwriting the group ID and local user ID
    peer_lock lock(peers_);
    for (auto& peer : peers_) {
        if (peer.connected() && peer.match_wildcard(m.group_, m.user_)) {
            peer.send_message(m, fn, binary);
        }
    }
}

void Client::send_event(event_ptr e)
{
    switch (eventmode_){
    case kAooEventModePoll:
        events_.push(std::move(e));
        break;
    case kAooEventModeCallback:
    {
        event_handler fn(eventhandler_, eventcontext_, kAooThreadLevelNetwork);
        e->dispatch(fn);
        break;
    }
    default:
        break;
    }
}

void Client::push_command(command_ptr cmd){
    commands_.push(std::move(cmd));
    signal();
}

bool Client::wait_for_event(float timeout){
    // LOG_DEBUG("AooClient: wait " << timeout << " seconds");

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
    if (result == 0) {
        return false; // nothing to do or timeout
    } else if (result < 0) {
        int err = socket_errno();
        if (err == EINTR) {
            return true;
        } else {
            // fatal error
            LOG_ERROR("AooClient: poll() failed: " << socket_strerror(err));
            throw error(kAooErrorSocket, "poll() failed");
        }
    }

    // event socket
    if (fds[0].revents){
        // read empty packet
        char buf[64];
        recv(eventsocket_, buf, sizeof(buf), 0);
        // LOG_DEBUG("AooClient: got signalled");
    }

    // tcp socket
    if (socket_ >= 0 && fds[1].revents){
        if (fds[1].revents & POLLERR) {
            LOG_DEBUG("AooClient: POLLERR");
        }
        if (fds[1].revents & POLLHUP) {
            LOG_DEBUG("AooClient: POLLHUP");
        }
        receive_data();
    }

    return true;
}

void Client::receive_data(){
    char buffer[AOO_MAX_PACKET_SIZE];
    auto result = recv(socket_, buffer, sizeof(buffer), 0);
    if (result > 0){
        try {
            receiver_.handle_message(buffer, result,
                    [&](const osc::ReceivedPacket& packet) {
                osc::ReceivedMessage msg(packet);
                handle_server_message(msg, packet.Size());
            });
        } catch (const osc::Exception& e) {
            LOG_ERROR("AooClient: exception in receive_data: " << e.what());
            on_exception("server TCP message", e);
        }
    } else if (result == 0) {
        // connection closed by the remote server
        on_socket_error(0);
    } else {
        int err = socket_errno();
        LOG_ERROR("AooClient: recv() failed (" << err << ")");
        on_socket_error(err);
    }
}

osc::OutboundPacketStream Client::start_server_message(size_t extra) {
    if (extra > 0) {
        auto total = AOO_MAX_PACKET_SIZE + extra;
        if (sendbuffer_.size() < total) {
            sendbuffer_.resize(total);
        }
    }
    // leave space for message size (int32_t)
    return osc::OutboundPacketStream(sendbuffer_.data() + 4, sendbuffer_.size() - 4);
}

void Client::send_server_message(const osc::OutboundPacketStream& msg) {
    // prepend message size (int32_t)
    auto data = msg.Data() - 4;
    auto size = msg.Size() + 4;
    // we know that the buffer is not really constnat
    aoo::to_bytes<int32_t>(msg.Size(), const_cast<char *>(data));

    int32_t nbytes = 0;
    while (nbytes < size){
        auto result = ::send(socket_, data + nbytes, size - nbytes, 0);
        if (result >= 0){
            nbytes += result;
            // LOG_DEBUG("AooClient: sent " << res << " bytes");
        } else {
            auto err = socket_errno();

            LOG_ERROR("AooClient: send() failed (" << err << ")");
            on_socket_error(err);
            return;
        }
    }
    LOG_DEBUG("AooClient: sent " << (data + 4) << " to server");
}

void Client::handle_server_message(const osc::ReceivedMessage& msg, int32_t n){
    AooMsgType type;
    int32_t onset;
    auto err = parse_pattern((const AooByte *)msg.AddressPattern(), n, type, onset);
    if (err != kAooOk){
        LOG_WARNING("AooClient: not an AOO NET message!");
        return;
    }

    try {
        if (type == kAooMsgTypeClient){
            // now compare subpattern
            auto pattern = msg.AddressPattern() + onset;
            LOG_DEBUG("AooClient: got message " << pattern << " from server");

            if (!strcmp(pattern, kAooMsgPong)){
                LOG_DEBUG("AooClient: got TCP pong from server");
            } else if (!strcmp(pattern, kAooMsgPeerJoin)) {
                handle_peer_add(msg);
            } else if (!strcmp(pattern, kAooMsgPeerLeave)) {
                handle_peer_remove(msg);
            } else if (!strcmp(pattern, kAooMsgPeerChanged)) {
                handle_peer_changed(msg);
            } else if (!strcmp(pattern, kAooMsgLogin)) {
                handle_login(msg);
            } else if (!strcmp(pattern, kAooMsgMessage)) {
                handle_server_notification(msg);
            } else if (!strcmp(pattern, kAooMsgGroupChanged)) {
                handle_group_changed(msg);
            } else if (!strcmp(pattern, kAooMsgUserChanged)) {
                handle_user_changed(msg);
            } else if (!strcmp(pattern, kAooMsgGroupEject)) {
                handle_group_eject(msg);
            } else if (!strcmp(pattern, kAooMsgGroupJoin) ||
                       !strcmp(pattern, kAooMsgGroupLeave) ||
                       !strcmp(pattern, kAooMsgGroupUpdate) ||
                       !strcmp(pattern, kAooMsgUserUpdate) ||
                       !strcmp(pattern, kAooMsgRequest)) {
                // handle response
                auto token = msg.ArgumentsBegin()->AsInt32();
                auto it = pending_requests_.find(token);
                if (it != pending_requests_.end()) {
                    it->second->handle_response(*this, msg);
                    pending_requests_.erase(it);
                } else {
                    LOG_ERROR("AooClient: couldn't find matching request");
                }
            } else {
                LOG_WARNING("AooClient: got unspported server message " << msg.AddressPattern());
            }
        } else {
            LOG_WARNING("AooClient: got unsupported message " << msg.AddressPattern());
        }
    } catch (const osc::Exception& e){
        LOG_ERROR("AooClient: exception on handling " << msg.AddressPattern()
                  << " message: " << e.what());
        on_exception("server TCP message", e, msg.AddressPattern());
    }
}

void Client::handle_login(const osc::ReceivedMessage& msg){
    // make sure that state hasn't changed
    if (connection_) {
        auto it = msg.ArgumentsBegin();
        (it++)->AsInt32(); // skip token
        auto result = (it++)->AsInt32();
        if (result == kAooErrorNone){
            auto version = (it++)->AsString();
            auto id = (AooId)(it++)->AsInt32();
            auto flags = (AooFlag)(it++)->AsInt32();
            auto metadata = osc_read_metadata(it); // optional

            // check version
            if (auto err = check_version(version); err != kAooOk) {
                LOG_WARNING("AooClient: login failed: " << aoo_strerror(err));
                connection_->reply_error(err);
                close();
                return;
            }

            server_relay_ = flags & kAooServerRelay;

            // connected!
            state_.store(client_state::connected);
            LOG_VERBOSE("AooClient: successfully logged in (client ID: "
                        << id << ")");
            // notify
            AooResponseConnect response;
            AOO_RESPONSE_INIT(&response, Connect, metadata);
            response.clientId = id;
            response.flags = flags;
            response.version = version;
            response.metadata = metadata.data ? &metadata : nullptr;

            connection_->reply((AooResponse&)response);
        } else {
            auto code = (it++)->AsInt32();
            auto msg = (it++)->AsString();
            LOG_WARNING("AooClient: login failed: "
                        << response_error_message(result, code, msg));
            connection_->reply_error(result, code, msg);
            close();
        }
    }
}

void Client::handle_server_notification(const osc::ReceivedMessage& msg) {
    auto it = msg.ArgumentsBegin();
    auto message = osc_read_metadata(it);

    auto e = std::make_unique<notification_event>(message);
    send_event(std::move(e));

    LOG_DEBUG("AooClient: received server notification (" << message.type << ")");
}

void Client::handle_group_eject(const osc::ReceivedMessage& msg) {
    auto it = msg.ArgumentsBegin();
    auto group = (it++)->AsInt32();

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

    // remove group membership
    auto mit = std::find_if(memberships_.begin(), memberships_.end(),
                            [&](auto& m) { return m.group_id == group; });
    if (mit != memberships_.end()) {
        memberships_.erase(mit);
    } else {
        LOG_ERROR("AooClient: group eject: not a member of group " << group);
    }

    auto e = std::make_unique<group_eject_event>(group);
    send_event(std::move(e));

    LOG_VERBOSE("AooClient: ejected from group " << group);
}

void Client::handle_group_changed(const osc::ReceivedMessage& msg) {
    auto it = msg.ArgumentsBegin();
    auto group = (it++)->AsInt32();
    auto user = (it++)->AsInt32();
    auto md = osc_read_metadata(it);

    auto e = std::make_unique<group_update_event>(group, user, md);
    send_event(std::move(e));

    LOG_VERBOSE("AooClient: group " << group << " has been updated");
}

void Client::handle_user_changed(const osc::ReceivedMessage& msg) {
    auto it = msg.ArgumentsBegin();
    auto group = (it++)->AsInt32();
    auto user = (it++)->AsInt32();
    auto md = osc_read_metadata(it);

    auto e = std::make_unique<user_update_event>(group, user, md);
    send_event(std::move(e));

    LOG_VERBOSE("AooClient: user " << user << " has been updated");
}

static osc::ReceivedPacket unwrap_message(const osc::ReceivedMessage& msg, ip_address& addr)
{
    auto it = msg.ArgumentsBegin();

    // read address as is (always unmapped)
    addr = osc_read_address(it);

    const void *msg_data;
    osc::osc_bundle_element_size_t msg_size;
    (it++)->AsBlob(msg_data, msg_size);

    return osc::ReceivedPacket((const char *)msg_data, msg_size);
}

void Client::handle_peer_add(const osc::ReceivedMessage& msg){
    auto it = msg.ArgumentsBegin();
    auto group_name = (it++)->AsString();
    auto group_id = (it++)->AsInt32();
    auto user_name = (it++)->AsString();
    auto user_id = (it++)->AsInt32();
    auto version = (it++)->AsString();
    auto flags = (AooFlag)(it++)->AsInt32();
    // collect IP addresses
    auto addrcount = (it++)->AsInt32();
    ip_address_list addrlist;
    for (int32_t i = 0; i < addrcount; ++i){
        // read as is! They might be used as identifiers in relay message.
        auto addr = osc_read_address(it);
        if (addr.is_ipv4_mapped()) {
            // peer addresses must be unmapped!
            LOG_WARNING("AooClient: ignore IPv4-mapped peer address " << addr);
            continue;
        }
        // filter local addresses so that we don't accidentally ping ourselves!
        if (addr.valid() && std::find(local_addr_.begin(), local_addr_.end(), addr)
                                == local_addr_.end()) {
            addrlist.push_back(addr);
        } else {
            LOG_DEBUG("AooClient: ignore local address " << addr);
        }
    }
    auto metadata = osc_read_metadata(it); // optional
    auto relay = osc_read_host(it); // optional

    peer_lock lock(peers_);
    // check if peer already exists (shouldn't happen)
    for (auto& p: peers_) {
        if (p.match(group_id, user_id)){
            LOG_ERROR("AooClient: peer " << p << " already added");
            return;
        }
    }
    // find corresponding group
    auto membership = find_group_membership(group_name);
    if (!membership) {
        // shouldn't happen
        LOG_ERROR("AooClient: add peer from group " << group_name
                  << ", but we are not a group member");
        return; // ignore
    }

    // get user relay address(es)
    auto family = udp_client_.address_family();
    auto use_ipv4_mapped = udp_client_.use_ipv4_mapped();
    ip_address_list user_relay;
    if (relay.port > 0) {
        user_relay = aoo::ip_address::resolve(relay.hostName, relay.port, family, use_ipv4_mapped);
        // add to group relay list
        auto& list = membership->relay_list;
        list.insert(list.end(), user_relay.begin(), user_relay.end());
    }
    auto md = metadata.size > 0 ? &metadata : nullptr;

    auto peer = peers_.emplace_front(group_name, group_id, user_name, user_id,
                                     version, flags, md, family, use_ipv4_mapped,
                                     std::move(addrlist), membership->user_id,
                                     std::move(user_relay), membership->relay_list);

    auto e = std::make_unique<peer_event>(kAooEventPeerHandshake, *peer);
    send_event(std::move(e));

    LOG_VERBOSE("AooClient: peer " << *peer << " joined");
}

void Client::handle_peer_remove(const osc::ReceivedMessage& msg){
    auto it = msg.ArgumentsBegin();
    auto group = (it++)->AsInt32();
    auto user = (it++)->AsInt32();

    peer_lock lock(peers_);
    auto peer = std::find_if(peers_.begin(), peers_.end(),
        [&](auto& p){ return p.match(group, user); });
    if (peer == peers_.end()){
        LOG_ERROR("AooClient: couldn't remove " << group << "|" << user);
        return;
    }

    // only send event if we're connected, which means
    // that an kAooEventPeerJoin event has been sent.
    if (peer->connected()){
        auto e = std::make_unique<peer_event>(kAooEventPeerLeave, *peer);
        send_event(std::move(e));
    }

    // find corresponding group
    auto membership = find_group_membership(group);
    if (!membership) {
        // shouldn't happen
        LOG_ERROR("AooClient: remove peer from group " << peer->group_name()
                  << ", but we are not a group member");
        return; // ignore
    }
    // remove user relay(s)
    for (auto& addr : peer->user_relay()) {
        // check if this relay is used by a peer
        for (auto& p : peers_) {
            if (p.need_relay() && p.relay_address() == addr) {
                std::stringstream ss;
                ss << p << " uses a relay provided by " << *peer
                   << ", so the connection might stop working";
                auto e = std::make_unique<error_event>(0, ss.str());
                send_event(std::move(e));
            }
        }
        // remove from group relay list
        auto& list = membership->relay_list;
        for (auto ptr = list.begin(); ptr != list.end(); ++ptr) {
            if (*ptr == addr) {
                list.erase(ptr);
                break;
            }
        }
    }

    peers_.erase(peer);

    LOG_VERBOSE("AooClient: peer " << group << "|" << user << " left");
}

void Client::handle_peer_changed(const osc::ReceivedMessage& msg) {
    auto it = msg.ArgumentsBegin();
    auto group = (it++)->AsInt32();
    auto user = (it++)->AsInt32();
    auto md = osc_read_metadata(it);

    peer_lock lock(peers_);
    for (auto& peer : peers_) {
        if (peer.match(group, user)) {
            auto e = std::make_unique<peer_update_event>(group, user, md);
            send_event(std::move(e));

            LOG_VERBOSE("AooClient: peer " << peer << " has been updated");

            return;
        }
    }

    LOG_WARNING("AooClient: peer " << group << "|" << user
                << " updated, but not found in list");
}

bool Client::signal() {
    // LOG_DEBUG("aoo_client signal");
    return socket_signal(eventsocket_);
}

void Client::close(bool silent){
    if (socket_ >= 0){
        socket_close(socket_);
        socket_ = -1;
        LOG_VERBOSE("AooClient: closed connection");
    }

    connection_ = nullptr;
    memberships_.clear();

    // remove all peers
    peer_lock lock(peers_);
    peers_.clear();

    // clear pending request
    pending_requests_.clear();

    if (!silent && state_.load() == client_state::connected){
        auto e = std::make_unique<disconnect_event>(0, "no error");
        send_event(std::move(e));
    }
    state_.store(client_state::disconnected);
}

Client::group_membership * Client::find_group_membership(const std::string &name) {
    for (auto& m : memberships_) {
        if (m.group_name == name) {
            return &m;
        }
    }
    return nullptr;
}

Client::group_membership * Client::find_group_membership(AooId id) {
    for (auto& m : memberships_) {
        if (m.group_id == id) {
            return &m;
        }
    }
    return nullptr;
}


void Client::on_socket_error(int err){
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

void Client::on_exception(const char *what, const osc::Exception &err,
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

//---------------------- udp_client ------------------------//

AooError udp_client::setup(int port, AooSocketFlags flags) {
    if (port <= 0) {
        return kAooErrorBadArgument;
    }
    if ((flags & kAooSocketIPv4Mapped) &&
            (!(flags & kAooSocketIPv6) || (flags & kAooSocketIPv4))) {
        LOG_ERROR("AooClient: combination of setup flags not allowed");
        return kAooErrorBadArgument;
    }

    port_ = port;

    if (flags & kAooSocketIPv6) {
        if (flags & kAooSocketIPv4) {
            address_family_ = ip_address::Unspec; // both IPv6 and IPv4
        } else {
            address_family_ = ip_address::IPv6;
        }
    } else {
        address_family_ = ip_address::IPv4;
    }

    use_ipv4_mapped_ = flags & kAooSocketIPv4Mapped;

    return kAooOk;
}

AooError udp_client::handle_bin_message(Client& client, const AooByte *data, int32_t size,
                                        const ip_address& addr, AooMsgType type, int32_t onset) {
    if (type == kAooMsgTypeRelay) {
        ip_address src;
        onset = binmsg_read_relay(data, size, src);
        if (onset > 0) {
        #if AOO_DEBUG_RELAY
            LOG_DEBUG("AooClient: handle binary relay message from " << src << " via " << addr);
        #endif
            auto msg = data + onset;
            auto msgsize = size - onset;
            return client.handleMessage(msg, msgsize, src.address(), src.length());
        } else {
            LOG_ERROR("AooClient: bad binary relay message");
            return kAooErrorBadFormat;
        }
    } else if (type == kAooMsgTypePeer) {
        // peer message
        //
        // NOTE: during the handshake process it is expected that
        // we receive UDP messages which we have to ignore:
        // a) pings from a peer which we haven't had the chance to add yet
        // b) pings sent to alternative endpoint addresses
        if (!client.handle_peer_bin_message(data, size, onset, addr)){
            LOG_VERBOSE("AooClient: ignore UDP binary message from endpoint " << addr);
        }
        return kAooOk;
    } else {
        LOG_WARNING("AooClient: unsupported binary message");
        return kAooErrorBadFormat;
    }
}

AooError udp_client::handle_osc_message(Client& client, const AooByte *data, int32_t size,
                                        const ip_address& addr, AooMsgType type, int32_t onset) {
    try {
        osc::ReceivedPacket packet((const char *)data, size);
        osc::ReceivedMessage msg(packet);

        if (type == kAooMsgTypePeer){
            // peer message
            //
            // NOTE: during the handshake process it is expected that
            // we receive UDP messages which we have to ignore:
            // a) pings from a peer which we haven't had the chance to add yet
            // b) pings sent to alternative endpoint addresses
            if (!client.handle_peer_osc_message(msg, onset, addr)){
                LOG_VERBOSE("AooClient: ignore UDP message " << msg.AddressPattern() + onset
                            << " from endpoint " << addr);
            }
        } else if (type == kAooMsgTypeClient){
            // server message
            if (is_server_address(addr)) {
                handle_server_message(client, msg, onset);
            } else {
                LOG_WARNING("AooClient: got OSC message from unknown server " << addr);
            }
        } else if (type == kAooMsgTypeRelay){
            ip_address src;
            auto packet = unwrap_message(msg, src);
            auto msg = (const AooByte *)packet.Contents();
            auto msgsize = packet.Size();
        #if AOO_DEBUG_RELAY
            LOG_DEBUG("AooClient: handle OSC relay message from " << src << " via " << addr);
        #endif
            return client.handleMessage(msg, msgsize, src.address(), src.length());
        } else {
            LOG_WARNING("AooClient: got unexpected message " << msg.AddressPattern());
            return kAooErrorNotImplemented;
        }

        return kAooOk;
    } catch (const osc::Exception& e){
        LOG_ERROR("AooClient: exception in handle_osc_message: " << e.what());
    #if 0
        on_exception("UDP message", e);
    #endif
        return kAooErrorBadFormat;
    }
}

void udp_client::update(Client& client, const sendfn& fn, time_tag now){
    auto elapsed_time = client.elapsed_time_since(now);
    auto delta = elapsed_time - last_ping_time_;

    auto state = client.current_state();

    if (state == client_state::handshake) {
        // check for time out
        // "first_ping_time_" is guaranteed to be set to 0
        // before the state changes to "handshake"
        auto start = first_ping_time_.load();
        if (start == 0){
            first_ping_time_.store(elapsed_time);
        } else if ((elapsed_time - start) > client.query_timeout()){
            // handshake has timed out!
            auto cmd = std::make_unique<Client::timeout_cmd>();
            client.push_command(std::move(cmd));
            return;
        }
        // send handshakes in fast succession
        if (delta >= client.query_interval()) {
            char buf[64];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            msg << osc::BeginMessage(kAooMsgServerQuery)
                << osc::EndMessage;

            send_server_message(msg, fn);

            last_ping_time_ = elapsed_time;
        }
    } else if (state == client_state::connected) {
        // send regular pings
        if (delta >= client.ping_interval()){
            char buf[64];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            msg << osc::BeginMessage(kAooMsgServerPing)
                << osc::EndMessage;

            send_server_message(msg, fn);

            last_ping_time_ = elapsed_time;
        }
    }

    // send outgoing peer/group messages
    message m;
    while (messages_.try_pop(m)){
        client.perform(m, fn);
    }
}

void udp_client::start_handshake(const ip_address& remote) {
    LOG_DEBUG("AooClient: start UDP handshake with " << remote);
    scoped_lock lock(mutex_);
    first_ping_time_ = 0;
    remote_addr_ = remote;
    public_addr_.clear();
}

void udp_client::queue_message(message&& m) {
    messages_.push(std::move(m));
}

void udp_client::send_server_message(const osc::OutboundPacketStream& msg, const sendfn& fn) {
    sync::shared_lock<sync::shared_mutex> lock(mutex_);
    if (!remote_addr_.valid()) {
        LOG_ERROR("AooClient: no server address");
        return;
    }
    auto addr = remote_addr_;
    lock.unlock();
    // send unlocked
    fn((const AooByte *)msg.Data(), msg.Size(), addr);
}

void udp_client::handle_server_message(Client& client, const osc::ReceivedMessage& msg, int onset) {
    auto pattern = msg.AddressPattern() + onset;
    LOG_DEBUG("AooClient: got server OSC message " << pattern);

    try {
        if (!strcmp(pattern, kAooMsgPong)){
            LOG_DEBUG("AooClient: got UDP pong from server");
        } else if (!strcmp(pattern, kAooMsgQuery)){
            handle_query(client, msg);
        } else {
            LOG_WARNING("AooClient: received unexpected UDP message "
                        << pattern << " from server");
        }
    } catch (const osc::Exception& e) {
        LOG_ERROR("AooClient: exception on handling " << pattern
                  << " message: " << e.what());
    #if 0
        on_exception("server UDP message", e, pattern);
    #endif
    }
}

void udp_client::handle_query(Client &client, const osc::ReceivedMessage &msg) {
    if (client.current_state() == client_state::handshake){
        auto it = msg.ArgumentsBegin();

        // read public address (make sure it is really unmapped)
        ip_address public_addr = osc_read_address(it).unmapped();

        {
            scoped_lock lock(mutex_);
            if (public_addr_ == public_addr) {
                LOG_DEBUG("AooClient: public address " << public_addr
                          << " already received");
                return; // already received
            }
            public_addr_ = public_addr;
        }
        LOG_DEBUG("AooClient: public address: " << public_addr);

        // now we can try to login
        auto cmd = std::make_unique<Client::login_cmd>(public_addr);
        client.push_command(std::move(cmd));
    }
}

bool udp_client::is_server_address(const ip_address& addr){
    scoped_shared_lock lock(mutex_);
    return addr == remote_addr_;
}

} // net
} // aoo
