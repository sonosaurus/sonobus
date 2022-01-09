/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "aoo_common.hpp"

#include "common/lockfree.hpp"
#include "common/sync.hpp"
#include "common/utils.hpp"

#include <iostream>
#include <vector>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <thread>
#include <atomic>

#define NETWORK_THREAD_POLL 0

#if NETWORK_THREAD_POLL
#define POLL_INTERVAL 1000 // microseconds
#endif

#define DEBUG_THREADS 0

extern t_class *aoo_receive_class;

extern t_class *aoo_send_class;

extern t_class *aoo_client_class;

struct t_aoo_client;

int aoo_client_resolve_address(const t_aoo_client *x, const aoo::ip_address& addr,
                               AooId id, int argc, t_atom *argv);

void aoo_client_handle_event(t_aoo_client *x,
                             const AooEvent *event, int32_t level);

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
    // socket
    int x_socket = -1;
    int x_port = 0;
    aoo::ip_address::ip_type x_type;
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
    t_node_imp(t_symbol *s, int socket, const aoo::ip_address& addr);

    ~t_node_imp();

    void release(t_pd *obj, void *x) override;

    AooClient * client() override { return x_client.get(); }

    aoo::ip_address::ip_type type() const override { return x_type; }

    int port() const override { return x_port; }

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

    bool get_source_arg(t_pd *x, int argc, const t_atom *argv,
                        aoo::ip_address& addr, AooId &id) const override{
        return get_endpoint_arg(x, argc, argv, addr, &id, "source");
    }

    bool get_sink_arg(t_pd *x, int argc, const t_atom *argv,
                      aoo::ip_address& addr, AooId &id) const override {
        return get_endpoint_arg(x, argc, argv, addr, &id, "sink");
    }

    bool get_peer_arg(t_pd *x, int argc, const t_atom *argv,
                      aoo::ip_address& addr) const override {
        return get_endpoint_arg(x, argc, argv, addr, nullptr, "peer");
    }

    int resolve_endpoint(const aoo::ip_address &addr, AooId id,
                         int argc, t_atom *argv) const override;
private:
    friend class t_node;

    static AooInt32 send(void *user, const AooByte *msg, AooInt32 size,
                         const void *addr, AooAddrSize len, AooFlag flags);

#if NETWORK_THREAD_POLL
    void perform_io();
#else
    void send_packets();

    void receive_packets();
#endif

    bool add_object(t_pd *obj, void *x, AooId id);

    bool get_endpoint_arg(t_pd *x, int argc, const t_atom *argv,
                          aoo::ip_address& addr, int32_t *id, const char *what) const;
};

// private methods

bool t_node_imp::get_endpoint_arg(t_pd *x, int argc, const t_atom *argv,
                                  aoo::ip_address& addr, int32_t *id, const char *what) const
{
    if (argc < (2 + (id != nullptr))){
        pd_error(x, "%s: too few arguments for %s", classname(x), what);
        return false;
    }

    // first try peer (group|user)
    if (argv[1].a_type == A_SYMBOL){
        t_symbol *group = atom_getsymbol(argv);
        t_symbol *user = atom_getsymbol(argv + 1);
        // we can't use length_ptr() because socklen_t != int32_t on many platforms
        AooAddrSize len = aoo::ip_address::max_length;
        if (x_client->getPeerByName(group->s_name, user->s_name,
                                    addr.address_ptr(), &len) == kAooOk) {
            *addr.length_ptr() = len;
        } else {
            pd_error(x, "%s: couldn't find peer %s|%s",
                     classname(x), group->s_name, user->s_name);
            return false;
        }
    } else {
        // otherwise try host|port
        t_symbol *host = atom_getsymbol(argv);
        int port = atom_getfloat(argv + 1);
        auto result = aoo::ip_address::resolve(host->s_name, port, x_type);
        if (!result.empty()){
            addr = result.front(); // just pick the first one
        } else {
            pd_error(x, "%s: couldn't resolve hostname '%s' for %s",
                     classname(x), host->s_name, what);
            return false;
        }
    }

    if (id){
        if (argv[2].a_type == A_FLOAT){
            AooId i = argv[2].a_w.w_float;
            if (i >= 0){
                *id = i;
            } else {
                pd_error(x, "%s: bad ID '%d' for %s",
                         classname(x), i, what);
                return false;
            }
        } else {
            pd_error(x, "%s: bad ID '%s' for %s", classname(x),
                     atom_getsymbol(argv + 2)->s_name, what);
            return false;
        }
    }

    return true;
}

int t_node_imp::resolve_endpoint(const aoo::ip_address &addr, AooId id,
                                  int argc, t_atom *argv) const {
    if (x_clientobj){
        return aoo_client_resolve_address((t_aoo_client *)x_clientobj,
                                          addr, id, argc, argv);
    } else {
        return endpoint_to_atoms(addr, id, argc, argv);
    }
}

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
                   x_client->run();
                });
            }
            // set event handler
            x_client->setEventHandler((AooEventHandler)aoo_client_handle_event,
                                       obj, kAooEventModePoll);
        } else {
            pd_error(obj, "%s on port %d already exists!",
                     classname(obj), x_port);
            return false;
        }
    } else  if (pd_class(obj) == aoo_send_class){
        if (x_client->addSource((AooSource *)x, id) != kAooOk){
            pd_error(obj, "%s with ID %d on port %d already exists!",
                     classname(obj), id, x_port);
        }
    } else if (pd_class(obj) == aoo_receive_class){
        if (x_client->addSink((AooSink *)x, id) != kAooOk){
            pd_error(obj, "%s with ID %d on port %d already exists!",
                     classname(obj), id, x_port);
        }
    } else {
        bug("t_node_imp: bad client");
        return false;
    }
    x_refcount++;
    return true;
}

#if NETWORK_THREAD_POLL
void t_node_imp::perform_io(){
    while (!x_quit.load(std::memory_order_relaxed)){
        aoo::ip_address addr;
        AooByte buf[AOO_MAX_PACKET_SIZE];
        int nbytes = aoo::socket_receive(x_socket, buf, AOO_MAX_PACKET_SIZE,
                                         &addr, POLL_INTERVAL);
        if (nbytes > 0){
            scoped_lock lock(x_clientmutex);
        #if DEBUG_THREADS
            std::cout << "perform_io: handle_message" << std::endl;
        #endif
            x_client->handleMessage(buf, nbytes, addr.address(), addr.length());
        } else if (nbytes < 0) {
            // ignore errors when quitting
            if (!x_quit){
                aoo::socket_error_print("recv");
            }
            return;
        } else {
            // timeout
        }

        if (x_update.exchange(false, std::memory_order_acquire)){
            scoped_lock lock(x_clientmutex);
        #if DEBUG_THREADS
            std::cout << "perform_io: send" << std::endl;
        #endif
            x_client->send(send, this);
        }
    }
}
#else
void t_node_imp::send_packets(){
    while (!x_quit.load(std::memory_order_relaxed)){
        x_event.wait();

        scoped_lock lock(x_clientmutex);
    #if DEBUG_THREADS
        std::cout << "send_packets" << std::endl;
    #endif
        x_client->send(send, this);
    }
}

void t_node_imp::receive_packets(){
    while (!x_quit.load(std::memory_order_relaxed)){
        aoo::ip_address addr;
        AooByte buf[AOO_MAX_PACKET_SIZE];
        int nbytes = aoo::socket_receive(x_socket, buf, AOO_MAX_PACKET_SIZE, &addr, -1);
        if (nbytes > 0){
            scoped_lock lock(x_clientmutex);
        #if DEBUG_THREADS
            std::cout << "receive_packets" << std::endl;
        #endif
            x_client->handleMessage(buf, nbytes, addr.address(), addr.length());
        } else if (nbytes < 0) {
            // ignore errors when quitting
            if (!x_quit){
                aoo::socket_error_print("recv");
            }
            break;
        } else {
            // ignore empty packets (used for quit signalling)
        }
    }
}
#endif

AooInt32 t_node_imp::send(void *user, const AooByte *msg, AooInt32 size,
                          const void *addr, AooAddrSize len, AooFlag flags)
{
    auto x = (t_node_imp *)user;
    aoo::ip_address dest((sockaddr *)addr, len);
    return aoo::socket_sendto(x->x_socket, msg, size, dest);
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
        // first create socket and bind to given port
        int sock = aoo::socket_udp(port);
        if (sock < 0){
            pd_error(obj, "%s: couldn't bind to port %d", classname(obj), port);
            return nullptr;
        }

        aoo::ip_address addr;
        if (aoo::socket_address(sock, addr) != 0){
            pd_error(obj, "%s: couldn't get socket address", classname(obj));
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
        node = new t_node_imp(s, sock, addr);
    }

    if (!node->add_object(obj, x, id)){
        // never fails for new t_node_imp!
        return nullptr;
    }

    return node;
}

t_node_imp::t_node_imp(t_symbol *s, int socket, const aoo::ip_address& addr)
    : x_proxy(this), x_bindsym(s),
      x_socket(socket), x_port(addr.port()), x_type(addr.type())
{
    LOG_DEBUG("create AooClient on port " << addr.port());
    x_client = AooClient::create(addr.address(), addr.length(), 0, nullptr);

    pd_bind(&x_proxy.x_pd, x_bindsym);

#if NETWORK_THREAD_POLL
    // start network I/O thread
    LOG_DEBUG("start network thread");
    x_iothread = std::thread([this](){
        aoo::sync::lower_thread_priority();
        perform_io();
    });
#else
    // start send thread
    LOG_DEBUG("start network send thread");
    x_sendthread = std::thread([this](){
        aoo::sync::lower_thread_priority();
        send_packets();
    });

    // start receive thread
    LOG_DEBUG("start network receive thread");
    x_recvthread = std::thread([this](){
        aoo::sync::lower_thread_priority();
        receive_packets();
    });
#endif

    verbose(0, "new aoo node on port %d", x_port);
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

    // ask threads to quit
    x_quit = true;

#if NETWORK_THREAD_POLL
    LOG_DEBUG("join network thread");
    aoo::socket_signal(x_socket); // wake perform_io()
    x_iothread.join();
#else
    LOG_DEBUG("join network threads");
    x_event.set(); // wake send_packets()
    aoo::socket_signal(x_socket); // wake receive_packets()
    x_sendthread.join();
    x_recvthread.join();
#endif

    aoo::socket_close(x_socket);

    if (x_clientthread.joinable()){
        x_client->quit();
        x_clientthread.join();
    }

    verbose(0, "released aoo node on port %d", x_port);
}

void aoo_node_setup(void)
{
    node_proxy_class = class_new(gensym("aoo node proxy"), 0, 0,
                                 sizeof(t_node_proxy), CLASS_PD, A_NULL);
}
