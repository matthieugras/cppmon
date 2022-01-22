set(BENCH_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})

if(ENABLE_SOCK_INTF)
  add_executable(uds_benchmark uds_benchmark.cpp)
  target_include_directories(uds_benchmark PRIVATE ${BENCH_INCLUDES})
  target_link_libraries(uds_benchmark monitor socket_deserialization
                        CONAN_PKG::fmt CONAN_PKG::abseil CONAN_PKG::benchmark)

  if(USE_JEMALLOC)
    target_link_libraries(uds_benchmark CONAN_PKG::jemalloc)
  endif()
  install(TARGETS uds_benchmark)
endif()

add_executable(parse_benchmark parse_benchmark.cpp)
target_include_directories(parse_benchmark PRIVATE ${BENCH_INCLUDES})
target_link_libraries(parse_benchmark monitor traceparser common
                      CONAN_PKG::benchmark)
if (USE_JEMALLOC)
  target_link_libraries(parse_benchmark CONAN_PKG::jemalloc)
endif()
install(TARGETS parse_benchmark)
