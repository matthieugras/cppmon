add_subdirectory(common)
add_subdirectory(parsing)

set(MAIN_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})

# Table
add_library(table STATIC table.cpp)
target_include_directories(table PUBLIC
    ${MAIN_INCLUDES})
target_link_libraries(table
    CONAN_PKG::fmt
    CONAN_PKG::abseil
    CONAN_PKG::boost
    common)

# Formula
add_library(formula STATIC formula.cpp)
target_include_directories(formula PUBLIC
    ${MAIN_INCLUDES})
target_link_libraries(formula
    CONAN_PKG::fmt
    CONAN_PKG::abseil
    CONAN_PKG::boost
    CONAN_PKG::nlohmann_json
    common)

# Monitor
add_library(monitor STATIC
    monitor_driver.cpp
    monitor.cpp)
target_include_directories(monitor PUBLIC
    ${MAIN_INCLUDES})
target_link_libraries(monitor
    CONAN_PKG::abseil
    CONAN_PKG::boost
    traceparser
    table
    formula)

# Main executable
add_executable(cppmon cppmon.cpp)
target_include_directories(cppmon PUBLIC
    ${MAIN_INCLUDES})
target_link_libraries(cppmon
    CONAN_PKG::fmt
    CONAN_PKG::abseil
    traceparser
    monitor)









