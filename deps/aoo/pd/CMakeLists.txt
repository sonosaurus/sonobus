cmake_minimum_required (VERSION 2.8)

set(PROJECT "pd")
set(FILENAME "aoo")
message(STATUS "---\n*** Pd external ***")

set(INCLUDE "../include/aoo")
set(COMMON "../common")

file(GLOB HEADERS "${INCLUDE}/*.h" "${INCLUDE}/*.hpp"
    "${COMMON}/*.hpp" "src/*.hpp")

file(GLOB SRC ${HEADERS} "src/*.cpp")

# Pd directories:
if (NOT PD_DIR)
    if (WIN32 OR MINGW)
        set(PD_INCLUDEDIR "${PROGRAMFILES}/Pd/src" CACHE PATH "Pd header files")
    elseif(APPLE)
        # isn't there a better way?
        file(GLOB _globresults "/Applications/Pd*.app/Contents/Resources/src")
        list(GET _globresults 0 _globresult)
        set(PD_INCLUDEDIR ${_globresult} CACHE PATH "Pd header files")
    else()
        set(PD_INCLUDEDIR "/usr/include/pd" CACHE PATH "Pd header files")
    endif()
    if (WIN32 OR MINGW)
        set(PD_BINDIR "${PROGRAMFILES}/Pd/bin" CACHE PATH "Pd binaries")
    endif()
else()
    set(PD_INCLUDEDIR "${PD_DIR}/src" CACHE PATH "Pd header files" FORCE)
    if (WIN32 OR MINGW)
        set(PD_BINDIR "${PD_DIR}/bin" CACHE PATH "Pd binaries" FORCE)
    endif()
endif()
include_directories(${PD_INCLUDEDIR})
message(STATUS "PD_INCLUDEDIR: ${PD_INCLUDEDIR}")
if (WIN32 OR MINGW)
    message(STATUS "PD_BINDIR: ${PD_BINDIR}")
endif()

# install directory:
if (WIN32 OR MINGW)
    set(PD_INSTALLDIR "${APPDATA}/Pd" CACHE PATH "Installation directoy")
elseif(APPLE)
    set(PD_INSTALLDIR "~/Library/Pd" CACHE PATH "Installation directory")
else()
    set(PD_INSTALLDIR "/usr/local/lib/pd-externals" CACHE PATH "Installation directory")
endif()
message(STATUS "PD_INSTALLDIR: ${PD_INSTALLDIR}")

# plugin extension
set(CMAKE_SHARED_MODULE_PREFIX "") # no prefix!
if (NOT PD_EXTENSION)
    if (WIN32 OR MINGW)
        set(CMAKE_SHARED_MODULE_SUFFIX ".dll")
    elseif(APPLE)
        set(CMAKE_SHARED_MODULE_SUFFIX ".pd_darwin")
    elseif(LINUX)
        set(CMAKE_SHARED_MODULE_SUFFIX ".pd_linux")
    endif()
else()
    set(CMAKE_SHARED_MODULE_SUFFIX ".${PD_EXTENSION}")
endif()
message(STATUS "PD_EXTENSION: ${CMAKE_SHARED_MODULE_SUFFIX}")

# the Pd external
add_library(${PROJECT} MODULE ${SRC})
set_target_properties(${PROJECT} PROPERTIES OUTPUT_NAME ${FILENAME})
target_compile_definitions(${PROJECT} PUBLIC "-DAOO_STATIC")
target_link_libraries(${PROJECT} ${AOO_STATIC} ${AOONET_STATIC}
    ${MD5_LIB} ${OSCPACK_LIB} ${CODEC_OPUS_LIB} ${LIBS})

# platform specific definitions and options
if (WIN32 OR MINGW)
    if (MINGW)
        set_target_properties(${PROJECT} PROPERTIES LINK_FLAGS "--enable-auto-import")
        target_link_libraries(${PROJECT} "${PD_BINDIR}/pd.dll")
    else()
        target_link_libraries(${PROJECT} "${PD_BINDIR}/pd.lib")
    endif()
    target_link_libraries(${PROJECT} "ws2_32")
elseif(APPLE)
    set_target_properties(${PROJECT} PROPERTIES LINK_FLAGS "-undefined suppress -flat_namespace -bundle")
else()
    set_target_properties(${PROJECT} PROPERTIES LINK_FLAGS "-rdynamic -fPIC")
endif()

install(TARGETS ${PROJECT} DESTINATION "${PD_INSTALLDIR}/${FILENAME}")
install(FILES "aoo_receive~-help.pd" "aoo_send~-help.pd" "aoo_net-help.pd" "../readme.rst" "../LICENSE"
        DESTINATION "${PD_INSTALLDIR}/${FILENAME}")
