/* Copyright (c) 2021 Christof Ressi
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/** \file
 * \brief AOO codec API
 *
 * This header contains the API for adding new audio codecs to the AOO library.
 */

#pragma once

#include "aoo_defines.h"

AOO_PACK_BEGIN

/*-----------------------------------------------------*/

/** \brief base class for all codec classes */
typedef struct AooCodec
{
    struct AooCodecInterface *interface;
} AooCodec;

/** \brief codec constructor
 *
 * \param format the desired format; validated and updated on success
 * \param error error code on failure
 * \return AooCodec instance on success, NULL on failure
 */
typedef AooCodec * (AOO_CALL *AooCodecNewFunc)(
        AooFormat *format,
        AooError *error
);

/** \brief codec destructor */
typedef void (AOO_CALL *AooCodecFreeFunc)(AooCodec *codec);

/** \brief encode audio samples to bytes */
typedef AooError (AOO_CALL *AooCodecEncodeFunc)(
        /** the encoder instance */
        AooCodec *encoder,
        /** [in] input samples (interleaved) */
        const AooSample *inSamples,
        /** [in] number of samples */
        AooInt32 numSamples,
        /** [out] output buffer */
        AooByte *outData,
        /** [in,out] max. buffer size in bytes
         * (updated to actual size) */
        AooInt32 *outSize
);

/** \brief decode bytes to samples */
typedef AooError (AOO_CALL *AooCodecDecodeFunc)(
        /** the decoder instance */
        AooCodec *decoder,
        /** [in] input data */
        const AooByte *inData,
        /** [in] input data size in bytes */
        AooInt32 numBytes,
        /** [out] output samples (interleaved) */
        AooSample *outSamples,
        /** [in,out] max. number of samples
         * (updated to actual number) */
        AooInt32 *numSamples
);

/** \brief type for AOO codec controls */
typedef AooInt32 AooCodecCtl;

/** \brief AOO codec control constants
 *
 * Negative values are reserved for generic controls;
 * codec specific controls must be positiv */
enum AooCodecControls
{
    /** reset the codec state (`NULL`) */
    kAooCodecCtlReset = -1000
};

/** \brief codec control function */
typedef AooError (AOO_CALL *AooCodecControlFunc)
(
        /** the encoder/decoder instance */
        AooCodec *codec,
        /** the control constant */
        AooCodecCtl ctl,
        /** pointer to value */
        void *data,
        /** the value size */
        AooSize size
);

/** \brief serialize format extension
 *
 * (= everything after the AooFormat header).
 * On success writes the format extension to the given buffer
 */
typedef AooError (AOO_CALL *AooCodecSerializeFunc)(
        /** [in] the source format */
        const AooFormat *format,
        /** [out] extension buffer; `NULL` returns the required buffer size. */
        AooByte *buffer,
        /** [in,out] max. buffer size; updated to actual resp. required size */
        AooInt32 *bufsize
);

/** \brief deserialize format extension
 *
 * (= everything after the AooFormat header).
 * On success writes the format extension to the given format structure.
 *
 * \note This function does *not* automatically update the `size` member
 * of the format structure, but you can simply point `fmtsize` to it.
 */
typedef AooError (AOO_CALL *AooCodecDeserializeFunc)(
        /** [in] the extension buffer */
        const AooByte *buffer,
        /** [in] the extension buffer size */
        AooInt32 bufsize,
        /** [out] destination format structure; `NULL` returns the required format size */
        AooFormat *format,
        /** max. format size; updated to actual resp. required size */
        AooInt32 *fmtsize
);

/** \brief interface to be implemented by AOO codec classes */
typedef struct AooCodecInterface
{
    /* encoder methods */
    AooCodecNewFunc encoderNew;
    AooCodecFreeFunc encoderFree;
    AooCodecControlFunc encoderControl;
    AooCodecEncodeFunc encoderEncode;
    /* decoder methods */
    AooCodecNewFunc decoderNew;
    AooCodecFreeFunc decoderFree;
    AooCodecControlFunc decoderControl;
    AooCodecDecodeFunc decoderDecode;
    /* free functions */
    AooCodecSerializeFunc serialize;
    AooCodecDeserializeFunc deserialize;
    void *future;
} AooCodecInterface;

/*----------------- helper functions ----------------------*/

/** \brief encode audio samples to bytes
 * \see AooCodecEncodeFunc */
AOO_INLINE AooError AooEncoder_encode(AooCodec *enc,
                           const AooSample *input, AooInt32 numSamples,
                           AooByte *output, AooInt32 *numBytes)
{
    return enc->interface->encoderEncode(enc, input, numSamples, output, numBytes);
}

/** \brief control encoder instance
 * \see AooCodecControlFunc */
AOO_INLINE AooError AooEncoder_control(AooCodec *enc,
                           AooCodecCtl ctl, void *data, AooSize size)
{
    return enc->interface->encoderControl(enc, ctl, data, size);
}

/** \brief reset encoder state */
AOO_INLINE AooError AooEncoder_reset(AooCodec *enc) {
    return enc->interface->encoderControl(enc, kAooCodecCtlReset, NULL, 0);
}

/** \brief decode bytes to audio samples
 * \see AooCodecDecodeFunc */
AOO_INLINE AooError AooDecoder_decode(AooCodec *dec,
                           const AooByte *input, AooInt32 numBytes,
                           AooSample *output, AooInt32 *numSamples)
{
    return dec->interface->decoderDecode(dec, input, numBytes, output, numSamples);
}

/** \brief control decoder instance
 * \see AooCodecControlFunc */
AOO_INLINE AooError AooDecoder_control(AooCodec *dec,
                           AooCodecCtl ctl, void *data, AooSize size)
{
    return dec->interface->decoderControl(dec, ctl, data, size);
}

/** \brief reset decoder state */
AOO_INLINE AooError AooDecoder_reset(AooCodec *dec)
{
    return dec->interface->encoderControl(dec, kAooCodecCtlReset, NULL, 0);
}

/*----------------- register codecs -----------------------*/

/** \brief register an external codec plugin */
AOO_API AooError AOO_CALL aoo_registerCodec(
        const AooChar *name, const AooCodecInterface *codec);

/** \brief The type of #aoo_registerCodec, which gets passed to the
 * entry function of the codec plugin to register itself. */
typedef AooError (AOO_CALL *AooCodecRegisterFunc)(
        const AooChar *name,                // codec name
        const AooCodecInterface *interface  // codec interface
);

/** \brief type of entry function for codec plugin module
 *
 * \note AOO doesn't support dynamic plugin loading out of the box,
 * but it is quite easy to implement on your own.
 * You just have to put one or more codecs in a shared library and export
 * a single function of type AooCodecSetupFunc with the name `aoo_load`:
 *
 *     void aoo_load(AooCodecRegisterFunc fn, AooLogFunc log, const AooAllocator *alloc);
 *
 * In your host application, you would then scan directories for shared libraries,
 * check if they export a function named `aoo_load`, and if yes, call it with a
 * pointer to #aoo_registerCodec and (optionally) a log function and custom allocator.
 */
typedef AooError (AOO_CALL *AooCodecLoadFunc)
        (const AooCodecRegisterFunc *registerFunc,
         AooLogFunc logFunc, AooAllocFunc allocFunc);

/** \brief type of exit function for codec plugin module
 *
 * Your codec plugin can optionally export a function `aoo_unload` which should be
 * called before program exit to properly release shared resources.
 */
typedef AooError (AOO_CALL *AooCodecUnloadFunc)(void);


/*-------------------------------------------------------------------------------------*/

AOO_PACK_END
