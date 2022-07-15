#ifndef _MMKV_ALGO_TREE_HASH_TABLE_H_
#define _MMKV_ALGO_TREE_HASH_TABLE_H_

#include "internal/tree_hashtable_impl.h"
#include "hash_util.h"
#include "internal/avl_list.h"

namespace mmkv {
namespace algo {

// Convenient type alias
// If you want to self-defined interface, you can take it as adapter

template<typename T, typename Comparator, typename HF=Hash<T>, typename GK=GetKey<T>, typename Alloc=LibcAllocatorWithRealloc<T>>
using AvlTreeHashSet = TreeHashTable<T, T, HF, GK, AvlListSet<T, Comparator, GK, Alloc>, Alloc>;

/**
 * \note
 *  To reuse the node and rename key, I don't set key to "const K" specifier.
 */
template<typename K, typename V, typename Comparator, typename HF=Hash<K>, typename GK=GetKey<KeyValue<K, V>>, typename Alloc=LibcAllocatorWithRealloc<KeyValue<K, V>>>
using AvlTreeHashMap = TreeHashTable<K, KeyValue<K, V>, HF, GK, AvlListMap<K, V, Comparator, GK, Alloc>, Alloc>;

} // algo
} // mmkv

#endif // _MMKV_ALGO_TREE_HASH_TABLE_H_