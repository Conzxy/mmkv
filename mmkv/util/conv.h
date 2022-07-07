#ifndef _MMKV_UTIL_CONV_H_
#define _MMKV_UTIL_CONV_H_

#include <stdint.h>

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

} // util
} // mmkv

#endif // _MMKV_UTIL_CONV_H_
