set(SHM_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})

add_library(shm_shared_state STATIC shm_shared_state.cpp)
target_include_directories(shm_shared_state PUBLIC ${SHM_INCLUDES})
target_link_libraries(shm_shared_state PUBLIC
    CONAN_PKG::fmt
    CONAN_PKG::boost)

add_library(shm_event_consumer STATIC shm_event_consumer.cpp)
target_include_directories(shm_event_consumer PUBLIC ${SHM_INCLUDES})
target_link_libraries(shm_event_consumer PUBLIC shm_shared_state)

add_library(shm_event_producer SHARED shm_event_producer.cpp)
target_include_directories(shm_event_producer PRIVATE
    ${SHM_INCLUDES})
target_link_libraries(shm_event_producer PRIVATE
    CONAN_PKG::fmt
    shm_shared_state)
target_link_options(shm_event_producer PRIVATE "LINKER:--exclude-libs,ALL")
