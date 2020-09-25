/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others. 
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */


#include "aoo_common.h"
#include "aoo/aoo_pcm.h"
#if USE_CODEC_OPUS
#include "aoo/aoo_opus.h"
#endif

#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#ifdef _WIN32
#include <synchapi.h>
#endif

#define CLAMP(x, a, b) ((x) < (a) ? (a) : (x) > (b) ? (b) : (x))

/*///////////////////////////// aoo_lock /////////////////////////////*/

#ifdef _WIN32
void aoo_lock_init(aoo_lock *x)
{
    InitializeSRWLock((SRWLOCK *)x);
}

void aoo_lock_destroy(aoo_lock *x){}

void aoo_lock_lock(aoo_lock *x)
{
    AcquireSRWLockExclusive((SRWLOCK *)x);
}

void aoo_lock_lock_shared(aoo_lock *x)
{
    AcquireSRWLockShared((SRWLOCK *)x);
}

void aoo_lock_unlock(aoo_lock *x)
{
    ReleaseSRWLockExclusive((SRWLOCK *)x);
}

void aoo_lock_unlock_shared(aoo_lock *x){
    ReleaseSRWLockShared((SRWLOCK *)x);
}
#else
void aoo_lock_init(aoo_lock *x)
{
    pthread_rwlock_init(x, 0);
}

void aoo_lock_destroy(aoo_lock *x)
{
    pthread_rwlock_destroy(x);
}

void aoo_lock_lock(aoo_lock *x)
{
    pthread_rwlock_wrlock(x);
}

void aoo_lock_lock_shared(aoo_lock *x)
{
    pthread_rwlock_rdlock(x);
}

void aoo_lock_unlock(aoo_lock *x)
{
    pthread_rwlock_unlock(x);
}

void aoo_lock_unlock_shared(aoo_lock *x)
{
    pthread_rwlock_unlock(x);
}
#endif

/*/////////////////////////// helper functions ///////////////////////////////////////*/

int32_t aoo_endpoint_to_atoms(const t_endpoint *e, int32_t id, t_atom *argv)
{
    t_symbol *host;
    int port;
    if (endpoint_getaddress(e, &host, &port)){
        SETSYMBOL(argv, host);
        SETFLOAT(argv + 1, port);
        if (id == AOO_ID_WILDCARD){
            SETSYMBOL(argv + 2, gensym("*"));
        } else {
            SETFLOAT(argv + 2, id);
        }
        return 1;
    }
    return 0;
}

static int aoo_getendpointarg(void *x, t_aoo_node *node, int argc, t_atom *argv,
                              struct sockaddr_storage *sa, socklen_t *len,
                              int32_t *id, const char *what)
{
    if (argc < 3){
        pd_error(x, "%s: too few arguments for %s", classname(x), what);
        return 0;
    }

    // first try peer (group|user)
    if (argv[1].a_type == A_SYMBOL){
        t_endpoint *e = 0;
        t_symbol *group = atom_getsymbol(argv);
        t_symbol *user = atom_getsymbol(argv + 1);

        e = aoo_node_find_peer(node, group, user);

        if (!e){
            pd_error(x, "%s: couldn't find peer %s|%s for %s",
                     classname(x), group->s_name, user->s_name, what);
            return 0;
        }

        // success - copy sockaddr
        memcpy(sa, &e->addr, e->addrlen);
        *len = e->addrlen;
    } else {
        // otherwise try host|port
        t_symbol *host = atom_getsymbol(argv);
        int port = atom_getfloat(argv + 1);

        if (!socket_getaddr(host->s_name, port, sa, len)){
            pd_error(x, "%s: couldn't resolve hostname '%s' for %s",
                     classname(x), host->s_name, what);
            return 0;
        }
    }

    if (argv[2].a_type == A_SYMBOL){
        if (*argv[2].a_w.w_symbol->s_name == '*'){
            *id = AOO_ID_WILDCARD;
        } else {
            pd_error(x, "%s: bad %s ID '%s'!",
                     classname(x), what, argv[2].a_w.w_symbol->s_name);
            return 0;
        }
    } else {
        *id = atom_getfloat(argv + 2);
    }
    return 1;
}

int aoo_getsinkarg(void *x, t_aoo_node *node, int argc, t_atom *argv,
                   struct sockaddr_storage *sa, socklen_t *len, int32_t *id)
{
    return aoo_getendpointarg(x, node, argc, argv, sa, len, id, "sink");
}

int aoo_getsourcearg(void *x, t_aoo_node *node, int argc, t_atom *argv,
                     struct sockaddr_storage *sa, socklen_t *len, int32_t *id)
{
    return aoo_getendpointarg(x, node, argc, argv, sa, len, id, "source");
}

static int aoo_getarg(const char *name, void *x, int which,
                      int argc, const t_atom *argv, t_float *f, t_float def)
{
    if (argc > which){
        if (argv[which].a_type == A_SYMBOL){
            t_symbol *sym = argv[which].a_w.w_symbol;
            if (sym == gensym("auto")){
                *f = def;
            } else {
                pd_error(x, "%s: bad '%s' argument '%s'", classname(x), name, sym->s_name);
                return 0;
            }
        } else {
            *f = atom_getfloat(argv + which);
        }
    } else {
        *f = def;
    }
    return 1;
}

int aoo_parseresend(void *x, int argc, const t_atom *argv,
                    int32_t *limit, int32_t *interval,
                    int32_t *maxnumframes)
{
    t_float f;
    if (aoo_getarg("limit", x, 0, argc, argv, &f, AOO_RESEND_LIMIT)){
        *limit = f;
    } else {
        return 0;
    }
    if (aoo_getarg("interval", x, 1, argc, argv, &f, AOO_RESEND_INTERVAL)){
        *interval = f;
    } else {
        return 0;
    }
    if (aoo_getarg("maxnumframes", x, 2, argc, argv, &f, AOO_RESEND_MAXNUMFRAMES)){
        *maxnumframes = f;
    } else {
        return 0;
    }
    return 1;
}

void aoo_format_makedefault(aoo_format_storage *f, int nchannels)
{
    aoo_format_pcm *fmt = (aoo_format_pcm *)f;
    fmt->header.codec = AOO_CODEC_PCM;
    fmt->header.blocksize = 64;
    fmt->header.samplerate = sys_getsr();
    fmt->header.nchannels = nchannels;
    fmt->bitdepth = AOO_PCM_FLOAT32;
}

int aoo_format_parse(void *x, aoo_format_storage *f, int argc, t_atom *argv)
{
    t_symbol *codec = atom_getsymbolarg(0, argc, argv);
    f->header.blocksize = argc > 1 ? atom_getfloat(argv + 1) : 64;
    f->header.samplerate = argc > 2 ? atom_getfloat(argv + 2) : sys_getsr();
    // omit nchannels

    if (codec == gensym(AOO_CODEC_PCM)){
        aoo_format_pcm *fmt = (aoo_format_pcm *)f;
        fmt->header.codec = AOO_CODEC_PCM;

        int bitdepth = argc > 3 ? atom_getfloat(argv + 3) : 4;
        switch (bitdepth){
        case 2:
            fmt->bitdepth = AOO_PCM_INT16;
            break;
        case 3:
            fmt->bitdepth = AOO_PCM_INT24;
            break;
        case 0: // default
        case 4:
            fmt->bitdepth = AOO_PCM_FLOAT32;
            break;
        case 8:
            fmt->bitdepth = AOO_PCM_FLOAT64;
            break;
        default:
            pd_error(x, "%s: bad bitdepth argument %d", classname(x), bitdepth);
            return 0;
        }
    }
#if USE_CODEC_OPUS
    else if (codec == gensym(AOO_CODEC_OPUS)){
        aoo_format_opus *fmt = (aoo_format_opus *)f;
        fmt->header.codec = AOO_CODEC_OPUS;
        // bitrate ("auto", "max" or float)
        if (argc > 3){
            if (argv[3].a_type == A_SYMBOL){
                t_symbol *sym = argv[3].a_w.w_symbol;
                if (sym == gensym("auto")){
                    fmt->bitrate = OPUS_AUTO;
                } else if (sym == gensym("max")){
                    fmt->bitrate = OPUS_BITRATE_MAX;
                } else {
                    pd_error(x, "%s: bad bitrate argument '%s'", classname(x), sym->s_name);
                    return 0;
                }
            } else {
                int bitrate = atom_getfloat(argv + 3);
                if (bitrate > 0){
                    fmt->bitrate = bitrate;
                } else {
                    pd_error(x, "%s: bitrate argument %d out of range", classname(x), bitrate);
                    return 0;
                }
            }
        } else {
            fmt->bitrate = OPUS_AUTO;
        }
        // complexity ("auto" or 0-10)
        if (argc > 4){
            if (argv[4].a_type == A_SYMBOL){
                t_symbol *sym = argv[4].a_w.w_symbol;
                if (sym == gensym("auto")){
                    fmt->complexity = OPUS_AUTO;
                } else {
                    pd_error(x, "%s: bad complexity argument '%s'", classname(x), sym->s_name);
                    return 0;
                }
            } else {
                int complexity = atom_getfloat(argv + 4);
                if (complexity < 0 || complexity > 10){
                    pd_error(x, "%s: complexity value %d out of range", classname(x), complexity);
                    return 0;
                }
                fmt->complexity = complexity;
            }
        } else {
            fmt->complexity = OPUS_AUTO;
        }
        // signal type ("auto", "music", "voice")
        if (argc > 5){
            t_symbol *type = atom_getsymbol(argv + 5);
            if (type == gensym("auto")){
                fmt->signal_type = OPUS_AUTO;
            } else if (type == gensym("music")){
                fmt->signal_type = OPUS_SIGNAL_MUSIC;
            } else if (type == gensym("voice")){
                fmt->signal_type = OPUS_SIGNAL_VOICE;
            } else {
                pd_error(x,"%s: unsupported signal type '%s'",
                         classname(x), type->s_name);
                return 0;
            }
        } else {
            fmt->signal_type = OPUS_AUTO;
        }
    }
#endif
    else {
        pd_error(x, "%s: unknown codec '%s'", classname(x), codec->s_name);
        return 0;
    }
    return 1;
}

int aoo_format_toatoms(const aoo_format *f, int argc, t_atom *argv)
{
    if (argc < 3){
        error("aoo_format_toatoms: too few atoms!");
        return 0;
    }
    t_symbol *codec = gensym(f->codec);
    SETSYMBOL(argv, codec);
    SETFLOAT(argv + 1, f->blocksize);
    SETFLOAT(argv + 2, f->samplerate);
    // omit nchannels

    if (codec == gensym(AOO_CODEC_PCM)){
        // pcm <blocksize> <samplerate> <bitdepth>
        if (argc < 4){
            error("aoo_format_toatoms: too few atoms for pcm format!");
            return 0;
        }
        aoo_format_pcm *fmt = (aoo_format_pcm *)f;
        int nbits;
        switch (fmt->bitdepth){
        case AOO_PCM_INT16:
            nbits = 2;
            break;
        case AOO_PCM_INT24:
            nbits = 3;
            break;
        case AOO_PCM_FLOAT32:
            nbits = 4;
            break;
        case AOO_PCM_FLOAT64:
            nbits = 8;
            break;
        default:
            nbits = 0;
        }
        SETFLOAT(argv + 3, nbits);
        return 4;
    }
#if USE_CODEC_OPUS
    else if (codec == gensym(AOO_CODEC_OPUS)){
        // opus <blocksize> <samplerate> <bitrate> <complexity> <signaltype>
        if (argc < 6){
            error("aoo_format_toatoms: too few atoms for opus format!");
            return 0;
        }
        aoo_format_opus *fmt = (aoo_format_opus *)f;
    #if 0
        SETFLOAT(argv + 3, fmt->bitrate);
    #else
        // workaround for bug in opus_multistream_encoder (as of opus v1.3.2)
        // where OPUS_GET_BITRATE would always return OPUS_AUTO.
        // We have no chance to get the actual bitrate for "auto" and "max",
        // so we return the symbols instead.
        switch (fmt->bitrate){
        case OPUS_AUTO:
            SETSYMBOL(argv + 3, gensym("auto"));
            break;
        case OPUS_BITRATE_MAX:
            SETSYMBOL(argv + 3, gensym("max"));
            break;
        default:
            SETFLOAT(argv + 3, fmt->bitrate);
            break;
        }
    #endif
        SETFLOAT(argv + 4, fmt->complexity);
        t_symbol *signaltype;
        switch (fmt->signal_type){
        case OPUS_SIGNAL_MUSIC:
            signaltype = gensym("music");
            break;
        case OPUS_SIGNAL_VOICE:
            signaltype = gensym("voice");
            break;
        default:
            signaltype = gensym("auto");
            break;
        }
        SETSYMBOL(argv + 5, signaltype);
        return 6;
    }
#endif
    else {
        error("aoo_format_toatoms: unknown format %s!", codec->s_name);
    }
    return 0;
}
