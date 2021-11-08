set(SHM_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})

add_library(shm_shared_state STATIC shm_shared_state.cpp)
target_include_directories(shm_shared_state PUBLIC ${SHM_INCLUDES})
target_link_libraries(shm_shared_state PUBLIC
    CONAN_PKG::fmt
    CONAN_PKG::boost)

add_library(cppmon_event_source SHARED cppmon_event_source.cpp)
target_include_directories(cppmon_event_source PUBLIC
    ${SHM_INCLUDES})
target_link_libraries(cppmon_event_source PRIVATE
    CONAN_PKG::fmt
    shm_shared_state)
target_link_options(cppmon_event_source PRIVATE "LINKER:--exclude-libs,ALL")
