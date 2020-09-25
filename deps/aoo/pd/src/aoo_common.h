/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others. 
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#pragma once

#include "m_pd.h"

#include "aoo_net.h"

// hack for pd-lib-builder
#ifdef AOO_BUILD
#undef AOO_BUILD
#endif

#include "aoo/aoo.h"

#ifndef _WIN32
#include <pthread.h>
#endif

// setup function
#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#elif __GNUC__ >= 4
#define EXPORT __attribute__((visibility("default")))
#else
#define EXPORT
#endif

#define classname(x) class_getname(*(t_pd *)x)

/*///////////////////////////// aoo_node /////////////////////////////*/

typedef struct _aoo_node t_aoo_node;

int aoo_node_socket(t_aoo_node *node);

int aoo_node_port(t_aoo_node *node);

int32_t aoo_node_sendto(t_aoo_node *node, const char *buf, int32_t size,
                        const struct sockaddr *addr);

t_aoo_node * aoo_node_add(int port, t_pd *obj, int32_t id);

void aoo_node_release(t_aoo_node *node, t_pd *obj, int32_t id);

t_endpoint * aoo_node_endpoint(t_aoo_node * node,
                               const struct sockaddr_storage *sa, socklen_t len);

t_endpoint * aoo_node_find_peer(t_aoo_node *node, t_symbol *group, t_symbol *user);

void aoo_node_add_peer(t_aoo_node *node, t_symbol *group, t_symbol *user,
                       const struct sockaddr *sa, socklen_t len);

void aoo_node_remove_peer(t_aoo_node *node, t_symbol *group, t_symbol *user);

void aoo_node_remove_group(t_aoo_node *node, t_symbol *group);

void aoo_node_remove_all_peers(t_aoo_node *node);

void aoo_node_notify(t_aoo_node *node);

/*///////////////////////////// aoo_lock /////////////////////////////*/

#ifdef _WIN32
typedef void * aoo_lock;
#else
typedef pthread_rwlock_t aoo_lock;
#endif

void aoo_lock_init(aoo_lock *x);

void aoo_lock_destroy(aoo_lock *x);

void aoo_lock_lock(aoo_lock *x);

void aoo_lock_lock_shared(aoo_lock *x);

void aoo_lock_unlock(aoo_lock *x);

void aoo_lock_unlock_shared(aoo_lock *x);

/*///////////////////////////// helper functions ///////////////////////////////*/

int aoo_endpoint_to_atoms(const t_endpoint *e, int32_t id, t_atom *argv);

int aoo_getsinkarg(void *x, t_aoo_node *node, int argc, t_atom *argv,
                   struct sockaddr_storage *sa, socklen_t *len, int32_t *id);

int aoo_getsourcearg(void *x, t_aoo_node *node, int argc, t_atom *argv,
                     struct sockaddr_storage *sa, socklen_t *len, int32_t *id);

int aoo_parseresend(void *x, int argc, const t_atom *argv,
                    int32_t *limit, int32_t *interval,
                    int32_t *maxnumframes);

void aoo_format_makedefault(aoo_format_storage *f, int nchannels);

int aoo_format_parse(void *x, aoo_format_storage *f, int argc, t_atom *argv);

int aoo_format_toatoms(const aoo_format *f, int argc, t_atom *argv);
