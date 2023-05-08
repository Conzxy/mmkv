cmake_minimum_required(VERSION 3.10)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

include(cmake/utils.cmake)

mmkv_extract_version()

project(mmkv VERSION ${MMKV_VERSION})

message(STATUS "Build Mmkv v${MMKV_VERSION}")

include(cmake/platform.cmake)
include(cmake/build_type.cmake)
include(cmake/compile_flag.cmake)
include(cmake/cache_var.cmake)
include(cmake/install.cmake)
include(cmake/proto_util.cmake)

set(CMAKE_CXX_STANDARD 14)

set(MMKV_SOURCE_DIR ${PROJECT_SOURCE_DIR}/mmkv)
set(THIRD_PARTY_DIR ${PROJECT_SOURCE_DIR}/third-party)

set(KANON_DIR ${THIRD_PARTY_DIR}/kanon)
set(TAKINA_DIR ${THIRD_PARTY_DIR}/takina)
set(CHISATO_DIR ${THIRD_PARTY_DIR}/chisato)
set(TERNARY_DIR ${THIRD_PARTY_DIR}/ternary-tree)
set(HKLUA_DIR ${THIRD_PARTY_DIR}/hklua)
set(XXHASH_DIR ${THIRD_PARTY_DIR}/xxHash)
set(KVARINT_DIR ${THIRD_PARTY_DIR}/kvarint)

add_subdirectory(third-party)
add_subdirectory(mmkv)

if (MMKV_TESTS)
  add_subdirectory(test)
endif ()
