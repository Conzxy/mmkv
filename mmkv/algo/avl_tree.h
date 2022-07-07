#ifndef _MMKV_ALGO_AVL_TREE_H_
#define _MMKV_ALGO_AVL_TREE_H_

#include "internal/avl_tree_impl.h"
#include "libc_allocator_with_realloc.h"

namespace mmkv {
namespace algo {

template<typename T, typename Comparator, typename Alloc=LibcAllocatorWithRealloc<T>>
using AvlTreeSet = AvlTree<T, T, Comparator, Alloc>;

template<typename K, typename V, typename Comparator, typename Alloc=LibcAllocatorWithRealloc<KeyValue<K, V>>>
using AvlTreeMap = AvlTree<K, KeyValue<K, V>, Comparator, Alloc>;

} // algo
} // mmkv

#endif // _MMKV_ALGO_AVL_TREE_H_
