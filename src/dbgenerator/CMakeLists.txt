set(DB_GENERATOR_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(dbgen dbgen.cpp)
target_include_directories(dbgen PRIVATE
    ${DB_GENERATOR_INCLUDES})
target_link_libraries(dbgen
    traceparser
    formula
    table
    common
    fmt::fmt
    CONAN_PKG::abseil
    CONAN_PKG::boost)
