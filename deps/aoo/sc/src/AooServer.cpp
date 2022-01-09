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
void createServer(World* world, int port) {
    auto server = std::make_unique<sc::AooServer>(world, port);
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
AooServer::AooServer(World *world, int port)
    : world_(world), port_(port)
{
    AooError err;
    server_ = ::AooServer::create(port, 0, &err);
    if (server_) {
        LOG_VERBOSE("new AooServer on port " << port);
        // start thread
        thread_ = std::thread([this]() {
            server_->run();
        });
    } else {
        throw std::runtime_error(aoo::socket_strerror(aoo::socket_errno()));
    }
}

AooServer::~AooServer(){
    if (server_) {
        server_->quit();
        // wait for thread to finish
        thread_.join();
    }
}

void AooServer::handleEvent(const AooEvent *event){
    char buf[1024];
    osc::OutboundPacketStream msg(buf, sizeof(buf));
    msg << osc::BeginMessage("/aoo/server/event") << port_;

    switch (event->type) {
    case kAooNetEventUserJoin:
    {
        auto e = (const AooNetEventUserJoin *)event;
        aoo::ip_address addr((const sockaddr*)e->address, e->addrlen);
        msg << "/user/join" << e->userName << e->userId
            << addr.address() << addr.port();
        break;
    }
    case kAooNetEventUserLeave:
    {
        auto e = (const AooNetEventUserLeave *)event;
        aoo::ip_address addr((const sockaddr*)e->address, e->addrlen);
        msg << "/user/leave" << e->userName << e->userId
            << addr.address() << addr.port();
        break;
    }
    case kAooNetEventUserGroupJoin:
    {
        auto e = (const AooNetEventUserGroupJoin *)event;
        msg << "/group/join" << e->groupName << e->userName << e->userId;
        break;
    }
    case kAooNetEventUserGroupLeave:
    {
        auto e = (const AooNetEventUserGroupLeave *)event;
        msg << "/group/leave" << e->groupName << e->userName << e->userId;
        break;
    }
    case kAooNetEventError:
    {
        auto e = (const AooNetEventError*)event;
        msg << "/error" << e->errorCode << e->errorMessage;
        break;
    }
    default:
        LOG_ERROR("AooServer: got unknown event " << event->type);
        return; // don't send event!
    }

    msg << osc::EndMessage;
    ::sendMsgNRT(world_, msg);
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

    auto cmdData = CmdData::create<sc::AooServerCmd>(world);
    if (cmdData) {
        cmdData->port = port;

        auto fn = [](World* world, void* data) {
            auto port = static_cast<sc::AooServerCmd*>(data)->port;

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
                    createServer(world, port);
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
