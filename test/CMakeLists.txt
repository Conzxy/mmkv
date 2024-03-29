#enable_testing()
file(GLOB MMKV_TEST_SOURCES "${PROJECT_SOURCE_DIR}/test/*/*test*.cc")
file(GLOB MMKV_BENCH_SOURCES "${PROJECT_SOURCE_DIR}/test/*/*bench*.cc")

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
  
    target_link_libraries(${mmkv_test_filename} ${ARGN})
    set_target_properties(${mmkv_test_filename}
      PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test"
      COMMAND ${mmkv_test_filename}
    )

    add_test(NAME ${mmkv_test_filename} COMMAND ${mmkv_test_filename})
  endforeach (mmkv_test_source ${MMKV_TEST_SOURCES})
endmacro()

find_package(GTest CONFIG REQUIRED)
find_package(benchmark CONFIG REQUIRED)

GenTest(MMKV_TEST_SOURCES GTest::gtest GTest::gtest_main mmkv kanon_net kanon_base)
GenTest(MMKV_BENCH_SOURCES benchmark benchmark_main mmkv kanon_net kanon_base)
