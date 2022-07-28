#ifndef _MMKV_ALGO_INTERNAL_AVL_LIST_H_
#define _MMKV_ALGO_INTERNAL_AVL_LIST_H_

#include "avl_tree_base_impl.h"
#include "func_util.h"

namespace mmkv {
namespace algo {

/**
 * \brief AvlTree but without count record
 * 
 * 用于一些不需要计数的场合，特别是作为其他数据结构的存储元素
 * \note
 *   Internal class
 *   Used for TreeHashTable
 */
template<typename K, typename V, typename Compare, typename GK = GetKey<V>, typename Alloc=LibcAllocatorWithRealloc<V>>
class AvlList : public AvlTreeBase<K, V, Compare, GK, Alloc, AvlList<K, V, Compare, GK, Alloc>> {
 public:
  using Base = AvlTreeBase<K, V, Compare, GK, Alloc, AvlList<K, V, Compare, GK, Alloc>>;
  AvlList() noexcept
    : Base()
  {}

};

template<typename V, typename Compare, typename GK = GetKey<V>, typename Alloc=LibcAllocatorWithRealloc<V>>
using AvlListSet = AvlList<V, V, Compare, GK, Alloc>;

template<typename K, typename V, typename Compare, typename GK = GetKey<KeyValue<K, V>>, typename Alloc=LibcAllocatorWithRealloc<KeyValue<K, V>>>
using AvlListMap = AvlList<K, KeyValue<K, V>, Compare, GK, Alloc>;

} // algo
} // mmkv

#endif // _MMKV_ALGO_INTERNAL_AVL_LIST_H_