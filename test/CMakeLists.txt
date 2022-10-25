#enable_testing()
file(GLOB MMKV_TEST_SOURCES "${PROJECT_SOURCE_DIR}/test/*/*test*.cc")
file(GLOB MMKV_BENCH_SOURCES "${PROJECT_SOURCE_DIR}/test/*/*bench*.cc")

#list(APPEND MMKV_TEST_BENCH_SOURCES ${MMKV_TEST_SOURCES} ${MMKV_BENCH_SOURCES})

############################################
# User can determine whether to build all tests when build target all
# e.g. cmake --build */mmkv/build [--target all -j 2]
############################################
set(BUILD_ALL_TESTS OFF CACHE BOOL "Don't exclude tests from target all")

message(STATUS "mmkv test source files: ${MMKV_TEST_SOURCES}")
message(STATUS "mmkv bench source files: ${MMKV_BENCH_SOURCES}")

if (NOT ${BUILD_ALL_TESTS})
  message(STATUS "[The mmkv test and bench files are excluded from the build of target all]")
endif (NOT ${BUILD_ALL_TESTS})

############################################
# generate test and bench program
############################################
macro (GenTest SRC)
  foreach (mmkv_test_source ${${SRC}})
    get_filename_component(mmkv_test_filename ${mmkv_test_source} NAME_WE)

    if (${BUILD_ALL_TESTS})
      add_executable(${mmkv_test_filename} ${mmkv_test_source})
    else ()
      message(STATUS "${mmkv_test_filename} is excluded from target all")
      add_executable(${mmkv_test_filename} EXCLUDE_FROM_ALL ${mmkv_test_source})
    endif (${BUILD_ALL_TESTS})
  
    #if (mmkv_test_filename MATCHES ".*hash.*")
    #  #target_link_libraries(${mmkv_test_filename} ${ARGN} "/usr/local/lib/libxxhash.so.0.8.1")
    #  target_link_libraries(${mmkv_test_filename} ${ARGN} xxhash)
    #else ()
    #  target_link_libraries(${mmkv_test_filename} ${ARGN})
    #endif(mmkv_test_filename MATCHES ".*hash.*")

    target_link_libraries(${mmkv_test_filename} ${ARGN} xxhash)
    set_target_properties(${mmkv_test_filename}
      PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test"
      COMMAND ${mmkv_test_filename}
    )

    add_test(NAME ${mmkv_test_filename} COMMAND ${mmkv_test_filename})
  endforeach (mmkv_test_source ${MMKV_TEST_SOURCES})
endmacro()

if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
  set(MMKV_LIB mmkv)
else ()
  set(MMKV_LIB mmkv_debug)
endif()

SetLibName(HKLUA_LIB hklua)

GenTest(MMKV_TEST_SOURCES gtest gtest_main ${MMKV_LIB} kanon_net kanon_base ${HKLUA_LIB} lua)
GenTest(MMKV_BENCH_SOURCES benchmark benchmark_main ${MMKV_LIB} kanon_net kanon_base ${HKLUA_LIB} lua)
