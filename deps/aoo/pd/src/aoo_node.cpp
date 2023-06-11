/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "aoo_common.hpp"

#include "common/lockfree.hpp"
#include "common/sync.hpp"
#include "common/utils.hpp"
#include "common/udp_server.hpp"

#include <iostream>
#include <vector>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <thread>
#include <atomic>

#define NETWORK_THREAD_POLL 0

#if NETWORK_THREAD_POLL
#define POLL_INTERVAL 0.001 // seconds
#endif

#define DEBUG_THREADS 0

extern t_class *aoo_receive_class;

extern t_class *aoo_send_class;

extern t_class *aoo_client_class;

struct t_aoo_client;

void aoo_client_handle_event(t_aoo_client *x, const AooEvent *event, int32_t level);

bool aoo_client_find_peer(t_aoo_client *x, const aoo::ip_address& addr,
                          t_symbol *& group, t_symbol *& user);

bool aoo_client_find_peer(t_aoo_client *x, t_symbol * group, t_symbol * user,
                          aoo::ip_address& addr);

/*////////////////////// aoo node //////////////////*/

static t_class *node_proxy_class;

class t_node_imp;

struct t_node_proxy
{
    t_node_proxy(t_node_imp *node){
        x_pd = node_proxy_class;
        x_node = node;
    }

    t_pd x_pd;
    t_node_imp *x_node;
};

class t_node_imp final : public t_node
{
    using unique_lock = aoo::sync::unique_lock<aoo::sync::mutex>;
    using scoped_lock = aoo::sync::scoped_lock<aoo::sync::mutex>;

    t_node_proxy x_proxy; // we can't directly bind t_node_imp because of vtable
    t_symbol *x_bindsym;
    AooClient::Ptr x_client;
    t_pd * x_clientobj = nullptr;
    aoo::sync::mutex x_clientmutex;
    std::thread x_clientthread;
    int32_t x_refcount = 0;
    // UDP server
    aoo::udp_server x_server;
    // threading
#if NETWORK_THREAD_POLL
    std::thread x_iothread;
    std::atomic<bool> x_update{false};
#else
    std::thread x_sendthread;
    std::thread x_recvthread;
    aoo::sync::event x_event;
#endif
    std::atomic<bool> x_quit{false};
public:
    // public methods
    t_node_imp(t_symbol *s, int port);

    ~t_node_imp();

    void release(t_pd *obj, void *x) override;

    AooClient * client() override { return x_client.get(); }

    aoo::ip_address::ip_type type() const override { return x_server.type(); }

    int port() const override { return x_server.port(); }

    void notify() override {
    #if NETWORK_THREAD_POLL
        x_update.store(true);
    #else
        x_event.set();
    #endif
    }

    void lock() override {
        x_clientmutex.lock();
    }

    void unlock() override {
        x_clientmutex.unlock();
    }

    bool resolve(t_symbol *host, int port, aoo::ip_address& addr) const override;

    bool find_peer(const aoo::ip_address& addr,
                   t_symbol *& group, t_symbol *& user) const override;

    bool find_peer(t_symbol * group, t_symbol * user,
                   aoo::ip_address& addr) const override;

    int serialize_endpoint(const aoo::ip_address &addr, AooId id,
                           int argc, t_atom *argv) const override;
private:
    friend class t_node;

    static AooInt32 send(void *user, const AooByte *msg, AooInt32 size,
                         const void *addr, AooAddrSize len, AooFlag flags);

    void handle_packet(int e, const aoo::ip_address& addr,
                       const AooByte *data, AooSize size);

#if NETWORK_THREAD_POLL
    void perform_io();
#else
    void send_packets();
#endif

    bool add_object(t_pd *obj, void *x, AooId id);
};

bool t_node_imp::resolve(t_symbol *host, int port, aoo::ip_address& addr) const {
    auto result = aoo::ip_address::resolve(host->s_name, port, type());
    if (!result.empty()){
        addr = result.front();
        return true;
    } else {
        return false;
    }
}

bool t_node_imp::find_peer(const aoo::ip_address& addr,
                           t_symbol *& group, t_symbol *& user) const {
    if (x_clientobj &&
            aoo_client_find_peer(
                (t_aoo_client *)x_clientobj, addr, group, user)) {
        return true;
    } else {
        return false;
    }
}

bool t_node_imp::find_peer(t_symbol * group, t_symbol * user,
                           aoo::ip_address& addr) const {
    if (x_clientobj &&
            aoo_client_find_peer(
                (t_aoo_client *)x_clientobj, group, user, addr)) {
        return true;
    } else {
        return false;
    }
}

int t_node_imp::serialize_endpoint(const aoo::ip_address &addr, AooId id,
                                   int argc, t_atom *argv) const {
    if (argc < 3 || !addr.valid()) {
        LOG_DEBUG("serialize_endpoint: invalid address");
        return 0;
    }
    t_symbol *group, *user;
    if (find_peer(addr, group, user)) {
        // group name, user name, id
        SETSYMBOL(argv, group);
        SETSYMBOL(argv + 1, user);
        SETFLOAT(argv + 2, id);
    } else {
        // ip string, port number, id
        SETSYMBOL(argv, gensym(addr.name()));
        SETFLOAT(argv + 1, addr.port());
        SETFLOAT(argv + 2, id);
    }
    return 3;
}

// private methods

bool t_node_imp::add_object(t_pd *obj, void *x, AooId id)
{
    scoped_lock lock(x_clientmutex);
    if (pd_class(obj) == aoo_client_class){
        // aoo_client
        if (!x_clientobj){
            x_clientobj = obj;
            // start thread lazily
            if (!x_clientthread.joinable()){
                x_clientthread = std::thread([this](){
                   x_client->run(kAooFalse);
                });
            }
            // set event handler
            x_client->setEventHandler((AooEventHandler)aoo_client_handle_event,
                                       obj, kAooEventModePoll);
        } else {
            pd_error(obj, "%s on port %d already exists!",
                     classname(obj), port());
            return false;
        }
    } else  if (pd_class(obj) == aoo_send_class){
        if (x_client->addSource((AooSource *)x, id) != kAooOk){
            pd_error(obj, "%s with ID %d on port %d already exists!",
                     classname(obj), id, port());
        }
    } else if (pd_class(obj) == aoo_receive_class){
        if (x_client->addSink((AooSink *)x, id) != kAooOk){
            pd_error(obj, "%s with ID %d on port %d already exists!",
                     classname(obj), id, port());
        }
    } else {
        bug("t_node_imp: bad client");
        return false;
    }
    x_refcount++;
    return true;
}

#if NETWORK_THREAD_POLL
void t_node_imp::perform_io() {
    while (!x_quit.load(std::memory_order_relaxed)) {
        x_server.run(POLL_INTERVAL);

        if (x_update.exchange(false, std::memory_order_acquire)) {
            scoped_lock lock(x_clientmutex);
        #if DEBUG_THREADS
            std::cout << "send messages" << std::endl;
        #endif
            x_client->send(send, this);
        }
    }
}
#else
void t_node_imp::send_packets() {
    while (!x_quit.load(std::memory_order_relaxed)) {
        x_event.wait();

        scoped_lock lock(x_clientmutex);
    #if DEBUG_THREADS
        std::cout << "send messages" << std::endl;
    #endif
        x_client->send(send, this);
    }
}
#endif

AooInt32 t_node_imp::send(void *user, const AooByte *msg, AooInt32 size,
                          const void *addr, AooAddrSize len, AooFlag flags)
{
    auto x = (t_node_imp *)user;
    aoo::ip_address dest((sockaddr *)addr, len);
    return x->x_server.send(dest, msg, size);
}

void t_node_imp::handle_packet(int e, const aoo::ip_address& addr,
                               const AooByte *data, AooSize size) {
    if (e == 0) {
        scoped_lock lock(x_clientmutex);
    #if DEBUG_THREADS
        std::cout << "handle message" << std::endl;
    #endif
        x_client->handleMessage(data, size, addr.address(), addr.length());
    } else {
        sys_lock();
        pd_error(nullptr, "aoo_node: recv() failed: %s\n",
                 aoo::socket_strerror(e).c_str());
        // TODO handle error
        sys_unlock();
    }
}

t_node * t_node::get(t_pd *obj, int port, void *x, AooId id)
{
    t_node_imp *node = nullptr;
    // make bind symbol for port number
    char buf[64];
    snprintf(buf, sizeof(buf), "aoo_node %d", port);
    t_symbol *s = gensym(buf);
    // find or create node
    auto y = (t_node_proxy *)pd_findbyclass(s, node_proxy_class);
    if (y){
        node = y->x_node;
    } else {
        try {
            // finally create aoo node instance
            node = new t_node_imp(s, port);
        } catch (const std::exception& e) {
            pd_error(obj, "%s: %s", classname(obj), e.what());
            return nullptr;
        }
    }

    if (!node->add_object(obj, x, id)){
        // never fails for new t_node_imp!
        return nullptr;
    }

    return node;
}

t_node_imp::t_node_imp(t_symbol *s, int port)
    : x_proxy(this), x_bindsym(s)
{
    // increase socket buffers
    const int sendbufsize = 1 << 18; // 256 KB
    // NB: with a threaded UDP server we wouldn't need large receive buffers...
#if NETWORK_THREAD_POLL
    // The receive thread also does the sending (and encoding)! This requires a larger buffer.
    const int recvbufsize = 1 << 20; // 1 B
#else
    const int recvbufsize = 1 << 18; // 256 KB
#endif
    // udp_server::start() throws on error!
    // TODO: should we use threaded=true instead?
    x_server.set_send_buffer_size(sendbufsize);
    x_server.set_receive_buffer_size(recvbufsize);
    x_server.start(port, [this](auto&&... args) { handle_packet(args...); }, false);

    LOG_DEBUG("create AooClient on port " << port);

    auto flags = aoo::socket_family(x_server.socket()) == aoo::ip_address::IPv6 ?
        kAooSocketDualStack : kAooSocketIPv4;

    x_client = AooClient::create(nullptr);
    x_client->setup(port, flags);

    pd_bind(&x_proxy.x_pd, x_bindsym);

#if NETWORK_THREAD_POLL
    // start network I/O thread
    LOG_DEBUG("start network thread");
    x_iothread = std::thread([this, pd=pd_this]() {
    #ifdef PDINSTANCE
        pd_setinstance(pd);
    #endif
        aoo::sync::lower_thread_priority();
        perform_io();
    });
#else
    // start send thread
    LOG_DEBUG("start network send thread");
    x_sendthread = std::thread([this, pd=pd_this]() {
    #ifdef PDINSTANCE
        pd_setinstance(pd);
    #endif
        aoo::sync::lower_thread_priority();
        send_packets();
    });

    // start receive thread
    LOG_DEBUG("start network receive thread");
    x_recvthread = std::thread([this, pd=pd_this]() {
    #ifdef PDINSTANCE
        pd_setinstance(pd);
    #endif
        aoo::sync::lower_thread_priority();
        x_server.run();
    });
#endif

    verbose(0, "new aoo node on port %d", port);
}

void t_node_imp::release(t_pd *obj, void *x)
{
    unique_lock lock(x_clientmutex);
    if (pd_class(obj) == aoo_client_class){
        // client
        x_clientobj = nullptr;
        x_client->setEventHandler(nullptr, nullptr, kAooEventModeNone);
    } else if (pd_class(obj) == aoo_send_class){
        x_client->removeSource((AooSource *)x);
    } else if (pd_class(obj) == aoo_receive_class){
        x_client->removeSink((AooSink *)x);
    } else {
        bug("t_node_imp::release");
        return;
    }
    lock.unlock(); // !

    if (--x_refcount == 0){
        // last instance
        delete this;
    } else if (x_refcount < 0){
        bug("t_node_imp::release: negative refcount!");
    }
}

t_node_imp::~t_node_imp()
{
    pd_unbind(&x_proxy.x_pd, x_bindsym);

    auto port = x_server.port();
#if NETWORK_THREAD_POLL
    LOG_DEBUG("join network thread");
    // notify I/O thread
    x_quit = true;
    x_iothread.join();
    x_server.stop();
#else
    LOG_DEBUG("join network threads");
    // notify send thread
    x_quit = true;
    x_event.set();
    // stop UDP server
    x_server.stop();
    // join both threads
    x_sendthread.join();
    x_recvthread.join();
#endif

    if (x_clientthread.joinable()){
        x_client->quit();
        x_clientthread.join();
    }

    verbose(0, "released aoo node on port %d", port);
}

void aoo_node_setup(void)
{
    node_proxy_class = class_new(gensym("aoo node proxy"), 0, 0,
                                 sizeof(t_node_proxy), CLASS_PD, A_NULL);
}
