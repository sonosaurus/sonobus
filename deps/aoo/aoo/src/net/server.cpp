/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "server.hpp"

#include "aoo/aoo.h"

#include <functional>
#include <algorithm>
#include <iostream>

#define AOO_NET_MSG_CLIENT_PING \
    AOO_MSG_DOMAIN AOO_NET_MSG_CLIENT AOO_NET_MSG_PING

#define AOO_NET_MSG_CLIENT_LOGIN \
    AOO_MSG_DOMAIN AOO_NET_MSG_CLIENT AOO_NET_MSG_LOGIN

#define AOO_NET_MSG_CLIENT_REPLY \
    AOO_MSG_DOMAIN AOO_NET_MSG_CLIENT AOO_NET_MSG_REPLY

#define AOO_NET_MSG_CLIENT_GROUP_JOIN \
    AOO_MSG_DOMAIN AOO_NET_MSG_CLIENT AOO_NET_MSG_GROUP AOO_NET_MSG_JOIN

#define AOO_NET_MSG_CLIENT_GROUP_LEAVE \
    AOO_MSG_DOMAIN AOO_NET_MSG_CLIENT AOO_NET_MSG_GROUP AOO_NET_MSG_LEAVE

#define AOO_NET_MSG_CLIENT_PEER_JOIN \
    AOO_MSG_DOMAIN AOO_NET_MSG_CLIENT AOO_NET_MSG_PEER AOO_NET_MSG_JOIN

#define AOO_NET_MSG_CLIENT_PEER_LEAVE \
    AOO_MSG_DOMAIN AOO_NET_MSG_CLIENT AOO_NET_MSG_PEER AOO_NET_MSG_LEAVE

#define AOO_NET_MSG_GROUP_JOIN \
    AOO_NET_MSG_GROUP AOO_NET_MSG_JOIN

#define AOO_NET_MSG_GROUP_LEAVE \
    AOO_NET_MSG_GROUP AOO_NET_MSG_LEAVE

/*//////////////////// AoO server /////////////////////*/

aoo_net_server * aoo_net_server_new(int port, uint32_t flags, aoo_error *err) {
    // create UDP socket
    int udpsocket = aoo::socket_udp(port);
    if (udpsocket < 0){
        auto e = aoo::socket_errno();
        *err = AOO_ERROR_UNSPECIFIED;
        LOG_ERROR("aoo_server: couldn't create UDP socket (" << e << ")");
        return nullptr;
    }

    // increase UDP receive buffer size to 16 MB
    if (aoo::socket_setrecvbufsize(udpsocket, 1<<24) < 0){
        aoo::socket_error_print("setrecvbufsize");
    }

    // create TCP socket
    int tcpsocket = aoo::socket_tcp(port);
    if (tcpsocket < 0){
        auto e = aoo::socket_errno();
        *err = AOO_ERROR_UNSPECIFIED;
        LOG_ERROR("aoo_server: couldn't create TCP socket (" << e << ")");
        aoo::socket_close(udpsocket);
        return nullptr;
    }

    // listen
    if (listen(tcpsocket, 32) < 0){
        auto e = aoo::socket_errno();
        *err = AOO_ERROR_UNSPECIFIED;
        LOG_ERROR("aoo_server: listen() failed (" << e << ")");
        aoo::socket_close(tcpsocket);
        aoo::socket_close(udpsocket);
        return nullptr;
    }

    return aoo::construct<aoo::net::server_imp>(tcpsocket, udpsocket);
}

aoo::net::server_imp::server_imp(int tcpsocket, int udpsocket)
    : tcpsocket_(tcpsocket), udpserver_(udpsocket)
{
    eventsocket_ = socket_udp(0);
    type_ = socket_family(udpsocket);
    // commands_.reserve(256);
    // events_.reserve(256);
}

void aoo_net_server_free(aoo_net_server *server){
    // cast to correct type because base class
    // has no virtual destructor!
    aoo::destroy(static_cast<aoo::net::server_imp *>(server));
}

aoo::net::server_imp::~server_imp() {
    socket_close(tcpsocket_);
    tcpsocket_ = -1;

    // clear explicitly to avoid crash!
    clients_.clear();
}

aoo_error aoo_net_server_run(aoo_net_server *server){
    return server->run();
}

aoo_error aoo::net::server_imp::run(){
    // wait for networking or other events
    while (!quit_.load()){
        if (!receive()){
            break;
        }
    }

    if (!notify_on_shutdown_.load()){
        // JC: need to close all the clients sockets without
        // having them send anything out, so that active communication
        // between connected peers can continue if the server goes down for maintainence
        for (auto& client : clients_){
            client.close(false);
        }
    }

    return AOO_OK;
}

aoo_error aoo_net_server_quit(aoo_net_server *server){
    return server->quit();
}

aoo_error aoo::net::server_imp::quit(){
    // set quit and wake up receive thread
    quit_.store(true);
    if (!socket_signal(eventsocket_)){
        // force wakeup by closing the socket.
        // this is not nice and probably undefined behavior,
        // the MSDN docs explicitly forbid it!
        socket_close(eventsocket_);
    }
    return AOO_OK;
}

aoo_error aoo_net_server_set_eventhandler(aoo_net_client *sink, aoo_eventhandler fn,
                                          void *user, int32_t mode)
{
    return sink->set_eventhandler(fn, user, mode);
}

aoo_error aoo::net::server_imp::set_eventhandler(aoo_eventhandler fn, void *user,
                                             int32_t mode)
{
    eventhandler_ = fn;
    eventcontext_ = user;
    eventmode_ = (aoo_event_mode)mode;
    return AOO_OK;
}

aoo_bool aoo_net_server_events_available(aoo_net_server *server){
    return server->events_available();
}

aoo_bool aoo::net::server_imp::events_available(){
    return !events_.empty();
}

aoo_error aoo_net_server_poll_events(aoo_net_server *server){
    return server->poll_events();
}

aoo_error aoo::net::server_imp::poll_events(){
    // always thread-safe
    std::unique_ptr<ievent> e;
    while (events_.try_pop(e)){
        eventhandler_(eventcontext_, &e->event_, AOO_THREAD_UNKNOWN);
    }
    return AOO_OK;
}

aoo_error aoo_net_server_ctl(aoo_net_server *server, int32_t ctl,
                             intptr_t index, void *p, size_t size)
{
    return server->control(ctl, index, p, size);
}

aoo_error aoo::net::server_imp::control(int32_t ctl, intptr_t index,
                                        void *ptr, size_t size)
{
    LOG_WARNING("aoo_server: unsupported control " << ctl);
    return AOO_ERROR_UNSPECIFIED;
}

namespace aoo {
namespace net {

std::string server_imp::error_to_string(error e){
    switch (e){
    case server_imp::error::access_denied:
        return "access denied";
    case server_imp::error::permission_denied:
        return "permission denied";
    case server_imp::error::wrong_password:
        return "wrong password";
    default:
        return "unknown error";
    }
}

std::shared_ptr<user> server_imp::get_user(const std::string& name,
                                       const std::string& pwd,
                                       uint32_t version, error& e)
{
    auto usr = find_user(name);
    if (usr){
        // check if someone is already logged in
        if (usr->active()){
            e = error::access_denied;
            return nullptr;
        }
        // check password for existing user
        if (usr->password == pwd){
            e = error::none;
            return usr;
        } else {
            e = error::wrong_password;
            return nullptr;
        }
    } else {
        // create new user (LATER add option to disallow this)
        if (true){
            auto id = get_next_user_id();
            usr = std::make_shared<user>(name, pwd, id, version);
            users_.push_back(usr);
            e = error::none;
            return usr;
        } else {
            e = error::permission_denied;
            return nullptr;
        }
    }
}

std::shared_ptr<user> server_imp::find_user(const std::string& name)
{
    for (auto& usr : users_){
        if (usr->name == name){
            return usr;
        }
    }
    return nullptr;
}

std::shared_ptr<group> server_imp::get_group(const std::string& name,
                                         const std::string& pwd, error& e)
{
    auto grp = find_group(name);
    if (grp){
        // check password for existing group
        if (grp->password == pwd){
            e = error::none;
            return grp;
        } else {
            e = error::wrong_password;
            return nullptr;
        }
    } else {
        // create new group (LATER add option to disallow this)
        if (true){
            grp = std::make_shared<group>(name, pwd);
            groups_.push_back(grp);
            e = error::none;
            return grp;
        } else {
            e = error::permission_denied;
            return nullptr;
        }
    }
}

std::shared_ptr<group> server_imp::find_group(const std::string& name)
{
    for (auto& grp : groups_){
        if (grp->name == name){
            return grp;
        }
    }
    return nullptr;
}


void server_imp::on_user_joined(user &usr){
    auto e = std::make_unique<user_event>(AOO_NET_USER_JOIN_EVENT,
                                          usr.name.c_str(), usr.id,
                                          usr.endpoint()->local_address()); // do we need this?
    send_event(std::move(e));
}

void server_imp::on_user_left(user &usr){
    auto e = std::make_unique<user_event>(AOO_NET_USER_LEAVE_EVENT,
                                          usr.name.c_str(), usr.id,
                                          usr.endpoint()->local_address()); // do we need this?
    send_event(std::move(e));
}

void server_imp::on_user_joined_group(user& usr, group& grp){
    // 1) send the new member to existing group members
    // 2) send existing group members to the new member
    for (auto& peer : grp.users()){
        if (peer->id != usr.id){
            char buf[AOO_MAXPACKETSIZE];

            auto notify = [&](client_endpoint* dest, user& u){
                osc::OutboundPacketStream msg(buf, sizeof(buf));
                msg << osc::BeginMessage(AOO_NET_MSG_CLIENT_PEER_JOIN)
                    << grp.name.c_str() << u.name.c_str();
                // only v0.2-pre3 and abvoe
                if (peer->version > 0){
                    msg << u.id;
                }
                // send *unmapped* addresses in case the client is IPv4 only
                for (auto& addr : u.endpoint()->public_addresses()){
                    msg << addr.name_unmapped() << addr.port();
                }
                msg << osc::EndMessage;

                dest->send_message(msg.Data(), msg.Size());
            };

            // notify new member
            notify(usr.endpoint(), *peer);

            // notify existing member
            notify(peer->endpoint(), usr);
        }
    }

    auto e = std::make_unique<group_event>(AOO_NET_GROUP_JOIN_EVENT,
                                           grp.name.c_str(),
                                           usr.name.c_str(), usr.id);
    send_event(std::move(e));
}

void server_imp::on_user_left_group(user& usr, group& grp){
    if (tcpsocket_ < 0){
        return; // prevent sending messages during shutdown
    }
    // notify group members
    for (auto& peer : grp.users()){
        if (peer->id != usr.id){
            char buf[AOO_MAXPACKETSIZE];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            msg << osc::BeginMessage(AOO_NET_MSG_CLIENT_PEER_LEAVE)
                  << grp.name.c_str() << usr.name.c_str() << usr.id
                  << osc::EndMessage;

            peer->endpoint()->send_message(msg.Data(), msg.Size());
        }
    }

    auto e = std::make_unique<group_event>(AOO_NET_GROUP_LEAVE_EVENT,
                                           grp.name.c_str(),
                                           usr.name.c_str(), usr.id);
    send_event(std::move(e));
}

void server_imp::handle_relay_message(const osc::ReceivedMessage& msg,
                                      const ip_address& src){
    auto it = msg.ArgumentsBegin();

    auto ip = (it++)->AsString();
    auto port = (it++)->AsInt32();
    ip_address dst(ip, port, type());

    const void *msgData;
    osc::osc_bundle_element_size_t msgSize;
    (it++)->AsBlob(msgData, msgSize);

    // forward message to matching client
    // send unmapped address in case the client is IPv4 only!
    char buf[AOO_MAXPACKETSIZE];
    osc::OutboundPacketStream out(buf, sizeof(buf));
    out << osc::BeginMessage(AOO_MSG_DOMAIN AOO_NET_MSG_RELAY)
        << src.name_unmapped() << src.port() << osc::Blob(msgData, msgSize)
        << osc::EndMessage;

    for (auto& client : clients_){
        if (client.match(dst)) {
            client.send_message(out.Data(), out.Size());
            return;
        }
    }
    LOG_WARNING("aoo_server: couldn't find matching client for relay message");
}

void server_imp::send_event(std::unique_ptr<ievent> e){
    switch (eventmode_){
    case AOO_EVENT_POLL:
        events_.push(std::move(e));
        break;
    case AOO_EVENT_CALLBACK:
        // server only has network threads
        eventhandler_(eventcontext_, &e->event_, AOO_THREAD_NETWORK);
        break;
    default:
        break;
    }
}

int32_t server_imp::get_next_user_id(){
    // LATER make random user ID
    return next_user_id_++;
}

bool server_imp::receive(){
    bool didclose = false;
    int numclients = clients_.size();

    // initialize pollfd array
    // allocate extra slots for main TCP socket and event socket
    pollarray_.resize(numclients + 2);

    for (int i = 0; i < (int)pollarray_.size(); ++i){
        pollarray_[i].events = POLLIN;
        pollarray_[i].revents = 0;
    }

    int index = 0;
    for (auto& c : clients_){
        pollarray_[index++].fd = c.socket();
    }

    auto& tcp_fd = pollarray_[numclients];
    tcp_fd.fd = tcpsocket_;

    auto& event_fd = pollarray_[numclients+1];
    event_fd.fd = eventsocket_;

    // NOTE: macOS/BSD requires the negative timeout to be exactly -1!
#ifdef _WIN32
    int result = WSAPoll(pollarray_.data(), pollarray_.size(), -1);
#else
    int result = poll(pollarray_.data(), pollarray_.size(), -1);
#endif
    if (result < 0){
        int err = errno;
        if (err == EINTR){
            return true; // ?
        } else {
            LOG_ERROR("aoo_server: poll failed (" << err << ")");
            return false;
        }
    }

    if (event_fd.revents){
        return false; // quit
    }

    if (tcp_fd.revents){
        // accept new client
        ip_address addr;
        auto sock = accept(tcpsocket_, addr.address_ptr(), addr.length_ptr());
        if (sock >= 0){
            clients_.emplace_back(*this, sock, addr);
            LOG_VERBOSE("aoo_server: accepted client (IP: "
                        << addr.name() << ", port: " << addr.port() << ")");
        } else {
            int err = socket_errno();
            LOG_ERROR("aoo_server: couldn't accept client (" << err << ")");
        }
    }

    // use original number of clients!
    index = 0;
    for (auto it = clients_.begin(); index < numclients; ++index, ++it){
        if (pollarray_[index].revents){
            // receive data from client
            if (!it->receive_data()){
                LOG_VERBOSE("aoo_server: close client");
                it->close();
                didclose = true;
            }
        }
    }

    if (didclose){
        update();
    }

    return true;
}

void server_imp::update(){
    clients_.remove_if([](auto& c){ return !c.active(); });
    // automatically purge stale users
    // LATER add an option so that users will persist
    for (auto it = users_.begin(); it != users_.end(); ){
        if (!(*it)->active()){
            it = users_.erase(it);
        } else {
            ++it;
        }
    }
    // automatically purge empty groups
    // LATER add an option so that groups will persist
    for (auto it = groups_.begin(); it != groups_.end(); ){
        if ((*it)->num_users() == 0){
            it = groups_.erase(it);
        } else {
            ++it;
        }
    }
}

uint32_t server_imp::flags() const {
    uint32_t flags = 0;
    if (allow_relay_.load(std::memory_order_relaxed)){
        flags |= AOO_NET_SERVER_RELAY;
    }
    return flags;
}

/*////////////////////////// udp_server /////////////////////*/

udp_server::udp_server(int socket) {
    socket_ = socket;
    type_ = socket_family(socket);
    receivethread_ = std::thread(&udp_server::receive_packets, this);
    workerthread_ = std::thread(&udp_server::handle_packets, this);
}

udp_server::~udp_server(){
    quit_ = true;
    event_.set();
    socket_signal(socket_);

    if (receivethread_.joinable()){
        receivethread_.join();
    }

    if (workerthread_.joinable()){
        workerthread_.join();
    }

    socket_close(socket_);
}

void udp_server::receive_packets(){
    while (!quit_.load(std::memory_order_relaxed)){
        ip_address addr;
        char buf[AOO_MAXPACKETSIZE];
        int nbytes = socket_receive(socket_, buf, AOO_MAXPACKETSIZE, &addr, -1);
        if (nbytes > 0){
            // add packet to queue
            recvbuffer_.produce([&](udp_packet& p){
                p.address = addr;
                p.data.assign(buf, buf + nbytes);
            });
        #if DEBUG_THREADS
            recvbufferfill_.fetch_add(1, std::memory_order_relaxed);
        #endif
            event_.set();
        } else if (nbytes < 0) {
            // ignore errors when quitting
            if (!quit_){
                socket_error_print("recv");
            }
            break;
        }
        // ignore empty packets (used for quit signalling)
    #if DEBUG_THREADS
        std::cout << "receive_packets: waiting" << std::endl;
    #endif
    }
}

void udp_server::handle_packets(){
    while (!quit_.load(std::memory_order_relaxed)){
        event_.wait();

        while (!recvbuffer_.empty()){
        #if DEBUG_THREADS
            std::cout << "perform_io: handle_message" << std::endl;
        #endif
            recvbuffer_.consume([&](const udp_packet& p){
                handle_packet(p.data.data(), p.data.size(), p.address);
            });
        #if DEBUG_THREADS
            auto fill = recvbufferfill_.fetch_sub(1, std::memory_order_relaxed) - 1;
            std::cerr << "receive buffer fill: " << fill << std::endl;
        #endif
        }
    }
}

void udp_server::handle_packet(const char *data, int32_t size, const ip_address& addr){
    try {
        osc::ReceivedPacket packet(data, size);
        osc::ReceivedMessage msg(packet);

        aoo_type type;
        int32_t onset;
        auto err = parse_pattern(data, size, type, onset);
        if (err != AOO_OK){
            LOG_WARNING("aoo_server: not an AOO NET message!");
            return;
        }

        if (type == AOO_TYPE_SERVER){
            handle_message(msg, onset, addr);
        } else if (type == AOO_TYPE_RELAY){
            handle_relay_message(msg, addr);
        } else {
            LOG_WARNING("aoo_server: not a client message!");
            return;
        }
    } catch (const osc::Exception& e){
        LOG_ERROR("aoo_server: exception in receive_udp: " << e.what());
        // ignore for now
    }
}

void udp_server::handle_message(const osc::ReceivedMessage &msg, int onset,
                                const ip_address& addr)
{
    auto pattern = msg.AddressPattern() + onset;
    LOG_DEBUG("aoo_server: handle client UDP message " << pattern);

    try {
        if (!strcmp(pattern, AOO_NET_MSG_PING)){
            // reply with /ping message
            char buf[512];
            osc::OutboundPacketStream reply(buf, sizeof(buf));
            reply << osc::BeginMessage(AOO_NET_MSG_CLIENT_PING)
                  << osc::EndMessage;

            send_message(reply.Data(), reply.Size(), addr);
        } else if (!strcmp(pattern, AOO_NET_MSG_REQUEST)){
            // reply with /reply message
            // send *unmapped* address in case the client is IPv4 only
            char buf[512];
            osc::OutboundPacketStream reply(buf, sizeof(buf));
            reply << osc::BeginMessage(AOO_NET_MSG_CLIENT_REPLY)
                  << addr.name_unmapped() << addr.port()
                  << osc::EndMessage;

            send_message(reply.Data(), reply.Size(), addr);
        } else {
            LOG_ERROR("aoo_server: unknown message " << pattern);
        }
    } catch (const osc::Exception& e){
        LOG_ERROR("aoo_server: exception on handling " << pattern
                  << " message: " << e.what());
        // ignore for now
    }
}

void udp_server::handle_relay_message(const osc::ReceivedMessage& msg,
                                      const ip_address& src){
    auto it = msg.ArgumentsBegin();

    auto ip = (it++)->AsString();
    auto port = (it++)->AsInt32();
    ip_address dst(ip, port, type_);

    const void *msgData;
    osc::osc_bundle_element_size_t msgSize;
    (it++)->AsBlob(msgData, msgSize);

    // forward message to matching client
    // send unmapped address in case the client is IPv4 only!
    char buf[AOO_MAXPACKETSIZE];
    osc::OutboundPacketStream out(buf, sizeof(buf));
    out << osc::BeginMessage(AOO_MSG_DOMAIN AOO_NET_MSG_RELAY)
        << src.name_unmapped() << src.port() << osc::Blob(msgData, msgSize)
        << osc::EndMessage;

    send_message(out.Data(), out.Size(), dst);
}

void udp_server::send_message(const char *msg, int32_t size,
                              const ip_address &addr)
{
    int result = ::sendto(socket_, msg, size, 0,
                          addr.address(), addr.length());
    if (result < 0){
        int err = socket_errno();
        // TODO handle error
        LOG_ERROR("aoo_server: send() failed (" << err << ")");
    }
}

/*////////////////////////// user ///////////////////////////*/

void user::on_close(server_imp& s){
    // disconnect user from groups
    for (auto& grp : groups_){
        grp->remove_user(*this);
        s.on_user_left_group(*this, *grp);
    }

    s.on_user_left(*this);

    groups_.clear();
    // clear endpoint so the server knows it can remove the user
    endpoint_ = nullptr;
}

bool user::add_group(std::shared_ptr<group> grp){
    auto it = std::find(groups_.begin(), groups_.end(), grp);
    if (it == groups_.end()){
        groups_.push_back(grp);
        return true;
    } else {
        return false;
    }
}

bool user::remove_group(const group& grp){
    // find by address
    auto it = std::find_if(groups_.begin(), groups_.end(),
                           [&](auto& g){ return g.get() == &grp; });
    if (it != groups_.end()){
        groups_.erase(it);
        return true;
    } else {
        return false;
    }
}

/*////////////////////////// group /////////////////////////*/

bool group::add_user(std::shared_ptr<user> grp){
    auto it = std::find(users_.begin(), users_.end(), grp);
    if (it == users_.end()){
        users_.push_back(grp);
        return true;
    } else {
        LOG_ERROR("group::add_user: bug");
        return false;
    }
}

bool group::remove_user(const user& usr){
    // find by address
    auto it = std::find_if(users_.begin(), users_.end(),
                           [&](auto& u){ return u.get() == &usr; });
    if (it != users_.end()){
        users_.erase(it);
        return true;
    } else {
        LOG_ERROR("group::remove_user: bug");
        return false;
    }
}

/*///////////////////////// client_endpoint /////////////////////////////*/

client_endpoint::client_endpoint(server_imp &s, int socket, const ip_address &addr)
    : server_(&s), socket_(socket), addr_(addr)
{
    // set TCP_NODELAY - do we need to do this?
    int val = 1;
    if (setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (char *)&val, sizeof(val)) < 0){
        LOG_WARNING("client_endpoint: couldn't set TCP_NODELAY");
        // ignore
    }

    sendbuffer_.setup(65536);
    recvbuffer_.setup(65536);
}

client_endpoint::~client_endpoint(){
    close();
}

bool client_endpoint::match(const ip_address& addr) const {
    // match public UDP addresses!
    for (auto& a : public_addresses_){
        if (a == addr){
            return true;
        }
    }
    return false;
}

void client_endpoint::close(bool notify){
    if (socket_ >= 0){
        LOG_VERBOSE("aoo_server: close client endpoint "
                    << addr_.name() << " " << addr_.port());
        socket_close(socket_);
        socket_ = -1;

        if (user_ && notify){
            user_->on_close(*server_);
        }
    }
}

void client_endpoint::send_message(const char *msg, int32_t size){
    if (sendbuffer_.write_packet((const uint8_t *)msg, size)){
        while (sendbuffer_.read_available()){
            uint8_t buf[1024];
            int32_t total = sendbuffer_.read_bytes(buf, sizeof(buf));

            int32_t nbytes = 0;
            while (nbytes < total){
                auto res = ::send(socket_, (char *)buf + nbytes, total - nbytes, 0);
                if (res >= 0){
                    nbytes += res;
                #if 0
                    LOG_VERBOSE("aoo_server: sent " << res << " bytes");
                #endif
                } else {
                    auto err = socket_errno();
                    // TODO handle error
                    LOG_ERROR("aoo_server: send() failed (" << err << ")");
                    return;
                }
            }
        }
        LOG_DEBUG("aoo_server: sent " << msg << " to client");
    } else {
        LOG_ERROR("aoo_server: couldn't send " << msg << " to client");
    }
}

bool client_endpoint::receive_data(){
    char buffer[AOO_MAXPACKETSIZE];
    auto result = recv(socket_, buffer, sizeof(buffer), 0);
    if (result == 0){
        LOG_WARNING("client_endpoint: connection was closed");
        return false;
    }
    if (result < 0){
        int err = socket_errno();
        // TODO handle error
        LOG_ERROR("client_endpoint: recv() failed (" << err << ")");
        return false;
    }

    recvbuffer_.write_bytes((uint8_t *)buffer, result);

    // handle packets
    uint8_t buf[AOO_MAXPACKETSIZE];
    int32_t size;
    while ((size = recvbuffer_.read_packet(buf, sizeof(buf))) > 0){
        try {
            osc::ReceivedPacket packet((char *)buf, size);
            if (packet.IsBundle()){
                osc::ReceivedBundle bundle(packet);
                if (!handle_bundle(bundle)){
                    return false;
                }
            } else {
                if (!handle_message(packet.Contents(), packet.Size())){
                    return false;
                }
            }
        } catch (const osc::Exception& e){
            LOG_ERROR("aoo_server: exception in client_endpoint::receive_data: " << e.what());
            return false; // close
        }
    }
    return true;
}

bool client_endpoint::handle_message(const char *data, int32_t n){
    osc::ReceivedPacket packet(data, n);
    osc::ReceivedMessage msg(packet);

    aoo_type type;
    int32_t onset;
    auto err = parse_pattern(data, n, type, onset);
    if (err != AOO_OK){
        LOG_WARNING("aoo_server: not an AOO NET message!");
        return false;
    }

    try {
        if (type == AOO_TYPE_SERVER){
            auto pattern = msg.AddressPattern() + onset;
            LOG_DEBUG("aoo_server: got server message " << pattern);
            if (!strcmp(pattern, AOO_NET_MSG_PING)){
                handle_ping(msg);
            } else if (!strcmp(pattern, AOO_NET_MSG_LOGIN)){
                handle_login(msg);
            } else if (!strcmp(pattern, AOO_NET_MSG_GROUP_JOIN)){
                handle_group_join(msg);
            } else if (!strcmp(pattern, AOO_NET_MSG_GROUP_LEAVE)){
                handle_group_leave(msg);
            } else {
                LOG_ERROR("aoo_server: unknown server message " << pattern);
                return false;
            }
        } else if (type == AOO_TYPE_RELAY){
            // use public address!
            server_->handle_relay_message(msg, public_addresses().front());
        } else {
            LOG_WARNING("aoo_client: got unexpected message " << msg.AddressPattern());
            return false;
        }

        return true;
    } catch (const osc::Exception& e){
        LOG_ERROR("aoo_server: exception on handling " << msg.AddressPattern()
                  << " message: " << e.what());
        return false;
    }
}

bool client_endpoint::handle_bundle(const osc::ReceivedBundle &bundle){
    auto it = bundle.ElementsBegin();
    while (it != bundle.ElementsEnd()){
        if (it->IsBundle()){
            osc::ReceivedBundle b(*it);
            if (!handle_bundle(b)){
                return false;
            }
        } else {
            if (!handle_message(it->Contents(), it->Size())){
                return false;
            }
        }
        ++it;
    }
    return true;
}

void client_endpoint::handle_ping(const osc::ReceivedMessage& msg){
    // send reply
    char buf[AOO_MAXPACKETSIZE];
    osc::OutboundPacketStream reply(buf, sizeof(buf));
    reply << osc::BeginMessage(AOO_NET_MSG_CLIENT_PING) << osc::EndMessage;

    send_message(reply.Data(), reply.Size());
}

void client_endpoint::handle_login(const osc::ReceivedMessage& msg)
{
    int32_t result = 0;
    uint32_t version = 0;
    std::string errmsg;

    auto it = msg.ArgumentsBegin();
    auto count = msg.ArgumentCount();
    if (count > 6){
        version = (uint32_t)(it++)->AsInt32();
        count--;
    }
    // for now accept login messages without version.
    // LATER they should fail, so clients have to upgrade.
    if (version == 0 || check_version(version)){
        std::string username = (it++)->AsString();
        std::string password = (it++)->AsString();
        count -= 2;

        server_imp::error err;
        if (!user_){
            user_ = server_->get_user(username, password, version, err);
            if (user_){
                // success - collect addresses
                while (count >= 2){
                    std::string ip = (it++)->AsString();
                    int32_t port = (it++)->AsInt32();
                    ip_address addr(ip, port, server_->type());
                    if (addr.valid()){
                        public_addresses_.push_back(addr);
                    }
                    count -= 2;
                }
                user_->set_endpoint(this);

                LOG_VERBOSE("aoo_server: login: id: " << user_->id
                            << ", username: " << username << ", password: " << password);

                result = 1;

                server_->on_user_joined(*user_);
            } else {
                errmsg = server_imp::error_to_string(err);
            }
        } else {
            errmsg = "already logged in"; // shouldn't happen
        }
    } else {
        errmsg = "version not supported";
    }
    // send reply
    char buf[AOO_MAXPACKETSIZE];
    osc::OutboundPacketStream reply(buf, sizeof(buf));
    reply << osc::BeginMessage(AOO_NET_MSG_CLIENT_LOGIN) << result;
    if (result){
        reply << user_->id;
        reply << (int32_t)server_->flags();
    } else {
        reply << errmsg.c_str();
    }
    reply << osc::EndMessage;

    send_message(reply.Data(), reply.Size());
}

void client_endpoint::handle_group_join(const osc::ReceivedMessage& msg)
{
    int result = 0;
    std::string errmsg;

    auto it = msg.ArgumentsBegin();
    std::string name = (it++)->AsString();
    std::string password = (it++)->AsString();

    server_imp::error err;
    if (user_){
        auto grp = server_->get_group(name, password, err);
        if (grp){
            if (user_->add_group(grp)){
                grp->add_user(user_);
                server_->on_user_joined_group(*user_, *grp);
                result = 1;
            } else {
                errmsg = "already a group member";
            }
        } else {
            errmsg = server_imp::error_to_string(err);
        }
    } else {
        errmsg = "not logged in";
    }

    // send reply
    char buf[AOO_MAXPACKETSIZE];
    osc::OutboundPacketStream reply(buf, sizeof(buf));
    reply << osc::BeginMessage(AOO_NET_MSG_CLIENT_GROUP_JOIN)
          << name.c_str() << result << errmsg.c_str() << osc::EndMessage;

    send_message(reply.Data(), reply.Size());
}

void client_endpoint::handle_group_leave(const osc::ReceivedMessage& msg){
    int result = 0;
    std::string errmsg;

    auto it = msg.ArgumentsBegin();
    std::string name = (it++)->AsString();

    if (user_){
        auto grp = server_->find_group(name);
        if (grp){
            if (user_->remove_group(*grp)){
                grp->remove_user(*user_);
                server_->on_user_left_group(*user_, *grp);
                result = 1;
            } else {
                errmsg = "not a group member";
            }
        } else {
            errmsg = "couldn't find group";
        }
    } else {
        errmsg = "not logged in";
    }

    // send reply
    char buf[AOO_MAXPACKETSIZE];
    osc::OutboundPacketStream reply(buf, sizeof(buf));
    reply << osc::BeginMessage(AOO_NET_MSG_CLIENT_GROUP_LEAVE)
          << name.c_str() << result << errmsg.c_str() << osc::EndMessage;

    send_message(reply.Data(), reply.Size());
}

/*///////////////////// events ////////////////////////*/

server_imp::error_event::error_event(int32_t type, int32_t code,
                                 const char * msg)
{
    error_event_.type = type;
    error_event_.error_code = code;
    error_event_.error_message = copy_string(msg);
}

server_imp::error_event::~error_event()
{
    free_string((char *)error_event_.error_message);
}

server_imp::user_event::user_event(int32_t type,
                               const char *name, int32_t id,
                               const ip_address& address){
    user_event_.type = type;
    user_event_.user_name = copy_string(name);
    user_event_.user_id = id;
    user_event_.address = copy_sockaddr(address.address(), address.length());
    user_event_.addrlen = address.length();
}

server_imp::user_event::~user_event()
{
    free_string((char *)user_event_.user_name);
    free_sockaddr((void *)user_event_.address, user_event_.addrlen);
}

server_imp::group_event::group_event(int32_t type, const char *group,
                                 const char *user, int32_t id)
{
    group_event_.type = type;
    group_event_.group_name = copy_string(group);
    group_event_.user_name = copy_string(user);
    group_event_.user_id = id;
}

server_imp::group_event::~group_event()
{
    free_string((char *)group_event_.group_name);
    free_string((char *)group_event_.user_name);
}

} // net
} // aoo
