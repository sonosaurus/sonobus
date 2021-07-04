/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others. 
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "aoo_common.hpp"

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

using namespace aoo;

static t_class *aoo_pack_class;

struct t_aoo_pack
{
    t_aoo_pack(int argc, t_atom *argv);
    ~t_aoo_pack();

    t_object x_obj;

    t_float x_f;
    aoo::source::pointer x_source;
    ip_address x_address; // fake IP address
    int32_t x_samplerate = 0;
    int32_t x_blocksize = 0;
    int32_t x_nchannels = 0;
    std::unique_ptr<t_float *[]> x_vec;
    t_clock *x_clock = nullptr;
    t_outlet *x_out = nullptr;
    t_outlet *x_msgout = nullptr;
    aoo_id x_sink_id = AOO_ID_NONE;
    int32_t x_sink_chn = 0;
    bool x_accept = false;
};

static int32_t aoo_pack_send(t_aoo_pack *x, const char *data, int32_t n,
                             const void *address, int32_t addrlen, uint32_t flags)
{
    t_atom *a = (t_atom *)alloca(n * sizeof(t_atom));
    for (int i = 0; i < n; ++i){
        SETFLOAT(&a[i], (unsigned char)data[i]);
    }
    outlet_list(x->x_out, &s_list, n, a);
    return 1;
}

static void aoo_pack_handle_event(t_aoo_pack *x, const aoo_event *event, int32_t)
{
    switch (event->type){
    case AOO_PING_EVENT:
    {
        auto e = (const aoo_ping_event *)event;
        double diff1 = aoo_osctime_duration(e->tt1, e->tt2) * 1000.0;
        double diff2 = aoo_osctime_duration(e->tt2, e->tt3) * 1000.0;
        double rtt = aoo_osctime_duration(e->tt1, e->tt3) * 1000.0;

        t_atom msg[5];
        SETFLOAT(msg, e->ep.id);
        SETFLOAT(msg + 1, diff1);
        SETFLOAT(msg + 2, diff2);
        SETFLOAT(msg + 3, rtt);
        SETFLOAT(msg + 4, e->lost_blocks);
        outlet_anything(x->x_msgout, gensym("ping"), 5, msg);
        break;
    }
    case AOO_INVITE_EVENT:
    {
        auto e = (const aoo_sink_event *)event;

        t_atom msg;
        SETFLOAT(&msg, e->ep.id);

        if (x->x_accept){
            x->x_source->add_sink(e->ep);

            outlet_anything(x->x_msgout, gensym("sink_add"), 1, &msg);
        } else {
            outlet_anything(x->x_msgout, gensym("invite"), 1, &msg);
        }
        break;
    }
    case AOO_UNINVITE_EVENT:
    {
        auto e = (const aoo_sink_event *)event;

        t_atom msg;
        SETFLOAT(&msg, e->ep.id);

        if (x->x_accept){
            x->x_source->remove_sink(e->ep);

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

static void aoo_pack_tick(t_aoo_pack *x)
{
    x->x_source->send((aoo_sendfn)aoo_pack_send, x);

    if (x->x_source->events_available()){
        x->x_source->poll_events();
    }
}

static void aoo_pack_list(t_aoo_pack *x, t_symbol *s, int argc, t_atom *argv)
{
    char *msg = (char *)alloca(argc);
    for (int i = 0; i < argc; ++i){
        msg[i] = (int)(argv[i].a_type == A_FLOAT ? argv[i].a_w.w_float : 0.f);
    }
    x->x_source->handle_message(msg, argc, x->x_address.address(), x->x_address.length());
}

static void aoo_pack_format(t_aoo_pack *x, t_symbol *s, int argc, t_atom *argv)
{
    aoo_format_storage f;
    if (format_parse((t_pd *)x, f, argc, argv, x->x_nchannels)){
        // Prevent user from accidentally creating huge number of channels.
        // This also helps to catch an issue with old patches (before 2.0-pre3),
        // which would pass the block size as the channel count, because the
        // "channel" argument hasn't been added yet.
        if (f.header.nchannels > x->x_nchannels){
            pd_error(x, "%s: 'channel' argument (%d) in 'format' message out of range!",
                     classname(x), f.header.nchannels);
            f.header.nchannels = x->x_nchannels;
        }

        x->x_source->set_format(f.header);
        // output actual format
        t_atom msg[16];
        int n = format_to_atoms(f.header, 16, msg);
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
        aoo_endpoint ep { x->x_address.address(),
            (int32_t)x->x_address.length(), x->x_sink_id };
        x->x_source->set_sink_channel_onset(ep, x->x_sink_chn);
    }
}

static void aoo_pack_packetsize(t_aoo_pack *x, t_floatarg f)
{
    x->x_source->set_packetsize(f);
}

static void aoo_pack_ping(t_aoo_pack *x, t_floatarg f)
{
    x->x_source->set_ping_interval(f);
}

static void aoo_pack_resend(t_aoo_pack *x, t_floatarg f)
{
    x->x_source->set_buffersize(f);
}

static void aoo_pack_redundancy(t_aoo_pack *x, t_floatarg f)
{
    x->x_source->set_redundancy(f);
}

static void aoo_pack_dll_bandwidth(t_aoo_pack *x, t_floatarg f)
{
    x->x_source->set_dll_bandwidth(f);
}

static void aoo_pack_set(t_aoo_pack *x, t_symbol *s, int argc, t_atom *argv)
{
    if (argc){
        // remove old sink
        x->x_source->remove_all();
        // add new sink
        aoo_id id = atom_getfloat(argv);
        aoo_endpoint ep { x->x_address.address(),
            (int32_t)x->x_address.length(), id };
        x->x_source->add_sink(ep, 0);
        x->x_sink_id = id;
        // set channel (if provided)
        if (argc > 1){
            int32_t chn = atom_getfloat(argv + 1);
            x->x_sink_chn = chn > 0 ? chn : 0;
            aoo_pack_channel(x, x->x_sink_chn);
        }
    }
}

static void aoo_pack_clear(t_aoo_pack *x)
{
    x->x_source->remove_all();
    x->x_sink_id = AOO_ID_NONE;
}

static void aoo_pack_start(t_aoo_pack *x)
{
    x->x_source->start();
}

static void aoo_pack_stop(t_aoo_pack *x)
{
    x->x_source->stop();
}

static t_int * aoo_pack_perform(t_int *w)
{
    t_aoo_pack *x = (t_aoo_pack *)(w[1]);
    int n = (int)(w[2]);

    assert(sizeof(t_sample) == sizeof(aoo_sample));

    uint64_t t = aoo_osctime_now();
    x->x_source->process((const aoo_sample **)x->x_vec.get(), n, t);

    clock_delay(x->x_clock, 0);

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
        x->x_source->setup(samplerate, blocksize, x->x_nchannels);
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
    void *x = pd_new(aoo_pack_class);
    new (x) t_aoo_pack(argc, argv);
    return x;
}

t_aoo_pack::t_aoo_pack(int argc, t_atom *argv)
{
    x_f = 0;
    x_clock = clock_new(this, (t_method)aoo_pack_tick);
    x_accept = 1;
    x_address = aoo::ip_address(1, 1); // fake IP address

    // arg #1: ID
    int id = atom_getfloatarg(0, argc, argv);

    // arg #2: num channels
    int nchannels = atom_getfloatarg(1, argc, argv);
    if (nchannels < 1){
        nchannels = 1;
    }
    x_nchannels = nchannels;

    // arg #3: sink ID
    if (argc > 2){
        x_sink_id = atom_getfloat(argv + 2);
    } else {
        x_sink_id = AOO_ID_NONE;
    }

    // arg #4: sink channel
    x_sink_chn = atom_getfloatarg(3, argc, argv);

    // make additional inlets
    if (nchannels > 1){
        int i = nchannels;
        while (--i){
            inlet_new(&x_obj, &x_obj.ob_pd, &s_signal, &s_signal);
        }
    }
    x_vec = std::make_unique<t_sample *[]>(nchannels);

    // make outlets
    x_out = outlet_new(&x_obj, 0);
    x_msgout = outlet_new(&x_obj, 0);

    // create and initialize aoo_sink object
    auto src = aoo::source::create(id >= 0 ? id : 0, 0);
    x_source.reset(src);

    x_source->set_eventhandler((aoo_eventhandler)aoo_pack_handle_event,
                               this, AOO_EVENT_POLL);

    // add sink
    if (x_sink_id != AOO_ID_NONE){
        aoo_endpoint ep { x_address.address(),
            (int32_t)x_address.length(), x_sink_id };
        x_source->add_sink(ep);
        // set channel
        x_source->set_sink_channel_onset(ep, x_sink_chn);
    }

    // default format
    aoo_format_storage fmt;
    format_makedefault(fmt, nchannels);
    x_source->set_format(fmt.header);

    // since process() and send() are called from the same thread,
    // we can use the minimal buffer size and thus save some memory.
    x_source->set_buffersize(0);
}

static void aoo_pack_free(t_aoo_pack *x)
{
    x->~t_aoo_pack();
}

t_aoo_pack::~t_aoo_pack()
{
    clock_free(x_clock);
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
    class_addmethod(aoo_pack_class, (t_method)aoo_pack_dll_bandwidth, gensym("dll_bandwidth"), A_FLOAT, A_NULL);
}
