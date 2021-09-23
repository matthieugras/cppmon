cmake_minimum_required(VERSION 3.20)

include(CheckIPOSupported)

project(cppmon VERSION 0.1
    DESCRIPTION "C++ Monitor for MFOTL"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wconversion -Wsign-conversion")
check_ipo_supported(LANGUAGES C CXX)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(MAIN_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})

# Table
add_library(table STATIC table.cpp)
target_include_directories(table PRIVATE
    ${MAIN_INCLUDES})
target_link_libraries(table
    CONAN_PKG::fmt
    CONAN_PKG::abseil
    CONAN_PKG::boost)

# Formula
add_library(formula STATIC formula.cpp)
target_include_directories(formula PRIVATE
    ${MAIN_INCLUDES})
target_link_libraries(formula
    CONAN_PKG::fmt
    CONAN_PKG::abseil
    CONAN_PKG::boost
    CONAN_PKG::nlohmann_json)

# Monitor
add_library(monitor STATIC
    monitor_driver.cpp
    monitor.cpp)
target_include_directories(monitor PRIVATE
    ${MAIN_INCLUDES})
target_link_libraries(monitor
    CONAN_PKG::abseil
    CONAN_PKG::boost
    table
    formula)

# Main executable
add_executable(cppmon cppmon.cpp)
target_include_directories(cppmon PRIVATE
    ${MAIN_INCLUDES})
target_link_libraries(cppmon
    CONAN_PKG::fmt
    CONAN_PKG::abseil
    monitor)

# Tests
add_executable(testexe
    test_main.cpp
    tabletest.cpp
    formulatest.cpp
    monitortest.cpp)
target_include_directories(testexe PRIVATE
    ${MAIN_INCLUDES})
target_link_libraries(testexe
    CONAN_PKG::gtest
    CONAN_PKG::fmt
    monitor
    formula
    table)

# Benchmark
add_executable(tablebench
    tablebench.cpp)
target_include_directories(tablebench PRIVATE
    ${MAIN_INCLUDES})
target_link_libraries(tablebench
    CONAN_PKG::benchmark
    table)









