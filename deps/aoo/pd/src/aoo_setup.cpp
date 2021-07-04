/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others. 
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "aoo_common.hpp"

#include "common/time.hpp"

#include "string.h"
#include "stdlib.h"

// setup function
#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#elif __GNUC__ >= 4
#define EXPORT __attribute__((visibility("default")))
#else
#define EXPORT
#endif

#define DEJITTER_THRESHOLD 0.75

#define DEJITTER_DEBUG 0

namespace aoo {

t_class *dejitter_class;

struct t_dejitter
{
    t_dejitter();
    ~t_dejitter();

    aoo::time_tag get_osctime(){
        update();
        return d_osctime_adjusted;
    }
private:
    t_pd d_header;

    t_clock *d_clock;
    aoo::time_tag d_last_osctime;
    aoo::time_tag d_osctime_adjusted;
    double d_last_clocktime = -1;
    double d_jitter_offset = 0;

    void update();

    static void tick(t_dejitter *x){
        x->update();
        clock_delay(x->d_clock, 1); // once per DSP tick
    }
};

t_dejitter::t_dejitter()
    : d_header(dejitter_class)
{
    d_clock = clock_new(this, (t_method)tick);
    clock_delay(d_clock, 0);
}

t_dejitter::~t_dejitter(){
    clock_free(d_clock);
}

// This is called once per DSP tick!
void t_dejitter::update()
{
    auto now = clock_getlogicaltime();
    if (now != d_last_clocktime){
        aoo::time_tag osctime = aoo::get_osctime(); // global function!
        if (d_last_osctime.value()){
            auto oldtime = d_osctime_adjusted;
            auto elapsed = aoo::time_tag::duration(d_last_osctime, osctime);
            auto ticktime = (double)sys_getblksize() / (double)sys_getsr();
            if ((elapsed / ticktime) < DEJITTER_THRESHOLD){
                auto diff = ticktime - elapsed;
                d_jitter_offset += diff;
                d_osctime_adjusted = osctime
                        + aoo::time_tag::from_seconds(d_jitter_offset);
            } else {
                d_jitter_offset = 0;
                d_osctime_adjusted = osctime;
            }
            // never let time go backwards!
            if (d_osctime_adjusted < oldtime){
                d_osctime_adjusted = oldtime;
            }
        #if DEJITTER_DEBUG
            DO_LOG_DEBUG("time difference: " << (elapsed * 1000.0) << " ms");
            DO_LOG_DEBUG("jitter offset: " << (d_jitter_offset * 1000.0) << " ms");
            DO_LOG_DEBUG("adjusted OSC time: " << d_osctime_adjusted);
        #endif
        }
        d_last_osctime = osctime;
        d_last_clocktime = now;
    }
}

uint64_t get_osctime(){
    thread_local double lastclocktime = -1;
    thread_local aoo::time_tag osctime;

    auto now = clock_getlogicaltime();
    if (now != lastclocktime){
        osctime = aoo::time_tag::now();
        lastclocktime = now;
    }
    return osctime;
}

t_dejitter * get_dejitter(){
    auto dejitter = (t_dejitter *)pd_findbyclass(gensym("__dejitter"),
                                                 dejitter_class);
    if (!dejitter){
        bug("get_osctime_dejitter");
    }
    return dejitter;
}

uint64_t get_osctime_dejitter(t_dejitter *x){
    if (x){
        return x->get_osctime();
    } else {
        return get_osctime();
    }
}

} // aoo

static void aoo_dejitter_setup(){
    aoo::dejitter_class = class_new(gensym("aoo dejitter"), 0, 0,
                                    sizeof(aoo::t_dejitter), CLASS_PD, A_NULL);

    auto dejitter = new aoo::t_dejitter(); // leak
    pd_bind((t_pd *)dejitter, gensym("__dejitter"));
}

void aoo_send_tilde_setup(void);
void aoo_receive_tilde_setup(void);
void aoo_pack_tilde_setup(void);
void aoo_unpack_tilde_setup(void);
void aoo_route_setup(void);
void aoo_node_setup(void);
void aoo_server_setup(void);
void aoo_client_setup(void);

extern "C" EXPORT void aoo_setup(void)
{
    post("AOO (audio over OSC) %s", aoo_version_string());
    post("  (c) 2020 Christof Ressi, Winfried Ritsch, et al.");

    aoo_initialize();

    std::string msg;
    if (aoo::check_ntp_server(msg)){
        post("%s", msg.c_str());
    } else {
        error("%s", msg.c_str());
    }

    post("");

    aoo_dejitter_setup();
    aoo_node_setup();

    aoo_send_tilde_setup();
    aoo_receive_tilde_setup();
    aoo_pack_tilde_setup();
    aoo_unpack_tilde_setup();
    aoo_route_setup();
    aoo_server_setup();
    aoo_client_setup();
}
