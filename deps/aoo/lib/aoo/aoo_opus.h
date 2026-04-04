/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others. 
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#pragma once

#include "aoo.h"

#include <opus/opus_multistream.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*/////////////////// Opus codec ////////////////////////*/

#define AOO_CODEC_OPUS "opus"

typedef struct aoo_format_opus
{
    aoo_format header;
    int32_t bitrate; // 0: default
    int32_t complexity; // 0: default
    int32_t signal_type;
    int32_t application_type; 
} aoo_format_opus;

AOO_API void aoo_codec_opus_setup(aoo_codec_registerfn fn);

#ifdef __cplusplus
} // extern "C"
#endif
