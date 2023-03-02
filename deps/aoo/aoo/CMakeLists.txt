# compile time options
option(AOO_CUSTOM_ALLOCATOR "build with custom allocator support" OFF)
message(STATUS "Use custom allocator: ${AOO_CUSTOM_ALLOCATOR}")

set(AOO_PACKET_SIZE 512 CACHE STRING "default UDP packet size")
message(STATUS "Default UDP packet size: ${AOO_PACKET_SIZE}")

set(AOO_MAX_PACKET_SIZE 4096 CACHE STRING "max. UDP packet size")
message(STATUS "Max. UDP packet size: ${AOO_MAX_PACKET_SIZE}")

if (AOO_PACKET_SIZE GREATER AOO_MAX_PACKET_SIZE)
    message(FATAL_ERROR "AOO_PACKET_SIZE must not exceed AOO_MAX_PACKET_SIZE")
endif()

option(AOO_DYNAMIC_RESAMPLING "enable/disable dynamic resampling" OFF)
message(STATUS "Use dynamic resampling: ${AOO_DYNAMIC_RESAMPLING}")

set(AOO_DLL_BANDWIDTH 0.0001 CACHE STRING "default DLL filter bandwidth")
message(STATUS "Default DLL filter bandwidth: ${AOO_DLL_BANDWIDTH}")

option(AOO_XRUN_DETECTION "enable/disable xrun detection" ON)
message(STATUS "Use xrun detection: ${AOO_XRUN_DETECTION}")

set(AOO_TIMER_TOLERANCE 0.25 CACHE STRING "default timer tolerance")
message(STATUS "Default timer tolerance: ${AOO_TIMER_TOLERANCE}")

option(AOO_BINARY_DATA_MSG "use binary data messages" ON)
message(STATUS "Use binary data message: ${AOO_BINARY_DATA_MSG}")

# compile time debugging options
option(AOO_DEBUG_MEMORY "debug memory usage" OFF)

option(AOO_DEBUG_DATA "debug data transmission" OFF)

option(AOO_DEBUG_RESEND "debug data retransmission" OFF)

option(AOO_DEBUG_DLL "debug time DLL filter" OFF)

option(AOO_DEBUG_TIMER "debug timer" OFF)

option(AOO_DEBUG_RESAMPLER "debug resampler" OFF)

option(AOO_DEBUG_JITTER_BUFFER "debug jitter buffer" OFF)

option(AOO_DEBUG_STREAM_MESSAGE "debug jitter buffer" OFF)

option(AOO_DEBUG_RELAY "debug relay" OFF)

option(AOO_DEBUG_CLIENT_MESSAGE "debug client message" OFF)

option(AOO_CLIENT_SIMULATE "network simulation in the client" OFF)

set(OPTIONS
    -DAOO_CUSTOM_ALLOCATOR=$<BOOL:${AOO_CUSTOM_ALLOCATOR}>
    -DAOO_PACKET_SIZE=${AOO_PACKET_SIZE}
    -DAOO_MAX_PACKET_SIZE=${AOO_MAX_PACKET_SIZE}
    -DAOO_DYNAMIC_RESAMPLING=$<BOOL:${AOO_DYNAMIC_RESAMPLING}>
    -DAOO_DLL_BANDWIDTH=${AOO_DLL_BANDWIDTH}
    -DAOO_XRUN_DETECTION=$<BOOL:${AOO_XRUN_DETECTION}>
    -DAOO_TIMER_TOLERANCE=${AOO_TIMER_TOLERANCE}
    -DAOO_BINARY_DATA_MSG=$<BOOL:${AOO_BINARY_DATA_MSG}>
    -DAOO_DEBUG_MEMORY=$<BOOL:${AOO_DEBUG_MEMORY}>
    -DAOO_DEBUG_DATA=$<BOOL:${AOO_DEBUG_DATA}>
    -DAOO_DEBUG_RESEND=$<BOOL:${AOO_DEBUG_RESEND}>
    -DAOO_DEBUG_DLL=$<BOOL:${AOO_DEBUG_DLL}>
    -DAOO_DEBUG_TIMER=$<BOOL:${AOO_DEBUG_TIMER}>
    -DAOO_DEBUG_RESAMPLER=$<BOOL:${AOO_DEBUG_RESAMPLER}>
    -DAOO_DEBUG_STREAM_MESSAGE=$<BOOL:${AOO_DEBUG_STREAM_MESSAGE}>
    -DAOO_DEBUG_JITTER_BUFFER=$<BOOL:${AOO_DEBUG_JITTER_BUFFER}>
    -DAOO_DEBUG_RELAY=$<BOOL:${AOO_DEBUG_RELAY}>
    -DAOO_DEBUG_CLIENT_MESSAGE=$<BOOL:${AOO_DEBUG_CLIENT_MESSAGE}>
    -DAOO_CLIENT_SIMULATE=$<BOOL:${AOO_CLIENT_SIMULATE}>
)

set(INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/../include/aoo")
set(COMMON "${CMAKE_CURRENT_SOURCE_DIR}/../common")
set(DEPS "${CMAKE_CURRENT_SOURCE_DIR}/../deps")

file(GLOB HEADERS "${INCLUDE}/*.h" "${INCLUDE}/*.hpp"
     "${INCLUDE}/codec/aoo_pcm.h" "src/*.hpp" "${COMMON}/*.hpp")

file(GLOB SOURCES "src/*.cpp" "src/codec/pcm.cpp" "${COMMON}/*.cpp")

if (NOT AOO_SYSTEM_OSCPACK)
    # only build required files in "osc" folder
    set(OSC "${DEPS}/oscpack/osc")
    list(APPEND HEADERS
        "${OSC}/OscException.h"
        "${OSC}/OscHostEndianness.h"
        "${OSC}/OscOutboundPacketStream.h"
        "${OSC}/OscReceivedElements.h"
        "${OSC}/OscTypes.h")
    list(APPEND SOURCES
        "${OSC}/OscOutboundPacketStream.cpp"
        "${OSC}/OscReceivedElements.cpp"
        "${OSC}/OscTypes.cpp")
endif()

if (AOO_CODEC_OPUS)
    list(APPEND HEADERS "${INCLUDE}/codec/aoo_opus.h")
    list(APPEND SOURCES "src/codec/opus.cpp")
endif()

if (AOO_NET)
    file(GLOB NET_HEADERS "src/net/*.hpp")

    file(GLOB NET_SOURCES "src/net/*.cpp")

    list(APPEND HEADERS ${NET_HEADERS})
    list(APPEND SOURCES ${NET_SOURCES})

    if (NOT AOO_SYSTEM_MD5)
        list(APPEND HEADERS "${DEPS}/md5/md5.h")
        list(APPEND SOURCES "${DEPS}/md5/md5.c")
    endif()
endif()

# AOO static library
# NOTE: always build static library because we need it for Pd, SC, tests, etc.
# AOO_BUILD_STATIC_LIBRARY only installs it
option(AOO_BUILD_STATIC_LIBRARY "build static AOO library" ON)

add_library(AOO_STATIC STATIC ${HEADERS} ${SOURCES})
if (NOT MSVC)
    # for MSVC we keep the "aoo_static" name to avoid name clash with
    # "aoo.lib" import library
    set_target_properties(AOO_STATIC PROPERTIES OUTPUT_NAME "aoo")
endif()

target_compile_definitions(AOO_STATIC PUBLIC ${OPTIONS} -DAOO_STATIC)
target_link_libraries(AOO_STATIC ${AOO_OSCPACK_LDFLAGS} ${AOO_OPUS_LDFLAGS})
if (AOO_NET)
    target_link_libraries(AOO_STATIC ${AOO_MD5_LDFLAGS})
endif()

# AOO shared library
option(AOO_BUILD_SHARED_LIBRARY "build shared AOO library" ON)

if (AOO_BUILD_SHARED_LIBRARY)
    add_library(AOO SHARED ${HEADERS} ${SOURCES})
    set_target_properties(AOO PROPERTIES OUTPUT_NAME "aoo")

    target_compile_definitions(AOO PUBLIC -DAOO_BUILD ${OPTIONS})
    if (WIN32)
        target_compile_definitions(AOO PUBLIC -DDLL_EXPORT)
        target_link_libraries(AOO "ws2_32")
    endif()
    target_link_libraries(AOO ${AOO_OPUS_LDFLAGS} ${LIBS})
endif()

if (AOO_BUILD_SHARED_LIBRARY OR AOO_BUILD_STATIC_LIBRARY)
    # install library files
    if (AOO_BUILD_SHARED_LIBRARY)
        list(APPEND TARGET_LIST AOO)
    endif()
    if (AOO_BUILD_STATIC_LIBRARY)
        list(APPEND TARGET_LIST AOO_STATIC)
    endif()
    install(TARGETS ${TARGET_LIST}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    # install public headers (note the trailing slash!)
    install(DIRECTORY "${INCLUDE}/"
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/aoo")
endif()
