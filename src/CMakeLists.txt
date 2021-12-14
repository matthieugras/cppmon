add_subdirectory(common)
add_subdirectory(parsing)
add_subdirectory(differential_testing)
if(ENABLE_SOCK_INTF)
  add_subdirectory(socket_interface)
endif()

set(MAIN_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
configure_file(config.h.in config.h @ONLY)

# Table
add_library(table STATIC table.cpp)
target_include_directories(table PUBLIC ${MAIN_INCLUDES})
target_link_libraries(table CONAN_PKG::fmt CONAN_PKG::abseil CONAN_PKG::boost
                      common)

# Formula
add_library(formula STATIC formula.cpp)
target_include_directories(formula PUBLIC ${MAIN_INCLUDES})
target_link_libraries(formula CONAN_PKG::fmt CONAN_PKG::abseil CONAN_PKG::boost
                      CONAN_PKG::nlohmann_json common)

# Monitor
add_library(
  monitor STATIC monitor.cpp aggregation_impl.cpp temporal_aggregation_impl.cpp
                 since_impl.cpp until_impl.cpp database.cpp)
target_include_directories(monitor PUBLIC ${MAIN_INCLUDES})
target_link_libraries(
  monitor
  CONAN_PKG::abseil
  CONAN_PKG::boost
  common
  traceparser
  table
  formula)

# Main executable
set(MAIN_SOURCES cppmon.cpp file_monitor_driver.cpp monitor_driver.cpp)
if(ENABLE_SOCK_INTF)
  set(MAIN_SOURCES ${MAIN_SOURCES} uds_monitor_driver.cpp)
endif()
add_executable(cppmon ${MAIN_SOURCES})
target_include_directories(cppmon PUBLIC ${MAIN_INCLUDES})
install(TARGETS cppmon)
if(USE_JEMALLOC)
  target_link_libraries(cppmon CONAN_PKG::jemalloc)
endif()
if(ENABLE_SOCK_INTF)
  target_link_libraries(cppmon socket_deserialization)
endif()
target_link_libraries(cppmon CONAN_PKG::fmt CONAN_PKG::abseil traceparser
                      monitor)
