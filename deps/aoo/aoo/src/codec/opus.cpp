/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "aoo/aoo_codec.h"
#include "aoo/codec/aoo_opus.h"

#include "../imp.hpp"

#include "common/utils.hpp"

#include <cassert>
#include <cstring>
#include <memory>

namespace {

//---------------- helper functions -----------------//

void print_format(const AooFormatOpus& f){
    const char *application;

    switch (f.applicationType){
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

    LOG_VERBOSE("Opus settings: "
                << "nchannels = " << f.header.numChannels
                << ", blocksize = " << f.header.blockSize
                << ", samplerate = " << f.header.sampleRate
                << ", application = " << application);
}

bool validate_format(AooFormatOpus& f, bool loud = true)
{
    if (strcmp(f.header.codec, kAooCodecOpus)){
        return false;
    }

    if (f.header.size < sizeof(AooFormatOpus)){
        return false;
    }

    // validate samplerate
    switch (f.header.sampleRate){
    case 8000:
    case 12000:
    case 16000:
    case 24000:
    case 48000:
        break;
    default:
        if (loud){
            LOG_VERBOSE("Opus: samplerate " << f.header.sampleRate
                        << " not supported - using 48000");
        }
        f.header.sampleRate = 48000;
        break;
    }

    // validate channels
    if (f.header.numChannels < 1 || f.header.numChannels > 255){
        if (loud){
            LOG_WARNING("Opus: channel count " << f.header.numChannels <<
                        " out of range - using 1 channels");
        }
        f.header.numChannels = 1;
    }

    // validate blocksize
    const int minblocksize = f.header.sampleRate / 400; // 2.5 ms (e.g. 120 samples @ 48 kHz)
    const int maxblocksize = minblocksize * 24; // 60 ms (e.g. 2880 samples @ 48 kHz)
    int blocksize = f.header.blockSize;
    if (blocksize <= minblocksize){
        f.header.blockSize = minblocksize;
    } else if (blocksize >= maxblocksize){
        f.header.blockSize = maxblocksize;
    } else {
        // round down to nearest multiple of 2.5 ms (in power of 2 steps)
        int result = minblocksize;
        while (result <= blocksize){
            result *= 2;
        }
        f.header.blockSize = result / 2;
    }

    // validate application type
    if (f.applicationType != OPUS_APPLICATION_VOIP
            && f.applicationType != OPUS_APPLICATION_AUDIO
            && f.applicationType != OPUS_APPLICATION_RESTRICTED_LOWDELAY)
    {
        if (loud){
            LOG_WARNING("Opus: bad application type, using OPUS_APPLICATION_AUDIO");
        }
        f.applicationType = OPUS_APPLICATION_AUDIO;
    }

    return true;
}

template<typename T>
T& as(void *p){
    return *reinterpret_cast<T *>(p);
}

#define CHECKARG(type) assert(size == sizeof(type))

//------------------------------- encoder --------------------------//

AooError createEncoderState(const AooFormatOpus& f,
                            OpusMSEncoder *& state, size_t& size){
    // setup channel mapping
    // only use decoupled streams (what's the point of coupled streams?)
    auto nchannels = f.header.numChannels;
    unsigned char mapping[256];
    for (int i = 0; i < nchannels; ++i){
        mapping[i] = i;
    }
    memset(mapping + nchannels, 255, 256 - nchannels);

    // create state
    size = opus_multistream_encoder_get_size(nchannels, 0);
    state = (OpusMSEncoder *)aoo::allocate(size);
    if (!state){
        return kAooErrorOutOfMemory;
    }
    auto err = opus_multistream_encoder_init(
                state, f.header.sampleRate, nchannels,
                nchannels, 0, mapping, f.applicationType);
    if (err != OPUS_OK){
        LOG_ERROR("Opus: opus_encoder_create() failed with error code " << err);
        aoo::deallocate(state, size);
        return kAooErrorBadArgument;
    }
    return kAooOk;
}

struct Encoder : AooCodec {
    Encoder(OpusMSEncoder *state, size_t size, const AooFormatOpus& f);
    ~Encoder(){
        aoo::deallocate(state_, size_);
    }
    OpusMSEncoder *state_;
    size_t size_;
    int numChannels_;
};

AooCodec * Encoder_new(AooFormat *f, AooError *err){
    auto fmt = (AooFormatOpus *)f;
    if (!validate_format(*fmt, true)){
        if (err) {
            *err = kAooErrorBadArgument;
        }
        return nullptr;
    }

    print_format(*fmt);

    OpusMSEncoder *state;
    size_t size;
    auto res = createEncoderState(*fmt, state, size);
    if (err){
        *err = res;
    }
    if (res == kAooOk){
        return aoo::construct<Encoder>(state, size, *fmt);
    } else {
        return nullptr;
    }
}

void Encoder_free(AooCodec *c){
    aoo::destroy(static_cast<Encoder *>(c));
}

AooError AOO_CALL Encoder_control(
        AooCodec *c, AooCtl ctl, void *ptr, AooSize size) {
    auto e = static_cast<Encoder *>(c);
    switch (ctl){
    case kAooCodecCtlReset:
    {
        auto err = opus_multistream_encoder_ctl(
                    e->state_, OPUS_RESET_STATE);
        if (err != OPUS_OK) {
            return kAooErrorBadArgument;
        }
        break;
    }
    case OPUS_SET_COMPLEXITY_REQUEST:
    {
        CHECKARG(opus_int32);
        auto complexity = as<opus_int32>(ptr);
        auto err = opus_multistream_encoder_ctl(
                    e->state_, OPUS_SET_COMPLEXITY(complexity));
        if (err != OPUS_OK){
            return kAooErrorBadArgument;
        }
        break;
    }
    case OPUS_GET_COMPLEXITY_REQUEST:
    {
        CHECKARG(opus_int32);
        auto complexity = (opus_int32 *)ptr;
        auto err = opus_multistream_encoder_ctl(
                    e->state_, OPUS_GET_COMPLEXITY(complexity));
        if (err != OPUS_OK){
            return kAooErrorBadArgument;
        }
        break;
    }
    case OPUS_SET_BITRATE_REQUEST:
    {
        CHECKARG(opus_int32);
        auto bitrate = as<opus_int32>(ptr);
        auto err = opus_multistream_encoder_ctl(
                    e->state_, OPUS_SET_BITRATE(bitrate));
        if (err != OPUS_OK){
            return kAooErrorBadArgument;
        }
        break;
    }
    case OPUS_GET_BITRATE_REQUEST:
    {
        CHECKARG(opus_int32);
        auto bitrate = (opus_int32 *)ptr;
    #if 1
        // This control is broken in opus_multistream_encoder (as of opus v1.3.2)
        // because it would always return the default bitrate.
        // As a temporary workaround, we just return OPUS_AUTO unconditionally.
        // LATER validate and cache the value instead.
        *bitrate = OPUS_AUTO;
    #else
        auto err = opus_multistream_encoder_ctl(
                    e->state_, OPUS_GET_BITRATE(bitrate));
        if (err != OPUS_OK){
            return kAooErrorBadArgument;
        }
    #endif
        break;
    }
    case OPUS_SET_SIGNAL_REQUEST:
    {
        CHECKARG(opus_int32);
        auto signal = as<opus_int32>(ptr);
        auto err = opus_multistream_encoder_ctl(
                    e->state_, OPUS_SET_SIGNAL(signal));
        if (err != OPUS_OK){
            return kAooErrorBadArgument;
        }
        break;
    }
    case OPUS_GET_SIGNAL_REQUEST:
    {
        CHECKARG(opus_int32);
        auto signal = (opus_int32 *)ptr;
        auto err = opus_multistream_encoder_ctl(
                    e->state_, OPUS_GET_SIGNAL(signal));
        if (err != OPUS_OK){
            return kAooErrorBadArgument;
        }
        break;
    }
    default:
        LOG_WARNING("Opus: unsupported codec ctl " << ctl);
        return kAooErrorNotImplemented;
    }
    return kAooOk;
}

AooError Encoder_encode(
        AooCodec *c, const AooSample *input, AooInt32 numSamples,
        AooByte *buf, AooInt32 *size)
{
    auto e = static_cast<Encoder *>(c);
    auto framesize = numSamples / e->numChannels_;
    auto result = opus_multistream_encode_float(
                e->state_, input, framesize, (unsigned char *)buf, *size);
    if (result > 0){
        *size = result;
        return kAooOk;
    } else {
        LOG_VERBOSE("Opus: opus_encode_float() failed with error code " << result);
        return kAooErrorUnknown;
    }
}

//------------------------- decoder -----------------------//

AooError createDecoderState(const AooFormatOpus& f,
                            OpusMSDecoder *& state, size_t& size){
    // setup channel mapping
    // only use decoupled streams (what's the point of coupled streams?)
    auto nchannels = f.header.numChannels;
    unsigned char mapping[256];
    for (int i = 0; i < nchannels; ++i){
        mapping[i] = i;
    }
    memset(mapping + nchannels, 255, 256 - nchannels);

    // create state
    size = opus_multistream_decoder_get_size(nchannels, 0);
    state = (OpusMSDecoder *)aoo::allocate(size);
    if (!state){
        return kAooErrorOutOfMemory;
    }
    auto err = opus_multistream_decoder_init(
                state, f.header.sampleRate,
                nchannels, nchannels, 0, mapping);
    if (err != OPUS_OK){
        LOG_ERROR("Opus: opus_decoder_create() failed with error code " << err);
        aoo::deallocate(state, size);
        return kAooErrorBadArgument;
    }
    return kAooOk;
}

struct Decoder : AooCodec {
    Decoder(OpusMSDecoder *state, size_t size, const AooFormatOpus& f);
    ~Decoder(){
        aoo::deallocate(state_, size_);
    }
    OpusMSDecoder *state_;
    size_t size_;
    int numChannels_;
};

AooCodec * Decoder_new(AooFormat *f, AooError *err){
    auto fmt = (AooFormatOpus *)f;
    if (!validate_format(*fmt, true)){
        if (err) {
            *err = kAooErrorBadArgument;
        }
        return nullptr;
    }

    print_format(*fmt);

    OpusMSDecoder *state;
    size_t size;
    auto res = createDecoderState(*fmt, state, size);
    if (err){
        *err = res;
    }
    if (res == kAooOk){
        return aoo::construct<Decoder>(state, size, *fmt);
    } else {
        return nullptr;
    }
}

void Decoder_free(AooCodec *c){
    aoo::destroy(static_cast<Decoder *>(c));
}

AooError Decoder_control(AooCodec *c, AooCtl ctl, void *ptr, AooSize size){
    auto d = static_cast<Decoder *>(c);
    switch (ctl){
    case kAooCodecCtlReset:
    {
        auto err = opus_multistream_decoder_ctl(
                    d->state_, OPUS_RESET_STATE);
        if (err != OPUS_OK) {
            return kAooErrorBadArgument;
        }
        break;
    }
    default:
        LOG_WARNING("Opus: unsupported codec ctl " << ctl);
        return kAooErrorNotImplemented;
    }
    return kAooOk;
}

AooError Decoder_decode(
        AooCodec *c, const AooByte *buf, AooInt32 size,
        AooSample *output, AooInt32 *numSamples)
{
    auto d = static_cast<Decoder *>(c);
    auto framesize = *numSamples / d->numChannels_;
    auto result = opus_multistream_decode_float(
                d->state_, (const unsigned char *)buf, size, output, framesize, 0);
    if (result > 0){
        *numSamples = result;
        return kAooOk;
    } else {
        LOG_VERBOSE("Opus: opus_decode_float() failed with error code " << result);
        return kAooErrorUnknown;
    }
}

//-------------------------- free functions ------------------------//

AooError serialize(const AooFormat *f, AooByte *buf, AooInt32 *size){
    if (!buf){
        *size = sizeof(AooInt32);
        return kAooOk;
    }
    if (*size >= sizeof(AooInt32)){
        auto fmt = (const AooFormatOpus *)f;
        aoo::to_bytes<AooInt32>(fmt->applicationType, buf);
        *size = sizeof(AooInt32);

        return kAooOk;
    } else {
        LOG_ERROR("Opus: couldn't write settings - buffer too small!");
        return kAooErrorBadArgument;
    }
}

AooError deserialize(
        const AooByte *buf, AooInt32 size, AooFormat *f, AooInt32 *fmtsize){
    if (!f){
        *fmtsize = sizeof(AooFormatOpus);
        return kAooOk;
    }
    if (size < sizeof(AooInt32)){
        LOG_ERROR("Opus: couldn't read format - not enough data!");
        return kAooErrorBadArgument;
    }
    if (*fmtsize < sizeof(AooFormatOpus)){
        LOG_ERROR("Opus: output format storage too small");
        return kAooErrorBadArgument;
    }
    auto fmt = (AooFormatOpus *)f;
    fmt->applicationType = aoo::from_bytes<AooInt32>(buf);
    *fmtsize = sizeof(AooFormatOpus);

    return kAooOk;
}

AooCodecInterface g_interface = {
    // encoder
    Encoder_new,
    Encoder_free,
    Encoder_control,
    Encoder_encode,
    // decoder
    Decoder_new,
    Decoder_free,
    Decoder_control,
    Decoder_decode,
    // helper
    serialize,
    deserialize,
    nullptr
};

Encoder::Encoder(OpusMSEncoder *state, size_t size, const AooFormatOpus& f) {
    interface = &g_interface;
    state_ = state;
    size_ = size;
    numChannels_ = f.header.numChannels;
}

Decoder::Decoder(OpusMSDecoder *state, size_t size, const AooFormatOpus& f) {
    interface = &g_interface;
    state_ = state;
    size_ = size;
    numChannels_ = f.header.numChannels;
}

} // namespace

void aoo_opusLoad(AooCodecRegisterFunc fn,
                  AooLogFunc log, AooAllocFunc alloc){
    fn(kAooCodecOpus, &g_interface);
    // the Opus codec is always statically linked, so we can simply use the
    // internal log function and allocator
}

void aoo_opusUnload() {}
