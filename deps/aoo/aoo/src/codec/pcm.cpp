/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others. 
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "aoo/aoo_codec.h"
#include "aoo/codec/aoo_pcm.h"

#include "../detail.hpp"

#include "common/utils.hpp"

#include <cassert>
#include <cstring>
#include <cmath>

namespace {

//-------------------- helper functions -----------------------//

// conversion routines between AooSample and PCM data
union convert {
    int8_t b[8];
    int16_t i16;
    int32_t i32;
    int64_t i64;
    float f;
    double d;
};

int32_t bytes_per_sample(AooPcmBitDepth bitdepth)
{
    switch (bitdepth){
    case kAooPcmInt8:
        return 1;
    case kAooPcmInt16:
        return 2;
    case kAooPcmInt24:
        return 3;
    case kAooPcmFloat32:
        return 4;
    case kAooPcmFloat64:
        return 8;
    default:
        // should have been validated!
        assert(false);
        return 0;
    }
}

void sample_to_int8(AooSample in, AooByte *out)
{
    // convert to 8 bit range
    AooSample temp = std::rint(in * INT8_MAX);
    // check for overflow!
    if (temp > INT8_MAX){
        temp = INT8_MAX;
    } else if (temp < INT8_MIN){
        temp = INT8_MIN;
    }
    *out = (AooByte)(int8_t)temp;
}

void sample_to_int16(AooSample in, AooByte *out)
{
    convert c;
    // convert to 16 bit range
    AooSample temp = std::rint(in * INT16_MAX);
    // check for overflow!
    if (temp > INT16_MAX){
        temp = INT16_MAX;
    } else if (temp < INT16_MIN){
        temp = INT16_MIN;
    }
    c.i16 = (int16_t)temp;
#if BYTE_ORDER == BIG_ENDIAN
    memcpy(out, c.b, 2); // optimized away
#else
    out[0] = c.b[1];
    out[1] = c.b[0];
#endif
}

void sample_to_int24(AooSample in, AooByte *out)
{
    convert c;
    // convert to 32 bit range
    double temp = std::rint(in * (double)INT32_MAX);
    // check for overflow!
    if (temp > INT32_MAX){
        temp = INT32_MAX;
    } else if (temp < INT32_MIN){
        temp = INT32_MIN;
    }
    c.i32 = (int32_t)temp;
    // only copy the highest 3 bytes!
#if BYTE_ORDER == BIG_ENDIAN
    out[0] = c.b[0];
    out[1] = c.b[1];
    out[2] = c.b[2];
#else
    out[0] = c.b[3];
    out[1] = c.b[2];
    out[2] = c.b[1];
#endif
}

void sample_to_float32(AooSample in, AooByte *out)
{
    aoo::to_bytes<float>(in, out);
}

void sample_to_float64(AooSample in, AooByte *out)
{
    aoo::to_bytes<double>(in, out);
}

AooSample int8_to_sample(const AooByte *in){
    return (AooSample)(int8_t)(*in) / (AooSample)INT8_MAX;
}

AooSample int16_to_sample(const AooByte *in){
    convert c;
#if BYTE_ORDER == BIG_ENDIAN
    memcpy(c.b, in, 2); // optimized away
#else
    c.b[0] = in[1];
    c.b[1] = in[0];
#endif
    return (AooSample)c.i16 / (AooSample)INT16_MAX;
}

AooSample int24_to_sample(const AooByte *in)
{
    convert c;
    // copy to the highest 3 bytes!
#if BYTE_ORDER == BIG_ENDIAN
    c.b[0] = in[0];
    c.b[1] = in[1];
    c.b[2] = in[2];
    c.b[3] = 0;
#else
    c.b[0] = 0;
    c.b[1] = in[2];
    c.b[2] = in[1];
    c.b[3] = in[0];
#endif
    return (AooSample)c.i32 / (AooSample)INT32_MAX;
}

AooSample float32_to_sample(const AooByte *in)
{
    return aoo::from_bytes<float>(in);
}

AooSample float64_to_sample(const AooByte *in)
{
    return aoo::from_bytes<double>(in);
}

void print_format(const AooFormatPcm& f)
{
    LOG_VERBOSE("PCM settings: "
                << "nchannels = " << f.header.numChannels
                << ", blocksize = " << f.header.blockSize
                << ", samplerate = " << f.header.sampleRate
                << ", bitdepth = " << bytes_per_sample(f.bitDepth));
}

bool validate_format(AooFormatPcm& f, bool loud = true)
{
    if (f.header.structSize < AOO_STRUCT_SIZE(AooFormatPcm, bitDepth)) {
        return false;
    }

    if (strcmp(f.header.codecName, kAooCodecPcm)){
        return false;
    }

    // validate block size
    if (f.header.blockSize <= 0){
        if (loud){
            LOG_WARNING("PCM: bad blocksize " << f.header.blockSize
                        << ", using 64 samples");
        }
        f.header.blockSize = 64;
    }
    // validate sample rate
    if (f.header.sampleRate <= 0){
        if (loud){
            LOG_WARNING("PCM: bad samplerate " << f.header.sampleRate
                        << ", using 44100");
        }
        f.header.sampleRate = 44100;
    }
    // validate channels
    if (f.header.numChannels <= 0 || f.header.numChannels > 255){
        if (loud){
            LOG_WARNING("PCM: bad channel count " << f.header.numChannels
                        << ", using 1 channel");
        }
        f.header.numChannels = 1;
    }
    // validate bitdepth
    if (f.bitDepth < 0 || f.bitDepth > kAooPcmBitDepthSize){
        if (loud){
            LOG_WARNING("PCM: bad bit depth, using 32-bit float");
        }
        f.bitDepth = kAooPcmFloat32;
    }

    return true;
}

//------------------- PCM codec -----------------------------//

struct PcmCodec : AooCodec {
    PcmCodec(const AooFormatPcm& f);

    int sampleSize_;
};

AooCodec * AOO_CALL PcmCodec_new(AooFormat *f, AooError *err){
    auto fmt = (AooFormatPcm *)f;
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

    return aoo::construct<PcmCodec>(*fmt);
}

void AOO_CALL PcmCodec_free(AooCodec *c){
    aoo::destroy(static_cast<PcmCodec *>(c));
}

AooError AOO_CALL PcmCodec_control(
        AooCodec *x, AooCtl ctl, void *ptr, AooSize size) {
    switch (ctl){
    case kAooCodecCtlReset:
        // no op
        return kAooOk;
    default:
        LOG_WARNING("PCM: unsupported codec ctl " << ctl);
        return kAooErrorNotImplemented;
    }
}

AooError AOO_CALL PcmCodec_encode(
        AooCodec *enc,const AooSample *s, AooInt32 n,
        AooByte *buf, AooInt32 *size)
{
    auto samplesize = static_cast<PcmCodec *>(enc)->sampleSize_;
    auto nbytes = samplesize * n;

    if (*size < nbytes){
        LOG_WARNING("PCM: size mismatch! input bytes: "
                    << nbytes << ", output bytes " << *size);
        return kAooErrorInsufficientBuffer;
    }

    auto samples_to_bytes = [&](auto fn){
        auto b = buf;
        for (int i = 0; i < n; ++i){
            fn(s[i], b);
            b += samplesize;
        }
    };

    switch (samplesize){
    case 1:
        samples_to_bytes(sample_to_int8);
        break;
    case 2:
        samples_to_bytes(sample_to_int16);
        break;
    case 3:
        samples_to_bytes(sample_to_int24);
        break;
    case 4:
        samples_to_bytes(sample_to_float32);
        break;
    case 8:
        samples_to_bytes(sample_to_float64);
        break;
    default:
        // unknown bitdepth
        return kAooErrorBadArgument;
    }

    *size = nbytes;

    return kAooOk;
}

AooError AOO_CALL PcmCodec_decode(
        AooCodec *dec, const AooByte *buf, AooInt32 size,
        AooSample *s, AooInt32 *n)
{
    if (!buf){
        // dropped block, just zero
        for (int i = 0; i < *n; ++i){
            s[i] = 0;
        }
        return kAooOk;
    }

    auto samplesize = static_cast<PcmCodec *>(dec)->sampleSize_;
    auto nsamples = size / samplesize;

    if (*n < nsamples){
        LOG_WARNING("PCM: size mismatch! input samples: "
                    << nsamples << ", output samples " << *n);
        return kAooErrorInsufficientBuffer;
    }

    auto blob_to_samples = [&](auto convfn){
        auto b = buf;
        for (int i = 0; i < *n; ++i, b += samplesize){
            s[i] = convfn(b);
        }
    };

    switch (samplesize){
    case 1:
        blob_to_samples(int8_to_sample);
        break;
    case 2:
        blob_to_samples(int16_to_sample);
        break;
    case 3:
        blob_to_samples(int24_to_sample);
        break;
    case 4:
        blob_to_samples(float32_to_sample);
        break;
    case 8:
        blob_to_samples(float64_to_sample);
        break;
    default:
        // unknown bitdepth
        return kAooErrorBadArgument;
    }

    *n = nsamples;

    return kAooOk;
}

AooError AOO_CALL serialize(
        const AooFormat *f, AooByte *buf, AooInt32 *size)
{
    if (!size) {
        return kAooErrorBadArgument;
    }
    if (!buf){
        *size = sizeof(AooInt32);
        return kAooOk;
    }
    if (*size < sizeof(AooInt32)) {
        LOG_ERROR("PCM: couldn't write settings - buffer too small!");
        return kAooErrorInsufficientBuffer;
    }
    if (!AOO_CHECK_FIELD(f, AooFormatPcm, bitDepth)) {
        LOG_ERROR("PCM: bad format struct size");
        return kAooErrorBadArgument;
    }

    auto fmt = (const AooFormatPcm *)f;
    aoo::to_bytes<AooInt32>(fmt->bitDepth, buf);
    *size = sizeof(AooInt32);

    return kAooOk;
}

AooError AOO_CALL deserialize(
        const AooByte *buf, AooInt32 size, AooFormat *f, AooInt32 *fmtsize)
{
    if (!fmtsize) {
        return kAooErrorBadArgument;
    }
    if (!f) {
        *fmtsize = AOO_STRUCT_SIZE(AooFormatPcm, bitDepth);
        return kAooOk;
    }
    if (size < sizeof(AooInt32)) {
        LOG_ERROR("PCM: couldn't read format - not enough data!");
        return kAooErrorBadArgument;
    }
    if (*fmtsize < AOO_STRUCT_SIZE(AooFormatPcm, bitDepth)) {
        LOG_ERROR("PCM: output format storage too small");
        return kAooErrorBadArgument;
    }
    auto fmt = (AooFormatPcm *)f;
    fmt->bitDepth = aoo::from_bytes<AooInt32>(buf);
    *fmtsize = AOO_STRUCT_SIZE(AooFormatPcm, bitDepth);

    return kAooOk;
}

AooCodecInterface g_interface = {
    sizeof(AooCodecInterface),
    // encoder
    PcmCodec_new,
    PcmCodec_free,
    PcmCodec_control,
    PcmCodec_encode,
    // decoder
    PcmCodec_new,
    PcmCodec_free,
    PcmCodec_control,
    PcmCodec_decode,
    // helper
    serialize,
    deserialize
};

PcmCodec::PcmCodec(const AooFormatPcm& f) {
    cls = &g_interface;
    sampleSize_ = bytes_per_sample(f.bitDepth);
}

} // namespace

void aoo_pcmLoad(const AooCodecHostInterface *iface) {
    iface->registerCodec(kAooCodecPcm, &g_interface);
    // the PCM codec is always statically linked, so we can simply use the
    // internal log function and allocator
}

void aoo_pcmUnload() {}
