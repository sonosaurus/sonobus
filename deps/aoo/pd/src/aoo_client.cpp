/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "aoo_common.hpp"

#include "common/sync.hpp"
#include "common/time.hpp"

#include "oscpack/osc/OscReceivedElements.h"

#include <functional>
#include <vector>
#include <map>

using namespace aoo;

#define AOO_CLIENT_POLL_INTERVAL 2

t_class *aoo_client_class;

enum t_target
{
    TARGET_BROADCAST,
    TARGET_PEER,
    TARGET_GROUP,
    TARGET_NONE
};

#define DEJITTER 1

struct t_osc_message
{
    t_osc_message(const char *data, int32_t size,
                  const ip_address& address)
        : data_(data, size), address_(address) {}
    const char *data() const { return data_.data(); }
    int32_t size() const { return data_.size(); }
    const ip_address& address() const { return address_; }
private:
    std::string data_;
    ip_address address_;
};

struct t_peer
{
    t_symbol *group;
    t_symbol *user;
    aoo::ip_address address;
    aoo_id id;
};

struct t_aoo_client
{
    t_aoo_client(int argc, t_atom *argv);
    ~t_aoo_client();

    t_object x_obj;

    t_node *x_node = nullptr;

    // for OSC messages
    ip_address x_peeraddr;
    t_symbol *x_group = nullptr;
    t_dejitter *x_dejitter = nullptr;
    t_float x_offset = -1; // immediately
    t_target x_target = TARGET_BROADCAST;
    bool x_connected = false;
    bool x_schedule = true;
    bool x_discard = false;
    bool x_reliable = false;
    std::multimap<float, t_osc_message> x_queue;

    void handle_peer_message(const char *data, int32_t size,
                             const ip_address& address, aoo::time_tag t);

    void handle_peer_bundle(const osc::ReceivedBundle& bundle,
                            const ip_address& address, aoo::time_tag t);

    void perform_message(const char *data, int32_t size,
                         const ip_address& address, double delay) const;

    void perform_message(const t_osc_message& msg) const {
        perform_message(msg.data(), msg.size(), msg.address(), 0);
    }

    void send_message(int argc, t_atom *argv,
                      const void *target, int32_t len);

    const t_peer * find_peer(const ip_address& addr) const;

    // replies
    using t_reply = std::function<void()>;
    std::vector<t_reply> replies_;
    aoo::sync::mutex reply_mutex_;
    void push_reply(t_reply reply){
        aoo::sync::scoped_lock<aoo::sync::mutex> lock(reply_mutex_);
        replies_.push_back(std::move(reply));
    }

    // peer list
    std::vector<t_peer> x_peers;

    t_clock *x_clock = nullptr;
    t_clock *x_queue_clock = nullptr;
    t_outlet *x_stateout = nullptr;
    t_outlet *x_msgout = nullptr;
    t_outlet *x_addrout = nullptr;
};

struct t_group_request {
    t_aoo_client *obj;
    t_symbol *group;
    t_symbol *pwd;
};

const t_peer * t_aoo_client::find_peer(const ip_address& addr) const {
    for (auto& peer : x_peers){
        if (peer.address == addr){
            return &peer;
        }
    }
    return nullptr;
}

int aoo_client_resolve_address(const t_aoo_client *x, const ip_address& addr,
                                aoo_id id, int argc, t_atom *argv){
    if (argc < 3){
        return 0;
    }

    auto peer = x->find_peer(addr);
    if (peer){
        SETSYMBOL(argv, peer->group);
        SETSYMBOL(argv + 1, peer->user);
        SETFLOAT(argv + 2, id);
        return 3;
    } else {
        return endpoint_to_atoms(addr, id, argc, argv);
    }
}

static void aoo_client_peer_list(t_aoo_client *x)
{
    for (auto& peer : x->x_peers){
        t_atom msg[5];
        SETSYMBOL(msg, peer.group);
        SETSYMBOL(msg + 1, peer.user);
        SETFLOAT(msg + 2, peer.id);
        address_to_atoms(peer.address, 2, msg + 3);

        outlet_anything(x->x_msgout, gensym("peer"), 5, msg);
    }
}

// send OSC messages to peers
void t_aoo_client::send_message(int argc, t_atom *argv,
                                const void *target, int32_t len)
{
    if (!argc){
        return;
    }
    if (!x_connected){
        pd_error(this, "%s: not connected", classname(this));
        return;
    }

    char *buf;
    int32_t count;

    // schedule OSC message as bundle (not needed for OSC bundles!)
    if (x_offset >= 0 && atom_getsymbol(argv)->s_name[0] != '#') {
        // make timetag relative to current OSC time
    #if DEJITTER
        aoo::time_tag now = get_osctime_dejitter(x_dejitter);
    #else
        aoo::time_tag now = get_osctime();
    #endif
        auto time = now + aoo::time_tag::from_seconds(x_offset * 0.001);

        const int headersize = 20; //#bundle string (8), timetag (8), message size (4)
        count = argc + headersize;
        buf = (char *)alloca(count);
        // make bundle header
        memcpy(buf, "#bundle\0", 8); // string length is exactly 8 bytes
        aoo::to_bytes((uint64_t)time, buf + 8);
        aoo::to_bytes((int32_t)argc, buf + 16);
        // add message to bundle
        for (int i = 0; i < argc; ++i){
            buf[i + headersize] = argv[i].a_type == A_FLOAT ? argv[i].a_w.w_float : 0;
        }
    } else {
        // send as is
        buf = (char *)alloca(argc);
        for (int i = 0; i < argc; ++i){
            buf[i] = argv[i].a_type == A_FLOAT ? argv[i].a_w.w_float : 0;
        }
        count = argc;
    }

    int32_t flags = x_reliable ? AOO_NET_MESSAGE_RELIABLE : 0;
    x_node->client()->send_message(buf, count, target, len, flags);

    x_node->notify();
}

static void aoo_client_broadcast(t_aoo_client *x, t_symbol *s, int argc, t_atom *argv)
{
    if (x->x_node){
        x->send_message(argc, argv, 0, 0);
    }
}

static void aoo_client_send_group(t_aoo_client *x, t_symbol *s, int argc, t_atom *argv)
{
    if (x->x_node){
        if (argc > 1){
            if (argv->a_type == A_SYMBOL){
                auto group = argv->a_w.w_symbol;
                x->send_message(argc - 1, argv + 1, group->s_name, 0);
            }
        }
        pd_error(x, "%s: bad arguments to 'send_group' - expecting <group> <data...>",
                 classname(x));
    }
}

static void aoo_client_send_peer(t_aoo_client *x, t_symbol *s, int argc, t_atom *argv)
{
    if (x->x_node){
        ip_address address;
        if (x->x_node->get_peer_arg((t_pd *)x, argc, argv, address)){
            x->send_message(argc - 2, argv + 2, address.address(), address.length());
        }
    }
}

static void aoo_client_list(t_aoo_client *x, t_symbol *s, int argc, t_atom *argv)
{
    if (x->x_node){
        switch (x->x_target){
        case TARGET_PEER:
            x->send_message(argc, argv, x->x_peeraddr.address(), x->x_peeraddr.length());
            break;
        case TARGET_GROUP:
            x->send_message(argc, argv, x->x_group->s_name, 0);
            break;
        case TARGET_BROADCAST:
            x->send_message(argc, argv, 0, 0);
            break;
        default: // TARGET_NONE
            break;
        }
    }
}

static void aoo_client_offset(t_aoo_client *x, t_floatarg f)
{
    x->x_offset = f;
}

static void aoo_client_schedule(t_aoo_client *x, t_floatarg f)
{
    x->x_schedule = (f != 0);
}

static void aoo_client_discard_late(t_aoo_client *x, t_floatarg f)
{
    x->x_discard = (f != 0);
}

static void aoo_client_reliable(t_aoo_client *x, t_floatarg f)
{
    x->x_reliable = (f != 0);
}

static void aoo_client_target(t_aoo_client *x, t_symbol *s, int argc, t_atom *argv)
{
    if (x->x_node){
        if (argc > 1){
            // <ip> <port> or <group> <peer>
            if (x->x_node->get_peer_arg((t_pd *)x, argc, argv, x->x_peeraddr)){
                x->x_target = TARGET_PEER;
            } else {
                // this is important, so that we don't accidentally broadcast!
                x->x_target = TARGET_NONE;
            }
        } else if (argc == 1){
            // <group>
            if (argv->a_type == A_SYMBOL){
                x->x_target = TARGET_GROUP;
                x->x_group = argv->a_w.w_symbol;
            } else {
                pd_error(x, "%s: bad argument to 'target' message", classname(x));
                x->x_target = TARGET_NONE;
            }
        } else {
            x->x_target = TARGET_BROADCAST;
        }
    }
}

// handle incoming OSC message/bundle from peer
void t_aoo_client::perform_message(const char *data, int32_t size,
                                   const ip_address& address, double delay) const
{
    // 1) peer + time tag
    t_atom info[3];
    auto peer = find_peer(address);
    if (peer){
        SETSYMBOL(info, peer->group);
        SETSYMBOL(info + 1, peer->user);
    } else {
        address_to_atoms(address, 2, info);
    }
    SETFLOAT(info + 2, delay);

    outlet_list(x_addrout, &s_list, 3, info);

    // 2) OSC message
    auto msg = (t_atom *)alloca(size * sizeof(t_atom));
    for (int i = 0; i < size; ++i){
        SETFLOAT(msg + i, (uint8_t)data[i]);
    }

    outlet_list(x_msgout, &s_list, size, msg);
}

static void aoo_client_queue_tick(t_aoo_client *x)
{
    auto& queue = x->x_queue;
    auto now = clock_getlogicaltime();

    for (auto it = queue.begin(); it != queue.end();){
        auto time = it->first;
        auto& msg = it->second;
        if (time <= now){
            x->perform_message(msg);
            it = queue.erase(it);
        } else {
            ++it;
        }
    }
    // reset clock
    if (!queue.empty()){
        // make sure update_jitter_offset() is called once per DSP tick!
        clock_set(x->x_queue_clock, queue.begin()->first);
    }
}



void t_aoo_client::handle_peer_message(const char *data, int32_t size,
                                       const ip_address& address, aoo::time_tag t)
{
    if (!t.is_immediate()){
        auto now = aoo::get_osctime();
        auto delay = aoo::time_tag::duration(now, t) * 1000.0;
        if (x_schedule){
            if (delay > 0){
                // put on queue and schedule on clock (using logical time)
                t_osc_message msg(data, size, address);
                auto abstime = clock_getsystimeafter(delay);
                auto pos = x_queue.emplace(abstime, std::move(msg));
                // only set clock if we're the first element in the queue
                if (pos == x_queue.begin()){
                    clock_set(x_queue_clock, abstime);
                }
            } else if (!x_discard){
                // treat like immediate message
                perform_message(data, size, address, 0);
            }
        } else {
            // output immediately with delay
            perform_message(data, size, address, delay);
        }
    } else {
        // send immediately
        perform_message(data, size, address, 0);
    }
}

void t_aoo_client::handle_peer_bundle(const osc::ReceivedBundle& bundle,
                                      const ip_address& address, aoo::time_tag t)
{
    auto it = bundle.ElementsBegin();
    while (it != bundle.ElementsEnd()){
        if (it->IsBundle()){
            osc::ReceivedBundle b(*it);
            handle_peer_bundle(b, address, b.TimeTag());
        } else {
            handle_peer_message(it->Contents(), it->Size(), address, t);
        }
        ++it;
    }
}

void aoo_client_handle_event(t_aoo_client *x, const aoo_event *event, int32_t level)
{
    switch (event->type){
    case AOO_NET_PEER_MESSAGE_EVENT:
    {
        auto e = (const aoo_net_message_event *)event;

        ip_address address((const sockaddr *)e->address, e->addrlen);

        try {
            osc::ReceivedPacket packet(e->data, e->size);
            if (packet.IsBundle()){
                osc::ReceivedBundle bundle(packet);
                x->handle_peer_bundle(bundle, address, bundle.TimeTag());
            } else {
                x->handle_peer_message(packet.Contents(), packet.Size(),
                                       address, aoo::time_tag::immediate());
            }
        } catch (const osc::Exception &err){
            pd_error(x, "%s: bad OSC message - %s", classname(x), err.what());
        }
        break;
    }
    case AOO_NET_DISCONNECT_EVENT:
    {
        post("%s: disconnected from server", classname(x));

        x->x_peers.clear();
        x->x_connected = false;

        outlet_float(x->x_stateout, 0); // disconnected
        break;
    }
    case AOO_NET_PEER_JOIN_EVENT:
    {
        auto e = (const aoo_net_peer_event *)event;

        ip_address addr((const sockaddr *)e->address, e->addrlen);
        auto group = gensym(e->group_name);
        auto user = gensym(e->user_name);
        auto id = e->user_id;

        for (auto& peer : x->x_peers){
            if (peer.group == group && peer.id == id){
                bug("aoo_client: couldn't add peer %s|%s",
                    group->s_name, user->s_name);
                return;
            }
        }
        x->x_peers.push_back({ group, user, addr, id });

        t_atom msg[5];
        SETSYMBOL(msg, group);
        SETSYMBOL(msg + 1, user);
        SETFLOAT(msg + 2, id);
        address_to_atoms(addr, 2, msg + 3);

        outlet_anything(x->x_msgout, gensym("peer_join"), 5, msg);
        break;
    }
    case AOO_NET_PEER_LEAVE_EVENT:
    {
        auto e = (const aoo_net_peer_event *)event;

        ip_address addr((const sockaddr *)e->address, e->addrlen);
        auto group = gensym(e->group_name);
        auto user = gensym(e->user_name);
        auto id = e->user_id;

        for (auto it = x->x_peers.begin(); it != x->x_peers.end(); ++it){
            if (it->group == group && it->id == id){
                x->x_peers.erase(it);

                t_atom msg[5];
                SETSYMBOL(msg, group);
                SETSYMBOL(msg + 1, user);
                SETFLOAT(msg + 2, id);
                address_to_atoms(addr, 2, msg + 3);

                outlet_anything(x->x_msgout, gensym("peer_leave"), 5, msg);

                return;
            }
        }
        bug("aoo_client: couldn't remove peer %s|%s",
            group->s_name, user->s_name);
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

static void aoo_client_tick(t_aoo_client *x)
{
    x->x_node->client()->poll_events();

    x->x_node->notify();

    // handle server replies
    aoo::sync::unique_lock<aoo::sync::mutex> lock(x->reply_mutex_,
                                                  aoo::sync::try_to_lock);
    if (lock.owns_lock()){
        for (auto& reply : x->replies_){
            reply();
        }
        x->replies_.clear();
    } else {
        LOG_DEBUG("aoo_client_tick: would block");
    }

    clock_delay(x->x_clock, AOO_CLIENT_POLL_INTERVAL);
}

struct t_error_reply {
    int code;
    std::string msg;
};

static void aoo_client_group_join(t_aoo_client *x, t_symbol *group, t_symbol *pwd);

static void aoo_client_connect(t_aoo_client *x, t_symbol *s, int argc, t_atom *argv)
{
    if (argc < 3){
        pd_error(x, "%s: too few arguments for '%s' method", classname(x), s->s_name);
        return;
    }
    if (x->x_node){
        t_symbol *host = atom_getsymbol(argv);
        int port = atom_getfloat(argv + 1);
        t_symbol *userName = atom_getsymbol(argv + 2);
        t_symbol *userPwd = argc > 3 ? atom_getsymbol(argv + 3) : gensym("");
        t_symbol *group = argc > 4 ? atom_getsymbol(argv + 4) : nullptr;
        t_symbol *groupPwd = argc > 5 ? atom_getsymbol(argv + 5) : nullptr;

        // LATER also send user ID
        auto cb = [](void *x, aoo_error result, const void *data){
            auto request = (t_group_request *)x;
            auto obj = request->obj;
            auto group = request->group;
            auto pwd = request->pwd;

            if (result == AOO_OK){
                auto reply = (const aoo_net_connect_reply *)data;
                auto user_id = reply->user_id;
                obj->push_reply([obj, user_id, group, pwd](){
                    // remove all peers (to be sure)
                    obj->x_peers.clear();
                    obj->x_connected = true;

                    t_atom msg;
                    SETFLOAT(&msg, user_id);
                    outlet_anything(obj->x_msgout, gensym("id"), 1, &msg);

                    outlet_float(obj->x_stateout, 1); // connected

                    if (group && pwd){
                        aoo_client_group_join(obj, group, pwd);
                    }
                });
            } else {
                auto reply = (const aoo_net_error_reply *)data;
                t_error_reply error { reply->error_code, reply->error_message };

                obj->push_reply([obj, error=std::move(error)](){
                    pd_error(obj, "%s: couldn't connect to server: %s",
                             classname(obj), error.msg.c_str());

                    if (!obj->x_connected){
                        outlet_float(obj->x_stateout, 0);
                    }
                });
            }

            delete request;
        };
        x->x_node->client()->connect(host->s_name, port,
                             userName->s_name, userPwd->s_name, cb,
                             new t_group_request { x, group, groupPwd });
    }
}

static void aoo_client_disconnect(t_aoo_client *x)
{
    if (x->x_node){
        auto cb = [](void *y, aoo_error result, const void *data){
            auto x = (t_aoo_client *)y;
            if (result == AOO_OK){
                x->push_reply([x](){
                    // we have to remove the peers manually!
                    x->x_peers.clear();
                    x->x_connected = false;

                    outlet_float(x->x_stateout, 0); // disconnected
                });
            } else {
                auto reply = (const aoo_net_error_reply *)data;
                t_error_reply error { reply->error_code, reply->error_message };

                x->push_reply([x, error=std::move(error)](){
                    pd_error(x, "%s: couldn't disconnect from server: %s",
                             classname(x), error.msg.c_str());
                });
            }
        };
        x->x_node->client()->disconnect(cb, x);
    }
}

static void aoo_client_group_join(t_aoo_client *x, t_symbol *group, t_symbol *pwd)
{
    if (x->x_node){
        auto cb = [](void *x, aoo_error result, const void *data){
            auto request = (t_group_request *)x;
            auto obj = request->obj;
            auto group = request->group;

            if (result == AOO_OK){
                obj->push_reply([obj, group](){
                    t_atom msg[2];
                    SETSYMBOL(msg, group);
                    SETFLOAT(msg + 1, 1); // 1: success
                    outlet_anything(obj->x_msgout, gensym("group_join"), 2, msg);
                });
            } else {
                auto reply = (const aoo_net_error_reply *)data;
                t_error_reply error { reply->error_code, reply->error_message };

                obj->push_reply([obj, group, error=std::move(error)](){
                    pd_error(obj, "%s: couldn't join group %s - %s",
                             classname(obj), group->s_name, error.msg.c_str());

                    t_atom msg[2];
                    SETSYMBOL(msg, group);
                    SETFLOAT(msg + 1, 0); // 0: fail
                    outlet_anything(obj->x_msgout, gensym("group_join"), 2, msg);
                });
            }

            delete request;
        };
        x->x_node->client()->join_group(group->s_name, pwd->s_name,
            cb, new t_group_request { x, group, nullptr });
    }
}

static void aoo_client_group_leave(t_aoo_client *x, t_symbol *group)
{
    if (x->x_node){
        auto cb = [](void *x, aoo_error result, const void *data){
            auto request = (t_group_request *)x;
            auto obj = request->obj;
            auto group = request->group;

            if (result == AOO_OK){
                obj->push_reply([obj, group](){
                    // we have to remove the peers manually!
                    for (auto it = obj->x_peers.begin(); it != obj->x_peers.end(); ) {
                        // remove all peers matching group
                        if (it->group == group){
                            it = obj->x_peers.erase(it);
                        } else {
                            ++it;
                        }
                    }

                    t_atom msg[2];
                    SETSYMBOL(msg, group);
                    SETFLOAT(msg + 1, 1); // 1: success
                    outlet_anything(obj->x_msgout, gensym("group_leave"), 2, msg);
                });
            } else {
                auto reply = (const aoo_net_error_reply *)data;
                t_error_reply error { reply->error_code, reply->error_message };

                obj->push_reply([obj, group, error=std::move(error)](){
                    pd_error(obj, "%s: couldn't leave group %s - %s",
                             classname(obj), group->s_name, error.msg.c_str());

                    t_atom msg[2];
                    SETSYMBOL(msg, group);
                    SETFLOAT(msg + 1, 0); // 0: fail
                    outlet_anything(obj->x_msgout, gensym("group_leave"), 2, msg);
                });
            }

            delete request;
        };
        x->x_node->client()->leave_group(group->s_name, cb,
                                 new t_group_request { x, group, nullptr });
    }
}

static void * aoo_client_new(t_symbol *s, int argc, t_atom *argv)
{
    void *x = pd_new(aoo_client_class);
    new (x) t_aoo_client(argc, argv);
    return x;
}

t_aoo_client::t_aoo_client(int argc, t_atom *argv)
{
    x_clock = clock_new(this, (t_method)aoo_client_tick);
    x_queue_clock = clock_new(this, (t_method)aoo_client_queue_tick);
    x_stateout = outlet_new(&x_obj, 0);
    x_msgout = outlet_new(&x_obj, 0);
    x_addrout = outlet_new(&x_obj, 0);

    int port = argc ? atom_getfloat(argv) : 0;

    x_node = port > 0 ? t_node::get((t_pd *)this, port) : nullptr;

    if (x_node){
        verbose(0, "new aoo client on port %d", port);
        // get dejitter context
        x_dejitter = get_dejitter();
        // start clock
        clock_delay(x_clock, AOO_CLIENT_POLL_INTERVAL);
    }
}

static void aoo_client_free(t_aoo_client *x)
{
    x->~t_aoo_client();
}

t_aoo_client::~t_aoo_client()
{
    if (x_node){
        if (x_connected){
            x_node->client()->disconnect(0, 0);
        }
        x_node->release((t_pd *)this);
    }

    // ignore pending requests (doesn't leak)

    clock_free(x_clock);
    clock_free(x_queue_clock);
}

void aoo_client_setup(void)
{
    aoo_client_class = class_new(gensym("aoo_client"), (t_newmethod)(void *)aoo_client_new,
        (t_method)aoo_client_free, sizeof(t_aoo_client), 0, A_GIMME, A_NULL);
    class_sethelpsymbol(aoo_client_class, gensym("aoo_net"));

    class_addmethod(aoo_client_class, (t_method)aoo_client_connect,
                    gensym("connect"), A_GIMME, A_NULL);
    class_addmethod(aoo_client_class, (t_method)aoo_client_disconnect,
                    gensym("disconnect"), A_NULL);
    class_addmethod(aoo_client_class, (t_method)aoo_client_group_join,
                    gensym("group_join"), A_SYMBOL, A_SYMBOL, A_NULL);
    class_addmethod(aoo_client_class, (t_method)aoo_client_group_leave,
                    gensym("group_leave"), A_SYMBOL, A_NULL);
    class_addmethod(aoo_client_class, (t_method)aoo_client_peer_list,
                    gensym("peer_list"), A_NULL);
    class_addmethod(aoo_client_class, (t_method)aoo_client_broadcast,
                    gensym("broadcast"), A_GIMME, A_NULL);
    class_addmethod(aoo_client_class, (t_method)aoo_client_send_peer,
                    gensym("send_peer"), A_GIMME, A_NULL);
    class_addmethod(aoo_client_class, (t_method)aoo_client_send_group,
                    gensym("send_group"), A_GIMME, A_NULL);
    class_addmethod(aoo_client_class, (t_method)aoo_client_target,
                    gensym("target"), A_GIMME, A_NULL);
    class_addmethod(aoo_client_class, (t_method)aoo_client_list,
                    gensym("send"), A_GIMME, A_NULL);
    class_addlist(aoo_client_class, aoo_client_list); // shortcut for "send"
    class_addmethod(aoo_client_class, (t_method)aoo_client_offset,
                    gensym("offset"), A_DEFFLOAT, A_NULL);
    class_addmethod(aoo_client_class, (t_method)aoo_client_schedule,
                    gensym("schedule"), A_FLOAT, A_NULL);
    class_addmethod(aoo_client_class, (t_method)aoo_client_discard_late,
                    gensym("discard_late"), A_FLOAT, A_NULL);
    class_addmethod(aoo_client_class, (t_method)aoo_client_reliable,
                    gensym("reliable"), A_FLOAT, A_NULL);
}
