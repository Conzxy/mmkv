file(GLOB_RECURSE MMKV_SRC
  util/*.cc
  protocol/*.cc
)

GenLib(mmkv ${MMKV_SRC})
