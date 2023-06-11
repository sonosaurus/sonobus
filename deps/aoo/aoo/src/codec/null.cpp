/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "aoo/aoo_codec.h"
#include "aoo/codec/aoo_null.h"

#include "../detail.hpp"

#include "common/utils.hpp"

#include <cassert>
#include <cstring>
#include <cmath>

namespace {

void print_format(const AooFormatNull& f)
{
    LOG_VERBOSE("Null codec settings: "
                << "nchannels = " << f.header.numChannels
                << ", blocksize = " << f.header.blockSize
                << ", samplerate = " << f.header.sampleRate);
}

bool validate_format(AooFormatNull& f, bool loud = true)
{
    if (f.header.structSize < AOO_STRUCT_SIZE(AooFormatNull, header)) {
        return false;
    }

    if (strcmp(f.header.codecName, kAooCodecNull)){
        return false;
    }

    // validate block size
    if (f.header.blockSize <= 0){
        if (loud){
            LOG_WARNING("Null codec: bad blocksize " << f.header.blockSize
                        << ", using 64 samples");
        }
        f.header.blockSize = 64;
    }
    // validate sample rate
    if (f.header.sampleRate <= 0){
        if (loud){
            LOG_WARNING("Null codec: bad samplerate " << f.header.sampleRate
                        << ", using 44100");
        }
        f.header.sampleRate = 44100;
    }
#if 1
    // bash to single channel
    f.header.numChannels = 1;
#else
    // validate channels
    if (f.header.numChannels <= 0 || f.header.numChannels > 255){
        if (loud){
            LOG_WARNING("Null codec: bad channel count " << f.header.numChannels
                        << ", using 1 channel");
        }
        f.header.numChannels = 1;
    }
#endif

    return true;
}

//------------------- PCM codec -----------------------------//

struct NullCodec : AooCodec {
    NullCodec(const AooFormatNull& f);
};

AooCodec * AOO_CALL NullCodec_new(AooFormat *f, AooError *err){
    auto fmt = (AooFormatNull*)f;
    if (!validate_format(*fmt, true)){
        if (err) {
            *err = kAooErrorBadArgument;
        }
        return nullptr;
    }

    print_format(*fmt);

    if (err){
        *err = kAooOk;
    }

    return aoo::construct<NullCodec>(*fmt);
}

void AOO_CALL NullCodec_free(AooCodec *c){
    aoo::destroy(static_cast<NullCodec *>(c));
}

AooError AOO_CALL NullCodec_control(
        AooCodec *x, AooCtl ctl, void *ptr, AooSize size) {
    switch (ctl){
    case kAooCodecCtlReset:
        // no op
        return kAooOk;
    default:
        LOG_WARNING("Null codec: unsupported codec ctl " << ctl);
        return kAooErrorNotImplemented;
    }
}

AooError AOO_CALL NullCodec_encode(
        AooCodec *enc,const AooSample *s, AooInt32 n,
        AooByte *buf, AooInt32 *size)
{
    // do nothing
    *size = 0;

    return kAooOk;
}

AooError AOO_CALL NullCodec_decode(
        AooCodec *dec, const AooByte *buf, AooInt32 size,
        AooSample *s, AooInt32 *n)
{
    // just zero
    for (int i = 0; i < *n; ++i){
        s[i] = 0;
    }
    return kAooOk;
}

AooError AOO_CALL serialize(
        const AooFormat *f, AooByte *buf, AooInt32 *size)
{
    if (!size) {
        return kAooErrorBadArgument;
    }
    *size = 0;

    return kAooOk;
}

AooError AOO_CALL deserialize(
        const AooByte *buf, AooInt32 size, AooFormat *f, AooInt32 *fmtsize)
{
    if (!fmtsize) {
        return kAooErrorBadArgument;
    }
    *fmtsize = sizeof(AooFormat);

    return kAooOk;
}

AooCodecInterface g_interface = {
    sizeof(AooCodecInterface),
    // encoder
    NullCodec_new,
    NullCodec_free,
    NullCodec_control,
    NullCodec_encode,
    // decoder
    NullCodec_new,
    NullCodec_free,
    NullCodec_control,
    NullCodec_decode,
    // helper
    serialize,
    deserialize
};

NullCodec::NullCodec(const AooFormatNull& f) {
    cls = &g_interface;
}

} // namespace

void aoo_nullLoad(const AooCodecHostInterface *iface) {
    iface->registerCodec(kAooCodecNull, &g_interface);
    // the dummy codec is always statically linked, so we can simply use the
    // internal log function and allocator
}

void aoo_nullUnload() {}
