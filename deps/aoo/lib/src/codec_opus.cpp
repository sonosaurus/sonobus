/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others. 
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "aoo/aoo_opus.h"
#include "aoo/aoo_utils.hpp"

#include <cassert>
#include <cstring>
#include <memory>

namespace {

void print_settings(const aoo_format_opus& f){
    const char *type;
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

    const char *apptype;
    switch (f.application_type){
    case OPUS_APPLICATION_RESTRICTED_LOWDELAY:
        apptype = "lowdelay";
        break;
    case OPUS_APPLICATION_VOIP:
        apptype = "voice";
        break;
    case OPUS_APPLICATION_AUDIO:
    default:
        apptype = "audio";
        break;
    }

    
    LOG_VERBOSE("Opus settings: "
                << "nchannels = " << f.header.nchannels
                << ", blocksize = " << f.header.blocksize
                << ", samplerate = " << f.header.samplerate
                << ", bitrate = " << f.bitrate
                << ", complexity = " << f.complexity
                << ", application = " << apptype
                << ", signal type = " << type);
}

/*/////////////////////// codec base ////////////////////////*/

struct codec {
    codec(){
        memset(&format, 0, sizeof(format));
    }
    aoo_format_opus format;
};

void validate_format(aoo_format_opus& f)
{
    // validate samplerate
    switch (f.header.samplerate){
    case 8000:
    case 12000:
    case 16000:
    case 24000:
    case 48000:
        break;
    default:
        LOG_VERBOSE("Opus: samplerate " << f.header.samplerate
                    << " not supported - using 48000");
        f.header.samplerate = 48000;
        break;
    }
    // validate channels (LATER support multichannel!)
    if (f.header.nchannels < 1 || f.header.nchannels > 255){
        LOG_WARNING("Opus: channel count " << f.header.nchannels <<
                    " out of range - using 1 channels");
        f.header.nchannels = 1;
    }
    // validate blocksize
    int minblocksize = f.header.samplerate / 400; // 120 samples @ 48 kHz
    int maxblocksize = minblocksize * 24; // 2880 samples @ 48 kHz
    int blocksize = f.header.blocksize;
    if (blocksize < minblocksize){
        f.header.blocksize = minblocksize;
    } else if (blocksize > maxblocksize){
        f.header.blocksize = maxblocksize;
    } else {
        // round down
        while (blocksize > (minblocksize * 2)-1){
            minblocksize *= 2;
        }
        f.header.blocksize = minblocksize;
    }
    // validate application type, default to AUDIO
    if (f.application_type == 0) {
        f.application_type = OPUS_APPLICATION_AUDIO;
    }
    // bitrate, complexity and signal type should be validated by opus
}

int32_t codec_getformat(void *x, aoo_format_storage *f)
{
    auto c = static_cast<codec *>(x);
    if (c->format.header.codec){
        memcpy(f, &c->format, sizeof(aoo_format_opus));
        return sizeof(aoo_format_opus);
    } else {
        return 0;
    }
}

/*/////////////////////////// encoder //////////////////////*/

struct encoder : codec {
    ~encoder(){
        if (state){
            opus_multistream_encoder_destroy(state);
        }
    }
    OpusMSEncoder *state = nullptr;
};

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
    auto c = static_cast<encoder *>(enc);
    if (c->state){
        auto framesize = n / c->format.header.nchannels;
        auto result = opus_multistream_encode_float(
                    c->state, s, framesize, (unsigned char *)buf, size);
        if (result > 0){
            return result;
        } else {
            LOG_VERBOSE("Opus: opus_encode_float() failed with error code " << result);
        }
    }
    return 0;
}

int32_t encoder_setformat(void *enc, aoo_format *f){
    if (strcmp(f->codec, AOO_CODEC_OPUS)){
        return 0;
    }
    auto c = static_cast<encoder *>(enc);
    auto fmt = reinterpret_cast<aoo_format_opus *>(f);

    validate_format(*fmt);

    int error = 0;
    if (c->state){
        opus_multistream_encoder_destroy(c->state);
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
    c->state = opus_multistream_encoder_create(fmt->header.samplerate,
                                       nchannels, nchannels, 0, mapping,
                                       fmt->application_type, &error);
    if (error == OPUS_OK){
        assert(c->state != nullptr);
        // apply settings
        // complexity
        opus_multistream_encoder_ctl(c->state, OPUS_SET_COMPLEXITY(fmt->complexity));
        opus_multistream_encoder_ctl(c->state, OPUS_GET_COMPLEXITY(&fmt->complexity));
        // bitrate
        opus_multistream_encoder_ctl(c->state, OPUS_SET_BITRATE(fmt->bitrate));
    #if 0
        // This control is broken in opus_multistream_encoder (as of opus v1.3.2)
        // because it would always return the default bitrate.
        // The only thing we can do is omit the function and just keep the input value.
        // This means that clients have to explicitly check for OPUS_AUTO and
        // OPUS_BITRATE_MAX when reading the 'bitrate' value after encoder_setformat().
        opus_multistream_encoder_ctl(c->state, OPUS_GET_BITRATE(&fmt->bitrate));
    #endif
        // signal type
        opus_multistream_encoder_ctl(c->state, OPUS_SET_SIGNAL(fmt->signal_type));
        opus_multistream_encoder_ctl(c->state, OPUS_GET_SIGNAL(&fmt->signal_type));
    } else {
        LOG_ERROR("Opus: opus_encoder_create() failed with error code " << error);
        return 0;
    }

    // save and print settings
    memcpy(&c->format, fmt, sizeof(aoo_format_opus));
    c->format.header.codec = AOO_CODEC_OPUS; // !
    print_settings(c->format);

    return 1;
}

#define encoder_getformat codec_getformat

int32_t encoder_writeformat(void *enc, aoo_format *fmt,
                            char *buf, int32_t size){
    if (size >= 16){
        // if encoder is null we assume the format passed in
        // is actually a reference to an aoo_format_opus,
        // and this call is used for serialization purposes
        aoo_format_opus * ofmt;
        if (enc == nullptr) {
            ofmt = reinterpret_cast<aoo_format_opus *>(fmt);            
        }
        else {
            auto c = static_cast<encoder *>(enc);
            ofmt = &c->format;
            memcpy(fmt, &ofmt->header, sizeof(aoo_format));
        }
        aoo::to_bytes<int32_t>(ofmt->bitrate, buf);
        aoo::to_bytes<int32_t>(ofmt->complexity, buf + 4);
        aoo::to_bytes<int32_t>(ofmt->signal_type, buf + 8);
        aoo::to_bytes<int32_t>(ofmt->application_type, buf + 12);
        return 16;
    } else {
        LOG_WARNING("Opus: couldn't write settings");
        return -1;
    }
}

int32_t encoder_readformat(void *enc, aoo_format *fmt,
                           const char *buf, int32_t size){
    if (strcmp(fmt->codec, AOO_CODEC_OPUS)){
        LOG_ERROR("opus: wrong format!");
        return -1;
    }
    if (size >= 12){
        auto c = static_cast<encoder *>(enc);
        aoo_format_opus f;
        int32_t retsize = 12;
        // opus will validate for us
        memcpy(&f.header, fmt, sizeof(aoo_format));
        f.bitrate = aoo::from_bytes<int32_t>(buf);
        f.complexity = aoo::from_bytes<int32_t>(buf + 4);
        f.signal_type = aoo::from_bytes<int32_t>(buf + 8);
        if (size >= 16) {
            f.application_type = aoo::from_bytes<int32_t>(buf + 12);
            retsize = 16;
        } else {
            f.application_type = OPUS_APPLICATION_AUDIO;
        }
        
        if (encoder_setformat(c, reinterpret_cast<aoo_format *>(&f))){
            // it could have been modified during validation, need to re-write the base format of 
            // passed in value
            memcpy(fmt, &f.header, sizeof(aoo_format));
            
            return retsize; // number of bytes
        } else {
            return -1; // error
        }
    } else {
        LOG_ERROR("Opus: couldn't read format - too little data!");
        return -1;
    }
}

int32_t encoder_reset(void *enc) {
    auto c = static_cast<encoder *>(enc);
    if (c->state){
        opus_multistream_encoder_ctl(c->state, OPUS_RESET_STATE);
        return 1;
    }
    return 0;
}


/*/////////////////////// decoder ///////////////////////////*/

struct decoder : codec {
    ~decoder(){
        if (state){
            opus_multistream_decoder_destroy(state);
        }
    }
    OpusMSDecoder * state = nullptr;
};

void *decoder_new(){
    return new decoder;
}

void decoder_free(void *dec){
    delete (decoder *)dec;
}

int32_t decoder_decode(void *dec,
                       const char *buf, int32_t size,
                       aoo_sample *s, int32_t n)
{
    auto c = static_cast<decoder *>(dec);
    if (c->state){
        auto framesize = n / c->format.header.nchannels;
        auto result = opus_multistream_decode_float(
                    c->state, (const unsigned char *)buf, size, s, framesize, 0);
        if (result > 0){
            return result;
        } else if (result < 0) {
            LOG_VERBOSE("Opus: opus_decode_float() failed with error code " << result);
            return result;
        }
    }
    return 0;
}

bool decoder_dosetformat(decoder *c, aoo_format_opus& f){
    if (c->state){
        opus_multistream_decoder_destroy(c->state);
    }
    int error = 0;
    // setup channel mapping
    // only use decoupled streams (what's the point of coupled streams?)

    // validate nchannels (we might not call validate_format())
    // the rest is validated by opus
    auto nchannels = f.header.nchannels;
    if (nchannels < 1 || nchannels > 255){
        LOG_WARNING("Opus: channel count " << nchannels << " out of range");
        return false;
    }
    unsigned char mapping[256];
    for (int i = 0; i < nchannels; ++i){
        mapping[i] = i;
    }
    memset(mapping + nchannels, 255, 256 - nchannels);
    // create state
    c->state = opus_multistream_decoder_create(f.header.samplerate,
                                       nchannels, nchannels, 0, mapping,
                                       &error);
    if (error == OPUS_OK){
        assert(c->state != nullptr);
        // these are actually encoder settings and do anything on the decoder
    #if 0
        // complexity
        opus_multistream_decoder_ctl(c->state, OPUS_SET_COMPLEXITY(f.complexity));
        opus_multistream_decoder_ctl(c->state, OPUS_GET_COMPLEXITY(&f.complexity));
        // bitrate
        opus_multistream_decoder_ctl(c->state, OPUS_SET_BITRATE(f.bitrate));
        opus_multistream_decoder_ctl(c->state, OPUS_GET_BITRATE(&f.bitrate));
        // signal type
        opus_multistream_decoder_ctl(c->state, OPUS_SET_SIGNAL(f.signal_type));
        opus_multistream_decoder_ctl(c->state, OPUS_GET_SIGNAL(&f.signal_type));
    #endif
        // save and print settings
        memcpy(&c->format, &f, sizeof(aoo_format_opus));
        c->format.header.codec = AOO_CODEC_OPUS; // !
        print_settings(f);
        return true;
    } else {
        LOG_ERROR("Opus: opus_decoder_create() failed with error code " << error);
        return false;
    }
}

int32_t decoder_setformat(void *dec, aoo_format *f)
{
    if (strcmp(f->codec, AOO_CODEC_OPUS)){
        return 0;
    }

    auto c = static_cast<decoder *>(dec);
    auto fmt = reinterpret_cast<aoo_format_opus *>(f);

    validate_format(*fmt);

    return decoder_dosetformat(c, *fmt);
}

#define decoder_getformat codec_getformat

int32_t decoder_readformat(void *dec, aoo_format *fmt,
                           const char *buf, int32_t size){
    if (strcmp(fmt->codec, AOO_CODEC_OPUS)){
        LOG_ERROR("opus: wrong format!");
        return -1;
    }
    if (size >= 12){
        auto c = static_cast<decoder *>(dec);
        aoo_format_opus f;
        int32_t retsize = 12;
        // opus will validate for us
        memcpy(&f.header, fmt, sizeof(aoo_format));
        f.bitrate = aoo::from_bytes<int32_t>(buf);
        f.complexity = aoo::from_bytes<int32_t>(buf + 4);
        f.signal_type = aoo::from_bytes<int32_t>(buf + 8);
        if (size >= 16) {
            f.application_type = aoo::from_bytes<int32_t>(buf + 12);
            retsize = 16;
        } else {
            f.application_type = OPUS_APPLICATION_AUDIO;
        }
        
        if (decoder_dosetformat(c, f)){
            return retsize; // number of bytes
        } else {
            return -1; // error
        }
    } else {
        LOG_ERROR("Opus: couldn't read format - too little data!");
        return -1;
    }
}

int32_t decoder_reset(void *enc) {
    auto c = static_cast<decoder *>(enc);
    if (c->state){
        opus_multistream_decoder_ctl(c->state, OPUS_RESET_STATE);
        return 1;
    }
    return 0;
}


aoo_codec codec_class = {
    AOO_CODEC_OPUS,
    encoder_new,
    encoder_free,
    encoder_setformat,
    encoder_getformat,
    encoder_readformat,
    encoder_writeformat,
    encoder_encode,
    encoder_reset,
    decoder_new,
    decoder_free,
    decoder_setformat,
    decoder_getformat,
    decoder_readformat,
    decoder_decode,
    decoder_reset
};

} // namespace

void aoo_codec_opus_setup(aoo_codec_registerfn fn){
    fn(AOO_CODEC_OPUS, &codec_class);
}

