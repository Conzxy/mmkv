file(GLOB_RECURSE MMKV_SRC
  db/*.cc
  disk/*.cc
  protocol/*.cc
  server/config.cc
  storage/*.cc
  util/*.cc
  replacement/*.cc
)

file(GLOB_RECURSE MMKV_SERVER_SRC
  app/server_main.cc
  server/*.cc
  util/*.cc
  protocol/*.cc
  disk/*.cc
  db/*.cc
  storage/*.cc
  ${TAKINA_DIR}/takina.cc
  ${CHISATO_DIR}/chisato.cc
)

file(GLOB_RECURSE MMKV_CLIENT_SRC
  app/client_main.cc
  protocol/*.cc
  client/*.cc
  util/*.cc
  ${TAKINA_DIR}/takina.cc
  ${TERNARY_DIR}/ternary_tree.c
)

list (APPEND MMKV_CLIENT_SRC ${LINENOISE_DIR}/linenoise.c)

file(GLOB_RECURSE RLOG_DUMP_SRC
  app/rlog_dump.cc
  protocol/command.cc
  protocol/mmbp_request.cc
  disk/file.cc
  disk/log_command.cc
  util/*.cc
  ${TAKINA_DIR}/takina.cc
)

if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
  GenLib(mmkv ${MMKV_SRC})
  set(MMKV_LIB mmkv)
else ()
  GenLib(mmkv_debug ${MMKV_SRC})
  set(MMKV_LIB mmkv_debug)
endif ()

set(MMKV_LIB_LIST kanon_net kanon_base xxhash)

# GenLib(mmkv_server_src ${MMKV_SERVER_SRC})
# GenLib(mmkv_client_src ${MMKV_CLIENT_SRC})
GenApplication(mmkv-server MMKV_SERVER_SRC MMKV_LIB_LIST)
target_include_directories(mmkv-server PRIVATE ${TAKINA_DIR} PRIVATE ${CHISATO_DIR})

GenApplication(mmkv-cli MMKV_CLIENT_SRC MMKV_LIB_LIST)
target_include_directories(mmkv-cli PRIVATE ${TAKINA_DIR} PRIVATE ${TERNARY_DIR})

GenApplication(rlogdump RLOG_DUMP_SRC MMKV_LIB_LIST)
target_include_directories(rlogdump PRIVATE ${TAKINA_DIR})
