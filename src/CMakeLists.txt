add_subdirectory(common)
add_subdirectory(parsing)
add_subdirectory(abtesting)

set(MAIN_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})

# Table
add_library(table STATIC table.cpp)
target_include_directories(table PUBLIC
    ${MAIN_INCLUDES})
target_link_libraries(table
    fmt::fmt
    CONAN_PKG::abseil
    CONAN_PKG::boost
    common)

# Formula
add_library(formula STATIC formula.cpp)
target_include_directories(formula PUBLIC
    ${MAIN_INCLUDES})
target_link_libraries(formula
    fmt::fmt
    CONAN_PKG::abseil
    CONAN_PKG::boost
    CONAN_PKG::nlohmann_json
    common)

# Monitor
add_library(monitor STATIC
    monitor_driver.cpp
    monitor.cpp
    since_impl.cpp
    until_impl.cpp)
target_include_directories(monitor PUBLIC
    ${MAIN_INCLUDES})
target_link_libraries(monitor
    CONAN_PKG::abseil
    CONAN_PKG::boost
    common
    traceparser
    table
    formula)

# Main executable
add_executable(cppmon cppmon.cpp)
target_include_directories(cppmon PUBLIC
    ${MAIN_INCLUDES})

if (USE_JEMALLOC)
  target_link_libraries(cppmon
      CONAN_PKG::jemalloc)
endif ()

target_link_libraries(cppmon
    fmt::fmt
    CONAN_PKG::abseil
    traceparser
    monitor)









