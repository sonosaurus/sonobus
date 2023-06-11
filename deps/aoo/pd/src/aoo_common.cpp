/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others. 
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */


#include "aoo_common.hpp"

#include "aoo/codec/aoo_null.h"
#include "aoo/codec/aoo_pcm.h"
#if AOO_USE_CODEC_OPUS
#include "aoo/codec/aoo_opus.h"
#endif

#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#define CLAMP(x, a, b) ((x) < (a) ? (a) : (x) > (b) ? (b) : (x))

/*/////////////////////////// helper functions ///////////////////////////////////////*/

int address_to_atoms(const aoo::ip_address& addr, int argc, t_atom *a)
{
    if (argc < 2){
        return 0;
    }
    SETSYMBOL(a, gensym(addr.name()));
    SETFLOAT(a + 1, addr.port());
    return 2;
}

int endpoint_to_atoms(const aoo::ip_address& addr, AooId id, int argc, t_atom *argv)
{
    if (argc < 3 || !addr.valid()){
        return 0;
    }
    SETSYMBOL(argv, gensym(addr.name()));
    SETFLOAT(argv + 1, addr.port());
    SETFLOAT(argv + 2, id);
    return 3;
}

void format_makedefault(AooFormatStorage &f, int nchannels)
{
    AooFormatPcm_init((AooFormatPcm *)&f, nchannels,
                      sys_getsr(), 64, kAooPcmFloat32);
}

static int32_t format_getparam(void *x, int argc, t_atom *argv, int which,
                               const char *name, int32_t def)
{
    if (argc > which){
        if (argv[which].a_type == A_FLOAT){
            return argv[which].a_w.w_float;
        }
    #if 1
        t_symbol *s = atom_getsymbol(argv + which);
        if (s != gensym("_")){
            pd_error(x, "%s: bad %s argument (%s), using %d", classname(x), name, s->s_name, def);
        }
    #endif
    }
    return def;
}

bool format_parse(t_pd *x, AooFormatStorage &f, int argc, t_atom *argv, int maxnumchannels)
{
    t_symbol *codec = atom_getsymbolarg(0, argc, argv);

    if (codec == gensym(kAooCodecNull)){
        // null <channels> <blocksize> <samplerate>
        auto numchannels = format_getparam(x, argc, argv, 1, "channels", 1);
        auto blocksize = format_getparam(x, argc, argv, 2, "blocksize", 64);
        auto samplerate = format_getparam(x, argc, argv, 3, "samplerate", sys_getsr());

        AooFormatNull_init((AooFormatNull *)&f.header, numchannels, samplerate, blocksize);
    } else if (codec == gensym(kAooCodecPcm)){
        // pcm <channels> <blocksize> <samplerate> <bitdepth>
        auto numchannels = format_getparam(x, argc, argv, 1, "channels", maxnumchannels);
        auto blocksize = format_getparam(x, argc, argv, 2, "blocksize", 64);
        auto samplerate = format_getparam(x, argc, argv, 3, "samplerate", sys_getsr());

        auto nbits = format_getparam(x, argc, argv, 4, "bitdepth", 4);
        AooPcmBitDepth bitdepth;
        switch (nbits){
        case 1:
            bitdepth =  kAooPcmInt8;
            break;
        case 2:
            bitdepth = kAooPcmInt16;
            break;
        case 3:
            bitdepth = kAooPcmInt24;
            break;
        case 4:
            bitdepth = kAooPcmFloat32;
            break;
        case 8:
            bitdepth = kAooPcmFloat64;
            break;
        default:
            pd_error(x, "%s: bad bitdepth argument %d", classname(x), nbits);
            return false;
        }

        AooFormatPcm_init((AooFormatPcm *)&f.header, numchannels, samplerate, blocksize, bitdepth);
    }
#if AOO_USE_CODEC_OPUS
    else if (codec == gensym(kAooCodecOpus)){
        // opus <channels> <blocksize> <samplerate> <application>
        opus_int32 numchannels = format_getparam(x, argc, argv, 1, "channels", maxnumchannels);
        opus_int32 blocksize = format_getparam(x, argc, argv, 2, "blocksize", 480); // 10ms
        opus_int32 samplerate = format_getparam(x, argc, argv, 3, "samplerate", 48000);

        // application type ("auto", "audio", "voip", "lowdelay"
        opus_int32 applicationType;
        if (argc > 4){
            t_symbol *type = atom_getsymbol(argv + 4);
            if (type == gensym("_") || type == gensym("audio")){
                applicationType = OPUS_APPLICATION_AUDIO;
            } else if (type == gensym("voip")){
                applicationType = OPUS_APPLICATION_VOIP;
            } else if (type == gensym("lowdelay")){
                applicationType = OPUS_APPLICATION_RESTRICTED_LOWDELAY;
            } else {
                pd_error(x,"%s: unsupported application type '%s'",
                         classname(x), type->s_name);
                return false;
            }
        } else {
            applicationType = OPUS_APPLICATION_AUDIO;
        }

        AooFormatOpus_init((AooFormatOpus *)&f.header, numchannels,
                           samplerate, blocksize, applicationType);
    }
#endif
    else {
        pd_error(x, "%s: unknown codec '%s'", classname(x), codec->s_name);
        return false;
    }
    return true;
}

int format_to_atoms(const AooFormat &f, int argc, t_atom *argv)
{
    if (argc < 4){
        bug("format_to_atoms: too few atoms!");
        return 0;
    }
    t_symbol *codec = gensym(f.codecName);
    SETSYMBOL(argv, codec);
    SETFLOAT(argv + 1, f.numChannels);
    SETFLOAT(argv + 2, f.blockSize);
    SETFLOAT(argv + 3, f.sampleRate);

    if (codec == gensym(kAooCodecNull)){
        // null <channels> <blocksize> <samplerate>
        return 4;
    } else if (codec == gensym(kAooCodecPcm)){
        // pcm <channels> <blocksize> <samplerate> <bitdepth>
        if (argc < 5){
            bug("format_to_atoms: too few atoms for pcm format!");
            return 0;
        }
        auto& fmt = (AooFormatPcm &)f;
        int nbytes;
        switch (fmt.bitDepth){
        case kAooPcmInt8:
            nbytes = 1;
            break;
        case kAooPcmInt16:
            nbytes = 2;
            break;
        case kAooPcmInt24:
            nbytes = 3;
            break;
        case kAooPcmFloat32:
            nbytes = 4;
            break;
        case kAooPcmFloat64:
            nbytes = 8;
            break;
        default:
            pd_error(0, "format_to_atoms: bad bitdepth argument %d", fmt.bitDepth);
            return 0;
        }
        SETFLOAT(argv + 4, nbytes);
        return 5;
    }
#if AOO_USE_CODEC_OPUS
    else if (codec == gensym(kAooCodecOpus)){
        // opus <channels> <blocksize> <samplerate> <application>
        if (argc < 5){
            bug("format_to_atoms: too few atoms for opus format!");
            return 0;
        }

        auto& fmt = (AooFormatOpus &)f;

        // application type
        t_symbol *type;
        switch (fmt.applicationType){
        case OPUS_APPLICATION_VOIP:
            type = gensym("voip");
            break;
        case OPUS_APPLICATION_RESTRICTED_LOWDELAY:
            type = gensym("lowdelay");
            break;
        case OPUS_APPLICATION_AUDIO:
            type = gensym("audio");
            break;
        default:
            pd_error(0, "format_to_atoms: bad application type argument %d",
                  fmt.applicationType);
            return 0;
        }
        SETSYMBOL(argv + 4, type);

        return 5;
    }
#endif
    else {
        pd_error(0, "format_to_atoms: unknown format %s!", codec->s_name);
    }
    return 0;
}

bool atom_to_datatype(const t_atom &a, AooDataType& type, void *x) {
    if (a.a_type == A_SYMBOL) {
        auto str = a.a_w.w_symbol->s_name;
        auto result = aoo_dataTypeFromString(str);
        if (result != kAooDataUnspecified) {
            type = result;
            return true;
        } else {
            pd_error(x, "%s: unknown data type '%s'", classname(x), str);
        }
    } else {
        pd_error(x, "%s: bad metadata type", classname(x));
    }
    return false;
}

void datatype_to_atom(AooDataType type, t_atom& a) {
    auto str = aoo_dataTypeToString(type);
    SETSYMBOL(&a, str ? gensym(str) : gensym("unknown"));
}
