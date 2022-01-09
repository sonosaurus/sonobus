/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "aoo_common.hpp"

#include "aoo/aoo_sink.hpp"

#include <vector>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <inttypes.h>

#define DEFBUFSIZE 25

/*///////////////////// aoo_receive~ ////////////////////*/

t_class *aoo_receive_class;

struct t_source
{
    aoo::ip_address s_address;
    int32_t s_id;
};

struct t_aoo_receive
{
    t_aoo_receive(int argc, t_atom *argv);
    ~t_aoo_receive();

    t_object x_obj;

    t_float x_f = 0;
    AooSink::Ptr x_sink;
    int32_t x_samplerate = 0;
    int32_t x_blocksize = 0;
    int32_t x_nchannels = 0;
    int32_t x_port = 0;
    AooId x_id = 0;
    std::unique_ptr<t_sample *[]> x_vec;
    // metadata
    t_symbol *x_metadata_type;
    std::vector<AooByte> x_metadata;
    // sources
    std::vector<t_source> x_sources;
    // node
    t_node * x_node = nullptr;
    // events
    t_outlet *x_msgout = nullptr;
    t_clock *x_clock = nullptr;
};

static t_source * aoo_receive_findsource(t_aoo_receive *x, int argc, t_atom *argv)
{
    aoo::ip_address addr;
    AooId id = 0;
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

    aoo::ip_address addr;
    AooId id = 0;
    if (x->x_node->get_source_arg((t_pd *)x, argc, argv, addr, id)){
        AooEndpoint ep { addr.address(), (AooAddrSize)addr.length(), id };

        if (x->x_metadata_type){
            AooDataView md;
            md.type = x->x_metadata_type->s_name;
            md.data = x->x_metadata.data();
            md.size = x->x_metadata.size();

            x->x_sink->inviteSource(ep, &md);
        } else {
            x->x_sink->inviteSource(ep, nullptr);
        }
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
        x->x_sink->uninviteAll();
        return;
    }

    if (argc < 3){
        pd_error(x, "%s: too few arguments for 'uninvite' message", classname(x));
        return;
    }

    t_source *src = aoo_receive_findsource(x, argc, argv);
    if (src){
        AooEndpoint ep { src->s_address.address(),
            (AooAddrSize)src->s_address.length(), src->s_id };
        x->x_sink->uninviteSource(ep);
        // notify send thread
        x->x_node->notify();
    }
}

static void aoo_receive_metadata(t_aoo_receive *x, t_symbol *s, int argc, t_atom *argv)
{
    if (argc > 0){
        // metadata type
        t_symbol *type;
        if (argv->a_type == A_SYMBOL){
            type = argv->a_w.w_symbol;
        } else {
            pd_error(x, "%s: bad metadata type", classname(x));
        #if 1
            x->x_metadata_type = nullptr;
        #endif
            return;
        }
        // metadata content
        if (argc > 1){
            // set new stream metadata
            auto size = argc - 1;
            x->x_metadata.resize(size);
            for (int i = 0; i < size; ++i){
                x->x_metadata[i] = (AooByte)atom_getfloat(argv + i + 1);
            }
        } else {
            // empty metadata is not allowed
            pd_error(x, "%s: metadata must not be empty", classname(x));
        #if 1
            x->x_metadata_type = nullptr;
        #endif
            return;
        }
        x->x_metadata_type = type;
    } else {
        // clear stream metadata
        x->x_metadata_type = nullptr;
    }
}

static void aoo_receive_buffersize(t_aoo_receive *x, t_floatarg f)
{
    x->x_sink->setBufferSize(f * 0.001);
}

static void aoo_receive_dynamic_resampling(t_aoo_receive *x, t_floatarg f)
{
    x->x_sink->setDynamicResampling(f);
}

static void aoo_receive_dll_bandwidth(t_aoo_receive *x, t_floatarg f)
{
    x->x_sink->setDllBandwidth(f);
}

static void aoo_receive_packetsize(t_aoo_receive *x, t_floatarg f)
{
    x->x_sink->setPacketSize(f);
}

static void aoo_receive_reset(t_aoo_receive *x, t_symbol *s, int argc, t_atom *argv)
{
    if (argc){
        // reset specific source
        t_source *source = aoo_receive_findsource(x, argc, argv);
        if (source){
            AooEndpoint ep { source->s_address.address(),
                (AooAddrSize)source->s_address.length(), source->s_id };
            x->x_sink->resetSource(ep);
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
        AooEndpoint ep { source->s_address.address(),
            (AooAddrSize)source->s_address.length(), source->s_id };
        double ratio = 0;
        x->x_sink->getBufferFillRatio(ep, ratio);

        t_atom msg[4];
        if (x->x_node->resolve_endpoint(source->s_address, source->s_id, 3, msg)){
            SETFLOAT(msg + 3, ratio);
            outlet_anything(x->x_msgout, gensym("fill_ratio"), 4, msg);
        }
    }
}

static void aoo_receive_resend(t_aoo_receive *x, t_floatarg f)
{
    x->x_sink->setResendData(f != 0);
}

static void aoo_receive_resend_limit(t_aoo_receive *x, t_floatarg f)
{
    x->x_sink->setResendLimit(f);
}

static void aoo_receive_resend_interval(t_aoo_receive *x, t_floatarg f)
{
    x->x_sink->setResendInterval(f * 0.001);
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

static void aoo_receive_handle_event(t_aoo_receive *x, const AooEvent *event, int32_t)
{
    switch (event->type){
    case kAooEventXRun:
    {
        t_atom msg;
        auto e = (const AooEventXRun *)event;
        SETFLOAT(&msg, e->count);
        outlet_anything(x->x_msgout, gensym("xrun"), 1, &msg);
        break;
    }
    case kAooEventSourceAdd:
    case kAooEventSourceRemove:
    case kAooEventInviteTimeout:
    case kAooEventUninviteTimeout:
    case kAooEventBufferUnderrun:
    case kAooEventFormatChange:
    case kAooEventStreamStart:
    case kAooEventStreamStop:
    case kAooEventStreamState:
    case kAooEventBlockLost:
    case kAooEventBlockDropped:
    case kAooEventBlockReordered:
    case kAooEventBlockResent:
    case kAooEventPing:
    {
        // common endpoint header
        auto& ep = ((const AooEventEndpoint *)event)->endpoint;
        aoo::ip_address addr((const sockaddr *)ep.address, ep.addrlen);
        t_atom msg[32];
        if (!x->x_node->resolve_endpoint(addr, ep.id, 3, msg)) {
            bug("aoo_receive_handle_event: resolve_endpoint");
            return;
        }
        // event data
        switch (event->type){
        case kAooEventSourceAdd:
        {
            // first add to source list
            x->x_sources.push_back({ addr, ep.id });

            outlet_anything(x->x_msgout, gensym("add"), 3, msg);
            break;
        }
        case kAooEventSourceRemove:
        {
            // first remove from source list
            auto& sources = x->x_sources;
            for (auto it = sources.begin(); it != sources.end(); ++it){
                if ((it->s_address == addr) && (it->s_id == ep.id)){
                    x->x_sources.erase(it);
                    break;
                }
            }
            outlet_anything(x->x_msgout, gensym("remove"), 3, msg);
            break;
        }
        case kAooEventInviteTimeout:
        {
            outlet_anything(x->x_msgout, gensym("invite_timeout"), 3, msg);
            break;
        }
        case kAooEventUninviteTimeout:
        {
            outlet_anything(x->x_msgout, gensym("uninvite_timeout"), 3, msg);
            break;
        }
        //---------------------- source events ------------------------------//
        case kAooEventBufferUnderrun:
        {
            SETSYMBOL(msg + 3, gensym("underrun"));
            outlet_anything(x->x_msgout, gensym("event"), 4, msg);
            break;
        }
        case kAooEventFormatChange:
        {
            auto e = (const AooEventFormatChange *)event;
            SETSYMBOL(msg + 3, gensym("format"));
            int n = format_to_atoms(*e->format, 29, msg + 4); // skip first 4 atoms
            outlet_anything(x->x_msgout, gensym("event"), n + 4, msg);
            break;
        }
        case kAooEventStreamStart:
        {
            auto e = (const AooEventStreamStart *)event;
            SETSYMBOL(msg + 3, gensym("start"));
            if (e->metadata){
                auto count = e->metadata->size + 5;
                t_atom *vec = (t_atom *)alloca(count * sizeof(t_atom));
                // copy endpoint + event name
                memcpy(vec, msg, 4 * sizeof(t_atom));
                // type
                SETSYMBOL(vec + 4, gensym(e->metadata->type));
                // data
                for (int i = 0; i < e->metadata->size; ++i){
                    SETFLOAT(vec + 5 + i, (uint8_t)e->metadata->data[i]);
                }
                outlet_anything(x->x_msgout, gensym("event"), count, vec);
            } else {
                outlet_anything(x->x_msgout, gensym("event"), 4, msg);
            }
            break;
        }
        case kAooEventStreamStop:
        {
            SETSYMBOL(msg + 3, gensym("stop"));
            outlet_anything(x->x_msgout, gensym("event"), 4, msg);
            break;
        }
        case kAooEventStreamState:
        {
            auto e = (const AooEventStreamState *)event;
            SETSYMBOL(msg + 3, gensym("state"));
            SETFLOAT(msg + 4, e->state);
            outlet_anything(x->x_msgout, gensym("event"), 5, msg);
            break;
        }
        case kAooEventBlockLost:
        {
            auto e = (const AooEventBlockLost *)event;
            SETSYMBOL(msg + 3, gensym("block_lost"));
            SETFLOAT(msg + 4, e->count);
            outlet_anything(x->x_msgout, gensym("event"), 5, msg);
            break;
        }
        case kAooEventBlockDropped:
        {
            auto e = (const AooEventBlockDropped *)event;
            SETSYMBOL(msg + 3, gensym("block_dropped"));
            SETFLOAT(msg + 4, e->count);
            outlet_anything(x->x_msgout, gensym("event"), 5, msg);
            break;
        }
        case kAooEventBlockReordered:
        {
            auto e = (const AooEventBlockReordered *)event;
            SETSYMBOL(msg + 3, gensym("block_reordered"));
            SETFLOAT(msg + 4, e->count);
            outlet_anything(x->x_msgout, gensym("event"), 5, msg);
            break;
        }
        case kAooEventBlockResent:
        {
            auto e = (const AooEventBlockResent *)event;
            SETSYMBOL(msg + 3, gensym("block_resent"));
            SETFLOAT(msg + 4, e->count);
            outlet_anything(x->x_msgout, gensym("event"), 5, msg);
            break;
        }
        case kAooEventPing:
        {
            auto e = (const AooEventPing *)event;
            double diff = aoo_ntpTimeDuration(e->t1, e->t2) * 1000.0;
            SETSYMBOL(msg + 3, gensym("ping"));
            SETFLOAT(msg + 4, diff);
            outlet_anything(x->x_msgout, gensym("event"), 5, msg);
            break;
        }
        default:
            bug("aoo_receive_handle_event: bad case label!");
            break;
        }

        break; // !
    }
    default:
        verbose(0, "%s: unknown event type (%d)", classname(x), event->type);
        break;
    }
}

static void aoo_receive_tick(t_aoo_receive *x)
{
    x->x_sink->pollEvents();
}

static t_int * aoo_receive_perform(t_int *w)
{
    t_aoo_receive *x = (t_aoo_receive *)(w[1]);
    int n = (int)(w[2]);

    if (x->x_node){
        auto err = x->x_sink->process(x->x_vec.get(), n, get_osctime());
        if (err != kAooErrorIdle){
            x->x_node->notify();
        }

        // handle events
        if (x->x_sink->eventsAvailable()){
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
    AooId id = f;

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

    x->x_sink->setId(id);

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
    x_metadata_type = nullptr;
    x_metadata.reserve(AOO_STREAM_METADATA_SIZE);

    // arg #1: port number
    x_port = atom_getfloatarg(0, argc, argv);

    // arg #2: ID
    AooId id = atom_getfloatarg(1, argc, argv);
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
    float buffersize = argc > 3 ? atom_getfloat(argv + 3) : DEFBUFSIZE;

    // make signal outlets
    for (int i = 0; i < nchannels; ++i){
        outlet_new(&x_obj, &s_signal);
    }
    x_vec = std::make_unique<t_sample *[]>(nchannels);

    // event outlet
    x_msgout = outlet_new(&x_obj, 0);

    // create and initialize AooSink object
    x_sink = AooSink::create(x_id, 0, nullptr);

    // set event handler
    x_sink->setEventHandler((AooEventHandler)aoo_receive_handle_event,
                             this, kAooEventModePoll);

    x_sink->setBufferSize(buffersize * 0.001);

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
    class_addmethod(aoo_receive_class, (t_method)aoo_receive_invite,
                    gensym("invite"), A_GIMME, A_NULL);
    class_addmethod(aoo_receive_class, (t_method)aoo_receive_uninvite,
                    gensym("uninvite"), A_GIMME, A_NULL);
    class_addmethod(aoo_receive_class, (t_method)aoo_receive_metadata,
                    gensym("metadata"), A_GIMME, A_NULL);
    class_addmethod(aoo_receive_class, (t_method)aoo_receive_buffersize,
                    gensym("bufsize"), A_FLOAT, A_NULL);
    class_addmethod(aoo_receive_class, (t_method)aoo_receive_dynamic_resampling,
                    gensym("dynamic_resampling"), A_FLOAT, A_NULL);
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
