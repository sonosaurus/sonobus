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

#define AOO_CLIENT_POLL_INTERVAL 2

t_class *aoo_client_class;

#define DEJITTER 1

struct t_peer_message
{
    t_peer_message(AooId grp, AooId usr, const AooDataView& msg)
        : group(grp), user(usr), type(gensym(msg.type)),
          data(msg.data, msg.data + msg.size) {}
    AooId group;
    AooId user;
    t_symbol *type;
    std::vector<AooByte> data;
};

struct t_group
{
    t_symbol *name;
    AooId id;
};

struct t_peer
{
    t_symbol *group_name;
    t_symbol *user_name;
    AooId group_id;
    AooId user_id;
    aoo::ip_address address;
};

struct t_aoo_client
{
    t_aoo_client(int argc, t_atom *argv);
    ~t_aoo_client();

    t_object x_obj;

    t_node *x_node = nullptr;

    // for OSC messages
    t_dejitter *x_dejitter = nullptr;
    t_float x_offset = -1; // immediately
    bool x_connected = false;
    bool x_schedule = true;
    bool x_discard = false;
    bool x_reliable = false;
    bool x_target = true;
    AooId x_target_group = kAooIdInvalid; // broadcast
    AooId x_target_user = kAooIdInvalid; // broadcast
    std::multimap<float, t_peer_message> x_queue; // TODO replace with heap

    void handle_message(AooId group, AooId user, AooNtpTime time, const AooDataView& msg);

    void dispatch_message(AooId group, AooId user, const AooDataView& msg, double delay) const;

    void send_message(int argc, t_atom *argv, AooId group, AooId user);

    const t_peer * find_peer(const aoo::ip_address& addr) const;

    const t_peer * find_peer(AooId group, AooId user) const;

    const t_peer * find_peer(t_symbol *group, t_symbol *user) const;

    const t_group * find_group(t_symbol *name) const;

    const t_group *find_group(AooId id) const;

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
    // group list
    std::vector<t_group> x_groups;

    t_clock *x_clock = nullptr;
    t_clock *x_queue_clock = nullptr;
    t_outlet *x_stateout = nullptr;
    t_outlet *x_msgout = nullptr;
    t_outlet *x_addrout = nullptr;
};

const t_peer * t_aoo_client::find_peer(const aoo::ip_address& addr) const {
    for (auto& peer : x_peers) {
        if (peer.address == addr) {
            return &peer;
        }
    }
    return nullptr;
}

const t_peer * t_aoo_client::find_peer(AooId group, AooId user) const {
    for (auto& peer : x_peers) {
        if (peer.group_id == group && peer.user_id == user) {
            return &peer;
        }
    }
    return nullptr;
}

const t_peer * t_aoo_client::find_peer(t_symbol *group, t_symbol *user) const {
    for (auto& peer : x_peers) {
        if (peer.group_name == group && peer.user_name == user) {
            return &peer;
        }
    }
    return nullptr;
}

const t_group * t_aoo_client::find_group(t_symbol *name) const {
    for (auto& group : x_groups) {
        if (group.name == name) {
            return &group;
        }
    }
    return nullptr;
}

const t_group * t_aoo_client::find_group(AooId id) const {
    for (auto& group : x_groups) {
        if (group.id == id) {
            return &group;
        }
    }
    return nullptr;
}

static int peer_to_atoms(const t_peer& peer, int argc, t_atom *argv) {
    if (argc >= 5) {
        SETSYMBOL(argv, peer.group_name);
        // don't send group ID because it might be too large for a float
        SETSYMBOL(argv + 1, peer.user_name);
        SETFLOAT(argv + 2, peer.user_id);
        address_to_atoms(peer.address, 2, argv + 3);
        return 5;
    } else {
        return 0;
    }
}

static void aoo_client_peer_list(t_aoo_client *x)
{
    for (auto& peer : x->x_peers){
        t_atom msg[5];
        peer_to_atoms(peer, 5, msg);
        outlet_anything(x->x_msgout, gensym("peer"), 5, msg);
    }
}

// send OSC messages to peers
void t_aoo_client::send_message(int argc, t_atom *argv, AooId group, AooId user)
{
    // need at least type + 1 argument
    if (argc < 2){
        return;
    }
    if (!x_connected){
        pd_error(this, "%s: not connected", classname(this));
        return;
    }

    // TODO should we rather set the type via a message?
    t_symbol *type = atom_getsymbol(argv);
    argv++; argc--;

    // schedule OSC message as bundle (not needed for OSC bundles!)
    aoo::time_tag time;
    if (x_offset >= 0) {
        // make timetag relative to current OSC time
    #if DEJITTER
        aoo::time_tag now = get_osctime_dejitter(x_dejitter);
    #else
        aoo::time_tag now = get_osctime();
    #endif
        time = now + aoo::time_tag::from_seconds(x_offset * 0.001);
    } else {
        time = aoo::time_tag::immediate();
    }

    auto buf = (AooByte *)alloca(argc);
    for (int i = 0; i < argc; ++i){
        buf[i] = (AooByte)atom_getfloat(argv + i);
    }
    AooDataView data { type->s_name, buf, (AooSize)argc };

    AooFlag flags = x_reliable ? kAooNetMessageReliable : 0;
    x_node->client()->sendMessage(group, user, data, time.value(), flags);

    x_node->notify();
}

static void aoo_client_broadcast(t_aoo_client *x, t_symbol *s, int argc, t_atom *argv)
{
    if (x->x_node){
        x->send_message(argc, argv, kAooIdInvalid, kAooIdInvalid);
    }
}

static void aoo_client_send_group(t_aoo_client *x, t_symbol *s, int argc, t_atom *argv)
{
    // only attempt to send if there are peers!
    if (x->x_node && !x->x_peers.empty()) {
        if (argc > 1 && argv->a_type == A_SYMBOL) {
            auto name = argv->a_w.w_symbol;
            if (auto group = x->find_group(name)) {
                x->send_message(argc - 1, argv + 1, group->id, kAooIdInvalid);
            } else {
                pd_error(x, "%: could not find group '%s'", name->s_name);
            }
        } else {
            pd_error(x, "%s: bad arguments to 'send_group': expecting <group> ...",
                     classname(x));
        }
    }
}

static void aoo_client_send_peer(t_aoo_client *x, t_symbol *s, int argc, t_atom *argv)
{
    if (x->x_node) {
        if (argc > 2 && argv[0].a_type == A_SYMBOL && argv[1].a_type == A_SYMBOL) {
            auto group = argv[0].a_w.w_symbol;
            auto user = argv[1].a_w.w_symbol;
            if (auto peer = x->find_peer(group, user)) {
                x->send_message(argc - 1, argv + 1, peer->group_id, peer->user_id);
            } else {
                pd_error(x, "%: could not find peer %s|%s", group->s_name, user->s_name);
            }
        } else {
            pd_error(x, "%s: bad arguments to 'send_peer': expecting <group> <user> ...",
                     classname(x));
        }
    }
}

static void aoo_client_send(t_aoo_client *x, t_symbol *s, int argc, t_atom *argv)
{
    if (x->x_node && x->x_target) {
        x->send_message(argc, argv, x->x_target_group, x->x_target_user);
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

static void aoo_client_port(t_aoo_client *x, t_floatarg f)
{
    int port = f;

    // 0 is allowed -> don't listen
    if (port < 0) {
        pd_error(x, "%s: bad port %d", classname(x), port);
        return;
    }

    if (x->x_node) {
        x->x_node->release((t_pd *)x);
    }

    if (port) {
        x->x_node = t_node::get((t_pd *)x, port);
    } else {
        x->x_node = 0;
    }
}

static void aoo_client_target(t_aoo_client *x, t_symbol *s, int argc, t_atom *argv)
{
    if (x->x_node){
        if (argc > 1){
            // <ip> <port> or <group> <user>
            if (x->x_node->get_peer_arg((t_pd *)x, argc, argv,
                                        x->x_target_group, x->x_target_user)){
                x->x_target = true;
            } else {
                x->x_target = false;
            }
        } else if (argc == 1){
            // <group>
            if (argv->a_type == A_SYMBOL) {
                auto name = argv->a_w.w_symbol;
                if (auto group = x->find_group(name)) {
                    x->x_target_group = group->id;
                    x->x_target_user = kAooIdInvalid; // broadcast to all users
                    x->x_target = true;
                } else {
                    pd_error(x, "%s: could not find group '%s'", classname(x), name->s_name);
                    x->x_target = false;
                }
            } else {
                pd_error(x, "%s: bad argument to 'target' message", classname(x));
                x->x_target = false;
            }
        } else {
            x->x_target_group = kAooIdInvalid; // broadcast to all groups
            x->x_target_user = kAooIdInvalid;
            x->x_target = true;
        }
    }
}

// handle incoming peer message
void t_aoo_client::dispatch_message(AooId group, AooId user,
                                    const AooDataView& msg, double delay) const
{
    // 1) peer + delay
    t_atom info[3];
    auto peer = find_peer(group, user);
    if (peer) {
        SETSYMBOL(info, peer->group_name);
        SETSYMBOL(info + 1, peer->user_name);
    } else {
        // should never happen because the peer should have been added
        bug("dispatch_message");
        return;
    }
    SETFLOAT(info + 2, delay);

    outlet_list(x_addrout, &s_list, 3, info);

    // 2) message
    auto size = msg.size + 1;
    auto vec = (t_atom *)alloca(size * sizeof(t_atom));
    SETSYMBOL(vec, gensym(msg.type));
    for (int i = 0; i < msg.size; ++i){
        SETFLOAT(vec + i + 1, (uint8_t)msg.data[i]);
    }

    outlet_anything(x_msgout, gensym("msg"), size, vec);
}

static void aoo_client_queue_tick(t_aoo_client *x)
{
    auto& queue = x->x_queue;
    auto now = clock_getlogicaltime();

    for (auto it = queue.begin(); it != queue.end();){
        auto time = it->first;
        auto& msg = it->second;
        if (time <= now) {
            AooDataView data { msg.type->s_name,
                        msg.data.data(), msg.data.size() };
            x->dispatch_message(msg.group, msg.user, data, 0);
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

void t_aoo_client::handle_message(AooId group, AooId user, AooNtpTime time,
                                  const AooDataView& data)
{
    aoo::time_tag tt(time);
    if (!tt.is_empty() && !tt.is_immediate()){
        auto now = get_osctime();
        auto delay = aoo::time_tag::duration(now, tt) * 1000.0;
        if (x_schedule) {
            if (delay > 0){
                // put on queue and schedule on clock (using logical time)
                t_peer_message msg(group, user, data);
                auto abstime = clock_getsystimeafter(delay);
                auto pos = x_queue.emplace(abstime, std::move(msg));
                // only set clock if we're the first element in the queue
                if (pos == x_queue.begin()){
                    clock_set(x_queue_clock, abstime);
                }
            } else if (!x_discard){
                // treat like immediate message
                dispatch_message(group, user, data, 0);
            }
        } else {
            // output immediately with delay
            dispatch_message(group, user, data, delay);
        }
    } else {
        // send immediately
        dispatch_message(group, user, data, 0);
    }
}

void aoo_client_handle_event(t_aoo_client *x, const AooEvent *event, int32_t level)
{
    switch (event->type){
    case kAooNetEventPeerMessage:
    {
        auto e = (const AooNetEventPeerMessage *)event;
        x->handle_message(e->groupId, e->userId, e->timeStamp, e->data);
        break;
    }
    case kAooNetEventDisconnect:
    {
        post("%s: disconnected from server", classname(x));

        x->x_peers.clear();
        x->x_groups.clear();
        x->x_connected = false;

        outlet_float(x->x_stateout, 0); // disconnected

        break;
    }
    case kAooNetEventPeerHandshake:
    case kAooNetEventPeerTimeout:
    case kAooNetEventPeerJoin:
    case kAooNetEventPeerLeave:
    {
        auto e = (const AooNetEventPeer *)event;

        auto group_name = gensym(e->groupName);
        auto user_name = gensym(e->userName);
        auto group_id = e->groupId;
        auto user_id = e->userId;
        aoo::ip_address addr((const sockaddr *)e->address.data, e->address.size);

        t_atom msg[5];

        switch (event->type) {
        case kAooNetEventPeerHandshake:
        {
            SETSYMBOL(msg, group_name);
            SETSYMBOL(msg + 1, user_name);
            SETFLOAT(msg + 2, user_id);
            outlet_anything(x->x_msgout, gensym("peer_handshake"), 3, msg);
            break;
        }
        case kAooNetEventPeerTimeout:
        {
            SETSYMBOL(msg, group_name);
            SETSYMBOL(msg + 1,user_name);
            SETFLOAT(msg + 2, user_id);
            outlet_anything(x->x_msgout, gensym("peer_timeout"), 3, msg);
            break;
        }
        case kAooNetEventPeerJoin:
        {
            if (x->find_peer(group_id, user_id)) {
                bug("aoo_client: can't add peer %s|%s: already exists",
                    group_name->s_name, user_name->s_name);
                return;
            }

            // add peer
            auto& peer = x->x_peers.emplace_back(
                        t_peer { group_name, user_name, group_id, user_id, addr });
            peer_to_atoms(peer, 5, msg);

            outlet_anything(x->x_msgout, gensym("peer_join"), 5, msg);

            break;
        }
        case kAooNetEventPeerLeave:
        {
            for (auto it = x->x_peers.begin(); it != x->x_peers.end(); ++it) {
                if (it->group_id == group_id && it->user_id == user_id) {
                    peer_to_atoms(*it, 5, msg);

                    // remove *before* sending the message
                    x->x_peers.erase(it);

                    outlet_anything(x->x_msgout, gensym("peer_leave"), 5, msg);

                    return;
                }
            }
            bug("aoo_client: can't remove peer %s|%s: does not exist",
                group_name->s_name, user_name->s_name);
            break;
        }
        default:
            break;
        }

        break; // !
    }
    case kAooNetEventPeerPing:
    case kAooNetEventPeerPingReply:
    {
        // AooNetEventPeerPingReply is compatible with AooNetEventPeerPing
        auto e = (const AooNetEventPeerPingReply *)event;
        bool reply = e->type == kAooNetEventPeerPingReply;
        t_atom msg[8];

        auto peer = x->find_peer(e->group, e->user);
        if (!peer) {
            bug("aoo_client: can't find peer %d|%d for %s event",
                e->group, e->user, reply ? "ping reply" : "ping");
            return;
        }

        peer_to_atoms(*peer, 5, msg);

        if (reply) {
            // ping reply
            auto delta1 = aoo::time_tag::duration(e->tt1, e->tt2) * 1000;
            auto delta2 = aoo::time_tag::duration(e->tt2, e->tt3) * 1000;
            auto rtt = aoo::time_tag::duration(e->tt1, e->tt3) * 1000;

            peer_to_atoms(*peer, 5, msg);
            SETFLOAT(msg + 5, delta1);
            SETFLOAT(msg + 6, delta2);
            SETFLOAT(msg + 7, rtt);

            outlet_anything(x->x_msgout, gensym("peer_ping_reply"), 8, msg);
        } else {
            // ping
            auto delta = aoo::time_tag::duration(e->tt1, e->tt2) * 1000;
            SETFLOAT(msg + 5, delta);

            outlet_anything(x->x_msgout, gensym("peer_ping"), 6, msg);
        }

        break;
    }
    case kAooNetEventError:
    {
        auto e = (const AooNetEventError *)event;
        pd_error(x, "%s: %s", classname(x), e->errorMessage);
        break;
    }
    default:
        verbose(0, "%s: unknown event type %d", classname(x), event->type);
        break;
    }
}

static void aoo_client_tick(t_aoo_client *x)
{
    x->x_node->client()->pollEvents();

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

struct t_error_response {
    int code;
    std::string msg;
};

static void AOO_CALL connect_cb(t_aoo_client *x, const AooNetRequestConnect *request,
                                AooError result, const AooNetResponseConnect *response) {
    if (result == kAooOk) {
        x->push_reply([x](){
            // remove all peers and groups (to be sure)
            x->x_peers.clear();
            x->x_groups.clear();
            x->x_connected = true;

            outlet_float(x->x_stateout, 1); // connected
        });
    } else {
        auto e = (const AooNetResponseError *)response;
        t_error_response error { e->errorCode, e->errorMessage };

        x->push_reply([x, error=std::move(error)](){
            pd_error(x, "%s: can't connect to server: %s",
                     classname(x), error.msg.c_str());

            if (!x->x_connected){
                outlet_float(x->x_stateout, 0);
            }
        });
    }
}

static void aoo_client_connect(t_aoo_client *x, t_symbol *s, int argc, t_atom *argv)
{
    if (x->x_connected){
        pd_error(x, "%s: already connected", classname(x));
        return;
    }
    if (argc < 2){
        pd_error(x, "%s: too few arguments for '%s' method", classname(x), s->s_name);
        return;
    }
    if (x->x_node){
        t_symbol *host = atom_getsymbol(argv);
        int port = atom_getfloat(argv + 1);
        const char *pwd = argc > 2 ? atom_getsymbol(argv + 2)->s_name : nullptr;

        x->x_node->client()->connect(host->s_name, port, pwd, nullptr,
                                     (AooNetCallback)connect_cb, x);
    }
}

// AooClient::disconnect() can't fail if we're connected, so we can
// essentially disconnect "synchronously". A subsequent call to
// AooClient::connect() will be scheduled after the disconnect request,
// so users can simply do [disconnect, connect ...(.
static void aoo_client_disconnect(t_aoo_client *x)
{
    if (!x->x_connected){
        return; // don't complain
    }
    x->x_peers.clear();
    x->x_groups.clear();
    x->x_connected = false;
    if (x->x_node){
        x->x_node->client()->disconnect(nullptr, nullptr);
    }
    outlet_float(x->x_stateout, 0);
}

static void AOO_CALL group_join_cb(t_aoo_client *x, const AooNetRequestGroupJoin *request,
                                   AooError result, const AooNetResponseGroupJoin *response){
    if (result == kAooOk) {
        x->push_reply([x, group=std::string(request->groupName), id=response->groupId](){
            // add group
            auto name = gensym(group.c_str());
            if (!x->find_group(id)) {
                x->x_groups.push_back({ name, id });
            } else {
                bug("group_join_cb");
            }

            t_atom msg[2];
            SETSYMBOL(msg, name);
            SETFLOAT(msg + 1, 1); // 1: success
            outlet_anything(x->x_msgout, gensym("group_join"), 2, msg);
        });
    } else {
        auto e = (const AooNetResponseError *)response;
        t_error_response error { e->errorCode, e->errorMessage };

        x->push_reply([x, group=std::string(request->groupName), error=std::move(error)](){
            pd_error(x, "%s: can't join group '%s': %s",
                     classname(x), group.c_str(), error.msg.c_str());

            t_atom msg[2];
            SETSYMBOL(msg, gensym(group.c_str()));
            SETFLOAT(msg + 1, 0); // 0: fail
            outlet_anything(x->x_msgout, gensym("group_join"), 2, msg);
        });
    }
}

static void aoo_client_group_join(t_aoo_client *x, t_symbol *s, int argc, t_atom *argv)
{
    if (!x->x_connected){
        pd_error(x, "%s: not connected", classname(x));
        return;
    }
    if (x->x_node) {
        if (argc < 3){
            pd_error(x, "%s: too few arguments for '%s' method", classname(x), s->s_name);
            return;
        }
        auto group = atom_getsymbol(argv)->s_name;
        auto group_pwd = atom_getsymbol(argv + 1)->s_name;
        auto user = atom_getsymbol(argv + 2)->s_name;
        auto user_pwd = argc > 3 ? atom_getsymbol(argv + 3)->s_name : nullptr;

        x->x_node->client()->joinGroup(group, group_pwd, nullptr, user, user_pwd, nullptr, nullptr,
                                       (AooNetCallback)group_join_cb, x);
    }
}

static void AOO_CALL group_leave_cb(t_aoo_client *x, const AooNetRequestGroupLeave *request,
                                    AooError result, const AooNetResponseError *e) {
    if (result == kAooOk){
        x->push_reply([x, id=request->group]() {
            // remove group
            t_symbol *name = nullptr;
            for (auto it = x->x_groups.begin(); it != x->x_groups.end(); ++it) {
                if (it->id == id) {
                    name = it->name;
                    x->x_groups.erase(it);
                    break;
                }
            }
            if (!name) {
                bug("group_leave_cb");
            }
            // we have to remove the peers manually!
            for (auto it = x->x_peers.begin(); it != x->x_peers.end(); ) {
                // remove all peers matching group
                if (it->group_id == id) {
                    it = x->x_peers.erase(it);
                } else {
                    ++it;
                }
            }

            t_atom msg[2];
            SETSYMBOL(msg, name);
            SETFLOAT(msg + 1, 1); // 1: success
            outlet_anything(x->x_msgout, gensym("group_leave"), 2, msg);
        });
    } else {
        t_error_response error { e->errorCode, e->errorMessage };

        x->push_reply([x, id=request->group, error=std::move(error)]() {
            auto group = x->find_group(id);
            if (!group) {
                bug("group_leave_cb");
                return;
            }
            pd_error(x, "%s: can't leave group '%s': %s",
                     classname(x), group->name->s_name, error.msg.c_str());

            t_atom msg[2];
            SETSYMBOL(msg, group->name);
            SETFLOAT(msg + 1, 0); // 0: fail
            outlet_anything(x->x_msgout, gensym("group_leave"), 2, msg);
        });
    }
}

static void aoo_client_group_leave(t_aoo_client *x, t_symbol *group)
{
    if (!x->x_connected){
        pd_error(x, "%s: not connected", classname(x));
        return;
    }
    if (x->x_node) {
        auto g = x->find_group(group);
        if (g) {
            x->x_node->client()->leaveGroup(g->id, (AooNetCallback)group_leave_cb, x);
        } else {
            pd_error(x, "%s: can't leave group %s: not a group member",
                     classname(x), group->s_name);
        }
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

    class_addlist(aoo_client_class, aoo_client_send);

    class_addmethod(aoo_client_class, (t_method)aoo_client_connect,
                    gensym("connect"), A_GIMME, A_NULL);
    class_addmethod(aoo_client_class, (t_method)aoo_client_disconnect,
                    gensym("disconnect"), A_NULL);
    class_addmethod(aoo_client_class, (t_method)aoo_client_group_join,
                    gensym("group_join"), A_GIMME, A_NULL);
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
    class_addmethod(aoo_client_class, (t_method)aoo_client_send,
                    gensym("send"), A_GIMME, A_NULL);
    class_addmethod(aoo_client_class, (t_method)aoo_client_offset,
                    gensym("offset"), A_DEFFLOAT, A_NULL);
    class_addmethod(aoo_client_class, (t_method)aoo_client_schedule,
                    gensym("schedule"), A_FLOAT, A_NULL);
    class_addmethod(aoo_client_class, (t_method)aoo_client_discard_late,
                    gensym("discard_late"), A_FLOAT, A_NULL);
    class_addmethod(aoo_client_class, (t_method)aoo_client_reliable,
                    gensym("reliable"), A_FLOAT, A_NULL);
    class_addmethod(aoo_client_class, (t_method)aoo_client_port,
                    gensym("port"), A_FLOAT, A_NULL);
}
