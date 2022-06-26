#include "AooServer.hpp"

#include <unordered_map>
#include <stdexcept>

namespace {

InterfaceTable *ft;

using scoped_lock = aoo::sync::scoped_lock<aoo::sync::shared_mutex>;
using scoped_shared_lock = aoo::sync::scoped_shared_lock<aoo::sync::shared_mutex>;

aoo::sync::shared_mutex gServerMutex;

using ServerMap = std::unordered_map<int, std::unique_ptr<sc::AooServer>>;
std::unordered_map<World*, ServerMap> gServerMap;

// called from NRT thread
void addServer(World* world, int port, std::unique_ptr<sc::AooServer> server) {
    scoped_lock lock(gServerMutex);
    auto& serverMap = gServerMap[world];
    serverMap[port] = std::move(server);
}

// called from NRT thread
void freeServer(World* world, int port) {
    scoped_lock lock(gServerMutex);
    auto it = gServerMap.find(world);
    if (it != gServerMap.end()) {
        auto& serverMap = it->second;
        serverMap.erase(port);
    }
}

sc::AooServer* findServer(World* world, int port) {
    scoped_shared_lock lock(gServerMutex);
    auto it = gServerMap.find(world);
    if (it != gServerMap.end()) {
        auto& serverMap = it->second;
        auto it2 = serverMap.find(port);
        if (it2 != serverMap.end()) {
            return it2->second.get();
        }
    }
    return nullptr;
}

} // namespace

namespace sc {

// called in NRT thread
AooServer::AooServer(World *world, int port, const char *password)
    : world_(world), port_(port)
{
    // setup UDP server
    udpserver_.start(port,
                     [this](auto&&... args) { handleUdpReceive(args...); });

    // setup TCP server
    tcpserver_.start(port,
                     [this](auto&&... args) { return handleAccept(args...); },
                     [this](auto&&... args) { return handleReceive(args...); });

    // success
    server_ = ::AooServer::create(0, nullptr);
    if (password) {
        server_->setPassword(password);
    }
    LOG_VERBOSE("AooServer: listening on port " << port);
    // first set event handler!
    server_->setEventHandler([](void *x, const AooEvent *e, AooThreadLevel) {
        static_cast<sc::AooServer *>(x)->handleEvent(e);
    }, this, kAooEventModeCallback);
    // then start network threads
    udpthread_ = std::thread([this](){
        udpserver_.run(-1);
    });
    tcpthread_ = std::thread([this](){
        tcpserver_.run();
    });
}

AooServer::~AooServer(){
    if (server_) {
        udpserver_.stop();
        if (udpthread_.joinable()) {
            udpthread_.join();
        }

        tcpserver_.stop();
        if (tcpthread_.joinable()) {
            tcpthread_.join();
        }
    }
}

void AooServer::handleEvent(const AooEvent *event){
    char buf[1024];
    osc::OutboundPacketStream msg(buf, sizeof(buf));
    msg << osc::BeginMessage("/aoo/server/event") << port_;

    switch (event->type) {
    case kAooNetEventServerClientLogin:
    {
        auto e = (const AooNetEventServerClientLogin *)event;
        aoo::ip_address addr;
        aoo::socket_peer(e->sockfd, addr);
        msg << "/client/add" << e->id << addr.address() << addr.port();
        break;
    }
    case kAooNetEventServerClientRemove:
    {
        auto e = (const AooNetEventServerClientRemove *)event;
        msg << "/client/remove" << e->id;
        break;
    }
    case kAooNetEventServerGroupAdd:
    {
        auto e = (const AooNetEventServerGroupAdd *)event;
        msg << "/group/add" << e->id << e->name; // TODO metadata
        break;
    }
    case kAooNetEventServerGroupRemove:
    {
        auto e = (const AooNetEventServerGroupRemove *)event;
        msg << "/group/remove" << e->id;
        break;
    }
    case kAooNetEventServerGroupJoin:
    {
        auto e = (const AooNetEventServerGroupJoin *)event;
        msg << "/group/join" << e->groupId << e->userId << e->userName << e->clientId;
        break;
    }
    case kAooNetEventServerGroupLeave:
    {
        auto e = (const AooNetEventServerGroupLeave *)event;
        msg << "/group/leave" << e->groupId << e->userId;
        break;
    }
    case kAooNetEventError:
    {
        auto e = (const AooNetEventError*)event;
        msg << "/error" << e->errorCode << e->errorMessage;
        break;
    }
    default:
        LOG_DEBUG("AooServer: got unknown event " << event->type);
        return; // don't send event!
    }

    msg << osc::EndMessage;
    ::sendMsgNRT(world_, msg);
}

AooId AooServer::handleAccept(int e, const aoo::ip_address& addr, AooSocket sock) {
    if (e == 0) {
        // reply function
        auto replyfn = [](void *x, AooId client,
                const AooByte *data, AooSize size) -> AooInt32 {
            return static_cast<aoo::tcp_server *>(x)->send(client, data, size);
        };
        AooId client;
        server_->addClient(replyfn, &tcpserver_, sock, &client); // doesn't fail
        return client;
    } else {
        LOG_ERROR("AooServer: accept() failed: " << aoo::socket_strerror(e));
        // TODO handle error?
        return kAooIdInvalid;
    }
}

void AooServer::handleReceive(AooId client, int e, const AooByte *data, AooSize size) {
    if (e == 0 && size > 0) {
        if (server_->handleClientMessage(client, data, size) != kAooOk) {
            server_->removeClient(client);
            tcpserver_.close(client);
        }
    } else {
        // remove client!
        server_->removeClient(client);
        if (e == 0) {
            LOG_VERBOSE("AooServer: client " << client << " disconnected");
        } else {
            LOG_ERROR("AooServer: TCP error in client "
                      << client << ": " << aoo::socket_strerror(e));
        }
    }
}

void AooServer::handleUdpReceive(int e, const aoo::ip_address& addr,
                                 const AooByte *data, AooSize size) {
    if (e == 0) {
        // reply function
        auto replyfn = [](void *x, const AooByte *data, AooInt32 size,
                const void *address, AooAddrSize addrlen, AooFlag) -> AooInt32 {
            aoo::ip_address addr((const struct sockaddr *)address, addrlen);
            return static_cast<aoo::udp_server *>(x)->send(addr, data, size);
        };
        server_->handleUdpMessage(data, size, addr.address(), addr.length(),
                                  replyfn, &udpserver_);
    } else {
        LOG_ERROR("AooServer: UDP error: " << aoo::socket_strerror(e));
        // TODO handle error?
    }
}

} // sc

namespace {

template<typename T>
void doCommand(World* world, void *replyAddr, T* cmd, AsyncStageFn fn) {
    DoAsynchronousCommand(world, replyAddr, 0, cmd,
        fn, 0, 0, CmdData::free<T>, 0, 0);
}

void aoo_server_new(World* world, void* user,
    sc_msg_iter* args, void* replyAddr)
{
    auto port = args->geti();
    auto pwd = args->gets("");

    auto cmdData = CmdData::create<sc::AooServerCreateCmd>(world);
    if (cmdData) {
        cmdData->port = port;
        snprintf(cmdData->password, sizeof(cmdData->password), "%s", pwd);

        auto fn = [](World* world, void* x) {
            auto data = (sc::AooServerCreateCmd *)x;
            auto port = data->port;
            auto pwd = data->password;

            char buf[1024];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            msg << osc::BeginMessage("/aoo/server/new") << port;

            if (findServer(world, port)) {
                char errbuf[256];
                snprintf(errbuf, sizeof(errbuf),
                    "AooServer on port %d already exists.", port);
                msg << 0 << errbuf;
            } else {
                try {
                    auto server = std::make_unique<sc::AooServer>(world, port, pwd);
                    addServer(world, port, std::move(server));
                    msg << 1; // success
                } catch (const std::runtime_error& e){
                    msg << 0 << e.what();
                }
            }

            msg << osc::EndMessage;
            ::sendMsgNRT(world, msg);

            return false; // done
        };

        doCommand(world, replyAddr, cmdData, fn);
    }
}

void aoo_server_free(World* world, void* user,
    sc_msg_iter* args, void* replyAddr)
{
    auto port = args->geti();

    auto cmdData = CmdData::create<sc::AooServerCmd>(world);
    if (cmdData) {
        cmdData->port = port;

        auto fn = [](World * world, void* data) {
            auto port = static_cast<sc::AooServerCmd*>(data)->port;

            freeServer(world, port);

            char buf[1024];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            msg << osc::BeginMessage("/aoo/server/free")
                << port << osc::EndMessage;
            ::sendMsgNRT(world, msg);

            return false; // done
        };

        doCommand(world, replyAddr, cmdData, fn);
    }
}

} // namespace

/*////////////// Setup /////////////////*/

void AooServerLoad(InterfaceTable *inTable){
    ft = inTable;

    AooPluginCmd(aoo_server_new);
    AooPluginCmd(aoo_server_free);
}
