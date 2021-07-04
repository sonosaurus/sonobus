/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others. 
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "aoo_common.hpp"

#include <vector>
#include <string.h>
#include <assert.h>
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

using namespace aoo;

// for hardware buffer sizes up to 1024 @ 44.1 kHz
#define DEFBUFSIZE 25

t_class *aoo_send_class;

struct t_sink
{
    aoo::ip_address s_address;
    int32_t s_id;
    uint32_t s_flags;
};

struct t_aoo_send
{
    t_aoo_send(int argc, t_atom *argv);
    ~t_aoo_send();

    t_object x_obj;

    t_float x_f = 0;
    aoo::source::pointer x_source;
    int32_t x_samplerate = 0;
    int32_t x_blocksize = 0;
    int32_t x_nchannels = 0;
    int32_t x_port = 0;
    aoo_id x_id = 0;
    std::unique_ptr<t_float *[]> x_vec;
    // sinks
    std::vector<t_sink> x_sinks;
    // node
    t_node *x_node = nullptr;
    // events
    t_clock *x_clock = nullptr;
    t_outlet *x_msgout = nullptr;
    bool x_accept = true;
};

static void aoo_send_doaddsink(t_aoo_send *x, const aoo::ip_address& addr,
                               aoo_id id, uint32_t flags)
{
    aoo_endpoint ep { addr.address(), (int32_t)addr.length(), id };
    x->x_source->add_sink(ep, flags);

    // add sink to list
    x->x_sinks.push_back({addr, id});

    // output message
    t_atom msg[3];
    if (x->x_node->resolve_endpoint(addr, id, 3, msg)){
        outlet_anything(x->x_msgout, gensym("sink_add"), 3, msg);
    } else {
        bug("t_node::resolve_endpoint");
    }
}

static void aoo_send_doremoveall(t_aoo_send *x)
{
    x->x_source->remove_all();

    int numsinks = x->x_sinks.size();
    if (!numsinks){
        return;
    }

    // temporary copies (for reentrancy)
    t_sink *sinks = (t_sink *)alloca(sizeof(t_sink) * numsinks);
    std::copy(x->x_sinks.begin(), x->x_sinks.end(), sinks);

    x->x_sinks.clear();

    // output messages
    for (int i = 0; i < numsinks; ++i){
        t_atom msg[3];
        if (x->x_node->resolve_endpoint(sinks[i].s_address, sinks[i].s_id, 3, msg)){
            outlet_anything(x->x_msgout, gensym("sink_remove"), 3, msg);
        } else {
            bug("t_node::resolve_endpoint");
        }
    }
}

static void aoo_send_doremovesink(t_aoo_send *x, const ip_address& addr, aoo_id id)
{
    aoo_endpoint ep { addr.address(), (int32_t)addr.length(), id };
    x->x_source->remove_sink(ep);

    // remove the sink matching endpoint and id
    for (auto it = x->x_sinks.begin(); it != x->x_sinks.end(); ++it){
        if (it->s_address == addr && it->s_id == id){
            x->x_sinks.erase(it);

            // output message
            t_atom msg[3];
            if (x->x_node->resolve_endpoint(addr, id, 3, msg)){
                outlet_anything(x->x_msgout, gensym("sink_remove"), 3, msg);
            } else {
                bug("t_node::resolve_endpoint");
            }
            return;
        }
    }
    bug("aoo_send_doremovesink");
}

static t_sink *aoo_send_findsink(t_aoo_send *x, const ip_address& addr, aoo_id id)
{
    for (auto& sink : x->x_sinks){
        if (sink.s_address == addr && sink.s_id == id){
            return &sink;
        }
    }
    return nullptr;
}

static void aoo_send_setformat(t_aoo_send *x, aoo_format& f)
{
    // Prevent user from accidentally creating huge number of channels.
    // This also helps to catch an issue with old patches (before 2.0-pre3),
    // which would pass the block size as the channel count, because the
    // "channel" argument hasn't been added yet.
    if (f.nchannels > x->x_nchannels){
        pd_error(x, "%s: 'channel' argument (%d) in 'format' message out of range!",
                 classname(x), f.nchannels);
        f.nchannels = x->x_nchannels;
    }

    x->x_source->set_format(f);
    // output actual format
    t_atom msg[16];
    int n = format_to_atoms(f, 16, msg);
    if (n > 0){
        outlet_anything(x->x_msgout, gensym("format"), n, msg);
    }
}

static void aoo_send_handle_event(t_aoo_send *x, const aoo_event *event, int32_t)
{
    switch (event->type){
    case AOO_XRUN_EVENT:
    {
        auto e = (const aoo_xrun_event *)event;

        t_atom msg;
        SETFLOAT(&msg, e->count);
        outlet_anything(x->x_msgout, gensym("xrun"), 1, &msg);
        break;
    }
    case AOO_PING_EVENT:
    {
        auto e = (const aoo_ping_event *)event;
        aoo::ip_address addr((const sockaddr *)e->ep.address, e->ep.addrlen);
        double diff1 = aoo_osctime_duration(e->tt1, e->tt2) * 1000.0;
        double diff2 = aoo_osctime_duration(e->tt2, e->tt3) * 1000.0;
        double rtt = aoo_osctime_duration(e->tt1, e->tt3) * 1000.0;

        t_atom msg[7];
        if (x->x_node->resolve_endpoint(addr, e->ep.id, 3, msg)){
            SETFLOAT(msg + 3, diff1);
            SETFLOAT(msg + 4, diff2);
            SETFLOAT(msg + 5, rtt);
            SETFLOAT(msg + 6, e->lost_blocks);
            outlet_anything(x->x_msgout, gensym("ping"), 7, msg);
        } else {
            bug("t_node::resolve_endpoint");
        }
        break;
    }
    case AOO_FORMAT_REQUEST_EVENT:
    {
        auto e = (const aoo_format_request_event *)event;
        aoo::ip_address addr((const sockaddr *)e->ep.address, e->ep.addrlen);

        if (aoo_send_findsink(x, addr, e->ep.id)){
            if (x->x_accept){
                // make copy, because format will be updated!
                aoo_format_storage f;
                memcpy(&f, e->format, e->format->size);
                aoo_send_setformat(x, f.header);
            } else {
                t_atom msg[32];
                if (x->x_node->resolve_endpoint(addr, e->ep.id, 3, msg)){
                    int n = format_to_atoms(*e->format, 29, msg + 3); // skip first three atoms
                    outlet_anything(x->x_msgout, gensym("format_request"), n + 3, msg);
                } else {
                    bug("t_node::resolve_endpoint");
                }
            }
        }
        break;
    }
    case AOO_INVITE_EVENT:
    {
        auto e = (const aoo_invite_event *)event;
        aoo::ip_address addr((const sockaddr *)e->ep.address, e->ep.addrlen);

        // silently ignore invite events for existing sink,
        // because multiple invite events might get sent in a row.
        if (!aoo_send_findsink(x, addr, e->ep.id)){
            if (x->x_accept){
                aoo_net_peer_info info;
                if (x->x_node->client()->get_peer_info(e->ep.address, e->ep.addrlen, &info) == AOO_OK){
                    aoo_send_doaddsink(x, addr, e->ep.id, info.flags);
                } else {
                    aoo_send_doaddsink(x, addr, e->ep.id, 0);
                }
            } else {
                t_atom msg[3];
                if (x->x_node->resolve_endpoint(addr, e->ep.id, 3, msg)){
                    outlet_anything(x->x_msgout, gensym("invite"), 3, msg);
                } else {
                    bug("t_node::resolve_endpoint");
                }
            }
        }
        break;
    }
    case AOO_UNINVITE_EVENT:
    {
        auto e = (const aoo_uninvite_event *)event;
        aoo::ip_address addr((const sockaddr *)e->ep.address, e->ep.addrlen);

        // ignore uninvite event for non-existing sink, because
        // multiple uninvite events might get sent in a row.
        if (aoo_send_findsink(x, addr, e->ep.id)){
            if (x->x_accept){
                aoo_send_doremovesink(x, addr, e->ep.id);
            } else {
                t_atom msg[3];
                if (x->x_node->resolve_endpoint(addr, e->ep.id, 3, msg)){
                    outlet_anything(x->x_msgout, gensym("uninvite"), 3, msg);
                } else {
                    bug("t_node::resolve_endpoint");
                }
            }
        }
        break;
    }
    default:
        break;
    }
}

static void aoo_send_tick(t_aoo_send *x)
{
    x->x_source->poll_events();
}

static void aoo_send_format(t_aoo_send *x, t_symbol *s, int argc, t_atom *argv)
{
    aoo_format_storage f;
    if (format_parse((t_pd *)x, f, argc, argv, x->x_nchannels)){
        aoo_send_setformat(x, f.header);
    }
}

static void aoo_send_accept(t_aoo_send *x, t_floatarg f)
{
    x->x_accept = f != 0;
}

static void aoo_send_channel(t_aoo_send *x, t_symbol *s, int argc, t_atom *argv)
{
    ip_address addr;
    uint32_t flags;
    aoo_id id;
    if (argc < 4){
        pd_error(x, "%s: too few arguments for 'channel' message", classname(x));
        return;
    }
    if (x->x_node->get_sink_arg((t_pd *)x, argc, argv, addr, flags, id)){
        int32_t chn = atom_getfloat(argv + 3);
    #if 1
        if (!aoo_send_findsink(x, addr, id)){
            pd_error(x, "%s: couldn't find sink!", classname(x));
            return;
        }
    #endif
        aoo_endpoint ep { addr.address(), (int32_t)addr.length(), id };
        x->x_source->set_sink_channel_onset(ep, chn);
    }
}

static void aoo_send_packetsize(t_aoo_send *x, t_floatarg f)
{
    x->x_source->set_packetsize(f);
}

static void aoo_send_ping(t_aoo_send *x, t_floatarg f)
{
    x->x_source->set_ping_interval(f);
}

static void aoo_send_resend(t_aoo_send *x, t_floatarg f)
{
    x->x_source->set_resend_buffersize(f);
}

static void aoo_send_redundancy(t_aoo_send *x, t_floatarg f)
{
    x->x_source->set_redundancy(f);
}

static void aoo_send_dll_bandwidth(t_aoo_send *x, t_floatarg f)
{
    x->x_source->set_dll_bandwidth(f);
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

    ip_address addr;
    uint32_t flags;
    aoo_id id;
    if (x->x_node->get_sink_arg((t_pd *)x, argc, argv, addr, flags, id)){
        // check if sink exists
        if (aoo_send_findsink(x, addr, id)){
            if (argv[1].a_type == A_SYMBOL){
                // group + user
                auto group = atom_getsymbol(argv)->s_name;
                auto user = atom_getsymbol(argv + 1)->s_name;
                pd_error(x, "%s: sink %s|%s %d already added!",
                         classname(x), group, user, id);
            } else {
                // host + port
                auto host = atom_getsymbol(argv)->s_name;
                pd_error(x, "%s: sink %s %d %d already added!",
                         classname(x), host, addr.port(), id);
            }
            return;
        }

        aoo_send_doaddsink(x, addr, id, flags);

        if (argc > 3){
            aoo_endpoint ep { addr.address(), (int32_t)addr.length(), id };
            x->x_source->set_sink_channel_onset(ep, atom_getfloat(argv + 3));
        }

        // print message (use actual IP address)
        verbose(0, "added sink %s %d %d", addr.name(), addr.port(), id);
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

    ip_address addr;
    uint32_t flags;
    aoo_id id;
    if (x->x_node->get_sink_arg((t_pd *)x, argc, argv, addr, flags, id)){
        if (aoo_send_findsink(x, addr, id)){
            aoo_send_doremovesink(x, addr, id);
            verbose(0, "removed sink %s %d %d", addr.name(), addr.port(), id);
        } else {
            if (argv[1].a_type == A_SYMBOL){
                // group + user
                auto group = atom_getsymbol(argv)->s_name;
                auto user = atom_getsymbol(argv + 1)->s_name;
                pd_error(x, "%s: sink %s|%s %d already added!",
                         classname(x), group, user, id);
            } else {
                // host + port
                auto host = atom_getsymbol(argv)->s_name;
                pd_error(x, "%s: sink %s %d %d not found!",
                         classname(x), host, addr.port(), id);
            }
        }
    }
}

static void aoo_send_start(t_aoo_send *x)
{
    x->x_source->start();
}

static void aoo_send_stop(t_aoo_send *x)
{
    x->x_source->stop();
}

static void aoo_send_listsinks(t_aoo_send *x)
{
    for (auto& sink : x->x_sinks){
        t_atom msg[3];
        if (x->x_node->resolve_endpoint(sink.s_address, sink.s_id, 3, msg)){
            outlet_anything(x->x_msgout, gensym("sink"), 3, msg);
        } else {
            bug("t_node::resolve_endpoint");
        }
    }
}

static t_int * aoo_send_perform(t_int *w)
{
    t_aoo_send *x = (t_aoo_send *)(w[1]);
    int n = (int)(w[2]);

    static_assert(sizeof(t_sample) == sizeof(aoo_sample),
                  "aoo_sample size must match t_sample");

    if (x->x_node){
        auto t = aoo::get_osctime();
        auto vec = (const aoo_sample **)x->x_vec.get();

        auto err = x->x_source->process(vec, n, t);
        if (err != AOO_ERROR_IDLE){
            x->x_node->notify();
        }

        if (x->x_source->events_available()){
            clock_delay(x->x_clock, 0);
        }
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

    if (blocksize != x->x_blocksize || samplerate != x->x_samplerate){
        // synchronize with network threads!
        if (x->x_node){
            x->x_node->lock();
        }
        x->x_source->setup(samplerate, blocksize, x->x_nchannels);
        if (x->x_node){
            x->x_node->unlock();
        }
        x->x_blocksize = blocksize;
        x->x_samplerate = samplerate;
    }

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
        x->x_node->release((t_pd *)x, x->x_source.get());
    }

    if (port){
        x->x_node = t_node::get((t_pd *)x, port, x->x_source.get(), x->x_id);
    } else {
        x->x_node = nullptr;
    }

    x->x_port = port;
}

static void aoo_send_id(t_aoo_send *x, t_floatarg f)
{
    aoo_id id = f;

    if (id == x->x_id){
        return;
    }

    if (id < 0){
        pd_error(x, "%s: bad id %d", classname(x), id);
        return;
    }

    if (x->x_node){
        x->x_node->release((t_pd *)x, x->x_source.get());
    }

    x->x_source->set_id(id);

    if (x->x_port){
        x->x_node = t_node::get((t_pd *)x, x->x_port, x->x_source.get(), x->x_id);
    } else {
        x->x_node = nullptr;
    }

    x->x_id = id;
}

static void * aoo_send_new(t_symbol *s, int argc, t_atom *argv)
{
    void *x = pd_new(aoo_send_class);
    new (x) t_aoo_send(argc, argv);
    return x;
}

t_aoo_send::t_aoo_send(int argc, t_atom *argv)
{
    x_clock = clock_new(this, (t_method)aoo_send_tick);

    // arg #1: port number
    x_port = atom_getfloatarg(0, argc, argv);

    // arg #2: ID
    aoo_id id = atom_getfloatarg(1, argc, argv);
    if (id < 0){
        pd_error(this, "%s: bad id % d, setting to 0", classname(this), id);
        id = 0;
    }
    x_id = id;

    // arg #3: num channels
    int nchannels = atom_getfloatarg(2, argc, argv);
    if (nchannels < 1){
        nchannels = 1;
    }
    x_nchannels = nchannels;

    // make additional inlets
    if (nchannels > 1){
        int i = nchannels;
        while (--i){
            inlet_new(&x_obj, &x_obj.ob_pd, &s_signal, &s_signal);
        }
    }
    x_vec = std::make_unique<t_sample *[]>(nchannels);

    // make event outlet
    x_msgout = outlet_new(&x_obj, 0);

    // create and initialize aoo_source object
    auto src = aoo::source::create(x_id, 0);
    x_source.reset(src);

    // set event handler
    x_source->set_eventhandler((aoo_eventhandler)aoo_send_handle_event,
                               this, AOO_EVENT_POLL);

    aoo_format_storage fmt;
    format_makedefault(fmt, nchannels);
    x_source->set_format(fmt.header);

    x_source->set_buffersize(DEFBUFSIZE);

    // finally we're ready to receive messages
    aoo_send_port(this, x_port);
}

static void aoo_send_free(t_aoo_send *x)
{
    x->~t_aoo_send();
}

t_aoo_send::~t_aoo_send()
{
    // first stop receiving messages
    if (x_node){
        x_node->release((t_pd *)this, x_source.get());
    }

    clock_free(x_clock);
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
    class_addmethod(aoo_send_class, (t_method)aoo_send_dll_bandwidth,
                    gensym("dll_bandwidth"), A_FLOAT, A_NULL);
    class_addmethod(aoo_send_class, (t_method)aoo_send_listsinks,
                    gensym("list_sinks"), A_NULL);
}
