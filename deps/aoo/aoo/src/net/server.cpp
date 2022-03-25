/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "server.hpp"

#include <functional>
#include <algorithm>
#include <iostream>
#include <sstream>

// OSC address patterns

#define kAooNetMsgClientPingReply \
    kAooMsgDomain kAooNetMsgClient kAooNetMsgPing kAooNetMsgReply

#define kAooNetMsgClientQuery \
    kAooMsgDomain kAooNetMsgClient kAooNetMsgQuery

#define kAooNetMsgClientLogin \
    kAooMsgDomain kAooNetMsgClient kAooNetMsgLogin

#define kAooNetMsgGroupJoin \
    kAooNetMsgGroup kAooNetMsgJoin

#define kAooNetMsgGroupLeave \
    kAooNetMsgGroup kAooNetMsgLeave

#define kAooNetMsgClientGroupJoin \
    kAooMsgDomain kAooNetMsgClient kAooNetMsgGroupJoin

#define kAooNetMsgClientGroupLeave \
    kAooMsgDomain kAooNetMsgClient kAooNetMsgGroupLeave

#define kAooNetMsgClientRequest \
    kAooMsgDomain kAooNetMsgClient kAooNetMsgRequest

#define kAooNetMsgPeerJoin \
    kAooNetMsgPeer kAooNetMsgJoin

#define kAooNetMsgPeerLeave \
    kAooNetMsgPeer kAooNetMsgLeave

#define kAooNetMsgClientPeerJoin \
    kAooMsgDomain kAooNetMsgClient kAooNetMsgPeerJoin

#define kAooNetMsgClientPeerLeave \
    kAooMsgDomain kAooNetMsgClient kAooNetMsgPeerLeave

#define kAooNetMsgClientMessage \
    kAooMsgDomain kAooNetMsgClient kAooNetMsgMessage

namespace aoo {
namespace net {

//--------------------- group -------------------------//

user* group::add_user(user&& usr) {
    if (!find_user(usr.name())) {
        return &users_.emplace_back(std::move(usr));
    } else {
        return nullptr;
    }
}

user* group::find_user(const std::string& name) {
    for (auto& usr : users_) {
        if (usr.name() == name) {
            return &usr;
        }
    }
    return nullptr;
}

user* group::find_user(AooId id) {
    for (auto& usr : users_) {
        if (usr.id() == id) {
            return &usr;
        }
    }
    return nullptr;
}

user* group::find_user(const client_endpoint& client) {
    for (auto& usr : users_) {
        if (usr.client() == client.id()) {
            return &usr;
        }
    }
    return nullptr;
}

bool group::remove_user(AooId id) {
    for (auto it = users_.begin(); it != users_.end(); ++it) {
        if (it->id() == id) {
            users_.erase(it);
            return true;
        }
    }
    return false;
}

AooId group::get_next_user_id() {
#if 1
    // try to reclaim ID
    for (AooId i = 0; i < next_user_id_; ++i) {
        if (!find_user(i)) {
            return i;
        }
    }
#endif
    // make new ID
    return next_user_id_++;
}

//------------------- client_endpoint -------------------//

bool client_endpoint::match(const ip_address& addr) const {
    // match public UDP addresses!
    for (auto& a : public_addresses_) {
        if (a == addr){
            return true;
        }
    }
    return false;
}

void client_endpoint::send_message(const osc::OutboundPacketStream& msg) const {
    // prepend message size (int32_t)
    auto data = msg.Data() - 4;
    auto size = msg.Size() + 4;
    // we know that the buffer is not really constant
    aoo::to_bytes<int32_t>(msg.Size(), const_cast<char *>(data));
    if (replyfn_(context_, id_, (AooByte *)data, size) < 0) {
        // TODO handle error
    }
}

void client_endpoint::send_error(Server& server, AooId token, AooNetRequestType type,
                                 AooError result, int32_t errcode, const char *errmsg) {
    const char *pattern;
    switch (type) {
    case kAooNetRequestLogin:
        pattern = kAooNetMsgClientLogin;
        break;
    case kAooNetRequestGroupJoin:
        pattern = kAooNetMsgClientGroupJoin;
        break;
    case kAooNetRequestGroupLeave:
        pattern = kAooNetMsgClientGroupLeave;
        break;
    case kAooNetRequestCustom:
        pattern = kAooNetMsgClientRequest;
        break;
    default:
        // TODO bug
        return;
    }

    auto msg = server.start_message();

    msg << osc::BeginMessage(pattern) << (int32_t)token
        << (int32_t)result << (int32_t)errcode << errmsg << osc::EndMessage;

    send_message(msg);
}

void client_endpoint::send_notification(Server& server, const AooDataView &data) const {
    auto msg = server.start_message(data.size);

    msg << osc::BeginMessage(kAooNetMsgClientMessage)
        << osc::Blob(data.data, data.size) << osc::EndMessage;

    send_message(msg);
}

void client_endpoint::send_peer_add(Server& server, const group& grp, const user& usr,
                                    const client_endpoint& client) const {

    LOG_DEBUG("AooServer: send peer " << grp << "|" << usr << " " << client.public_addresses().front()
              << " to client " << id() << " " << public_addresses().front());

    auto msg = server.start_message(usr.metadata().size());

    msg << osc::BeginMessage(kAooNetMsgClientPeerJoin)
        << grp.name().c_str() << grp.id()
        << usr.name().c_str() << usr.id() << usr.metadata()
    // addresses
        << (int32_t)client.public_addresses().size();
    for (auto& addr : client.public_addresses()){
        msg << addr;
    }
    msg << usr.relay_addr() << osc::EndMessage;

    send_message(msg);
}

void client_endpoint::send_peer_remove(Server& server, const group& grp, const user& usr) const {
    LOG_DEBUG("AooServer: remove peer " << grp.name() << "|" << usr.name());

    auto msg = server.start_message();

    msg << osc::BeginMessage(kAooNetMsgClientPeerLeave)
        << grp.id() << usr.id() << osc::EndMessage;

    send_message(msg);
}

void client_endpoint::on_close(Server& server) {
    for (auto& gu : group_users_) {
        if (auto grp = server.find_group(gu.group)) {
            if (auto usr = grp->find_user(gu.user)) {
                server.on_user_left_group(*grp, *usr);
                server.do_remove_user_from_group(*grp, *usr);
            } else {
                LOG_ERROR("AooServer: client_endpoint::on_close: could not find user " << gu.user);
            }
        } else {
            LOG_ERROR("AooServer: client_endpoint::on_close: could not find group " << gu.group);
        }
    }
    group_users_.clear();
    id_ = kAooIdInvalid;
}

void client_endpoint::handle_message(Server &server, const AooByte *data, int32_t n) {
    receiver_.handle_message((const char *)data, n,
            [&](const osc::ReceivedPacket& packet) {
        osc::ReceivedMessage msg(packet);
        server.handle_message(*this, msg, packet.Size());
    });
}

void client_endpoint::on_group_join(const group& grp, const user& usr) {
#if 1
    for (auto& gu : group_users_) {
        if (gu.group == grp.id() && gu.user == usr.id()) {
            LOG_ERROR("AooServer: client_endpoint::add_group_user: "
                      << "group user already exists");
            return;
        }
    }
#endif
    group_users_.emplace_back(group_user { grp.id(), usr.id() });
}

void client_endpoint::on_group_leave(const group& grp, const user& usr, bool force) {
    if (force) {
        // TODO send message to user, so that it knows it has been removed!
    }
    for (auto it = group_users_.begin(); it != group_users_.end(); ++it) {
        if (it->group == grp.id() && it->user == usr.id()) {
            group_users_.erase(it);
            return;
        }
    }
    LOG_ERROR("AooServer: client_endpoint::remove_group_user(): "
              << "couldn't remove group user " << grp.id() << "|" << usr.id());
}

} // net
} // aoo

//----------------------- Server --------------------------//

AOO_API AooServer * AOO_CALL AooServer_new(
        AooFlag flags, AooError *err) {
    return aoo::construct<aoo::net::Server>();
}

aoo::net::Server::Server() {
    sendbuffer_.resize(AOO_MAX_PACKET_SIZE);
}

AOO_API void AOO_CALL AooServer_free(AooServer *server){
    // cast to correct type because base class
    // has no virtual destructor!
    aoo::destroy(static_cast<aoo::net::Server *>(server));
}

aoo::net::Server::~Server() {
    // JC: need to close all the clients sockets without
    // having them send anything out, so that active communication
    // between connected peers can continue if the server goes down for maintainence
}

AOO_API AooError AOO_CALL AooServer_handleUdpMessage(
        AooServer *server,
        const AooByte *data, AooInt32 size,
        const void *address, AooAddrSize addrlen,
        AooSendFunc replyFn, void *user) {
    return server->handleUdpMessage(data, size, address, addrlen,
                                    replyFn, user);
}

AooError AOO_CALL aoo::net::Server::handleUdpMessage(
        const AooByte *data, AooInt32 size,
        const void *address, AooAddrSize addrlen,
        AooSendFunc replyFn, void *user) {
    AooMsgType type;
    int32_t onset;
    auto err = parse_pattern(data, size, type, onset);
    if (err != kAooOk){
        LOG_WARNING("AooServer: not an AOO NET message!");
        return kAooErrorUnknown;
    }

    ip_address addr((const struct sockaddr *)address, addrlen);
    sendfn fn(replyFn, user);

    if (type == kAooTypeServer){
        return handle_udp_message(data, size, onset, addr, fn);
    } else if (type == kAooTypeRelay){
        return handle_relay(data, size, addr, fn);
    } else {
        LOG_WARNING("AooServer: not a client message!");
        return kAooErrorUnknown;
    }
}

AOO_API AooError AOO_CALL AooServer_addClient(
        AooServer *server, AooNetReplyFunc replyFn,
        void *user, AooSocket sockfd, AooId *id) {
    return server->addClient(replyFn, user, sockfd, id);
}

AooError AOO_CALL aoo::net::Server::addClient(
        AooNetReplyFunc replyFn, void *user, AooSocket sockfd, AooId *clientId) {
    // NB: we don't protect against duplicate clients;
    // the user is supposed to only call this when a new client
    // socket is accepted.
    auto id = get_next_client_id();
    clients_.emplace(id, client_endpoint(sockfd, id, replyFn, user));
    if (clientId) {
        *clientId = id;
    }
    LOG_DEBUG("AooServer: add client " << id);
    return kAooOk;
}

AOO_API AooError AOO_CALL AooServer_removeClient(AooServer *server, AooId clientId) {
    return server->removeClient(clientId);
}

AooError AOO_CALL aoo::net::Server::removeClient(AooId clientId) {
    if (remove_client(clientId)) {
        LOG_DEBUG("AooServer: remove client " << clientId);
        return kAooOk;
    } else {
        LOG_ERROR("AooServer: removeClient: client " << clientId << " not found");
        return kAooErrorUnknown;
    }
}

AOO_API AooError AOO_CALL AooServer_handleClientMessage(
        AooServer *server, AooId client, const AooByte *data, AooInt32 size) {
    return server->handleClientMessage(client, data, size);
}

AooError AOO_CALL aoo::net::Server::handleClientMessage(
        AooId clientId, const AooByte *data, AooInt32 size) {
    auto client = find_client(clientId);
    if (!client) {
        LOG_ERROR("AooServer: handleClientMessage: can't find client " << clientId);
        return kAooErrorUnknown;
    }
    try {
        client->handle_message(*this, data, size);
    } catch (const std::exception& e) {
        LOG_ERROR("AooServer: could not handle client message: " << e.what());
        return kAooErrorUnknown;
    }

    return kAooOk;
}

AOO_API AooError AOO_CALL AooServer_setRequestHandler(
        AooServer *server, AooNetRequestHandler cb, void *user, AooFlag flags) {
    return server->setRequestHandler(cb, user, flags);
}

AooError AOO_CALL aoo::net::Server::setRequestHandler(
        AooNetRequestHandler cb, void *user, AooFlag flags) {
    request_handler_ = cb;
    request_context_ = user;
    return kAooOk;
}

AOO_API AooError AooServer_acceptRequest(
        AooServer *server, AooId client, AooId token,
        const AooNetRequest *request, AooNetResponse *response)
{
    return server->acceptRequest(client, token, request, response);
}

AooError AOO_CALL aoo::net::Server::acceptRequest(
        AooId client, AooId token, const AooNetRequest *request,
        AooNetResponse *response)
{
    // currently all requests that can be intercepted require a response
    if (!request || !response) {
        return kAooErrorBadArgument;
    }
    // just make sure that request and response match up
    if (response->type != request->type) {
        return kAooErrorBadArgument;
    }
    // query request needs to handled specially
    if (request->type == kAooNetRequestQuery) {
        do_query((const AooNetRequestQuery&)request,
                 (AooNetResponseQuery&)response);
        return kAooOk; // always succeeds
    }
    // client requests
    auto c = find_client(client);
    if (c) {
        switch (request->type) {
        case kAooNetRequestLogin:
            return do_login(*c, token, (const AooNetRequestLogin&)*request,
                            (AooNetResponseLogin&)*response);
        case kAooNetRequestGroupJoin:
            return do_group_join(*c, token, (const AooNetRequestGroupJoin&)*request,
                                 (AooNetResponseGroupJoin&)*response);
        case kAooNetRequestGroupLeave:
            return do_group_leave(*c, token, (const AooNetRequestGroupLeave&)*request,
                                  (AooNetResponseGroupLeave&)*response);
            break;
        case kAooNetRequestCustom:
            return do_custom_request(*c, token, (const AooNetRequestCustom&)*request,
                                     (AooNetResponseCustom&)*response);
        default:
            return kAooErrorUnknown;
        }
    } else {
        return kAooErrorUnknown;
    }
}

AOO_API AooError AooServer_declineRequest(
        AooServer *server,
        AooId client, AooId token, const AooNetRequest *request,
        AooError errorCode, const AooChar *errorMessage)
{
    return server->declineRequest(client, token, request, errorCode, errorMessage);
}

AooError AOO_CALL aoo::net::Server::declineRequest(
        AooId client, AooId token, const AooNetRequest *request,
        AooError errorCode, const AooChar *errorMessage)
{
    if (!request) {
        return kAooErrorBadArgument;
    }
    // query request needs to handled specially
    if (request->type == kAooNetRequestQuery) {
        // for now, just ignore request
        // TODO think about error response
        return kAooOk;
    }
    // client requests
    auto c = find_client(client);
    if (c) {
        // LATER find a dedicated error code for declined requests
        c->send_error(*this, token, request->type,
                      kAooErrorUnknown, errorCode, errorMessage);
        return kAooOk;
    } else {
        return kAooErrorUnknown;
    }
}

AOO_API AooError AOO_CALL AooServer_notifyClient(
        AooServer *server, AooId client, const AooDataView *data) {
    return  server->notifyClient(client, data);
}

AooError AOO_CALL aoo::net::Server::notifyClient(
        AooId client, const AooDataView *data) {
    if (auto c = find_client(client)) {
        c->send_notification(*this, *data);
        return kAooOk;
    } else {
        LOG_ERROR("AooServer: notifyClient: can't find client!");
        return kAooErrorUnknown;
    }
}

AOO_API AooError AOO_CALL AooServer_notifyGroup(
        AooServer *server, AooId group, AooId user,
        const AooDataView *data) {
    return server->notifyGroup(group, user, data);
}

AooError AOO_CALL aoo::net::Server::notifyGroup(
        AooId group, AooId user, const AooDataView *data) {
    if (auto g = find_group(group)) {
        if (user == kAooIdInvalid) {
            // all users
            for (auto& u : g->users()) {
                if (auto c = find_client(u)) {
                    c->send_notification(*this, *data);
                    return kAooOk;
                } else {
                    LOG_ERROR("AooServer: notifyGroup: can't find client for user " << u);
                }
            }
            return kAooOk;
        } else {
            // single user
            if (auto u = g->find_user(user)) {
                if (auto c = find_client(*u)) {
                    c->send_notification(*this, *data);
                    return kAooOk;
                } else {
                    LOG_ERROR("AooServer: notifyGroup: can't find client for user " << *u);
                }
            } else {
                LOG_ERROR("AooServer::notifyGroup: can't find user " << user);
            }
        }
    } else {
        LOG_ERROR("AooServer: notifyGroup: can't find group " << group);
    }
    return kAooErrorUnknown;
}

AOO_API AooError AOO_CALL AooServer_findGroup(
        AooServer *server, const AooChar *name, AooId *id) {
    return server->findGroup(name, id);
}

AooError AOO_CALL aoo::net::Server::findGroup(
        const AooChar *name, AooId *id) {
    if (auto grp = find_group(name)) {
        if (id) {
            *id = grp->id();
        }
        return kAooOk;
    } else {
        return kAooErrorUnknown;
    }
}

AOO_API AooError AOO_CALL AooServer_addGroup(
        AooServer *server, const AooChar *name, const AooChar *password,
        const AooDataView *metadata, const AooIpEndpoint *relayAddress, AooFlag flags, AooId *groupId) {
    return server->addGroup(name, password, metadata, relayAddress, flags, groupId);
}

AooError AOO_CALL aoo::net::Server::addGroup(
        const AooChar *name, const AooChar *password, const AooDataView *metadata,
        const AooIpEndpoint *relayAddress, AooFlag flags, AooId *groupId) {
    // this might "waste" a group ID, but we don't care.
    auto id = get_next_group_id();
    std::string hashed_pwd = password ? aoo::net::encrypt(password) : "";
    auto relay_addr = relayAddress ? ip_host { relayAddress->hostName, relayAddress->port }
                                   : ip_host {};
    if (add_group(group(name, hashed_pwd, id, metadata, relay_addr, true))) {
        if (groupId) {
            *groupId = id;
        }
        return kAooOk;
    } else {
        LOG_ERROR("AooServer: addGroup: group " << name << " already exists");
    }
    return kAooErrorUnknown;
}

AOO_API AooError AOO_CALL AooServer_removeGroup(
        AooServer *server, AooId group) {
    return server->removeGroup(group);
}

AooError AOO_CALL aoo::net::Server::removeGroup(AooId group) {
    if (remove_group(group)) {
        return kAooOk;
    } else {
        LOG_ERROR("AooServer: removeGroup: group " << group << " not found");
        return kAooErrorUnknown;
    }
}

AOO_API AooError AOO_CALL AooServer_findUserInGroup(
        AooServer *server, AooId group, const AooChar *userName, AooId *userId) {
    return server->findUserInGroup(group, userName, userId);
}

AooError AOO_CALL aoo::net::Server::findUserInGroup(
        AooId group, const AooChar *userName, AooId *userId) {
    if (auto grp = find_group(group)) {
        if (auto usr = grp->find_user(userName)) {
            if (userId) {
                *userId = usr->id();
            }
            return kAooOk;
        }
    }
    return kAooErrorUnknown;
}

AOO_API AooError AOO_CALL AooServer_addUserToGroup(
        AooServer *server, AooId group,
        const AooChar *userName, const AooChar *userPwd,
        const AooDataView *metadata, AooFlag flags, AooId *userId) {
    return server->addUserToGroup(group, userName, userPwd, metadata, flags, userId);
}

AooError AOO_CALL aoo::net::Server::addUserToGroup(
        AooId group, const AooChar *userName, const AooChar *userPwd,
        const AooDataView *metadata, AooFlag flags, AooId *userId) {
    if (auto g = find_group(group)) {
        auto id = g->get_next_user_id();
        std::string hashed_pwd = userPwd ? aoo::net::encrypt(userPwd) : "";
        if (auto u = g->add_user(user(userName, hashed_pwd, id, g->id(), kAooIdInvalid,
                                      metadata, ip_host{}, true))) {
            if (userId) {
                *userId = u->id();
            }
            return kAooOk;
        } else {
            LOG_ERROR("AooServer: addUserToGroup: user " << userName
                      << " already exists in group " << group);
        }
    } else {
        LOG_ERROR("AooServer: addUserToGroup: group " << group << " not found");
    }
    return kAooErrorUnknown;
}

AOO_API AooError AOO_CALL AooServer_removeUserFromGroup(
        AooServer *server, AooId group, AooId user) {
    return server->removeUserFromGroup(group, user);
}

AooError AOO_CALL aoo::net::Server::removeUserFromGroup(
        AooId group, AooId user) {
    if (auto grp = find_group(group)) {
        if (auto usr = grp->find_user(user)) {
            if (usr->active()) {
                // tell client that it has been kicked out of the group
                if (auto client = find_client(*usr)) {
                    client->on_group_leave(*grp, *usr, true);
                } else {
                    LOG_ERROR("AooServer: removeUserFromGroup: can't find client for user " << *usr);
                }
                // notify peers
                on_user_left_group(*grp, *usr);
            }
            do_remove_user_from_group(*grp, *usr);
            return kAooOk;
        } else {
            LOG_ERROR("AooServer: removeUserToGroup: user "
                      << user << " not found  in group " << group);
        }
    } else {
        LOG_ERROR("AooServer: removeUserToGroup: group " << group << " not found");
    }
    return kAooErrorUnknown;
}

AOO_API AooError AOO_CALL AooServer_groupControl(
        AooServer *server, AooId group, AooCtl ctl,
        AooIntPtr index, void *data, AooSize size) {
    return server->groupControl(group, ctl, index, data, size);
}

AooError AOO_CALL aoo::net::Server ::groupControl(
        AooId group, AooCtl ctl, AooIntPtr index,
        void *data, AooSize size) {
    LOG_WARNING("AooServer: unsupported group control " << ctl);
    return kAooErrorNotImplemented;
}

AOO_API AooError AOO_CALL AooServer_update(AooServer *server) {
    return server->update();
}

AooError AOO_CALL aoo::net::Server::update() {
    // LATER
    return kAooOk;
}

AOO_API AooError AOO_CALL AooServer_setEventHandler(
        AooServer *server, AooEventHandler fn, void *user, AooEventMode mode) {
    return server->setEventHandler(fn, user, mode);
}

AooError AOO_CALL aoo::net::Server::setEventHandler(
        AooEventHandler fn, void *user, AooEventMode mode) {
    eventhandler_ = fn;
    eventcontext_ = user;
    eventmode_ = mode;
    return kAooOk;
}

AOO_API AooBool AOO_CALL AooServer_eventsAvailable(AooServer *server) {
    return server->eventsAvailable();
}

AooBool AOO_CALL aoo::net::Server::eventsAvailable(){
    return !events_.empty();
}

AOO_API AooError AOO_CALL AooServer_pollEvents(AooServer *server) {
    return server->pollEvents();
}

AooError AOO_CALL aoo::net::Server::pollEvents(){
    // always thread-safe
    event_handler fn(eventhandler_, eventcontext_, kAooThreadLevelUnknown);
    event_ptr e;
    while (events_.try_pop(e)){
        e->dispatch(fn);
    }
    return kAooOk;
}

template<typename T>
T& as(void *p){
    return *reinterpret_cast<T *>(p);
}

#define CHECKARG(type) assert(size == sizeof(type))

AOO_API AooError AOO_CALL AooServer_control(
        AooServer *server, AooCtl ctl,
        AooIntPtr index, void *data, AooSize size)
{
    return server->control(ctl, index, data, size);
}

AooError AOO_CALL aoo::net::Server::control(
        AooCtl ctl, AooIntPtr index, void *ptr, AooSize size)
{
    switch (ctl) {
    case kAooCtlSetPassword:
    {
        CHECKARG(AooChar *);
        auto pwd = as<AooChar*>(ptr);
        if (pwd) {
            password_ = encrypt(pwd);
        } else {
            password_ = "";
        }
        break;
    }
    case kAooCtlSetTcpHost:
    {
        CHECKARG(AooIpEndpoint*);
        auto ep = as<AooIpEndpoint*>(ptr);
        if (ep) {
            tcp_addr_ = ip_host(*ep);
        } else {
            tcp_addr_ = ip_host{};
        }
        break;
    }
    case kAooCtlSetRelayHost:
    {
        CHECKARG(AooIpEndpoint*);
        auto ep = as<AooIpEndpoint*>(ptr);
        if (ep) {
            relay_addr_ = ip_host(*ep);
        } else {
            relay_addr_ = ip_host{};
        }
        break;
    }
    case kAooCtlSetServerRelay:
        CHECKARG(AooBool);
        allow_relay_.store(as<AooBool>(ptr));
        break;
    case kAooCtlGetServerRelay:
        CHECKARG(AooBool);
        as<AooBool>(ptr) = allow_relay_.load();
        break;
    case kAooCtlSetGroupAutoCreate:
        CHECKARG(AooBool);
        group_auto_create_.store(as<AooBool>(ptr));
        break;
    case kAooCtlGetGroupAutoCreate:
        CHECKARG(AooBool);
        as<AooBool>(ptr) = group_auto_create_.load();
        break;
    default:
        LOG_WARNING("AooServer: unsupported control " << ctl);
        return kAooErrorNotImplemented;
    }
    return kAooOk;
}

namespace aoo {
namespace net {

client_endpoint * Server::find_client(AooId id) {
    auto it = clients_.find(id);
    if (it != clients_.end()) {
        return &it->second;
    } else {
        return nullptr;
    }
}

client_endpoint * Server::find_client(const ip_address& addr) {
    for (auto& client : clients_) {
        if (client.second.match(addr)) {
            return &client.second;
        }
    }
    return nullptr;
}

bool Server::remove_client(AooId id) {
    auto it = clients_.find(id);
    if (it == clients_.end()) {
        return false;
    }
    it->second.on_close(*this);
    clients_.erase(it);

    auto e = std::make_unique<client_remove_event>(id);
    send_event(std::move(e));

    return true;
}

group* Server::add_group(group&& grp) {
    if (!find_group(grp.name())) {
        auto result = groups_.emplace(grp.id(), std::move(grp));
        if (result.second) {
            return &result.first->second;
        }
    }
    return nullptr;
}

group* Server::find_group(AooId id) {
    auto it = groups_.find(id);
    if (it != groups_.end()) {
        return &it->second;
    } else {
        return nullptr;
    }
}

group* Server::find_group(const std::string& name) {
    for (auto& grp : groups_) {
        if (grp.second.name() == name) {
            return &grp.second;
        }
    }
    return nullptr;
}

bool Server::remove_group(AooId id) {
    auto it = groups_.find(id);
    if (it == groups_.end()) {
        return false;
    }
    // if the group has been removed manually, it might still
    // contain users, so we have to notify them!
    auto& grp = it->second;
    for (auto& usr : grp.users()) {
        if (auto client = find_client(usr)) {
            client->on_group_leave(grp, usr, true);
        } else {
            LOG_ERROR("AooServer: remove_group: can't find client for user " << usr);
        }
    }
    groups_.erase(it);
    return true;
}

void Server::on_user_joined_group(const group& grp, const user& usr,
                                  const client_endpoint& client) {
    LOG_DEBUG("AooServer: user " << usr << " joined group " << grp);
    // 1) send the new member to existing group members
    // 2) send existing group members to the new member
    for (auto& peer : grp.users()) {
        if ((peer.id() != usr.id()) && peer.active()) {
            if (auto other = find_client(peer)) {
                // notify new member
                client.send_peer_add(*this, grp, peer, *other);
                // notify existing member
                other->send_peer_add(*this, grp, usr, client);
            } else {
                LOG_ERROR("AooServer: user_joined_group: can't find client for peer " << peer);
            }
        }
    }

    // send event
    auto e = std::make_unique<group_join_event>(grp, usr);
    send_event(std::move(e));
}

void Server::on_user_left_group(const group& grp, const user& usr) {
    LOG_DEBUG("AooServer: user " << usr << " left group " << grp);
    // notify peers
    for (auto& peer : grp.users()) {
        if (peer.id() != usr.id()) {
            if (auto other = find_client(peer)) {
                other->send_peer_remove(*this, grp, usr);
            } else {
                LOG_ERROR("AooServer: user_left_group: can't find client for peer " << peer);
            }
        }
    }

    // send event
    auto e = std::make_unique<group_leave_event>(grp, usr);
    send_event(std::move(e));
}

void Server::do_remove_user_from_group(group& grp, user& usr) {
    if (usr.persistent()) {
        // just unset
        usr.unset();
    } else {
        // remove from group
        if (!grp.remove_user(usr.id())) {
            LOG_ERROR("AooServer: can't remove user " << usr << " from group " << grp);
        }
        // remove group if empty and not persistent
        if (!grp.persistent() && !grp.user_count()) {
            // send event
            auto e = std::make_unique<group_remove_event>(grp);
            send_event(std::move(e));

            // finally remove it
            remove_group(grp.id());
        }
    }
}

osc::OutboundPacketStream Server::start_message(size_t extra_size) {
    if (extra_size > 0) {
        auto total = AOO_MAX_PACKET_SIZE + extra_size;
        if (sendbuffer_.size() < total) {
            sendbuffer_.resize(total);
        }
    }
    // leave space for message size (int32_t)
    return osc::OutboundPacketStream(sendbuffer_.data() + 4, sendbuffer_.size() - 4);
}

void Server::handle_ping(const client_endpoint& client,
                         const osc::ReceivedMessage& msg) {
    // send reply
    auto reply = start_message();

    reply << osc::BeginMessage(kAooNetMsgClientPingReply) << osc::EndMessage;

    client.send_message(reply);
}

void Server::handle_message(client_endpoint& client,
                            const osc::ReceivedMessage& msg, int32_t size) {
    AooMsgType type;
    int32_t onset;
    auto err = parse_pattern((const AooByte *)msg.AddressPattern(), size, type, onset);
    if (err != kAooOk){
        throw std::runtime_error("AOO NET message!");
    }

    try {
        if (type == kAooTypeServer){
            auto pattern = msg.AddressPattern() + onset;
            LOG_DEBUG("AooServer: got server message " << pattern);
            if (!strcmp(pattern, kAooNetMsgPing)){
                handle_ping(client, msg);
            } else if (!strcmp(pattern, kAooNetMsgLogin)){
                handle_login(client, msg);
            } else if (!strcmp(pattern, kAooNetMsgGroupJoin)){
                handle_group_join(client, msg);
            } else if (!strcmp(pattern, kAooNetMsgGroupLeave)){
                handle_group_leave(client, msg);
            } else {
                throw std::runtime_error("unknown server message " + std::string(pattern));
            }
        } else {
            throw std::runtime_error("AooServer: got unexpected message " + std::string(msg.AddressPattern()));
        }
    } catch (const osc::Exception& e) {
        std::stringstream ss;
        ss << "exception while handling " << msg.AddressPattern() << " message: " << e.what();
        throw std::runtime_error(ss.str());
    }
}

void Server::handle_login(client_endpoint& client, const osc::ReceivedMessage& msg)
{
    auto it = msg.ArgumentsBegin();
    auto token = (AooId)(it++)->AsInt32();
    auto version = (uint32_t)(it++)->AsInt32();
    auto pwd = (it++)->AsString();
    auto metadata = osc_read_metadata(it);
    // IP addresses
    auto count = (it++)->AsInt32();
    for (int i = 0; i < count; ++i) {
        client.add_public_address(osc_read_address(it));
    }

    AooNetRequestLogin request;
    request.type = kAooNetRequestLogin;
    request.flags = 0;
    request.password = pwd;
    request.metadata = metadata.data ? &metadata : nullptr;
    // check version
    if (!check_version(version)) {
        LOG_DEBUG("AooServer: client " << client.id() << ": version mismatch");
        client.send_error(*this, token, request.type, kAooErrorUnknown,
                          0, "AOO version not supported");
        return;
    }
    // check password
    if (!password_.empty() && pwd != password_) {
        LOG_DEBUG("AooServer: client " << client.id() << ": wrong password");
        client.send_error(*this, token, request.type, kAooErrorUnknown,
                          0, "wrong password");
        return;
    }

    if (!handle_request(client, token, (AooNetRequest&)request)) {
        AooNetResponseLogin response;
        response.type = kAooNetRequestLogin;
        response.flags = 0;
        response.metadata = nullptr;

        do_login(client, token, request, response);
    }
}

AooError Server::do_login(client_endpoint& client, AooId token,
                          const AooNetRequestLogin& request,
                          AooNetResponseLogin& response) {
    // flags
    AooFlag flags = 0;
    if (allow_relay_.load()) {
        flags |= kAooNetServerRelay;
    }

    // send reply
    auto extra = response.metadata ? response.metadata->size : 0;
    auto msg = start_message(extra);

    msg << osc::BeginMessage(kAooNetMsgClientLogin)
        << token << (int32_t)kAooOk
        << client.id() << (int32_t)flags << response.metadata
        << osc::EndMessage;

    client.send_message(msg);

    auto e = std::make_unique<client_login_event>(client);
    send_event(std::move(e));

    return kAooOk;
}

void Server::handle_group_join(client_endpoint& client, const osc::ReceivedMessage& msg)
{
    auto it = msg.ArgumentsBegin();
    auto token = (AooId)(it++)->AsInt32();
    auto group_name = (it++)->AsString();
    auto group_pwd = (it++)->AsString();
    auto group_md = osc_read_metadata(it);
    auto user_name = (it++)->AsString();
    auto user_pwd = (it++)->AsString();
    auto user_md = osc_read_metadata(it);
    AooIpEndpoint relay;
    relay.hostName = (it++)->AsString();
    relay.port = (it++)->AsInt32();

    AooNetRequestGroupJoin request;
    request.type = kAooNetRequestGroupJoin;
    request.flags = 0;
    request.groupName = group_name;
    request.groupPwd = group_pwd;
    request.groupId = kAooIdInvalid; // set later
    request.groupFlags = 0;
    request.groupMetadata = group_md.size ? &group_md : nullptr;
    request.userName = user_name;
    request.userPwd = user_pwd;
    request.userId = kAooIdInvalid; // set later
    request.userFlags = 0;
    request.userMetadata = user_md.size ? &user_md : nullptr;
    request.relayAddress = relay.port > 0 ? &relay : nullptr;

    auto grp = find_group(request.groupName);
    user *usr = nullptr;
    if (grp) {
        request.groupId = grp->id();
        // check group password
        if (!grp->check_pwd(request.groupPwd)) {
            client.send_error(*this, token, request.type, kAooErrorUnknown,
                              0, "wrong group password");
            return;
        }
        usr = grp->find_user(request.userName);
        if (usr) {
            request.userId = usr->id();
            // check if someone is already logged in
            if (usr->active()) {
                client.send_error(*this, token, request.type, kAooErrorUnknown,
                                  0, "user already logged in");
                return;
            }
            // check user password
            if (!usr->check_pwd(request.userPwd)) {
                client.send_error(*this, token, request.type, kAooErrorUnknown,
                                  0, "wrong user password");
                return;
            }
        } else {
            // user needs to be created
            request.userId = kAooIdInvalid;
            // check if the client is allowed to create users
            if (!grp->user_auto_create()) {
                client.send_error(*this, token, request.type, kAooErrorUnknown,
                                  0, "not allowed to create user");
                return;
            }
        }
    } else {
        // group needs to be created
        request.groupId = kAooIdInvalid;
        // check if the client is allowed to create groups
        if (!group_auto_create_.load()) {
            client.send_error(*this, token, request.type, kAooErrorUnknown,
                              0, "not allowed to create group");
            return;
        }
    }

    if (!handle_request(client, token, (AooNetRequest&)request)) {
        AooNetResponseGroupJoin response;
        response.type = kAooNetRequestGroupJoin;
        response.flags = 0;
        response.groupId = 0;
        response.groupFlags = 0;
        response.groupMetadata = nullptr;
        response.userId = 0;
        response.userFlags = 0;
        response.userMetadata = nullptr;
        response.privateMetadata = nullptr;
        response.relayAddress = nullptr;

        do_group_join(client, token, request, response);
    }
}

AooError Server::do_group_join(client_endpoint &client, AooId token,
                               const AooNetRequestGroupJoin& request,
                               AooNetResponseGroupJoin& response) {
    // prefer response group metadata
    auto group_md = response.groupMetadata ? response.groupMetadata : request.groupMetadata;
    // prefer group relay address (set in request handler) over server relay
    auto group_relay = response.relayAddress ? ip_host(*response.relayAddress) : relay_addr_;
    // find group or create it if necessary
    auto grp = find_group(request.groupId);
    if (!grp) {
        grp = add_group(group(request.groupName, request.groupPwd, get_next_group_id(),
                              group_md, group_relay, false));
        if (grp) {
            // send event
            auto e = std::make_unique<group_add_event>(*grp);
            send_event(std::move(e));
        } else {
            // group has been added in the meantime... LATER try to deal with this
            client.send_error(*this, token, request.type, kAooErrorUnknown,
                              0, "could not create group: already exists");
            return kAooErrorUnknown;
        }
    }
    // prefer response user metadata
    auto user_md = response.userMetadata ? response.userMetadata : request.userMetadata;
    // (optional) user provided relay address
    auto user_relay = request.relayAddress ? ip_host(*request.relayAddress) : ip_host{};
    // find user or create it if necessary
    auto usr = grp->find_user(request.userId);
    if (!usr) {
        auto id = grp->get_next_user_id();
        usr = grp->add_user(user(request.userName, request.userPwd, id, grp->id(),
                                 client.id(), user_md, user_relay, false));
        if (!usr) {
            // user has been added in the meantime... LATER try to deal with this
            client.send_error(*this, token, request.type, kAooErrorUnknown,
                              0, "could not create user: already exists");
            return kAooErrorUnknown;
        }
    }
    // update response
    response.groupId = grp->id();
    response.userId = usr->id();

    client.on_group_join(*grp, *usr);

    // send reply
    auto extra = grp->metadata().size() + usr->metadata().size() +
            (response.privateMetadata ? response.privateMetadata->size : 0);
    auto msg = start_message(extra);

    msg << osc::BeginMessage(kAooNetMsgClientGroupJoin)
        << token << (int32_t)kAooOk
        << grp->id() << usr->id()
        << grp->metadata() << usr->metadata()
        << response.privateMetadata << group_relay // *not* group->relay()!
        << osc::EndMessage;

    client.send_message(msg);

    // after reply!
    on_user_joined_group(*grp, *usr, client);

    return kAooOk; // success
}

void Server::handle_group_leave(client_endpoint& client, const osc::ReceivedMessage& msg){
    auto it = msg.ArgumentsBegin();
    auto token = (AooId)(it++)->AsInt32();
    auto group = (it++)->AsInt32();

    AooNetRequestGroupLeave request;
    request.type = kAooNetRequestGroupLeave;
    request.group = group;

    AooNetResponseGroupLeave response;
    response.type = kAooNetRequestGroupLeave;

    if (!handle_request(client, token, (AooNetRequest&)request)) {
        do_group_leave(client, token, request, response);
    }
}

AooError Server::do_group_leave(client_endpoint& client, AooId token,
                                const AooNetRequestGroupLeave& request,
                                AooNetResponseGroupLeave& response) {
    const char *errmsg;

    if (auto grp = find_group(request.group)) {
        // find the user in the group that is associated with this client
        // LATER we might move this logic to the client side and send the
        // user ID to the server.
        if (auto usr = grp->find_user(client)) {
            on_user_left_group(*grp, *usr);

            client.on_group_leave(*grp, *usr, false);

            do_remove_user_from_group(*grp, *usr);

            // send reply
            auto msg = start_message();

            msg << osc::BeginMessage(kAooNetMsgClientGroupLeave)
                << token << (int32_t)kAooOk << osc::EndMessage;

            client.send_message(msg);

            return kAooOk;
        } else {
            errmsg = "client is not a group member";
        }
    } else {
        errmsg = "can't find group";
    }
    client.send_error(*this, token, request.type, kAooErrorUnknown, 0, errmsg);

    return kAooErrorUnknown;
}

void Server::handle_custom_request(client_endpoint& client, const osc::ReceivedMessage& msg) {
    auto it = msg.ArgumentsBegin();
    auto token = (AooId)(it++)->AsInt32();
    auto flags = (it++)->AsInt32();
    auto data = osc_read_metadata(it);

    AooNetRequestCustom request;
    request.type = kAooNetRequestCustom;
    request.flags = flags;
    request.data = data;

    if (!handle_request(client, token, (AooNetRequest&)request)) {
        // requests must be handled by the user!
        client.send_error(*this, token, request.type, kAooErrorUnknown,
                          0, "request not handled");
    }
}

AooError Server::do_custom_request(client_endpoint& client, AooId token,
                                   const AooNetRequestCustom& request,
                                   AooNetResponseCustom& response) {
    // send reply
    auto msg = start_message(response.data.size);

    msg << osc::BeginMessage(kAooNetMsgClientGroupLeave)
        << token << (int32_t)kAooOk << (int32_t)response.flags
        << &response.data << osc::EndMessage;

    client.send_message(msg);

    return kAooOk;
}

AooError Server::handle_udp_message(const AooByte *data, AooSize size, int onset,
                                const ip_address& addr, const sendfn& fn) {
    if (binmsg_check(data, size)) {
        LOG_WARNING("AooServer: unsupported binary message");
        return kAooErrorUnknown;
    }
    // OSC format
    try {
        osc::ReceivedPacket packet((const char *)data, size);
        osc::ReceivedMessage msg(packet);

        auto pattern = msg.AddressPattern() + onset;
        LOG_DEBUG("AooServer: handle client UDP message " << pattern);

        if (!strcmp(pattern, kAooNetMsgPing)){
            handle_ping(msg, addr, fn);
        } else if (!strcmp(pattern, kAooNetMsgQuery)) {
            handle_query(msg, addr, fn);
        } else {
            LOG_ERROR("AooServer: unknown message " << pattern);
            return kAooErrorUnknown;
        }
        return kAooOk;
    } catch (const osc::Exception& e){
        auto pattern = (const char *)data + onset;
        LOG_ERROR("AooServer: exception on handling " << pattern
                  << " message: " << e.what());
        return kAooErrorUnknown;
    }
}

AooError Server::handle_relay(const AooByte *data, AooSize size,
                              const ip_address& addr, const sendfn& fn) const {
    if (!allow_relay_.load()) {
    #if AOO_DEBUG_RELAY
        LOG_DEBUG("AooServer: ignore relay message from " << addr);
    #endif
        return kAooOk; // ?
    }

    if (binmsg_check(data, size)) {
        // --- binary format ---
        ip_address dst;
        auto onset = binmsg_read_relay(data, size, dst);
        if (onset > 0) {
        #if AOO_DEBUG_RELAY
            LOG_DEBUG("AooServer: forward binary relay message from " << addr << " to " << dst);
        #endif
            if (addr.type() == dst.type()) {
                // simply replace the header (= rewrite address)
                binmsg_write_relay(const_cast<AooByte *>(data), size, addr);
                fn(data, size, dst);
                return kAooOk;
            } else {
                // rewrite whole message
                AooByte buf[AOO_MAX_PACKET_SIZE];
                auto result = write_relay_message(buf, sizeof(buf), data, size, addr);
                if (result > 0) {
                    fn(buf, result + size, dst);
                    return kAooOk;
                } else {
                    LOG_ERROR("AooServer: can't relay: buffer too small");
                }
            }
        } else {
            LOG_ERROR("AooServer: bad relay message");
        }
        return kAooErrorUnknown;
    } else {
        // --- OSC format ---
        try {
            osc::ReceivedPacket packet((const char *)data, size);
            osc::ReceivedMessage msg(packet);

            auto it = msg.ArgumentsBegin();
            // infer the address type from the sender.
            // E.g. the destination address might be IPv4,
            // but our UDP socket is IPv6, so it would need
            // to convert the address to IPv4-mapped.
            auto dst = osc_read_address(it, addr.type());

            const void *msgData;
            osc::osc_bundle_element_size_t msgSize;
            (it++)->AsBlob(msgData, msgSize);

            // don't prepend size for UDP message!
            char buf[AOO_MAX_PACKET_SIZE];
            osc::OutboundPacketStream out(buf, sizeof(buf));
            out << osc::BeginMessage(kAooMsgDomain kAooNetMsgRelay)
                << addr << osc::Blob(msgData, msgSize)
                << osc::EndMessage;

        #if AOO_DEBUG_RELAY
            LOG_DEBUG("AooServer: forward OSC relay message from " << addr << " to " << dst);
        #endif
            fn((const AooByte *)out.Data(), out.Size(), dst);

            return kAooOk;
        } catch (const osc::Exception& e){
            LOG_ERROR("AooServer: exception in handle_relay: " << e.what());
            return kAooErrorUnknown;
        }
    }
}

void Server::handle_ping(const osc::ReceivedMessage& msg,
                         const ip_address& addr, const sendfn& fn) const {
    // reply with /ping message
    // NB: don't prepend size for UDP message!
    char buf[512];
    osc::OutboundPacketStream reply(buf, sizeof(buf));
    reply << osc::BeginMessage(kAooNetMsgClientPingReply)
          << osc::EndMessage;

    fn((const AooByte *)reply.Data(), reply.Size(), addr);
}

void Server::handle_query(const osc::ReceivedMessage& msg,
                          const ip_address& addr, const sendfn& fn) {
    AooNetRequestQuery request;
    request.type = kAooNetRequestQuery;
    request.flags = 0;
    request.replyFunc = fn.fn();
    request.replyContext = fn.user();
    request.replyAddr.data = addr.address();
    request.replyAddr.size = addr.length();
    if (!handle_request((AooNetRequest&)request)) {
        AooNetResponseQuery response;
        response.type = kAooNetRequestQuery;
        response.flags = 0;
        response.serverAddress.hostName = nullptr;
        response.serverAddress.port = 0;
        do_query(request, response);
    }
}

void Server::do_query(const AooNetRequestQuery& request,
                      const AooNetResponseQuery& response) const {
    sendfn fn(request.replyFunc, request.replyContext);
    ip_address public_addr((const struct sockaddr *)request.replyAddr.data,
                           request.replyAddr.size);
    auto server_addr = response.serverAddress.hostName ?
                ip_host(response.serverAddress) : tcp_addr_;

    // do not prepend size for UDP message!
    char buf[AOO_MAX_PACKET_SIZE];
    osc::OutboundPacketStream reply(buf, sizeof(buf));
    reply << osc::BeginMessage(kAooNetMsgClientQuery)
          << public_addr << server_addr << osc::EndMessage;

    fn((const AooByte *)reply.Data(), reply.Size(), public_addr);
}

AooId Server::get_next_client_id(){
    // LATER make random group ID
    return next_client_id_++;
}

AooId Server::get_next_group_id(){
    // LATER make random group ID
    return next_group_id_++;
}

void Server::send_event(event_ptr e) {
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

} // net
} // aoo
