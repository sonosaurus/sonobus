/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others. 
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "aoo_common.h"

#include <string.h>

static t_class *aoo_route_class;

typedef struct _aoo_route
{
    t_object x_obj;
    int x_n;
    t_outlet **x_outlets;
    t_atom *x_selectors;
    t_outlet *x_rejectout;
} t_aoo_route;

static void aoo_route_list(t_aoo_route *x, t_symbol *s, int argc, t_atom *argv)
{
    // copy address pattern string
    char buf[64];
    int i = 0;
    for (; i < argc && i < 63; ++i){
        char c = (int)atom_getfloat(argv + i);
        if (c){
            buf[i] = c;
        } else {
            break; // end of address pattern
        }
    }
    buf[i] = 0;

    // parse address pattern
    int32_t type, id;
    if (aoo_parse_pattern(buf, i, &type, &id)){
        int success = 0;
        t_symbol *sym = (type == AOO_TYPE_SOURCE) ? gensym("source") :
                        (type == AOO_TYPE_SINK) ? gensym("sink") : gensym("");

        for (int j = 0; j < x->x_n; ++j){
            t_atom *sel = &x->x_selectors[i];
            if ((sel->a_type == A_SYMBOL && sel->a_w.w_symbol == sym) ||
                (sel->a_type == A_FLOAT && (id == AOO_ID_WILDCARD || sel->a_w.w_float == id)))
            {
                outlet_list(x->x_outlets[j], s, argc, argv);
                success = 1;
            }
        }

        if (success){
            return;
        }
    }
    // reject
    outlet_list(x->x_rejectout, s, argc, argv);
}

static void * aoo_route_new(t_symbol *s, int argc, t_atom *argv)
{
    t_aoo_route *x = (t_aoo_route *)pd_new(aoo_route_class);

    x->x_n = (argc > 1) ? argc : 1;
    x->x_outlets = (t_outlet **)getbytes(sizeof(t_outlet *) * x->x_n);
    x->x_selectors = (t_atom *)getbytes(sizeof(t_atom) * x->x_n);
    for (int i = 0; i < x->x_n; ++i){
        x->x_outlets[i] = outlet_new(&x->x_obj, 0);
        x->x_selectors[i] = argv[i];
    }
    if (x->x_n == 1){
        if (argc && argv->a_type == A_SYMBOL){
            symbolinlet_new(&x->x_obj, &x->x_selectors->a_w.w_symbol);
            if (!argc){
                SETSYMBOL(x->x_selectors, &s_);
            }
        } else {
            floatinlet_new(&x->x_obj, &x->x_selectors->a_w.w_float);
            if (!argc){
                SETFLOAT(x->x_selectors, AOO_ID_NONE);
            }
        }
    }

    x->x_rejectout = outlet_new(&x->x_obj, 0);

    return x;
}

static void aoo_route_free(t_aoo_route *x)
{
    freebytes(x->x_outlets, sizeof(t_outlet *) * x->x_n);
    freebytes(x->x_selectors, sizeof(t_atom) * x->x_n);
}

void aoo_route_setup(void)
{
    aoo_route_class = class_new(gensym("aoo_route"), (t_newmethod)(void *)aoo_route_new,
        (t_method)aoo_route_free, sizeof(t_aoo_route), 0, A_GIMME, A_NULL);
    class_addlist(aoo_route_class, (t_method)aoo_route_list);
}
