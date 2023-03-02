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

#define DEFAULT_LATENCY 25
// offset for "fake" stream messages
#define STREAM_MESSAGE_OFFSET 64

/*///////////////////// aoo_receive~ ////////////////////*/

t_class *aoo_receive_class;

struct t_source
{
    aoo::ip_address s_address;
    AooId s_id;
    t_symbol *s_group;
    t_symbol *s_user;
};

struct t_stream_message
{
    t_stream_message(const AooStreamMessage& msg, const AooEndpoint& ep)
        : address((const sockaddr *)ep.address, ep.addrlen), id(ep.id),
          type(msg.type), data(msg.data, msg.data + msg.size) {}
    aoo::ip_address address;
    AooId id;
    AooDataType type;
    std::vector<AooByte> data;
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
    AooDataType x_metadata_type;
    std::vector<AooByte> x_metadata;
    // sources
    std::vector<t_source> x_sources;
    // node
    t_node *x_node = nullptr;
    // events
    t_outlet *x_msgout = nullptr;
    t_clock *x_clock = nullptr;
    t_clock *x_queue_clock = nullptr;

    t_priority_queue<t_stream_message> x_queue;

    bool get_source_arg(int argc, t_atom *argv,
                        aoo::ip_address& addr, AooId& id, bool check) const;

    bool check(const char *name) const;

    bool check(int argc, t_atom *argv, int minargs, const char *name) const;

    void dispatch_stream_message(const AooStreamMessage& msg, const aoo::ip_address& address, AooId id);
};

bool t_aoo_receive::get_source_arg(int argc, t_atom *argv,
                                   aoo::ip_address& addr, AooId& id, bool check) const
{
    if (!x_node) {
        pd_error(this, "%s: no socket!", classname(this));
        return false;
    }
    if (argc < 3){
        pd_error(this, "%s: too few arguments for source", classname(this));
        return false;
    }
    // first try peer (group|user)
    if (argv[1].a_type == A_SYMBOL) {
        t_symbol *group = atom_getsymbol(argv);
        t_symbol *user = atom_getsymbol(argv + 1);
        id = atom_getfloat(argv + 2);
        // first search source list, in case the client has been disconnected
        for (auto& s : x_sources) {
            if (s.s_group == group && s.s_user == user && s.s_id == id) {
                addr = s.s_address;
                return true;
            }
        }
        if (!check) {
            // not yet in list -> try to get from client
            if (x_node->find_peer(group, user, addr)) {
                return true;
            }
        }
        pd_error(this, "%s: couldn't find source %s|%s %d",
                 classname(this), group->s_name, user->s_name, id);
        return false;
    } else {
        // otherwise try host|port
        t_symbol *host = atom_getsymbol(argv);
        int port = atom_getfloat(argv + 1);
        id = atom_getfloat(argv + 2);
        if (x_node->resolve(host, port, addr)) {
            if (check) {
                // try to find in list
                for (auto& s : x_sources) {
                    if (s.s_address == addr && s.s_id == id) {
                        return true;
                    }
                }
                pd_error(this, "%s: couldn't find source %s %d %d",
                         classname(this), host->s_name, port, id);
                return false;
            } else {
                return true;
            }
        } else {
            pd_error(this, "%s: couldn't resolve source hostname '%s'",
                     classname(this), host->s_name);
            return false;
        }
    }
}

bool t_aoo_receive::check(const char *name) const
{
    if (x_node){
        return true;
    } else {
        pd_error(this, "%s: '%s' failed: no socket!", classname(this), name);
        return false;
    }
}

bool t_aoo_receive::check(int argc, t_atom *argv, int minargs, const char *name) const
{
    if (!check(name)) return false;

    if (argc < minargs){
        pd_error(this, "%s: too few arguments for '%s' message", classname(this), name);
        return false;
    }

    return true;
}


static void aoo_receive_invite(t_aoo_receive *x, t_symbol *s, int argc, t_atom *argv)
{
    if (!x->check(argc, argv, 3, "invite")) return;

    aoo::ip_address addr;
    AooId id = 0;
    if (x->get_source_arg(argc, argv, addr, id, false)) {
        AooEndpoint ep { addr.address(), (AooAddrSize)addr.length(), id };

        if (!x->x_metadata.empty()){
            AooData md;
            md.type = x->x_metadata_type;
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
    if (!x->check("uninvite")) return;

    if (!argc){
        x->x_sink->uninviteAll();
        return;
    }

    if (argc < 3){
        pd_error(x, "%s: too few arguments for 'uninvite' message", classname(x));
        return;
    }

    aoo::ip_address addr;
    AooId id = 0;
    if (x->get_source_arg(argc, argv, addr, id, true)){
        AooEndpoint ep { addr.address(), (AooAddrSize)addr.length(), id };
        x->x_sink->uninviteSource(ep);
        // notify send thread
        x->x_node->notify();
    }
}

static void aoo_receive_metadata(t_aoo_receive *x, t_symbol *s, int argc, t_atom *argv)
{
    if (!argc){
        return;
    }
    if (argc < 2) {
        // empty metadata is not allowed
        pd_error(x, "%s: metadata must not be empty", classname(x));
    #if 1
        x->x_metadata_type = kAooDataUnspecified;
        x->x_metadata.clear();
    #endif
        return;
    }
    // metadata type
    AooDataType type;
    if (!atom_to_datatype(*argv, type, x)) {
    #if 1
        x->x_metadata_type = kAooDataUnspecified;
        x->x_metadata.clear();
    #endif
        return;
    }
    x->x_metadata_type = type;
    // metadata content
    auto size = argc - 1;
    x->x_metadata.resize(size);
    for (int i = 0; i < size; ++i){
        x->x_metadata[i] = (AooByte)atom_getfloat(argv + i + 1);
    }
}

static void aoo_receive_latency(t_aoo_receive *x, t_floatarg f)
{
    x->x_sink->setLatency(f * 0.001);
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
        aoo::ip_address addr;
        AooId id = 0;
        if (x->get_source_arg(argc, argv, addr, id, true)) {
            AooEndpoint ep { addr.address(), (AooAddrSize)addr.length(), id };
            x->x_sink->resetSource(ep);
        }
    } else {
        // reset all sources
        x->x_sink->reset();
    }
}

static void aoo_receive_fill_ratio(t_aoo_receive *x, t_symbol *s, int argc, t_atom *argv){
    aoo::ip_address addr;
    AooId id = 0;
    if (x->get_source_arg(argc, argv, addr, id, true)) {
        AooEndpoint ep { addr.address(), (AooAddrSize)addr.length(), id };
        double ratio = 0;
        x->x_sink->getBufferFillRatio(ep, ratio);

        t_atom msg[4];
        if (x->x_node->serialize_endpoint(addr, id, 3, msg)){
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

static void aoo_receive_source_list(t_aoo_receive *x)
{
    if (!x->check("source_list")) return;

    for (auto& src : x->x_sources)
    {
        t_atom msg[3];
        if (x->x_node->serialize_endpoint(src.s_address, src.s_id, 3, msg)) {
            outlet_anything(x->x_msgout, gensym("source"), 3, msg);
        } else {
            bug("aoo_receive_source_list: serialize_endpoint");
        }
    }
}

static void aoo_receive_handle_stream_message(t_aoo_receive *x, const AooStreamMessage *msg, const AooEndpoint *ep);

static void aoo_receive_handle_event(t_aoo_receive *x, const AooEvent *event, int32_t)
{
    switch (event->type){
    case kAooEventXRun:
    {
        t_atom msg;
        SETFLOAT(&msg, event->xrun.count);
        outlet_anything(x->x_msgout, gensym("xrun"), 1, &msg);
        break;
    }
    case kAooEventSourceAdd:
    case kAooEventSourceRemove:
    case kAooEventInviteDecline:
    case kAooEventInviteTimeout:
    case kAooEventUninviteTimeout:
    case kAooEventBufferOverrun:
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
        auto& ep = event->endpoint.endpoint;
        aoo::ip_address addr((const sockaddr *)ep.address, ep.addrlen);
        t_atom msg[32];
        if (!x->x_node->serialize_endpoint(addr, ep.id, 3, msg)) {
            bug("aoo_receive_handle_event: serialize_endpoint");
            return;
        }
        // event data
        switch (event->type){
        case kAooEventSourceAdd:
        {
            // first add to source list; try to find peer name!
            t_symbol *group = nullptr;
            t_symbol *user = nullptr;
            x->x_node->find_peer(addr, group, user);
            x->x_sources.push_back({ addr, ep.id, group, user });

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
        case kAooEventInviteDecline:
        {
            outlet_anything(x->x_msgout, gensym("invite_decline"), 3, msg);
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
        case kAooEventPing:
        {
            auto& e = event->ping;

            double diff1 = aoo_ntpTimeDuration(e.t1, e.t2) * 1000.0;
            double diff2 = aoo_ntpTimeDuration(e.t2, e.t3) * 1000.0;
            double rtt = aoo_ntpTimeDuration(e.t1, e.t3) * 1000.0;

            SETSYMBOL(msg + 3, gensym("ping"));
            SETFLOAT(msg + 4, diff1);
            SETFLOAT(msg + 5, diff2);
            SETFLOAT(msg + 6, rtt);

            outlet_anything(x->x_msgout, gensym("event"), 7, msg);

            break;
        }
        case kAooEventBufferOverrun:
        {
            SETSYMBOL(msg + 3, gensym("overrun"));
            outlet_anything(x->x_msgout, gensym("event"), 4, msg);
            break;
        }
        case kAooEventBufferUnderrun:
        {
            SETSYMBOL(msg + 3, gensym("underrun"));
            outlet_anything(x->x_msgout, gensym("event"), 4, msg);
            break;
        }
        case kAooEventFormatChange:
        {
            SETSYMBOL(msg + 3, gensym("format"));
            // skip first 4 atoms
            int n = format_to_atoms(*event->formatChange.format, 29, msg + 4);
            outlet_anything(x->x_msgout, gensym("event"), n + 4, msg);
            break;
        }
        case kAooEventStreamStart:
        {
            auto& e = event->streamStart;
            SETSYMBOL(msg + 3, gensym("start"));
            if (e.metadata){
                auto count = e.metadata->size + 5;
                t_atom *vec = (t_atom *)alloca(count * sizeof(t_atom));
                // copy endpoint + event name
                memcpy(vec, msg, 4 * sizeof(t_atom));
                // type
                datatype_to_atom(e.metadata->type, vec[4]);
                // data
                for (int i = 0; i < e.metadata->size; ++i){
                    SETFLOAT(vec + 5 + i, (uint8_t)e.metadata->data[i]);
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
            auto state = event->streamState.state;
            auto offset = event->streamState.sampleOffset;
            if (offset > 0) {
                // HACK: schedule as fake stream message
                AooStreamMessage msg;
                msg.type = STREAM_MESSAGE_OFFSET + state;
                msg.sampleOffset = offset;
                // pass 1 byte of dummy data
                msg.size = 1;
                msg.data = (AooByte *)event;
                aoo_receive_handle_stream_message(x, &msg, &ep);
            } else {
                SETSYMBOL(msg + 3, gensym("state"));
                SETFLOAT(msg + 4, state);
                outlet_anything(x->x_msgout, gensym("event"), 5, msg);
            }
            break;
        }
        case kAooEventBlockLost:
        {
            SETSYMBOL(msg + 3, gensym("block_lost"));
            SETFLOAT(msg + 4, event->blockLost.count);
            outlet_anything(x->x_msgout, gensym("event"), 5, msg);
            break;
        }
        case kAooEventBlockDropped:
        {
            SETSYMBOL(msg + 3, gensym("block_dropped"));
            SETFLOAT(msg + 4, event->blockDropped.count);
            outlet_anything(x->x_msgout, gensym("event"), 5, msg);
            break;
        }
        case kAooEventBlockReordered:
        {
            SETSYMBOL(msg + 3, gensym("block_reordered"));
            SETFLOAT(msg + 4, event->blockReordered.count);
            outlet_anything(x->x_msgout, gensym("event"), 5, msg);
            break;
        }
        case kAooEventBlockResent:
        {
            SETSYMBOL(msg + 3, gensym("block_resent"));
            SETFLOAT(msg + 4, event->blockResent.count);
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

static void aoo_receive_queue_tick(t_aoo_receive *x)
{
    auto& queue = x->x_queue;
    auto now = clock_getlogicaltime();

    while (!queue.empty()){
        if (queue.top().time <= now) {
            auto& m = queue.top().data;
            AooStreamMessage msg { 0, m.type, m.data.data(), m.data.size() };
            x->dispatch_stream_message(msg, m.address, m.id);
            queue.pop();
        } else {
            break;
        }
    }
    // reschedule
    if (!queue.empty()){
        clock_set(x->x_queue_clock, queue.top().time);
    }
}

void t_aoo_receive::dispatch_stream_message(const AooStreamMessage& msg,
                                            const aoo::ip_address& address, AooId id) {
    auto size = msg.size + 4;
    auto vec = (t_atom *)alloca(sizeof(t_atom) * size);
    if (!x_node->serialize_endpoint(address, id, 3, vec)) {
        bug("dispatch_stream_message: serialize_endpoint");
        return;
    }
    if (msg.type >= STREAM_MESSAGE_OFFSET) {
        // fake stream message
        assert(size == 5); // see aoo_receive_handle_event()
        SETSYMBOL(vec + 3, gensym("state"));
        SETFLOAT(vec + 4, msg.type - STREAM_MESSAGE_OFFSET);

        outlet_anything(x_msgout, gensym("event"), size, vec);
    } else {
        // message type
        datatype_to_atom(msg.type, vec[3]);
        // message content
        for (int i = 0; i < msg.size; ++i) {
            SETFLOAT(&vec[i + 4], msg.data[i]);
        }

        outlet_anything(x_msgout, gensym("msg"), size, vec);
    }
}

static void aoo_receive_handle_stream_message(t_aoo_receive *x, const AooStreamMessage *msg, const AooEndpoint *ep)
{
    auto delay = (double)msg->sampleOffset / (double)x->x_samplerate * 1000.0;
    if (delay > 0) {
        // put on queue and schedule on clock (using logical time)
        auto abstime = clock_getsystimeafter(delay);
        // reschedule if we are the next due element
        if (x->x_queue.empty() || abstime < x->x_queue.top().time) {
            clock_set(x->x_queue_clock, abstime);
        }
        x->x_queue.emplace(t_stream_message(*msg, *ep), abstime);
    } else {
        // dispatch immediately
        aoo::ip_address addr((const sockaddr *)ep->address, ep->addrlen);
        x->dispatch_stream_message(*msg, addr, ep->id);
    }

}

static t_int * aoo_receive_perform(t_int *w)
{
    t_aoo_receive *x = (t_aoo_receive *)(w[1]);
    int n = (int)(w[2]);

    if (x->x_node){
        auto err = x->x_sink->process(x->x_vec.get(), n, get_osctime(),
                                      (AooStreamMessageHandler)aoo_receive_handle_stream_message, x);
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
    x_queue_clock = clock_new(this, (t_method)aoo_receive_queue_tick);
    x_metadata_type = kAooDataUnspecified;

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
    float latency = argc > 3 ? atom_getfloat(argv + 3) : DEFAULT_LATENCY;

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

    x_sink->setLatency(latency * 0.001);

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
    clock_free(x_queue_clock);
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
    class_addmethod(aoo_receive_class, (t_method)aoo_receive_latency,
                    gensym("latency"), A_FLOAT, A_NULL);
    class_addmethod(aoo_receive_class, (t_method)aoo_receive_buffersize,
                    gensym("buffersize"), A_FLOAT, A_NULL);
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
    class_addmethod(aoo_receive_class, (t_method)aoo_receive_source_list,
                    gensym("source_list"), A_NULL);
    class_addmethod(aoo_receive_class, (t_method)aoo_receive_reset,
                    gensym("reset"), A_GIMME, A_NULL);
    class_addmethod(aoo_receive_class, (t_method)aoo_receive_fill_ratio,
                    gensym("fill_ratio"), A_GIMME, A_NULL);
}
