# C interface test
add_executable(test_c_interface "test_c_interface.c")

# force C standard
set(TEST_C_STANDARD "c89")

set(STATIC_LIBAOO TRUE)

set(TEST_LIBS ${AOO_OSCPACK_LDFLAGS} ${AOO_MD5_LDFLAGS} ${AOO_OPUS_LDFLAGS})
if (STATIC_LIBAOO)
    target_compile_definitions(test_c_interface PUBLIC -DAOO_STATIC)
    list(APPEND TEST_LIBS AOO_STATIC)
else()
    list(APPEND TEST_LIBS AOO)
endif()

if (NOT MSVC)
    # C89 does not have "inline" and GCC/Clang warn about unused static non-inline functions
    target_compile_options(test_c_interface PUBLIC "-Wno-unused-function")
    # force C standard
    target_compile_options(test_c_interface PUBLIC "-std=${TEST_C_STANDARD}" "-pedantic")
endif()

target_link_libraries(test_c_interface ${TEST_LIBS} ${LIBS})
if (WIN32)
    target_link_libraries(test_c_interface "ws2_32")
endif()

# RT memory pool test
add_executable(test_rt_memory_pool "test_rt_memory_pool.cpp")
target_sources(test_rt_memory_pool PUBLIC "${CMAKE_SOURCE_DIR}/aoo/src/rt_memory_pool.hpp")
if (NOT MSVC)
    target_link_libraries(test_rt_memory_pool "pthread")
endif()
