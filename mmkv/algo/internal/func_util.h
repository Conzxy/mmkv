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

}
}

#endif // _MMKV_ALGO_FUNC_UTIL_H_
