#ifndef _MMKV_ALGO_HASH_UTIL_H
#define _MMKV_ALGO_HASH_UTIL_H

#include <stdint.h>
#include "string.h"

#include "xxhash.h"
#include "key_value.h"

namespace mmkv {
namespace algo {

template<typename T>
struct Hash {
  static_assert(sizeof(T) < 0, "Non-supported type");
};

template<typename Alloc>
struct Hash<std::basic_string<char, std::char_traits<char>, Alloc>> {
  uint64_t operator()(String const& x) const noexcept {
    return XXH64(x.c_str(), x.size(), 0);
  }
};

#define BASIC_TYPE_SPECILIZATION(type) \
template<> \
struct Hash<type> { \
  uint64_t operator()(type x) const noexcept { \
    return XXH64(&x, sizeof(x), 0); \
  } \
};

BASIC_TYPE_SPECILIZATION(char)
BASIC_TYPE_SPECILIZATION(unsigned char)
BASIC_TYPE_SPECILIZATION(short)
BASIC_TYPE_SPECILIZATION(unsigned short)
BASIC_TYPE_SPECILIZATION(int)
BASIC_TYPE_SPECILIZATION(unsigned int)
BASIC_TYPE_SPECILIZATION(long)
BASIC_TYPE_SPECILIZATION(unsigned long)
BASIC_TYPE_SPECILIZATION(long long)
BASIC_TYPE_SPECILIZATION(unsigned long long)
BASIC_TYPE_SPECILIZATION(float)
BASIC_TYPE_SPECILIZATION(double)
BASIC_TYPE_SPECILIZATION(long double)

} // algo
} // mmkv

#include "internal/func_util.h"

#endif // _MMKV_ALGO_HASH_UTIL_H
