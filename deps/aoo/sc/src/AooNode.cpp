#include "Aoo.hpp"
#include "AooClient.hpp"

#include "common/lockfree.hpp"
#include "common/net_utils.hpp"
#include "common/sync.hpp"
#include "common/time.hpp"

#include <unordered_map>
#include <cstring>
#include <stdio.h>
#include <errno.h>
#include <iostream>

#include <thread>
#include <atomic>

#define NETWORK_THREAD_POLL 0

#if NETWORK_THREAD_POLL
#define POLL_INTERVAL 1000 // microseconds
#endif

#define DEBUG_THREADS 0

class AooNode final : public INode {
    friend class INode;
public:
    AooNode(World *world, int socket, const aoo::ip_address& addr);

    ~AooNode() override;

    aoo::ip_address::ip_type type() const override { return type_; }

    int port() const override { return port_; }

    AooClient * client() override {
        return client_.get();
    }

    bool registerClient(sc::AooClient *c) override;

    void unregisterClient(sc::AooClient *c) override;

    void notify() override {
    #if NETWORK_THREAD_POLL
        update_.store(true);
    #else
        event_.set();
    #endif
    }

    void lock() override {
        clientMutex_.lock();
    }

    void unlock() override {
        clientMutex_.unlock();
    }

    bool getSinkArg(sc_msg_iter *args, aoo::ip_address& addr,
                    AooId &id) const override {
        return getEndpointArg(args, addr, &id, "sink");
    }

    bool getSourceArg(sc_msg_iter *args, aoo::ip_address& addr,
                      AooId &id) const override {
        return getEndpointArg(args, addr, &id, "source");
    }

    bool getPeerArg(sc_msg_iter *args, aoo::ip_address& addr) const override {
        return getEndpointArg(args, addr, nullptr, "peer");
    }
private:
    using unique_lock = aoo::sync::unique_lock<aoo::sync::mutex>;
    using scoped_lock = aoo::sync::scoped_lock<aoo::sync::mutex>;

    int socket_ = -1;
    int port_ = 0;
    aoo::ip_address::ip_type type_;
    // client
    AooClient::Ptr client_;
    aoo::sync::mutex clientMutex_;
    std::thread clientThread_;
    sc::AooClient *clientObject_ = nullptr;
    // threading
#if NETWORK_THREAD_POLL
    std::thread iothread_;
    std::atomic<bool> update_{false};
#else
    std::thread sendthread_;
    std::thread recvthread_;
    aoo::sync::event event_;
#endif
    std::atomic<bool> quit_{false};

    // private methods
    bool getEndpointArg(sc_msg_iter *args, aoo::ip_address& addr,
                        int32_t *id, const char *what) const;

    static AooInt32 send(void *user, const AooByte *msg, AooInt32 size,
                         const void *addr, AooAddrSize addrlen, AooFlag flags);

#if NETWORK_THREAD_POLL
    void performNetworkIO();
#else
    void sendPackets();
    void receivePackets();
#endif

    void handleClientMessage(const char *data, int32_t size,
                             const aoo::ip_address& addr, aoo::time_tag time);

    void handleClientBundle(const osc::ReceivedBundle& bundle,
                            const aoo::ip_address& addr);
};

// public methods

AooNode::AooNode(World *world, int socket, const aoo::ip_address& addr)
    : socket_(socket), port_(addr.port()), type_(addr.type())
{
    client_ = AooClient::create(addr.address(), addr.length(), 0, nullptr);

#if NETWORK_THREAD_POLL
    // start network I/O thread
    iothread_ = std::thread([this](){
        aoo::sync::lower_thread_priority();
        performNetworkIO();
    });
#else
    // start receive thread
    sendthread_ = std::thread([this](){
        aoo::sync::lower_thread_priority();
        sendPackets();
    });
    // start receive thread
    recvthread_ = std::thread([this](){
        aoo::sync::lower_thread_priority();
        receivePackets();
    });
#endif

    LOG_VERBOSE("new node on port " << port_);
}

AooNode::~AooNode(){
    // ask threads to quit
    quit_ = true;

#if NETWORK_THREAD_POLL
    aoo::socket_signal(socket_); // wake perform_io()
    iothread_.join();
#else
    event_.set(); // wake send_packets()
    aoo::socket_signal(socket_); // wake receive_packets()
    sendthread_.join();
    recvthread_.join();
#endif

    aoo::socket_close(socket_);

    // quit client thread
    if (clientThread_.joinable()){
        client_->quit();
        clientThread_.join();
    }

    LOG_VERBOSE("released node on port " << port_);
}

using NodeMap = std::unordered_map<int, std::weak_ptr<AooNode>>;

static aoo::sync::mutex gNodeMapMutex;
static std::unordered_map<World *, NodeMap> gNodeMap;

static NodeMap& getNodeMap(World *world){
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

    if (!node){
        // first create socket
        int sock = aoo::socket_udp(port);
        if (sock < 0){
            LOG_ERROR("AooNode: couldn't bind to port " << port);
            return nullptr;
        }

        aoo::ip_address addr;
        if (aoo::socket_address(sock, addr) != 0){
            LOG_ERROR("AooNode: couldn't get socket address");
            aoo::socket_close(sock);
            return nullptr;
        }

        // increase socket buffers
        const int sendbufsize = 1 << 16; // 65 KB
    #if NETWORK_THREAD_POLL
        const int recvbufsize = 1 << 20; // 1 MB
    #else
        const int recvbufsize = 1 << 16; // 65 KB
    #endif
        aoo::socket_setsendbufsize(sock, sendbufsize);
        aoo::socket_setrecvbufsize(sock, recvbufsize);

        // finally create aoo node instance
        node = std::make_shared<AooNode>(world, sock, addr);
        nodeMap.emplace(port, node);
    }

    return node;
}

bool AooNode::registerClient(sc::AooClient *c){
    scoped_lock lock(clientMutex_);
    if (clientObject_){
        LOG_ERROR("aoo client on port " << port_
                  << " already exists!");
        return false;
    }
    if (!clientThread_.joinable()){
        // lazily create client thread
        clientThread_ = std::thread([this](){
            client_->run();
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
        auto result = aoo::ip_address::resolve(host, port, type_);
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
    return aoo::socket_sendto(x->socket_, msg, size, address);
}

#if NETWORK_THREAD_POLL
void AooNode::performNetworkIO(){
    while (!quit_.load(std::memory_order_relaxed)){
        aoo::ip_address addr;
        char buf[AOO_MAX_PACKET_SIZE];
        int nbytes = aoo::socket_receive(socket_, buf, AOO_MAX_PACKET_SIZE,
                                         &addr, POLL_INTERVAL);
        if (nbytes > 0){
            scoped_lock lock(clientMutex_);
        #if DEBUG_THREADS
            std::cout << "performNetworkIO: receive" << std::endl;
        #endif
            try {
                osc::ReceivedPacket packet(buf, nbytes);
                if (packet.IsBundle()){
                    osc::ReceivedBundle bundle(packet);
                    handleClientBundle(bundle, addr);
                } else {
                    handleClientMessage(buf, nbytes, addr,
                                        aoo::time_tag::immediate());
                }
            } catch (const osc::Exception &err){
                LOG_ERROR("AooNode: bad OSC message - " << err.what());
            }
        } else if (nbytes < 0) {
            // ignore errors when quitting
            if (!quit_){
                aoo::socket_error_print("recv");
            }
            return;
        } else {
            // timeout
        }

        if (update_.exchange(false, std::memory_order_acquire)){
            scoped_lock lock(clientMutex_);
        #if DEBUG_THREADS
            std::cout << "performNetworkIO: send" << std::endl;
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
        std::cout << "sendPackets" << std::endl;
    #endif
        client_->send(send, this);
    }
}

void AooNode::receivePackets(){
    while (!quit_.load(std::memory_order_relaxed)){
        aoo::ip_address addr;
        char buf[AOO_MAX_PACKET_SIZE];
        int nbytes = socket_receive(socket_, buf, AOO_MAX_PACKET_SIZE, &addr, -1);
        if (nbytes > 0){
            scoped_lock lock(clientMutex_);
        #if DEBUG_THREADS
            std::cout << "receivePackets" << std::endl;
        #endif
            try {
                osc::ReceivedPacket packet(buf, nbytes);
                if (packet.IsBundle()){
                    osc::ReceivedBundle bundle(packet);
                    handleClientBundle(bundle, addr);
                } else {
                    handleClientMessage(buf, nbytes, addr,
                                        aoo::time_tag::immediate());
                }
            } catch (const osc::Exception &err){
                LOG_ERROR("AooNode: bad OSC message - " << err.what());
            }
        } else if (nbytes < 0) {
            // ignore errors when quitting
            if (!quit_){
                aoo::socket_error_print("recv");
            }
            return;
        } else {
            // timeout
        }
    }
}
#endif

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
