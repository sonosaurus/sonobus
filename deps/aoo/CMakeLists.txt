cmake_minimum_required (VERSION 3.1)

message(STATUS "Project: aoo")
project(aoo)

include(GNUInstallDirs)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(cmake/test_atomic.cmake)
include(cmake/test_linker_flag.cmake)

# built everything (including Opus) with -fPIC
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# dependencies
include_directories(deps)

# Opus
# NOTE: build it first, so we don't pollute it with our own settings
option(AOO_CODEC_OPUS "use Opus codec" ON)
if (AOO_CODEC_OPUS)
    option(AOO_SYSTEM_OPUS "use system provided Opus library" OFF)
    if (AOO_SYSTEM_OPUS)
        set(AOO_OPUS_LDFLAGS "opus" CACHE STRING "Opus linker flags")
        message(STATUS "Opus library: ${AOO_OPUS_LDFLAGS}")
    else()
        message(STATUS "\n*** Opus library ***\n")

        add_subdirectory("deps/opus" EXCLUDE_FROM_ALL)

        # HACK to force local opus lib (needed for MSVC)
        add_library(opus_local ALIAS opus)

        set(AOO_OPUS_LDFLAGS opus_local)
    endif()
endif()

message(STATUS "\n*** Global settings ***\n")

# oscpack
option(AOO_SYSTEM_OSCPACK "use system provided oscpack library" OFF)
if (AOO_SYSTEM_OSCPACK)
	set(AOO_OSCPACK_LDFLAGS "oscpack" CACHE STRING "oscpack linker flags")
endif()

# md5
option(AOO_SYSTEM_MD5 "use system provided md5 library" OFF)
if (AOO_SYSTEM_MD5)
	set(AOO_MD5_LDFLAGS "md5" CACHE STRING "md5 linker flags")
endif()

if(UNIX AND NOT APPLE AND NOT MINGW)
	set(LINUX TRUE)
endif()

# some MinGW setups don't define WIN32!
if (MINGW AND NOT WIN32)
    message(WARNING "Your MinGW setup does not define WIN32")
    set(WIN32 TRUE)
endif()

# check for Clang or AppleClang
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	set(CMAKE_COMPILER_IS_CLANG 1)
endif()
message(STATUS "CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

message(STATUS "CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")

if(LINUX AND CMAKE_COMPILER_IS_GNUCXX)
	option(AOO_STATIC_LIBS "link with static libraries (libstdc++ and libgcc)" ON)
endif()
if(MINGW)
	option(AOO_STATIC_LIBS "link with static libraries (libstdc++, libgcc and phread)" ON)
	set(CMAKE_EXECUTABLE_SUFFIX ".exe")
endif()

# logging
set(AOO_LOG_LEVEL "Warning" CACHE STRING "compile time log level")
set_property(CACHE AOO_LOG_LEVEL PROPERTY STRINGS "None;Error;Warning;Verbose;Debug")
message(STATUS "Log level: ${AOO_LOG_LEVEL}")
add_definitions(-DAOO_LOG_LEVEL=kAooLogLevel${AOO_LOG_LEVEL})

# Windows paths
if (WIN32)
    # check if "Program Files (x86)" exists (64-bit Windows) and if we compile for 32-bit
    set(_pf_x86 "ProgramFiles(x86)")
    if (DEFINED ENV{${_pf_x86}} AND (CMAKE_SIZEOF_VOID_P EQUAL 4))
        set(PROGRAMFILES $ENV{${_pf_x86}})
    else()
        set(PROGRAMFILES $ENV{PROGRAMFILES})
    endif()
    set(APPDATA $ENV{APPDATA})
    set(LOCALAPPDATA $ENV{LOCALAPPDATA})
endif()

# use C++17 if possible, otherwise fallback to C++14
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)

# feature tests
if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANG)
    add_compile_options("-fvisibility=hidden")

    check_cxx_compiler_flag("-msse" HAVE_CXX_SSE)
    check_cxx_compiler_flag("-msse2" HAVE_CXX_SSE2)
    check_cxx_compiler_flag("-msse3" HAVE_CXX_SSE3)
    check_cxx_compiler_flag("-msse4" HAVE_CXX_SSE4)
    check_cxx_compiler_flag("-mfpmath=sse" HAVE_CXX_FPMATH_SSE)

    test_linker_flag("-latomic" AOO_HAVE_LIB_ATOMIC)

    # atomic double support
    test_atomic(ATOMIC_DOUBLE "atomic_double.cpp")
    # message(STATUS "atomic doubles supported: ${AOO_HAVE_ATOMIC_DOUBLE}")

    # atomic 64-bit integer support
    test_atomic(ATOMIC_INT64 "atomic_int64.cpp")
    # message(STATUS "atomic 64-bit integers supported: ${AOO_HAVE_ATOMIC_INT64}")
endif()

# global optimization flags

add_compile_options("-ffast-math" "-funroll-loops" "-fomit-frame-pointer")
if (HAVE_CXX_SSE)
    add_compile_options("-msse")
endif()
if (HAVE_CXX_SSE2)
    add_compile_options("-msse2")
endif()
if (HAVE_CXX_SSE3)
    add_compile_options("-msse3")
endif()
# people still own old machines that don't support SSE4
if (FALSE)
    check_cxx_compiler_flag("-msse4" HAVE_CXX_SSE4)
    if (HAVE_CXX_SSE4)
        add_compile_options("-msse4")
    endif()
endif()
if (HAVE_CXX_FPMATH_SSE)
    add_compile_options("-mfpmath=sse")
endif()
option(AOO_NATIVE "optimize for this machine (not portable!)" OFF)
if (AOO_NATIVE)
    add_compile_options("-march=native")
endif()

# platform specific compile options

# clang++:
if (CMAKE_COMPILER_IS_CLANG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

# MinGW
if (MINGW)
    add_compile_options("-mstackrealign")
endif()

# atomics
if (AOO_HAVE_ATOMIC_DOUBLE)
    add_definitions(-DAOO_HAVE_ATOMIC_DOUBLE)
else()
    message(STATUS "No built-in support for atomic doubles - "
        "will be emulated in software")
endif()
if (AOO_HAVE_ATOMIC_INT64)
    add_definitions(-DAOO_HAVE_ATOMIC_INT64)
else()
    message(STATUS "No built-in support for atomic 64-bit integers - "
        "will be emulated in software")
endif()
# link with "libatomic" if available and necessary
if (AOO_HAVE_LIB_ATOMIC AND
        (NOT AOO_HAVE_ATOMIC_DOUBLE OR NOT AOO_HAVE_ATOMIC_INT64))
    list(APPEND LIBS "-latomic")
endif()

# opus support
message(STATUS "Use Opus codec: ${AOO_CODEC_OPUS}")
add_definitions(-DUSE_CODEC_OPUS=$<BOOL:${AOO_CODEC_OPUS}>)
if (AOO_CODEC_OPUS AND (NOT AOO_SYSTEM_OPUS))
    # because of a quirk in the opus source code structure,
    # we can't include "opus/opus_multistream.h"
    add_definitions(-DAOO_OPUS_MULTISTREAM_H=<opus/include/opus_multistream.h>)
endif()

# networking support
option(AOO_NET "Build with networking support" ON)
message(STATUS "Use AOO NET: ${AOO_NET}")
add_definitions(-DUSE_AOO_NET=$<BOOL:${AOO_NET}>)

# platform specific linker flags
if (LINUX)
    list(APPEND LIBS "-pthread")
    if(AOO_STATIC_LIBS)
        list(APPEND LIBS "-static-libstdc++" "-static-libgcc")
    endif()
    set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)
endif()
if (MINGW)
    if (AOO_STATIC_LIBS)
        list(APPEND LIBS "-static-libstdc++" "-static-libgcc" "-static -lpthread")
    else()
        list(APPEND LIBS "-lpthread")
    endif()
    if (AOO_CODEC_OPUS)
        list(APPEND LIBS "ssp") # for fortified functions
    endif()
endif()
if (APPLE)
    list(APPEND LIBS "-lpthread")
endif()

# headers
include_directories(".")
include_directories("include")

# AOO library
message(STATUS "\n*** AOO library ***\n")
add_subdirectory("aoo")

# Pd external
option(AOO_BUILD_PD_EXTERNAL "build Pd external" ON)
if (AOO_BUILD_PD_EXTERNAL)
    if (NOT AOO_NET)
        message(FATAL_ERROR "Pd external requires AOO_NET")
    endif()
    message(STATUS "\n*** Pd external ***\n")
    add_subdirectory("pd")
endif()

# SC extension
option(AOO_BUILD_SC_EXTENSION "build SC extension" ON)
if (AOO_BUILD_SC_EXTENSION)
    if (NOT AOO_NET)
        message(FATAL_ERROR "SC extension requires AOO_NET")
    endif()
    message(STATUS "\n*** SC extension ***\n")
    add_subdirectory("sc")
endif()


# test suite
option(AOO_BUILD_TEST_SUITE "build test suite" ON)
if (AOO_BUILD_TEST_SUITE)
    message(STATUS "\n*** Test suite ***\n")
    add_subdirectory("tests")
endif()
