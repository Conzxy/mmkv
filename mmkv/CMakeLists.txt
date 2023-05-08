mmkv_gen_proto_code(PROTOC_PATH "protoc" FILES "${CMAKE_CURRENT_SOURCE_DIR}/sharder/sharder.proto")
mmkv_gen_proto_code(PROTOC_PATH "protoc" FILES 
  ${CMAKE_CURRENT_SOURCE_DIR}/tracker/controller.proto 
  ${CMAKE_CURRENT_SOURCE_DIR}/tracker/configuration.proto)
mmkv_gen_proto_code(PROTOC_PATH "protoc" FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/forwarder/forwarder.proto)

set(PB_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "mmkv pb output dir = ${PB_OUTPUT_DIR}")

file(GLOB_RECURSE DISTRIBUTION_SRC
  tracker/*.cc
  configd/*.cc
  sharder/*.cc
)

file(GLOB_RECURSE MMKV_SRC
  db/*.cc
  disk/*.cc
  protocol/*.cc
  ${KVARINT_DIR}/kvarint.c 
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

file(GLOB MMKV_SERVER_SRC
  app/server_main.cc
  server/*.cc
  util/*.cc
  lua/*.cc
  protocol/*.cc
  ${KVARINT_DIR}/kvarint.c 
  disk/*.cc
  db/*.cc
  storage/*.cc
  tracker/*.cc
  sharder/*.cc
  forwarder/*.cc
  ${TAKINA_DIR}/takina.cc
  ${CHISATO_DIR}/chisato.cc
)

list(APPEND
  MMKV_SERVER_SRC 
  ${PB_OUTPUT_DIR}/sharder.pb.cc
  ${PB_OUTPUT_DIR}/configuration.pb.cc
  ${PB_OUTPUT_DIR}/controller.pb.cc
  ${PB_OUTPUT_DIR}/forwarder.pb.cc
)

file(GLOB_RECURSE CONFIGD_SRC
  app/configd_main.cc
  server/*.cc
  util/*.cc
  protocol/*.cc
  ${KVARINT_DIR}/kvarint.c 
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
  ${KVARINT_DIR}/kvarint.c 
  client/*.cc
  util/*.cc
  ${TAKINA_DIR}/takina.cc
  ${TERNARY_DIR}/ternary_tree.c
)

file(GLOB_RECURSE RLOG_DUMP_SRC
  app/rlog_dump.cc
  protocol/command.cc
  protocol/mmbp_request.cc
  ${KVARINT_DIR}/kvarint.c 
  disk/file.cc
  disk/log_command.cc
  util/*.cc
  ${TAKINA_DIR}/takina.cc
)

if (MMKV_ON_UNIX)
  mmkv_gen_lib(mmkv ${MMKV_SRC})
  target_include_directories(mmkv
    PUBLIC ${PROJECT_SOURCE_DIR}
    PUBLIC ${XXHASH_DIR}
    PUBLIC ${KANON_DIR}
    PUBLIC ${HKLUA_DIR}
    PUBLIC ${CHISATO_DIR}
    PUBLIC ${TAKINA_DIR}
    PUBLIC ${PB_OUTPUT_DIR}
  )
  SetLibName(HKLUA_LIB hklua)
  target_link_libraries(mmkv
    PUBLIC ${HKLUA_LIB})
endif ()

mmkv_gen_lib(mmkv_xxhash ${XXHASH_DIR}/xxhash.c)

SetLibName(HKLUA_LIB hklua)
set(MMKV_COMMON_LIBS kanon_protobuf kanon_net kanon_base mmkv_xxhash)
set(MMKV_SERVER_LIBS ${MMKV_COMMON_LIBS} dl ${HKLUA_LIB})
set(MMKV_CLIENT_LIBS ${MMKV_COMMON_LIBS} replxx)

mmkv_gen_app(mmkv-cli SOURCES ${MMKV_CLIENT_SRC} LIBS ${MMKV_CLIENT_LIBS})
target_include_directories(mmkv-cli 
  PRIVATE ${TAKINA_DIR}
  PRIVATE ${TERNARY_DIR}
  PRIVATE ${PROJECT_SOURCE_DIR}
  PRIVATE ${XXHASH_DIR}
  PRIVATE ${PB_OUTPUT_DIR}
)

mmkv_gen_app(mmkv-server SOURCES ${MMKV_SERVER_SRC} LIBS ${MMKV_SERVER_LIBS})
target_include_directories(mmkv-server 
  PRIVATE ${TAKINA_DIR}
  PRIVATE ${CHISATO_DIR}
  PRIVATE ${PROJECT_SOURCE_DIR}
  PRIVATE ${XXHASH_DIR}
  PRIVATE ${HKLUA_DIR}
  PRIVATE ${PB_OUTPUT_DIR}
)

mmkv_gen_app(rlogdump SOURCES ${RLOG_DUMP_SRC} LIBS ${MMKV_COMMON_LIBS})
target_include_directories(rlogdump 
  PRIVATE ${TAKINA_DIR}
  PRIVATE ${PROJECT_SOURCE_DIR}
  PRIVATE ${XXHASH_DIR}
  PRIVATE ${PB_OUTPUT_DIR}
)

mmkv_gen_app(configd SOURCES ${CONFIGD_SRC} LIBS ${MMKV_COMMON_LIBS})
target_include_directories(configd 
  PRIVATE ${CHISATO_DIR} 
  PRIVATE ${TAKINA_DIR}
  PRIVATE ${XXHASH_DIR}
  PRIVATE ${PROJECT_SOURCE_DIR}
  PRIVATE ${PB_OUTPUT_DIR}
)
