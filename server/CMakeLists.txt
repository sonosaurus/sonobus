
cmake_minimum_required(VERSION 3.15)

# C++17 required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (WIN32)
    #set (CMAKE_GENERATOR_TOOLSET ClangCL)
    #static linking in Windows
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# If we are compiling for Mac OS we want to target OS versions down to 10.9
option(UniversalBinary "Build universal binary for mac" ON)

if (APPLE)
    set (CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE INTERNAL "")
    if (UniversalBinary)
        set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE INTERNAL "")
        #set(CMAKE_OSX_ARCHITECTURES arm64 x86_64)        
    endif()
endif()



# The top-level CMakeLists.txt file for a project must contain a literal, direct call to the
# `project()` command. `project()` sets up some helpful variables that describe source/binary
# directories, and the current project version. This is a standard CMake command.

project(sonobus_server VERSION 2.0.0)

set(BUILDVERSION 1)


# include AOO (which includes opus)
set(AOO_BUILD_SHARED_LIBRARY OFF CACHE INTERNAL "" FORCE)
set(AOO_BUILD_PD_EXTERNAL OFF CACHE INTERNAL "" FORCE)
set(AOO_BUILD_SC_EXTENSION OFF CACHE INTERNAL "" FORCE)
set(AOO_BUILD_TEST_SUITE OFF CACHE INTERNAL "" FORCE)
set(AOO_CODEC_OPUS ON CACHE INTERNAL "" FORCE)
set(AOO_SYSTEM_OPUS ON CACHE INTERNAL "" FORCE)

if (APPLE)
    add_definitions(-DAOO_OPUS_MULTISTREAM_H="../../../../mac/include/opus/opus_multistream.h")
elseif (WIN32)
    add_definitions(-DAOO_OPUS_MULTISTREAM_H="../../../../windows/include/opus/opus_multistream.h")
endif()

add_subdirectory(../subdeps/aoo aoobuild EXCLUDE_FROM_ALL )

# nlohmann JSON
set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")
add_subdirectory(../subdeps/nlohmann_json jsonbuild EXCLUDE_FROM_ALL)




add_executable(sonobus_server 
	main.cpp
	SonoBusAooMetaData.cpp 
	SonoBusAooMetaData.h)


target_link_libraries(sonobus_server 
	PRIVATE 
	aoo aoo_common aoo_binary
	nlohmann_json::nlohmann_json
)


install(TARGETS sonobus_server)
