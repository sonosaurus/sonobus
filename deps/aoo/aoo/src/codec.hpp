#pragma once

#include "aoo/aoo.h"
#include "imp.hpp"

#include <memory>

namespace aoo {

class encoder;
class decoder;

class codec {
public:
    codec(const aoo_codec *c)
        : codec_(c){}

    const char *name() const {
        return codec_->name;
    }

    std::unique_ptr<encoder> create_encoder() const;

    std::unique_ptr<decoder> create_decoder() const;

    aoo_error serialize(const aoo_format& f,
                        char *buf, int32_t &n) const {
        return codec_->serialize(&f, buf, &n);
    }

    aoo_error deserialize(const aoo_format& header,
                          const char *data, int32_t n,
                          aoo_format& f, int32_t size) const {
        return codec_->deserialize(&header, data, n, &f, size);
    }
protected:
    const aoo_codec *codec_;
};

class base_codec : public codec {
public:
    base_codec(const aoo_codec *c, void *obj)
        : codec(c), obj_(obj){}
    base_codec(const aoo_codec&) = delete;

    int32_t nchannels() const { return nchannels_; }

    int32_t samplerate() const { return samplerate_; }

    int32_t blocksize() const { return blocksize_; }
protected:
    void *obj_;
    int32_t nchannels_ = 0;
    int32_t samplerate_ = 0;
    int32_t blocksize_ = 0;

    void save_format(const aoo_format& f){
        nchannels_ = f.nchannels;
        samplerate_ = f.samplerate;
        blocksize_ = f.blocksize;
    }
};

class encoder : public base_codec {
public:
    using base_codec::base_codec;

    ~encoder(){
        codec_->encoder_free(obj_);
    }

    aoo_error set_format(aoo_format& fmt){
        auto result = codec_->encoder_ctl(obj_,
            AOO_CODEC_SET_FORMAT, &fmt, sizeof(aoo_format));
        if (result == AOO_OK){
            save_format(fmt); // after validation!
        }
        return result;
    }

    aoo_error get_format(aoo_format& fmt, size_t size) const {
        return codec_->encoder_ctl(obj_, AOO_CODEC_GET_FORMAT,
                                   &fmt, size);
    }

    bool compare(const aoo_format& fmt) const {
        return codec_->encoder_ctl(obj_, AOO_CODEC_FORMAT_EQUAL,
                                   (void *)&fmt, fmt.size);
    }

    aoo_error reset() {
        return codec_->encoder_ctl(obj_, AOO_CODEC_RESET, nullptr, 0);
    }

    aoo_error encode(const aoo_sample *s, int32_t n, char *buf, int32_t &size){
        return codec_->encoder_encode(obj_, s, n, buf, &size);
    }
};

inline std::unique_ptr<encoder> codec::create_encoder() const {
    return std::make_unique<encoder>(codec_, codec_->encoder_new());
}

class decoder : public base_codec {
public:
    using base_codec::base_codec;
    ~decoder(){
        codec_->decoder_free(obj_);
    }

    aoo_error set_format(aoo_format& fmt){
        auto result = codec_->decoder_ctl(obj_,
            AOO_CODEC_SET_FORMAT, &fmt, sizeof(aoo_format));
        if (result == AOO_OK){
            save_format(fmt); // after validation!
        }
        return result;
    }

    aoo_error get_format(aoo_format& fmt, size_t size) const {
        return codec_->decoder_ctl(obj_, AOO_CODEC_GET_FORMAT,
                                   &fmt, size);
    }

    bool compare(const aoo_format& fmt) const {
        return codec_->decoder_ctl(obj_, AOO_CODEC_FORMAT_EQUAL,
                                   (void *)&fmt, fmt.size);
    }

    aoo_error reset() {
        return codec_->decoder_ctl(obj_, AOO_CODEC_RESET, nullptr, 0);
    }

    aoo_error decode(const char *buf, int32_t size, aoo_sample *s, int32_t &n){
        return codec_->decoder_decode(obj_, buf, size, s, &n);
    }
};

inline std::unique_ptr<decoder> codec::create_decoder() const {
    return std::make_unique<decoder>(codec_, codec_->decoder_new());
}

const codec * find_codec(const char * name);

} // aoo
