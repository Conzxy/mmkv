// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_ALGO_AVL_TREE_H_
#define _MMKV_ALGO_AVL_TREE_H_

#include "internal/avl_tree.h"
#include "libc_allocator_with_realloc.h"

namespace mmkv {
namespace algo {

// Convenient type alias
template<typename T, typename Comparator, typename GK=GetKey<T>, typename Alloc=LibcAllocatorWithRealloc<T>>
using AvlTreeSet = AvlTree<T, T, Comparator, GK, Alloc>;

template<typename K, typename V, typename Comparator, typename GK=GetKey<KeyValue<K, V>>, typename Alloc=LibcAllocatorWithRealloc<KeyValue<K, V>>>
using AvlTreeMap = AvlTree<K, KeyValue<K, V>, Comparator, GK, Alloc>;

} // algo
} // mmkv

#endif // _MMKV_ALGO_AVL_TREE_H_
