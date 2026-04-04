/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "aoo_common.h"

#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <inttypes.h>

#define DEFBUFSIZE 25

/*///////////////////// aoo_receive~ ////////////////////*/

t_class *aoo_receive_class;

typedef struct _source
{
    t_endpoint *s_endpoint;
    int32_t s_id;
} t_source;

typedef struct _aoo_receive
{
    t_object x_obj;
    t_float x_f;
    aoo_sink *x_aoo_sink;
    int32_t x_samplerate;
    int32_t x_blocksize;
    int32_t x_nchannels;
    int32_t x_port;
    int32_t x_id;
    t_sample **x_vec;
    // sinks
    t_source *x_sources;
    int x_numsources;
    // server
    t_aoo_node * x_node;
    aoo_lock x_lock;
    // events
    t_outlet *x_msgout;
    t_clock *x_clock;
} t_aoo_receive;

static t_source * aoo_receive_findsource(t_aoo_receive *x, int argc, t_atom *argv)
{
    struct sockaddr_storage sa;
    socklen_t len = 0;
    int32_t id = 0;
    if (aoo_getsourcearg(x, x->x_node, argc, argv, &sa, &len, &id)){
        for (int i = 0; i < x->x_numsources; ++i){
            if (endpoint_match(x->x_sources[i].s_endpoint, &sa) &&
                x->x_sources[i].s_id == id)
            {
                return &x->x_sources[i];
            }
        }
        t_symbol *host = atom_getsymbol(argv);
        int port = atom_getfloat(argv + 1);
        pd_error(x, "%s: couldn't find source %s %d %d",
                 classname(x), host->s_name, port, id);
    }
    return 0;
}

// called from the network receive thread
void aoo_receive_handle_message(t_aoo_receive *x, const char * data,
                                int32_t n, void *endpoint, aoo_replyfn fn)
{
    // synchronize with aoo_receive_dsp()
    aoo_lock_lock_shared(&x->x_lock);
    // handle incoming message
    aoo_sink_handle_message(x->x_aoo_sink, data, n, endpoint, fn);
    aoo_lock_unlock_shared(&x->x_lock);
}

// called from the network send thread
void aoo_receive_send(t_aoo_receive *x)
{
    // synchronize with aoo_receive_dsp()
    aoo_lock_lock_shared(&x->x_lock);
    // send outgoing messages
    while (aoo_sink_send(x->x_aoo_sink)) ;
    aoo_lock_unlock_shared(&x->x_lock);
}

static void aoo_receive_invite(t_aoo_receive *x, t_symbol *s, int argc, t_atom *argv)
{
    if (!x->x_node){
        pd_error(x, "%s: can't invite source - no socket!", classname(x));
        return;
    }

    if (argc < 3){
        pd_error(x, "%s: too few arguments for 'invite' message", classname(x));
        return;
    }

    struct sockaddr_storage sa;
    socklen_t len = 0;
    int32_t id = 0;
    t_endpoint *e = 0;
    if (aoo_getsourcearg(x, x->x_node, argc, argv, &sa, &len, &id)){
        for (int i = 0; i < x->x_numsources; ++i){
            t_source *src = &x->x_sources[i];
            if (src->s_id == id && endpoint_match(src->s_endpoint, &sa)){
                e = src->s_endpoint;
                break;
            }
        }
        if (!e){
            e = aoo_node_endpoint(x->x_node, &sa, len);
        }
        aoo_sink_invite_source(x->x_aoo_sink, e, id, (aoo_replyfn)endpoint_send);
        // notify send thread
        aoo_node_notify(x->x_node);
    }
}

static void aoo_receive_uninvite(t_aoo_receive *x, t_symbol *s, int argc, t_atom *argv)
{
    if (!x->x_node){
        pd_error(x, "%s: can't uninvite source - no socket!", classname(x));
        return;
    }

    if (!argc){
        aoo_sink_uninvite_all(x->x_aoo_sink);
        return;
    }

    if (argc < 3){
        pd_error(x, "%s: too few arguments for 'uninvite' message", classname(x));
        return;
    }

    t_source *src = aoo_receive_findsource(x, argc, argv);
    if (src){
        aoo_sink_uninvite_source(x->x_aoo_sink, src->s_endpoint,
                                src->s_id, (aoo_replyfn)endpoint_send);
        // notify send thread
        aoo_node_notify(x->x_node);
    }
}

static void aoo_receive_buffersize(t_aoo_receive *x, t_floatarg f)
{
    aoo_sink_set_buffersize(x->x_aoo_sink, f);
}

static void aoo_receive_timefilter(t_aoo_receive *x, t_floatarg f)
{
    aoo_sink_set_timefilter_bandwith(x->x_aoo_sink, f);
}

static void aoo_receive_packetsize(t_aoo_receive *x, t_floatarg f)
{
    aoo_sink_set_packetsize(x->x_aoo_sink, f);
}

static void aoo_receive_reset(t_aoo_receive *x, t_symbol *s, int argc, t_atom *argv)
{
    if (argc){
        // reset specific source
        t_source *source = aoo_receive_findsource(x, argc, argv);
        if (source){
            aoo_sink_reset_source(x->x_aoo_sink, source->s_endpoint, source->s_id);
        }
    } else {
        // reset all sources
        aoo_sink_reset(x->x_aoo_sink);
    }
}

static void aoo_receive_resend(t_aoo_receive *x, t_symbol *s, int argc, t_atom *argv)
{
    int32_t limit = 0, interval = 0, maxnumframes = 0;
    if (!aoo_parseresend(x, argc, argv, &limit, &interval, &maxnumframes)){
        return;
    }
    aoo_sink_set_resend_limit(x->x_aoo_sink, limit);
    aoo_sink_set_resend_interval(x->x_aoo_sink, interval);
    aoo_sink_set_resend_maxnumframes(x->x_aoo_sink, maxnumframes);
}

static void aoo_receive_listsources(t_aoo_receive *x)
{
    for (int i = 0; i < x->x_numsources; ++i){
        t_source *s = &x->x_sources[i];
        t_symbol *host = 0;
        int port = 0;
        if (endpoint_getaddress(s->s_endpoint, &host, &port)){
            t_atom msg[3];
            SETSYMBOL(msg, host);
            SETFLOAT(msg + 1, port);
            SETFLOAT(msg + 2, s->s_id);
            outlet_anything(x->x_msgout, gensym("source"), 3, msg);
        } else {
            pd_error(x, "%s: couldn't get endpoint address for source", classname(x));
        }
    }
}

static void aoo_receive_listen(t_aoo_receive *x, t_floatarg f)
{
    int port = f;
    if (x->x_node){
        if (aoo_node_port(x->x_node) == port){
            return;
        }
        // release old node
        aoo_node_release(x->x_node, (t_pd *)x, x->x_id);
    }
    // add new node
    if (port){
        x->x_node = aoo_node_add(port, (t_pd *)x, x->x_id);
        if (x->x_node){
            post("listening on port %d", aoo_node_port(x->x_node));
        }
    } else {
        // stop listening
        x->x_node = 0;
    }
}

static int32_t aoo_receive_handle_events(t_aoo_receive *x, const aoo_event **events, int32_t n)
{
    t_atom msg[32];
    for (int i = 0; i < n; ++i){
        switch (events[i]->type){
        case AOO_SOURCE_ADD_EVENT:
        {
            aoo_source_event *e = (aoo_source_event *)events[i];

            // first add to source list
            int oldsize = x->x_numsources;
            if (oldsize){
                x->x_sources = (t_source *)resizebytes(x->x_sources,
                    oldsize * sizeof(t_source), (oldsize + 1) * sizeof(t_source));
            } else {
                x->x_sources = (t_source *)getbytes(sizeof(t_source));
            }
            t_source *s = &x->x_sources[oldsize];
            s->s_endpoint = (t_endpoint *)e->endpoint;
            s->s_id = e->id;
            x->x_numsources++;

            // output event
            if (!aoo_endpoint_to_atoms(e->endpoint, e->id, msg)){
                continue;
            }
            outlet_anything(x->x_msgout, gensym("source_add"), 3, msg);
            break;
        }
        case AOO_SOURCE_FORMAT_EVENT:
        {
            aoo_source_event *e = (aoo_source_event *)events[i];
            if (!aoo_endpoint_to_atoms(e->endpoint, e->id, msg)){
                continue;
            }
            aoo_format_storage f;
            if (aoo_sink_get_source_format(x->x_aoo_sink, e->endpoint, e->id, &f) > 0) {
                int fsize = aoo_format_toatoms(&f.header, 29, msg + 3); // skip first three atoms
                outlet_anything(x->x_msgout, gensym("source_format"), fsize + 3, msg);
            }
            break;
        }
        case AOO_SOURCE_STATE_EVENT:
        {
            aoo_source_state_event *e = (aoo_source_state_event *)events[i];
            if (!aoo_endpoint_to_atoms(e->endpoint, e->id, msg)){
                continue;
            }
            SETFLOAT(&msg[3], e->state);
            outlet_anything(x->x_msgout, gensym("source_state"), 4, msg);
            break;
        }
        case AOO_BLOCK_LOST_EVENT:
        {
            aoo_block_lost_event *e = (aoo_block_lost_event *)events[i];
            if (!aoo_endpoint_to_atoms(e->endpoint, e->id, msg)){
                continue;
            }
            SETFLOAT(&msg[3], e->count);
            outlet_anything(x->x_msgout, gensym("block_lost"), 4, msg);
            break;
        }
        case AOO_BLOCK_REORDERED_EVENT:
        {
            aoo_block_reordered_event *e = (aoo_block_reordered_event *)events[i];
            if (!aoo_endpoint_to_atoms(e->endpoint, e->id, msg)){
                continue;
            }
            SETFLOAT(&msg[3], e->count);
            outlet_anything(x->x_msgout, gensym("block_reordered"), 4, msg);
            break;
        }
        case AOO_BLOCK_RESENT_EVENT:
        {
            aoo_block_resent_event *e = (aoo_block_resent_event *)events[i];
            if (!aoo_endpoint_to_atoms(e->endpoint, e->id, msg)){
                continue;
            }
            SETFLOAT(&msg[3], e->count);
            outlet_anything(x->x_msgout, gensym("block_resent"), 4, msg);
            break;
        }
        case AOO_BLOCK_GAP_EVENT:
        {
            aoo_block_gap_event *e = (aoo_block_gap_event *)events[i];
            if (!aoo_endpoint_to_atoms(e->endpoint, e->id, msg)){
                continue;
            }
            SETFLOAT(&msg[3], e->count);
            outlet_anything(x->x_msgout, gensym("block_gap"), 4, msg);
            break;
        }
        case AOO_PING_EVENT:
        {
            aoo_ping_event *e = (aoo_ping_event *)events[i];
            if (!aoo_endpoint_to_atoms(e->endpoint, e->id, msg)){
                continue;
            }
            double diff = aoo_osctime_duration(e->tt1, e->tt2) * 1000.0;
            SETFLOAT(msg + 3, diff);
            outlet_anything(x->x_msgout, gensym("ping"), 4, msg);
            break;
        }
        default:
            break;
        }
    }
    return 1;
}

static void aoo_receive_tick(t_aoo_receive *x)
{
    aoo_sink_handle_events(x->x_aoo_sink, (aoo_eventhandler)aoo_receive_handle_events, x);
}

static t_int * aoo_receive_perform(t_int *w)
{
    t_aoo_receive *x = (t_aoo_receive *)(w[1]);
    int n = (int)(w[2]);

    uint64_t t = aoo_osctime_get();
    if (aoo_sink_process(x->x_aoo_sink, x->x_vec, n, t) <= 0){
        // output zeros
        for (int i = 0; i < x->x_nchannels; ++i){
            memset(x->x_vec[i], 0, sizeof(t_float) * n);
        }
    }

    // handle events
    if (aoo_sink_events_available(x->x_aoo_sink) > 0){
        clock_delay(x->x_clock, 0);
    }

    return w + 3;
}

static void aoo_receive_dsp(t_aoo_receive *x, t_signal **sp)
{
    int32_t blocksize = sp[0]->s_n;
    int32_t samplerate = sp[0]->s_sr;

    for (int i = 0; i < x->x_nchannels; ++i){
        x->x_vec[i] = sp[i]->s_vec;
    }

    // synchronize with network threads!
    aoo_lock_lock(&x->x_lock); // writer lock!

    if (blocksize != x->x_blocksize || samplerate != x->x_samplerate){
        aoo_sink_setup(x->x_aoo_sink, samplerate, blocksize, x->x_nchannels);
        x->x_blocksize = blocksize;
        x->x_samplerate = samplerate;
    }

    aoo_lock_unlock(&x->x_lock);

    dsp_add(aoo_receive_perform, 2, (t_int)x, (t_int)x->x_blocksize);
}

static void aoo_receive_port(t_aoo_receive *x, t_floatarg f)
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

static void aoo_receive_id(t_aoo_receive *x, t_floatarg f)
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

    aoo_sink_set_id(x->x_aoo_sink, id);

    x->x_node = x->x_port ? aoo_node_add(x->x_port, (t_pd *)x, id) : 0;
    x->x_id = id;
}

static void * aoo_receive_new(t_symbol *s, int argc, t_atom *argv)
{
    t_aoo_receive *x = (t_aoo_receive *)pd_new(aoo_receive_class);

    x->x_f = 0;
    x->x_node = 0;
    x->x_sources = 0;
    x->x_numsources = 0;
    x->x_blocksize = 0;
    x->x_samplerate = 0;
    x->x_node = 0;
    x->x_clock = clock_new(x, (t_method)aoo_receive_tick);

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

    // arg #4: buffer size (ms)
    int buffersize = argc > 3 ? atom_getfloat(argv + 3) : DEFBUFSIZE;

    // make signal outlets
    for (int i = 0; i < nchannels; ++i){
        outlet_new(&x->x_obj, &s_signal);
    }
    x->x_vec = (t_sample **)getbytes(sizeof(t_sample *) * nchannels);

    // event outlet
    x->x_msgout = outlet_new(&x->x_obj, 0);

    // create and initialize aoo_sink object
    x->x_aoo_sink = aoo_sink_new(x->x_id);

    aoo_receive_buffersize(x, buffersize);

    // finally we're ready to receive messages
    aoo_receive_port(x, x->x_port);

    return x;
}

static void aoo_receive_free(t_aoo_receive *x)
{
    if (x->x_node){
        aoo_node_release(x->x_node, (t_pd *)x, x->x_id);
    }

    aoo_sink_free(x->x_aoo_sink);

    aoo_lock_destroy(&x->x_lock);

    freebytes(x->x_vec, sizeof(t_sample *) * x->x_nchannels);
    if (x->x_sources){
        freebytes(x->x_sources, x->x_numsources * sizeof(t_source));
    }

    clock_free(x->x_clock);
}

void aoo_receive_tilde_setup(void)
{
    aoo_receive_class = class_new(gensym("aoo_receive~"), (t_newmethod)(void *)aoo_receive_new,
        (t_method)aoo_receive_free, sizeof(t_aoo_receive), 0, A_GIMME, A_NULL);
    class_addmethod(aoo_receive_class, (t_method)aoo_receive_dsp,
                    gensym("dsp"), A_CANT, A_NULL);
    class_addmethod(aoo_receive_class, (t_method)aoo_receive_port,
                    gensym("port"), A_FLOAT, A_NULL);
    class_addmethod(aoo_receive_class, (t_method)aoo_receive_id,
                    gensym("id"), A_FLOAT, A_NULL);
    class_addmethod(aoo_receive_class, (t_method)aoo_receive_invite,
                    gensym("invite"), A_GIMME, A_NULL);
    class_addmethod(aoo_receive_class, (t_method)aoo_receive_uninvite,
                    gensym("uninvite"), A_GIMME, A_NULL);
    class_addmethod(aoo_receive_class, (t_method)aoo_receive_buffersize,
                    gensym("bufsize"), A_FLOAT, A_NULL);
    class_addmethod(aoo_receive_class, (t_method)aoo_receive_timefilter,
                    gensym("timefilter"), A_FLOAT, A_NULL);
    class_addmethod(aoo_receive_class, (t_method)aoo_receive_packetsize,
                    gensym("packetsize"), A_FLOAT, A_NULL);
    class_addmethod(aoo_receive_class, (t_method)aoo_receive_resend,
                    gensym("resend"), A_GIMME, A_NULL);
    class_addmethod(aoo_receive_class, (t_method)aoo_receive_listsources,
                    gensym("list_sources"), A_NULL);
    class_addmethod(aoo_receive_class, (t_method)aoo_receive_reset,
                    gensym("reset"), A_GIMME, A_NULL);
}
