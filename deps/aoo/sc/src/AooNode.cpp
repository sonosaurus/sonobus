#include "AooNode.hpp"

// public methods

AooNode::AooNode(World *world, int port) {
    // increase socket buffers
    const int sendbufsize = 1 << 16; // 65 KB
#if NETWORK_THREAD_POLL
    const int recvbufsize = 1 << 20; // 1 MB
#else
    const int recvbufsize = 1 << 16; // 65 KB
#endif
    // udp_server::start() throws on error!
    server_.start(port, [this](auto&&... args) { handlePacket(args...); },
                  false, recvbufsize, sendbufsize);

    LOG_DEBUG("create AooClient on port " << port);
    client_ = AooClient::create(server_.socket(), 0, nullptr);

#if NETWORK_THREAD_POLL
    // start network I/O thread
    LOG_DEBUG("start network thread");
    iothread_ = std::thread([this](){
        aoo::sync::lower_thread_priority();
        performNetworkIO();
    });
#else
    // start send thread
    LOG_DEBUG("start network send thread");
    sendthread_ = std::thread([this](){
        aoo::sync::lower_thread_priority();
        sendPackets();
    });
    // start receive thread
    LOG_DEBUG("start network receive thread");
    recvthread_ = std::thread([this](){
        aoo::sync::lower_thread_priority();
        server_.run();
    });
#endif

    LOG_VERBOSE("new node on port " << port);
}

AooNode::~AooNode() {
    auto port = server_.port();
#if NETWORK_THREAD_POLL
    LOG_DEBUG("join network thread");
    // notify I/O thread
    quit_ = true;
    iothread_.join();
    server_.stop();
#else
    LOG_DEBUG("join network threads");
    // notify send thread
    quit_ = true;
    event_.set();
    // stop UDP server
    server_.stop();
    // join both threads
    sendthread_.join();
    recvthread_.join();
#endif

    // quit client thread
    if (clientThread_.joinable()){
        client_->quit();
        clientThread_.join();
    }

    LOG_VERBOSE("released node on port " << port);
}

using NodeMap = std::unordered_map<int, std::weak_ptr<AooNode>>;

static aoo::sync::mutex gNodeMapMutex;
static std::unordered_map<World *, NodeMap> gNodeMap;

static NodeMap& getNodeMap(World *world) {
    // we only have to protect against concurrent insertion;
    // the object reference is stable.
    aoo::sync::scoped_lock<aoo::sync::mutex> lock(gNodeMapMutex);
    return gNodeMap[world];
}

INode::ptr INode::get(World *world, int port){
    std::shared_ptr<AooNode> node;

    auto& nodeMap = getNodeMap(world);

    // find or create node
    auto it = nodeMap.find(port);
    if (it != nodeMap.end()){
        node = it->second.lock();
    }

    if (!node) {
        // finally create aoo node instance
        try {
            node = std::make_shared<AooNode>(world, port);
            nodeMap.emplace(port, node);
        } catch (const std::exception& e) {
            LOG_ERROR("AooNode: " << e.what());
        }
    }

    return node;
}

bool AooNode::registerClient(sc::AooClient *c){
    scoped_lock lock(clientMutex_);
    if (clientObject_){
        LOG_ERROR("AooClient on port " << port()
                  << " already exists!");
        return false;
    }
    if (!clientThread_.joinable()){
        // lazily create client thread
        clientThread_ = std::thread([this](){
            client_->run(kAooFalse);
        });
    }
    clientObject_ = c;
    client_->setEventHandler(
        [](void *user, const AooEvent *event, AooEventMode) {
            static_cast<sc::AooClient*>(user)->handleEvent(event);
        }, c, kAooEventModeCallback);
    return true;
}

void AooNode::unregisterClient(sc::AooClient *c){
    scoped_lock lock(clientMutex_);
    assert(clientObject_ == c);
    clientObject_ = nullptr;
    client_->setEventHandler(nullptr, nullptr, kAooEventModeNone);
}

// private methods

bool AooNode::getEndpointArg(sc_msg_iter *args, aoo::ip_address& addr,
                             int32_t *id, const char *what) const
{
    if (args->remain() < 2){
        LOG_ERROR("too few arguments for " << what);
        return false;
    }

    auto s = args->gets("");

    // first try peer (group|user)
    if (args->nextTag() == 's'){
        auto group = s;
        auto user = args->gets();
        // we can't use length_ptr() because socklen_t != int32_t on many platforms
        AooAddrSize len = aoo::ip_address::max_length;
        if (client_->getPeerByName(group, user, addr.address_ptr(), &len) == kAooOk) {
            *addr.length_ptr() = len;
        } else {
            LOG_ERROR("couldn't find peer " << group << "|" << user);
            return false;
        }
    } else {
        // otherwise try host|port
        auto host = s;
        int port = args->geti();
        auto result = aoo::ip_address::resolve(host, port, type());
        if (!result.empty()){
            addr = result.front(); // pick the first result
        } else {
            LOG_ERROR("couldn't resolve hostname '"
                      << host << "' for " << what);
            return false;
        }
    }

    if (id){
        if (args->remain()){
            AooId i = args->geti(-1);
            if (i >= 0){
                *id = i;
            } else {
                LOG_ERROR("bad ID '" << i << "' for " << what);
                return false;
            }
        } else {
            LOG_ERROR("too few arguments for " << what);
            return false;
        }
    }

    return true;
}

AooInt32 AooNode::send(void *user, const AooByte *msg, AooInt32 size,
                       const void *addr, AooAddrSize addrlen, AooFlag flags)
{
    auto x = (AooNode *)user;
    aoo::ip_address address((const sockaddr *)addr, addrlen);
    return x->server_.send(address, msg, size);
}

#if NETWORK_THREAD_POLL
void AooNode::performNetworkIO(){
    while (!quit_.load(std::memory_order_relaxed)) {
        server_.run(POLL_INTERVAL);

        if (update_.exchange(false, std::memory_order_acquire)){
            scoped_lock lock(clientMutex_);
        #if DEBUG_THREADS
            std::cout << "send messages" << std::endl;
        #endif
            client_->send(send, this);
        }
    }
}
#else
void AooNode::sendPackets(){
    while (!quit_.load(std::memory_order_relaxed)){
        event_.wait();

        scoped_lock lock(clientMutex_);
    #if DEBUG_THREADS
        std::cout << "send messages" << std::endl;
    #endif
        client_->send(send, this);
    }
}
#endif

void AooNode::handlePacket(int e, const aoo::ip_address& addr,
                           const AooByte *data, AooSize size) {
    if (e == 0) {
        scoped_lock lock(clientMutex_);
    #if DEBUG_THREADS
        std::cout << "handle message" << std::endl;
    #endif
        client_->handleMessage(data, size, addr.address(), addr.length());
    } else {
        LOG_ERROR("AooNode: recv() failed: " << aoo::socket_strerror(e));
    }
}

void AooNode::handleClientMessage(const char *data, int32_t size,
                                  const aoo::ip_address& addr, aoo::time_tag time)
{
    if (size > 4 && !memcmp("/aoo", data, 4)){
        // AOO message
        client_->handleMessage((const AooByte *)data, size,
                               addr.address(), addr.length());
    } else if (!strncmp("/sc/msg", data, size)){
        // OSC message coming from language client
        if (clientObject_){
            clientObject_->forwardMessage(data, size, time);
        }
    } else {
        LOG_WARNING("AooNode: unknown OSC message " << data);
    }
}

void AooNode::handleClientBundle(const osc::ReceivedBundle &bundle,
                                 const aoo::ip_address& addr){
    auto time = bundle.TimeTag();
    auto it = bundle.ElementsBegin();
    while (it != bundle.ElementsEnd()){
        if (it->IsBundle()){
            osc::ReceivedBundle b(*it);
            handleClientBundle(b, addr);
        } else {
            handleClientMessage(it->Contents(), it->Size(), addr, time);
        }
        ++it;
    }
}
