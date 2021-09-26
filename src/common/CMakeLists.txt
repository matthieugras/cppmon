set(COMMON_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})

add_library(common STATIC event_data.cpp)
target_include_directories(common PUBLIC
    ${COMMON_INCLUDES})
target_link_libraries(common
    CONAN_PKG::boost
    CONAN_PKG::fmt
    CONAN_PKG::nlohmann_json)