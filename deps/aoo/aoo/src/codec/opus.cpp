/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others. 
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "aoo/codec/aoo_opus.h"
#include "common/utils.hpp"

#include <cassert>
#include <cstring>
#include <memory>

namespace {

aoo_allocator g_allocator {
    [](size_t n, void *){ return operator new(n); },
    nullptr,
    [](void *ptr, size_t, void *){ operator delete(ptr); },
    nullptr
};

void *allocate(size_t n){
    return g_allocator.alloc(n, g_allocator.context);
}

void deallocate(void *ptr, size_t n){
    g_allocator.free(ptr, n, g_allocator.context);
}

void print_settings(const aoo_format_opus& f){
    const char *application, *type;

    switch (f.application_type){
    case OPUS_APPLICATION_VOIP:
        application = "VOIP";
        break;
    case OPUS_APPLICATION_RESTRICTED_LOWDELAY:
        application = "low delay";
        break;
    default:
        application = "audio";
        break;
    }

    switch (f.signal_type){
    case OPUS_SIGNAL_MUSIC:
        type = "music";
        break;
    case OPUS_SIGNAL_VOICE:
        type = "voice";
        break;
    default:
        type = "auto";
        break;
    }

    LOG_VERBOSE("Opus settings: "
                << "nchannels = " << f.header.nchannels
                << ", blocksize = " << f.header.blocksize
                << ", samplerate = " << f.header.samplerate
                << ", application = " << application
                << ", bitrate = " << f.bitrate
                << ", complexity = " << f.complexity
                << ", signal type = " << type);
}

/*/////////////////////// codec base ////////////////////////*/

struct codec {
    codec(){
        memset(&format, 0, sizeof(format));
    }
    aoo_format_opus format;
};

void validate_format(aoo_format_opus& f, bool loud = true)
{
    f.header.codec = AOO_CODEC_OPUS; // static string!
    f.header.size = sizeof(aoo_format_opus); // actual size!
    // validate samplerate
    switch (f.header.samplerate){
    case 8000:
    case 12000:
    case 16000:
    case 24000:
    case 48000:
        break;
    default:
        if (loud){
            LOG_VERBOSE("Opus: samplerate " << f.header.samplerate
                        << " not supported - using 48000");
        }
        f.header.samplerate = 48000;
        break;
    }
    // validate channels (LATER support multichannel!)
    if (f.header.nchannels < 1 || f.header.nchannels > 255){
        if (loud){
            LOG_WARNING("Opus: channel count " << f.header.nchannels <<
                        " out of range - using 1 channels");
        }
        f.header.nchannels = 1;
    }
    // validate blocksize
    const int minblocksize = f.header.samplerate / 400; // 2.5 ms (e.g. 120 samples @ 48 kHz)
    const int maxblocksize = minblocksize * 24; // 60 ms (e.g. 2880 samples @ 48 kHz)
    int blocksize = f.header.blocksize;
    if (blocksize <= minblocksize){
        f.header.blocksize = minblocksize;
    } else if (blocksize >= maxblocksize){
        f.header.blocksize = maxblocksize;
    } else {
        // round down to nearest multiple of 2.5 ms (in power of 2 steps)
        int result = minblocksize;
        while (result <= blocksize){
            result *= 2;
        }
        f.header.blocksize = result / 2;
    }
    // validate application type
    if (f.application_type != OPUS_APPLICATION_VOIP
            && f.application_type != OPUS_APPLICATION_AUDIO
            && f.application_type != OPUS_APPLICATION_RESTRICTED_LOWDELAY)
    {
        if (loud){
            LOG_WARNING("Opus: bad application type, using OPUS_APPLICATION_AUDIO");
        }
        f.application_type = OPUS_APPLICATION_AUDIO;
    }
    // bitrate, complexity and signal type should be validated by opus
}

aoo_error compare(codec *c, const aoo_format_opus *fmt)
{
    // copy and validate!
    aoo_format_opus f1;
    memcpy(&f1, fmt, sizeof(aoo_format_opus));

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
            f1.application_type == f2.application_type &&
            f1.bitrate == f2.bitrate &&
            f1.complexity == f2.complexity &&
            f1.signal_type == f2.signal_type;
}

aoo_error get_format(codec *c, aoo_format *f, size_t size)
{
    // check if format has been set
    if (c->format.header.codec){
        if (size >= c->format.header.size){
            memcpy(f, &c->format, sizeof(aoo_format_opus));
            return AOO_OK;
        } else {
            return AOO_ERROR_UNSPECIFIED;
        }
    } else {
        return AOO_ERROR_UNSPECIFIED;
    }
}

/*/////////////////////////// encoder //////////////////////*/

struct encoder : codec {
    ~encoder(){
        if (state){
            deallocate(state, size);
        }
    }
    OpusMSEncoder *state = nullptr;
    size_t size = 0;
};

void *encoder_new(){
    auto obj = allocate(sizeof(encoder));
    new (obj) encoder {};
    return obj;
}

void encoder_free(void *enc){
    static_cast<encoder *>(enc)->~encoder();
    deallocate(enc, sizeof(encoder));
}

aoo_error encode(void *enc,
                 const aoo_sample *s, int32_t n,
                 char *buf, int32_t *size)
{
    auto c = static_cast<encoder *>(enc);
    if (c->state){
        auto framesize = n / c->format.header.nchannels;
        auto result = opus_multistream_encode_float(
            c->state, s, framesize, (unsigned char *)buf, *size);
        if (result > 0){
            *size = result;
            return AOO_OK;
        } else {
            LOG_VERBOSE("Opus: opus_encode_float() failed with error code " << result);
        }
    }
    return AOO_ERROR_UNSPECIFIED;
}

aoo_error encoder_set_format(encoder *c, aoo_format_opus *f){
    if (strcmp(f->header.codec, AOO_CODEC_OPUS)){
        return AOO_ERROR_UNSPECIFIED;
    }
    if (f->header.size < sizeof(aoo_format_opus)){
        return AOO_ERROR_UNSPECIFIED;
    }

    validate_format(*f);

    // LATER only deallocate if channels, sr and application type
    // have changed, otherwise simply reset the encoder.
    if (c->state){
        deallocate(c->state, c->size);
        c->state = nullptr;
        c->size = 0;
    }
    // setup channel mapping
    // only use decoupled streams (what's the point of coupled streams?)
    auto nchannels = f->header.nchannels;
    unsigned char mapping[256];
    for (int i = 0; i < nchannels; ++i){
        mapping[i] = i;
    }
    memset(mapping + nchannels, 255, 256 - nchannels);
    // create state
    size_t size = opus_multistream_encoder_get_size(nchannels, 0);
    auto state = (OpusMSEncoder *)allocate(size);
    if (!state){
        return AOO_ERROR_UNSPECIFIED;
    }
    auto error = opus_multistream_encoder_init(state, f->header.samplerate,
        nchannels, nchannels, 0, mapping, f->application_type);
    if (error != OPUS_OK){
        LOG_ERROR("Opus: opus_encoder_create() failed with error code " << error);
        return AOO_ERROR_UNSPECIFIED;
    }
    c->state = state;
    c->size = size;
    // apply settings
    // complexity
    opus_multistream_encoder_ctl(c->state, OPUS_SET_COMPLEXITY(f->complexity));
    opus_multistream_encoder_ctl(c->state, OPUS_GET_COMPLEXITY(&f->complexity));
    // bitrate
    opus_multistream_encoder_ctl(c->state, OPUS_SET_BITRATE(f->bitrate));
#if 0
    // This control is broken in opus_multistream_encoder (as of opus v1.3.2)
    // because it would always return the default bitrate.
    // The only thing we can do is omit the function and just keep the input value.
    // This means that clients have to explicitly check for OPUS_AUTO and
    // OPUS_BITRATE_MAX when reading the 'bitrate' value after encoder_setformat().
    opus_multistream_encoder_ctl(c->state, OPUS_GET_BITRATE(&f->bitrate));
#endif
    // signal type
    opus_multistream_encoder_ctl(c->state, OPUS_SET_SIGNAL(f->signal_type));
    opus_multistream_encoder_ctl(c->state, OPUS_GET_SIGNAL(&f->signal_type));

    // save and print settings
    memcpy(&c->format, f, sizeof(aoo_format_opus));
    print_settings(c->format);

    return AOO_OK;
}

aoo_error encoder_ctl(void *x, int32_t ctl, void *ptr, int32_t size){
    switch (ctl){
    case AOO_CODEC_SET_FORMAT:
        assert(size >= sizeof(aoo_format));
        return encoder_set_format((encoder *)x, (aoo_format_opus *)ptr);
    case AOO_CODEC_GET_FORMAT:
        return get_format((codec *)x, (aoo_format *)ptr, size);
    case AOO_CODEC_RESET:
        if (opus_multistream_encoder_ctl(static_cast<encoder *>(x)->state,
                                         OPUS_RESET_STATE) == OPUS_OK) {
            return AOO_OK;
        } else {
            return AOO_ERROR_UNSPECIFIED;
        }
    case AOO_CODEC_FORMAT_EQUAL:
        assert(size >= sizeof(aoo_format));
        return compare((codec *)x, (const aoo_format_opus *)ptr);
    default:
        LOG_WARNING("Opus: unsupported codec ctl " << ctl);
        return AOO_ERROR_UNSPECIFIED;
    }
}

/*/////////////////////// decoder ///////////////////////////*/

struct decoder : codec {
    ~decoder(){
        if (state){
            deallocate(state, size);
        }
    }
    OpusMSDecoder * state = nullptr;
    size_t size = 0;
};

void *decoder_new(){
    auto obj = allocate(sizeof(decoder));
    new (obj) decoder {};
    return obj;
}

void decoder_free(void *dec){
    static_cast<decoder *>(dec)->~decoder();
    deallocate(dec, sizeof(decoder));
}

aoo_error decode(void *dec,
                 const char *buf, int32_t size,
                 aoo_sample *s, int32_t *n)
{
    auto c = static_cast<decoder *>(dec);
    if (c->state){
        auto framesize = *n / c->format.header.nchannels;
        auto result = opus_multistream_decode_float(
            c->state, (const unsigned char *)buf, size, s, framesize, 0);
        if (result > 0){
            *n = result;
            return AOO_OK;
        } else if (result < 0) {
            LOG_VERBOSE("Opus: opus_decode_float() failed with error code " << result);
        }
    }
    return AOO_ERROR_UNSPECIFIED;
}

aoo_error decoder_set_format(decoder *c, aoo_format *f)
{
    if (strcmp(f->codec, AOO_CODEC_OPUS)){
        return AOO_ERROR_UNSPECIFIED;
    }
    if (f->size < sizeof(aoo_format_opus)){
        return AOO_ERROR_UNSPECIFIED;
    }

    auto fmt = reinterpret_cast<aoo_format_opus *>(f);

    validate_format(*fmt);

    // LATER only deallocate if channels and sr have changed,
    // otherwise simply reset the decoder.
    if (c->state){
        deallocate(c->state, c->size);
        c->state = nullptr;
        c->size = 0;
    }
    // setup channel mapping
    // only use decoupled streams (what's the point of coupled streams?)
    auto nchannels = fmt->header.nchannels;
    unsigned char mapping[256];
    for (int i = 0; i < nchannels; ++i){
        mapping[i] = i;
    }
    memset(mapping + nchannels, 255, 256 - nchannels);
    // create state
    size_t size = opus_multistream_decoder_get_size(nchannels, 0);
    auto state = (OpusMSDecoder *)allocate(size);
    if (!state){
        return AOO_ERROR_UNSPECIFIED;
    }
    auto error = opus_multistream_decoder_init(state, fmt->header.samplerate,
                                               nchannels, nchannels, 0, mapping);
    if (error != OPUS_OK){
        LOG_ERROR("Opus: opus_decoder_create() failed with error code " << error);
        return AOO_ERROR_UNSPECIFIED;
    }
    c->state = state;
    c->size = size;
    // these are actually encoder settings and don't do anything on the decoder
#if 0
    // complexity
    opus_multistream_decoder_ctl(c->state, OPUS_SET_COMPLEXITY(f->complexity));
    opus_multistream_decoder_ctl(c->state, OPUS_GET_COMPLEXITY(&f->complexity));
    // bitrate
    opus_multistream_decoder_ctl(c->state, OPUS_SET_BITRATE(f->bitrate));
    opus_multistream_decoder_ctl(c->state, OPUS_GET_BITRATE(&f->bitrate));
    // signal type
    opus_multistream_decoder_ctl(c->state, OPUS_SET_SIGNAL(f->signal_type));
    opus_multistream_decoder_ctl(c->state, OPUS_GET_SIGNAL(&f->signal_type));
#endif

    // save and print settings
    memcpy(&c->format, fmt, sizeof(aoo_format_opus));
    print_settings(c->format);

    return AOO_OK;
}

aoo_error decoder_ctl(void *x, int32_t ctl, void *ptr, int32_t size){
    switch (ctl){
    case AOO_CODEC_SET_FORMAT:
        assert(size >= sizeof(aoo_format));
        return decoder_set_format((decoder *)x, (aoo_format *)ptr);
    case AOO_CODEC_GET_FORMAT:
        return get_format((decoder *)x, (aoo_format *)ptr, size);
    case AOO_CODEC_RESET:
        if (opus_multistream_decoder_ctl(static_cast<decoder *>(x)->state,
                                         OPUS_RESET_STATE) == OPUS_OK) {
            return AOO_OK;
        } else {
            return AOO_ERROR_UNSPECIFIED;
        }
    case AOO_CODEC_FORMAT_EQUAL:
        assert(size >= sizeof(aoo_format));
        return compare((codec *)x, (const aoo_format_opus *)ptr);
    default:
        LOG_WARNING("Opus: unsupported codec ctl " << ctl);
        return AOO_ERROR_UNSPECIFIED;
    }
}

/*////////////////////// codec ////////////////////*/

aoo_error serialize(const aoo_format *f, char *buf, int32_t *size){
    if (*size >= 16){
        auto fmt = (const aoo_format_opus *)f;
        aoo::to_bytes<int32_t>(fmt->bitrate, buf);
        aoo::to_bytes<int32_t>(fmt->complexity, buf + 4);
        aoo::to_bytes<int32_t>(fmt->signal_type, buf + 8);
        aoo::to_bytes<int32_t>(fmt->application_type, buf + 12);
        *size = 16;

        return AOO_OK;
    } else {
        LOG_WARNING("Opus: couldn't write settings");
        return AOO_ERROR_UNSPECIFIED;
    }
}

aoo_error deserialize(const aoo_format *header, const char *buf,
                      int32_t nbytes, aoo_format *f, int32_t size){
    if (nbytes < 16){
        LOG_ERROR("Opus: couldn't read format - not enough data!");
        return AOO_ERROR_UNSPECIFIED;
    }
    if (size < sizeof(aoo_format_opus)){
        LOG_ERROR("Opus: output format storage too small");
        return AOO_ERROR_UNSPECIFIED;
    }
    auto fmt = (aoo_format_opus *)f;
    // header
    fmt->header.codec = AOO_CODEC_OPUS; // static string!
    fmt->header.size = sizeof(aoo_format_opus); // actual size!
    fmt->header.blocksize = header->blocksize;
    fmt->header.nchannels = header->nchannels;
    fmt->header.samplerate = header->samplerate;
    // options
    fmt->bitrate = aoo::from_bytes<int32_t>(buf);
    fmt->complexity = aoo::from_bytes<int32_t>(buf + 4);
    fmt->signal_type = aoo::from_bytes<int32_t>(buf + 8);
    fmt->application_type = aoo::from_bytes<int32_t>(buf + 12);

    return AOO_OK;
}

aoo_codec codec_class = {
    AOO_CODEC_OPUS,
    // encoder
    encoder_new,
    encoder_free,
    encoder_ctl,
    encode,
    // decoder
    decoder_new,
    decoder_free,
    decoder_ctl,
    decode,
    // helper
    serialize,
    deserialize
};

} // namespace

void aoo_codec_opus_setup(aoo_codec_registerfn fn, const aoo_allocator *alloc){
    if (alloc){
        g_allocator = *alloc;
    }
    fn(AOO_CODEC_OPUS, &codec_class);
}

