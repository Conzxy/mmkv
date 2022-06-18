#ifndef _MMKV_ALGO_KEY_VALUE_H_
#define _MMKV_ALGO_KEY_VALUE_H_

namespace mmkv {
namespace algo {

template<typename K, typename V>
struct KeyValue {
  K key;
  V Value;
};

} // namespace algo
} // namespace mmkv

#endif // _MMKV_ALGO_KEY_VALUE_H_
