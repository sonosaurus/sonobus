/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others. 
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "aoo/codec/aoo_pcm.h"
#include "common/utils.hpp"

#include <cassert>
#include <cstring>

namespace {

aoo_allocator g_allocator {
    [](size_t n, void *){ return operator new(n); },
    nullptr,
    [](void *ptr, size_t, void *){ operator delete(ptr); },
    nullptr
};

// conversion routines between aoo_sample and PCM data
union convert {
    int8_t b[8];
    int16_t i16;
    int32_t i32;
    int64_t i64;
    float f;
    double d;
};

int32_t bytes_per_sample(int32_t bd)
{
    switch (bd){
    case AOO_PCM_INT16:
        return 2;
    case AOO_PCM_INT24:
        return 3;
    case AOO_PCM_FLOAT32:
        return 4;
    case AOO_PCM_FLOAT64:
        return 8;
    default:
        assert(false);
        return 0;
    }
}

void sample_to_int16(aoo_sample in, char *out)
{
    convert c;
    int32_t temp = in * 0x7fff + 0.5f;
    c.i16 = (temp > INT16_MAX) ? INT16_MAX : (temp < INT16_MIN) ? INT16_MIN : temp;
#if BYTE_ORDER == BIG_ENDIAN
    memcpy(out, c.b, 2); // optimized away
#else
    out[0] = c.b[1];
    out[1] = c.b[0];
#endif
}

void sample_to_int24(aoo_sample in, char *out)
{
    convert c;
    int32_t temp = in * 0x7fffffff + 0.5f;
    c.i32 = (temp > INT32_MAX) ? INT32_MAX : (temp < INT32_MIN) ? INT32_MIN : temp;
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

void sample_to_float32(aoo_sample in, char *out)
{
    aoo::to_bytes<float>(in, out);
}

void sample_to_float64(aoo_sample in, char *out)
{
    aoo::to_bytes<double>(in, out);
}

aoo_sample int16_to_sample(const char *in){
    convert c;
#if BYTE_ORDER == BIG_ENDIAN
    memcpy(c.b, in, 2); // optimized away
#else
    c.b[0] = in[1];
    c.b[1] = in[0];
#endif
    return(aoo_sample)c.i16 / 32768.f;
}

aoo_sample int24_to_sample(const char *in)
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
    return (aoo_sample)c.i32 / 0x7fffffff;
}

aoo_sample float32_to_sample(const char *in)
{
    return aoo::from_bytes<float>(in);
}

aoo_sample float64_to_sample(const char *in)
{
    return aoo::from_bytes<double>(in);
}

void print_settings(const aoo_format_pcm& f)
{
    LOG_VERBOSE("PCM settings: "
                << "nchannels = " << f.header.nchannels
                << ", blocksize = " << f.header.blocksize
                << ", samplerate = " << f.header.samplerate
                << ", bitdepth = " << bytes_per_sample(f.bitdepth));
}

/*//////////////////// codec //////////////////////////*/

struct codec {
    codec(){
        memset(&format, 0, sizeof(aoo_format_pcm));
    }
    aoo_format_pcm format;
};

void validate_format(aoo_format_pcm& f, bool loud = true)
{
    f.header.codec = AOO_CODEC_PCM; // static string!
    f.header.size = sizeof(aoo_format_pcm); // actual size!

    // validate blocksize
    if (f.header.blocksize <= 0){
        if (loud){
            LOG_WARNING("PCM: bad blocksize " << f.header.blocksize
                        << ", using 64 samples");
        }
        f.header.blocksize = 64;
    }
    // validate samplerate
    if (f.header.samplerate <= 0){
        if (loud){
            LOG_WARNING("PCM: bad samplerate " << f.header.samplerate
                        << ", using 44100");
        }
        f.header.samplerate = 44100;
    }
    // validate channels
    if (f.header.nchannels <= 0 || f.header.nchannels > 255){
        if (loud){
            LOG_WARNING("PCM: bad channel count " << f.header.nchannels
                        << ", using 1 channel");
        }
        f.header.nchannels = 1;
    }
    // validate bitdepth
    if (f.bitdepth < 0 || f.bitdepth > AOO_PCM_BITDEPTH_SIZE){
        if (loud){
            LOG_WARNING("PCM: bad bitdepth, using 32bit float");
        }
        f.bitdepth = AOO_PCM_FLOAT32;
    }
}

aoo_error compare(codec *c, const aoo_format_pcm *fmt)
{
    // copy and validate!
    aoo_format_pcm f1;
    memcpy(&f1, fmt, sizeof(aoo_format_pcm));

    auto& f2 = c->format;
    auto& h1 = f1.header;
    auto& h2 = f2.header;

    // check before validate()!
    if (strcmp(h1.codec, h2.codec) ||
            h1.size != h2.size) {
        return false;
    }

    validate_format(f1, false);

    return h1.blocksize == h2.blocksize &&
            h1.samplerate == h2.samplerate &&
            h1.nchannels == h2.nchannels &&
            f1.bitdepth == f2.bitdepth;
}

aoo_error set_format(codec *c, aoo_format_pcm *fmt)
{
    if (strcmp(fmt->header.codec, AOO_CODEC_PCM)){
        return AOO_ERROR_UNSPECIFIED;
    }
    if (fmt->header.size < sizeof(aoo_format_pcm)){
        return AOO_ERROR_UNSPECIFIED;
    }

    validate_format(*fmt);

    // save and print settings
    memcpy(&c->format, fmt, sizeof(aoo_format_pcm));
    print_settings(c->format);

    return AOO_OK;
}

aoo_error get_format(codec *c, aoo_format *f, size_t size)
{
    // check if format has been set
    if (c->format.header.codec){
        if (size >= c->format.header.size){
            memcpy(f, &c->format, sizeof(aoo_format_pcm));
            return AOO_OK;
        } else {
            return AOO_ERROR_UNSPECIFIED;
        }
    } else {
        return AOO_ERROR_UNSPECIFIED;
    }
}

aoo_error pcm_ctl(void *x, int32_t ctl, void *ptr, int32_t size){
    switch (ctl){
    case AOO_CODEC_SET_FORMAT:
        assert(size >= sizeof(aoo_format));
        return set_format((codec *)x, (aoo_format_pcm *)ptr);
    case AOO_CODEC_GET_FORMAT:
        return get_format((codec *)x, (aoo_format *)ptr, size);
    case AOO_CODEC_RESET:
        // no op
        return AOO_OK;
    case AOO_CODEC_FORMAT_EQUAL:
        assert(size >= sizeof(aoo_format));
        return compare((codec *)x, (aoo_format_pcm *)ptr);
    default:
        LOG_WARNING("PCM: unsupported codec ctl " << ctl);
        return AOO_ERROR_UNSPECIFIED;
    }
}

void *encoder_new(){
    auto obj = g_allocator.alloc(sizeof(codec), g_allocator.context);
    new (obj) codec {};
    return obj;
}

void encoder_free(void *enc){
    static_cast<codec *>(enc)->~codec();
    g_allocator.free(enc, sizeof(codec), g_allocator.context);
}

aoo_error encode(void *enc,
                 const aoo_sample *s, int32_t n,
                 char *buf, int32_t *size)
{
    auto bitdepth = static_cast<codec *>(enc)->format.bitdepth;
    auto samplesize = bytes_per_sample(bitdepth);
    auto nbytes = samplesize * n;

    if (*size < nbytes){
        LOG_WARNING("PCM: size mismatch! input bytes: "
                    << nbytes << ", output bytes " << *size);
        return AOO_ERROR_UNSPECIFIED;
    }

    auto samples_to_blob = [&](auto fn){
        auto b = buf;
        for (int i = 0; i < n; ++i){
            fn(s[i], b);
            b += samplesize;
        }
    };

    switch (bitdepth){
    case AOO_PCM_INT16:
        samples_to_blob(sample_to_int16);
        break;
    case AOO_PCM_INT24:
        samples_to_blob(sample_to_int24);
        break;
    case AOO_PCM_FLOAT32:
        samples_to_blob(sample_to_float32);
        break;
    case AOO_PCM_FLOAT64:
        samples_to_blob(sample_to_float64);
        break;
    default:
        // unknown bitdepth
        break;
    }

    *size = nbytes;

    return AOO_OK;
}

void *decoder_new(){
    return new codec;
}

void decoder_free(void *dec){
    delete (codec *)dec;
}

aoo_error decode(void *dec,
                 const char *buf, int32_t size,
                 aoo_sample *s, int32_t *n)
{
    auto c = static_cast<codec *>(dec);
    assert(c->format.header.blocksize != 0);

    if (!buf){
        for (int i = 0; i < *n; ++i){
            s[i] = 0;
        }
        return AOO_OK; // dropped block
    }

    auto samplesize = bytes_per_sample(c->format.bitdepth);
    auto nsamples = size / samplesize;

    if (*n < nsamples){
        LOG_WARNING("PCM: size mismatch! input samples: "
                    << nsamples << ", output samples " << *n);
        return AOO_ERROR_UNSPECIFIED;
    }

    auto blob_to_samples = [&](auto convfn){
        auto b = buf;
        for (int i = 0; i < *n; ++i, b += samplesize){
            s[i] = convfn(b);
        }
    };

    switch (c->format.bitdepth){
    case AOO_PCM_INT16:
        blob_to_samples(int16_to_sample);
        break;
    case AOO_PCM_INT24:
        blob_to_samples(int24_to_sample);
        break;
    case AOO_PCM_FLOAT32:
        blob_to_samples(float32_to_sample);
        break;
    case AOO_PCM_FLOAT64:
        blob_to_samples(float64_to_sample);
        break;
    default:
        // unknown bitdepth
        return AOO_ERROR_UNSPECIFIED;
    }

    *n = nsamples;

    return AOO_OK;
}

aoo_error serialize(const aoo_format *f, char *buf, int32_t *size)
{
    if (*size >= 4){
        auto fmt = (const aoo_format_pcm *)f;
        aoo::to_bytes<int32_t>(fmt->bitdepth, buf);
        *size = 4;

        return AOO_OK;
    } else {
        LOG_ERROR("PCM: couldn't write settings - buffer too small!");
        return AOO_ERROR_UNSPECIFIED;
    }
}

aoo_error deserialize(const aoo_format *header, const char *buf,
                      int32_t nbytes, aoo_format *f, int32_t size)
{
    if (nbytes < 4){
        LOG_ERROR("PCM: couldn't read format - not enough data!");
        return AOO_ERROR_UNSPECIFIED;
    }
    if (size < sizeof(aoo_format_pcm)){
        LOG_ERROR("PCM: output format storage too small");
        return AOO_ERROR_UNSPECIFIED;
    }
    auto fmt = (aoo_format_pcm *)f;
    // header
    fmt->header.codec = AOO_CODEC_PCM; // static string!
    fmt->header.size = sizeof(aoo_format_pcm); // actual size!
    fmt->header.blocksize = header->blocksize;
    fmt->header.nchannels = header->nchannels;
    fmt->header.samplerate = header->samplerate;
    // options
    fmt->bitdepth = (aoo_pcm_bitdepth)aoo::from_bytes<int32_t>(buf);

    return AOO_OK;
}

aoo_codec codec_class = {
    AOO_CODEC_PCM,
    // encoder
    encoder_new,
    encoder_free,
    pcm_ctl,
    encode,
    // decoder
    decoder_new,
    decoder_free,
    pcm_ctl,
    decode,
    // helper
    serialize,
    deserialize
};

} // namespace

void aoo_codec_pcm_setup(aoo_codec_registerfn fn, const aoo_allocator *alloc){
    if (alloc){
        g_allocator = *alloc;
    }
    fn(AOO_CODEC_PCM, &codec_class);
}

