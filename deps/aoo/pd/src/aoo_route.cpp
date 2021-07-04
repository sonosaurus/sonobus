/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others. 
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "aoo_common.hpp"

using namespace aoo;

static t_class *aoo_route_class;

struct t_desc {
    t_atom sel;
    t_outlet *outlet;
};

struct t_aoo_route
{
    t_aoo_route(int argc, t_atom *argv);

    t_object x_obj;
    int x_n = 0;
    std::unique_ptr<t_desc[]> x_vec;
    t_outlet *x_rejectout;
};

static void aoo_route_list(t_aoo_route *x, t_symbol *s, int argc, t_atom *argv)
{
    // copy address pattern string
    char buf[64];
    int count = 0;
    for (; count < argc && count < 63; ++count){
        char c = (int)atom_getfloat(argv + count);
        if (c){
            buf[count] = c;
        } else {
            break; // end of address pattern
        }
    }
    buf[count] = 0;

    bool success = false;

    // parse address pattern
    aoo_type type;
    aoo_id id;
    int32_t onset;
    if (aoo_parse_pattern(buf, count, &type, &id, &onset) == AOO_OK){
        t_symbol *sym = (type == AOO_TYPE_SOURCE) ? gensym("source") :
                        (type == AOO_TYPE_SINK) ? gensym("sink") : gensym("");

        for (int i = 0; i < x->x_n; ++i){
            auto& sel = x->x_vec[i].sel;
            if ((sel.a_type == A_SYMBOL && sel.a_w.w_symbol == sym) ||
                (sel.a_type == A_FLOAT && sel.a_w.w_float == id))
            {
                outlet_list(x->x_vec[i].outlet, s, argc, argv);
                success = true;
            }
        }
    }
    if (!success){
        outlet_list(x->x_rejectout, s, argc, argv);
    }
}

static void * aoo_route_new(t_symbol *s, int argc, t_atom *argv)
{
    void *x = pd_new(aoo_route_class);
    new (x) t_aoo_route(argc, argv);
    return x;
}

t_aoo_route::t_aoo_route(int argc, t_atom *argv)
{
    // no argument -> single selector
    x_n = (argc > 1) ? argc : 1;
    x_vec = std::make_unique<t_desc[]>(x_n);
    for (int i = 0; i < x_n; ++i){
        if (argc){
            x_vec[i].sel = argv[i];
        } else {
            SETFLOAT(&x_vec[i].sel, AOO_ID_NONE);
        }
        x_vec[i].outlet = outlet_new(&x_obj, 0);
    }
    // a single selector can be set via an inlet
    if (x_n == 1){
        if (argc && argv->a_type == A_SYMBOL){
            symbolinlet_new(&x_obj, &x_vec[0].sel.a_w.w_symbol);
        } else {
            floatinlet_new(&x_obj, &x_vec[0].sel.a_w.w_float);
        }
    }

    x_rejectout = outlet_new(&x_obj, 0);
}

static void aoo_route_free(t_aoo_route *x)
{
    x->~t_aoo_route();
}

void aoo_route_setup(void)
{
    aoo_route_class = class_new(gensym("aoo_route"), (t_newmethod)(void *)aoo_route_new,
        (t_method)aoo_route_free, sizeof(t_aoo_route), 0, A_GIMME, A_NULL);
    class_addlist(aoo_route_class, (t_method)aoo_route_list);
}
