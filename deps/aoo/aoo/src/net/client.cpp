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

#define kAooNetMsgServerPing \
    kAooMsgDomain kAooNetMsgServer kAooNetMsgPing

#define kAooNetMsgPeerPing \
    kAooMsgDomain kAooNetMsgPeer kAooNetMsgPing

#define kAooNetMsgPeerReply \
    kAooMsgDomain kAooNetMsgPeer kAooNetMsgReply

#define kAooNetMsgPeerMessage \
    kAooMsgDomain kAooNetMsgPeer kAooNetMsgMessage

#define kAooNetMsgServerLogin \
    kAooMsgDomain kAooNetMsgServer kAooNetMsgLogin

#define kAooNetMsgServerRequest \
    kAooMsgDomain kAooNetMsgServer kAooNetMsgRequest

#define kAooNetMsgPeerJoin \
    kAooNetMsgPeer kAooNetMsgJoin

#define kAooNetMsgPeerLeave \
    kAooNetMsgPeer kAooNetMsgLeave

#define kAooNetMsgGroupJoin \
    kAooNetMsgGroup kAooNetMsgJoin

#define kAooNetMsgGroupLeave \
    kAooNetMsgGroup kAooNetMsgLeave

#define kAooNetMsgServerGroupJoin \
    kAooMsgDomain kAooNetMsgServer kAooNetMsgGroupJoin

#define kAooNetMsgServerGroupLeave \
    kAooMsgDomain kAooNetMsgServer kAooNetMsgGroupLeave

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
        snprintf(&output[i * 2], 3, "%02X", result[i]);
    }

    return output;
}

void copy_string(const std::string& src, char *dst, int32_t size){
    const auto limit = size - 1; // leave space for '\0'
    auto n = (src.size() > limit) ? limit : src.size();
    memcpy(dst, src.data(), n);
    dst[n] = '\0';
}

// optimized version of aoo_parse_pattern() for client/server
AooError parse_pattern(const AooByte *msg, int32_t n, AooMsgType& type, int32_t& offset)
{
    int32_t count = 0;
    if (n >= kAooBinMsgHeaderSize &&
        !memcmp(msg, kAooBinMsgDomain, kAooBinMsgDomainSize))
    {
        // domain (int32), type (int16), cmd (int16), id (int32) ...
        type = aoo::from_bytes<int16_t>(msg + 4);
        offset = 12;

        return kAooOk;
    } else if (n >= kAooMsgDomainLen
            && !memcmp(msg, kAooMsgDomain, kAooMsgDomainLen))
    {
        count += kAooMsgDomainLen;
        if (n >= (count + kAooNetMsgServerLen)
            && !memcmp(msg + count, kAooNetMsgServer, kAooNetMsgServerLen))
        {
            type = kAooTypeServer;
            count += kAooNetMsgServerLen;
        }
        else if (n >= (count + kAooNetMsgClientLen)
            && !memcmp(msg + count, kAooNetMsgClient, kAooNetMsgClientLen))
        {
            type = kAooTypeClient;
            count += kAooNetMsgClientLen;
        }
        else if (n >= (count + kAooNetMsgPeerLen)
            && !memcmp(msg + count, kAooNetMsgPeer, kAooNetMsgPeerLen))
        {
            type = kAooTypePeer;
            count += kAooNetMsgPeerLen;
        }
        else if (n >= (count + kAooNetMsgRelayLen)
            && !memcmp(msg + count, kAooNetMsgRelay, kAooNetMsgRelayLen))
        {
            type = kAooTypeRelay;
            count += kAooNetMsgRelayLen;
        } else {
            return kAooErrorUnknown;
        }

        offset = count;

        return kAooOk;
    } else {
        return kAooErrorUnknown; // not an AOO message
    }
}

} // net
} // aoo

//--------------------- AooClient -----------------------------//

AOO_API AooClient * AOO_CALL AooClient_new(
        const void *address, AooAddrSize addrlen, AooFlag flags, AooError *err) {
    return aoo::construct<aoo::net::Client>(address, addrlen, flags, err);
}

aoo::net::Client::Client(const void *address, AooAddrSize addrlen,
                                 AooFlag flags, AooError *err)
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

AOO_API AooError AOO_CALL AooClient_run(AooClient *client){
    return client->run();
}

AooError AOO_CALL aoo::net::Client::run(){
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
                    msg << osc::BeginMessage(kAooNetMsgServerPing)
                        << osc::EndMessage;

                    send_server_message((const AooByte *)msg.Data(), msg.Size());
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
            LOG_ERROR("aoo_client: source already added");
            return kAooErrorUnknown;
        } else if (s.id == id){
            LOG_WARNING("aoo_client: source with id " << id
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
    LOG_ERROR("aoo_client: source not found");
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
            LOG_ERROR("aoo_client: sink already added");
            return kAooOk;
        } else if (s.id == id){
            LOG_WARNING("aoo_client: sink with id " << id
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
    LOG_ERROR("aoo_client: sink not found");
    return kAooErrorUnknown;
}

AOO_API AooError AOO_CALL AooClient_getPeerByName(
        AooClient *client, const AooChar *group,
        const AooChar *user, void *address, AooAddrSize *addrlen)
{
    return client->getPeerByName(group, user, address, addrlen);
}

AooError AOO_CALL aoo::net::Client::getPeerByName(
        const char *group, const char *user,
        void *address, AooAddrSize *addrlen)
{
    peer_lock lock(peers_);
    for (auto& p : peers_){
        // we can only access the address if the peer is connected!
        if (p.match(group, user) && p.connected()){
            if (address){
                auto& addr = p.address();
                if (*addrlen < (AooAddrSize)addr.length()){
                    return kAooErrorUnknown;
                }
                memcpy(address, addr.address(), addr.length());
                *addrlen = addr.length();
            }
            return kAooOk;
        }
    }
    return kAooErrorUnknown;
}

AOO_API AooError AOO_CALL AooClient_sendRequest(
        AooClient *client, AooNetRequestType request, void *data,
        AooNetCallback callback, void *user) {
    return client->sendRequest(request, data, callback, user);
}

AooError AOO_CALL aoo::net::Client::sendRequest(
        AooNetRequestType request, void *data,
        AooNetCallback callback, void *user)
{
    switch (request){
    case kAooNetRequestConnect:
    {
        auto d = (AooNetRequestConnect *)data;
        do_connect(d->hostName, d->port, d->userName, d->userPwd, callback, user);
        break;
    }
    case kAooNetRequestDisconnect:
        do_disconnect(callback, user);
        break;
    case kAooNetRequestJoinGroup:
    {
        auto d = (AooNetRequestJoinGroup *)data;
        do_join_group(d->groupName, d->groupPwd, callback, user);
        break;
    }
    case kAooNetRequestLeaveGroup:
    {
        auto d = (AooNetRequestLeaveGroup *)data;
        do_leave_group(d->groupName, callback, user);
        break;
    }
    default:
        LOG_ERROR("aoo client: unknown request " << request);
        return kAooErrorUnknown;
    }
    return kAooOk;
}

AOO_API AooError AOO_CALL AooClient_sendPeerMessage(
        AooClient *client, const AooByte *data, AooInt32 size,
        const void *addr, AooAddrSize len, AooFlag flags)
{
    return client->sendPeerMessage(data, size, addr, len, flags);
}

AooError AOO_CALL aoo::net::Client::sendPeerMessage(
        const AooByte *data, AooInt32 size,
        const void *addr, AooAddrSize len, AooFlag flags)
{
    // for now, we simply achieve 'reliable' messages by relaying over TCP
    // LATER implement ack mechanism over UDP.
    bool reliable = flags & kAooNetMessageReliable;

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
        LOG_WARNING("aoo_client: not an AOO NET message!");
        return kAooErrorUnknown;
    }

    if (type == kAooTypeSource){
        // forward to matching source
        for (auto& s : sources_){
            if (s.id == id){
                return s.source->handleMessage(data, size, addr, len);
            }
        }
        LOG_WARNING("aoo_client: handle_message(): source not found");
    } else if (type == kAooTypeSink){
        // forward to matching sink
        for (auto& s : sinks_){
            if (s.id == id){
                return s.sink->handleMessage(data, size, addr, len);
            }
        }
        LOG_WARNING("aoo_client: handle_message(): sink not found");
    } else if (udp_client_){
        // forward to UDP client
        ip_address address((const sockaddr *)addr, len);
        return udp_client_->handle_message(data, size, address, type, onset);
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
    struct proxy_data {
        Client *self;
        AooSendFunc fn;
        void *user;
    };

    auto proxy = [](void *x, const AooByte *data, AooInt32 size,
            const void *address, AooAddrSize addrlen, AooFlag flags) -> AooInt32 {
        auto p = static_cast<proxy_data *>(x);

        bool relay = flags & kAooEndpointRelay;
        if (relay){
            // relay via server
            ip_address addr((const sockaddr *)address, addrlen);

            sendfn reply(p->fn, p->user);

            p->self->udp().send_peer_message(data, size, addr, reply, true);

            return kAooOk;
        } else {
            // just forward
            return p->fn(p->user, data, size, address, addrlen, flags);
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
    std::unique_ptr<ievent> e;
    while (events_.try_pop(e)){
        eventhandler_(eventcontext_, &e->event_, kAooThreadLevelUnknown);
    }
    return kAooOk;
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
    default:
        LOG_WARNING("aoo_client: unsupported control " << ctl);
        return kAooErrorNotImplemented;
    }
    return kAooOk;
}

namespace aoo {
namespace net {

bool Client::handle_peer_message(const osc::ReceivedMessage& msg, int onset,
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
void Client::perform_send_message(const AooByte *data, int32_t size, int32_t flags,
                                  const sendfn& fn, T&& filter)
{
    bool reliable = flags & kAooNetMessageReliable;
    // embed inside an OSC message:
    // /aoo/peer/msg' (b)<message>
    try {
        char buf[AOO_MAX_PACKET_SIZE];
        osc::OutboundPacketStream msg(buf, sizeof(buf));
        msg << osc::BeginMessage(kAooNetMsgPeerMessage)
            << osc::Blob(data, size) << osc::EndMessage;

        peer_lock lock(peers_);
        for (auto& peer : peers_){
            if (filter(peer)){
                auto& addr = peer.address();
                LOG_DEBUG("aoo_client: send message " << data
                          << " to " << addr);
                // Note: reliable messages are dispatched in the TCP network thread,
                // unreliable messages are dispatched in the UDP network thread.
                if (reliable){
                    send_peer_message((const AooByte *)msg.Data(), msg.Size(), addr);
                } else if (udp_client_){
                    udp_client_->send_peer_message((const AooByte *)msg.Data(), msg.Size(),
                                                   addr, fn, peer.relay());
                }
            }
        }
    } catch (const osc::Exception& e){
        LOG_ERROR("aoo_client: error sending OSC message: " << e.what());
    }
}

void Client::do_connect(const char *host, int port,
                        const char *name, const char *pwd,
                        AooNetCallback cb, void *user)
{
    auto cmd = std::make_unique<connect_cmd>(cb, user, host, port,
                                             name, encrypt(pwd));

    push_command(std::move(cmd));
}

void Client::perform_connect(const std::string& host, int port,
                             const std::string& name, const std::string& pwd,
                             AooNetCallback cb, void *user)
{
    auto state = state_.load();
    if (state != client_state::disconnected){
        AooNetReplyError reply;
        reply.errorCode = 0;
        if (state == client_state::connected){
            reply.errorMessage = "already connected";
        } else {
            reply.errorMessage = "already connecting";
        }

        if (cb) cb(user, kAooErrorUnknown, &reply);

        return;
    }

    state_.store(client_state::connecting);

    int err = try_connect(host, port);
    if (err != 0){
        // event
        std::string errmsg = socket_strerror(err);

        close();

        AooNetReplyError reply;
        reply.errorCode = err;
        reply.errorMessage = errmsg.c_str();

        if (cb) cb(user, kAooErrorUnknown, &reply);

        return;
    }

    username_ = name;
    password_ = pwd;
    callback_ = cb;
    userdata_ = user;

    state_.store(client_state::handshake);
}

int Client::try_connect(const std::string &host, int port){
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
        LOG_DEBUG("\t" << addr);
    }

    // for actual TCP connection, just pick the first result
    auto& remote = result.front();
    LOG_VERBOSE("try to connect to " << remote);

    // try to connect (LATER make timeout configurable)
    if (socket_connect(socket_, remote, 5) < 0){
        int err = socket_errno();
        LOG_ERROR("aoo_client: couldn't connect to " << remote << ": "
                  << socket_strerror(err));
        return err;
    }

    // get local network interface
    ip_address temp;
    if (socket_address(socket_, temp) < 0){
        int err = socket_errno();
        LOG_ERROR("aoo_client: couldn't get socket name: "
                  << socket_strerror(err));
        return err;
    }
    ip_address local(temp.name(), udp_client_->port(), type_);

    LOG_VERBOSE("aoo_client: successfully connected to " << remote);
    LOG_VERBOSE("aoo_client: local address: " << local);

    udp_client_->start_handshake(local, std::move(result));

    return 0;
}

void Client::do_disconnect(AooNetCallback cb, void *user){
    auto cmd = std::make_unique<disconnect_cmd>(cb, user);

    push_command(std::move(cmd));
}

void Client::perform_disconnect(AooNetCallback cb, void *user){
    auto state = state_.load();
    if (state != client_state::connected){
        AooNetReplyError reply;
        reply.errorMessage = (state == client_state::disconnected)
                ? "not connected" : "still connecting";
        reply.errorCode = 0;

        if (cb) cb(user, kAooErrorUnknown, &reply);

        return;
    }

    close(true);

    if (cb) cb(user, kAooOk, nullptr); // always succeeds
}

void Client::perform_login(const ip_address_list& addrlist){
    state_.store(client_state::login);

    char buf[AOO_MAX_PACKET_SIZE];
    osc::OutboundPacketStream msg(buf, sizeof(buf));
    msg << osc::BeginMessage(kAooNetMsgServerLogin)
        << (int32_t)make_version()
        << username_.c_str() << password_.c_str();
    for (auto& addr : addrlist){
        // unmap IPv4 addresses for IPv4 only servers
        msg << addr.name_unmapped() << addr.port();
    }
    msg << osc::EndMessage;

    send_server_message((const AooByte *)msg.Data(), msg.Size());
}

void Client::perform_timeout(){
    AooNetReplyError reply;
    reply.errorCode = 0;
    reply.errorMessage = "UDP handshake time out";

    callback_(userdata_, kAooErrorUnknown, &reply);

    if (state_.load() == client_state::handshake){
        close();
    }
}

void Client::do_join_group(const char *group, const char *pwd,
                           AooNetCallback cb, void *user){
    auto cmd = std::make_unique<group_join_cmd>(cb, user, group, encrypt(pwd));

    push_command(std::move(cmd));
}

void Client::perform_join_group(const std::string &group, const std::string &pwd,
                                AooNetCallback cb, void *user)
{

    auto request = [group, cb, user](
            const char *pattern,
            const osc::ReceivedMessage& msg)
    {
        if (!strcmp(pattern, kAooNetMsgGroupJoin)){
            auto it = msg.ArgumentsBegin();
            std::string g = (it++)->AsString();
            if (g == group){
                int32_t status = (it++)->AsInt32();
                if (status > 0){
                    LOG_VERBOSE("aoo_client: successfully joined group " << group);
                    if (cb) cb(user, kAooOk, nullptr);
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
                    AooNetReplyError reply;
                    reply.errorCode = 0;
                    reply.errorMessage = errmsg.c_str();

                    if (cb) cb(user, kAooErrorUnknown, &reply);
                }

                return true;
            }
        }
        return false;
    };
    pending_requests_.push_back(request);

    char buf[AOO_MAX_PACKET_SIZE];
    osc::OutboundPacketStream msg(buf, sizeof(buf));
    msg << osc::BeginMessage(kAooNetMsgServerGroupJoin)
        << group.c_str() << pwd.c_str() << osc::EndMessage;

    send_server_message((const AooByte *)msg.Data(), msg.Size());
}

void Client::do_leave_group(const char *group,
                                AooNetCallback cb, void *user){
    auto cmd = std::make_unique<group_leave_cmd>(cb, user, group);

    push_command(std::move(cmd));
}

void Client::perform_leave_group(const std::string &group,
                                 AooNetCallback cb, void *user)
{
    auto request = [this, group, cb, user](
            const char *pattern,
            const osc::ReceivedMessage& msg)
    {
        if (!strcmp(pattern, kAooNetMsgGroupLeave)){
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

                    if (cb) cb(user, kAooOk, nullptr);
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
                    AooNetReplyError reply;
                    reply.errorCode = 0;
                    reply.errorMessage = errmsg.c_str();

                    if (cb) cb(user, kAooErrorUnknown, &reply);
                }

                return true;
            }
        }
        return false;
    };
    pending_requests_.push_back(request);

    char buf[AOO_MAX_PACKET_SIZE];
    osc::OutboundPacketStream msg(buf, sizeof(buf));
    msg << osc::BeginMessage(kAooNetMsgServerGroupLeave)
        << group.c_str() << osc::EndMessage;

    send_server_message((const AooByte *)msg.Data(), msg.Size());
}

void Client::send_event(std::unique_ptr<ievent> e)
{
    switch (eventmode_){
    case kAooEventModePoll:
        events_.push(std::move(e));
        break;
    case kAooEventModeCallback:
        // client only has network threads
        eventhandler_(eventcontext_, &e->event_, kAooThreadLevelAudio);
        break;
    default:
        break;
    }
}

void Client::push_command(std::unique_ptr<icommand>&& cmd){
    commands_.push(std::move(cmd));

    signal();
}

bool Client::wait_for_event(float timeout){
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

void Client::receive_data(){
    char buffer[AOO_MAX_PACKET_SIZE];
    auto result = recv(socket_, buffer, sizeof(buffer), 0);
    if (result > 0){
        recvbuffer_.write_bytes((uint8_t *)buffer, result);

        // handle packets
        uint8_t buf[AOO_MAX_PACKET_SIZE];
        while (true){
            auto size = recvbuffer_.read_packet(buf, sizeof(buf));
            if (size > 0){
                try {
                    osc::ReceivedPacket packet((const char *)buf, size);
                    if (packet.IsBundle()){
                        osc::ReceivedBundle bundle(packet);
                        handle_server_bundle(bundle);
                    } else {
                        handle_server_message((const AooByte *)packet.Contents(),
                                              packet.Size());
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

void Client::send_server_message(const AooByte *data, int32_t size){
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

void Client::send_peer_message(const AooByte *data, int32_t size,
                               const ip_address& addr) {
    // /aoo/relay <ip> <port> <msg>
    char buf[AOO_MAX_PACKET_SIZE];
    osc::OutboundPacketStream msg(buf, sizeof(buf));
    // send unmapped IP address in case peer is IPv4 only!
    msg << osc::BeginMessage(kAooMsgDomain kAooNetMsgRelay)
        << addr.name_unmapped() << addr.port() << osc::Blob(data, size)
        << osc::EndMessage;

    send_server_message((const AooByte *)msg.Data(), msg.Size());
}

void Client::handle_server_message(const AooByte *data, int32_t n){
    osc::ReceivedPacket packet((const char *)data, n);
    osc::ReceivedMessage msg(packet);

    AooMsgType type;
    int32_t onset;
    auto err = parse_pattern(data, n, type, onset);
    if (err != kAooOk){
        LOG_WARNING("aoo_client: not an AOO NET message!");
        return;
    }

    try {
        if (type == kAooTypeClient){
            // now compare subpattern
            auto pattern = msg.AddressPattern() + onset;
            LOG_DEBUG("aoo_client: got message " << pattern << " from server");

            if (!strcmp(pattern, kAooNetMsgPing)){
                LOG_DEBUG("aoo_client: got TCP ping from server");
            } else if (!strcmp(pattern, kAooNetMsgPeerJoin)){
                handle_peer_add(msg);
            } else if (!strcmp(pattern, kAooNetMsgPeerLeave)){
                handle_peer_remove(msg);
            } else if (!strcmp(pattern, kAooNetMsgLogin)){
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
        } else if (type == kAooTypeRelay){
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

void Client::handle_server_bundle(const osc::ReceivedBundle &bundle){
    auto it = bundle.ElementsBegin();
    while (it != bundle.ElementsEnd()){
        if (it->IsBundle()){
            osc::ReceivedBundle b(*it);
            handle_server_bundle(b);
        } else {
            handle_server_message((const AooByte *)it->Contents(), it->Size());
        }
        ++it;
    }
}

void Client::handle_login(const osc::ReceivedMessage& msg){
    // make sure that state hasn't changed
    if (state_.load() == client_state::login){
        auto it = msg.ArgumentsBegin();
        int32_t status = (it++)->AsInt32();

        if (status > 0){
            int32_t id = (it++)->AsInt32();
            // for backwards compatibility (LATER remove check)
            uint32_t flags = (it != msg.ArgumentsEnd()) ?
                (uint32_t)(it++)->AsInt32() : 0;
            // connected!
            state_.store(client_state::connected);
            LOG_VERBOSE("aoo_client: successfully logged in (user ID: "
                        << id << " )");
            // notify
            AooNetReplyConnect reply;
            reply.userId = id;
            reply.serverFlags = flags;
            server_flags_ = flags;

            callback_(userdata_, kAooOk, &reply);
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
            AooNetReplyError reply;
            reply.errorCode = 0;
            reply.errorMessage = errmsg.c_str();

            cb(ud, kAooErrorUnknown, &reply);
        }
    }
}

static void unwrap_message(const osc::ReceivedMessage& msg,
    ip_address& addr, ip_address::ip_type type, const AooByte * & retdata, std::size_t & retsize)
{
    auto it = msg.ArgumentsBegin();

    auto ip = (it++)->AsString();
    auto port = (it++)->AsInt32();

    addr = ip_address(ip, port, type);

    const void *blobData;
    osc::osc_bundle_element_size_t blobSize;
    (it++)->AsBlob(blobData, blobSize);

    retdata = (const AooByte *)blobData;
    retsize = blobSize;
}

void Client::handle_relay_message(const osc::ReceivedMessage &msg){
    ip_address addr;
    const AooByte * retmsg = 0;
    std::size_t retsize = 0;
    unwrap_message(msg, addr, type(), retmsg, retsize);

    osc::ReceivedPacket packet ((const char *)retmsg, retsize);
    osc::ReceivedMessage relayMsg(packet);

    // for now, we only handle peer OSC messages
    if (!strcmp(relayMsg.AddressPattern(), kAooNetMsgPeerMessage)){
        // get embedded OSC message
        const void *data;
        osc::osc_bundle_element_size_t size;
        relayMsg.ArgumentsBegin()->AsBlob(data, size);

        LOG_DEBUG("aoo_client: got relayed peer message " << (const char *)data
                  << " from " << addr);

        auto e = std::make_unique<Client::message_event>(
                    (const char *)data, size, addr);

        send_event(std::move(e));
    } else {
        LOG_WARNING("aoo_client: got unexpected relay message "
                    << relayMsg.AddressPattern());
    }
}

void Client::handle_peer_add(const osc::ReceivedMessage& msg){
    auto count = msg.ArgumentCount();
    auto it = msg.ArgumentsBegin();
    std::string group = (it++)->AsString();
    std::string user = (it++)->AsString();
    int32_t id = (it++)->AsInt32();
    count -= 3;

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
        if (p.match(group, id)){
            LOG_ERROR("aoo_client: peer " << p << " already added");
            return;
        }
    }
    peers_.emplace_front(*this, id, group, user, std::move(addrlist));

    auto e = std::make_unique<peer_event>(
                kAooNetEventPeerHandshake,
                group.c_str(), user.c_str(), id);
    send_event(std::move(e));

    LOG_VERBOSE("aoo_client: new peer " << peers_.front());
}

void Client::handle_peer_remove(const osc::ReceivedMessage& msg){
    auto it = msg.ArgumentsBegin();
    std::string group = (it++)->AsString();
    std::string user = (it++)->AsString();
    int32_t id = (it++)->AsInt32();

    peer_lock lock(peers_);
    auto result = std::find_if(peers_.begin(), peers_.end(),
        [&](auto& p){ return p.match(group, id); });
    if (result == peers_.end()){
        LOG_ERROR("aoo_client: couldn't remove " << group << "|" << user);
        return;
    }

    // only send event if we're connected, which means
    // that an kAooNetEventPeerJoin event has been sent.
    if (result->connected()){
        ip_address addr = result->address();

        auto e = std::make_unique<peer_event>(
            kAooNetEventPeerLeave, addr,
            group.c_str(), user.c_str(), id);
        send_event(std::move(e));
    }

    peers_.erase(result);

    LOG_VERBOSE("aoo_client: peer " << group << "|" << user << " left");
}

bool Client::signal(){
    // LOG_DEBUG("aoo_client signal");
    return socket_signal(eventsocket_);
}

void Client::close(bool manual){
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
        auto e = std::make_unique<event>(kAooNetEventDisconnect);
        send_event(std::move(e));
    }
    state_.store(client_state::disconnected);
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

//-------------------------- events ---------------------------//

Client::error_event::error_event(int32_t code, const char *msg)
{
    error_event_.type = kAooNetEventError;
    error_event_.errorCode = code;
    error_event_.errorMessage = aoo::copy_string(msg);
}

Client::error_event::~error_event()
{
    free_string((char *)error_event_.errorMessage);
}

Client::ping_event::ping_event(int32_t type, const ip_address& addr,
                               uint64_t tt1, uint64_t tt2, uint64_t tt3)
{
    ping_event_.type = type;
    ping_event_.address = copy_sockaddr(addr.address(), addr.length());
    ping_event_.addrlen = addr.length();
    ping_event_.tt1 = tt1;
    ping_event_.tt2 = tt2;
    ping_event_.tt3 = tt3;
}

Client::ping_event::~ping_event()
{
    free_sockaddr((void *)ping_event_.address, ping_event_.addrlen);
}

Client::peer_event::peer_event(int32_t type, const ip_address& addr,
                               const char *group, const char *user,
                               int32_t id)
{
    peer_event_.type = type;
    peer_event_.address = copy_sockaddr(addr.address(), addr.length());
    peer_event_.addrlen = addr.length();
    peer_event_.groupName = aoo::copy_string(group);
    peer_event_.userName = aoo::copy_string(user);
    peer_event_.userId = id;
}

Client::peer_event::peer_event(int32_t type, const char *group,
                               const char *user, int32_t id)
{
    peer_event_.type = type;
    peer_event_.address = nullptr;
    peer_event_.addrlen = 0;
    peer_event_.groupName = aoo::copy_string(group);
    peer_event_.userName = aoo::copy_string(user);
    peer_event_.userId = id;
}


Client::peer_event::~peer_event()
{
    free_string((char *)peer_event_.userName);
    free_string((char *)peer_event_.groupName);
    free_sockaddr((void *)peer_event_.address, peer_event_.addrlen);
}

Client::message_event::message_event(const char *data, int32_t size,
                                     const ip_address& addr)
{
    message_event_.type = kAooNetEventPeerMessage;
    message_event_.address = copy_sockaddr(addr.address(), addr.length());
    message_event_.addrlen = addr.length();
    auto msg = (AooByte *)aoo::allocate(size);
    memcpy(msg, data, size);
    message_event_.data = msg;
    message_event_.size = size;
}

Client::message_event::~message_event()
{
    aoo::deallocate((char *)message_event_.data, message_event_.size);
    free_sockaddr((void *)message_event_.address, message_event_.addrlen);
}

//---------------------- udp_client ------------------------//

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
            auto cmd = std::make_unique<Client::timeout_cmd>();

            client_->push_command(std::move(cmd));

            return;
        }
        // send handshakes in fast succession
        if (delta >= client_->request_interval()){
            char buf[64];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            msg << osc::BeginMessage(kAooNetMsgServerRequest) << osc::EndMessage;

            scoped_shared_lock lock(mutex_);
            for (auto& addr : server_addrlist_){
                reply((const AooByte *)msg.Data(), msg.Size(), addr);
            }
            last_ping_time_ = elapsed_time;
        }
    } else if (state == client_state::connected){
        // send regular pings
        if (delta >= client_->ping_interval()){
            char buf[64];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            msg << osc::BeginMessage(kAooNetMsgServerPing)
                << osc::EndMessage;

            send_server_message((const AooByte *)msg.Data(), msg.Size(), reply);
            last_ping_time_ = elapsed_time;
        }
    }
}

AooError udp_client::handle_message(const AooByte *data, int32_t n,
                                    const ip_address &addr,
                                    AooMsgType type, int32_t onset){
    try {
        osc::ReceivedPacket packet((const char *)data, n);
        osc::ReceivedMessage msg(packet);

        LOG_DEBUG("aoo_client: handle UDP message " << msg.AddressPattern()
            << " from " << addr);

        if (type == kAooTypePeer){
            // peer message
            //
            // NOTE: during the handshake process it is expected that
            // we receive UDP messages which we have to ignore:
            // a) pings from a peer which we haven't had the chance to add yet
            // b) pings sent to alternative endpoint addresses
            if (!client_->handle_peer_message(msg, onset, addr)){
                LOG_VERBOSE("aoo_client: ignoring UDP message "
                            << msg.AddressPattern() << " from endpoint " << addr);
            }
        } else if (type == kAooTypeClient){
            // server message
            if (is_server_address(addr)){
                handle_server_message(msg, onset);
            } else {
                LOG_WARNING("aoo_client: got message from unknown server " << addr);
            }
        } else if (type == kAooTypeRelay){
            handle_relay_message(msg);
        } else {
            LOG_WARNING("aoo_client: got unexpected message " << msg.AddressPattern());
            return kAooErrorUnknown;
        }

        return kAooOk;
    } catch (const osc::Exception& e){
        LOG_ERROR("aoo_client: exception in handle_message: " << e.what());
    #if 0
        on_exception("UDP message", e);
    #endif
        return kAooErrorUnknown;
    }
}

void udp_client::handle_relay_message(const osc::ReceivedMessage &msg){
    ip_address addr;
    const AooByte * retmsg = 0;
    std::size_t retsize = 0;
    unwrap_message(msg, addr, client_->type(), retmsg, retsize);
#if DEBUG_RELAY
    LOG_DEBUG("aoo_client: got relayed message of size: " << retsize);
#endif

    client_->handleMessage(retmsg, retsize,
                           addr.address(), addr.length());
}

void udp_client::send_peer_message(const AooByte *data, int32_t size,
                                   const ip_address& addr,
                                   const sendfn& fn, bool relay)
{
    if (relay){
        char buf[AOO_MAX_PACKET_SIZE];
        osc::OutboundPacketStream msg(buf, sizeof(buf));
        // send unmapped address in case the peer is IPv4 only!
        msg << osc::BeginMessage(kAooMsgDomain kAooNetMsgRelay)
            << addr.name_unmapped() << addr.port() << osc::Blob(data, size)
            << osc::EndMessage;
        send_server_message((const AooByte *)msg.Data(), msg.Size(), fn);
    } else {
        fn(data, size, addr);
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

void udp_client::send_server_message(const AooByte *data, int32_t size, const sendfn& fn)
{
    scoped_shared_lock lock(mutex_);
    if (!server_addrlist_.empty()){
        auto& remote = server_addrlist_.front();
        if (remote.valid()){
            fn(data, size, remote);
        }
    }
}

void udp_client::handle_server_message(const osc::ReceivedMessage& msg, int onset){
    auto pattern = msg.AddressPattern() + onset;
    try {
        if (!strcmp(pattern, kAooNetMsgPing)){
            LOG_DEBUG("aoo_client: got UDP ping from server");
        } else if (!strcmp(pattern, kAooNetMsgReply)){
            if (client_->current_state() == client_state::handshake){
                // retrieve public IP + port
                auto it = msg.ArgumentsBegin();
                std::string ip = (it++)->AsString();
                int port = (it++)->AsInt32();

                ip_address addr(ip, port, client_->type());

                scoped_lock lock(mutex_);
                for (auto& a : public_addrlist_){
                    if (a == addr){
                        LOG_DEBUG("aoo_client: public address " << addr
                                  << " already received");
                        return; // already received
                    }
                }
                public_addrlist_.push_back(addr);
                LOG_VERBOSE("aoo_client: got public address " << addr);

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

                    auto cmd = std::make_unique<Client::login_cmd>(std::move(addrlist));

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

//------------------------- peer ------------------------------//

peer::peer(Client& client, int32_t id, const std::string& group,
           const std::string& user, ip_address_list&& addrlist)
    : client_(&client), id_(id), group_(group), user_(user),
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
    os << p.group_ << "|" << p.user_;
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
            msg << osc::BeginMessage(kAooNetMsgPeerPing) << osc::EndMessage;

            client_->udp().send_peer_message((const AooByte *)msg.Data(), msg.Size(),
                                             real_address_, reply, relay());

            last_pingtime_ = elapsed_time;
        }
        // reply to /ping message
        if (got_ping_.exchange(false)){
            char buf[64];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            msg << osc::BeginMessage(kAooNetMsgPeerReply) << osc::EndMessage;

            client_->udp().send_peer_message((const AooByte *)msg.Data(), msg.Size(),
                                             real_address_, reply, relay());
        }
    } else if (!timeout_) {
        // try to establish UDP connection with peer
        if (elapsed_time > client_->request_timeout()){
            // time out
            bool can_relay = client_->have_server_flag(kAooNetServerRelay);
            if (can_relay && !relay()){
                LOG_VERBOSE("aoo_client: try to relay " << *this);
                // try to relay traffic over server
                start_time_ = now; // reset timer
                last_pingtime_ = 0;
                relay_ = true;
                return;
            }

            // couldn't establish relay connection!
            const char *what = relay() ? "relay" : "peer-to-peer";
            LOG_ERROR("aoo_client: couldn't establish UDP " << what
                      << " connection to " << *this << "; timed out after "
                      << client_->request_timeout() << " seconds");

            std::stringstream ss;
            ss << "couldn't establish connection with peer " << *this;

            auto e1 = std::make_unique<Client::error_event>(0, ss.str().c_str());
            client_->send_event(std::move(e1));

            auto e2 = std::make_unique<Client::peer_event>(
                        kAooNetEventPeerTimeout,
                        group().c_str(), user().c_str(), id());
            client_->send_event(std::move(e2));

            timeout_ = true;

            return;
        }
        // send handshakes in fast succession to all addresses
        // until we get a reply from one of them (see handle_message())
        if (delta >= client_->request_interval()){
            char buf[64];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            // Include user ID, so peers can identify us even
            // if we're behind a symmetric NAT.
            // NOTE: This trick doesn't work if both parties are
            // behind a symmetrict NAT; in that case, UDP hole punching
            // simply doesn't work.
            msg << osc::BeginMessage(kAooNetMsgPeerPing)
                << (int32_t)id_ << osc::EndMessage;

            for (auto& addr : addresses_){
                client_->udp().send_peer_message((const AooByte *)msg.Data(),
                                                 msg.Size(), addr, reply, relay());
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
    if (!strcmp(pattern, kAooNetMsgPing)){
        if (msg.ArgumentCount() > 0){
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
    if (!connected()){
        if (!handle_first_message(msg, onset, addr)){
            return;
        }

        // push event
        auto e = std::make_unique<Client::peer_event>(
                    kAooNetEventPeerJoin, addr,
                    group().c_str(), user().c_str(), id());

        client_->send_event(std::move(e));

        LOG_VERBOSE("aoo_client: successfully established connection with "
                  << *this << " " << addr << (relay() ? " (relayed)" : ""));
    }

    auto pattern = msg.AddressPattern() + onset;
    try {
        if (!strcmp(pattern, kAooNetMsgPing)){
            LOG_DEBUG("aoo_client: got ping from " << *this);
            got_ping_.store(true);
        } else if (!strcmp(pattern, kAooNetMsgReply)){
            LOG_DEBUG("aoo_client: got reply from " << *this);
        } else if (!strcmp(pattern, kAooNetMsgMessage)){
            // get embedded OSC message
            const void *data;
            osc::osc_bundle_element_size_t size;
            msg.ArgumentsBegin()->AsBlob(data, size);

            LOG_DEBUG("aoo_client: got message " << (const char *)data
                      << " from " << addr);

            auto e = std::make_unique<Client::message_event>(
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
