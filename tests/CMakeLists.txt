set(TEST_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(testexe
    test_main.cpp
    tabletest.cpp
    formulatest.cpp
    monitortest.cpp
    binarybuffertest.cpp)
target_include_directories(testexe PRIVATE
    ${TEST_INCLUDES})
target_link_libraries(testexe
    CONAN_PKG::gtest
    fmt::fmt
    common
    monitor
    formula
    table)