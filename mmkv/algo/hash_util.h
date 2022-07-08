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

// Get Key
template<typename K>
struct GetKey {
  constexpr K const& operator()(K const& key) const noexcept {
    return key;
  }
};

template<typename K, typename V>
struct GetKey <KeyValue<K, V>> {
  constexpr K const& operator()(KeyValue<K, V> const& kv) const noexcept {
    return kv.key;
  }
};

// Equal Key
template<typename K>
struct EqualKey {
  constexpr bool operator()(K const& k1, K const& k2) const noexcept {
    return k1 == k2;
  }
};

} // algo
} // mmkv

#endif // _MMKV_ALGO_HASH_UTIL_H
