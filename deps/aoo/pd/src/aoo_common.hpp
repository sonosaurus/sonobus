/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#pragma once

#include "m_pd.h"

#include "aoo/aoo.hpp"
#include "aoo/aoo_net.hpp"

#include "common/net_utils.hpp"

#define classname(x) class_getname(*(t_pd *)x)

namespace aoo {

/*///////////////////////////// OSC time ///////////////////////////////*/

uint64_t get_osctime();

struct t_dejitter;

t_dejitter *get_dejitter();

uint64_t get_osctime_dejitter(t_dejitter *context);

/*///////////////////////////// aoo_node /////////////////////////////*/

class t_node {
public:
    static t_node * get(t_pd *obj, int port, void *x = nullptr, aoo_id id = 0);

    virtual ~t_node() {}

    virtual void release(t_pd *obj, void *x = nullptr) = 0;

    virtual aoo::net::client * client() = 0;

    virtual int port() const = 0;

    virtual ip_address::ip_type type() const = 0;

    virtual void notify() = 0;

    virtual void lock() = 0;

    virtual void unlock() = 0;

    virtual bool get_source_arg(t_pd *x, int argc, const t_atom *argv,
                                ip_address& addr, aoo_id &id) const = 0;

    virtual bool get_sink_arg(t_pd *x, int argc, const t_atom *argv,
                              ip_address& addr, uint32_t& flags, aoo_id &id) const = 0;

    virtual bool get_peer_arg(t_pd *x, int argc, const t_atom *argv,
                              ip_address& addr) const = 0;

    virtual int resolve_endpoint(const ip_address& addr, aoo_id id,
                                 int argc, t_atom *argv) const = 0;
};

/*///////////////////////////// helper functions ///////////////////////////////*/

int address_to_atoms(const ip_address& addr, int argc, t_atom *argv);

int endpoint_to_atoms(const ip_address& addr, aoo_id id, int argc, t_atom *argv);

void format_makedefault(aoo_format_storage &f, int nchannels);

bool format_parse(t_pd *x, aoo_format_storage &f, int argc, t_atom *argv,
                  int maxnumchannels);

int format_to_atoms(const aoo_format &f, int argc, t_atom *argv);

} // aoo
