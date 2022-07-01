#ifndef _MMKV_ALGO_SORTED_SET_H_
#define _MMKV_ALGO_SOETED_SET_H_

#include "avl_tree.h"
#include "mmkv/algo/libc_allocator_with_realloc.h"

namespace mmkv {
namespace algo {

template<typename V, typename Compare, typename Alloc=LibcAllocatorWithRealloc<V>>
using SortedSet = AvlTree<V, V, Compare, Alloc>;

} // algo
} // mmkv

#endif // _MMKV_ALGO_SORTED_SET_H_
