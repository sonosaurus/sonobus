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

//--------------------- AooClient -----------------------------//

AOO_API AooClient * AOO_CALL AooClient_new(
        AooSocket udpSocket, AooFlag flags, AooError *err) {
    aoo::ip_address address;
    if (aoo::socket_address(udpSocket, address) != 0) {
        if (err) {
            *err = kAooErrorUnknown;
        }
        return nullptr;
    }
    return aoo::construct<aoo::net::Client>(udpSocket, address, flags, err);
}

aoo::net::Client::Client(int socket, const ip_address& address,
                         AooFlag flags, AooError *err)
    : udp_client_(socket, address.port(), address.type())
{
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

AOO_API AooError AOO_CALL AooClient_run(AooClient *client, AooBool nonBlocking){
    return client->run(nonBlocking);
}

AooError AOO_CALL aoo::net::Client::run(AooBool nonBlocking){
    start_time_ = time_tag::now();

    while (!quit_.load()){
        double timeout = 0;

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
        } else {
            timeout = -1;
        }

        if (!wait_for_event(nonBlocking ? 0 : timeout)){
            break;
        }

        // handle commands
        std::unique_ptr<icommand> cmd;
        while (commands_.try_pop(cmd)){
            cmd->perform(*this);
        }

        if (!peers_.try_free()){
            LOG_DEBUG("AooClient: try_free() would block");
        }
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
            return kAooErrorUnknown;
        } else if (s.id == id){
            LOG_WARNING("AooClient: source with id " << id
                        << " already added!");
            return kAooErrorUnknown;
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
    return kAooErrorUnknown;
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
            return kAooOk;
        } else if (s.id == id){
            LOG_WARNING("AooClient: sink with id " << id
                        << " already added!");
            return kAooErrorUnknown;
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
    return kAooErrorUnknown;
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
        AooClient *client, AooId group, const AooData *metadata,
        AooResponseHandler cb, void *context) {
    if (!metadata) {
        return kAooErrorBadArgument;
    }
    return client->updateGroup(group, *metadata, cb, context);
}

AooError AOO_CALL aoo::net::Client::updateGroup(
        AooId group, const AooData& metadata,
        AooResponseHandler cb, void *context) {
    auto cmd = std::make_unique<group_update_cmd>(group, metadata, cb, context);
    push_command(std::move(cmd));
    return kAooOk;
}

AOO_API AooError AOO_CALL AooClient_updateUser(
        AooClient *client, AooId group, AooId user, const AooData *metadata,
        AooResponseHandler cb, void *context) {
    if (!metadata) {
        return kAooErrorBadArgument;
    }
    return client->updateUser(group, user, *metadata, cb, context);
}

AooError AOO_CALL aoo::net::Client::updateUser(
        AooId group, AooId user, const AooData& metadata,
        AooResponseHandler cb, void *context) {
    auto cmd = std::make_unique<user_update_cmd>(group, user, metadata, cb, context);
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
                // we may only access the address if the peer is connected!
                if (p.connected()) {
                    if (*addrlen >= p.address().length()) {
                        memcpy(address, p.address().address(), p.address().length());
                        *addrlen = p.address().length();
                    } else {
                        return kAooErrorInsufficientBuffer;
                    }
                } else {
                    *addrlen = 0;
                }
            }
            return kAooOk;
        }
    }
    return kAooErrorUnknown;
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
    return kAooErrorUnknown;
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
                    memcpy(groupNameBuffer, p.user_name().c_str(), len);
                    *userNameSize = len;
                } else {
                    return kAooErrorInsufficientBuffer;
                }
            }
            return kAooOk;
        }
    }
    return kAooErrorUnknown;
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
        return kAooErrorUnknown;
    }

    if (type == kAooTypeSource){
        // forward to matching source
        for (auto& s : sources_){
            if (s.id == id){
                return s.source->handleMessage(data, size, addr, len);
            }
        }
        LOG_WARNING("AooClient: handle_message(): source not found");
    } else if (type == kAooTypeSink){
        // forward to matching sink
        for (auto& s : sinks_){
            if (s.id == id){
                return s.sink->handleMessage(data, size, addr, len);
            }
        }
        LOG_WARNING("AooClient: handle_message(): sink not found");
    } else {
        // forward to UDP client
        ip_address address((const sockaddr *)addr, len);
        if (binmsg_check(data, size)) {
            return udp_client_.handle_bin_message(*this, data, size, address, type, onset);
        } else {
            return udp_client_.handle_osc_message(*this, data, size, address, type, onset);
        }
    }

    return kAooErrorUnknown;
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
    auto drop = sim_packet_drop_.load();
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
    return kAooErrorUnknown;
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
    case kAooCtlNeedRelay:
    {
        CHECKARG(AooBool);
        auto ep = reinterpret_cast<const AooEndpoint *>(index);
        ip_address addr((sockaddr *)ep->address, ep->addrlen);
        peer_lock lock(peers_);
        for (auto& peer : peers_){
            if (peer.match(addr)){
                as<AooBool>(ptr) = peer.relay() ? kAooTrue : kAooFalse;
                return kAooOk;
            }
        }
        return kAooErrorUnknown;
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
        return kAooErrorUnknown;
    }
#if AOO_CLIENT_SIMULATE
    case kAooCtlSetSimulatePacketReorder:
        CHECKARG(AooSeconds);
        sim_packet_reorder_.store(as<AooSeconds>(ptr));
        break;
    case kAooCtlSetSimulatePacketDrop:
        CHECKARG(float);
        sim_packet_drop_.store(as<float>(ptr));
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
        // forward to matching peer
        peer_lock lock(peers_);
        for (auto& p : peers_) {
            if (p.match(group, user)) {
                p.handle_osc_message(*this, pattern, it, addr);
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
        const char *msg = (state == client_state::connected) ?
            "already connected" : "already connecting";

        cmd.reply_error(kAooErrorUnknown, msg, 0);

        return;
    }

    auto result = ip_address::resolve(cmd.host_.name, cmd.host_.port, udp_client_.type());
    if (result.empty()){
        int err = socket_errno();
        auto msg = socket_strerror(err);
        // LATER think about best way for error handling. Maybe exception?
        LOG_ERROR("AooClient: couldn't resolve hostname: " << msg);

        cmd.reply_error(kAooErrorUnknown, msg.c_str(), err);

        return;
    }

    assert(connection_ == nullptr);
    assert(memberships_.empty());
    connection_ = std::make_unique<connect_cmd>(cmd);

    LOG_DEBUG("AooClient: server address list:");
    for (auto& addr : result){
        LOG_DEBUG("\t" << addr);
    }

    state_.store(client_state::handshake);

    udp_client_.start_handshake(std::move(result));
}

int Client::try_connect(const ip_address& remote){
    socket_ = socket_tcp(0);
    if (socket_ < 0){
        int err = socket_errno();
        LOG_ERROR("AooClient: couldn't create socket: " << socket_strerror(err));
        return err;
    }

    LOG_VERBOSE("AooClient: try to connect to " << remote);

    // try to connect (LATER make timeout configurable)
    if (socket_connect(socket_, remote, 5.0) < 0) {
        int err = socket_errno();
        LOG_ERROR("AooClient: couldn't connect to " << remote << ": "
                  << socket_strerror(err));
        return err;
    }

    LOG_VERBOSE("AooClient: successfully connected to " << remote);

    return 0;
}

void Client::perform(const login_cmd& cmd) {
    assert(connection_ != nullptr);
    assert(memberships_.empty());

    state_.store(client_state::connecting);

    // for actual TCP connection, just pick the first result
    int err = try_connect(cmd.server_ip_.front());
    if (err != 0){
        // cache
        auto msg = socket_strerror(err);
        auto connection = std::move(connection_);

        close();

        connection->reply_error(kAooErrorUnknown, msg.c_str(), err);

        return;
    }
    // get local network interface
    ip_address temp;
    if (socket_address(socket_, temp) < 0){
        int err = socket_errno();
        auto msg = socket_strerror(err);
        auto connection = std::move(connection_);
        LOG_ERROR("AooClient: couldn't get socket name: " << msg);

        close();

        connection->reply_error(kAooErrorUnknown, msg.c_str(), err);

        return;
    }
    local_addr_ = ip_address(temp.name(), udp_client_.port(), udp_client_.type());
    LOG_VERBOSE("AooClient: local address: " << local_addr_);

    // send login request
    auto token = next_token_++;
    auto count = cmd.public_ip_.size() + 1;

    auto msg = start_server_message(connection_->metadata_.size());

    msg << osc::BeginMessage(kAooMsgServerLogin)
        << token << (int32_t)make_version()
        << encrypt(connection_->pwd_).c_str()
        << connection_->metadata_
    // addresses
        << (int32_t)count
        << local_addr_;
    for (auto& addr : cmd.public_ip_){
        msg << addr;
    }
    msg << osc::EndMessage;

    send_server_message(msg);
}

void Client::perform(const timeout_cmd& cmd) {
    if (connection_ && state_.load() == client_state::handshake) {
        auto connection = std::move(connection_); // cache!

        close();

        connection->reply_error(kAooErrorUnknown, "UDP handshake time out", 0);
    }
}

//------------------ disconnect -----------------------//

void Client::perform(const disconnect_cmd& cmd) {
    auto state = state_.load();
    if (state != client_state::connected) {
        const char *msg = (state == client_state::disconnected) ?
                "not connected" : "still connecting";

        cmd.reply_error(kAooErrorUnknown, msg, 0);

        return;
    }

    close(true);

    AooResponseDisconnect response;
    response.type = kAooRequestDisconnect;
    response.flags = 0;

    cmd.reply(kAooOk, (AooResponse&)response); // always succeeds
}

//------------------ group_join -----------------------//

void Client::perform(const group_join_cmd& cmd)
{
    if (state_.load() != client_state::connected) {
        cmd.reply_error(kAooErrorUnknown, "not connected", 0);
        return;
    }
    // check if we're already a group member
    for (auto& m : memberships_) {
        if (m.group_name == cmd.group_name_) {
            cmd.reply_error(kAooErrorUnknown, "already a group member", 0);
            return;
        }
    }
    auto token = next_token_++;
    pending_requests_.emplace(token, std::make_unique<group_join_cmd>(cmd));

    auto msg = start_server_message(cmd.group_md_.size() + cmd.user_md_.size());

    msg << osc::BeginMessage(kAooMsgServerGroupJoin) << token
        << cmd.group_name_.c_str() << encrypt(cmd.group_pwd_).c_str() << cmd.group_md_
        << cmd.user_name_.c_str() << encrypt(cmd.user_pwd_).c_str() << cmd.user_md_
        << cmd.relay_ << osc::EndMessage;

    send_server_message(msg);
}

void Client::handle_response(const group_join_cmd& cmd,
                             const osc::ReceivedMessage& msg) {
    auto it = msg.ArgumentsBegin();
    auto token = (it++)->AsInt32(); // skip
    auto result = (it++)->AsInt32();
    if (result == kAooOk) {
        auto group = (it++)->AsInt32();
        auto user = (it++)->AsInt32();
        auto group_md = osc_read_metadata(it);
        auto user_md = osc_read_metadata(it);
        auto private_md = osc_read_metadata(it);
        AooIpEndpoint relay;
        relay.hostName = (it++)->AsString();
        relay.port = (it++)->AsInt32();

        // add group membership
        if (!find_group_membership(cmd.group_name_)) {
            group_membership m { cmd.group_name_, cmd.user_name_, group, user, {} };
            // add relay servers (in descending priority)
            // 1) our own relay
            if (cmd.relay_.valid()) {
                auto addrlist = ip_address::resolve(cmd.relay_.name, cmd.relay_.port, udp_client_.type());
                m.relay_list.insert(m.relay_list.end(), addrlist.begin(), addrlist.end());
            }
            // 2) server group relay
            if (relay.port > 0) {
                auto addrlist = ip_address::resolve(cmd.relay_.name, cmd.relay_.port, udp_client_.type());
                m.relay_list.insert(m.relay_list.end(), addrlist.begin(), addrlist.end());
            }
            // 3) use UDP server as relay
            if (server_relay_) {
                auto& host = connection_->host_;
                auto addrlist = ip_address::resolve(host.name, host.port, udp_client_.type());
                m.relay_list.insert(m.relay_list.end(), addrlist.begin(), addrlist.end());
            }
            memberships_.push_back(std::move(m));
        } else {
            LOG_ERROR("AooClient: group join response: already a member of group " << cmd.group_name_);
        }

        AooResponseGroupJoin response;
        response.type = kAooRequestGroupJoin;
        response.flags = 0;
        response.groupId = group;
        response.userId = user;
        response.groupMetadata = group_md.size ? &group_md : nullptr;
        response.userMetadata = user_md.size ? &user_md : nullptr;
        response.privateMetadata = private_md.size ? &private_md : nullptr;
        response.relayAddress = relay.port > 0 ? &relay : nullptr;

        cmd.reply(result, (AooResponse&)response);
        LOG_VERBOSE("AooClient: successfully joined group " << cmd.group_name_);
    } else {
        auto code = (it++)->AsInt32();
        auto msg = (it++)->AsString();
        cmd.reply_error(result, msg, code);
        LOG_WARNING("AooClient: couldn't join group "
                    << cmd.group_name_ << ": " << msg);
    }
}

//------------------ group_leave -----------------------//

void Client::perform(const group_leave_cmd& cmd)
{
    // TODO check for group membership?

    auto token = next_token_++;
    pending_requests_.emplace(token, std::make_unique<group_leave_cmd>(cmd));

    auto msg = start_server_message();

    msg << osc::BeginMessage(kAooMsgServerGroupLeave)
        << token << cmd.group_ << osc::EndMessage;

    send_server_message(msg);
}

void Client::handle_response(const group_leave_cmd& cmd,
                             const osc::ReceivedMessage& msg) {
    auto it = msg.ArgumentsBegin();
    auto token = (it++)->AsInt32(); // skip
    auto result = (it++)->AsInt32();
    if (result == kAooOk) {
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
        response.type = kAooRequestGroupLeave;
        response.flags = 0;

        cmd.reply(result, (AooResponse&)response);
        LOG_VERBOSE("AooClient: successfully left group " << cmd.group_);
    } else {
        auto code = (it++)->AsInt32();
        auto msg = (it++)->AsString();
        cmd.reply_error(result, msg, code);
        LOG_WARNING("AooClient: couldn't leave group "
                    << cmd.group_ << ": " << msg);
    }
}

//------------------ group_update -----------------------//

void Client::perform(const group_update_cmd& cmd) {
    // TODO check for group membership?

    auto token = next_token_++;
    pending_requests_.emplace(token, std::make_unique<group_update_cmd>(cmd));

    auto msg = start_server_message(cmd.md_.size());

    msg << osc::BeginMessage(kAooMsgServerGroupUpdate)
        << token << cmd.group_ << cmd.md_ << osc::EndMessage;

    send_server_message(msg);
}

void Client::handle_response(const group_update_cmd& cmd,
                             const osc::ReceivedMessage& msg) {
    auto it = msg.ArgumentsBegin();
    auto token = (it++)->AsInt32(); // skip
    auto result = (it++)->AsInt32();
    if (result == kAooOk) {
        AooResponseGroupUpdate response;
        response.type = kAooRequestGroupUpdate;
        response.flags = 0;
        response.groupMetadata.type = cmd.md_.type();
        response.groupMetadata.data = cmd.md_.data();
        response.groupMetadata.size = cmd.md_.size();

        cmd.reply(result, (AooResponse&)response);
        LOG_VERBOSE("AooClient: successfully updated group " << cmd.group_);
    } else {
        auto code = (it++)->AsInt32();
        auto msg = (it++)->AsString();
        cmd.reply_error(result, msg, code);
        LOG_WARNING("AooClient: could not update group "
                    << cmd.group_ << ": " << msg);
    }
}

//------------------ user_update -----------------------//

void Client::perform(const user_update_cmd& cmd) {
    // TODO check for group membership?

    auto token = next_token_++;
    pending_requests_.emplace(token, std::make_unique<user_update_cmd>(cmd));

    auto msg = start_server_message(cmd.md_.size());

    msg << osc::BeginMessage(kAooMsgServerUserUpdate)
        << token << cmd.group_ << cmd.user_ << cmd.md_ << osc::EndMessage;

    send_server_message(msg);
}

void Client::handle_response(const user_update_cmd& cmd,
                             const osc::ReceivedMessage& msg) {
    auto it = msg.ArgumentsBegin();
    auto token = (it++)->AsInt32(); // skip
    auto result = (it++)->AsInt32();
    if (result == kAooOk) {
        AooResponseUserUpdate response;
        response.type = kAooRequestUserUpdate;
        response.flags = 0;
        response.userMetadata.type = cmd.md_.type();
        response.userMetadata.data = cmd.md_.data();
        response.userMetadata.size = cmd.md_.size();

        cmd.reply(result, (AooResponse&)response);
        LOG_VERBOSE("AooClient: successfully updated user "
                    << cmd.user_ << " in group " << cmd.group_);
    } else {
        auto code = (it++)->AsInt32();
        auto msg = (it++)->AsString();
        cmd.reply_error(result, msg, code);
        LOG_WARNING("AooClient: could not update user " << cmd.user_
                    << " in group " << cmd.group_ << ": " << msg);
    }
}

//------------------ custom_request -----------------------//

void Client::perform(const custom_request_cmd& cmd) {
    if (state_.load() != client_state::connected) {
        cmd.reply_error(kAooErrorUnknown, "not connected", 0);
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
    if (result < 0){
        int err = socket_errno();
        if (err == EINTR){
            return true; // ?
        } else {
            LOG_ERROR("AooClient: poll failed (" << err << ")");
            socket_error_print("poll");
            return false;
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
        if (type == kAooTypeClient){
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
        auto token = (AooId)(it++)->AsInt32(); // skip
        auto result = (AooError)(it++)->AsInt32();

        if (result == kAooOk){
            auto id = (AooId)(it++)->AsInt32();
            auto flags = (AooFlag)(it++)->AsInt32();
            auto metadata = osc_read_metadata(it);

            server_relay_ = flags & kAooServerRelay;

            // connected!
            state_.store(client_state::connected);
            LOG_VERBOSE("AooClient: successfully logged in (client ID: "
                        << id << ")");
            // notify
            AooResponseConnect response;
            response.type = kAooRequestConnect;
            response.flags = 0;
            response.clientId = id;
            response.metadata = metadata.data ? &metadata : nullptr;

            connection_->reply(kAooOk, (AooResponse&)response);
        } else {
            auto code = (it++)->AsInt32();
            auto msg = (it++)->AsString();
            LOG_WARNING("AooClient: login failed: " << msg);

            // cache connection
            auto connection = std::move(connection_);

            close();

            connection->reply_error(kAooErrorUnknown, msg, code);
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

void Client::handle_group_changed(const osc::ReceivedMessage& msg) {
    auto it = msg.ArgumentsBegin();
    auto group = (it++)->AsInt32();
    auto md = osc_read_metadata(it);

    auto e = std::make_unique<group_update_event>(group, md);
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

static osc::ReceivedPacket unwrap_message(const osc::ReceivedMessage& msg,
    ip_address& addr, ip_address::ip_type type)
{
    auto it = msg.ArgumentsBegin();

    addr = osc_read_address(it, type);

    const void *blobData;
    osc::osc_bundle_element_size_t blobSize;
    (it++)->AsBlob(blobData, blobSize);

    return osc::ReceivedPacket((const char *)blobData, blobSize);
}

void Client::handle_peer_add(const osc::ReceivedMessage& msg){
    auto it = msg.ArgumentsBegin();
    auto group_name = (it++)->AsString();
    auto group_id = (it++)->AsInt32();
    auto user_name = (it++)->AsString();
    auto user_id = (it++)->AsInt32();
    auto metadata = osc_read_metadata(it);
    // collect IP addresses
    auto count = (it++)->AsInt32();
    ip_address_list addrlist;
    while (count--){
        // force IP protocol!
        auto addr = osc_read_address(it, udp_client_.type());
        // NB: filter local addresses so that we don't accidentally ping ourselves!
        if (addr.valid() && addr != local_addr_) {
            addrlist.push_back(addr);
        }
    }
    auto relay = osc_read_host(it);

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
    // get relay address(es)
    ip_address_list relaylist;
    if (relay.valid()) {
        relaylist = aoo::ip_address::resolve(relay.name, relay.port, udp_client_.type());
        // add to group relay list
        auto& list = membership->relay_list;
        list.insert(list.end(), relaylist.begin(), relaylist.end());
    }

    auto peer = peers_.emplace_front(group_name, group_id, user_name, user_id,
                                     membership->user_id, std::move(addrlist),
                                     metadata.size > 0 ? &metadata : nullptr,
                                     std::move(relaylist), membership->relay_list);

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
            if (p.match(addr)) {
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

void Client::close(bool manual){
    if (socket_ >= 0){
        socket_close(socket_);
        socket_ = -1;
        LOG_VERBOSE("AooClient: connection closed");
    }

    connection_ = nullptr;
    memberships_.clear();

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

AooError udp_client::handle_bin_message(Client& client, const AooByte *data, int32_t size,
                                        const ip_address& addr, AooMsgType type, int32_t onset) {
    if (type == kAooTypeRelay) {
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
            return kAooErrorUnknown;
        }
    } else if (type == kAooTypePeer) {
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
        return kAooErrorUnknown;
    }
}

AooError udp_client::handle_osc_message(Client& client, const AooByte *data, int32_t size,
                                        const ip_address& addr, AooMsgType type, int32_t onset) {
    try {
        osc::ReceivedPacket packet((const char *)data, size);
        osc::ReceivedMessage msg(packet);

        if (type == kAooTypePeer){
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
        } else if (type == kAooTypeClient){
            // server message
            if (is_server_address(addr)) {
                handle_server_message(client, msg, onset);
            } else {
                LOG_WARNING("AooClient: got OSC message from unknown server " << addr);
            }
        } else if (type == kAooTypeRelay){
            ip_address src;
            auto packet = unwrap_message(msg, src, type_);
            auto msg = (const AooByte *)packet.Contents();
            auto msgsize = packet.Size();
        #if AOO_DEBUG_RELAY
            LOG_DEBUG("AooClient: handle OSC relay message from " << src << " via " << addr);
        #endif
            return client.handleMessage(msg, msgsize, src.address(), src.length());
        } else {
            LOG_WARNING("AooClient: got unexpected message " << msg.AddressPattern());
            return kAooErrorUnknown;
        }

        return kAooOk;
    } catch (const osc::Exception& e){
        LOG_ERROR("AooClient: exception in handle_osc_message: " << e.what());
    #if 0
        on_exception("UDP message", e);
    #endif
        return kAooErrorUnknown;
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
            msg << osc::BeginMessage(kAooMsgServerQuery) << osc::EndMessage;

            scoped_shared_lock lock(mutex_);
            for (auto& addr : server_addrlist_){
                fn((const AooByte *)msg.Data(), msg.Size(), addr);
            }
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

void udp_client::start_handshake(ip_address_list&& remote) {
    scoped_lock lock(mutex_);
    first_ping_time_ = 0;
    server_addrlist_ = std::move(remote);
    tcp_addrlist_.clear();
    public_addrlist_.clear();
}

void udp_client::queue_message(message&& m) {
    messages_.push(std::move(m));
}

void udp_client::send_server_message(const osc::OutboundPacketStream& msg, const sendfn& fn) {
    sync::shared_lock<sync::shared_mutex> lock(mutex_);
    if (server_addrlist_.empty()) {
        LOG_ERROR("AooClient: no server address");
        return;
    }
    // just pick any address
    ip_address addr(server_addrlist_.front());
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
            if (client.current_state() == client_state::handshake){
                auto it = msg.ArgumentsBegin();

                // public IP + port
                std::string ip = (it++)->AsString();
                int port = (it++)->AsInt32();
                ip_address public_addr(ip, port, type());

                // TCP server IP + port
                ip = (it++)->AsString();
                port = (it++)->AsInt32();
                ip_address tcp_addr;
                if (!ip.empty()) {
                    tcp_addr = ip_address(ip, port, type());
                } else {
                    // use UDP server address
                    // TODO later always require the server to send the address
                    scoped_lock lock(mutex_);
                    if (!server_addrlist_.empty()) {
                        tcp_addr = server_addrlist_.front();
                    } else {
                        LOG_ERROR("AooClient: no UDP server address");
                        return;
                    }
                }

                scoped_lock lock(mutex_);
                for (auto& addr : public_addrlist_) {
                    if (addr == tcp_addr) {
                        LOG_DEBUG("AooClient: public address " << addr
                                  << " already received");
                        return; // already received
                    }
                }
                public_addrlist_.push_back(public_addr);
                tcp_addrlist_.push_back(tcp_addr);
                LOG_VERBOSE("AooClient: public address: " << public_addr
                            << ", TCP server address: " << tcp_addr);

                // check if we got all public addresses
                // LATER improve this
                if (tcp_addrlist_.size() == server_addrlist_.size()) {
                    // now we can try to login
                    auto cmd = std::make_unique<Client::login_cmd>(
                                std::move(tcp_addrlist_),
                                std::move(public_addrlist_));

                    client.push_command(std::move(cmd));
                }
            }
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

} // net
} // aoo
