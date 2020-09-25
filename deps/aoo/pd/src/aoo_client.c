/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "aoo/aoo_net.h"

#include "aoo_common.h"

#include <pthread.h>

#define AOO_CLIENT_POLL_INTERVAL 2

t_class *aoo_client_class;

typedef struct _aoo_client
{
    t_object x_obj;
    aoonet_client *x_client;
    t_aoo_node *x_node;
    pthread_t x_thread;
    t_clock *x_clock;
    t_outlet *x_stateout;
    t_outlet *x_msgout;
} t_aoo_client;

void aoo_client_send(t_aoo_client *x)
{
    aoonet_client_send(x->x_client);
}

void aoo_client_handle_message(t_aoo_client *x, const char * data,
                               int32_t n, void *endpoint, aoo_replyfn fn)
{
    t_endpoint *e = (t_endpoint *)endpoint;
    aoonet_client_handle_message(x->x_client, data, n, &e->addr);
}

static int32_t aoo_client_handle_events(t_aoo_client *x,
                                        const aoo_event **events, int32_t n)
{
    for (int i = 0; i < n; ++i){
        switch (events[i]->type){
        case AOONET_CLIENT_CONNECT_EVENT:
        {
            aoonet_client_group_event *e = (aoonet_client_group_event *)events[i];
            if (e->result > 0){
                outlet_float(x->x_stateout, 1); // connected
            } else {
                pd_error(x, "%s: couldn't connect to server - %s",
                         classname(x), e->errormsg);

                outlet_float(x->x_stateout, 0); // disconnected
            }
            break;
        }
        case AOONET_CLIENT_DISCONNECT_EVENT:
        {
            aoonet_client_group_event *e = (aoonet_client_group_event *)events[i];
            if (e->result == 0){
                pd_error(x, "%s: disconnected from server - %s",
                         classname(x), e->errormsg);
            }

            aoo_node_remove_all_peers(x->x_node);

            outlet_float(x->x_stateout, 0); // disconnected
            break;
        }
        case AOONET_CLIENT_GROUP_JOIN_EVENT:
        {
            aoonet_client_group_event *e = (aoonet_client_group_event *)events[i];
            if (e->result > 0){
                t_atom msg;
                SETSYMBOL(&msg, gensym(e->name));
                outlet_anything(x->x_msgout, gensym("group_join"), 1, &msg);
            } else {
                pd_error(x, "%s: couldn't join group %s - %s",
                         classname(x), e->name, e->errormsg);
            }
            break;
        }
        case AOONET_CLIENT_GROUP_LEAVE_EVENT:
        {
            aoonet_client_group_event *e = (aoonet_client_group_event *)events[i];
            if (e->result > 0){
                aoo_node_remove_group(x->x_node, gensym(e->name));

                t_atom msg;
                SETSYMBOL(&msg, gensym(e->name));
                outlet_anything(x->x_msgout, gensym("group_leave"), 1, &msg);
            } else {
                pd_error(x, "%s: couldn't leave group %s - %s",
                         classname(x), e->name, e->errormsg);
            }
            break;
        }
        case AOONET_CLIENT_PEER_JOIN_EVENT:
        {
            aoonet_client_peer_event *e = (aoonet_client_peer_event *)events[i];

            if (e->result > 0){
                aoo_node_add_peer(x->x_node, gensym(e->group), gensym(e->user),
                                  (const struct sockaddr *)e->address, e->length);

                t_atom msg[4];
                SETSYMBOL(msg, gensym(e->group));
                SETSYMBOL(msg + 1, gensym(e->user));
                if (sockaddr_to_atoms((const struct sockaddr *)e->address,
                                      e->length, msg + 2))
                {
                    outlet_anything(x->x_msgout, gensym("peer_join"), 4, msg);
                }
            } else {
                bug("%s: AOONET_CLIENT_PEER_JOIN_EVENT", classname(x));
            }
            break;
        }
        case AOONET_CLIENT_PEER_LEAVE_EVENT:
        {
            aoonet_client_peer_event *e = (aoonet_client_peer_event *)events[i];

            if (e->result > 0){
                aoo_node_remove_peer(x->x_node, gensym(e->group), gensym(e->user));

                t_atom msg[4];
                SETSYMBOL(msg, gensym(e->group));
                SETSYMBOL(msg + 1, gensym(e->user));
                if (sockaddr_to_atoms((const struct sockaddr *)e->address,
                                      e->length, msg + 2))
                {
                    outlet_anything(x->x_msgout, gensym("peer_leave"), 4, msg);
                }
            } else {
                bug("%s: AOONET_CLIENT_PEER_LEAVE_EVENT", classname(x));
            }
            break;
        }
        case AOONET_CLIENT_ERROR_EVENT:
        {
            aoonet_client_event *e = (aoonet_client_event *)events[i];
            pd_error(x, "%s: %s", classname(x), e->errormsg);
            break;
        }
        default:
            pd_error(x, "%s: got unknown event %d", classname(x), events[i]->type);
            break;
        }
    }
    return 1;
}

static void aoo_client_tick(t_aoo_client *x)
{
    aoonet_client_handle_events(x->x_client,
                               (aoo_eventhandler)aoo_client_handle_events, x);

    aoo_node_notify(x->x_node);

    clock_delay(x->x_clock, AOO_CLIENT_POLL_INTERVAL);
}

static void * aoo_client_threadfn(void *y)
{
    t_aoo_client *x = (t_aoo_client *)y;
    aoonet_client_run(x->x_client);
    return 0;
}

static void aoo_client_connect(t_aoo_client *x, t_symbol *s, int argc, t_atom *argv)
{
    if (argc < 4){
        pd_error(x, "%s: too few arguments for '%s' method", classname(x), s->s_name);
        return;
    }
    if (x->x_client){
        // first remove peers (to be sure)
        aoo_node_remove_all_peers(x->x_node);

        t_symbol *host = atom_getsymbol(argv);
        int port = atom_getfloat(argv + 1);
        t_symbol *username = atom_getsymbol(argv + 2);
        t_symbol *pwd = atom_getsymbol(argv + 3);

        aoonet_client_connect(x->x_client, host->s_name, port,
                              username->s_name, pwd->s_name);
    }
}

static void aoo_client_disconnect(t_aoo_client *x)
{
    if (x->x_client){
        aoo_node_remove_all_peers(x->x_node);

        aoonet_client_disconnect(x->x_client);
    }
}

static void aoo_client_group_join(t_aoo_client *x, t_symbol *group, t_symbol *pwd)
{
    if (x->x_client){
        aoonet_client_group_join(x->x_client, group->s_name, pwd->s_name);
    }
}

static void aoo_client_group_leave(t_aoo_client *x, t_symbol *s)
{
    if (x->x_client){
        aoonet_client_group_leave(x->x_client, s->s_name);
    }
}

static void * aoo_client_new(t_symbol *s, int argc, t_atom *argv)
{
    t_aoo_client *x = (t_aoo_client *)pd_new(aoo_client_class);

    x->x_clock = clock_new(x, (t_method)aoo_client_tick);
    x->x_stateout = outlet_new(&x->x_obj, 0);
    x->x_msgout = outlet_new(&x->x_obj, 0);

    int port = argc ? atom_getfloat(argv) : 0;

    x->x_node = port > 0 ? aoo_node_add(port, (t_pd *)x, 0) : 0;

    if (x->x_node){
        x->x_client = aoonet_client_new(x->x_node, (aoo_sendfn)aoo_node_sendto, port);
        if (x->x_client){
            verbose(0, "new aoo client on port %d", port);
            // start thread
            pthread_create(&x->x_thread, 0, aoo_client_threadfn, x);
            // start clock
            clock_delay(x->x_clock, AOO_CLIENT_POLL_INTERVAL);
        }
    }
    return x;
}

static void aoo_client_free(t_aoo_client *x)
{
    if (x->x_node){
        aoo_node_remove_all_peers(x->x_node);

        aoo_node_release(x->x_node, (t_pd *)x, 0);
    }

    if (x->x_client){
        aoonet_client_quit(x->x_client);
        // wait for thread to finish
        pthread_join(x->x_thread, 0);
        aoonet_client_free(x->x_client);
    }

    clock_free(x->x_clock);
}

void aoo_client_setup(void)
{
    aoo_client_class = class_new(gensym("aoo_client"), (t_newmethod)(void *)aoo_client_new,
        (t_method)aoo_client_free, sizeof(t_aoo_client), 0, A_GIMME, A_NULL);
    class_sethelpsymbol(aoo_client_class, gensym("aoo_server"));

    class_addmethod(aoo_client_class, (t_method)aoo_client_connect,
                    gensym("connect"), A_GIMME, A_NULL);
    class_addmethod(aoo_client_class, (t_method)aoo_client_disconnect,
                    gensym("disconnect"), A_NULL);
    class_addmethod(aoo_client_class, (t_method)aoo_client_group_join,
                    gensym("group_join"), A_SYMBOL, A_SYMBOL, A_NULL);
    class_addmethod(aoo_client_class, (t_method)aoo_client_group_leave,
                    gensym("group_leave"), A_SYMBOL, A_NULL);
}
