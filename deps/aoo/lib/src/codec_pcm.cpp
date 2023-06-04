/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others. 
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "aoo/aoo_pcm.h"
#include "aoo/aoo_utils.hpp"

#include <cassert>
#include <cstring>

namespace {

// Q_8BIt is a constant required for the 8-bit dithered noise shaping
// it's the pre-calculation of 1 / 256 and represents the "step size" of the quantized data for an 8-bit resolution
const float Q_8BIT = 0.00390625f;
// constants for the quantization error tracking ring-buffer
const int ERROR_BUF_MASK = 15;
const int ERROR_BUF_SIZE = 16;
// example coefficients for a FIR filter of size 9 from "Minimally Audible Noise Shaping" by Lipshitz 1991
const float SHAPED_BS[] = { 2.847f, -4.685f, 6.214f, -7.184f, 6.639f, -5.032f, 3.263f, -1.632f, 0.4191f};

// returns a random number from 0.f to 1.f
float get_random_number() {
    return static_cast<float> (rand()) / static_cast<float> (RAND_MAX);
}

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
    case AOO_PCM_INT8:
        return 1;
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

// converts the sample to an 8-bit value including noise shaped dithering
// it's based on the implementations from Ardour and from Audacity and uses
// methods from "Minimally Audible Noise Shaping" by Lipshitz from 1991
void sample_to_int8(aoo_sample in, char *out, int *mPhase, float *errorBuffer)
{
    // add shaped noise with a FIR filter function based on recent errors
    in += errorBuffer[*mPhase] * SHAPED_BS[0]
        + errorBuffer[(*mPhase - 1) & ERROR_BUF_MASK] * SHAPED_BS[1]
        + errorBuffer[(*mPhase - 2) & ERROR_BUF_MASK] * SHAPED_BS[2]
        + errorBuffer[(*mPhase - 3) & ERROR_BUF_MASK] * SHAPED_BS[3]
        + errorBuffer[(*mPhase - 4) & ERROR_BUF_MASK] * SHAPED_BS[4]
        + errorBuffer[(*mPhase - 5) & ERROR_BUF_MASK] * SHAPED_BS[5]
        + errorBuffer[(*mPhase - 6) & ERROR_BUF_MASK] * SHAPED_BS[6]
        + errorBuffer[(*mPhase - 7) & ERROR_BUF_MASK] * SHAPED_BS[7]
        + errorBuffer[(*mPhase - 8) & ERROR_BUF_MASK] * SHAPED_BS[8];

    *mPhase = (*mPhase + 1) & ERROR_BUF_MASK;
    // save input value before quantization
    float pre_quantization = in;

    // add two linear probability density function (LPDF) values together
    // to create a triangular probability density function (TPDF)
    in += Q_8BIT * (get_random_number() + get_random_number() - 1.0f);

    // quantize
    int32_t temp = in * 0x80;
    // save quantization error
    errorBuffer[*mPhase] = pre_quantization - temp / 128.0f;

    out[0] = (temp > INT8_MAX) ? INT8_MAX : (temp < INT8_MIN) ? INT8_MIN : temp;
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

aoo_sample int8_to_sample(const char *in)
{
    return(aoo_sample)in[0] / 128.f;
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

// the encoder requires a state for the noise shaping per channel, the decoder is simpler and doesn't need any states
struct encoder : codec {
    ~encoder(){
        if (errorBuffer) {
            for (int i = 0; i < nchannels; i++) {
                delete[] errorBuffer[i];
            }
            delete[] errorBuffer;
        }
        if (mPhase) {
            delete[] mPhase;
        }
    }
    // remember the number of channels so that we can free the right amount of resources later on
    int nchannels = 0;
    int *mPhase = nullptr;
    float **errorBuffer = nullptr;
};

void validate_format(aoo_format_pcm * fmt)
{
    // validate blocksize
    if (fmt->header.blocksize <= 0){
        LOG_WARNING("PCM: bad blocksize " << fmt->header.blocksize
                    << ", using 64 samples");
        fmt->header.blocksize = 64;
    }
    // validate samplerate
    if (fmt->header.samplerate <= 0){
        LOG_WARNING("PCM: bad samplerate " << fmt->header.samplerate
                    << ", using 44100");
        fmt->header.samplerate = 44100;
    }
    // validate channels
    if (fmt->header.nchannels <= 0 || fmt->header.nchannels > 255){
        LOG_WARNING("PCM: bad channel count " << fmt->header.nchannels
                    << ", using 1 channel");
        fmt->header.nchannels = 1;
    }
    // validate bitdepth
    if (fmt->bitdepth < 0 || fmt->bitdepth > AOO_PCM_BITDEPTH_SIZE){
        LOG_WARNING("PCM: bad bitdepth, using 32bit float");
        fmt->bitdepth = AOO_PCM_FLOAT32;
    }
}

int32_t encoder_setformat(void *enc, aoo_format *f)
{
    if (strcmp(f->codec, AOO_CODEC_PCM)){
        return 0;
    }
    auto c = static_cast<encoder *>(enc);
    auto fmt = reinterpret_cast<aoo_format_pcm *>(f);
    validate_format(fmt);

    // clear noise shaping state
    if (c->errorBuffer != nullptr) {
        // the number of channels could have changed in the format so use the old channels number for the free calls!
        for(int i = 0; i < c->nchannels; i++) {
            delete[] c->errorBuffer[i];
        }
        delete[] c->errorBuffer;
    }
    if (c->mPhase != nullptr) {
        delete[] c->mPhase;
    }

    // init noise shaping state
    c->nchannels = fmt->header.nchannels;
    c->errorBuffer = new float*[c->nchannels];
    for (int i = 0; i < c->nchannels; i++){
        c->errorBuffer[i] = new float[ERROR_BUF_SIZE];
        std::fill(c->errorBuffer[i], c->errorBuffer[i] + ERROR_BUF_SIZE, 0);
    }
    c->mPhase = new int[ERROR_BUF_SIZE];
    std::fill(c->mPhase, c->mPhase + ERROR_BUF_SIZE, 0);

    // save and print settings
    memcpy(&c->format, fmt, sizeof(aoo_format_pcm));
    c->format.header.codec = AOO_CODEC_PCM; // !
    print_settings(c->format);

    return 1;
}

int32_t encoder_readformat(void *enc, aoo_format *fmt,
                           const char *buf, int32_t size)
{
    if (size >= 4){
        auto c = static_cast<codec *>(enc);
        // TODO validate
        if (!strcmp(fmt->codec, AOO_CODEC_PCM) && fmt->blocksize > 0
                && fmt->samplerate > 0 && fmt->blocksize > 0)
        {
            memcpy(&c->format.header, fmt, sizeof(aoo_format));
            c->format.bitdepth = (aoo_pcm_bitdepth)aoo::from_bytes<int32_t>(buf);
            c->format.header.codec = AOO_CODEC_PCM; // !
            //print_settings(c->format);
            
            if (encoder_setformat(enc, &c->format.header)) {
                // it could have been modified during validation, need to re-write the base format of 
                // passed in value
                memcpy(fmt, &c->format.header, sizeof(aoo_format));
                return 4;
            }
            else {
                return -1;
            }
        } else {
            LOG_ERROR("PCM: bad format!");
        }
    } else {
        LOG_ERROR("PCM: couldn't read format - not enough data!");
    }
    return -1;
}

int32_t codec_reset(void *enc) {
    auto c = static_cast<codec *>(enc);
    if (c){
        return 1;
    }
    return 0;
}

int32_t codec_getformat(void *x, aoo_format_storage *f)
{
    auto c = static_cast<codec *>(x);
    if (c->format.header.codec){
        memcpy(f, &c->format, sizeof(aoo_format_pcm));
        return sizeof(aoo_format_pcm);
    } else {
        return 0;
    }
}

void *encoder_new(){
    return new encoder;
}

void encoder_free(void *enc){
    delete (encoder *)enc;
}

int32_t encoder_encode(void *enc,
                       const aoo_sample *s, int32_t n,
                       char *buf, int32_t size)
{
    auto bitdepth = static_cast<codec *>(enc)->format.bitdepth;
    auto samplesize = bytes_per_sample(bitdepth);

    if (size < (n * samplesize)){
        return 0;
    }

    auto samples_to_blob_8bit = [&]{
        auto c = static_cast<encoder *>(enc);
        int nchannels = c->format.header.nchannels;
        auto b = buf;
        for (int i = 0; i < n; ++i){
            sample_to_int8(s[i], b, &(c->mPhase[i % nchannels]), c->errorBuffer[i % nchannels]);
            ++b;
        }
    };

    auto samples_to_blob = [&](auto fn){
        auto b = buf;
        for (int i = 0; i < n; ++i){
            fn(s[i], b);
            b += samplesize;
        }
    };

    switch (bitdepth){
    case AOO_PCM_INT8:
        samples_to_blob_8bit();
        break;
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

    return n * samplesize;
}

int32_t encoder_writeformat(void *enc, aoo_format *fmt,
                            char *buf, int32_t size)
{
    if (size >= 4){
        aoo_format_pcm * ofmt;
        if (enc == nullptr) {
            ofmt = reinterpret_cast<aoo_format_pcm *>(fmt);            
        }
        else {
            auto c = static_cast<codec *>(enc);
            ofmt = &c->format;
            memcpy(fmt, &ofmt->header, sizeof(aoo_format));
        }
        aoo::to_bytes<int32_t>(ofmt->bitdepth, buf);

        return 4;
    } else {
        LOG_ERROR("PCM: couldn't write settings - buffer too small!");
        return -1;
    }
}

void *decoder_new(){
    return new codec;
}

void decoder_free(void *dec){
    delete (codec *)dec;
}

int32_t decoder_decode(void *dec,
                       const char *buf, int32_t size,
                       aoo_sample *s, int32_t n)
{
    auto c = static_cast<codec *>(dec);
    assert(c->format.header.blocksize != 0);

    if (!buf){
        for (int i = 0; i < n; ++i){
            s[i] = 0;
        }
        return 0;
    }

    auto samplesize = bytes_per_sample(c->format.bitdepth);

    if (n < (size / samplesize)){
        return 0;
    }

    auto blob_to_samples = [&](auto convfn){
        auto b = buf;
        for (int i = 0; i < n; ++i, b += samplesize){
            s[i] = convfn(b);
        }
    };

    switch (c->format.bitdepth){
    case AOO_PCM_INT8:
        blob_to_samples(int8_to_sample);
        break;
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
        return 0;
    }

    return size / samplesize;
}

int32_t decoder_setformat(void *enc, aoo_format *f)
{
    if (strcmp(f->codec, AOO_CODEC_PCM)){
        return 0;
    }
    auto c = static_cast<codec *>(enc);
    auto fmt = reinterpret_cast<aoo_format_pcm *>(f);
    validate_format(fmt);

    // save and print settings
    memcpy(&c->format, fmt, sizeof(aoo_format_pcm));
    c->format.header.codec = AOO_CODEC_PCM; // !
    print_settings(c->format);

    return 1;
}

int32_t decoder_readformat(void *dec, aoo_format *fmt,
                           const char *buf, int32_t size)
{
    if (size >= 4){
        auto c = static_cast<codec *>(dec);
        // TODO validate
        if (!strcmp(fmt->codec, AOO_CODEC_PCM) && fmt->blocksize > 0
                && fmt->samplerate > 0 && fmt->blocksize > 0)
        {
            memcpy(&c->format.header, fmt, sizeof(aoo_format));
            c->format.bitdepth = (aoo_pcm_bitdepth)aoo::from_bytes<int32_t>(buf);
            c->format.header.codec = AOO_CODEC_PCM; // !
            print_settings(c->format);
            
            return 4;
        } else {
            LOG_ERROR("PCM: bad format!");
        }
    } else {
        LOG_ERROR("PCM: couldn't read format - not enough data!");
    }
    return -1;
}

aoo_codec codec_class = {
    AOO_CODEC_PCM,
    encoder_new,
    encoder_free,
    encoder_setformat,
    codec_getformat,
    encoder_readformat,
    encoder_writeformat,
    encoder_encode,
    codec_reset,
    decoder_new,
    decoder_free,
    decoder_setformat,
    codec_getformat,
    decoder_readformat,
    decoder_decode,
    codec_reset
};

} // namespace

void aoo_codec_pcm_setup(aoo_codec_registerfn fn){
    fn(AOO_CODEC_PCM, &codec_class);
}

