cmake_minimum_required(VERSION 3.20)

include(CheckIPOSupported)
include(FetchContent)

project(cppmon VERSION 0.1
    DESCRIPTION "C++ Monitor for MFOTL"
    LANGUAGES C CXX)

option(USE_JEMALLOC "Use jemalloc" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

string(REPLACE "-O2" "-O3" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
string(APPEND CMAKE_CXX_FLAGS_DEBUG " -O2")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wconversion -Wsign-conversion -Wno-unused-command-line-argument")
check_ipo_supported(LANGUAGES C CXX)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

FetchContent_Declare(lexy
    GIT_REPOSITORY https://github.com/foonathan/lexy.git
    GIT_TAG a59057dbcd4dd87d89cf1a3cc149ce3a7d130ba3)

FetchContent_MakeAvailable(lexy)

add_subdirectory(src)
add_subdirectory(tests)
