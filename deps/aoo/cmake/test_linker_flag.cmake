function(test_linker_flag flag var)
    if (DEFINED ${var})
        return()
    endif()

    message(STATUS "Testing linker flag '${flag}'")

    try_compile(RESULT_VAR
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/empty.cpp"
        OUTPUT_VARIABLE COMPILE_OUTPUT
        CXX_STANDARD 17
        LINK_LIBRARIES ${flag})

    if (RESULT_VAR)
        message(STATUS "- ok")
    else()
        message(STATUS "- failed")
        message(VERBOSE ${COMPILE_OUTPUT})
    endif()

    set(${var} ${RESULT_VAR} CACHE INTERNAL "linker flag '${flag}' supported")
endfunction()
