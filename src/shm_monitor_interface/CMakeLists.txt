set(SHM_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})
set(SHM_EXPORT_INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/export)

add_library(socket_serialization STATIC serialization.cpp)
target_include_directories(socket_serialization PUBLIC ${SHM_INCLUDES})
target_link_libraries(socket_serialization PUBLIC
    CONAN_PKG::fmt
    CONAN_PKG::boost)

add_library(socket_deserialization STATIC deserialization.cpp)
target_include_directories(socket_deserialization PUBLIC ${SHM_INCLUDES})
target_link_libraries(socket_deserialization PUBLIC
    CONAN_PKG::fmt
    CONAN_PKG::boost
    CONAN_PKG::abseil
    traceparser
    common)

add_library(cppmon_event_source SHARED socket_event_source.cpp)
target_include_directories(cppmon_event_source
    PRIVATE ${SHM_INCLUDES}
    INTERFACE ${SHM_EXPORT_INTERFACE})
target_link_libraries(cppmon_event_source PRIVATE
    CONAN_PKG::fmt
    CONAN_PKG::boost
    socket_serialization)
target_link_options(cppmon_event_source PRIVATE "LINKER:--exclude-libs,ALL")
install(TARGETS cppmon_event_source)
install(FILES "${SHM_EXPORT_INTERFACE}/cppmon_event_source_export.h"
    DESTINATION include)
