/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others. 
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "aoo_common.h"

#include <string.h>
#include <assert.h>
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

static t_class *aoo_pack_class;

typedef struct _aoo_pack
{
    t_object x_obj;
    t_float x_f;
    aoo_source *x_aoo_source;
    int32_t x_samplerate;
    int32_t x_blocksize;
    int32_t x_nchannels;
    t_float **x_vec;
    t_clock *x_clock;
    t_outlet *x_out;
    t_outlet *x_msgout;
    int32_t x_sink_id;
    int32_t x_sink_chn;
    int x_accept;
} t_aoo_pack;

static int32_t aoo_pack_reply(t_aoo_pack *x, const char *data, int32_t n)
{
    t_atom *a = (t_atom *)alloca(n * sizeof(t_atom));
    for (int i = 0; i < n; ++i){
        SETFLOAT(&a[i], (unsigned char)data[i]);
    }
    outlet_list(x->x_out, &s_list, n, a);
    return 1;
}

static int32_t aoo_pack_handle_events(t_aoo_pack *x, const aoo_event ** events, int32_t n)
{
    for (int i = 0; i < n; ++i){
        switch (events[i]->type){
        case AOO_PING_EVENT:
        {
            aoo_ping_event *e = (aoo_ping_event *)events[i];
            double diff1 = aoo_osctime_duration(e->tt1, e->tt2) * 1000.0;
            double diff2 = aoo_osctime_duration(e->tt2, e->tt3) * 1000.0;
            double rtt = aoo_osctime_duration(e->tt1, e->tt3) * 1000.0;

            t_atom msg[5];
            SETFLOAT(msg, e->id);
            SETFLOAT(msg + 1, diff1);
            SETFLOAT(msg + 2, diff2);
            SETFLOAT(msg + 3, rtt);
            SETFLOAT(msg + 4, e->lost_blocks);
            outlet_anything(x->x_msgout, gensym("ping"), 5, msg);
            break;
        }
        case AOO_INVITE_EVENT:
        {
            aoo_sink_event *e = (aoo_sink_event *)events[i];

            t_atom msg;
            SETFLOAT(&msg, e->id);

            if (x->x_accept){
                aoo_source_add_sink(x->x_aoo_source, x,
                                    e->id, (aoo_replyfn)aoo_pack_reply);

                outlet_anything(x->x_msgout, gensym("sink_add"), 1, &msg);
            } else {
                outlet_anything(x->x_msgout, gensym("invite"), 1, &msg);
            }
            break;
        }
        case AOO_UNINVITE_EVENT:
        {
            aoo_sink_event *e = (aoo_sink_event *)events[i];

            t_atom msg;
            SETFLOAT(&msg, e->id);

            if (x->x_accept){
                aoo_source_remove_sink(x->x_aoo_source, x, e->id);

                outlet_anything(x->x_msgout, gensym("sink_remove"), 1, &msg);
            } else {
                outlet_anything(x->x_msgout, gensym("uninvite"), 1, &msg);
            }
            break;
        }
        default:
            break;
        }
    }
    return 1;
}

static void aoo_pack_tick(t_aoo_pack *x)
{
    while (aoo_source_send(x->x_aoo_source)) ;

    aoo_source_handle_events(x->x_aoo_source, (aoo_eventhandler)aoo_pack_handle_events, x);
}

static void aoo_pack_list(t_aoo_pack *x, t_symbol *s, int argc, t_atom *argv)
{
    char *msg = (char *)alloca(argc);
    for (int i = 0; i < argc; ++i){
        msg[i] = (int)(argv[i].a_type == A_FLOAT ? argv[i].a_w.w_float : 0.f);
    }
    aoo_source_handle_message(x->x_aoo_source, msg, argc, x, (aoo_replyfn)aoo_pack_reply);
}

static void aoo_pack_format(t_aoo_pack *x, t_symbol *s, int argc, t_atom *argv)
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

static void aoo_pack_accept(t_aoo_pack *x, t_floatarg f)
{
    x->x_accept = f != 0;
}

static void aoo_pack_channel(t_aoo_pack *x, t_floatarg f)
{
    x->x_sink_chn = f > 0 ? f : 0;
    if (x->x_sink_id != AOO_ID_NONE){
        aoo_source_set_sink_channelonset(x->x_aoo_source, x, x->x_sink_id, x->x_sink_chn);
    }
}

static void aoo_pack_packetsize(t_aoo_pack *x, t_floatarg f)
{
    aoo_source_set_packetsize(x->x_aoo_source, f);
}

static void aoo_pack_ping(t_aoo_pack *x, t_floatarg f)
{
    aoo_source_set_ping_interval(x->x_aoo_source, f);
}

static void aoo_pack_resend(t_aoo_pack *x, t_floatarg f)
{
    aoo_source_set_buffersize(x->x_aoo_source, f);
}

static void aoo_pack_redundancy(t_aoo_pack *x, t_floatarg f)
{
    aoo_source_set_redundancy(x->x_aoo_source, f);
}

static void aoo_pack_timefilter(t_aoo_pack *x, t_floatarg f)
{
    aoo_source_set_timefilter_bandwith(x->x_aoo_source, f);
}

static void aoo_pack_set(t_aoo_pack *x, t_symbol *s, int argc, t_atom *argv)
{
    if (argc){
        // remove old sink
        aoo_source_remove_all(x->x_aoo_source);
        // add new sink
        if (argv->a_type == A_SYMBOL){
            if (*argv->a_w.w_symbol->s_name == '*'){
                aoo_source_add_sink(x->x_aoo_source, x, AOO_ID_WILDCARD, (aoo_replyfn)aoo_pack_reply);
            } else {
                pd_error(x, "%s: bad argument '%s' to 'set' message!",
                         classname(x), argv->a_w.w_symbol->s_name);
                return;
            }
            x->x_sink_id = AOO_ID_WILDCARD;
        } else {
            int32_t id = atom_getfloat(argv);
            aoo_source_add_sink(x->x_aoo_source, x, id, (aoo_replyfn)aoo_pack_reply);
            x->x_sink_id = id;
        }
        // set channel (if provided)
        if (argc > 1){
            int32_t chn = atom_getfloat(argv + 1);
            x->x_sink_chn = chn > 0 ? chn : 0;
        }
        aoo_pack_channel(x, x->x_sink_chn);
    }
}

static void aoo_pack_clear(t_aoo_pack *x)
{
    aoo_source_remove_all(x->x_aoo_source);
    x->x_sink_id = AOO_ID_NONE;
}

static void aoo_pack_start(t_aoo_pack *x)
{
    aoo_source_start(x->x_aoo_source);
}

static void aoo_pack_stop(t_aoo_pack *x)
{
    aoo_source_stop(x->x_aoo_source);
}

static t_int * aoo_pack_perform(t_int *w)
{
    t_aoo_pack *x = (t_aoo_pack *)(w[1]);
    int n = (int)(w[2]);

    assert(sizeof(t_sample) == sizeof(aoo_sample));

    uint64_t t = aoo_osctime_get();
    if (aoo_source_process(x->x_aoo_source,(const aoo_sample **)x->x_vec, n, t) > 0){
        clock_set(x->x_clock, 0);
    }
    return w + 3;
}

static void aoo_pack_dsp(t_aoo_pack *x, t_signal **sp)
{
    int32_t blocksize = sp[0]->s_n;
    int32_t samplerate = sp[0]->s_sr;

    for (int i = 0; i < x->x_nchannels; ++i){
        x->x_vec[i] = sp[i]->s_vec;
    }

    if (blocksize != x->x_blocksize || samplerate != x->x_samplerate){
        aoo_source_setup(x->x_aoo_source, samplerate, blocksize, x->x_nchannels);
        x->x_blocksize = blocksize;
        x->x_samplerate = samplerate;
    }

    dsp_add(aoo_pack_perform, 2, (t_int)x, (t_int)x->x_blocksize);

    clock_unset(x->x_clock);
}

static void aoo_pack_loadbang(t_aoo_pack *x, t_floatarg f)
{
    // LB_LOAD
    if (f == 0){
        if (x->x_sink_id != AOO_ID_NONE){
            // set sink ID
            t_atom a;
            SETFLOAT(&a, x->x_sink_id);
            aoo_pack_set(x, 0, 1, &a);
            aoo_pack_channel(x, x->x_sink_chn);
        }
    }
}

static void * aoo_pack_new(t_symbol *s, int argc, t_atom *argv)
{
    t_aoo_pack *x = (t_aoo_pack *)pd_new(aoo_pack_class);

    x->x_f = 0;
    x->x_clock = clock_new(x, (t_method)aoo_pack_tick);
    x->x_accept = 1;

    // arg #1: ID
    int src = atom_getfloatarg(0, argc, argv);
    x->x_aoo_source = aoo_source_new(src >= 0 ? src : 0);

    // since process() and send() are called from the same thread,
    // we can use the minimal buffer size and thus safe some memory.
    int32_t bufsize = 0;
    aoo_source_set_buffersize(x->x_aoo_source, bufsize);

    // arg #2: num channels
    int nchannels = atom_getfloatarg(1, argc, argv);
    if (nchannels < 1){
        nchannels = 1;
    }
    x->x_nchannels = nchannels;
    x->x_samplerate = 0;
    x->x_blocksize = 0;

    // arg #3: sink ID
    if (argc > 2){
        x->x_sink_id = atom_getfloat(argv + 2);
    } else {
        x->x_sink_id = AOO_ID_NONE;
    }

    // arg #4: sink channel
    x->x_sink_chn = atom_getfloatarg(3, argc, argv);

    // make additional inlets
    if (nchannels > 1){
        int i = nchannels;
        while (--i){
            inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
        }
    }
    x->x_vec = (t_sample **)getbytes(sizeof(t_sample *) * nchannels);
    // make outlets
    x->x_out = outlet_new(&x->x_obj, 0);
    x->x_msgout = outlet_new(&x->x_obj, 0);

    // default format
    aoo_format_storage fmt;
    aoo_format_makedefault(&fmt, nchannels);
    aoo_source_set_format(x->x_aoo_source, &fmt.header);

    return x;
}

static void aoo_pack_free(t_aoo_pack *x)
{
    // clean up
    freebytes(x->x_vec, sizeof(t_sample *) * x->x_nchannels);
    clock_free(x->x_clock);
    aoo_source_free(x->x_aoo_source);
}

void aoo_pack_tilde_setup(void)
{
    aoo_pack_class = class_new(gensym("aoo_pack~"), (t_newmethod)(void *)aoo_pack_new,
        (t_method)aoo_pack_free, sizeof(t_aoo_pack), 0, A_GIMME, A_NULL);
    CLASS_MAINSIGNALIN(aoo_pack_class, t_aoo_pack, x_f);
    class_addmethod(aoo_pack_class, (t_method)aoo_pack_dsp, gensym("dsp"), A_CANT, A_NULL);
    class_addmethod(aoo_pack_class, (t_method)aoo_pack_loadbang, gensym("loadbang"), A_FLOAT, A_NULL);
    class_addlist(aoo_pack_class, (t_method)aoo_pack_list);
    class_addmethod(aoo_pack_class, (t_method)aoo_pack_set, gensym("set"), A_GIMME, A_NULL);
    class_addmethod(aoo_pack_class, (t_method)aoo_pack_clear, gensym("clear"), A_NULL);
    class_addmethod(aoo_pack_class, (t_method)aoo_pack_start, gensym("start"), A_NULL);
    class_addmethod(aoo_pack_class, (t_method)aoo_pack_stop, gensym("stop"), A_NULL);
    class_addmethod(aoo_pack_class, (t_method)aoo_pack_accept, gensym("accept"), A_FLOAT, A_NULL);
    class_addmethod(aoo_pack_class, (t_method)aoo_pack_format, gensym("format"), A_GIMME, A_NULL);
    class_addmethod(aoo_pack_class, (t_method)aoo_pack_channel, gensym("channel"), A_FLOAT, A_NULL);
    class_addmethod(aoo_pack_class, (t_method)aoo_pack_packetsize, gensym("packetsize"), A_FLOAT, A_NULL);
    class_addmethod(aoo_pack_class, (t_method)aoo_pack_ping, gensym("ping"), A_FLOAT, A_NULL);
    class_addmethod(aoo_pack_class, (t_method)aoo_pack_resend, gensym("resend"), A_FLOAT, A_NULL);
    class_addmethod(aoo_pack_class, (t_method)aoo_pack_redundancy, gensym("redundancy"), A_FLOAT, A_NULL);
    class_addmethod(aoo_pack_class, (t_method)aoo_pack_timefilter, gensym("timefilter"), A_FLOAT, A_NULL);
}
