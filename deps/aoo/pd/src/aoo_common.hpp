/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#pragma once

#include "m_pd.h"

#include "aoo/aoo.h"
#include "aoo/aoo_client.hpp"

#include "common/net_utils.hpp"
#include "common/priority_queue.hpp"

#define classname(x) class_getname(*(t_pd *)x)

/*///////////////////////////// OSC time ///////////////////////////////*/

uint64_t get_osctime();

struct t_dejitter;

t_dejitter *get_dejitter();

uint64_t get_osctime_dejitter(t_dejitter *context);

/*///////////////////////////// aoo_node /////////////////////////////*/

class t_node {
public:
    static t_node * get(t_pd *obj, int port, void *x = nullptr, AooId id = 0);

    virtual ~t_node() {}

    virtual void release(t_pd *obj, void *x = nullptr) = 0;

    virtual AooClient * client() = 0;

    virtual int port() const = 0;

    virtual aoo::ip_address::ip_type type() const = 0;

    virtual void notify() = 0;

    virtual void lock() = 0;

    virtual void unlock() = 0;

    virtual bool resolve(t_symbol *host, int port, aoo::ip_address& addr) const = 0;

    virtual bool find_peer(const aoo::ip_address& addr,
                           t_symbol *& group, t_symbol *& user) const = 0;

    virtual bool find_peer(t_symbol * group, t_symbol * user,
                           aoo::ip_address& addr) const = 0;

    virtual int serialize_endpoint(const aoo::ip_address& addr, AooId id,
                                   int argc, t_atom *argv) const = 0;
};

/*///////////////////////////// helper functions ///////////////////////////////*/

int address_to_atoms(const aoo::ip_address& addr, int argc, t_atom *argv);

int endpoint_to_atoms(const aoo::ip_address& addr, AooId id, int argc, t_atom *argv);

void format_makedefault(AooFormatStorage &f, int nchannels);

bool format_parse(t_pd *x, AooFormatStorage &f, int argc, t_atom *argv,
                  int maxnumchannels);

int format_to_atoms(const AooFormat &f, int argc, t_atom *argv);

bool atom_to_datatype(const t_atom &a, AooDataType& type, void *x);

void datatype_to_atom(AooDataType type, t_atom& a);

/*//////////////////////////// priority queue ////////////////////////////////*/

template<typename T>
struct t_queue_item {
    template<typename U>
    t_queue_item(U&& _data, double _time)
        : data(std::forward<U>(_data)), time(_time) {}
    T data;
    double time;
};

template<typename T>
bool operator> (const t_queue_item<T>& a, const t_queue_item<T>& b) {
    return a.time > b.time;
}

template<typename T>
using t_priority_queue = aoo::priority_queue<t_queue_item<T>, std::greater<t_queue_item<T>>>;
