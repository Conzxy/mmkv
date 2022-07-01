file(GLOB_RECURSE MMKV_SRC
  util/*.cc
  protocol/*.cc
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

set(LIB_LIST mmkv kanon_net kanon_base xxhash)

if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
  GenLib(mmkv ${MMKV_SRC})
else ()
  GenLib(mmkv_debug ${MMKV_SRC})
endif()

macro (GenApplication app_name src_list lib_list)
  set(SRC_LIST ${${src_list}})
  set(LIB_LIST ${${lib_list}})
  message(STATUS "application: ${app_name}")
  message(STATUS "Source list: ${SRC_LIST}")
  message(STATUS "Lib list: ${LIB_LIST}")

  add_executable(${app_name} EXCLUDE_FROM_ALL ${SRC_LIST})
  target_link_libraries(${app_name} ${LIB_LIST})

  set_target_properties(${app_name}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
  )
#endfunction()
endmacro()

GenApplication(mmkv_server MMKV_SERVER_SRC LIB_LIST)
GenApplication(mmkv_client MMKV_CLIENT_SRC LIB_LIST)
