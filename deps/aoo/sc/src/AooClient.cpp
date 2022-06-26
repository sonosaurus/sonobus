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

void AooClient::connect(int token, const char* host, int port, const char *pwd) {
    if (!node_){
        sendError("/aoo/client/connect", token, kAooErrorUnknown);
        return;
    }

    auto cb = [](void* x, const AooNetRequest *request,
                 AooError result, const AooNetResponse *response) {
        auto data = (RequestData *)x;
        auto client = data->client;
        auto token = data->token;

        if (result == kAooOk) {
            char buf[1024];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            msg << osc::BeginMessage("/aoo/client/connect")
                << client->node_->port() << token << result
                << reinterpret_cast<const AooNetResponseConnect *>(response)->clientId
                << osc::EndMessage;

            ::sendMsgNRT(client->world_, msg);
        } else {
            client->sendError("/aoo/client/connect", token, result, response);
        }

        delete data;
    };

    node_->client()->connect(host, port, pwd, nullptr, cb, new RequestData { this, token });
}

void AooClient::disconnect(int token) {
    if (!node_) {
        sendError("/aoo/client/disconnect", token, kAooErrorUnknown);
        return;
    }

    auto cb = [](void* x, const AooNetRequest *request,
            AooError result, const AooNetResponse *response) {
        auto data = (RequestData *)x;
        auto client = data->client;
        auto token = data->token;

        if (result == kAooOk) {
            char buf[1024];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            msg << osc::BeginMessage("/aoo/client/disconnect")
                << client->node_->port() << token << result
                << osc::EndMessage;

            ::sendMsgNRT(client->world_, msg);
        } else {
            client->sendError("/aoo/client/disconnect", token, result, response);
        }

        delete data;
    };

    node_->client()->disconnect(cb, new RequestData{ this, token });
}

void AooClient::joinGroup(int token, const char* groupName, const char *groupPwd,
                          const char *userName, const char *userPwd) {
    if (!node_) {
        sendError("/aoo/client/group/join", token, kAooErrorUnknown);
        return;
    }

    auto cb = [](void* x, const AooNetRequest *request,
            AooError result, const AooNetResponse* response) {
        auto data = (RequestData *)x;
        auto client = data->client;
        auto token = data->token;

        if (result == kAooOk) {
            auto r = (const AooNetResponseGroupJoin *)response;

            char buf[1024];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            msg << osc::BeginMessage("/aoo/client/group/join")
                << client->node_->port() << token << result
                << r->groupId << r->userId
                << osc::EndMessage;

            ::sendMsgNRT(client->world_, msg);
        } else {
            client->sendError("/aoo/client/group/join", token, result, response);
        }

        delete data;
    };

    node_->client()->joinGroup(groupName, groupPwd, nullptr,
                               userName, userPwd, nullptr, nullptr,
                               cb, new RequestData{ this, token });
}

void AooClient::leaveGroup(int token, AooId group) {
    if (!node_) {
        sendError("/aoo/client/group/leave", token, kAooErrorUnknown);
        return;
    }

    auto cb = [](void* x, const AooNetRequest *request,
            AooError result, const AooNetResponse* response) {
        auto data = (RequestData *)x;
        auto client = data->client;
        auto token = data->token;

        if (result == kAooOk) {
            char buf[1024];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            msg << osc::BeginMessage("/aoo/client/group/join")
                << client->node_->port() << token << result
                << osc::EndMessage;

            ::sendMsgNRT(client->world_, msg);
        } else {
            client->sendError("/aoo/client/group/join", token, result, response);
        }

        delete data;
    };

    node_->client()->leaveGroup(group, cb, new RequestData{ this, token });
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
        handlePeerMessage(e->groupId, e->userId, e->timeStamp, e->data);
        return; // don't send event
    }
    case kAooNetEventClientDisconnect:
    {
        auto e = (const AooNetEventDisconnect *)event;
        msg << "/disconnect" << e->errorCode << e->errorMessage;
        break;
    }
    case kAooNetEventPeerHandshake:
    case kAooNetEventPeerTimeout:
        // ignore for now
        return;
    case kAooNetEventPeerJoin:
    {
        auto e = (const AooNetEventPeer*)event;
        aoo::ip_address addr(e->address);
        msg << "/peer/join" << e->groupId << e->userId
            << e->groupName << e->userName << addr.name() << addr.port();
        break;
    }
    case kAooNetEventPeerLeave:
    {
        auto e = (const AooNetEventPeer*)event;
        aoo::ip_address addr(e->address);
        msg << "/peer/leave" << e->groupId << e->userId
            << e->groupName << e->userName << addr.name() << addr.port();
        break;
    }
    case kAooNetEventPeerPing:
    case kAooNetEventPeerPingReply:
        // TODO
        return;
    case kAooNetEventError:
    {
        auto e = (const AooNetEventError*)event;
        msg << "/error" << e->errorCode << e->errorMessage;
        break;
    }
    default:
        LOG_DEBUG("AooClient: got unknown event " << event->type);
        return; // don't send event!
    }

    msg << osc::EndMessage;
    ::sendMsgNRT(world_, msg);
}

void AooClient::sendError(const char *cmd, AooId token, AooError result, const AooNetResponse *response) {
    char buf[1024];
    osc::OutboundPacketStream msg(buf, sizeof(buf));
    msg << osc::BeginMessage(cmd) << node_->port() << token << result;
    if (response) {
        msg << reinterpret_cast<const AooNetResponseError *>(response)->errorMessage;
    } else {
        msg << "internal error";
    }
    msg << osc::EndMessage;

    ::sendMsgNRT(world_, msg);
}

// group, user, offset, flags, type, content
void AooClient::forwardMessage(const char *data, int32_t size,
                               aoo::time_tag time)
{
    // SCLang can't send time tags, so we have to wrap peer messages
    // as a bundle (containing the absolute time tag from the
    // moment where it has been sent) + an offset in seconds.
    // We use sc_msg_iter because we need it for getPeerArg().
    // Also, it is a bit easier to use in this case.

    // skip '/sc/msg' (8 bytes)
    sc_msg_iter args(size - 8, data + 8);

    if (args.count < 6) {
        return;
    }
    AooId group = args.geti();
    AooId user = args.geti();
    auto offset = args.getf();
    auto flags = (AooFlag)args.geti();
    auto type = args.gets();
    // get message content
    if (args.nextTag() != 'b'){
        return;
    }
    auto blobsize = args.getbsize();
    if (blobsize <= 0){
        return;
    }
    auto blobdata = args.rdpos + 4;
    args.skipb();

    aoo::time_tag abstime = time + aoo::time_tag::from_seconds(offset);
    AooDataView msg { type, (AooByte *)blobdata, (AooSize)blobsize };

    node_->client()->sendMessage(group, user, msg, abstime.value(), flags);
}

// Called with NRT lock (see handleEvent()), although probably
// not necessary...
void AooClient::handlePeerMessage(AooId groupId, AooId userId, AooNtpTime time,
                                  const AooDataView& data)
{
    LOG_DEBUG("handlePeerMessage: " << data.type);
    aoo::time_tag tt(time);
    // SCLang can't handle immediate bundles,
    // so we have to send OSC messages instead
    bool bundle = !tt.is_empty() && !tt.is_immediate();
    // leave room for bundle pattern, time tag and bundle size
    auto offset = bundle ? 20 : 0;

    char buf[1024];
    osc::OutboundPacketStream msg(buf + offset, sizeof(buf) - offset);
    msg << osc::BeginMessage("/aoo/msg")
        << node_->port() << groupId << userId << data.type;

    if (!strcmp(data.type, kAooDataTypeOSC)) {
        // HACK to avoid manually parsing OSC messages in SCLang:
        // parse inner OSC message and append its address pattern and
        // arguments to our message. The Client can simply look at the
        // 'type' argument to determine how to interpret the message.
        // LATER optimize this by appending the type tags and copy the data.
        osc::ReceivedPacket packet((const char *)data.data, data.size);
        osc::ReceivedMessage m(packet);
        msg << m.AddressPattern();
        for (auto it = m.ArgumentsBegin(); it != m.ArgumentsEnd(); ++it) {
            switch (it->TypeTag()) {
            case 'i':
                msg << it->AsInt32Unchecked();
                break;
            case 'f':
                msg << it->AsFloatUnchecked();
                break;
            case 'h':
                msg << it->AsInt64Unchecked();
                break;
            case 'd':
                msg << it->AsDoubleUnchecked();
                break;
            case 's':
            case 'S':
                msg << it->AsStringUnchecked();
                break;
            case 'b':
            {
                const void *blobdata;
                osc::osc_bundle_element_size_t blobsize;
                it->AsBlobUnchecked(blobdata, blobsize);
                msg << osc::Blob(blobdata, blobsize);
                break;
            }
            case 't':
                msg << osc::TimeTag(it->AsTimeTagUnchecked());
                break;
            case 'r':
            case 'm':
                msg << osc::MidiMessage(it->AsMidiMessageUnchecked());
                break;
            case 'c':
                msg << it->AsCharUnchecked();
                break;
            case 'I':
                msg << osc::Infinitum;
                break;
            case 'N':
                msg << osc::Nil;
                break;
            case 'T':
                msg << true;
                break;
            case 'F':
                msg << false;
                break;
            case '[':
                msg << osc::BeginArray;
                break;
            case ']':
                msg << osc::EndArray;
                break;
            default:
                LOG_ERROR("unexpected OSC type tag '" << it->TypeTag() << '"');
                return; // don't send message
            }
        }
    } else {
        msg << osc::Blob(data.data, data.size);
    }
    msg << osc::EndMessage;

    if (bundle) {
        // write bundle pattern, time tag and bundle size
        memcpy(buf, "#bundle\0", 8);
        aoo::to_bytes<uint64_t>(time, buf + 8);
        aoo::to_bytes<int32_t>(msg.Size(), buf + 16);
    }

    ::sendMsgNRT(world_, buf, msg.Size() + offset);
}

} // namespace sc

namespace {

template<typename T>
void doCommand(World* world, void *replyAddr, T* cmd, AsyncStageFn fn) {
    DoAsynchronousCommand(world, replyAddr, 0, cmd,
        fn, 0, 0, CmdData::free<T>, 0, 0);
}

void aoo_client_new(World* world, void* user, sc_msg_iter* args, void* replyAddr)
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

void aoo_client_free(World* world, void* user, sc_msg_iter* args, void* replyAddr)
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
sc::AooClient* getClient(World *world, int port, const char *cmd)
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
    auto token = args->geti();
    auto serverHost = args->gets();
    auto serverPort = args->geti();
    auto password = args->gets();

    auto cmdData = CmdData::create<sc::ConnectCmd>(world);
    if (cmdData) {
        cmdData->port = port;
        cmdData->token = token;
        snprintf(cmdData->serverHost, sizeof(cmdData->serverHost),
            "%s", serverHost);
        cmdData->serverPort = serverPort;
        snprintf(cmdData->password, sizeof(cmdData->password),
            "%s", password);

        auto fn = [](World * world, void* cmdData) {
            auto data = (sc::ConnectCmd*)cmdData;
            auto client = getClient(world, data->port, "/aoo/client/connect");
            if (client) {
                client->connect(data->token, data->serverHost,
                                data->serverPort, data->password);
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
    auto token = args->geti();

    auto cmdData = CmdData::create<sc::AooClientCmd>(world);
    if (cmdData) {
        cmdData->port = port;
        cmdData->token = token;

        auto fn = [](World * world, void* cmdData) {
            auto data = (sc::AooClientCmd *)cmdData;
            auto client = getClient(world, data->port, "/aoo/client/disconnect");
            if (client) {
                client->disconnect(data->token);
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
    auto token = args->geti();
    auto groupName = args->gets();
    auto groupPwd = args->gets();
    auto userName = args->gets();
    auto userPwd = args->gets();

    auto cmdData = CmdData::create<sc::GroupJoinCmd>(world);
    if (cmdData) {
        cmdData->port = port;
        cmdData->token = token;
        snprintf(cmdData->groupName, sizeof(cmdData->groupName),
            "%s", groupName);
        snprintf(cmdData->groupPwd, sizeof(cmdData->groupPwd),
            "%s", groupPwd);
        snprintf(cmdData->userName, sizeof(cmdData->userName),
            "%s", userName);
        snprintf(cmdData->userPwd, sizeof(cmdData->userPwd),
            "%s", userPwd);

        auto fn = [](World * world, void* cmdData) {
            auto data = (sc::GroupJoinCmd *)cmdData;
            auto client = getClient(world, data->port, "/aoo/client/group/join");
            if (client) {
                client->joinGroup(data->token, data->groupName, data->groupPwd,
                                  data->userName, data->userPwd);
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
    auto token = args->geti();
    auto group = args->geti();

    auto cmdData = CmdData::create<sc::GroupLeaveCmd>(world);
    if (cmdData) {
        cmdData->port = port;
        cmdData->token = token;
        cmdData->group = group;

        auto fn = [](World * world, void* cmdData) {
            auto data = (sc::GroupLeaveCmd *)cmdData;
            auto client = getClient(world, data->port, "/aoo/client/group/leave");
            if (client) {
                client->leaveGroup(data->token, data->group);
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
