/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "client.hpp"

#include "aoo/aoo_source.hpp"
#include "aoo/aoo_sink.hpp"

#include <cstring>
#include <functional>
#include <algorithm>
#include <sstream>

#if AOO_NET_CLIENT_SIMULATE
#include <random>
#endif

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

// debugging

#define FORCE_RELAY 0

// OSC patterns

#define kAooNetMsgPingReply \
    kAooNetMsgPing kAooNetMsgReply

#define kAooNetMsgServerPing \
    kAooMsgDomain kAooNetMsgServer kAooNetMsgPing

#define kAooNetMsgClientPingReply \
    kAooMsgDomain kAooNetMsgClient kAooNetMsgPingReply

#define kAooNetMsgPeerPing \
    kAooMsgDomain kAooNetMsgPeer kAooNetMsgPing

#define kAooNetMsgPeerPingReply \
    kAooMsgDomain kAooNetMsgPeer kAooNetMsgPingReply

#define kAooNetMsgPeerMessage \
    kAooMsgDomain kAooNetMsgPeer kAooNetMsgMessage

#define kAooNetMsgPeerAck \
    kAooMsgDomain kAooNetMsgPeer kAooNetMsgAck

#define kAooNetMsgServerLogin \
    kAooMsgDomain kAooNetMsgServer kAooNetMsgLogin

#define kAooNetMsgServerQuery \
    kAooMsgDomain kAooNetMsgServer kAooNetMsgQuery

#define kAooNetMsgClientQuery \
    kAooMsgDomain kAooNetMsgClient kAooNetMsgQuery

#define kAooNetMsgPeerJoin \
    kAooNetMsgPeer kAooNetMsgJoin

#define kAooNetMsgPeerLeave \
    kAooNetMsgPeer kAooNetMsgLeave

#define kAooNetMsgGroupJoin \
    kAooNetMsgGroup kAooNetMsgJoin

#define kAooNetMsgGroupLeave \
    kAooNetMsgGroup kAooNetMsgLeave

#define kAooNetMsgGroupUpdate \
    kAooNetMsgGroup kAooNetMsgUpdate

#define kAooNetMsgUserUpdate \
    kAooNetMsgUser kAooNetMsgUpdate

#define kAooNetMsgGroupChanged \
    kAooNetMsgGroup kAooNetMsgChanged

#define kAooNetMsgUserChanged \
    kAooNetMsgUser kAooNetMsgChanged

#define kAooNetMsgPeerChanged \
    kAooNetMsgPeer kAooNetMsgChanged

#define kAooNetMsgServerGroupJoin \
    kAooMsgDomain kAooNetMsgServer kAooNetMsgGroupJoin

#define kAooNetMsgServerGroupLeave \
    kAooMsgDomain kAooNetMsgServer kAooNetMsgGroupLeave

#define kAooNetMsgServerGroupUpdate \
    kAooMsgDomain kAooNetMsgServer kAooNetMsgGroupUpdate

#define kAooNetMsgServerUserUpdate \
    kAooMsgDomain kAooNetMsgServer kAooNetMsgUserUpdate

#define kAooNetMsgServerRequest \
    kAooMsgDomain kAooNetMsgServer kAooNetMsgRequest

namespace aoo {

// OSC peer message:
const int32_t kMessageMaxAddrSize = kAooMsgDomainLen + kAooNetMsgPeerLen + 16 + kAooMsgDataLen;
// address pattern string: max 16 bytes
// typetag string: max. 12 bytes
// args (without type and blob data): max. 36 bytes
const int32_t kMessageHeaderSize = kMessageMaxAddrSize + 48 + kAooDataTypeMaxLen + 1;

// binary peer message:
// args (without type and blob data): 24 bytes (max.)
const int32_t kBinMessageHeaderSize = kAooBinMsgLargeHeaderSize + 24 + kAooDataTypeMaxLen + 1;

} // aoo

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

                    msg << osc::BeginMessage(kAooNetMsgServerPing)
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
        const AooDataView *metadata, AooNetCallback cb, void *context) {
    return client->connect(hostName, port, password, metadata, cb, context);
}

AooError AOO_CALL aoo::net::Client::connect(
        const AooChar *hostName, AooInt32 port, const AooChar *password,
        const AooDataView *metadata, AooNetCallback cb, void *context) {
    auto cmd = std::make_unique<connect_cmd>(hostName, port, password, metadata, cb, context);
    push_command(std::move(cmd));
    return kAooOk;
}

AOO_API AooError AOO_CALL AooClient_disconnect(
        AooClient *client, AooNetCallback cb, void *context) {
    return client->disconnect(cb, context);
}

AooError AOO_CALL aoo::net::Client::disconnect(AooNetCallback cb, void *context) {
    auto cmd = std::make_unique<disconnect_cmd>(cb, context);
    push_command(std::move(cmd));
    return kAooOk;
}

AOO_API AooError AOO_CALL AooClient_joinGroup(
        AooClient *client,
        const AooChar *groupName, const AooChar *groupPwd, const AooDataView *groupMetadata,
        const AooChar *userName, const AooChar *userPwd, const AooDataView *userMetadata,
        const AooIpEndpoint *relayAddress, AooNetCallback cb, void *context) {
    return client->joinGroup(groupName, groupPwd, groupMetadata, userName, userPwd,
                             userMetadata, relayAddress, cb, context);
}

AooError AOO_CALL aoo::net::Client::joinGroup(
        const AooChar *groupName, const AooChar *groupPwd, const AooDataView *groupMetadata,
        const AooChar *userName, const AooChar *userPwd, const AooDataView *userMetadata,
        const AooIpEndpoint *relayAddress, AooNetCallback cb, void *context) {
    auto cmd = std::make_unique<group_join_cmd>(groupName, groupPwd, groupMetadata,
                                                userName, userPwd, userMetadata,
                                                (relayAddress ? ip_host(*relayAddress) : ip_host{}),
                                                cb, context);
    push_command(std::move(cmd));
    return kAooOk;
}

AOO_API AooError AOO_CALL AooClient_leaveGroup(
        AooClient *client, AooId group, AooNetCallback cb, void *context) {
    return client->leaveGroup(group, cb, context);
}

AooError AOO_CALL aoo::net::Client::leaveGroup(
        AooId group, AooNetCallback cb, void *context) {
    auto cmd = std::make_unique<group_leave_cmd>(group, cb, context);
    push_command(std::move(cmd));
    return kAooOk;
}

AOO_API AooError AOO_CALL AooClient_updateGroup(
        AooClient *client, AooId group, const AooDataView *metadata,
        AooNetCallback cb, void *context) {
    if (!metadata) {
        return kAooErrorBadArgument;
    }
    return client->updateGroup(group, *metadata, cb, context);
}

AooError AOO_CALL aoo::net::Client::updateGroup(
        AooId group, const AooDataView& metadata,
        AooNetCallback cb, void *context) {
    auto cmd = std::make_unique<group_update_cmd>(group, metadata, cb, context);
    push_command(std::move(cmd));
    return kAooOk;
}

AOO_API AooError AOO_CALL AooClient_updateUser(
        AooClient *client, AooId group, AooId user, const AooDataView *metadata,
        AooNetCallback cb, void *context) {
    if (!metadata) {
        return kAooErrorBadArgument;
    }
    return client->updateUser(group, user, *metadata, cb, context);
}

AooError AOO_CALL aoo::net::Client::updateUser(
        AooId group, AooId user, const AooDataView& metadata,
        AooNetCallback cb, void *context) {
    auto cmd = std::make_unique<user_update_cmd>(group, user, metadata, cb, context);
    push_command(std::move(cmd));
    return kAooOk;
}

AOO_API AooError AOO_CALL AooClient_customRequest(
        AooClient *client, const AooDataView *data, AooFlag flags,
        AooNetCallback cb, void *context) {
    if (!data) {
        return kAooErrorBadArgument;
    }
    return client->customRequest(*data, flags, cb, context);
}

AooError AOO_CALL aoo::net::Client::customRequest(
        const AooDataView& data, AooFlag flags, AooNetCallback cb, void *context) {
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
        const AooDataView *message, AooNtpTime timeStamp, AooFlag flags)
{
    if (!message) {
        return kAooErrorBadArgument;
    }
    return client->sendMessage(group, user, *message, timeStamp, flags);
}

AooError AOO_CALL aoo::net::Client::sendMessage(
        AooId group, AooId user, const AooDataView& msg,
        AooNtpTime timeStamp, AooFlag flags)
{
    // TODO implement ack mechanism over UDP.
    bool reliable = flags & kAooNetMessageReliable;
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

#if AOO_NET_CLIENT_SIMULATE
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

    if (drop > 0 || reorder > 0 || jitter) {
        // wrap send function
        struct wrap_state {
            Client *client;
            sendfn reply;
            time_tag now;
            float drop;
            float reorder;
            bool jitter;
        } state;
        state.client = this;
        state.reply = reply;
        state.now = now;
        state.drop = drop;
        state.reorder = reorder;
        state.jitter = jitter;

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

        // replace
        reply = sendfn(wrapfn, &state);
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
        AooClient *client, const AooNetRequest *request,
        AooNetCallback callback, void *user, AooFlag flags) {
    if (!request) {
        return kAooErrorBadArgument;
    }
    return client->sendRequest(*request, callback, user, flags);
}

AooError AOO_CALL aoo::net::Client::sendRequest(
        const AooNetRequest& request, AooNetCallback callback, void *user, AooFlag flags)
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
#if AOO_NET_CLIENT_SIMULATE
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

    msg << osc::BeginMessage(kAooNetMsgServerLogin)
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

    cmd.reply(kAooOk, nullptr); // always succeeds
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

    msg << osc::BeginMessage(kAooNetMsgServerGroupJoin) << token
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

        AooNetResponseGroupJoin response;
        response.type = kAooNetRequestGroupJoin;
        response.flags = 0;
        response.groupId = group;
        response.userId = user;
        response.groupMetadata = group_md.size ? &group_md : nullptr;
        response.userMetadata = user_md.size ? &user_md : nullptr;
        response.privateMetadata = private_md.size ? &private_md : nullptr;
        response.relayAddress = relay.port > 0 ? &relay : nullptr;

        cmd.reply(result, (AooNetResponse *)&response);
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

    msg << osc::BeginMessage(kAooNetMsgServerGroupLeave)
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

        cmd.reply(result, nullptr);
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

    msg << osc::BeginMessage(kAooNetMsgServerGroupUpdate)
        << token << cmd.group_ << cmd.md_ << osc::EndMessage;

    send_server_message(msg);
}

void Client::handle_response(const group_update_cmd& cmd,
                             const osc::ReceivedMessage& msg) {
    auto it = msg.ArgumentsBegin();
    auto token = (it++)->AsInt32(); // skip
    auto result = (it++)->AsInt32();
    if (result == kAooOk) {
        AooNetResponseGroupUpdate response;
        response.type = kAooNetRequestGroupUpdate;
        response.flags = 0;
        response.groupMetadata.type = cmd.md_.type();
        response.groupMetadata.data = cmd.md_.data();
        response.groupMetadata.size = cmd.md_.size();

        cmd.reply(result, (AooNetResponse *)&response);
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

    msg << osc::BeginMessage(kAooNetMsgServerUserUpdate)
        << token << cmd.group_ << cmd.user_ << cmd.md_ << osc::EndMessage;

    send_server_message(msg);
}

void Client::handle_response(const user_update_cmd& cmd,
                             const osc::ReceivedMessage& msg) {
    auto it = msg.ArgumentsBegin();
    auto token = (it++)->AsInt32(); // skip
    auto result = (it++)->AsInt32();
    if (result == kAooOk) {
        AooNetResponseUserUpdate response;
        response.type = kAooNetRequestUserUpdate;
        response.flags = 0;
        response.userMetadata.type = cmd.md_.type();
        response.userMetadata.data = cmd.md_.data();
        response.userMetadata.size = cmd.md_.size();

        cmd.reply(result, (AooNetResponse *)&response);
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

    msg << osc::BeginMessage(kAooNetMsgServerRequest)
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

            if (!strcmp(pattern, kAooNetMsgPingReply)){
                LOG_DEBUG("AooClient: got TCP ping reply from server");
            } else if (!strcmp(pattern, kAooNetMsgPeerJoin)) {
                handle_peer_add(msg);
            } else if (!strcmp(pattern, kAooNetMsgPeerLeave)) {
                handle_peer_remove(msg);
            } else if (!strcmp(pattern, kAooNetMsgPeerChanged)) {
                handle_peer_changed(msg);
            } else if (!strcmp(pattern, kAooNetMsgLogin)) {
                handle_login(msg);
            } else if (!strcmp(pattern, kAooNetMsgMessage)) {
                handle_server_notification(msg);
            } else if (!strcmp(pattern, kAooNetMsgGroupChanged)) {
                handle_group_changed(msg);
            } else if (!strcmp(pattern, kAooNetMsgUserChanged)) {
                handle_user_changed(msg);
            } else if (!strcmp(pattern, kAooNetMsgGroupJoin) ||
                       !strcmp(pattern, kAooNetMsgGroupLeave) ||
                       !strcmp(pattern, kAooNetMsgGroupUpdate) ||
                       !strcmp(pattern, kAooNetMsgUserUpdate) ||
                       !strcmp(pattern, kAooNetMsgRequest)) {
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

            server_relay_ = flags & kAooNetServerRelay;

            // connected!
            state_.store(client_state::connected);
            LOG_VERBOSE("AooClient: successfully logged in (client ID: "
                        << id << ")");
            // notify
            AooNetResponseConnect response;
            response.type = kAooNetRequestConnect;
            response.clientId = id;
            response.flags = 0;
            response.metadata = metadata.data ? &metadata : nullptr;

            connection_->reply(kAooOk, (AooNetResponse*)&response);
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

    auto e = std::make_unique<peer_event>(kAooNetEventPeerHandshake, *peer);
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
    // that an kAooNetEventPeerJoin event has been sent.
    if (peer->connected()){
        auto e = std::make_unique<peer_event>(kAooNetEventPeerLeave, *peer);
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
                auto e = std::make_unique<net_error_event>(0, ss.str());
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
        auto e = std::make_unique<base_event>(kAooNetEventClientDisconnect);
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
    auto e = std::make_unique<net_error_event>(err, msg);

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

    auto e = std::make_unique<net_error_event>(0, msg);

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
            msg << osc::BeginMessage(kAooNetMsgServerQuery) << osc::EndMessage;

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
            msg << osc::BeginMessage(kAooNetMsgServerPing)
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
        if (!strcmp(pattern, kAooNetMsgPingReply)){
            LOG_DEBUG("AooClient: got UDP ping from server");
        } else if (!strcmp(pattern, kAooNetMsgQuery)){
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

//------------------------- peer ------------------------------//

peer::peer(const std::string& groupname, AooId groupid,
           const std::string& username, AooId userid, AooId localid,
           ip_address_list&& addrlist, const AooDataView *metadata,
           ip_address_list&& user_relay, const ip_address_list& group_relay)
    : group_name_(groupname), user_name_(username), group_id_(groupid), user_id_(userid),
      local_id_(localid), metadata_(metadata), addrlist_(std::move(addrlist)),
      user_relay_(std::move(user_relay)), group_relay_(group_relay)
{
    start_time_ = time_tag::now();

    LOG_DEBUG("AooClient: create peer " << *this);
}

peer::~peer(){
    LOG_DEBUG("AooClient: destroy peer " << *this);
}

bool peer::match(const ip_address& addr) const {
    if (connected()){
        return real_address_ == addr;
    } else {
        return false;
    }
}

bool peer::match(const std::string& group) const {
    return group_name_ == group; // immutable!
}

bool peer::match(const std::string& group, const std::string& user) const {
    return group_name_ == group && user_name_ == user; // immutable!
}

bool peer::match(AooId group) const {
    return group_id_ == group; // immutable!
}

bool peer::match(AooId group, AooId user) const {
    return group_id_ == group && user_id_ == user;
}

bool peer::match_wildcard(AooId group, AooId user) const {
    return (group_id_ == group || group == kAooIdInvalid) &&
            (user_id_ == user || user == kAooIdInvalid);
}

void peer::send(Client& client, const sendfn& fn, time_tag now) {
    if (connected()) {
        do_send(client, fn, now);
    } else if (!timeout_) {
        // try to establish UDP connection with peer
        auto elapsed_time = time_tag::duration(start_time_, now);
        auto delta = elapsed_time - last_pingtime_;
        if (elapsed_time > client.query_timeout()){
            // time out -> try to relay
            if (!group_relay_.empty() && !relay()) {
                start_time_ = now;
                last_pingtime_ = 0;
                // for now we just try with the first relay address.
                // LATER try all of them.
                relay_address_ = group_relay_.front();
                relay_ = true;
                LOG_WARNING("AooClient: UDP handshake with " << *this
                            << " timed out, try to relay over " << relay_address_);
                return;
            }

            // couldn't establish connection!
            const char *what = relay() ? "relay" : "peer-to-peer";
            LOG_ERROR("AooClient: couldn't establish UDP " << what
                      << " connection to " << *this << "; timed out after "
                      << client.query_timeout() << " seconds");

            std::stringstream ss;
            ss << "couldn't establish connection with peer " << *this;

            // TODO: do we really need to send the error event?
            auto e1 = std::make_unique<net_error_event>(0, ss.str());
            client.send_event(std::move(e1));

            auto e2 = std::make_unique<peer_event>(
                        kAooNetEventPeerTimeout, *this);
            client.send_event(std::move(e2));

            timeout_ = true;

            return;
        }
        // send handshakes in fast succession to all addresses
        // until we get a reply from one of them (see handle_message())
        if (delta >= client.query_interval()){
            char buf[64];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            // /aoo/peer/ping <group> <user> <tt>
            // NB: we send *our* user ID, see above. The empty timetag
            // distinguishes a handshake ping from a regular ping!
            //
            // With the group and user ID, peers can identify us even if
            // we're behind a symmetric NAT. This trick doesn't work
            // if both parties are behind a symmetrict NAT; in that case,
            // UDP hole punching simply doesn't work.
            msg << osc::BeginMessage(kAooNetMsgPeerPing)
                << group_id_ << local_id_ << osc::TimeTag(0)
                << osc::EndMessage;

            for (auto& addr : addrlist_) {
                send(msg, addr, fn);
            }

            last_pingtime_ = elapsed_time;

            LOG_DEBUG("AooClient: send handshake ping to " << *this);
        }
    }
}

void peer::do_send(Client& client, const sendfn& fn, time_tag now) {
    auto elapsed_time = time_tag::duration(start_time_, now);
    auto delta = elapsed_time - last_pingtime_;
    // 1) send regular ping
    if (delta >= client.ping_interval()) {
        // /aoo/peer/ping
        // NB: we send *our* user ID, so that the receiver can easily match the message.
        // We do not have to specifiy the peer's user ID because the group Id is already
        // sufficient. (There can only be one user per client in a group.)
        char buf[64];
        osc::OutboundPacketStream msg(buf, sizeof(buf));
        msg << osc::BeginMessage(kAooNetMsgPeerPing)
            << group_id_ << local_id_ << osc::TimeTag(now)
            << osc::EndMessage;

        send(msg, fn);

        last_pingtime_ = elapsed_time;

        LOG_DEBUG("AooClient: send ping to " << *this << " (" << now << ")");
    }
    // 2) reply to /ping message
    // NB: we send *our* user ID, see above.
    if (got_ping_.exchange(false, std::memory_order_acquire)) {
        // The empty timetag distinguishes a handshake
        // ping (reply) from a regular ping (reply).
        auto tt1 = ping_tt1_;
        if (!tt1.is_empty()) {
            // regular ping reply
            char buf[64];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            msg << osc::BeginMessage(kAooNetMsgPeerPingReply)
                << group_id_ << local_id_
                << osc::TimeTag(tt1) << osc::TimeTag(now)
                << osc::EndMessage;

            send(msg, fn);

            LOG_DEBUG("AooClient: send ping reply to " << *this
                      << " (" << time_tag(tt1) << " " << now << ")");
        } else {
            // handshake ping reply
            char buf[64];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            msg << osc::BeginMessage(kAooNetMsgPeerPingReply)
                << group_id_ << local_id_
                << osc::TimeTag(0) << osc::TimeTag(0)
                << osc::EndMessage;

            send(msg, fn);

            LOG_DEBUG("AooClient: send handshake ping reply to " << *this);
        }
    }
    // 3) send outgoing acks
    // LATER send them in batches!
    message_ack ack;
    while (send_acks_.try_pop(ack)) {
        send_ack(ack, fn);
    }
    // 4) handle incoming acks
    while (received_acks_.try_pop(ack)) {
        if (auto msg = send_buffer_.find(ack.seq)) {
            if (ack.frame >= 0) {
                msg->ack_frame(ack.frame);
            } else {
                // negative -> all frames
                msg->ack_all();
            }
        } else {
        #if AOO_DEBUG_CLIENT_MESSAGE
            LOG_DEBUG("AooClient: got outdated ack (seq: " << ack.seq
                      << ", frame: " << ack.frame << ") from " << *this);
        #endif
        }
    }
    // 5) pop acknowledged messages
    while (!send_buffer_.empty()) {
        auto& msg = send_buffer_.front();
        if (msg.complete()) {
        #if AOO_DEBUG_CLIENT_MESSAGE
            LOG_DEBUG("AooClient: pop acknowledged message (seq: "
                      << msg.sequence_ << ") from " << *this);
        #endif
            send_buffer_.pop();
        } else {
            break;
        }
    }
    // 6) resend messages
    if (!send_buffer_.empty()) {
        bool binary = client.binary();
        for (auto& msg : send_buffer_) {
            if (msg.need_resend(elapsed_time)) {
                message_packet p;
                p.type = msg.data_.type();
                p.data = nullptr;
                p.size = 0;
                p.tt = msg.tt_;
                p.sequence = msg.sequence_;
                p.totalsize = msg.data_.size();
                p.nframes = msg.nframes_;
                p.frame = 0;
                p.reliable = true;
                for (int i = 0; i < p.nframes; ++i) {
                    if (!msg.has_frame(i)) {
                        p.frame = i;
                        msg.get_frame(i, p.data, p.size);
                    #if AOO_DEBUG_CLIENT_MESSAGE
                        LOG_DEBUG("AooClient: resend message (seq: " << msg.sequence_
                                  << ", frame: " << i << ") to " << *this);
                    #endif
                        if (binary) {
                            send_packet_bin(p, fn);
                        } else {
                            send_packet_osc(p, fn);
                        }
                    }
                }
            }
        }
    }
}

// OSC:
// /aoo/peer/msg <group> <user> <flags> <seq> <total> <nframes> <frame> <tt> <type> <data>
//
// binary:
// header (group + user), flags (int16), size (int16), seq,
// [total (int32), nframes (int16), frame (int16)], [tt (uint64)], [type (str)], data (bin)
//
// 'total', 'nframes' and 'frame' are omitted for single-frame messages.
// 'tt' and 'type' are only sent with the first frame. 'tt' might be ommited if zero.

// if 'flags' contains kAooNetMessageReliable, the other end sends an ack messages:
//
// OSC:
// /aoo/peer/ack <count> <seq1> <frame1> <seq2> <frame2> etc. // frame < 0 -> all
//
// binary:
// type (int8), cmd (int8), count (int16), <seq1> <frame1> <seq2> <frame2> etc.
//
// LATER: seq1 (int32), offset1 (int16), bitset1 (int16), etc. // offset < 0 -> all

// prevent excessive resending in low-latency networks
#define AOO_NET_CLIENT_MIN_RESEND_TIME 0.02

void peer::send_message(const message& m, const sendfn& fn, bool binary) {
    // LATER make packet size settable at runtime, see AooSource::send_data()
    const int32_t maxsize = AOO_PACKET_SIZE - (binary ? kBinMessageHeaderSize : kMessageHeaderSize);
    auto d = std::div((int32_t)m.data_.size(), maxsize);

    message_packet p;
    p.type = m.data_.type();
    p.data = nullptr;
    p.size = 0;
    p.tt = m.tt_;
    // p.sequence =
    p.totalsize = m.data_.size();
    p.nframes = d.quot + (d.rem != 0);
    p.frame = 0;
    p.reliable = m.reliable_;

    if (p.reliable) {
        p.sequence = next_sequence_reliable_++;
        auto framesize = d.quot ? maxsize : d.rem;
        // wait twice the average RTT before resending
        auto rtt = average_rtt_.load(std::memory_order_relaxed);
        auto interval = std::max<float>(rtt * 2, AOO_NET_CLIENT_MIN_RESEND_TIME);
        sent_message sm(m.data_, m.tt_, p.sequence, p.nframes, framesize, interval);
        send_buffer_.push(std::move(sm));
    } else {
        // NB: use different sequences for reliable and unreliable messages!
        p.sequence = next_sequence_unreliable_++;
    }

    if (p.nframes > 1) {
        // multi-frame message
        for (int32_t i = 0; i < p.nframes; ++i) {
            p.frame = i;
            if (i == (p.nframes - 1)) {
                p.size = d.rem;
                p.data = m.data_.data() + m.data_.size() - d.rem;
            } else {
                p.size = maxsize;
                p.data = m.data_.data() + (i * maxsize);
            }
            if (binary) {
                send_packet_bin(p, fn);
            } else {
                send_packet_osc(p, fn);
            }
        }
    } else {
        // single-frame message
        p.data = m.data_.data();
        p.size = m.data_.size();
        if (binary) {
            send_packet_bin(p, fn);
        } else {
            send_packet_osc(p, fn);
        }
    }
}

void peer::send_packet_osc(const message_packet& p, const sendfn& fn) const {
    char buf[AOO_MAX_PACKET_SIZE];
    osc::OutboundPacketStream msg(buf, sizeof(buf));
    AooFlag flags = p.reliable * kAooNetMessageReliable;
    // NB: we send *our* ID (= the sender)
    msg << osc::BeginMessage(kAooNetMsgPeerMessage)
        << group_id() << local_id() << (int32_t)flags
        << p.sequence << p.totalsize << p.nframes << p.frame
        << osc::TimeTag(p.tt) << p.type << osc::Blob(p.data, p.size)
        << osc::EndMessage;
    send(msg, fn);
}

void peer::send_packet_bin(const message_packet& p, const sendfn& fn) const {
    AooByte buf[AOO_MAX_PACKET_SIZE];

    AooFlag flags = (p.reliable * kAooBinMsgMessageReliable) |
            ((p.nframes > 1) * kAooBinMsgMessageFrames) |
            ((!p.tt.is_empty()) * kAooBinMsgMessageTimestamp);

    // NB: we send *our* user ID
    auto offset = aoo::binmsg_write_header(buf, sizeof(buf), kAooTypePeer,
                                           kAooBinMsgCmdMessage, group_id(), local_id());
    auto ptr = buf + offset;
    auto end = buf + sizeof(buf);
    aoo::write_bytes<int32_t>(p.sequence, ptr);
    aoo::write_bytes<uint16_t>(flags, ptr);
    aoo::write_bytes<uint16_t>(p.size, ptr);
    if (flags & kAooBinMsgMessageFrames) {
        aoo::write_bytes<int32_t>(p.totalsize, ptr);
        aoo::write_bytes<int16_t>(p.nframes, ptr);
        aoo::write_bytes<int16_t>(p.frame, ptr);
    }
    // only send type and timetag with first frame
    if (p.frame == 0) {
        if (flags & kAooBinMsgMessageTimestamp) {
            aoo::write_bytes<uint64_t>(p.tt, ptr);
        }
        ptr += binmsg_write_string(ptr, end - ptr, p.type);
    }
    // write actual data
    assert((end - ptr) >= p.size);
    memcpy(ptr, p.data, p.size);
    ptr += p.size;

    send(buf, ptr - buf, fn);
}

// if the message is reliable, the other end sends an ack messages:
// /aoo/peer/ack <group> <user> <count> <seq1> <frame1> <seq2> <frame2> etc.
// resp.
// header, count (int32), seq1 (int32), offset1 (int16), bitset1 (int16), etc. // offset < 0 -> all
void peer::send_ack(const message_ack &ack, const sendfn& fn) {
#if AOO_DEBUG_CLIENT_MESSAGE
    LOG_DEBUG("AooClient: send ack (seq: " << ack.seq << ", frame: " << ack.frame
              << ") to " << *this);
#endif
    if (binary_.load(std::memory_order_relaxed)) {
        AooByte buf[64];
        auto onset = binmsg_write_header(buf, sizeof(buf), kAooTypePeer,
                                         kAooBinMsgCmdAck, group_id_, local_id_);
        auto ptr = buf + onset;
        aoo::write_bytes<int32_t>(1, ptr);
        aoo::write_bytes<int32_t>(ack.seq, ptr);
        aoo::write_bytes<int32_t>(ack.frame, ptr);

        send(buf, ptr - buf, fn);
    } else {
        char buf[64];
        osc::OutboundPacketStream msg(buf, sizeof(buf));
        msg << osc::BeginMessage(kAooNetMsgPeerAck)
            << group_id_ << local_id_
            << (int32_t)1 << ack.seq << ack.frame
            << osc::EndMessage;

        send(msg, fn);
    }
}

void peer::handle_osc_message(Client& client, const char *pattern,
                              osc::ReceivedMessageArgumentIterator it,
                              const ip_address& addr) {
    LOG_DEBUG("AooClient: got OSC message " << pattern << " from " << *this);

    if (!strcmp(pattern, kAooNetMsgPing)) {
        handle_ping(client, it, addr, false);
    } else if (!strcmp(pattern, kAooNetMsgPingReply)) {
        handle_ping(client, it, addr, true);
    } else if (!strcmp(pattern, kAooNetMsgMessage)) {
        handle_client_message(client, it);
    } else if (!strcmp(pattern, kAooNetMsgAck)) {
        handle_ack(client, it);
    } else {
        LOG_WARNING("AooClient: got unknown message "
                    << pattern << " from " << *this);
    }
}

void peer::handle_bin_message(Client& client, const AooByte *data, AooSize size,
                              int onset, const ip_address& addr) {
    LOG_DEBUG("AooClient: got binary message from " << *this);

    switch (binmsg_cmd(data, size)) {
    case kAooBinMsgCmdMessage:
        handle_client_message(client, data + onset, size - onset);
        break;
    case kAooBinMsgCmdAck:
        handle_ack(client, data + onset, size - onset);
        break;
    default:
        LOG_WARNING("AooClient: got unknown binary message from " << *this);
    }
}

void peer::handle_ping(Client& client, osc::ReceivedMessageArgumentIterator it,
                       const ip_address& addr, bool reply) {
    if (!connected()) {
        // first ping
    #if FORCE_RELAY
        // force relay
        if (!relay()){
            return;
        }
    #endif
        // Try to find matching address.
        // If we receive a message from a peer behind a symmetric NAT,
        // its IP address will be different from the one we obtained
        // from the server, that's why we're sending and checking the
        // group/user ID in the first place.
        // NB: this only works if *we* are behind a full cone or restricted cone NAT.
        if (std::find(addrlist_.begin(), addrlist_.end(), addr) == addrlist_.end()) {
            LOG_WARNING("AooClient: peer " << *this << " is located behind a symmetric NAT!");
        }

        real_address_ = addr;

        connected_.store(true, std::memory_order_release);

        // push event
        auto e = std::make_unique<peer_event>(kAooNetEventPeerJoin, *this);
        client.send_event(std::move(e));

        LOG_VERBOSE("AooClient: successfully established connection with "
                    << *this << " " << addr << (relay() ? " (relayed)" : ""));
    }

    if (reply) {
        time_tag tt1 = (it++)->AsTimeTag();
        if (!tt1.is_empty()) {
            // regular ping reply
            time_tag tt2 = (it++)->AsTimeTag();
            time_tag tt3 = time_tag::now();

            auto rtt = time_tag::duration(tt1, tt3);
        #if 1
            // NB: we are the only thread writing to average_rtt_, so we don't need a CAS loop!
            auto avg = average_rtt_.load(std::memory_order_relaxed);
            if (avg > 0) {
                // simple low-pass filtering; maybe use cumulative average instead?
                const float coeff = 0.5;
                auto newval = avg * coeff + rtt * (1.0 - coeff);
                average_rtt_.store(newval);
            } else {
                // first ping
                average_rtt_.store(rtt);
            }
        #else
            average_rtt_.store(rtt);
        #endif

            // only send event for regular ping reply!
            auto e = std::make_unique<peer_ping_reply_event>(*this, tt1, tt2, tt3);
            client.send_event(std::move(e));

            LOG_DEBUG("AooClient: got ping reply from " << *this
                      << "(tt1: " << tt1 << ", tt2: " << tt2 << ", tt3: " << tt3
                      << ", rtt: " << rtt << ", average: " << average_rtt_.load() << ")");
        } else {
            // handshake ping reply
            LOG_DEBUG("AooClient: got handshake ping reply from " << *this);
        }
    } else {
        time_tag tt1 = (it++)->AsTimeTag();
        // reply to both handshake and regular pings!!!
        // otherwise, the handshake might fail on the other side.
        // This is not 100% threadsafe, but regular pings will never
        // be sent fast enough to actually cause a race condition.
        // TODO: maybe use a queue instead?
        ping_tt1_ = tt1;
        got_ping_.store(true, std::memory_order_release);
        if (!tt1.is_empty()) {
            // regular ping
            time_tag tt2 = time_tag::now();

            // only send event for regular ping!
            auto e = std::make_unique<peer_ping_event>(*this, tt1, tt2);
            client.send_event(std::move(e));

            LOG_DEBUG("AooClient: got ping from " << *this
                      << "(tt1: " << tt1 << ", tt2: " << tt2 << ")");
        } else {
            // handshake ping
            LOG_DEBUG("AooClient: got handshake ping from " << *this);
        }
    }
}

void peer::handle_client_message(Client &client, osc::ReceivedMessageArgumentIterator it) {
    auto flags = (AooFlag)(it++)->AsInt32();

    message_packet p;
    p.sequence = (it++)->AsInt32();
    p.totalsize = (it++)->AsInt32();
    p.nframes = (it++)->AsInt32();
    p.frame = (it++)->AsInt32();
    p.tt = (time_tag)(it++)->AsTimeTag();
    auto data = osc_read_metadata(it);
    p.type = data.type;
    p.data = data.data;
    p.size = data.size;

    // tell the send thread that it should answer with OSC messages
    binary_.store(false, std::memory_order_relaxed);

    do_handle_client_message(client, p, flags);
}

void peer::handle_client_message(Client &client, const AooByte *data, AooSize size) {
    auto ptr = data;
    int32_t remaining = size;
    AooFlag flags = 0;
    message_packet p;

    if (remaining < 8) {
        goto bad_message;
    }
    p.sequence = aoo::read_bytes<int32_t>(ptr);
    flags = aoo::read_bytes<uint16_t>(ptr);
    p.size = aoo::read_bytes<uint16_t>(ptr);
    remaining -= 8;

    if (flags & kAooBinMsgMessageFrames) {
        if (remaining < 8) {
            goto bad_message;
        }
        p.totalsize = aoo::read_bytes<int32_t>(ptr);
        p.nframes = aoo::read_bytes<int16_t>(ptr);
        p.frame = aoo::read_bytes<int16_t>(ptr);
        remaining -= 8;
    } else {
        p.totalsize = p.size;
        p.nframes = 1;
        p.frame = 0;
    }
    // type and timetag is only sent with first frame
    p.type = nullptr;
    p.tt = 0;
    if (p.frame == 0) {
        if (flags & kAooBinMsgMessageTimestamp) {
            if (remaining < 8) {
                goto bad_message;
            }
            p.tt = aoo::read_bytes<uint64_t>(ptr);
            remaining -= 8;
        }
        p.type = (const char *)ptr;
        auto len = binmsg_skip_string(ptr, remaining);
        ptr += len;
        remaining -= len;
    }

    if (remaining < p.size) {
        goto bad_message;
    }
    p.data = ptr;

    // tell the send thread that it should answer with binary messages
    binary_.store(true, std::memory_order_relaxed);

    do_handle_client_message(client, p, flags);

    return;

bad_message:
    LOG_ERROR("AooClient: received malformed binary message from " << *this);
}

void peer::do_handle_client_message(Client& client, const message_packet& p, AooFlag flags) {
    if (flags & kAooNetMessageReliable) {
        // *** reliable message ***
        auto last_pushed = receive_buffer_.last_pushed();
        auto last_popped = receive_buffer_.last_popped();
        if (p.sequence <= last_popped) {
            // outdated message
        #if AOO_DEBUG_CLIENT_MESSAGE
            LOG_DEBUG("AooClient: ignore outdated message (seq: "
                      << p.sequence << ", frame: " << p.frame << ") from " << *this);
        #endif
            // don't forget to acknowledge!
            send_acks_.push(p.sequence, p.frame);
            return;
        }
        if (p.sequence > last_pushed) {
            // check if we have skipped messages
            // (sequence always starts at 0)
            int32_t skipped = (last_pushed >= 0) ? p.sequence - last_pushed - 1 : p.sequence;
            if (skipped > 0) {
            #if AOO_DEBUG_CLIENT_MESSAGE
                LOG_DEBUG("AooClient: skipped " << skipped << " messages from " << *this);
            #endif
                // insert empty messages
                int32_t onset = (last_pushed >= 0) ? last_pushed + 1 : 0;
                for (int i = 0; i < skipped; ++i) {
                    receive_buffer_.push(received_message(onset + i));
                }
            }
            // add new message
        #if AOO_DEBUG_CLIENT_MESSAGE
            LOG_DEBUG("AooClient: add new message (seq: " << p.sequence
                      << ", frame: " << p.frame << ") from " << *this);
        #endif
            auto& msg = receive_buffer_.push(received_message(p.sequence));
            msg.init(p.nframes, p.totalsize);
            msg.add_frame(p.frame, p.data, p.size);
            if (p.frame == 0) {
                assert(p.type != nullptr);
                msg.set_info(p.type, p.tt);
            }
        } else {
            // add to existing message
        #if AOO_DEBUG_CLIENT_MESSAGE
            LOG_DEBUG("AooClient: add to existing message (seq: " << p.sequence
                      << ", frame: " << p.frame << ") from " << *this);
        #endif
            if (auto msg = receive_buffer_.find(p.sequence)) {
                if (!msg->initialized()) {
                    msg->init(p.nframes, p.totalsize);
                }
                if (!msg->has_frame(p.frame)) {
                    msg->add_frame(p.frame, p.data, p.size);
                    if (p.frame == 0) {
                        assert(p.type != nullptr);
                        msg->set_info(p.type, p.tt);
                    }
                } else {
                #if AOO_DEBUG_CLIENT_MESSAGE
                    LOG_DEBUG("AooClient: ignore duplicate message (seq: " << p.sequence
                              << ", frame: " << p.frame << ") from " << *this);
                #endif
                }
            } else {
                LOG_ERROR("AooClient: could not find message (seq: "
                          << p.sequence << ") from " << *this);
            }
        }
        // check for completed messages
        while (!receive_buffer_.empty()) {
            auto& msg = receive_buffer_.front();
            if (msg.complete()) {
                AooDataView md { msg.type(), msg.data(), (AooSize)msg.size() };
                auto e = std::make_unique<peer_message_event>(
                            group_id(), user_id(), msg.tt_, md);
                client.send_event(std::move(e));

                receive_buffer_.pop();
            } else {
                break;
            }
        }
        // schedule acknowledgement
        send_acks_.push(p.sequence, p.frame);
    } else {
        // *** unreliable message ***
        if (p.nframes > 1) {
            // try to reassemble message
            if (current_msg_.sequence_ != p.sequence) {
            #if AOO_DEBUG_CLIENT_MESSAGE
                LOG_DEBUG("AooClient: new multi-frame message from " << *this);
            #endif
                // start new message (any incomplete previous message is discarded!)
                current_msg_.init(p.sequence, p.nframes, p.totalsize);
            }
            current_msg_.add_frame(p.frame, p.data, p.size);
            if (p.frame == 0) {
                assert(p.type != nullptr);
                current_msg_.set_info(p.type, p.tt);
            }
            if (current_msg_.complete()) {
                AooDataView d { current_msg_.type(), current_msg_.data(), (AooSize)current_msg_.size() };
                auto e = std::make_unique<peer_message_event>(
                            group_id(), user_id(), p.tt, d);
                client.send_event(std::move(e));
            }
        } else {
            // output immediately
            AooDataView d { p.type, p.data, (AooSize)p.size };
            auto e = std::make_unique<peer_message_event>(
                        group_id(), user_id(), p.tt, d);
            client.send_event(std::move(e));
        }
    }
}

void peer::handle_ack(Client &client, osc::ReceivedMessageArgumentIterator it) {
    auto count = (it++)->AsInt32();
    while (count--) {
        auto seq = (it++)->AsInt32();
        auto frame = (it++)->AsInt32();
    #if AOO_DEBUG_CLIENT_MESSAGE
        LOG_DEBUG("AooClient: got ack (seq: " << seq
                  << ", frame: " << frame << ") from " << *this);
    #endif
        received_acks_.push(seq, frame);
    }
}

void peer::handle_ack(Client &client, const AooByte *data, AooSize size) {
    auto ptr = data;
    if (size >= 4) {
        auto count = aoo::read_bytes<int32_t>(ptr);
        if ((size - 4) >= (count * 8)) {
            while (count--) {
                auto seq = aoo::read_bytes<int32_t>(ptr);
                auto frame = aoo::read_bytes<int32_t>(ptr);
            #if AOO_DEBUG_CLIENT_MESSAGE
                LOG_DEBUG("AooClient: got ack (seq: " << seq
                          << ", frame: " << frame << ") from " << *this);
            #endif
                received_acks_.push(seq, frame);
            }
            return; // done
        }
    }
    LOG_ERROR("AooClient: got malformed binary ack message from " << *this);
}

// only called if peer is connected!
void peer::send(const AooByte *data, AooSize size, const sendfn &fn) const {
    assert(connected());
    send(data, size, real_address_, fn);
}

void peer::send(const AooByte *data, AooSize size,
                const ip_address &addr, const sendfn &fn) const {
    if (relay()) {
    #if AOO_DEBUG_RELAY
        if (binmsg_check(data, size)) {
            LOG_DEBUG("AooClient: relay binary message to peer " << *this);
        } else {
            LOG_DEBUG("AooClient: relay message " << (const char *)data << " to peer " << *this);
        }
    #endif
        AooByte buf[AOO_MAX_PACKET_SIZE];
        auto result = write_relay_message(buf, sizeof(buf), data, size, addr);

        fn(buf, result, relay_address_);
    } else {
        fn(data, size, addr);
    }
}

} // net
} // aoo
