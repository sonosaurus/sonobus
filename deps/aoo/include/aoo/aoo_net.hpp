/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#pragma once

#include "aoo/aoo_net.h"

#include <memory>

namespace aoo {
namespace net {

// NOTE: aoo::server and aoo::client don't have public virtual
// destructors and have to be freed with the destroy() method.
// We provide a custom deleter and shared pointer to simplify this task.
//
// By following certain COM conventions (no virtual destructors,
// no method overloading, only POD parameters and return types),
// we get ABI independent C++ interfaces on Windows and all other
// platforms where the vtable layout for such classes is stable
// (generally true for Linux and macOS, in my experience).
//
// In practice this means you only have to build 'aoo' once as a
// shared library and can then use its C++ interface in applications
// built with different compilers resp. compiler versions.
//
// If you want to be on the safe safe, use the C interface :-)

/*//////////////////////// AoO server ///////////////////////*/

class server {
public:
    class deleter {
    public:
        void operator()(server *x){
            destroy(x);
        }
    };
    // smart pointer for AoO server instance
    using pointer = std::unique_ptr<server, deleter>;

    // create a new AoO server instance
    static server * create(int port, uint32_t flags, aoo_error *err);

    // destroy the AoO server instance
    static void destroy(server *server);

    // run the AOO server; this function blocks indefinitely.
    virtual aoo_error run() = 0;

    // quit the AOO server from another thread
    virtual aoo_error quit() = 0;

    // set event handler callback + mode
    virtual aoo_error set_eventhandler(aoo_eventhandler fn,
                                       void *user, int32_t mode) = 0;

    // check for pending events (always thread safe)
    virtual aoo_bool events_available() = 0;

    // poll events (threadsafe, but not reentrant)
    // will call the event handler function one or more times
    virtual aoo_error poll_events() = 0;

    // server controls (threadsafe, but not reentrant)
    virtual aoo_error control(int32_t ctl, intptr_t index, void *ptr, size_t size) = 0;

    // ----------------------------------------------------------
    // type-safe convenience methods for frequently used controls

    // *none*
protected:
    ~server(){} // non-virtual!
};

inline server * server::create(int port, uint32_t flags, aoo_error *err){
    return aoo_net_server_new(port, flags, err);
}

inline void server::destroy(server *server){
    aoo_net_server_free(server);
}

/*//////////////////////// AoO client ///////////////////////*/

class client {
public:
    class deleter {
    public:
        void operator()(client *x){
            destroy(x);
        }
    };
    // smart pointer for AoO client instance
    using pointer = std::unique_ptr<client, deleter>;

    // create a new AoO client instance
    static client * create(const void *address, int32_t addrlen,
                           uint32_t flags);

    // destroy the AoO client instance
    static void destroy(client *client);

    // run the AOO client; this function blocks indefinitely.
    virtual aoo_error run() = 0;

    // quit the AOO client from another thread
    virtual aoo_error quit() = 0;

    // add AOO source
    virtual aoo_error add_source(aoo::source *src, aoo_id id) = 0;

    // remove AOO source
    virtual aoo_error remove_source(aoo::source *src) = 0;

    // add AOO sink
    virtual aoo_error add_sink(aoo::sink *src, aoo_id id) = 0;

    // remove AOO sink
    virtual aoo_error remove_sink(aoo::sink *src) = 0;

    // find peer by name and return its IP endpoint address
    // address: pointer to sockaddr_storage
    // addrlen: initialized with max. storage size, updated to actual size
    // info (optional): provide additional info
    // NOTE: if 'address' is NULL, we only check if the peer exists
    virtual aoo_error get_peer_address(const char *group, const char *user,
                                       void *address, int32_t *addrlen, uint32_t *flags) = 0;

    // find peer by its IP address and return additional info
    virtual aoo_error get_peer_info(const void *address, int32_t addrlen,
                                    aoo_net_peer_info *info) = 0;

    // send a request to the AOO server (always thread safe)
    virtual aoo_error send_request(aoo_net_request_type request, void *data,
                                   aoo_net_callback callback, void *user) = 0;

    // send a message to one or more peers
    // 'addr' + 'len' accept the following values:
    // a) 'struct sockaddr *' + 'socklen_t': send to a single peer
    // b) 'const char *' (group name) + 0: send to all peers of a specific group
    // c) 'NULL' + 0: send to all peers
    // the 'flags' parameter allows for (future) additional settings
    virtual aoo_error send_message(const char *data, int32_t n,
                                   const void *addr, int32_t len, int32_t flags) = 0;

    // handle messages from peers (thread safe, called from a network thread)
    // 'addr' should be sockaddr *
    virtual aoo_error handle_message(const char *data, int32_t n,
                                     const void *addr, int32_t len) = 0;

    // send outgoing messages (threadsafe, called from a network thread)
    virtual aoo_error send(aoo_sendfn fn, void *user) = 0;

    // set event handler callback + mode
    virtual aoo_error set_eventhandler(aoo_eventhandler fn,
                                       void *user, int32_t mode) = 0;

    // check for pending events (always thread safe)
    virtual aoo_bool events_available() = 0;

    // poll events (threadsafe, but not reentrant)
    // will call the event handler function one or more times
    virtual aoo_error poll_events() = 0;

    // client controls (threadsafe, but not reentrant)
    virtual aoo_error control(int32_t ctl, intptr_t index, void *ptr, size_t size) = 0;

    // ----------------------------------------------------------
    // type-safe convenience methods for frequently used controls

    // *none*

    // ----------------------------------------------------------
    // type-safe convenience methods for frequently used requests

    // connect to AOO server (always thread safe)
    aoo_error connect(const char *host, int port,
                      const char *name, const char *pwd,
                      aoo_net_callback cb, void *user)
    {
        aoo_net_connect_request data =  { host, port, name, pwd };
        return send_request(AOO_NET_CONNECT_REQUEST, &data, cb, user);
    }

    // disconnect from AOO server (always thread safe)
    aoo_error disconnect(aoo_net_callback cb, void *user)
    {
        return send_request(AOO_NET_DISCONNECT_REQUEST, NULL, cb, user);
    }

    // join an AOO group
    aoo_error join_group(const char *group, const char *pwd,
                         aoo_net_callback cb, void *user)
    {
        aoo_net_group_request data = { group, pwd };
        return send_request(AOO_NET_GROUP_JOIN_REQUEST, &data, cb, user);
    }

    // leave an AOO group
    aoo_error leave_group(const char *group, aoo_net_callback cb, void *user)
    {
        aoo_net_group_request data = { group, NULL };
        return send_request(AOO_NET_GROUP_LEAVE_REQUEST, &data, cb, user);
    }
protected:
    ~client(){} // non-virtual!
};

inline client * client::create(const void *address, int32_t addrlen,
                               uint32_t flags)
{
    return aoo_net_client_new(address, addrlen, flags);
}

inline void client::destroy(client *client){
    aoo_net_client_free(client);
}

} // net
} // aoo
