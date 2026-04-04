/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others. 
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#pragma once

#include "aoo.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*/////////////////// PCM codec ////////////////////////*/

#define AOO_CODEC_PCM "pcm"

typedef enum
{
    AOO_PCM_INT16,
    AOO_PCM_INT24,
    AOO_PCM_FLOAT32,
    AOO_PCM_FLOAT64,
    AOO_PCM_BITDEPTH_SIZE
} aoo_pcm_bitdepth;

typedef struct aoo_format_pcm
{
    aoo_format header;
    int32_t bitdepth;
} aoo_format_pcm;

AOO_API void aoo_codec_pcm_setup(aoo_codec_registerfn fn);

#ifdef __cplusplus
} // extern "C"
#endif
