# Makefile to build library 'aoo' for Pure Data.
# Needs Makefile.pdlibbuilder as helper makefile for platform-dependent build
# settings and rules.
#
# use : make PDDIR=/path/to/pure-data/
#
# The following command will build the external and install the distributable
# files into a subdirectory called build :
#
# make install PDDIR=/path/to/pure-data/ PDLIBDIR=./build

# library name
lib.name = aoo

# input source files
class.sources = \
    src/aoo_pack~.c \
    src/aoo_unpack~.c \
    src/aoo_route.c \
    src/aoo_send~.c \
    src/aoo_receive~.c \
    src/aoo_server.c \
    src/aoo_client.c \
    $(empty)

## parametrizing the build
loglevel=2
use_codec_opus=yes
aoo_timefilter_bandwidth=0.0001
aoo_timefilter_tolerance=0.25
aoo_debug_dll=0
aoo_debug_resampling=0
aoo_debug_block_buffer=0

## (external) dependencies
AOO = ../lib
DEPS = ../deps

## oscpack
## set 'system_oscpack' to 'yes' (and set the appropriate
## OSCPACK_CFLAGS/OSCPACK_LIBS) to use a system-provided
## oscpack implementation
OSCPACK = $(DEPS)/oscpack

ifneq ($(system_oscpack),yes)
OSCPACK_CFLAGS = -I$(OSCPACK)/..
OSCPACK_LIBS =
endif

## md5
## set 'system_md5' to 'yes' (and set the appropriate
## MD5_CFLAGS/MD5_LIBS) to use a system-provided
## md5 implementation
MD5 = $(DEPS)/md5

ifneq ($(system_md5),yes)
MD5_CFLAGS = -I$(MD5)/..
MD5_LIBS =
endif

cflags = \
    -I$(AOO) \
    -I$(AOO)/src \
    -I$(AOO)/src/lib \
    -Wno-unused -Wno-unused-parameter \
    -fvisibility=hidden \
    -DNDEBUG \
    -DAOO_STATIC \
    -DLOGLEVEL=$(loglevel) \
    -DAOO_TIMEFILTER_BANDWIDTH=$(aoo_timefilter_bandwidth) \
    -DAOO_TIMEFILTER_TOLERANCE=$(aoo_timefilter_tolerance) \
    -DAOO_DEBUG_DLL=$(aoo_debug_dll) \
    -DAOO_DEBUG_RESAMPLING=$(aoo_debug_resampling) \
    -DAOO_DEBUG_BLOCK_BUFFER=$(aoo_debug_block_buffer) \
    $(empty)

ifneq ($(aoo_timefilter_check),)
cflags += \
    -DAOO_TIMEFILTER_CHECK=$(aoo_timefilter_check)
endif

cflags += \
    $(OSCPACK_CFLAGS) \
    $(MD5_CFLAGS) \
    $(empty)

ldlibs =

make-lib-executable=yes

lib.setup.sources = src/aoo_setup.c

shared.sources = \
    src/aoo_node.c \
    src/aoo_common.c \
    src/aoo_net.c \
    $(AOO)/src/common.cpp \
    $(AOO)/src/sync.cpp \
    $(AOO)/src/time.cpp \
    $(AOO)/src/source.cpp \
    $(AOO)/src/sink.cpp \
    $(AOO)/src/server.cpp \
    $(AOO)/src/client.cpp \
    $(AOO)/src/net_utils.cpp \
    $(AOO)/src/codec_pcm.cpp \
    $(empty)

ifneq ($(system_oscpack),yes)
shared.sources += \
    $(OSCPACK)/osc/OscTypes.cpp \
    $(OSCPACK)/osc/OscReceivedElements.cpp \
    $(OSCPACK)/osc/OscOutboundPacketStream.cpp \
    $(empty)
endif
ifneq ($(system_md5),yes)
shared.sources += \
    $(MD5)/md5.c \
    $(empty)
endif

ldlibs += $(OSCPACK_LIBS) $(MD5_LIBS)

# Opus
# set 'use_codec_opus' to 'yes' to enable the Opus audio codec
OPUS_CFLAGS = -DUSE_CODEC_OPUS=1
OPUS_LIBS = -lopus

ifeq ($(use_codec_opus),yes)
cflags += $(OPUS_CFLAGS)
ldlibs += $(OPUS_LIBS)
shared.sources += $(AOO)/src/codec_opus.cpp
endif

# hack to set the C++ standard only for the shared library
$(shared.sources:.cpp=.o): cxx.flags += -std=c++14

# platform specific options
define forWindows
    ldlibs += -lws2_32 -static-libstdc++ -static-libgcc
endef

define forDarwin
    cflags += -mmacosx-version-min=10.9
    ldlibs += -lpthread
endef

define forLinux
    ldlibs += -pthread
endef

# all extra files to be included in binary distribution of the library
datafiles = aoo_receive~-help.pd aoo_send~-help.pd aoo_server-help.pd ../readme.rst

# include Makefile.pdlibbuilder from submodule directory 'pd-lib-builder'
PDLIBBUILDER_DIR=pd-lib-builder
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder
