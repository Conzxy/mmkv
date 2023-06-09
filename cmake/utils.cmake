function(mmkv_extract_version)
    file(READ "${CMAKE_CURRENT_LIST_DIR}/mmkv/version.h" file_contents)
    string(REGEX MATCH "MMKV_VER_MAJOR ([0-9]+)" _ "${file_contents}")
    if(NOT CMAKE_MATCH_COUNT EQUAL 1)
        message(FATAL_ERROR "Could not extract major version number from mmkv/version.h")
    endif()
    set(ver_major ${CMAKE_MATCH_1})

    string(REGEX MATCH "MMKV_VER_MINOR ([0-9]+)" _ "${file_contents}")
    if(NOT CMAKE_MATCH_COUNT EQUAL 1)
        message(FATAL_ERROR "Could not extract minor version number from mmkv/version.h")
    endif()

    set(ver_minor ${CMAKE_MATCH_1})
    string(REGEX MATCH "MMKV_VER_PATCH ([0-9]+)" _ "${file_contents}")
    if(NOT CMAKE_MATCH_COUNT EQUAL 1)
        message(FATAL_ERROR "Could not extract patch version number from mmkv/version.h")
    endif()
    set(ver_patch ${CMAKE_MATCH_1})

    set(MMKV_VERSION_MAJOR ${ver_major} PARENT_SCOPE)
    set(MMKV_VERSION_MINOR ${ver_minor} PARENT_SCOPE)
    set(MMKV_VERSION_PATCH ${ver_patch} PARENT_SCOPE)
    set(MMKV_VERSION "${ver_major}.${ver_minor}.${ver_patch}" PARENT_SCOPE)
endfunction()

function (mmkv_gen_lib lib)
  #if (NOT ${BUILD_SHARED_LIBS})
  if (MMKV_BUILD_STATIC_LIBS)
    message(STATUS "Build static library: ${lib}")
    add_library(${lib} STATIC ${ARGN})
    
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
      set(archive_output_path "${CMAKE_BINARY_DIR}/mmkv/lib/debug")
    else ()
      set(archive_output_path "${CMAKE_BINARY_DIR}/mmkv/lib/release")
    endif ()

    message(STATUS "${lib} output dir: ${archive_output_path}")
    set_target_properties(${lib}
     PROPERTIES
     ARCHIVE_OUTPUT_DIRECTORY
     ${archive_output_path})
  else ()
    message(STATUS "Build shared library: ${lib}")
    add_library(${lib} SHARED ${ARGN})
    # Follow the windows convention
    # *.dll to bin directory
    if (WIN32)
      set(so_output_dir bin)
    else ()
      set(so_output_dir lib)
    endif ()

    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
      set(shared_output_path "${CMAKE_BINARY_DIR}/mmkv/${so_output_dir}/debug")
    else ()
      set(shared_output_path "${CMAKE_BINARY_DIR}/mmkv/${so_output_dir}/release")
    endif ()
      
    message(STATUS "${lib} output dir: ${shared_output_path}")
    
    if (WIN32)
      # In windows, CMake think it is RUNTIME output instead of LIBRARY
      set_target_properties(${lib}
       PROPERTIES
       RUNTIME_OUTPUT_DIRECTORY 
       ${shared_output_path})

      # In windows, need consider *.Lib file
      # Don't put *.dll and *.Lib in same directory
      set(dll_lib_output_dir ${shared_output_path}/lib)
      message(STATUS "DLL lib output dir: ${dll_lib_output_dir}")
      set_target_properties(${lib}
       PROPERTIES
       ARCHIVE_OUTPUT_DIRECTORY 
       ${dll_lib_output_dir})
    else ()
      set_target_properties(${lib}
       PROPERTIES
       LIBRARY_OUTPUT_DIRECTORY 
       ${shared_output_path})
    endif ()
  endif ()

  message(STATUS "Lib ${lib} Sources: ${ARGN}")
endfunction ()

function (conzxy_copy_dll_to_target_dir)
  cmake_parse_arguments(PARSE_ARGV 0 CONZXY "" "DLL_PATH;TARGET" "")
  add_custom_command(TARGET ${CONZXY_TARGET} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CONZXY_DLL_PATH}"
    "$<TARGET_FILE_DIR:${CONZXY_TARGET}>")  
endfunction ()

function (mmkv_gen_app app_name src_list lib_list)
  cmake_parse_arguments(PARSE_ARGV 1 CONZXY "" "" "SOURCES;LIBS")
  message(STATUS "application: ${app_name}")
  message(STATUS "Source list: ${CONZXY_SOURCES}")
  
  list (LENGTH CONZXY_LIBS libs_len)
  list (LENGTH CONZXY_SOURCES sources_len)
  
  if (libs_len EQUAL 0)
    message(FATAL_ERROR "app: {${app_name}} no source files!\n
      Stop build")
  endif ()

  add_executable(${app_name} EXCLUDE_FROM_ALL ${CONZXY_SOURCES})
  if (libs_len GREATER 0)
    message(STATUS "Libs list: ${CONZXY_LIBS}")
    target_link_libraries(${app_name} PRIVATE ${CONZXY_LIBS})
  endif ()
  
  if (CMAKE_BUILD_TYPE STREQUAL "Debug") 
    set (app_output_path "${CMAKE_SOURCE_DIR}/bin/debug")
  else ()
    set (app_output_path "${CMAKE_SOURCE_DIR}/bin")
  endif ()

  set_target_properties(${app_name}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY 
    "${app_output_path}"
  )
endfunction ()

function (SetLibName var lib_name)
  if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(${var} ${lib_name}_debug PARENT_SCOPE)
  else ()
    set(${var} ${lib_name} PARENT_SCOPE)
  endif()
endfunction ()
