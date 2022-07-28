#ifndef _MMKV_UTIL_CONV_H_
#define _MMKV_UTIL_CONV_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <kanon/net/endian_api.h>

namespace mmkv {
namespace util {

inline double int2double(uint64_t i) noexcept {
  return *(double*)&i;
}

inline uint64_t double2u64(double d) noexcept {
  return *(uint64_t*)&d;
}

inline int64_t double2i64(double d) noexcept {
  return *(uint64_t*)&d;
}

inline uint32_t raw2u32(char const *buf) noexcept {
  return *(uint32_t*)buf;
}

inline uint64_t raw2u64(char const *buf) noexcept {
  return *(uint64_t*)buf;
}

inline bool str2u64(char const *buf, uint64_t &i) noexcept {
  char *end = NULL;
  auto res = strtoull(buf, &end, 10);
  if (res == 0 && end == buf)
    return false;
  
  i = res;
  return true;
}

} // util
} // mmkv

#endif // _MMKV_UTIL_CONV_H_
