set(STATIC_LIBAOO TRUE)

set(TEST_LIBS ${AOO_OSCPACK_LDFLAGS} ${AOO_MD5_LDFLAGS} ${AOO_OPUS_LDFLAGS})
if (STATIC_LIBAOO)
    list(APPEND TEST_LIBS AOO_STATIC ${LIBS})
else()
    list(APPEND TEST_LIBS AOO ${LIBS})
endif()
if (WIN32)
    list(APPEND TEST_LIBS "ws2_32")
endif()

if (STATIC_LIBAOO)
    set(TEST_FLAGS "-DAOO_STATIC")
endif()

# C interface test
add_executable(test_c_interface "test_c_interface.c")
set(TEST_C_STANDARD "c89")
target_compile_definitions(test_c_interface PUBLIC ${TEST_FLAGS})
if (NOT MSVC)
    # C89 does not have "inline" and GCC/Clang warn about unused static non-inline functions
    target_compile_options(test_c_interface PUBLIC "-Wno-unused-function")
    # force C standard
    target_compile_options(test_c_interface PUBLIC "-std=${TEST_C_STANDARD}" "-pedantic")
endif()
target_link_libraries(test_c_interface ${TEST_LIBS})

# RT memory pool test
add_executable(test_rt_memory_pool "test_rt_memory_pool.cpp")
target_compile_definitions(test_rt_memory_pool PUBLIC ${TEST_FLAGS})
target_link_libraries(test_rt_memory_pool ${TEST_LIBS})
