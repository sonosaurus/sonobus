/* Copyright (c) 2021 Christof Ressi
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#pragma once

/*---------- global compile time settings ----------*/

/** \brief AOO_NET support */
#ifndef AOO_NET
# define AOO_NET 1
#endif

/** \brief log level */
#ifndef AOO_LOG_LEVEL
#define AOO_LOG_LEVEL kAooLogLevelWarning
#endif

/** \brief custom allocator support  */
#ifndef AOO_CUSTOM_ALLOCATOR
# define AOO_CUSTOM_ALLOCATOR 0
#endif

/** \brief default UDP packet size */
#ifndef AOO_PACKET_SIZE
# define AOO_PACKET_SIZE 512
#endif

/** \brief max. UDP packet size */
#ifndef AOO_MAX_PACKET_SIZE
# define AOO_MAX_PACKET_SIZE 4096
#endif

#if AOO_PACKET_SIZE > AOO_MAX_PACKET_SIZE
#error "AOO_PACKET_SIZE must exceed AOO_MAX_PACKET_SIZE"
#endif

/** \brief default size of the RT memory pool */
#ifndef AOO_MEM_POOL_SIZE
 #define AOO_MEM_POOL_SIZE (1 << 20) /* 1 MB */
#endif

/** \brief clip audio output between -1 and 1 */
#ifndef AOO_CLIP_OUTPUT
# define AOO_CLIP_OUTPUT 0
#endif

/** \brief use built-in Opus codec */
#ifndef AOO_USE_CODEC_OPUS
#define AOO_USE_CODEC_OPUS 1
#endif

/*--------------- compile time debug options ----------------*/

/** \brief debug memory usage */
#ifndef AOO_DEBUG_MEMORY
# define AOO_DEBUG_MEMORY 0
#endif

/** \brief debug the state of the DLL filter */
#ifndef AOO_DEBUG_DLL
# define AOO_DEBUG_DLL 0
#endif

/** \brief debug incoming/outgoing data */
#ifndef AOO_DEBUG_DATA
# define AOO_DEBUG_DATA 0
#endif

/** \brief debug data resending */
#ifndef AOO_DEBUG_RESEND
# define AOO_DEBUG_RESEND 0
#endif

/** \brief debug the state of the xrun detector */
#ifndef AOO_DEBUG_TIMER
# define AOO_DEBUG_TIMER 0
#endif

/** \brief debug the state of the dynamic resampler */
#ifndef AOO_DEBUG_RESAMPLING
# define AOO_DEBUG_RESAMPLING 0
#endif

/** \brief debug the state of the jitter buffer */
#ifndef AOO_DEBUG_JITTER_BUFFER
# define AOO_DEBUG_JITTER_BUFFER 0
#endif

/** \brief debug stream message scheduling */
#ifndef AOO_DEBUG_STREAM_MESSAGE
# define AOO_DEBUG_STREAM_MESSAGE 0
#endif

/** \brief debug relay messages */
#ifndef AOO_DEBUG_RELAY
#define AOO_DEBUG_RELAY 0
#endif

/** \brief debug client messages */
#ifndef AOO_DEBUG_CLIENT_MESSAGE
#define AOO_DEBUG_CLIENT_MESSAGE 0
#endif
