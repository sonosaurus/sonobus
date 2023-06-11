/* Copyright (c) 2021 Christof Ressi
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/** \file
 * \brief dummy codec settings
 */

#pragma once

#include "aoo/aoo_config.h"
#include "aoo/aoo_defines.h"
#include "aoo/aoo_types.h"

#include <string.h>

AOO_PACK_BEGIN

/*--------------------------------------------------*/

#define kAooCodecNull "null"

/** \brief null codec format */
typedef struct AooFormatNull
{
    AooFormat header;
} AooFormatNull;

/*------------------------------------------------*/

/** \brief initialize AooFormatNull structure */
AOO_INLINE void AooFormatNull_init(
        AooFormatNull *fmt, AooInt32 numChannels,
        AooInt32 sampleRate, AooInt32 blockSize)
{
    AOO_STRUCT_INIT(&fmt->header, AooFormatNull, header);
    fmt->header.numChannels = numChannels;
    fmt->header.sampleRate = sampleRate;
    fmt->header.blockSize = blockSize;
    strcpy(fmt->header.codecName, kAooCodecNull);
}

/*-----------------------------------------------*/

AOO_PACK_END
