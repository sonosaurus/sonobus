/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "aoo_common.hpp"

#include "aoo/aoo_server.hpp"

#include "common/udp_server.hpp"
#include "common/tcp_server.hpp"
#include "common/utils.hpp"

#include <thread>

#define AOO_SERVER_POLL_INTERVAL 2

static t_class *aoo_server_class;

struct t_aoo_server
{
    t_aoo_server(int argc, t_atom *argv);
    ~t_aoo_server();

    t_object x_obj;

    AooServer::Ptr x_server;
    aoo::udp_server x_udpserver;
    aoo::tcp_server x_tcpserver;
    std::thread x_udpthread;
    std::thread x_tcpthread;
    int x_port = 0;
    int x_numclients = 0;
    t_clock *x_clock = nullptr;
    t_outlet *x_stateout = nullptr;
    t_outlet *x_msgout = nullptr;

    void close();
    AooId handle_accept(int e, const aoo::ip_address& addr);
    void handle_receive(int e, AooId client, const aoo::ip_address& addr,
                        const AooByte *data, AooSize size);
    void handle_udp_receive(int e, const aoo::ip_address& addr,
                            const AooByte *data, AooSize size);
    void add_client(AooId id, const aoo::ip_address& addr);
    void remove_client(AooId id, const aoo::ip_address& addr);
};

void t_aoo_server::close() {
    if (x_port > 0) {
        x_udpserver.stop();
        if (x_udpthread.joinable()) {
            x_udpthread.join();
        }

        x_tcpserver.stop();
        if (x_tcpthread.joinable()) {
            x_tcpthread.join();
        }
    }
    clock_unset(x_clock);
    x_port = 0;
}

AooId t_aoo_server::handle_accept(int e, const aoo::ip_address& addr) {
    if (e == 0) {
        // reply function
        auto replyfn = [](void *x, AooId client,
                const AooByte *data, AooSize size) -> AooInt32 {
            return static_cast<aoo::tcp_server *>(x)->send(client, data, size);
        };
        AooId client;
        x_server->addClient(replyfn, &x_tcpserver, &client); // doesn't fail

        sys_lock();
        add_client(client, addr);
        sys_unlock();

        return client;
    } else {
        // called on network thread - must lock Pd!
        sys_lock();
        pd_error(this, "%s: accept() failed: %s",
                 classname(this), aoo::socket_strerror(e).c_str());
        // TODO handle error?
        sys_unlock();
        return kAooIdInvalid;
    }
}

void t_aoo_server::handle_receive(int e, AooId client, const aoo::ip_address& addr,
                                  const AooByte *data, AooSize size) {
    if (e == 0 && size > 0) {
        if (auto err = x_server->handleClientMessage(client, data, size); err != kAooOk) {
            // remove client!
            x_server->removeClient(client);
            x_tcpserver.close(client);

            sys_lock();
            remove_client(client, addr);
            sys_unlock();
        }
    } else {
        // remove client!
        x_server->removeClient(client);
        // called on network thread - must lock Pd!
        sys_lock();
        if (e == 0) {
            verbose(0, "%s: client %d disconnected",
                    classname(this), client);
        } else {
            pd_error(this, "%s: TCP error in client %d: %s",
                     classname(this), client, aoo::socket_strerror(e).c_str());
        }
        remove_client(client, addr);
        // TODO handle error?
        sys_unlock();
    }
}

void t_aoo_server::handle_udp_receive(int e, const aoo::ip_address& addr,
                                    const AooByte *data, AooSize size) {
    if (e == 0) {
        // reply function
        auto replyfn = [](void *x, const AooByte *data, AooInt32 size,
                const void *address, AooAddrSize addrlen, AooFlag) -> AooInt32 {
            aoo::ip_address addr((const struct sockaddr *)address, addrlen);
            return static_cast<aoo::udp_server *>(x)->send(addr, data, size);
        };
        x_server->handleUdpMessage(data, size, addr.address(), addr.length(),
                                   replyfn, &x_udpserver);
    } else {
        // called on network thread - must lock Pd!
        sys_lock();
        pd_error(this, "%s: UDP error: %s",
                 classname(this), aoo::socket_strerror(e).c_str());
        // TODO handle error
        sys_unlock();
    }
}

void t_aoo_server::add_client(AooId id, const aoo::ip_address& addr) {
    t_atom msg[3];

    char strid[64];
    snprintf(strid, sizeof(strid), "0x%X", id);
    SETSYMBOL(msg, gensym(strid));
    address_to_atoms(addr, 2, msg + 1);

    outlet_anything(x_msgout, gensym("client_add"), 3, msg);

    x_numclients++;

    outlet_float(x_stateout, x_numclients);
}

void t_aoo_server::remove_client(AooId id, const aoo::ip_address& addr) {
    t_atom msg[3];

    char strid[64];
    snprintf(strid, sizeof(strid), "0x%X", id);
    SETSYMBOL(msg, gensym(strid));
    address_to_atoms(addr, 2, msg + 1);

    outlet_anything(x_msgout, gensym("client_remove"), 3, msg);

    x_numclients--;

    outlet_float(x_stateout, x_numclients);
}

static void aoo_server_handle_event(t_aoo_server *x, const AooEvent *event, int32_t)
{
    switch (event->type) {
    case kAooEventClientLogin:
    {
        auto& e = event->clientLogin;

        // TODO: address + metadata
        t_atom msg;
        char id[64];
        snprintf(id, sizeof(id), "0x%X", e.id);
        SETSYMBOL(&msg, gensym(id));

        if (e.error == kAooOk) {
            outlet_anything(x->x_msgout, gensym("client_login"), 1, &msg);
        } else {
            pd_error(x, "%s: client %d failed to login: %s",
                     classname(x), e.id, aoo_strerror(e.error));
            // For now, we expect the client to disconnect. LATER maybe close client.
        }

        break;
    }
    case kAooEventGroupAdd:
    {
        // TODO add group
        t_atom msg;
        SETSYMBOL(&msg, gensym(event->groupAdd.name));
        // TODO metadata
        outlet_anything(x->x_msgout, gensym("group_add"), 1, &msg);

        break;
    }
    case kAooEventGroupRemove:
    {
        // TODO remove group
        t_atom msg;
        SETSYMBOL(&msg, gensym(event->groupRemove.name));
        outlet_anything(x->x_msgout, gensym("group_remove"), 1, &msg);

        break;
    }
    case kAooEventGroupJoin:
    {
        auto& e = event->groupJoin;

        t_atom msg[3];
        SETSYMBOL(msg, gensym(e.groupName));
        SETSYMBOL(msg + 1, gensym(e.userName));
        SETFLOAT(msg + 2, e.userId); // always small
        outlet_anything(x->x_msgout, gensym("group_join"), 3, msg);

        break;
    }
    case kAooEventGroupLeave:
    {
        auto& e = event->groupLeave;

        t_atom msg[3];
        SETSYMBOL(msg, gensym(e.groupName));
        SETSYMBOL(msg + 1, gensym(e.userName));
        SETFLOAT(msg + 2, e.userId); // always small
        outlet_anything(x->x_msgout, gensym("group_leave"), 3, msg);

        break;
    }
    case kAooEventError:
    {
        pd_error(x, "%s: %s", classname(x), event->error.errorMessage);
        break;
    }
    default:
        verbose(0, "%s: unknown event type %d", classname(x), event->type);
        break;
    }
}

static void aoo_server_tick(t_aoo_server *x)
{
    x->x_server->pollEvents();
    clock_delay(x->x_clock, AOO_SERVER_POLL_INTERVAL);
}

static void aoo_server_relay(t_aoo_server *x, t_floatarg f) {
    if (x->x_server) {
        x->x_server->setServerRelay(f != 0);
    }
}

static void aoo_server_port(t_aoo_server *x, t_floatarg f)
{
    int port = f;
    if (port == x->x_port) {
        return;
    }

    x->close();

    x->x_numclients = 0;
    outlet_float(x->x_stateout, 0);

    if (port > 0) {
        try {
            // setup UDP server
            // TODO: increase socket receive buffer for relay? Use threaded receive?
            x->x_udpserver.start(port,
                [x](auto&&... args) { x->handle_udp_receive(args...); });

            auto flags = aoo::socket_family(x->x_udpserver.socket()) == aoo::ip_address::IPv6 ?
                             kAooSocketDualStack : kAooSocketIPv4;

            x->x_server->setup(port, flags);

            // setup TCP server
            x->x_tcpserver.start(port,
                [x](auto&&... args) { return x->handle_accept(args...); },
                [x](auto&&... args) { x->handle_receive(args...); });

            verbose(0, "aoo server listening on port %d", port);
        } catch (const std::runtime_error& e) {
            pd_error(x, "%s: %s", classname(x), e.what());
            return;
        }

        // first set event handler!
        x->x_server->setEventHandler((AooEventHandler)aoo_server_handle_event,
                                     x, kAooEventModePoll);
        // then start network threads
        x->x_udpthread = std::thread([x, pd=pd_this]() {
        #ifdef PDINSTANCE
            pd_setinstance(pd);
        #endif
            x->x_udpserver.run(-1);
        });
        x->x_tcpthread = std::thread([x, pd=pd_this]() {
        #ifdef PDINSTANCE
            pd_setinstance(pd);
        #endif
            x->x_tcpserver.run();
        });
        // start clock
        clock_delay(x->x_clock, AOO_SERVER_POLL_INTERVAL);

        x->x_port = port;
    }
}

static void * aoo_server_new(t_symbol *s, int argc, t_atom *argv)
{
    void *x = pd_new(aoo_server_class);
    new (x) t_aoo_server(argc, argv);
    return x;
}

t_aoo_server::t_aoo_server(int argc, t_atom *argv)
{
    x_clock = clock_new(this, (t_method)aoo_server_tick);
    x_stateout = outlet_new(&x_obj, 0);
    x_msgout = outlet_new(&x_obj, 0);

    x_server = AooServer::create(nullptr);

    int port = atom_getfloatarg(0, argc, argv);
    aoo_server_port(this, port);
}

static void aoo_server_free(t_aoo_server *x)
{
    x->~t_aoo_server();
}

t_aoo_server::~t_aoo_server()
{
    close();
    clock_free(x_clock);
}

void aoo_server_setup(void)
{
    aoo_server_class = class_new(gensym("aoo_server"), (t_newmethod)(void *)aoo_server_new,
        (t_method)aoo_server_free, sizeof(t_aoo_server), 0, A_GIMME, A_NULL);
    class_sethelpsymbol(aoo_server_class, gensym("aoo_net"));
    class_addmethod(aoo_server_class, (t_method)aoo_server_relay,
         gensym("relay"), A_FLOAT, A_NULL);
    class_addmethod(aoo_server_class, (t_method)aoo_server_port,
         gensym("port"), A_FLOAT, A_NULL);
}
