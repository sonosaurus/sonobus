/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "aoo_common.hpp"

#include <vector>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <inttypes.h>

using namespace aoo;

#define DEFBUFSIZE 25

/*///////////////////// aoo_receive~ ////////////////////*/

t_class *aoo_receive_class;

struct t_source
{
    ip_address s_address;
    int32_t s_id;
};

struct t_aoo_receive
{
    t_aoo_receive(int argc, t_atom *argv);
    ~t_aoo_receive();

    t_object x_obj;

    t_float x_f = 0;
    aoo::sink::pointer x_sink;
    int32_t x_samplerate = 0;
    int32_t x_blocksize = 0;
    int32_t x_nchannels = 0;
    int32_t x_port = 0;
    aoo_id x_id = 0;
    std::unique_ptr<t_sample *[]> x_vec;
    // sinks
    std::vector<t_source> x_sources;
    // server
    t_node * x_node = nullptr;
    // events
    t_outlet *x_msgout = nullptr;
    t_clock *x_clock = nullptr;
};

static t_source * aoo_receive_findsource(t_aoo_receive *x, int argc, t_atom *argv)
{
    ip_address addr;
    aoo_id id = 0;
    if (x->x_node->get_source_arg((t_pd *)x, argc, argv, addr, id)){
        for (auto& src : x->x_sources){
            if (src.s_address == addr && src.s_id == id){
                return &src;
            }
        }
        t_symbol *host = atom_getsymbol(argv);
        int port = atom_getfloat(argv + 1);
        pd_error(x, "%s: couldn't find source %s %d %d",
                 classname(x), host->s_name, port, id);
    }
    return 0;
}

static void aoo_receive_format(t_aoo_receive *x, t_symbol *s, int argc, t_atom *argv)
{
    if (!x->x_node){
        pd_error(x, "%s: can't request format - no socket!", classname(x));
        return;
    }

    // host, ip, id, codec ...
    if (argc < 4){
        pd_error(x, "%s: too few arguments for 'format' message", classname(x));
        return;
    }

    ip_address addr;
    aoo_id id = 0;
    if (!x->x_node->get_source_arg((t_pd *)x, argc, argv, addr, id)){
        return;
    }

    aoo_format_storage f;
    if (format_parse((t_pd *)x, f, argc - 3, argv + 3, x->x_nchannels)){
        // don't use more channels than we actually have
        if (f.header.nchannels > x->x_nchannels){
            f.header.nchannels = x->x_nchannels;
        }
        aoo_endpoint ep { addr.address(), (int32_t)addr.length(), id };
        x->x_sink->request_source_format(ep, f.header);
    }
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

    ip_address addr;
    aoo_id id = 0;
    if (x->x_node->get_source_arg((t_pd *)x, argc, argv, addr, id)){
        aoo_endpoint ep { addr.address(), (int32_t)addr.length(), id };
        x->x_sink->invite_source(ep);
        // notify send thread
        x->x_node->notify();
    }
}

static void aoo_receive_uninvite(t_aoo_receive *x, t_symbol *s, int argc, t_atom *argv)
{
    if (!x->x_node){
        pd_error(x, "%s: can't uninvite source - no socket!", classname(x));
        return;
    }

    if (!argc){
        x->x_sink->uninvite_all();
        return;
    }

    if (argc < 3){
        pd_error(x, "%s: too few arguments for 'uninvite' message", classname(x));
        return;
    }

    t_source *src = aoo_receive_findsource(x, argc, argv);
    if (src){
        aoo_endpoint ep { src->s_address.address(),
            (int32_t)src->s_address.length(), src->s_id };
        x->x_sink->uninvite_source(ep);
        // notify send thread
        x->x_node->notify();
    }
}

static void aoo_receive_buffersize(t_aoo_receive *x, t_floatarg f)
{
    x->x_sink->set_buffersize(f);
}

static void aoo_receive_dll_bandwidth(t_aoo_receive *x, t_floatarg f)
{
    x->x_sink->set_dll_bandwidth(f);
}

static void aoo_receive_packetsize(t_aoo_receive *x, t_floatarg f)
{
    x->x_sink->set_packetsize(f);
}

static void aoo_receive_reset(t_aoo_receive *x, t_symbol *s, int argc, t_atom *argv)
{
    if (argc){
        // reset specific source
        t_source *source = aoo_receive_findsource(x, argc, argv);
        if (source){
            aoo_endpoint ep { source->s_address.address(),
                (int32_t)source->s_address.length(), source->s_id };
            x->x_sink->reset_source(ep);
        }
    } else {
        // reset all sources
        x->x_sink->reset();
    }
}

static void aoo_receive_fill_ratio(t_aoo_receive *x, t_symbol *s, int argc, t_atom *argv){
    // reset specific source
    t_source *source = aoo_receive_findsource(x, argc, argv);
    if (source){
        aoo_endpoint ep { source->s_address.address(),
            (int32_t)source->s_address.length(), source->s_id };
        float ratio = 0;
        x->x_sink->get_buffer_fill_ratio(ep, ratio);

        t_atom msg[4];
        if (x->x_node->resolve_endpoint(source->s_address, source->s_id, 3, msg)){
            SETFLOAT(msg + 3, ratio);
            outlet_anything(x->x_msgout, gensym("fill_ratio"), 4, msg);
        }
    }
}

static void aoo_receive_resend(t_aoo_receive *x, t_floatarg f)
{
    x->x_sink->set_resend_data(f != 0);
}

static void aoo_receive_resend_limit(t_aoo_receive *x, t_floatarg f)
{
    x->x_sink->set_resend_limit(f);
}

static void aoo_receive_resend_interval(t_aoo_receive *x, t_floatarg f)
{
    x->x_sink->set_resend_interval(f);
}

static void aoo_receive_listsources(t_aoo_receive *x)
{
    for (auto& src : x->x_sources)
    {
        t_atom msg[3];
        if (address_to_atoms(src.s_address, 3, msg) > 0){
            SETFLOAT(msg + 2, src.s_id);
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
        if (x->x_node->port() == port){
            return;
        }
        // release old node
        x->x_node->release((t_pd *)x, x->x_sink.get());
    }
    // add new node
    if (port){
        x->x_node = t_node::get((t_pd *)x, port, x->x_sink.get(), x->x_id);
        if (x->x_node){
            post("listening on port %d", x->x_node->port());
        }
    } else {
        // stop listening
        x->x_node = 0;
    }
}

static void aoo_receive_handle_event(t_aoo_receive *x, const aoo_event *event, int32_t)
{
    t_atom msg[32];
    switch (event->type){
    case AOO_XRUN_EVENT:
    {
        auto e = (const aoo_xrun_event *)event;
        SETFLOAT(msg, e->count);
        outlet_anything(x->x_msgout, gensym("xrun"), 1, msg);
        break;
    }
    case AOO_SOURCE_ADD_EVENT:
    {
        auto e = (const aoo_source_event *)event;
        aoo::ip_address addr((const sockaddr *)e->ep.address, e->ep.addrlen);

        // first add to source list
        x->x_sources.push_back({ addr, e->ep.id });

        // output event
        if (!x->x_node->resolve_endpoint(addr, e->ep.id, 3, msg)){
            return;
        }
        outlet_anything(x->x_msgout, gensym("source_add"), 3, msg);
        break;
    }
    case AOO_SOURCE_REMOVE_EVENT:
    {
        auto e = (const aoo_source_event *)event;
        aoo::ip_address addr((const sockaddr *)e->ep.address, e->ep.addrlen);

        // first remove from source list
        auto& sources = x->x_sources;
        for (auto it = sources.begin(); it != sources.end(); ++it){
            if ((it->s_address == addr) && (it->s_id == e->ep.id)){
                x->x_sources.erase(it);
                break;
            }
        }

        // output event
        if (!x->x_node->resolve_endpoint(addr, e->ep.id, 3, msg)){
            return;
        }
        outlet_anything(x->x_msgout, gensym("source_remove"), 3, msg);
        break;
    }
    case AOO_INVITE_TIMEOUT_EVENT:
    {
        auto e = (const aoo_source_event *)event;
        aoo::ip_address addr((const sockaddr *)e->ep.address, e->ep.addrlen);

        // output event
        if (!x->x_node->resolve_endpoint(addr, e->ep.id, 3, msg)){
            return;
        }
        outlet_anything(x->x_msgout, gensym("invite_timeout"), 3, msg);
        break;
    }
    case AOO_FORMAT_TIMEOUT_EVENT:
    {
        auto e = (const aoo_source_event *)event;
        aoo::ip_address addr((const sockaddr *)e->ep.address, e->ep.addrlen);

        // output event
        if (!x->x_node->resolve_endpoint(addr, e->ep.id, 3, msg)){
            return;
        }
        outlet_anything(x->x_msgout, gensym("format_timeout"), 3, msg);
        break;
    }
    case AOO_BUFFER_UNDERRUN_EVENT:
    {
        auto e = (const aoo_buffer_underrun_event *)event;
        aoo::ip_address addr((const sockaddr *)e->ep.address, e->ep.addrlen);

        // output event
        if (!x->x_node->resolve_endpoint(addr, e->ep.id, 3, msg)){
            return;
        }
        outlet_anything(x->x_msgout, gensym("underrun"), 3, msg);
        break;
    }
    case AOO_FORMAT_CHANGE_EVENT:
    {
        auto e = (const aoo_format_change_event *)event;
        aoo::ip_address addr((const sockaddr *)e->ep.address, e->ep.addrlen);

        if (!x->x_node->resolve_endpoint(addr, e->ep.id, 3, msg)){
            return;
        }
        int n = format_to_atoms(*e->format, 29, msg + 3); // skip first three atoms
        outlet_anything(x->x_msgout, gensym("source_format"), n + 3, msg);
        break;
    }
    case AOO_STREAM_STATE_EVENT:
    {
        auto e = (const aoo_stream_state_event *)event;
        aoo::ip_address addr((const sockaddr *)e->ep.address, e->ep.addrlen);

        if (!x->x_node->resolve_endpoint(addr, e->ep.id, 3, msg)){
            return;
        }
        SETFLOAT(&msg[3], e->state);
        outlet_anything(x->x_msgout, gensym("source_state"), 4, msg);
        break;
    }
    case AOO_BLOCK_LOST_EVENT:
    {
        auto e = (const aoo_block_lost_event *)event;
        aoo::ip_address addr((const sockaddr *)e->ep.address, e->ep.addrlen);

        if (!x->x_node->resolve_endpoint(addr, e->ep.id, 3, msg)){
            return;
        }
        SETFLOAT(&msg[3], e->count);
        outlet_anything(x->x_msgout, gensym("block_lost"), 4, msg);
        break;
    }
    case AOO_BLOCK_REORDERED_EVENT:
    {
        auto e = (const aoo_block_reordered_event *)event;
        aoo::ip_address addr((const sockaddr *)e->ep.address, e->ep.addrlen);

        if (!x->x_node->resolve_endpoint(addr, e->ep.id, 3, msg)){
            return;
        }
        SETFLOAT(&msg[3], e->count);
        outlet_anything(x->x_msgout, gensym("block_reordered"), 4, msg);
        break;
    }
    case AOO_BLOCK_RESENT_EVENT:
    {
        auto e = (const aoo_block_resent_event *)event;
        aoo::ip_address addr((const sockaddr *)e->ep.address, e->ep.addrlen);

        if (!x->x_node->resolve_endpoint(addr, e->ep.id, 3, msg)){
            return;
        }
        SETFLOAT(&msg[3], e->count);
        outlet_anything(x->x_msgout, gensym("block_resent"), 4, msg);
        break;
    }
    case AOO_BLOCK_DROPPED_EVENT:
    {
        auto e = (const aoo_block_gap_event *)event;
        aoo::ip_address addr((const sockaddr *)e->ep.address, e->ep.addrlen);

        if (!x->x_node->resolve_endpoint(addr, e->ep.id, 3, msg)){
            return;
        }
        SETFLOAT(&msg[3], e->count);
        outlet_anything(x->x_msgout, gensym("block_dropped"), 4, msg);
        break;
    }
    case AOO_BLOCK_GAP_EVENT:
    {
        auto e = (const aoo_block_gap_event *)event;
        aoo::ip_address addr((const sockaddr *)e->ep.address, e->ep.addrlen);

        if (!x->x_node->resolve_endpoint(addr, e->ep.id, 3, msg)){
            return;
        }
        SETFLOAT(&msg[3], e->count);
        outlet_anything(x->x_msgout, gensym("block_gap"), 4, msg);
        break;
    }
    case AOO_PING_EVENT:
    {
        auto e = (const aoo_ping_event *)event;
        aoo::ip_address addr((const sockaddr *)e->ep.address, e->ep.addrlen);

        if (!x->x_node->resolve_endpoint(addr, e->ep.id, 3, msg)){
            return;
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

static void aoo_receive_tick(t_aoo_receive *x)
{
    x->x_sink->poll_events();
}

static t_int * aoo_receive_perform(t_int *w)
{
    t_aoo_receive *x = (t_aoo_receive *)(w[1]);
    int n = (int)(w[2]);

    if (x->x_node){
        auto t = aoo::get_osctime();
        auto vec = x->x_vec.get();

        auto err = x->x_sink->process(vec, n, t);
        if (err != AOO_ERROR_IDLE){
            x->x_node->notify();
        }

        // handle events
        if (x->x_sink->events_available()){
            clock_delay(x->x_clock, 0);
        }
    } else {
        // zero outputs
        for (int i = 0; i < x->x_nchannels; ++i){
            std::fill(x->x_vec[i], x->x_vec[i] + n, 0);
        }
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
    if (blocksize != x->x_blocksize || samplerate != x->x_samplerate){
        // synchronize with network threads!
        if (x->x_node){
            x->x_node->lock();
        }
        x->x_sink->setup(samplerate, blocksize, x->x_nchannels);
        if (x->x_node){
            x->x_node->unlock();
        }
        x->x_blocksize = blocksize;
        x->x_samplerate = samplerate;
    }

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
        x->x_node->release((t_pd *)x, x->x_sink.get());
    }

    if (port){
        x->x_node = t_node::get((t_pd *)x, port, x->x_sink.get(), x->x_id);
    } else {
        x->x_node = 0;
    }

    x->x_port = port;
}

static void aoo_receive_id(t_aoo_receive *x, t_floatarg f)
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
        x->x_node->release((t_pd *)x, x->x_sink.get());
    }

    x->x_sink->set_id(id);

    if (x->x_port){
        x->x_node = t_node::get((t_pd *)x, x->x_port, x->x_sink.get(), id);
    } else {
        x->x_node = nullptr;
    }

    x->x_id = id;
}

static void * aoo_receive_new(t_symbol *s, int argc, t_atom *argv)
{
    void *x = pd_new(aoo_receive_class);
    new (x) t_aoo_receive(argc, argv);
    return x;
}

t_aoo_receive::t_aoo_receive(int argc, t_atom *argv)
{
    x_clock = clock_new(this, (t_method)aoo_receive_tick);

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

    // arg #4: buffer size (ms)
    int buffersize = argc > 3 ? atom_getfloat(argv + 3) : DEFBUFSIZE;

    // make signal outlets
    for (int i = 0; i < nchannels; ++i){
        outlet_new(&x_obj, &s_signal);
    }
    x_vec = std::make_unique<t_sample *[]>(nchannels);

    // event outlet
    x_msgout = outlet_new(&x_obj, 0);

    // create and initialize aoo_sink object
    auto sink = aoo::sink::create(x_id, 0);
    x_sink.reset(sink);

    // set event handler
    x_sink->set_eventhandler((aoo_eventhandler)aoo_receive_handle_event,
                             this, AOO_EVENT_POLL);

    x_sink->set_buffersize(buffersize);

    // finally we're ready to receive messages
    aoo_receive_port(this, x_port);
}

static void aoo_receive_free(t_aoo_receive *x)
{
    x->~t_aoo_receive();
}

t_aoo_receive::~t_aoo_receive()
{
    if (x_node){
        x_node->release((t_pd *)this, x_sink.get());
    }

    clock_free(x_clock);
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
    class_addmethod(aoo_receive_class, (t_method)aoo_receive_format,
                    gensym("format"), A_GIMME, A_NULL);
    class_addmethod(aoo_receive_class, (t_method)aoo_receive_invite,
                    gensym("invite"), A_GIMME, A_NULL);
    class_addmethod(aoo_receive_class, (t_method)aoo_receive_uninvite,
                    gensym("uninvite"), A_GIMME, A_NULL);
    class_addmethod(aoo_receive_class, (t_method)aoo_receive_buffersize,
                    gensym("bufsize"), A_FLOAT, A_NULL);
    class_addmethod(aoo_receive_class, (t_method)aoo_receive_dll_bandwidth,
                    gensym("dll_bandwidth"), A_FLOAT, A_NULL);
    class_addmethod(aoo_receive_class, (t_method)aoo_receive_packetsize,
                    gensym("packetsize"), A_FLOAT, A_NULL);
    class_addmethod(aoo_receive_class, (t_method)aoo_receive_resend,
                    gensym("resend"), A_FLOAT, A_NULL);
    class_addmethod(aoo_receive_class, (t_method)aoo_receive_resend_limit,
                    gensym("resend_limit"), A_FLOAT, A_NULL);
    class_addmethod(aoo_receive_class, (t_method)aoo_receive_resend_interval,
                    gensym("resend_interval"), A_FLOAT, A_NULL);
    class_addmethod(aoo_receive_class, (t_method)aoo_receive_listsources,
                    gensym("list_sources"), A_NULL);
    class_addmethod(aoo_receive_class, (t_method)aoo_receive_reset,
                    gensym("reset"), A_GIMME, A_NULL);
    class_addmethod(aoo_receive_class, (t_method)aoo_receive_fill_ratio,
                    gensym("fill_ratio"), A_GIMME, A_NULL);
}
