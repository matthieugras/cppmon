set(PARSER_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})
add_library(traceparser STATIC traceparser.cpp)
target_include_directories(traceparser PUBLIC
    ${PARSER_INCLUDES})
target_link_libraries(traceparser
    CONAN_PKG::fmt
    CONAN_PKG::boost
    CONAN_PKG::abseil
    foonathan::lexy
    common)