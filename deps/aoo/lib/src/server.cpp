/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "server.hpp"

#include <functional>
#include <algorithm>
#include <random>

#define AOONET_MSG_CLIENT_PING \
    AOO_MSG_DOMAIN AOONET_MSG_CLIENT AOONET_MSG_PING

#define AOONET_MSG_CLIENT_LOGIN \
    AOO_MSG_DOMAIN AOONET_MSG_CLIENT AOONET_MSG_LOGIN

#define AOONET_MSG_CLIENT_REPLY \
    AOO_MSG_DOMAIN AOONET_MSG_CLIENT AOONET_MSG_REPLY

#define AOONET_MSG_CLIENT_GROUP_PUBLIC \
    AOO_MSG_DOMAIN AOONET_MSG_CLIENT AOONET_MSG_GROUP AOONET_MSG_PUBLIC

#define AOONET_MSG_CLIENT_GROUP_PUBLIC_ADD \
    AOO_MSG_DOMAIN AOONET_MSG_CLIENT AOONET_MSG_GROUP AOONET_MSG_PUBLIC AOONET_MSG_ADD

#define AOONET_MSG_CLIENT_GROUP_PUBLIC_DEL \
    AOO_MSG_DOMAIN AOONET_MSG_CLIENT AOONET_MSG_GROUP AOONET_MSG_PUBLIC AOONET_MSG_DEL


#define AOONET_MSG_CLIENT_GROUP_JOIN \
    AOO_MSG_DOMAIN AOONET_MSG_CLIENT AOONET_MSG_GROUP AOONET_MSG_JOIN

#define AOONET_MSG_CLIENT_GROUP_LEAVE \
    AOO_MSG_DOMAIN AOONET_MSG_CLIENT AOONET_MSG_GROUP AOONET_MSG_LEAVE

#define AOONET_MSG_CLIENT_PEER_JOIN \
    AOO_MSG_DOMAIN AOONET_MSG_CLIENT AOONET_MSG_PEER AOONET_MSG_JOIN

#define AOONET_MSG_CLIENT_PEER_LEAVE \
    AOO_MSG_DOMAIN AOONET_MSG_CLIENT AOONET_MSG_PEER AOONET_MSG_LEAVE

#define AOONET_MSG_GROUP_JOIN \
    AOONET_MSG_GROUP AOONET_MSG_JOIN

#define AOONET_MSG_GROUP_LEAVE \
    AOONET_MSG_GROUP AOONET_MSG_LEAVE

#define AOONET_MSG_GROUP_PUBLIC \
    AOONET_MSG_GROUP AOONET_MSG_PUBLIC


namespace aoo {
namespace net {

char * copy_string(const char * s);

} // net
} // aoo

/*//////////////////// AoO server /////////////////////*/

aoonet_server * aoonet_server_new(int port, int32_t *err) {
    int val = 0;

    // make 'any' address
    sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = INADDR_ANY;
    sa.sin_port = htons(port);

    // create and bind UDP socket
    int udpsocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpsocket < 0){
        *err = aoo::net::socket_errno();
        LOG_ERROR("aoo_server: couldn't create UDP socket (" << *err << ")");
        return nullptr;
    }

    // set non-blocking
    // (this is not necessary on Windows, because WSAEventSelect will do it automatically)
#ifndef _WIN32
    val = 1;
    if (ioctl(udpsocket, FIONBIO, (char *)&val) < 0){
        *err = aoo::net::socket_errno();
        LOG_ERROR("aoo_server: couldn't set socket to non-blocking (" << *err << ")");
        aoo::net::socket_close(udpsocket);
        return nullptr;
    }
#endif

    if (bind(udpsocket, (sockaddr *)&sa, sizeof(sa)) < 0){
        *err = aoo::net::socket_errno();
        LOG_ERROR("aoo_server: couldn't bind UDP socket (" << *err << ")");
        aoo::net::socket_close(udpsocket);
        return nullptr;
    }

    // create TCP socket
    int tcpsocket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpsocket < 0){
        *err = aoo::net::socket_errno();
        LOG_ERROR("aoo_server: couldn't create TCP socket (" << *err << ")");
        aoo::net::socket_close(udpsocket);
        return nullptr;
    }

    // set SO_REUSEADDR
    val = 1;
    if (setsockopt(tcpsocket, SOL_SOCKET, SO_REUSEADDR,
                      (char *)&val, sizeof(val)) < 0)
    {
        *err = aoo::net::socket_errno();
        LOG_ERROR("aoo_server: couldn't set SO_REUSEADDR (" << *err << ")");
        aoo::net::socket_close(tcpsocket);
        aoo::net::socket_close(udpsocket);
        return nullptr;
    }

    // set TCP_NODELAY
    val = 1;
    if (setsockopt(tcpsocket, IPPROTO_TCP, TCP_NODELAY, (char *)&val, sizeof(val)) < 0){
        LOG_WARNING("aoo_server: couldn't set TCP_NODELAY");
        // ignore
    }

    // set non-blocking
    // (this is not necessary on Windows, because WSAEventSelect will do it automatically)
#ifndef _WIN32
    val = 1;
    if (ioctl(tcpsocket, FIONBIO, (char *)&val) < 0){
        *err = aoo::net::socket_errno();
        LOG_ERROR("aoo_server: couldn't set socket to non-blocking (" << *err << ")");
        aoo::net::socket_close(tcpsocket);
        aoo::net::socket_close(udpsocket);
        return nullptr;
    }
#endif

    // bind TCP socket
    if (bind(tcpsocket, (sockaddr *)&sa, sizeof(sa)) < 0){
        *err = aoo::net::socket_errno();
        LOG_ERROR("aoo_server: couldn't bind TCP socket (" << *err << ")");
        aoo::net::socket_close(tcpsocket);
        aoo::net::socket_close(udpsocket);
        return nullptr;
    }

    // listen
    if (listen(tcpsocket, 32) < 0){
        *err = aoo::net::socket_errno();
        LOG_ERROR("aoo_server: listen() failed (" << *err << ")");
        aoo::net::socket_close(tcpsocket);
        aoo::net::socket_close(udpsocket);
        return nullptr;
    }

    return new aoo::net::server(tcpsocket, udpsocket);
}

aoo::net::server::server(int tcpsocket, int udpsocket)
    : tcpsocket_(tcpsocket), udpsocket_(udpsocket)
{
#ifdef _WIN32
    waitevent_ = CreateEvent(NULL, FALSE, FALSE, NULL);
    tcpevent_ = WSACreateEvent();
    udpevent_ = WSACreateEvent();
    WSAEventSelect(tcpsocket_, tcpevent_, FD_ACCEPT);
    WSAEventSelect(udpsocket_, udpevent_, FD_READ | FD_WRITE);
#else
    if (pipe(waitpipe_) != 0){
        // TODO handle error
    }
#endif
    commands_.resize(256, 1);
    events_.resize(256, 1);
}

void aoonet_server_free(aoonet_server *server){
    // cast to correct type because base class
    // has no virtual destructor!
    delete static_cast<aoo::net::server *>(server);
}

aoo::net::server::~server() {
#ifdef _WIN32
    CloseHandle(waitevent_);
    WSACloseEvent(tcpevent_);
    WSACloseEvent(udpevent_);
#else
    close(waitpipe_[0]);
    close(waitpipe_[1]);
#endif

    socket_close(tcpsocket_);
    socket_close(udpsocket_);
}

int32_t aoonet_server_run(aoonet_server *server){
    return server->run();
}

int32_t aoo::net::server::run(){
    while (!quit_.load()){
        // wait for networking or other events
        wait_for_event();

        if (quit_.load()) {
            break;
        }

        // handle commands
        while (commands_.read_available()){
            std::unique_ptr<icommand> cmd;
            commands_.read(cmd);
            cmd->perform(*this);
        }
    }

    // need to close all the clients sockets without
    // having them send anything out, so that active communication
    // between connected peers can continue if the server goes down for maintainence
    for (int i = 0; i < clients_.size(); ++i){
        clients_[i]->close(false);
    }
    
    return 1;
}

int32_t aoonet_server_quit(aoonet_server *server){
    return server->quit();
}

int32_t aoo::net::server::quit(){
    quit_.store(true);
    signal();
    return 0;
}

int32_t aoonet_server_events_available(aoonet_server *server){
    return server->events_available();
}

int32_t aoo::net::server::events_available(){
    return 0;
}

int32_t aoonet_server_handle_events(aoonet_server *server, aoo_eventhandler fn, void *user){
    return server->handle_events(fn, user);
}

int32_t aoo::net::server::handle_events(aoo_eventhandler fn, void *user){
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

std::string server::error_to_string(error e){
    switch (e){
    case server::error::access_denied:
        return "access denied";
    case server::error::permission_denied:
        return "permission denied";
    case server::error::wrong_password:
        return "wrong password";
    default:
        return "unknown error";
    }
}

std::shared_ptr<user> server::get_user(const std::string& name,
                                       const std::string& pwd, error& e)
{
    auto usr = find_user(name);
    if (usr){
        // check if someone is already logged in
        if (usr->endpoint){
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
            usr = std::make_shared<user>(name, pwd);
            users_.push_back(usr);
            e = error::none;
            return usr;
        } else {
            e = error::permission_denied;
            return nullptr;
        }
    }
}

std::shared_ptr<user> server::find_user(const std::string& name)
{
    for (auto& usr : users_){
        if (usr->name == name){
            return usr;
        }
    }
    return nullptr;
}

std::shared_ptr<group> server::get_group(const std::string& name,
                                         const std::string& pwd,
                                         bool is_public,
                                         error& e)
{
    auto grp = find_group(name);
    if (grp){
        // check password for existing group
        if (grp->is_public != is_public) {
            e = error::permission_denied;
            return nullptr;
        }
        else if (grp->password == pwd){
            e = error::none;
            return grp;
        } else {
            e = error::wrong_password;
            return nullptr;
        }
    } else {
        // create new group (LATER add option to disallow this)
        if (true){
            grp = std::make_shared<group>(name, pwd, is_public);
            groups_.push_back(grp);
            e = error::none;
            return grp;
        } else {
            e = error::permission_denied;
            return nullptr;
        }
    }
}

std::shared_ptr<group> server::find_group(const std::string& name)
{
    for (auto& grp : groups_){
        if (grp->name == name){
            return grp;
        }
    }
    return nullptr;
}

int32_t server::get_group_count() const
{
    return (int32_t) groups_.size();
}

int32_t server::get_user_count() const
{
    return (int32_t) users_.size();    
}

void server::on_user_joined(user &usr){
    auto e = std::make_unique<user_event>(AOONET_SERVER_USER_JOIN_EVENT,
                                          usr.name.c_str());
    push_event(std::move(e));
}

void server::on_user_left(user &usr){
    auto e = std::make_unique<user_event>(AOONET_SERVER_USER_LEAVE_EVENT,
                                          usr.name.c_str());
    push_event(std::move(e));
}

void server::on_user_joined_group(user& usr, group& grp){
    // 1) send the new member to existing group members
    // 2) send existing group members to the new member
    for (auto& peer : grp.users()){
        if (peer.get() != &usr){
            char buf[AOO_MAXPACKETSIZE];

            auto notify = [&](client_endpoint* dest, user& u){
                auto e = u.endpoint;

                osc::OutboundPacketStream msg(buf, sizeof(buf));
                msg << osc::BeginMessage(AOONET_MSG_CLIENT_PEER_JOIN)
                    << grp.name.c_str() << u.name.c_str()
                    << e->public_address.name().c_str() << e->public_address.port()
                    << e->local_address.name().c_str() << e->local_address.port()
                    << e->token
                    << osc::EndMessage;

                dest->send_message(msg.Data(), (int32_t) msg.Size());
            };

            // notify new member
            notify(usr.endpoint, *peer);

            // notify existing member
            notify(peer->endpoint, usr);

        }
    }

    if (grp.is_public) {
        on_public_group_modified(grp);
    }

    auto e = std::make_unique<group_event>(AOONET_SERVER_GROUP_JOIN_EVENT,
                                          grp.name.c_str(), usr.name.c_str());
    push_event(std::move(e));
}

void server::on_user_left_group(user& usr, group& grp){
    // notify group members
    for (auto& peer : grp.users()){
        if (peer.get() != &usr){
            char buf[AOO_MAXPACKETSIZE];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            msg << osc::BeginMessage(AOONET_MSG_CLIENT_PEER_LEAVE)
                  << grp.name.c_str() << usr.name.c_str()
                  << osc::EndMessage;

            peer->endpoint->send_message(msg.Data(), (int32_t) msg.Size());
        }
    }

    if (grp.is_public) {
        on_public_group_modified(grp);

        update(); // possibly prune empty
    }

    auto e = std::make_unique<group_event>(AOONET_SERVER_GROUP_LEAVE_EVENT,
                                           grp.name.c_str(), usr.name.c_str());
    push_event(std::move(e));
}

void server::on_user_wants_public_groups(user& usr){
    // 1) send all existing public groups to the user
    for (auto& grp : groups_){
        if (!grp->is_public) continue;

        char buf[AOO_MAXPACKETSIZE];

        osc::OutboundPacketStream msg(buf, sizeof(buf));
        msg << osc::BeginMessage(AOONET_MSG_CLIENT_GROUP_PUBLIC_ADD)
        << grp->name.c_str()
        << (int32_t) grp->users().size()
        << osc::EndMessage;

        usr.endpoint->send_message(msg.Data(), (int32_t) msg.Size());
    }
}

void server::on_public_group_modified(group& grp)
{
    char buf[AOO_MAXPACKETSIZE];

    osc::OutboundPacketStream msg(buf, sizeof(buf));
    msg << osc::BeginMessage(AOONET_MSG_CLIENT_GROUP_PUBLIC_ADD)
    << grp.name.c_str()
    << (int32_t) grp.users().size()
    << osc::EndMessage;

    // notify all users who care
    for (auto & peer : users_) {
        if (peer->watch_public_groups) {
            peer->endpoint->send_message(msg.Data(), (int32_t) msg.Size());
        }
    }
}

void server::on_public_group_removed(group& grp)
{
    // notify all users who care
    char buf[AOO_MAXPACKETSIZE];

    osc::OutboundPacketStream msg(buf, sizeof(buf));
    msg << osc::BeginMessage(AOONET_MSG_CLIENT_GROUP_PUBLIC_DEL)
    << grp.name.c_str()
    << osc::EndMessage;

    // notify all users who care
    for (auto & peer : users_) {
        if (peer->watch_public_groups) {
            peer->endpoint->send_message(msg.Data(), (int32_t) msg.Size());
        }
    }
}


void server::wait_for_event(){
    bool didclose = false;
#ifdef _WIN32
    // allocate three extra slots for master TCP socket, UDP socket and wait event
    int numevents = (clients_.size() + 3);
    auto events = (HANDLE *)alloca(numevents * sizeof(HANDLE));
    int numclients = clients_.size();
    for (int i = 0; i < numclients; ++i){
        events[i] = clients_[i]->event;
    }
    int tcpindex = numclients;
    int udpindex = numclients + 1;
    int waitindex = numclients + 2;
    events[tcpindex] = tcpevent_;
    events[udpindex] = udpevent_;
    events[waitindex] = waitevent_;

    DWORD result = WaitForMultipleObjects(numevents, events, FALSE, INFINITE);

    WSANETWORKEVENTS ne;
    memset(&ne, 0, sizeof(ne));

    int index = result - WAIT_OBJECT_0;
    if (index == tcpindex){
        WSAEnumNetworkEvents(tcpsocket_, tcpevent_, &ne);

        if (ne.lNetworkEvents & FD_ACCEPT){
            // accept new clients
            while (true){
                ip_address addr;
                auto sock = accept(tcpsocket_, (struct sockaddr *)&addr.address, &addr.length);
                if (sock != INVALID_SOCKET){
                    clients_.push_back(std::make_unique<client_endpoint>(*this, sock, addr));
                    LOG_VERBOSE("aoo_server: accepted client (IP: "
                                << addr.name() << ", port: " << addr.port() << ")");
                } else {
                    int err = socket_errno();
                    if (err != WSAEWOULDBLOCK){
                        LOG_ERROR("aoo_server: couldn't accept client (" << err << ")");
                    }
                    break;
                }
            }
        }
    } else if (index == udpindex){
        WSAEnumNetworkEvents(udpsocket_, udpevent_, &ne);

        if (ne.lNetworkEvents & FD_READ){
            receive_udp();
        }
    } else if (index >= 0 && index < numclients){
        // iterate over all clients, starting at index (= the first item which caused an event)
        for (int i = index; i < numclients; ++i){
            result = WaitForMultipleObjects(1, &events[i], TRUE, 0);
            if (result == WAIT_FAILED || result == WAIT_TIMEOUT){
                continue;
            }
            WSAEnumNetworkEvents(clients_[i]->socket, clients_[i]->event, &ne);

            if (ne.lNetworkEvents & FD_READ){
                // receive data from client
                if (!clients_[i]->receive_data()){
                    clients_[i]->close();
                    didclose = true;
                }
            } else if (ne.lNetworkEvents & FD_CLOSE){
                // connection was closed
                int err = ne.iErrorCode[FD_CLOSE_BIT];
                LOG_VERBOSE("aoo_server: client connection was closed (" << err << ")");

                clients_[i]->close();
                didclose = true;
            } else {
                // ignore FD_WRITE
            }
        }
    }
#else
    // allocate three extra slots for master TCP socket, UDP socket and wait pipe
    int numfds = (int)(clients_.size() + 3);
    auto fds = (struct pollfd *)alloca(numfds * sizeof(struct pollfd));
    for (int i = 0; i < numfds; ++i){
        fds[i].events = POLLIN;
        fds[i].revents = 0;
    }
    int numclients = (int)clients_.size();
    for (int i = 0; i < numclients; ++i){
        fds[i].fd = clients_[i]->socket;
    }
    int tcpindex = numclients;
    int udpindex = numclients + 1;
    int waitindex = numclients + 2;
    fds[tcpindex].fd = tcpsocket_;
    fds[udpindex].fd = udpsocket_;
    fds[waitindex].fd = waitpipe_[0];

    // NOTE: macOS requires the negative timeout to be exactly -1!
    int result = poll(fds, numfds, -1);
    if (result < 0){
        int err = errno;
        if (err == EINTR){
            // ?
        } else {
            LOG_ERROR("aoo_server: poll failed (" << err << ")");
            // what to do?
        }
        return;
    }

    if (fds[waitindex].revents & POLLIN){
        // clear pipe
        char c;
        read(waitpipe_[0], &c, 1);
    }

    if (quit_.load()) {
        return;
    }
    
    if (fds[tcpindex].revents & POLLIN){
        // accept new clients
        while (true){
            ip_address addr;
            int sock = accept(tcpsocket_, (struct sockaddr *)&addr.address, &addr.length);
            if (sock >= 0){
                clients_.push_back(std::make_unique<client_endpoint>(*this, sock, addr));
                LOG_VERBOSE("aoo_server: accepted client (IP: "
                            << addr.name() << ", port: " << addr.port() << ")");
            } else {
                int err = socket_errno();
                if (err != EWOULDBLOCK){
                    LOG_ERROR("aoo_server: couldn't accept client (" << err << ")");
                }
                break;
            }
        }
    }

    if (fds[udpindex].revents & POLLIN){
        receive_udp();
    }


    for (int i = 0; i < numclients; ++i){
        if (fds[i].revents & POLLIN){
            // receive data from client
            if (!clients_[i]->receive_data()){
                clients_[i]->close();
                didclose = true;
            }
        }
    }
#endif

    if (didclose){
        update();
    }
}

void server::update(){
    // remove closed clients
    auto result = std::remove_if(clients_.begin(), clients_.end(),
                                 [](auto& c){ return !c->is_active(); });
    clients_.erase(result, clients_.end());
    // automatically purge stale users
    // LATER add an option so that users will persist
    for (auto it = users_.begin(); it != users_.end(); ){
        if (!(*it)->is_active()){
            it = users_.erase(it);
        } else {
            ++it;
        }
    }
    // automatically purge empty groups
    // LATER add an option so that groups will persist
    for (auto it = groups_.begin(); it != groups_.end(); ){
        if ((*it)->num_users() == 0){
            if ((*it)->is_public) {
                on_public_group_removed(*(*it));
            }

            it = groups_.erase(it);
        } else {
            ++it;
        }
    }
}

void server::receive_udp(){
    if (udpsocket_ < 0){
        return;
    }
    // read as much data as possible until recv() would block
    while (true){
        char buf[AOO_MAXPACKETSIZE];
        ip_address addr;
        int32_t result = recvfrom(udpsocket_, buf, sizeof(buf), 0,
                               (struct sockaddr *)&addr.address, &addr.length);
        if (result > 0){
            try {
                osc::ReceivedPacket packet(buf, result);
                osc::ReceivedMessage msg(packet);

                int32_t type;
                auto onset = aoonet_parse_pattern(buf, result, &type);
                if (!onset){
                    LOG_WARNING("aoo_server: not an AOO NET message!");
                    return;
                }

                if (type != AOO_TYPE_SERVER){
                    LOG_WARNING("aoo_server: not a client message!");
                    return;
                }

                handle_udp_message(msg, onset, addr);
            } catch (const osc::Exception& e){
                LOG_ERROR("aoo_server: exception in receive_udp: " << e.what());
            }
        } else if (result < 0){
            int err = socket_errno();
        #ifdef _WIN32
            if (err == WSAEWOULDBLOCK)
        #else
            if (err == EWOULDBLOCK)
        #endif
            {
            #if 0
                LOG_VERBOSE("aoo_server: recv() would block");
            #endif
            }
            else
            {
                // TODO handle error
                LOG_ERROR("aoo_server: recv() failed (" << err << ")");
            }
            return;
        }
    }
}

void server::send_udp_message(const char *msg, int32_t size,
                              const ip_address &addr)
{
    auto result = ::sendto(udpsocket_, msg, size, 0,
                          (struct sockaddr *)&addr.address, addr.length);
    if (result < 0){
        int err = socket_errno();
    #ifdef _WIN32
        if (err != WSAEWOULDBLOCK)
    #else
        if (err != EWOULDBLOCK)
    #endif
        {
            // TODO handle error
            LOG_ERROR("aoo_server: send() failed (" << err << ")");
        } else {
            LOG_VERBOSE("aoo_server: send() would block");
        }
    }
}

void server::handle_udp_message(const osc::ReceivedMessage &msg, int onset,
                                const ip_address& addr)
{
    auto pattern = msg.AddressPattern() + onset;
    LOG_DEBUG("aoo_server: handle client UDP message " << pattern);

    try {
        if (!strcmp(pattern, AOONET_MSG_PING)){
            // reply with /ping message
            char buf[512];
            osc::OutboundPacketStream reply(buf, sizeof(buf));
            reply << osc::BeginMessage(AOONET_MSG_CLIENT_PING)
                  << osc::EndMessage;

            send_udp_message(reply.Data(), (int32_t) reply.Size(), addr);
        } else if (!strcmp(pattern, AOONET_MSG_REQUEST)){
            // reply with /reply message
            char buf[512];
            osc::OutboundPacketStream reply(buf, sizeof(buf));
            reply << osc::BeginMessage(AOONET_MSG_CLIENT_REPLY)
                  << addr.name().c_str() << addr.port() << osc::EndMessage;

            send_udp_message(reply.Data(), (int32_t) reply.Size(), addr);
        } else {
            LOG_ERROR("aoo_server: unknown message " << pattern);
        }
    } catch (const osc::Exception& e){
        LOG_ERROR("aoo_server: exception on handling " << pattern
                  << " message: " << e.what());
    }
}

void server::signal(){
#ifdef _WIN32
    SetEvent(waitevent_);
#else
    write(waitpipe_[1], "\0", 1);
#endif
}

/*////////////////////////// user ///////////////////////////*/

void user::on_close(server& s){
    // disconnect user from groups
    for (auto& grp : groups_){
        grp->remove_user(*this);
        s.on_user_left_group(*this, *grp);
    }

    s.on_user_left(*this);

    groups_.clear();
    // clear endpoint so the server knows it can remove the user
    endpoint = nullptr;
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

client_endpoint::client_endpoint(server &s, int sock, const ip_address &addr)
    : server_(&s), socket(sock), addr_(addr)
{
    int val = 0;
    // NOTE: on POSIX systems, the socket returned by accept() does *not*
    // inherit file status flags and it might not inherit socket options, either.
#ifdef _WIN32
    event = WSACreateEvent();
    WSAEventSelect(socket, event, FD_READ | FD_WRITE | FD_CLOSE);
#else
    // set non-blocking
    // (this is not necessary on Windows, because WSAEventSelect will do it automatically)
    val = 1;
    if (ioctl(socket, FIONBIO, (char *)&val) < 0){
        int err = aoo::net::socket_errno();
        LOG_ERROR("client_endpoint: couldn't set socket to non-blocking (" << err << ")");
        close();
    }
#endif

    // set TCP_NODELAY
    val = 1;
    if (setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (char *)&val, sizeof(val)) < 0){
        LOG_WARNING("client_endpoint: couldn't set TCP_NODELAY");
        // ignore
    }

    // keepalive and user timeout
    int keepidle = 10;
    int keepinterval = 10;
    int keepcnt = 2;

    val = 1;
    if (setsockopt (socket, SOL_SOCKET, SO_KEEPALIVE, (char*) &val, sizeof (val)) < 0){
        LOG_WARNING("client_endpoint: couldn't set SO_KEEPALIVE");
        // ignore
    }
#ifdef TCP_KEEPIDLE
    if (setsockopt (socket, IPPROTO_TCP, TCP_KEEPIDLE, (char*) &keepidle, sizeof (keepidle)) < 0){
        LOG_WARNING("client_endpoint: couldn't set SO_KEEPIDLE");
        // ignore
    }
#elif defined(TCP_KEEPALIVE)
    if (setsockopt (socket, IPPROTO_TCP, TCP_KEEPALIVE, (char*) &keepidle, sizeof (keepidle)) < 0){
        LOG_WARNING("client_endpoint: couldn't set SO_KEEPALIVE");
        // ignore
    }
#endif

#ifdef TCP_KEEPINTVL
    if (setsockopt (socket, IPPROTO_TCP, TCP_KEEPINTVL, (char*) &keepinterval, sizeof (keepinterval)) < 0){
        LOG_WARNING("client_endpoint: couldn't set SO_KEEPINTVL");
        // ignore
    }
#endif

#ifdef TCP_KEEPCNT
    if (setsockopt (socket, IPPROTO_TCP, TCP_KEEPCNT, (char*) &keepcnt, sizeof (keepcnt)) < 0){
        LOG_WARNING("client_endpoint: couldn't set SO_KEEPCNT");
        // ignore
    }
#endif

#ifdef TCP_USER_TIMEOUT
    int utimeout = (keepidle + keepinterval * keepcnt - 1) * 1000;
    // attempt to set TCP_USER_TIMEOUT option
    if (setsockopt (socket, IPPROTO_TCP, TCP_USER_TIMEOUT, (char*) &utimeout, sizeof (utimeout)) < 0){
        LOG_WARNING("client_endpoint: couldn't set TCP_USER_TIMEOUT");
        // ignore
    }
#endif

    sendbuffer_.setup(65536);
    recvbuffer_.setup(65536);

    // generate random token
    //std::random_device randdev;
    //std::default_random_engine reng(randdev());
    //std::uniform_int_distribution<int64_t> uniform_dist(1); // minimum of 1, max of maxint
    //token = uniform_dist(reng);
}

client_endpoint::~client_endpoint(){
#ifdef _WIN32
    WSACloseEvent(event);
#endif
    close();
}

void client_endpoint::close(bool notify){
    if (socket >= 0){
        LOG_VERBOSE("aoo_server: close client endpoint");
        socket_close(socket);
        socket = -1;

        if (user_ && notify){
            user_->on_close(*server_);
        }
    }
}

void client_endpoint::send_message(const char *msg, int32_t size){
    if (sendbuffer_.write_packet((const uint8_t *)msg, size)){
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
                auto res = ::send(socket, (char *)buf + nbytes, total - nbytes, 0);
                if (res >= 0){
                    nbytes += res;
                #if 0
                    LOG_VERBOSE("aoo_server: sent " << res << " bytes");
                #endif
                } else {
                    auto err = socket_errno();
                #ifdef _WIN32
                    if (err != WSAEWOULDBLOCK)
                #else
                    if (err != EWOULDBLOCK)
                #endif
                    {
                        // TODO handle error
                        LOG_ERROR("aoo_server: send() failed (" << err << ")");
                    } else {
                        // store in pending buffer
                        pending_send_data_.assign(buf + nbytes, buf + total);
                        LOG_VERBOSE("aoo_server: send() would block");
                    }
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
    // read as much data as possible until recv() would block
    while (true){
        char buffer[AOO_MAXPACKETSIZE];
        auto result = recv(socket, buffer, sizeof(buffer), 0);
        if (result == 0){
            LOG_WARNING("client_endpoint: connection was closed");
            return false;
        }
        if (result < 0){
            int err = socket_errno();
        #ifdef _WIN32
            if (err == WSAEWOULDBLOCK)
        #else
            if (err == EWOULDBLOCK)
        #endif
            {
            #if 0
                LOG_VERBOSE("client_endpoint: recv() would block");
            #endif
                return true;
            }
            else
            {
                // TODO handle error
                LOG_ERROR("client_endpoint: recv() failed (" << err << ")");
                return false;
            }
        }

        recvbuffer_.write_bytes((uint8_t *)buffer, (int32_t)result);

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
                                handle_message(msg);
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
                        handle_message(msg);
                    } else if (packet.IsBundle()){
                        osc::ReceivedBundle bundle(packet);
                        dispatchBundle(bundle);
                    } else {
                        // ignore
                    }
                } catch (const osc::Exception& e){
                    LOG_ERROR("aoo_server: exception in client_endpoint::receive_data: " << e.what());
                }
            } else {
                break;
            }
        }
    }
}

void client_endpoint::handle_message(const osc::ReceivedMessage &msg){
    // first check main pattern
    int32_t len = (int32_t) strlen(msg.AddressPattern());
    int32_t onset = AOO_MSG_DOMAIN_LEN + AOONET_MSG_SERVER_LEN;

    if ((len < onset) ||
        memcmp(msg.AddressPattern(), AOO_MSG_DOMAIN AOONET_MSG_SERVER, onset))
    {
        LOG_ERROR("aoo_server: received bad message " << msg.AddressPattern()
                  << " from client");
        return;
    }

    // now compare subpattern
    auto pattern = msg.AddressPattern() + onset;
    LOG_DEBUG("aoo_server: got message " << pattern);

    try {
        if (!strcmp(pattern, AOONET_MSG_PING)){
            handle_ping(msg);
        } else if (!strcmp(pattern, AOONET_MSG_LOGIN)){
            handle_login(msg);
        } else if (!strcmp(pattern, AOONET_MSG_GROUP_JOIN)){
            handle_group_join(msg);
        } else if (!strcmp(pattern, AOONET_MSG_GROUP_LEAVE)){
            handle_group_leave(msg);
        } else if (!strcmp(pattern, AOONET_MSG_GROUP_PUBLIC)){
            handle_group_public(msg);
        } else {
            LOG_ERROR("aoo_server: unknown message " << msg.AddressPattern());
        }
    } catch (const osc::Exception& e){
        LOG_ERROR("aoo_server: exception on handling " << msg.AddressPattern()
                  << " message: " << e.what());
    }
}

void client_endpoint::handle_ping(const osc::ReceivedMessage& msg){
    // send reply
    char buf[AOO_MAXPACKETSIZE];
    osc::OutboundPacketStream reply(buf, sizeof(buf));
    reply << osc::BeginMessage(AOONET_MSG_CLIENT_PING) << osc::EndMessage;

    send_message(reply.Data(), (int32_t)reply.Size());
}

void client_endpoint::handle_login(const osc::ReceivedMessage& msg)
{
    int32_t result = 0;
    std::string errmsg;

    auto it = msg.ArgumentsBegin();
    std::string username = (it++)->AsString();
    std::string password = (it++)->AsString();
    std::string public_ip = (it++)->AsString();
    int32_t public_port = (it++)->AsInt32();
    std::string local_ip = (it++)->AsString();
    int32_t local_port = (it++)->AsInt32();
    int64_t ctoken = msg.ArgumentCount() > 6 ? (it++)->AsInt64() : 0;
    
    if (ctoken) {
        token = ctoken;
    }
    
    server::error err;
    if (!user_){
        user_ = server_->get_user(username, password, err);
        if (user_){
            // success
            public_address = ip_address(public_ip, public_port);
            local_address = ip_address(local_ip, local_port);
            user_->endpoint = this;

            LOG_VERBOSE("aoo_server: login: "
                        << "username: " << username << ", password: " << password
                        << ", public IP: " << public_ip << ", public port: " << public_port
                        << ", local IP: " << local_ip << ", local port: " << local_port << ", token: " << token);

            result = 1;

            server_->on_user_joined(*user_);
        } else {
            errmsg = server::error_to_string(err);
        }
    } else {
        errmsg = "already logged in"; // shouldn't happen
    }

    // send reply
    char buf[AOO_MAXPACKETSIZE];
    osc::OutboundPacketStream reply(buf, sizeof(buf));
    reply << osc::BeginMessage(AOONET_MSG_CLIENT_LOGIN)
          << result << errmsg.c_str() << osc::EndMessage;

    send_message(reply.Data(), (int32_t)reply.Size());
}

void client_endpoint::handle_group_join(const osc::ReceivedMessage& msg)
{
    int result = 0;
    std::string errmsg;

    auto it = msg.ArgumentsBegin();
    std::string name = (it++)->AsString();
    std::string password = (it++)->AsString();

    bool is_public = false;
    if (msg.ArgumentCount() > 2) {
        is_public = (it++)->AsBool();
    }

    server::error err;
    if (user_){
        auto grp = server_->get_group(name, password, is_public, err);
        if (grp){
            if (user_->add_group(grp)){
                grp->add_user(user_);
                server_->on_user_joined_group(*user_, *grp);
                result = 1;
            } else {
                errmsg = "already a group member";
            }
        } else {
            errmsg = server::error_to_string(err);
        }
    } else {
        errmsg = "not logged in";
    }

    // send reply
    char buf[AOO_MAXPACKETSIZE];
    osc::OutboundPacketStream reply(buf, sizeof(buf));
    reply << osc::BeginMessage(AOONET_MSG_CLIENT_GROUP_JOIN)
          << name.c_str() << result << errmsg.c_str() << osc::EndMessage;

    send_message(reply.Data(), (int32_t)reply.Size());
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
    reply << osc::BeginMessage(AOONET_MSG_CLIENT_GROUP_LEAVE)
          << name.c_str() << result << errmsg.c_str() << osc::EndMessage;

    send_message(reply.Data(), (int32_t)reply.Size());
}

void client_endpoint::handle_group_public(const osc::ReceivedMessage& msg)
{
    int result = 0;
    std::string errmsg;

    auto it = msg.ArgumentsBegin();
    bool shouldWatch = (it++)->AsBool();

    server::error err;
    if (user_){
        // register interest in seeing public groups
        user_->watch_public_groups = shouldWatch;

        if (shouldWatch) {
            // send current batch
            server_->on_user_wants_public_groups(*user_);
        }
    } else {
        errmsg = "not logged in";
    }

    // send reply
    char buf[AOO_MAXPACKETSIZE];
    osc::OutboundPacketStream reply(buf, sizeof(buf));
    reply << osc::BeginMessage(AOONET_MSG_CLIENT_GROUP_PUBLIC)
          << shouldWatch << result << errmsg.c_str() << osc::EndMessage;

    send_message(reply.Data(), (int32_t)reply.Size());
}

/*///////////////////// events ////////////////////////*/

server::event::event(int32_t type, int32_t result,
                     const char * errmsg)
{
    server_event_.type = type;
    server_event_.result = result;
    server_event_.errormsg = copy_string(errmsg);
}

server::event::~event()
{
    delete server_event_.errormsg;
}

server::user_event::user_event(int32_t type, const char *name)
{
    user_event_.type = type;
    user_event_.name = copy_string(name);
}

server::user_event::~user_event()
{
    delete user_event_.name;
}

server::group_event::group_event(int32_t type,
                         const char *group, const char *user)
{
    group_event_.type = type;
    group_event_.group = copy_string(group);
    group_event_.user = copy_string(user);
}

server::group_event::~group_event()
{
    delete group_event_.group;
    delete group_event_.user;
}

} // net
} // aoo
