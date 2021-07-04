/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "aoo_common.hpp"

#include <thread>

using namespace aoo;

#define AOO_SERVER_POLL_INTERVAL 2

static t_class *aoo_server_class;

struct t_aoo_server
{
    t_aoo_server(int argc, t_atom *argv);
    ~t_aoo_server();

    t_object x_obj;

    aoo::net::server::pointer x_server;
    int32_t x_numusers = 0;
    std::thread x_thread;
    t_clock *x_clock = nullptr;
    t_outlet *x_stateout = nullptr;
    t_outlet *x_msgout = nullptr;
};

static void aoo_server_handle_event(t_aoo_server *x, const aoo_event *event, int32_t)
{
    switch (event->type){
    case AOO_NET_USER_JOIN_EVENT:
    {
        auto e = (const aoo_net_user_event *)event;

        aoo::ip_address addr((const sockaddr *)e->address, e->addrlen);

        t_atom msg[4];
        SETSYMBOL(msg, gensym(e->user_name));
        SETFLOAT(msg + 1, e->user_id);
        address_to_atoms(addr, 2, msg + 2);

        outlet_anything(x->x_msgout, gensym("user_join"), 4, msg);

        x->x_numusers++;

        outlet_float(x->x_stateout, x->x_numusers);

        break;
    }
    case AOO_NET_USER_LEAVE_EVENT:
    {
        auto e = (const aoo_net_user_event *)event;

        aoo::ip_address addr((const sockaddr *)e->address, e->addrlen);

        t_atom msg[4];
        SETSYMBOL(msg, gensym(e->user_name));
        SETFLOAT(msg + 1, e->user_id);
        address_to_atoms(addr, 2, msg + 2);

        outlet_anything(x->x_msgout, gensym("user_leave"), 4, msg);

        x->x_numusers--;

        outlet_float(x->x_stateout, x->x_numusers);

        break;
    }
    case AOO_NET_GROUP_JOIN_EVENT:
    {
        auto e = (const aoo_net_group_event *)event;

        t_atom msg[3];
        SETSYMBOL(msg, gensym(e->group_name));
        SETSYMBOL(msg + 1, gensym(e->user_name));
        SETFLOAT(msg + 2, e->user_id);
        outlet_anything(x->x_msgout, gensym("group_join"), 3, msg);

        break;
    }
    case AOO_NET_GROUP_LEAVE_EVENT:
    {
        auto e = (const aoo_net_group_event *)event;

        t_atom msg[3];
        SETSYMBOL(msg, gensym(e->group_name));
        SETSYMBOL(msg + 1, gensym(e->user_name));
        SETFLOAT(msg + 2, e->user_id);
        outlet_anything(x->x_msgout, gensym("group_leave"), 3, msg);

        break;
    }
    case AOO_NET_ERROR_EVENT:
    {
        auto e = (const aoo_net_error_event *)event;
        pd_error(x, "%s: %s", classname(x), e->error_message);
        break;
    }
    default:
        pd_error(x, "%s: got unknown event %d", classname(x), event->type);
        break;
    }
}

static void aoo_server_tick(t_aoo_server *x)
{
    x->x_server->poll_events();
    clock_delay(x->x_clock, AOO_SERVER_POLL_INTERVAL);
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

    int port = argc ? atom_getfloat(argv) : 0;

    if (port > 0){
        aoo_error err;
        x_server.reset(aoo::net::server::create(port, 0, &err));
        if (x_server){
            verbose(0, "aoo server listening on port %d", port);
            // first set event handler!
            // set event handler
            x_server->set_eventhandler((aoo_eventhandler)aoo_server_handle_event,
                                       this, AOO_EVENT_POLL);
            // start thread
            x_thread = std::thread([this](){
                x_server->run();
            });
            // start clock
            clock_delay(x_clock, AOO_SERVER_POLL_INTERVAL);
        } else {
            char buf[MAXPDSTRING];
            socket_strerror(socket_errno(), buf, sizeof(buf));
            pd_error(this, "%s: %s (%d)", classname(this), buf, err);
        }
    }
}

static void aoo_server_free(t_aoo_server *x)
{
    x->~t_aoo_server();
}

t_aoo_server::~t_aoo_server()
{
    if (x_server){
        x_server->quit();
        // wait for thread to finish
        x_thread.join();
    }
    clock_free(x_clock);
}

void aoo_server_setup(void)
{
    aoo_server_class = class_new(gensym("aoo_server"), (t_newmethod)(void *)aoo_server_new,
        (t_method)aoo_server_free, sizeof(t_aoo_server), 0, A_GIMME, A_NULL);
    class_sethelpsymbol(aoo_server_class, gensym("aoo_net"));
}
