#ifndef _MMKV_ALGO_INTERNAL_AVL_TREE_ITERATOR_H_
#define _MMKV_ALGO_INTERNAL_AVL_TREE_ITERATOR_H_

#include <iterator>

#include "avl_node.h"
#include "avl_util.h"

namespace mmkv {
namespace algo {

template<typename T>
class AvlTreeConstIterator {
 protected:
  using Node = avl::AvlNode<T>;
  using BaseNode = avl::AvlBaseNode;
  using Self = AvlTreeConstIterator;
  
  template<typename K, typename V, typename Compare, typename GK, typename Alloc, typename D>
  friend class AvlTreeBase;
 public:
  using value_type = T;
  using reference = T const&;
  using pointer = T const*;
  using const_reference = T const&;
  using const_pointer = T const*;
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = std::ptrdiff_t;

  AvlTreeConstIterator()
    : AvlTreeConstIterator(nullptr)
  {
  }
  
  AvlTreeConstIterator(BaseNode const* node) noexcept
    : node_((Node*)node)
  {
  }

  ~AvlTreeConstIterator() = default;
  
  Self& operator++() noexcept {
    Increment();
    return *this;
  }

  Self& operator--() noexcept {
    Decrement();
    return *this;
  }

  Self operator++(int) noexcept {
    Self ret(node_);
    Increment();
    return ret;
  }

  Self operator--(int) noexcept {
    Self ret(node_);
    Decrement();
    return ret;
  }
  
  T const& operator*() const noexcept {
    return node_->value;
  }
  
  T const* operator->() const noexcept {
    return &node_->value;
  }

  friend bool operator==(Self x, Self y) noexcept {
    return x.node_ == y.node_;
  }

  friend bool operator!=(Self x, Self y) noexcept {
    return !(x == y);
  }

 protected:
  void Increment() noexcept {
    node_ = (Node*)_GetNextNode(node_);
  }

  void Decrement() noexcept {
    node_ = (Node*)_GetPrevNode(node_);
  }

  Node* node_;
};

template<typename T>
class AvlTreeIterator final : public AvlTreeConstIterator<T> {
  using Base = AvlTreeConstIterator<T>;
  using typename Base::Node;
  using Self = AvlTreeIterator;
  using Base::node_;
  
  template<typename K, typename V, typename Compare, typename GK, typename Alloc, typename D>
  friend class AvlTreeBase;
 public:
  using reference = T&;
  using pointer = T*;

  AvlTreeIterator()
    : AvlTreeIterator(nullptr)
  {
  }

  AvlTreeIterator(typename Base::BaseNode* node) noexcept
    : Base(node)
  {
  }

  ~AvlTreeIterator() = default;
  
  Self& operator++() noexcept {
    Base::Increment();
    return *this;
  }

  Self& operator--() noexcept {
    Base::Decrement();
    return *this;
  }

  Self operator++(int) noexcept {
    Self ret(node_);
    Base::Increment();
    return ret;
  }

  Self operator--(int) noexcept {
    Self ret(node_);
    Base::Decrement();
    return ret;
  }
  
  T& operator*() noexcept {
    return node_->value;
  }

  T const& operator*() const noexcept {
    return node_->value;
  }

  T* operator->() noexcept {
    return &node_->value;
  }

  T const* operator->() const noexcept {
    return &node_->value;
  }
};

} // algo
} // mmkv

#endif // _MMKV_ALGO_INTERNAL_AVL_TREE_ITERATOR_H_
