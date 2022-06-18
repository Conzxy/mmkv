#ifndef _MMKV_ALGO_HASH_UTIL_H
#define _MMKV_ALGO_HASH_UTIL_H

#include <stdint.h>
#include <string>

#include "xxhash.h"
#include "key_value.h"

namespace mmkv {
namespace algo {

template<typename T>
struct Hash;

template<>
struct Hash<std::string> {
  uint64_t operator()(std::string const& x) noexcept {
    return XXH64(x.c_str(), x.size(), 0);
  }
};

#define BASIC_TYPE_SPECILIZATION(type) \
template<> \
struct Hash<type> { \
  uint64_t operator()(type x) noexcept { \
    return XXH64(&x, sizeof(x), 0); \
  } \
};

BASIC_TYPE_SPECILIZATION(int)

// Get Key
template<typename K>
struct GetKey {
  K const& operator()(K const& key) const noexcept {
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
