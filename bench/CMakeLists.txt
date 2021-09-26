set(BENCH_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(tablebench
    tablebench.cpp)
target_include_directories(tablebench PRIVATE
    ${BENCH_INCLUDES})
target_link_libraries(tablebench
    CONAN_PKG::benchmark
    table)