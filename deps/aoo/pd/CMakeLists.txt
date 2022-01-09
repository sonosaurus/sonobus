set(INCLUDE "../include/aoo")
set(COMMON "../common")

file(GLOB HEADERS "${INCLUDE}/*.h" "${INCLUDE}/*.hpp"
    "${COMMON}/*.hpp" "src/*.hpp")

file(GLOB SOURCES ${HEADERS} "src/*.cpp")

# Pd directories:
if (NOT PD_DIR)
    if (WIN32)
        set(PD_INCLUDEDIR "${PROGRAMFILES}/Pd/src" CACHE PATH "Pd include directory")
    elseif(APPLE)
        # isn't there a better way?
        file(GLOB _globresults "/Applications/Pd*.app/Contents/Resources/src")
        list(GET _globresults 0 _globresult)
        set(PD_INCLUDEDIR ${_globresult} CACHE PATH "Pd include directory")
    else()
        set(PD_INCLUDEDIR "/usr/include/pd" CACHE PATH "Pd include directory")
    endif()
    if (WIN32)
        set(PD_BINDIR "${PROGRAMFILES}/Pd/bin" CACHE PATH "Pd binary directory")
    endif()
else()
    set(PD_INCLUDEDIR "${PD_DIR}/src" CACHE PATH "Pd include directory" FORCE)
    if (WIN32)
        set(PD_BINDIR "${PD_DIR}/bin" CACHE PATH "Pd binary directory" FORCE)
    endif()
endif()
include_directories(${PD_INCLUDEDIR})
message(STATUS "Pd include directory: ${PD_INCLUDEDIR}")
if (WIN32)
    message(STATUS "Pd binary directory: ${PD_BINDIR}")
endif()

# install directory:
if (WIN32)
    set(PD_INSTALLDIR "${APPDATA}/Pd" CACHE PATH "Pd install directoy")
elseif(APPLE)
    set(PD_INSTALLDIR "~/Library/Pd" CACHE PATH "Pd install directory")
else()
    set(PD_INSTALLDIR "/usr/local/lib/pd-externals" CACHE PATH "Pd install directory")
endif()
message(STATUS "Pd install directory: ${PD_INSTALLDIR}")

# plugin extension
if (WIN32)
    set(PD_EXTENSION "dll" CACHE STRING "Pd extension")
elseif (APPLE)
    set(PD_EXTENSION "pd_darwin" CACHE STRING "Pd extension")
elseif (LINUX)
    set(PD_EXTENSION "pd_linux" CACHE STRING "Pd extension")
endif()
message(STATUS "Pd extension: ${PD_EXTENSION}")
set(CMAKE_SHARED_MODULE_SUFFIX ".${PD_EXTENSION}")
set(CMAKE_SHARED_MODULE_PREFIX "") # no prefix!

# the Pd external
add_library(PD MODULE ${SOURCES})
set_target_properties(PD PROPERTIES OUTPUT_NAME "aoo")
target_compile_definitions(PD PUBLIC -DAOO_STATIC)
target_link_libraries(PD AOO_STATIC ${AOO_OPUS_LDFLAGS} ${LIBS})

option(PD_MULTI_INSTANCE "Multi-instance support (for libpd)" OFF)
message(STATUS "Pd multi-instance support: ${PD_MULTI_INSTANCE}")
if (PD_MULTI_INSTANCE)
    target_compile_definitions(PD PUBLIC -DPDINSTANCE)
endif()

# platform specific definitions and options
if (WIN32)
    if (MINGW)
        set_target_properties(PD PROPERTIES LINK_FLAGS "--enable-auto-import")
        target_link_libraries(PD "${PD_BINDIR}/pd.dll")
    else()
        target_link_libraries(PD "${PD_BINDIR}/pd.lib")
    endif()
    target_link_libraries(PD "ws2_32")
elseif(APPLE)
    set_target_properties(PD PROPERTIES LINK_FLAGS "-undefined suppress -flat_namespace -bundle")
else()
    set_target_properties(PD PROPERTIES LINK_FLAGS "-rdynamic -fPIC")
endif()

install(TARGETS PD DESTINATION "${PD_INSTALLDIR}/aoo")
install(FILES "aoo_receive~-help.pd" "aoo_send~-help.pd" "aoo_net-help.pd" "../README.md" "../LICENSE"
        DESTINATION "${PD_INSTALLDIR}/aoo")
