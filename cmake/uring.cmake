include(ExternalProject)
include(ProcessorCount)
ProcessorCount(NCPUS)
find_program(MAKE_EXE make gmake)
string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_UPPER)
list(APPEND URING_C_FLAGS ${CMAKE_C_FLAGS}
     ${CMAKE_C_FLAGS_${CMAKE_BUILD_TYPE_UPPER}})
list(JOIN URING_C_FLAGS " " URING_C_FLAGS)
list(APPEND URING_CXX_FLAGS ${CMAKE_CXX_FLAGS}
     ${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE_UPPER}})
list(JOIN URING_CXX_FLAGS " " URING_CXX_FLAGS)
ExternalProject_Add(
  liburing
  GIT_REPOSITORY https://github.com/axboe/liburing.git
  GIT_TAG "liburing-2.1"
  GIT_REMOTE_UPDATE_STRATEGY CHECKOUT
  CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
                    --cc=${CMAKE_C_COMPILER} --cxx=${CMAKE_CXX_COMPILER}
  BUILD_COMMAND ${CMAKE_COMMAND} -E env CPP_FLAGS=${URING_CXX_FLAGS}
                CFLAGS=${URING_C_FLAGS} ${MAKE_EXE} -C src -n -j${NCPUS}
  INSTALL_COMMAND ${MAKE_EXE} -j${NCPUS} install
  BUILD_BYPRODUCTS <INSTALL_DIR>/lib/liburing.a <INSTALL_DIR>/include/liburing.h
  BUILD_IN_SOURCE TRUE
  LOG_DOWNLOAD TRUE
  LOG_CONFIGURE TRUE
  LOG_BUILD TRUE
  LOG_INSTALL TRUE
  LOG_OUTPUT_ON_FAILURE TRUE
  LOG_MERGED_STDOUTERR TRUE)
ExternalProject_Get_Property(liburing install_dir)
set(URING_LIB_DIR "${install_dir}/lib")
set(URING_INC_DIR "${install_dir}/include")
file(MAKE_DIRECTORY ${URING_INC_DIR})
add_library(uring STATIC IMPORTED GLOBAL)
add_dependencies(uring liburing)
set_target_properties(
  uring PROPERTIES IMPORTED_LOCATION ${URING_LIB_DIR}/liburing.a
                   INTERFACE_INCLUDE_DIRECTORIES ${URING_INC_DIR})
