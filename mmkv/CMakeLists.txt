file(GLOB_RECURSE MMKV_SRC
  util/*.cc
  protocol/*.cc
  disk/*.cc
  storage/*.cc
  db/*.cc
)

file(GLOB_RECURSE MMKV_SERVER_SRC
  app/server_main.cc
  server/*.cc
)

file(GLOB_RECURSE MMKV_CLIENT_SRC
  app/client_main.cc
  client/*.cc
)

file(GLOB_RECURSE RLOG_DUMP_SRC
  app/rlog_dump.cc)

if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
  GenLib(mmkv ${MMKV_SRC})
  set(MMKV_LIB mmkv)
else ()
  GenLib(mmkv_debug ${MMKV_SRC})
  set(MMKV_LIB mmkv_debug)
endif()

set(LIB_LIST ${MMKV_LIB} kanon_net kanon_base xxhash)

GenLib(mmkv_server_src ${MMKV_SERVER_SRC})
GenLib(mmkv_client_src ${MMKV_CLIENT_SRC})

macro (GenApplication app_name src_list lib_list)
  set(SRC_LIST ${${src_list}})
  set(LIB_LIST ${${lib_list}})
  message(STATUS "application: ${app_name}")
  message(STATUS "Source list: ${SRC_LIST}")

  add_executable(${app_name} EXCLUDE_FROM_ALL ${SRC_LIST})
  list (LENGTH LIB_LIST lib_list_len)
  if (NOT ${lib_list_len} EQUAL 0)
    message(STATUS "Lib list: ${LIB_LIST}")
    target_link_libraries(${app_name} ${LIB_LIST})
  endif ()

  set_target_properties(${app_name}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
  )
#endfunction()
endmacro()

set (EMPTY_LIST )
GenApplication(mmkv_server MMKV_SERVER_SRC LIB_LIST)
GenApplication(mmkv_cli MMKV_CLIENT_SRC LIB_LIST)
GenApplication(rlogdump RLOG_DUMP_SRC LIB_LIST)
