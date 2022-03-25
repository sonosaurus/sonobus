#pragma once

#include "Aoo.hpp"
#include "AooClient.hpp"

#include "common/lockfree.hpp"
#include "common/net_utils.hpp"
#include "common/sync.hpp"
#include "common/time.hpp"
#include "common/udp_server.hpp"

#include <unordered_map>
#include <cstring>
#include <stdio.h>
#include <errno.h>
#include <iostream>

#include <thread>
#include <atomic>

#define NETWORK_THREAD_POLL 0

#if NETWORK_THREAD_POLL
#define POLL_INTERVAL 0.001 // seconds
#endif

#define DEBUG_THREADS 0

class AooNode final : public INode {
    friend class INode;
public:
    AooNode(World *world, int port);

    ~AooNode() override;

    aoo::ip_address::ip_type type() const override { return server_.type(); }

    int port() const override { return server_.port(); }

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

    // UDP server
    aoo::udp_server server_;
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
#endif
    void handlePacket(int e, const aoo::ip_address& addr,
                      const AooByte *data, AooSize size);

    void handleClientMessage(const char *data, int32_t size,
                             const aoo::ip_address& addr, aoo::time_tag time);

    void handleClientBundle(const osc::ReceivedBundle& bundle,
                            const aoo::ip_address& addr);
};
