set(AB_TEST_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(dbgen dbgen.cpp)
target_include_directories(dbgen PRIVATE
    ${AB_TEST_INCLUDES})
target_link_libraries(dbgen
    traceparser
    formula
    table
    common
    CONAN_PKG::fmt
    CONAN_PKG::abseil
    CONAN_PKG::boost)

add_executable(abtest abtesttool.cpp)
target_include_directories(abtest PRIVATE
    ${AB_TEST_INCLUDES})
target_link_libraries(abtest
    CONAN_PKG::fmt
    CONAN_PKG::abseil
    CONAN_PKG::boost)
