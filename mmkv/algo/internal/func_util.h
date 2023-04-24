// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_ALGO_FUNC_UTIL_H_
#define _MMKV_ALGO_FUNC_UTIL_H_

#include "../key_value.h"

namespace mmkv {
namespace algo {

template<typename K>
K& get_key(K& k) noexcept {
  return k;
}

template<typename K, typename V>
K& get_key(KeyValue<K, V>& kv) noexcept {
  return kv.key;
}
 
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

#endif // _MMKV_ALGO_FUNC_UTIL_H_
