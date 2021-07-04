/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others. 
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#pragma once

#include "aoo/aoo.h"

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
    // OPUS_APPLICATION_VOIP, OPUS_APPLICATION_AUDIO or
    // OPUS_APPLICATION_RESTRICTED_LOWDELAY
    int32_t application_type;
    // bitrate in bits/s, OPUS_BITRATE_MAX or OPUS_AUTO
    int32_t bitrate;
    // complexity 0-10 or OPUS_AUTO
    int32_t complexity;
    // OPUS_SIGNAL_VOICE, OPUS_SIGNAL_MUSIC or OPUS_AUTO
    int32_t signal_type;
} aoo_format_opus;

#ifdef __cplusplus
} // extern "C"
#endif
