function(test_atomic feature file)
    if (DEFINED AOO_HAVE_${feature})
        return()
    endif()

    message(STATUS "Testing ${feature} support")

    # Not all platforms support lock-free atomics of all types.
    #
    # Usually, compilers will implement non-lock-free atomics with something
    # like a global spinlock. If for some reason they don't support it,
    # we use our own emulated atomics implementation (see "common/sync.hpp")
    #
    # If C++17 is available and REQUIRE_ALWAYS_LOCKFREE is defined, we get a
    # compilation error if the atomics are not always lock-free, thus forcing
    # our own implementation of emulated atomics. For C++14 we could also do
    # a runtime test, but it wouldn't work for cross compiling...
    
    if (AOO_HAVE_LIB_ATOMIC)
        # link with "libatomic" in case the atomics are not lockfree
        set(LIBATOMIC "-latomic")
    endif()
    try_compile(RESULT_VAR
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${file}"
        OUTPUT_VARIABLE COMPILE_OUTPUT
        CXX_STANDARD 17
        LINK_LIBRARIES ${LIBATOMIC})

    if (RESULT_VAR)
        message(STATUS "- ok")
    else()
        message(STATUS "- failed")
        message(VERBOSE ${COMPILE_OUTPUT})
    endif()

    set(AOO_HAVE_${feature} ${RESULT_VAR} CACHE INTERNAL "${feature} support")
endfunction()
