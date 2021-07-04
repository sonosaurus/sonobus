cmake_minimum_required (VERSION 2.8)

set(PROJECT "sc")
set(FILENAME "aoo")
message(STATUS "---\n*** SuperCollider extension  ***")

set(COMMON "../common")

file(GLOB HEADERS "src/*.hpp" "${COMMON}/net_utils.hpp" "${COMMON}/time.hpp")

file(GLOB SRC ${HEADERS} "src/*.cpp")

# SuperCollider API:
set(SC_INCLUDEDIR "" CACHE PATH "SuperCollider source files")
message(STATUS "SC_INCLUDEDIR: ${SC_INCLUDEDIR}")
include_directories(${SC_INCLUDEDIR}/include/plugin_interface)
include_directories(${SC_INCLUDEDIR}/include/common)
include_directories(${SC_INCLUDEDIR}/plugin_interface)
include_directories(${SC_INCLUDEDIR}/common)

# Installation path
# install directory:
if (WIN32 OR MINGW)
    set(SC_INSTALLDIR "${LOCALAPPDATA}/SuperCollider/Extensions/" CACHE PATH "Installation directory")
elseif(APPLE)
    set(SC_INSTALLDIR "~/Library/Application Support/SuperCollider/Extensions/" CACHE PATH "Installation directoy")
else()
    set(SC_INSTALLDIR "~/.local/share/SuperCollider/Extensions/" CACHE PATH "Installation directoy")
endif()
message(STATUS "SC_INSTALLDIR: ${SC_INSTALLDIR}")

# plugin extension
set(CMAKE_SHARED_MODULE_PREFIX "") # no prefix!
if(APPLE OR MINGW OR WIN32)
set(CMAKE_SHARED_MODULE_SUFFIX ".scx")
endif()

option(SUPERNOVA "Build plugins for supernova" OFF)
if (SUPERNOVA)
	# for <nova-tt/spin_lock.h>
	include_directories(${SC_INCLUDEDIR}/external_libraries/nova-tt)
	# for <boost/atomic.hpp>
	include_directories(${SC_INCLUDEDIR}/external_libraries/boost)
endif()
message(STATUS "SUPERNOVA: ${SUPERNOVA}")

# the UGens
add_library(${PROJECT} MODULE ${SRC})
set_target_properties(${PROJECT} PROPERTIES OUTPUT_NAME ${FILENAME})
target_compile_definitions(${PROJECT} PUBLIC "-DAOO_STATIC")
target_link_libraries(${PROJECT} ${AOO_STATIC} ${AOONET_STATIC}
    ${MD5_LIB} ${OSCPACK_LIB} ${CODEC_OPUS_LIB} ${LIBS})
if (WIN32 OR MINGW)
    target_link_libraries(${PROJECT} "ws2_32")
endif()

if(SUPERNOVA)
    add_library(${PROJECT}_supernova MODULE ${SRC})
    set_target_properties(${PROJECT} PROPERTIES OUTPUT_NAME ${FILENAME}_supernova)
    target_compile_definitions(${PROJECT}_supernova PUBLIC "-DSUPERNOVA=1" "-DAOO_STATIC")
    target_link_libraries(${PROJECT}_supernova ${AOO_STATIC} ${AOONET_STATIC}
        ${MD5_LIB} ${OSCPACK_LIB} ${CODEC_OPUS_LIB} ${LIBS})
    if (WIN32 OR MINGW)
        target_link_libraries(${PROJECT}_supernova "ws2_32")
    endif()
endif()

# installation
install(TARGETS ${PROJECT} DESTINATION "${SC_INSTALLDIR}/${FILENAME}/plugins")
if (SUPERNOVA)
    install(TARGETS ${PROJECT}_supernova DESTINATION "${SC_INSTALLDIR}/${FILENAME}/plugins")
endif()
install(DIRECTORY classes DESTINATION "${SC_INSTALLDIR}/${FILENAME}")
install(DIRECTORY HelpSource DESTINATION "${SC_INSTALLDIR}/${FILENAME}")
install(FILES "../readme.rst" "../LICENSE" DESTINATION "${SC_INSTALLDIR}/${FILENAME}")
