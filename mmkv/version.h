#ifndef MMKV_VERSION_H__
#define MMKV_VERSION_H__

#define MMKV_VER_MAJOR 1
#define MMKV_VER_MINOR 1
#define MMKV_VER_PATCH 0

#define MMKV_VERSION                                                           \
  (MMKV_VER_MAJOR * 10000 + MMKV_VER_MINOR * 100 + MMKV_VER_PATCH)
#ifndef MMKV_2STR
#  define MMKV_2STR(x) #x
#endif

#define MMKV_VERSION_STR_IMPL(major__, minor__, patch__)                       \
  MMKV_2STR(major__) "." MMKV_2STR(minor__) "." MMKV_2STR(patch__)

#define MMKV_VERSION_STR                                                       \
  MMKV_VERSION_STR_IMPL(MMKV_VER_MAJOR, MMKV_VER_MINOR, MMKV_VER_PATCH)

// Just test, don't use this!
#define MMKV_VERSION_STR2                                                      \
  MMKV_2STR(MMKV_VER_MAJOR)                                                    \
  "." MMKV_2STR(MMKV_VER_MINOR) "." MMKV_2STR(MMKV_VER_PATCH)

#endif
