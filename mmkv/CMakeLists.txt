file(GLOB_RECURSE DISTRIBUTION_SRC
  tracker/*.cc
  configd/*.cc
  sharder/*.cc
)

file(GLOB_RECURSE MMKV_SRC
  db/*.cc
  disk/*.cc
  protocol/*.cc
  server/*.cc
  storage/*.cc
  util/*.cc
  replacement/*.cc
  lua/*.cc
  client/tracker_client.cc
  ${DISTRIBUTION_SRC}
  cluster/*.cc
  ${CHISATO_DIR}/chisato.cc
  ${TAKINA_DIR}/takina.cc
)

file(GLOB_RECURSE MMKV_SERVER_SRC
  app/server_main.cc
  server/*.cc
  util/*.cc
  lua/*.cc
  protocol/*.cc
  disk/*.cc
  db/*.cc
  storage/*.cc
  tracker/*.cc
  sharder/*.cc
  ${TAKINA_DIR}/takina.cc
  ${CHISATO_DIR}/chisato.cc
)

file(GLOB_RECURSE CONFIGD_SRC
  app/configd_main.cc
  server/*.cc
  util/*.cc
  protocol/*.cc
  disk/*.cc
  db/*.cc
  storage/*.cc
  ${DISTRIBUTION_SRC}
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

file(GLOB_RECURSE RLOG_DUMP_SRC
  app/rlog_dump.cc
  protocol/command.cc
  protocol/mmbp_request.cc
  disk/file.cc
  disk/log_command.cc
  util/*.cc
  ${TAKINA_DIR}/takina.cc
)

if (MMKV_ON_UNIX)
  mmkv_gen_lib(mmkv ${MMKV_SRC})
  target_include_directories(mmkv
    PUBLIC ${XXHASH_DIR}
  )
endif ()

mmkv_gen_lib(mmkv_xxhash ${XXHASH_DIR}/xxhash.c)

SetLibName(HKLUA_LIB hklua)
set(MMKV_COMMON_LIBS kanon_net kanon_base mmkv_xxhash)
set(MMKV_SERVER_LIBS ${MMKV_COMMON_LIBS} lua dl ${HKLUA_LIB})
set(MMKV_CLIENT_LIBS ${MMKV_COMMON_LIBS} replxx)

mmkv_gen_app(mmkv-cli SOURCES ${MMKV_CLIENT_SRC} LIBS ${MMKV_CLIENT_LIBS})
target_include_directories(mmkv-cli 
  PRIVATE ${TAKINA_DIR}
  PRIVATE ${TERNARY_DIR}
  PRIVATE ${PROJECT_SOURCE_DIR}
  PRIVATE ${XXHASH_DIR}
)

mmkv_gen_app(mmkv-server SOURCES ${MMKV_SERVER_SRC} LIBS ${MMKV_SERVER_LIBS})
target_include_directories(mmkv-server 
  PRIVATE ${TAKINA_DIR}
  PRIVATE ${CHISATO_DIR}
  PRIVATE ${PROJECT_SOURCE_DIR}
  PRIVATE ${XXHASH_DIR}
)

mmkv_gen_app(rlogdump SOURCES ${RLOG_DUMP_SRC} LIBS ${MMKV_COMMON_LIBS})
target_include_directories(rlogdump 
  PRIVATE ${TAKINA_DIR}
  PRIVATE ${PROJECT_SOURCE_DIR}
  PRIVATE ${XXHASH_DIR}
)

mmkv_gen_app(configd SOURCES ${CONFIGD_SRC} LIBS ${MMKV_COMMON_LIBS})
target_include_directories(configd 
  PRIVATE ${CHISATO_DIR} 
  PRIVATE ${TAKINA_DIR}
  PRIVATE ${XXHASH_DIR}
)
