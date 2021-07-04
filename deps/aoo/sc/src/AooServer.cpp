#include "AooServer.hpp"

#include <unordered_map>
#include <stdexcept>

namespace {

InterfaceTable *ft;

using scoped_lock = aoo::sync::scoped_lock<aoo::sync::shared_mutex>;
using scoped_shared_lock = aoo::sync::scoped_shared_lock<aoo::sync::shared_mutex>;

aoo::sync::shared_mutex gServerMutex;

using ServerMap = std::unordered_map<int, std::unique_ptr<AooServer>>;
std::unordered_map<World*, ServerMap> gServerMap;

// called from NRT thread
void createServer(World* world, int port) {
    auto server = std::make_unique<AooServer>(world, port);
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

AooServer* findServer(World* world, int port) {
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

// called in NRT thread
AooServer::AooServer(World *world, int port)
    : world_(world), port_(port)
{
    aoo_error err;
    auto server = aoo::net::server::create(port, 0, &err);
    server_.reset(server);
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

void AooServer::handleEvent(const aoo_event *event){
    char buf[1024];
    osc::OutboundPacketStream msg(buf, sizeof(buf));
    msg << osc::BeginMessage("/aoo/server/event") << port_;

    switch (event->type) {
    case AOO_NET_USER_JOIN_EVENT:
    {
        auto e = (const aoo_net_user_event*)event;
        aoo::ip_address addr((const sockaddr*)e->address, e->addrlen);
        msg << "/user/join" << e->user_name << e->user_id
            << addr.address() << addr.port();
        break;
    }
    case AOO_NET_USER_LEAVE_EVENT:
    {
        auto e = (const aoo_net_user_event*)event;
        aoo::ip_address addr((const sockaddr*)e->address, e->addrlen);
        msg << "/user/leave" << e->user_name << e->user_id
            << addr.address() << addr.port();
        break;
    }
    case AOO_NET_GROUP_JOIN_EVENT:
    {
        auto e = (const aoo_net_group_event*)event;
        msg << "/group/join" << e->group_name << e->user_name << e->user_id;
        break;
    }
    case AOO_NET_GROUP_LEAVE_EVENT:
    {
        auto e = (const aoo_net_group_event*)event;
        msg << "/group/leave" << e->group_name << e->user_name << e->user_id;
        break;
    }
    case AOO_NET_ERROR_EVENT:
    {
        auto e = (const aoo_net_error_event*)event;
        msg << "/error" << e->error_code << e->error_message;
        break;
    }
    default:
        LOG_ERROR("AooServer: got unknown event " << event->type);
        return; // don't send event!
    }

    msg << osc::EndMessage;
    ::sendMsgNRT(world_, msg);
}

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

    auto cmdData = CmdData::create<AooServerCmd>(world);
    if (cmdData) {
        cmdData->port = port;

        auto fn = [](World* world, void* data) {
            auto port = static_cast<AooServerCmd*>(data)->port;

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

    auto cmdData = CmdData::create<AooServerCmd>(world);
    if (cmdData) {
        cmdData->port = port;

        auto fn = [](World * world, void* data) {
            auto port = static_cast<AooServerCmd*>(data)->port;

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
