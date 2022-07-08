#ifndef _MMKV_ALGO_HASH_SET_H_
#define _MMKV_ALGO_HASH_SET_H_

#include "hash_util.h"
#include "hash_table.h"

namespace mmkv {
namespace algo {

template<typename K>
using HashSet = HashTable<K, K, Hash<K>, GetKey<K>, EqualKey<K>, LibcAllocatorWithRealloc<K>>;

} // algo
} // mmkv

#endif // _MMKV_ALGO_HASH_SET_H_
