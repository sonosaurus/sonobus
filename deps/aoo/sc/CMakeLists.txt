set(COMMON "../common")

file(GLOB HEADERS "src/*.hpp" "${COMMON}/net_utils.hpp" "${COMMON}/time.hpp")

file(GLOB SRC ${HEADERS} "src/*.cpp")

# SuperCollider API:
set(SC_INCLUDEDIR "" CACHE PATH "SC include directory")
message(STATUS "SC include directory: ${SC_INCLUDEDIR}")
include_directories(${SC_INCLUDEDIR}/include/plugin_interface)
include_directories(${SC_INCLUDEDIR}/include/common)
include_directories(${SC_INCLUDEDIR}/plugin_interface)
include_directories(${SC_INCLUDEDIR}/common)

# Installation path
# install directory:
if (WIN32)
    set(SC_INSTALLDIR "${LOCALAPPDATA}/SuperCollider/Extensions/" CACHE PATH "SC install directory")
elseif(APPLE)
    set(SC_INSTALLDIR "~/Library/Application Support/SuperCollider/Extensions/" CACHE PATH "SC install directoy")
else()
    set(SC_INSTALLDIR "~/.local/share/SuperCollider/Extensions/" CACHE PATH "SC install directoy")
endif()
message(STATUS "SC install directory: ${SC_INSTALLDIR}")

# plugin extension
set(CMAKE_SHARED_MODULE_PREFIX "") # no prefix!
if (APPLE OR WIN32)
	set(CMAKE_SHARED_MODULE_SUFFIX ".scx")
endif()

option(SC_SUPERNOVA "Build plugins for SC_SUPERNOVA" OFF)
if (SC_SUPERNOVA)
	# for <nova-tt/spin_lock.h>
	include_directories(${SC_INCLUDEDIR}/external_libraries/nova-tt)
	# for <boost/atomic.hpp>
	include_directories(${SC_INCLUDEDIR}/external_libraries/boost)
endif()
message(STATUS "SC_SUPERNOVA: ${SC_SUPERNOVA}")

# the UGens
add_library(SC MODULE ${SRC})
set_target_properties(SC PROPERTIES OUTPUT_NAME "aoo")
target_compile_definitions(SC PUBLIC -DAOO_STATIC)
target_link_libraries(SC AOO_STATIC ${AOO_OPUS_LDFLAGS} ${LIBS})
if (WIN32)
    target_link_libraries(SC "ws2_32")
endif()

if (SC_SUPERNOVA)
    add_library(SUPERNOVA MODULE ${SRC})
    set_target_properties(SUPERNOVA PROPERTIES OUTPUT_NAME "aoo_supernova")
    target_compile_definitions(SUPERNOVA PUBLIC -DSUPERNOVA -DAOO_STATIC)
    target_link_libraries(SUPERNOVA AOO_STATIC ${AOO_OPUS_LDFLAGS} ${LIBS})
    if (WIN32)
        target_link_libraries(SUPERNOVA "ws2_32")
    endif()
endif()

# installation
install(TARGETS SC DESTINATION "${SC_INSTALLDIR}/aoo/plugins")
if (SC_SUPERNOVA)
    install(TARGETS SUPERNOVA DESTINATION "${SC_INSTALLDIR}/aoo/plugins")
endif()
install(DIRECTORY classes DESTINATION "${SC_INSTALLDIR}/aoo")
install(DIRECTORY HelpSource DESTINATION "${SC_INSTALLDIR}/aoo")
install(FILES "../README.md" "../LICENSE" DESTINATION "${SC_INSTALLDIR}/aoo")
