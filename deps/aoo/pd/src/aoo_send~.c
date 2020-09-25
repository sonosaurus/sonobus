/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others. 
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "aoo_common.h"

#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>

#ifdef _WIN32
# include <malloc.h> // MSVC or mingw on windows
# ifdef _MSC_VER
#  define alloca _alloca
# endif
#elif defined(__linux__) || defined(__APPLE__)
# include <alloca.h> // linux, mac, mingw, cygwin
#else
# include <stdlib.h> // BSDs for example
#endif

// for hardware buffer sizes up to 1024 @ 44.1 kHz
#define DEFBUFSIZE 25

t_class *aoo_send_class;

typedef struct _sink
{
    t_endpoint *s_endpoint;
    int32_t s_id;
} t_sink;

typedef struct _aoo_send
{
    t_object x_obj;
    t_float x_f;
    aoo_source *x_aoo_source;
    int32_t x_samplerate;
    int32_t x_blocksize;
    int32_t x_nchannels;
    int32_t x_port;
    int32_t x_id;
    t_float **x_vec;
    // sinks
    t_sink *x_sinks;
    int x_numsinks;
    // node
    t_aoo_node *x_node;
    aoo_lock x_lock;
    // events
    t_clock *x_clock;
    t_outlet *x_msgout;
    int x_accept;
} t_aoo_send;

static void aoo_send_doaddsink(t_aoo_send *x, t_endpoint *e, int32_t id)
{
    aoo_source_add_sink(x->x_aoo_source, e, id, (aoo_replyfn)endpoint_send);

    // add sink to list
    int n = x->x_numsinks;
    if (n){
        x->x_sinks = (t_sink *)resizebytes(x->x_sinks,
            n * sizeof(t_sink), (n + 1) * sizeof(t_sink));
    } else {
        x->x_sinks = (t_sink *)getbytes(sizeof(t_sink));
    }
    t_sink *sink = &x->x_sinks[n];
    sink->s_endpoint = e;
    sink->s_id = id;
    x->x_numsinks++;

    // output message
    t_atom msg[3];
    if (aoo_endpoint_to_atoms(e, id, msg)){
        outlet_anything(x->x_msgout, gensym("sink_add"), 3, msg);
    } else {
        bug("aoo_endpoint_to_atoms");
    }
}

static void aoo_send_doremoveall(t_aoo_send *x)
{
    aoo_source_remove_all(x->x_aoo_source);

    int numsinks = x->x_numsinks;
    if (!numsinks){
        return;
    }

    // temporary copies
    t_sink *sinks = (t_sink *)alloca(sizeof(t_sink) * numsinks);
    memcpy(sinks, x->x_sinks, sizeof(t_sink) * numsinks);

    // clear sink list
    freebytes(x->x_sinks, x->x_numsinks * sizeof(t_sink));
    x->x_numsinks = 0;

    // output messages
    for (int i = 0; i < numsinks; ++i){
        t_atom msg[3];
        if (aoo_endpoint_to_atoms(sinks[i].s_endpoint, sinks[i].s_id, msg))
        {
            outlet_anything(x->x_msgout, gensym("sink_remove"), 3, msg);
        } else {
            bug("aoo_endpoint_to_atoms");
        }
    }
}

static void aoo_send_doremovesink(t_aoo_send *x, t_endpoint *e, int32_t id)
{
    aoo_source_remove_sink(x->x_aoo_source, e, id);

    // remove from list
    int n = x->x_numsinks;
    if (n > 1){
        if (id == AOO_ID_WILDCARD){
            // pre-allocate temporary copies
            int removed = 0;
            t_sink *sinks = (t_sink *)alloca(sizeof(t_sink) * n);

            // remove all sinks matching endpoint
            t_sink *end = x->x_sinks + n;
            for (t_sink *s = x->x_sinks; s != end; ){
                if (s->s_endpoint == e){
                    sinks[removed++] = *s;
                    memmove(s, s + 1, (end - s - 1) * sizeof(t_sink));
                    end--;
                } else {
                    s++;
                }
            }
            int newsize = end - x->x_sinks;
            if (newsize > 0){
                x->x_sinks = (t_sink *)resizebytes(x->x_sinks,
                    n * sizeof(t_sink), newsize * sizeof(t_sink));
            } else {
                freebytes(x->x_sinks, n * sizeof(t_sink));
                x->x_sinks = 0;
            }
            x->x_numsinks = newsize;

            // output messages
            for (int i = 0; i < removed; ++i){
                t_atom msg[3];
                if (aoo_endpoint_to_atoms(sinks[i].s_endpoint, sinks[i].s_id, msg))
                {
                    outlet_anything(x->x_msgout, gensym("sink_remove"), 3, msg);
                } else {
                    bug("aoo_endpoint_to_atoms");
                }
            }
            return;
        } else {
            // remove the sink matching endpoint and id
            for (int i = 0; i < n; ++i){
                if ((x->x_sinks[i].s_endpoint == e) &&
                    (x->x_sinks[i].s_id == id))
                {
                    memmove(&x->x_sinks[i], &x->x_sinks[i + 1],
                            (n - i - 1) * sizeof(t_sink));
                    x->x_sinks = (t_sink *)resizebytes(x->x_sinks,
                        n * sizeof(t_sink), (n - 1) * sizeof(t_sink));
                    x->x_numsinks--;

                    // output message
                    t_atom msg[3];
                    if (aoo_endpoint_to_atoms(e, id, msg)){
                        outlet_anything(x->x_msgout, gensym("sink_remove"), 3, msg);
                    } else {
                        bug("aoo_endpoint_to_atoms");
                    }
                    return;
                }
            }
        }
    } else if (n == 1) {
        if ((x->x_sinks->s_endpoint == e) &&
            (id == AOO_ID_WILDCARD || id == x->x_sinks->s_id))
        {
            // get the actual sink ID, in case 'id' is a wildcard
            int32_t old = x->x_sinks->s_id;

            freebytes(x->x_sinks, sizeof(t_sink));
            x->x_sinks = 0;
            x->x_numsinks = 0;

            // output message
            t_atom msg[3];
            if (aoo_endpoint_to_atoms(e, old, msg)){
                outlet_anything(x->x_msgout, gensym("sink_remove"), 3, msg);
            } else {
                bug("aoo_endpoint_to_atoms");
            }
            return;
        }
    }

    // wildcard IDs are allowed to not match anything
    if (id != AOO_ID_WILDCARD){
        bug("aoo_send_doremovesink");
    }
}

// called from the network receive thread
void aoo_send_handle_message(t_aoo_send *x, const char * data,
                                int32_t n, void *endpoint, aoo_replyfn fn)
{
    // synchronize with aoo_receive_dsp()
    aoo_lock_lock_shared(&x->x_lock);
    // handle incoming message
    aoo_source_handle_message(x->x_aoo_source, data, n, endpoint, fn);
    aoo_lock_unlock_shared(&x->x_lock);
}

// called from the network send thread
void aoo_send_send(t_aoo_send *x)
{
    // synchronize with aoo_receive_dsp()
    aoo_lock_lock_shared(&x->x_lock);
    // send outgoing messages
    while (aoo_source_send(x->x_aoo_source)) ;
    aoo_lock_unlock_shared(&x->x_lock);
}

static int32_t aoo_send_handle_events(t_aoo_send *x, const aoo_event **events, int32_t n)
{
    for (int i = 0; i < n; ++i){
        switch (events[i]->type){
        case AOO_PING_EVENT:
        {
            aoo_ping_event *e = (aoo_ping_event *)events[i];
            double diff1 = aoo_osctime_duration(e->tt1, e->tt2) * 1000.0;
            double diff2 = aoo_osctime_duration(e->tt2, e->tt3) * 1000.0;
            double rtt = aoo_osctime_duration(e->tt1, e->tt3) * 1000.0;

            t_atom msg[7];
            if (aoo_endpoint_to_atoms(e->endpoint, e->id, msg)){
                SETFLOAT(msg + 3, diff1);
                SETFLOAT(msg + 4, diff2);
                SETFLOAT(msg + 5, rtt);
                SETFLOAT(msg + 6, e->lost_blocks);
                outlet_anything(x->x_msgout, gensym("ping"), 7, msg);
            } else {
                bug("aoo_endpoint_to_atoms");
            }
            break;
        }
        case AOO_INVITE_EVENT:
        {
            aoo_sink_event *e = (aoo_sink_event *)events[i];

            if (x->x_accept){
                aoo_send_doaddsink(x, e->endpoint, e->id);
            } else {
                t_atom msg[3];
                if (aoo_endpoint_to_atoms(e->endpoint, e->id, msg)){
                    outlet_anything(x->x_msgout, gensym("invite"), 3, msg);
                } else {
                    bug("aoo_endpoint_to_atoms");
                }
            }

            break;
        }
        case AOO_UNINVITE_EVENT:
        {
            aoo_sink_event *e = (aoo_sink_event *)events[i];

            if (x->x_accept){
                aoo_send_doremovesink(x, e->endpoint, e->id);
            } else {
                t_atom msg[3];
                if (aoo_endpoint_to_atoms(e->endpoint, e->id, msg)){
                    outlet_anything(x->x_msgout, gensym("uninvite"), 3, msg);
                } else {
                    bug("aoo_endpoint_to_atoms");
                }
            }
            break;
        }
        default:
            break;
        }
    }
    return 1;
}

static void aoo_send_tick(t_aoo_send *x)
{
    aoo_source_handle_events(x->x_aoo_source, (aoo_eventhandler)aoo_send_handle_events, x);
}

static void aoo_send_format(t_aoo_send *x, t_symbol *s, int argc, t_atom *argv)
{
    aoo_format_storage f;
    f.header.nchannels = x->x_nchannels;
    if (aoo_format_parse(x, &f, argc, argv)){
        aoo_source_set_format(x->x_aoo_source, &f.header);
        // output actual format
        t_atom msg[16];
        int n = aoo_format_toatoms(&f.header, 16, msg);
        if (n > 0){
            outlet_anything(x->x_msgout, gensym("format"), n, msg);
        }
    }
}

static t_sink *aoo_send_findsink(t_aoo_send *x,
                                 const struct sockaddr_storage *sa, int32_t id)
{
    for (int i = 0; i < x->x_numsinks; ++i){
        if (x->x_sinks[i].s_id == id &&
            endpoint_match(x->x_sinks[i].s_endpoint, sa))
        {
            return &x->x_sinks[i];
        }
    }
    return 0;
}

static void aoo_send_accept(t_aoo_send *x, t_floatarg f)
{
    x->x_accept = f != 0;
}

static void aoo_send_channel(t_aoo_send *x, t_symbol *s, int argc, t_atom *argv)
{
    struct sockaddr_storage sa;
    socklen_t len;
    int32_t id;
    if (argc < 4){
        pd_error(x, "%s: too few arguments for 'channel' message", classname(x));
        return;
    }
    if (aoo_getsinkarg(x, x->x_node, argc, argv, &sa, &len, &id)){
        t_sink *sink = aoo_send_findsink(x, &sa, id);
        if (!sink){
            pd_error(x, "%s: couldn't find sink!", classname(x));
            return;
        }
        int32_t chn = atom_getfloat(argv + 3);

        aoo_source_set_sink_channelonset(x->x_aoo_source, sink->s_endpoint, sink->s_id, chn);
    }
}

static void aoo_send_packetsize(t_aoo_send *x, t_floatarg f)
{
    aoo_source_set_packetsize(x->x_aoo_source, f);
}

static void aoo_send_ping(t_aoo_send *x, t_floatarg f)
{
    aoo_source_set_ping_interval(x->x_aoo_source, f);
}

static void aoo_send_resend(t_aoo_send *x, t_floatarg f)
{
    aoo_source_set_buffersize(x->x_aoo_source, f);
}

static void aoo_send_redundancy(t_aoo_send *x, t_floatarg f)
{
    aoo_source_set_redundancy(x->x_aoo_source, f);
}

static void aoo_send_timefilter(t_aoo_send *x, t_floatarg f)
{
    aoo_source_set_timefilter_bandwith(x->x_aoo_source, f);
}

static void aoo_send_add(t_aoo_send *x, t_symbol *s, int argc, t_atom *argv)
{
    if (!x->x_node){
        pd_error(x, "%s: can't add sink - no socket!", classname(x));
        return;
    }

    if (argc < 3){
        pd_error(x, "%s: too few arguments for 'add' message", classname(x));
        return;
    }

    struct sockaddr_storage sa;
    socklen_t len;
    int32_t id;
    if (aoo_getsinkarg(x, x->x_node, argc, argv, &sa, &len, &id)){
        t_symbol *host = atom_getsymbol(argv);
        int port = atom_getfloat(argv + 1);
        t_endpoint *e = aoo_node_endpoint(x->x_node, &sa, len);
        // check if sink exists
        if (id != AOO_ID_WILDCARD){
            for (int i = 0; i < x->x_numsinks; ++i){
                if (x->x_sinks[i].s_endpoint == e){
                    if (x->x_sinks[i].s_id == AOO_ID_WILDCARD){
                        pd_error(x, "%s: sink %s %d %d already added via wildcard!",
                                 classname(x), host->s_name, port, id);
                        return;
                    } else if (x->x_sinks[i].s_id == id){
                        pd_error(x, "%s: sink %s %d %d already added!",
                                 classname(x), host->s_name, port, id);
                        return;
                    }
                }
            }
        }

        if (argc > 3){
            int32_t chn = atom_getfloat(argv + 3);
            aoo_source_set_sink_channelonset(x->x_aoo_source, e, id, chn);
        }

        if (id == AOO_ID_WILDCARD){
            // first remove all sinks on this endpoint
            aoo_send_doremovesink(x, e, AOO_ID_WILDCARD);
        }

        aoo_send_doaddsink(x, e, id);

        // print message (use actual hostname)
        if (endpoint_getaddress(e, &host, &port)){
            if (id == AOO_ID_WILDCARD){
                verbose(0, "added all sinks on %s %d", host->s_name, port);
            } else {
                verbose(0, "added sink %s %d %d", host->s_name, port, id);
            }
        }
    }
}

static void aoo_send_remove(t_aoo_send *x, t_symbol *s, int argc, t_atom *argv)
{
    if (!x->x_node){
        pd_error(x, "%s: can't remove sink - no socket!", classname(x));
        return;
    }

    if (!argc){
        aoo_send_doremoveall(x);
        return;
    }

    if (argc < 3){
        pd_error(x, "%s: too few arguments for 'remove' message", classname(x));
        return;
    }

    struct sockaddr_storage sa;
    socklen_t len;
    int32_t id;
    if (aoo_getsinkarg(x, x->x_node, argc, argv, &sa, &len, &id)){
        t_symbol *host = atom_getsymbol(argv);
        int port = atom_getfloat(argv + 1);
        t_endpoint *e = 0;
        if (id != AOO_ID_WILDCARD){
            // check if sink exists
            for (int i = 0; i < x->x_numsinks; ++i){
                t_sink *sink = &x->x_sinks[i];
                if (endpoint_match(sink->s_endpoint, &sa)){
                    if (sink->s_id == AOO_ID_WILDCARD){
                        pd_error(x, "%s: can't remove sink %s %d %d because of wildcard!",
                                 classname(x), host->s_name, port, id);
                        return;
                    } else if (sink->s_id == id) {
                        e = sink->s_endpoint;
                        break;
                    }
                }
            }
        } else {
            e = aoo_node_endpoint(x->x_node, &sa, len);
        }

        if (!e){
            pd_error(x, "%s: couldn't find sink %s %d %d!",
                     classname(x), host->s_name, port, id);
            return;
        }

        aoo_send_doremovesink(x, e, id);

        // print message (use actual hostname)
        if (endpoint_getaddress(e, &host, &port)){
            if (id == AOO_ID_WILDCARD){
                verbose(0, "removed all sinks on %s %d", host->s_name, port);
            } else {
                verbose(0, "removed sink %s %d %d", host->s_name, port, id);
            }
        }
    }
}

static void aoo_send_start(t_aoo_send *x)
{
    aoo_source_start(x->x_aoo_source);
}

static void aoo_send_stop(t_aoo_send *x)
{
    aoo_source_stop(x->x_aoo_source);
}

static void aoo_send_listsinks(t_aoo_send *x)
{
    for (int i = 0; i < x->x_numsinks; ++i){
        t_sink *s = &x->x_sinks[i];
        t_symbol *host;
        int port;
        if (endpoint_getaddress(s->s_endpoint, &host, &port)){
            t_atom msg[3];
            SETSYMBOL(msg, host);
            SETFLOAT(msg + 1, port);
            if (s->s_id == AOO_ID_WILDCARD){
                SETSYMBOL(msg + 2, gensym("*"));
            } else {
                SETFLOAT(msg + 2, s->s_id);
            }
            outlet_anything(x->x_msgout, gensym("sink"), 3, msg);
        } else {
            pd_error(x, "%s: couldn't get endpoint address for sink", classname(x));
        }
    }
}

static t_int * aoo_send_perform(t_int *w)
{
    t_aoo_send *x = (t_aoo_send *)(w[1]);
    int n = (int)(w[2]);

    assert(sizeof(t_sample) == sizeof(aoo_sample));

    uint64_t t = aoo_osctime_get();
    if (aoo_source_process(x->x_aoo_source, (const aoo_sample **)x->x_vec, n, t) > 0){
        if (x->x_node){
            aoo_node_notify(x->x_node);
        }
    }
    if (aoo_source_events_available(x->x_aoo_source) > 0){
        clock_set(x->x_clock, 0);
    }

    return w + 3;
}

static void aoo_send_dsp(t_aoo_send *x, t_signal **sp)
{
    int32_t blocksize = sp[0]->s_n;
    int32_t samplerate = sp[0]->s_sr;

    for (int i = 0; i < x->x_nchannels; ++i){
        x->x_vec[i] = sp[i]->s_vec;
    }

    // synchronize with network threads!
    aoo_lock_lock(&x->x_lock); // writer lock!

    if (blocksize != x->x_blocksize || samplerate != x->x_samplerate){
        aoo_source_setup(x->x_aoo_source, samplerate, blocksize, x->x_nchannels);
        x->x_blocksize = blocksize;
        x->x_samplerate = samplerate;
    }

    aoo_lock_unlock(&x->x_lock);

    dsp_add(aoo_send_perform, 2, (t_int)x, (t_int)x->x_blocksize);
}

static void aoo_send_port(t_aoo_send *x, t_floatarg f)
{
    int port = f;

    // 0 is allowed -> don't listen
    if (port < 0){
        pd_error(x, "%s: bad port %d", classname(x), port);
        return;
    }

    if (x->x_node){
        aoo_node_release(x->x_node, (t_pd *)x, x->x_id);
    }

    x->x_node = port ? aoo_node_add(port, (t_pd *)x, x->x_id) : 0;
    x->x_port = port;
}

static void aoo_send_id(t_aoo_send *x, t_floatarg f)
{
    int id = f;

    if (id == x->x_id){
        return;
    }

    if (id < 0){
        pd_error(x, "%s: bad id %d", classname(x), id);
        return;
    }

    if (x->x_node){
        aoo_node_release(x->x_node, (t_pd *)x, x->x_id);
    }

    aoo_source_set_id(x->x_aoo_source, id);

    x->x_node = x->x_port ? aoo_node_add(x->x_port, (t_pd *)x, id) : 0;
    x->x_id = id;
}

static void * aoo_send_new(t_symbol *s, int argc, t_atom *argv)
{
    t_aoo_send *x = (t_aoo_send *)pd_new(aoo_send_class);

    x->x_f = 0;
    x->x_clock = clock_new(x, (t_method)aoo_send_tick);
    x->x_sinks = 0;
    x->x_numsinks = 0;
    x->x_accept = 1;
    x->x_node = 0;
    x->x_blocksize = 0;
    x->x_samplerate = 0;

    aoo_lock_init(&x->x_lock);

    // arg #1: port number
    x->x_port = atom_getfloatarg(0, argc, argv);

    // arg #2: ID
    int id = atom_getfloatarg(1, argc, argv);
    if (id < 0){
        pd_error(x, "%s: bad id % d, setting to 0", classname(x), id);
        id = 0;
    }
    x->x_id = id;

    // arg #3: num channels
    int nchannels = atom_getfloatarg(2, argc, argv);
    if (nchannels < 1){
        nchannels = 1;
    }
    x->x_nchannels = nchannels;

    // make additional inlets
    if (nchannels > 1){
        int i = nchannels;
        while (--i){
            inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
        }
    }
    x->x_vec = (t_sample **)getbytes(sizeof(t_sample *) * nchannels);

    // make event outlet
    x->x_msgout = outlet_new(&x->x_obj, 0);

    // create and initialize aoo_source object
    x->x_aoo_source = aoo_source_new(x->x_id);

    aoo_format_storage fmt;
    aoo_format_makedefault(&fmt, nchannels);
    aoo_source_set_format(x->x_aoo_source, &fmt.header);

    aoo_source_set_buffersize(x->x_aoo_source, DEFBUFSIZE);

    // finally we're ready to receive messages
    aoo_send_port(x, x->x_port);

    return x;
}

static void aoo_send_free(t_aoo_send *x)
{
    // first stop receiving messages
    if (x->x_node){
        aoo_node_release(x->x_node, (t_pd *)x, x->x_id);
    }

    aoo_source_free(x->x_aoo_source);

    aoo_lock_destroy(&x->x_lock);

    freebytes(x->x_vec, sizeof(t_sample *) * x->x_nchannels);
    if (x->x_sinks){
        freebytes(x->x_sinks, x->x_numsinks * sizeof(t_sink));
    }

    clock_free(x->x_clock);
}

void aoo_send_tilde_setup(void)
{
    aoo_send_class = class_new(gensym("aoo_send~"), (t_newmethod)(void *)aoo_send_new,
        (t_method)aoo_send_free, sizeof(t_aoo_send), 0, A_GIMME, A_NULL);
    CLASS_MAINSIGNALIN(aoo_send_class, t_aoo_send, x_f);
    class_addmethod(aoo_send_class, (t_method)aoo_send_dsp,
                    gensym("dsp"), A_CANT, A_NULL);
    class_addmethod(aoo_send_class, (t_method)aoo_send_port,
                    gensym("port"), A_FLOAT, A_NULL);
    class_addmethod(aoo_send_class, (t_method)aoo_send_id,
                    gensym("id"), A_FLOAT, A_NULL);
    class_addmethod(aoo_send_class, (t_method)aoo_send_add,
                    gensym("add"), A_GIMME, A_NULL);
    class_addmethod(aoo_send_class, (t_method)aoo_send_remove,
                    gensym("remove"), A_GIMME, A_NULL);
    class_addmethod(aoo_send_class, (t_method)aoo_send_start,
                    gensym("start"), A_NULL);
    class_addmethod(aoo_send_class, (t_method)aoo_send_stop,
                    gensym("stop"), A_NULL);
    class_addmethod(aoo_send_class, (t_method)aoo_send_accept,
                    gensym("accept"), A_FLOAT, A_NULL);
    class_addmethod(aoo_send_class, (t_method)aoo_send_format,
                    gensym("format"), A_GIMME, A_NULL);
    class_addmethod(aoo_send_class, (t_method)aoo_send_channel,
                    gensym("channel"), A_GIMME, A_NULL);
    class_addmethod(aoo_send_class, (t_method)aoo_send_packetsize,
                    gensym("packetsize"), A_FLOAT, A_NULL);
    class_addmethod(aoo_send_class, (t_method)aoo_send_ping,
                    gensym("ping"), A_FLOAT, A_NULL);
    class_addmethod(aoo_send_class, (t_method)aoo_send_resend,
                    gensym("resend"), A_FLOAT, A_NULL);
    class_addmethod(aoo_send_class, (t_method)aoo_send_redundancy,
                    gensym("redundancy"), A_FLOAT, A_NULL);
    class_addmethod(aoo_send_class, (t_method)aoo_send_timefilter,
                    gensym("timefilter"), A_FLOAT, A_NULL);
    class_addmethod(aoo_send_class, (t_method)aoo_send_listsinks,
                    gensym("list_sinks"), A_NULL);
}
