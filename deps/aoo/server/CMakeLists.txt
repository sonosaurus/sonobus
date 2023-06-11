set(INCLUDE "../include/aoo")
set(COMMON "../common")

add_executable(SERVER "main.cpp")
set_target_properties(SERVER PROPERTIES OUTPUT_NAME "aooserver")
target_compile_definitions(SERVER PUBLIC -DAOO_STATIC)
target_link_libraries(SERVER AOO_STATIC ${LIBS})

# platform specific definitions and options
if (WIN32)
    target_link_libraries(SERVER "ws2_32")
endif()

install(TARGETS SERVER
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
