# simplified version which only builds the "osc" folder

file(GLOB OSCPACK_SRC "osc/*.h" "osc/*.cpp")

add_library(${OSCPACK_LIB} STATIC ${OSCPACK_SRC})
