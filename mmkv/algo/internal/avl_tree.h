// SPDX-LICENSE-IDENTIFIER: Apache-2.0
/***********************************************************************/
/* Date: 2022/6/29                                                     */
/***********************************************************************/

#ifndef _MMKV_ALGO_INTERNAL_AVL_TREE_H_
#define _MMKV_ALGO_INTERNAL_AVL_TREE_H_

#include "avl_tree_base_impl.h"

namespace mmkv {
namespace algo {

/**
 * \brief AvlTree(Strict self-balanced binary search tree)
 *
 * Compared to the rbtree,
 * the height of avltree is less than rbtree
 * therefore, the performance of find() is better
 * however, the performance of insert and delete operation 
 * is not worse than rbtree(see test/algo/avltree_bench.cc)
 *
 * \note
 *   Public class
 *   moveable
 */
template<typename K, typename V, typename Comparator, typename GK = GetKey<V>, typename Alloc = LibcAllocatorWithRealloc<V>>
class AvlTree : public AvlTreeBase<K, V, Comparator, GK, Alloc, AvlTree<K, V, Comparator, GK, Alloc>> {
  using Base = AvlTreeBase<K, V, Comparator, GK, Alloc, AvlTree<K, V, Comparator, GK, Alloc>>;
  using typename Base::size_type;
  // OR friend Base
  friend class AvlTreeBase<K, V, Comparator, GK, Alloc, AvlTree<K, V, Comparator, GK, Alloc>>;
 public:
  size_type size() const noexcept { return count_; }

 private:
  void _IncreaseCount() noexcept { ++count_; }
  void _DecreaseCount() noexcept { --count_; }
  void _SetCount(size_type n) noexcept { count_ = n; }

  size_type count_;
};

} // algo
} // mmkv

#endif // _MMKV_ALGO_INTERNAL_AVL_TREE_H_
