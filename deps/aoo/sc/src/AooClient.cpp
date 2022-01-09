#include "AooClient.hpp"

#include <unordered_map>

namespace {

using scoped_lock = aoo::sync::scoped_lock<aoo::sync::shared_mutex>;
using scoped_shared_lock = aoo::sync::scoped_shared_lock<aoo::sync::shared_mutex>;

InterfaceTable *ft;

aoo::sync::shared_mutex gClientMutex;

using ClientMap = std::unordered_map<int, std::unique_ptr<sc::AooClient>>;
std::unordered_map<World*, ClientMap> gClientMap;

// called from NRT thread
void createClient(World* world, int port) {
    auto client = std::make_unique<sc::AooClient>(world, port);
    scoped_lock lock(gClientMutex);
    auto& clientMap = gClientMap[world];
    clientMap[port] = std::move(client);
}

// called from NRT thread
void freeClient(World* world, int port) {
    scoped_lock lock(gClientMutex);
    auto it = gClientMap.find(world);
    if (it != gClientMap.end()) {
        auto& clientMap = it->second;
        clientMap.erase(port);
    }
}

sc::AooClient* findClient(World* world, int port) {
    scoped_shared_lock lock(gClientMutex);
    auto it = gClientMap.find(world);
    if (it != gClientMap.end()) {
        auto& clientMap = it->second;
        auto it2 = clientMap.find(port);
        if (it2 != clientMap.end()) {
            return it2->second.get();
        }
    }
    return nullptr;
}

} // namespace

namespace sc {

// called in NRT thread
AooClient::AooClient(World *world, int32_t port)
    : world_(world) {
    auto node = INode::get(world, port);
    if (node) {
        if (node->registerClient(this)) {
            LOG_VERBOSE("new AooClient on port " << port);
            node_ = node;
        }
    }
}

// called in NRT thread
AooClient::~AooClient() {
    if (node_) {
        node_->unregisterClient(this);
    }

    LOG_DEBUG("~AooClient");
}

void AooClient::connect(const char* host, int port,
                        const char* user, const char* pwd) {
    if (!node_){
        sendReply("/aoo/client/connect", false);
        return;
    }

    auto cb = [](void* x, AooError result, const void* data) {
        auto client = (AooClient*)x;

        char buf[1024];
        osc::OutboundPacketStream msg(buf, sizeof(buf));
        msg << osc::BeginMessage("/aoo/client/connect") << client->node_->port();

        if (result == kAooOk) {
            auto reply = (const AooNetReplyConnect *)data;
            // send success + ID
            msg << 1 << reply->userId;
        } else {
            auto e = (const AooNetReplyError *)data;
            // send fail + error message
            msg << 0 << e->errorMessage;
        }

        msg << osc::EndMessage;

        ::sendMsgNRT(client->world_, msg);
    };
    node_->client()->connect(host, port, user, pwd, cb, this);
}

void AooClient::disconnect() {
    if (!node_) {
        sendReply("/aoo/client/disconnect", false);
        return;
    }

    auto cb = [](void* x, AooError result, const void* data) {
        auto client = (AooClient*)x;
        if (result == kAooOk) {
            client->sendReply("/aoo/client/disconnect", true);
        } else {
            auto e = (const AooNetReplyError *)data;
            client->sendReply("/aoo/client/disconnect", false, e->errorMessage);
        }
    };
    node_->client()->disconnect(cb, this);
}

void AooClient::joinGroup(const char* name, const char* pwd) {
    if (!node_) {
        sendGroupReply("/aoo/client/group/join", name, false);
        return;
    }

    auto cb = [](void* x, AooError result, const void* data) {
        auto request = (GroupRequest *)x;
        if (result == kAooOk) {
            request->obj->sendGroupReply("/aoo/client/group/join",
                request->group.c_str(), true);
        } else {
            auto e = (const AooNetReplyError *)data;
            request->obj->sendGroupReply("/aoo/client/group/join",
                request->group.c_str(), false, e->errorMessage);
        }
        delete request;
    };
    node_->client()->joinGroup(name, pwd, cb, new GroupRequest{ this, name });
}

void AooClient::leaveGroup(const char* name) {
    if (!node_) {
        sendGroupReply("/aoo/client/group/leave", name, false);
        return;
    }

    auto cb = [](void* x, AooError result, const void* data) {
        auto request = (GroupRequest*)x;
        if (result == kAooOk) {
            request->obj->sendGroupReply("/aoo/client/group/leave",
                request->group.c_str(), true);
        } else {
            auto e = (const AooNetReplyError*)data;
            request->obj->sendGroupReply("/aoo/client/group/leave",
                request->group.c_str(), false, e->errorMessage);
        }
        delete request;
    };
    node_->client()->leaveGroup(name, cb, new GroupRequest{ this, name });
}

// called from network thread
void AooClient::handleEvent(const AooEvent* event) {
    char buf[1024];
    osc::OutboundPacketStream msg(buf, sizeof(buf));
    msg << osc::BeginMessage("/aoo/client/event") << node_->port();

    switch (event->type) {
    case kAooNetEventPeerMessage:
    {
        auto e = (const AooNetEventPeerMessage *)event;

        aoo::ip_address address((const sockaddr *)e->address, e->addrlen);

        try {
            osc::ReceivedPacket packet((const char *)e->data, e->size);
            if (packet.IsBundle()){
                osc::ReceivedBundle bundle(packet);
                handlePeerBundle(bundle, address);
            } else {
                handlePeerMessage(packet.Contents(), packet.Size(),
                                  address, aoo::time_tag::immediate());
            }
        } catch (const osc::Exception &err){
            LOG_ERROR("AooClient: bad OSC message - " << err.what());
        }
        return; // don't send event
    }
    case kAooNetEventDisconnect:
    {
        msg << "/disconnect";
        break;
    }
    case kAooNetEventPeerJoin:
    {
        auto e = (const AooNetEventPeer*)event;
        aoo::ip_address addr((const sockaddr*)e->address, e->addrlen);
        msg << "/peer/join" << addr.name() << addr.port()
            << e->groupName << e->userName << e->userId;
        break;
    }
    case kAooNetEventPeerLeave:
    {
        auto e = (const AooNetEventPeer*)event;
        aoo::ip_address addr((const sockaddr*)e->address, e->addrlen);
        msg << "/peer/leave" << addr.name() << addr.port()
            << e->groupName << e->userName << e->userId;
        break;
    }
    case kAooNetEventError:
    {
        auto e = (const AooNetEventError*)event;
        msg << "/error" << e->errorCode << e->errorMessage;
        break;
    }
    default:
        LOG_ERROR("AooClient: got unknown event " << event->type);
        return; // don't send event!
    }

    msg << osc::EndMessage;
    ::sendMsgNRT(world_, msg);
}

void AooClient::sendReply(const char *cmd, bool success,
                          const char *errmsg){
    char buf[1024];
    osc::OutboundPacketStream msg(buf, sizeof(buf));
    msg << osc::BeginMessage(cmd) << node_->port() << (int)success;
    if (errmsg){
        msg << errmsg;
    }
    msg << osc::EndMessage;

    ::sendMsgNRT(world_, msg);
}

void AooClient::sendGroupReply(const char* cmd, const char* group,
                               bool success, const char* errmsg)
{
    char buf[1024];
    osc::OutboundPacketStream msg(buf, sizeof(buf));
    msg << osc::BeginMessage(cmd) << node_->port() << group << (int)success;
    if (errmsg) {
        msg << errmsg;
    }
    msg << osc::EndMessage;

    ::sendMsgNRT(world_, msg);
}

void AooClient::forwardMessage(const char *data, int32_t size,
                               aoo::time_tag time)
{
    // We use sc_msg_iter because we need it for getPeerArg().
    // Also, it is a bit easier to use in this case.

    // skip '/sc/msg' (8 bytes)
    sc_msg_iter args(size - 8, data + 8);

    // get message
    if (args.nextTag() != 'b'){
        return;
    }
    auto msgSize = args.getbsize();
    if (msgSize <= 0){
        return;
    }

    auto msg = args.rdpos + 4;
    args.skipb();

    // rewrite OSC bundle timetag
    // NOTE: we know that SCLang can't send nested bundles
    if (!time.is_immediate() && msgSize > 16 && !memcmp("#bundle", msg, 8)){
        aoo::time_tag relTime(OSCtime(msg + 8));
        // don't rewrite immediate bundles!
        if (!relTime.is_immediate()){
            auto absTime = time + relTime;
        #if 0
            LOG_DEBUG("time: " << time << ", rel: " << relTime
                      << ", abs: " << absTime);
        #endif
            // we know that the original buffer is not really constant
            aoo::to_bytes((uint64_t)absTime, (AooByte *)msg + 8);
        }
    }

    uint32_t flags = args.geti();

    // get target
    if (args.remain() > 0){
        auto count = args.count;
        if (args.tags[count] && args.tags[count + 1]) {
            // peer: host|port or group|user
            aoo::ip_address addr;
            auto success = node_->getPeerArg(&args, addr);
            if (success) {
                node_->client()->sendPeerMessage((const AooByte *)msg,
                    msgSize, addr.address(), addr.length(), flags);
                node_->notify();
            }
        } else {
            // group name
            auto group = args.gets("");
            node_->client()->sendPeerMessage((const AooByte *)msg,
                                             msgSize, group, 0, flags);
            node_->notify();
        }
    } else {
        // broadcast
        node_->client()->sendPeerMessage((const AooByte *)msg,
                                         msgSize, 0, 0, flags);
        node_->notify();
    }
}

constexpr int32_t round4(int32_t n){
    return (n + 3) & ~3;
}

// Called with NRT lock (see handleEvent()), although probably
// not necessary...
#if 0
void AooClient::handlePeerMessage(const char *msg, int32_t size,
                                  const aoo::ip_address& address, aoo::time_tag time)
{
    // wrap message in bundle
    char buf[64];
    osc::OutboundPacketStream info(buf, sizeof(buf));
    info << osc::BeginMessage("/aoo/addr")
        << address.name() << address.port() << osc::EndMessage;

    const int headerSize = 16;

    auto bundleSize = headerSize + 4 + info.Size() + 4 + size;
    auto bundleData = (char *)alloca(bundleSize);
    auto ptr = bundleData;
    memcpy(ptr, "#bundle\0", 8);
    aoo::to_bytes<uint64_t>(time, ptr + 8);
    ptr += headerSize;

    aoo::to_bytes<int32_t>(info.Size(), ptr);
    memcpy(ptr + 4, info.Data(), info.Size());
    ptr += info.Size() + 4;

    aoo::to_bytes<int32_t>(size, ptr);
    memcpy(ptr + 4, msg, size);

    ::sendMsgNRT(world_, bundleData, bundleSize);
}
#else
void AooClient::handlePeerMessage(const char *data, int32_t size,
                                  const aoo::ip_address& address, aoo::time_tag time)
{
    LOG_DEBUG("handlePeerMessage: " << data);
    bool isBundle = !time.is_immediate();

    auto typeTags = OSCstrskip(data);

    auto oldPattern = data;
    auto oldPatternSize = typeTags - data;

    auto argumentData = OSCstrskip(typeTags);
    auto argumentDataSize = (data + size) - argumentData;

    // IPv6 address strings can be up to 45 characters!
    auto buf = (char *)alloca(size + 128);
    auto ptr = buf;
    char * msgPtr = nullptr;

    // SCLang can't handle immediate bundles,
    // so we have to send OSC messages instead
    if (isBundle){
        // write bundle + time tag
        memcpy(ptr, "#bundle\0", 8);
        aoo::to_bytes<uint64_t>(time, ptr + 8);
        ptr += 16;

        // skip message size
        ptr += 4;
        msgPtr = ptr;
    }

    // write message
    memcpy(ptr, "/aoo/msg\0\0\0\0", 12);
    ptr += 12;

    // write type tags
    // prepend port, source host, source port, original address pattern
    auto count = sprintf(ptr, ",%s%s", "isis", typeTags + 1);
    auto typeTagSize = round4(count + 1);
    memset(ptr + count, 0, typeTagSize - count);
    ptr += typeTagSize;

    // prepend new arguments
    // 1) local port
    aoo::to_bytes<int32_t>(node_->port(), ptr);
    ptr += 4;
    // 2) remote address (needs padding!)
    auto addressSize = round4(strlen(address.name()) + 1);
    strncpy(ptr, address.name(), addressSize);
    ptr += addressSize;
    // 3) remote port
    aoo::to_bytes<int32_t>(address.port(), ptr);
    ptr += 4;
    // 4) original address pattern (OSC string)
    memcpy(ptr, oldPattern, oldPatternSize);
    ptr += oldPatternSize;

    // original argument data
    memcpy(ptr, argumentData, argumentDataSize);
    ptr += argumentDataSize;

    if (isBundle){
        // finally write OSC message size:
        auto msgSize = ptr - msgPtr;
        aoo::to_bytes<int32_t>(msgSize, msgPtr - 4);
    }

    auto newSize = ptr - buf;
#if 0
    std::stringstream ss;
    for (int i = 0; i < newSize; ++i){
        ss << (int32_t)(uint8_t)buf[i] << " ";
    }
    LOG_DEBUG("size: " << newSize << ", data: " << ss.str());
#endif

    ::sendMsgNRT(world_, buf, newSize);
}
#endif

void AooClient::handlePeerBundle(const osc::ReceivedBundle& bundle,
                                 const aoo::ip_address& address)
{
    auto time = bundle.TimeTag();
    auto it = bundle.ElementsBegin();
    while (it != bundle.ElementsEnd()){
        if (it->IsBundle()){
            osc::ReceivedBundle b(*it);
            handlePeerBundle(b, address);
        } else {
            handlePeerMessage(it->Contents(), it->Size(),
                              address, time);
        }
        ++it;
    }
}

} // namespace sc

namespace {

template<typename T>
void doCommand(World* world, void *replyAddr, T* cmd, AsyncStageFn fn) {
    DoAsynchronousCommand(world, replyAddr, 0, cmd,
        fn, 0, 0, CmdData::free<T>, 0, 0);
}

void aoo_client_new(World* world, void* user,
    sc_msg_iter* args, void* replyAddr)
{
    auto port = args->geti();

    auto cmdData = CmdData::create<sc::AooClientCmd>(world);
    if (cmdData) {
        cmdData->port = port;

        auto fn = [](World* world, void* data) {
            auto port = static_cast<sc::AooClientCmd*>(data)->port;
            char buf[1024];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            msg << osc::BeginMessage("/aoo/client/new") << port;

            if (findClient(world, port)) {
                char errbuf[256];
                snprintf(errbuf, sizeof(errbuf),
                    "AooClient on port %d already exists.", port);
                msg << 0 << errbuf;
            } else {
                createClient(world, port);
                msg << 1;
            }

            msg << osc::EndMessage;
            ::sendMsgNRT(world, msg);

            return false; // done
        };

        doCommand(world, replyAddr, cmdData, fn);
    }
}

void aoo_client_free(World* world, void* user,
    sc_msg_iter* args, void* replyAddr)
{
    auto port = args->geti();

    auto cmdData = CmdData::create<sc::AooClientCmd>(world);
    if (cmdData) {
        cmdData->port = port;

        auto fn = [](World * world, void* data) {
            auto port = static_cast<sc::AooClientCmd*>(data)->port;

            freeClient(world, port);

            char buf[1024];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            msg << osc::BeginMessage("/aoo/client/free")
                << port << osc::EndMessage;
            ::sendMsgNRT(world, msg);

            return false; // done
        };

        doCommand(world, replyAddr, cmdData, fn);
    }
}

// called from the NRT
sc::AooClient* aoo_client_get(World *world, int port, const char *cmd)
{
    auto client = findClient(world, port);
    if (!client){
        char errbuf[256];
        snprintf(errbuf, sizeof(errbuf),
            "couldn't find AooClient on port %d", port);

        char buf[1024];
        osc::OutboundPacketStream msg(buf, sizeof(buf));

        msg << osc::BeginMessage(cmd) << port << 0 << errbuf
            << osc::EndMessage;

        ::sendMsgNRT(world, msg);
    }
    return client;
}

void aoo_client_connect(World* world, void* user,
    sc_msg_iter* args, void* replyAddr)
{
    auto port = args->geti();
    auto serverName = args->gets();
    auto serverPort = args->geti();
    auto userName = args->gets();
    auto userPwd = args->gets();

    auto cmdData = CmdData::create<sc::ConnectCmd>(world);
    if (cmdData) {
        cmdData->port = port;
        snprintf(cmdData->serverName, sizeof(cmdData->serverName),
            "%s", serverName);
        cmdData->serverPort = serverPort;
        snprintf(cmdData->userName, sizeof(cmdData->userName),
            "%s", userName);
        snprintf(cmdData->userPwd, sizeof(cmdData->userPwd),
            "%s", userPwd);

        auto fn = [](World * world, void* cmdData) {
            auto data = (sc::ConnectCmd*)cmdData;
            auto client = aoo_client_get(world, data->port,
                                         "/aoo/client/connect");
            if (client) {
                client->connect(data->serverName, data->serverPort,
                                data->userName, data->userPwd);
            }

            return false; // done
        };

        doCommand(world, replyAddr, cmdData, fn);
    }
}

void aoo_client_disconnect(World* world, void* user,
    sc_msg_iter* args, void* replyAddr)
{
    auto port = args->geti();

    auto cmdData = CmdData::create<sc::AooClientCmd>(world);
    if (cmdData) {
        cmdData->port = port;

        auto fn = [](World * world, void* cmdData) {
            auto data = (sc::AooClientCmd *)cmdData;
            auto client = aoo_client_get(world, data->port,
                                         "/aoo/client/disconnect");
            if (client) {
                client->disconnect();
            }

            return false; // done
        };

        doCommand(world, replyAddr, cmdData, fn);
    }
}

void aoo_client_group_join(World* world, void* user,
    sc_msg_iter* args, void* replyAddr)
{
    auto port = args->geti();
    auto name = args->gets();
    auto pwd = args->gets();

    auto cmdData = CmdData::create<sc::GroupCmd>(world);
    if (cmdData) {
        cmdData->port = port;
        snprintf(cmdData->name, sizeof(cmdData->name),
            "%s", name);
        snprintf(cmdData->pwd, sizeof(cmdData->pwd),
            "%s", pwd);

        auto fn = [](World * world, void* cmdData) {
            auto data = (sc::GroupCmd *)cmdData;
            auto client = aoo_client_get(world, data->port,
                                         "/aoo/client/group/join");
            if (client) {
                client->joinGroup(data->name, data->pwd);
            }

            return false; // done
        };

        doCommand(world, replyAddr, cmdData, fn);
    }
}

void aoo_client_group_leave(World* world, void* user,
    sc_msg_iter* args, void* replyAddr)
{
    auto port = args->geti();
    auto name = args->gets();

    auto cmdData = CmdData::create<sc::GroupCmd>(world);
    if (cmdData) {
        cmdData->port = port;
        snprintf(cmdData->name, sizeof(cmdData->name),
            "%s", name);

        auto fn = [](World * world, void* cmdData) {
            auto data = (sc::GroupCmd *)cmdData;
            auto client = aoo_client_get(world, data->port,
                                         "/aoo/client/group/leave");
            if (client) {
                client->leaveGroup(data->name);
            }

            return false; // done
        };

        doCommand(world, replyAddr, cmdData, fn);
    }
}

} // namespace

/*////////////// Setup /////////////////*/

void AooClientLoad(InterfaceTable *inTable){
    ft = inTable;

    AooPluginCmd(aoo_client_new);
    AooPluginCmd(aoo_client_free);
    AooPluginCmd(aoo_client_connect);
    AooPluginCmd(aoo_client_disconnect);
    AooPluginCmd(aoo_client_group_join);
    AooPluginCmd(aoo_client_group_leave);
}
