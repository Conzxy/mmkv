#ifndef _MMKV_ALGO_INTERNAL_BLIST_ITERATOR_H_
#define _MMKV_ALGO_INTERNAL_BLIST_ITERATOR_H_

#include <iterator>

#include "bnode.h"

namespace mmkv {
namespace algo {
namespace blist {

template<typename T>
class BlistConstIterator {
  using Node = BNode<T>;
  using Self = BlistConstIterator;

 public:
  using value_type = T;
  using iterator_category = std::bidirectional_iterator_tag;
  using reference = T const&;
  using const_reference = T const&;
  using difference_type = std::ptrdiff_t;
  using pointer = T const*;
  using const_pointer = T const*;

  BlistConstIterator(Node const* node) noexcept
    : node_((Node*)node)
  { 
  }
  
  reference operator*() noexcept {
    return node_->value;
  }

  const_reference operator*() const noexcept {
    return node_->value;
  }

  pointer operator->() noexcept {
    return &node_->value;
  }

  const_pointer operator->() const noexcept {
    return &node_->value;
  }

  Self& operator++() noexcept {
    node_ = node_->next;
    return *this;
  }

  Self& operator--() noexcept {
    node_ = node_->prev;
    return *this;
  }
  
  Self operator++(int) noexcept {
    auto ret = node_;
    node_ = node_->next;
    return ret;
  }

  Self operator--(int) noexcept {
    auto ret = node_;
    node_ = node_->prev;
    return ret;
  }

  friend bool operator==(BlistConstIterator x, BlistConstIterator y) noexcept {
    return x.node_ == y.node_;
  }

  friend bool operator!=(BlistConstIterator x, BlistConstIterator y) noexcept {
    return !(x == y);
  }

 private:
  Node* node_;
};

template<typename T>
class BlistIterator {
  using Node = BNode<T>;
  using Self = BlistIterator;

 public:
  using value_type = T;
  using iterator_category = std::bidirectional_iterator_tag;
  using reference = T&;
  using const_reference = T const&;
  using difference_type = std::ptrdiff_t;
  using pointer = T*;
  using const_pointer = T const*;

  BlistIterator(Node* node) noexcept
    : node_(node)
  { 
  }
  
  operator BlistConstIterator<T>() noexcept {
    return node_;
  }

  reference operator*() noexcept {
    return node_->value;
  }

  const_reference operator*() const noexcept {
    return node_->value;
  }

  pointer operator->() noexcept {
    return &node_->value;
  }

  const_pointer operator->() const noexcept {
    return &node_->value;
  }
  
  Self& operator++() noexcept {
    node_ = node_->next;
    return *this;
  }

  Self& operator--() noexcept {
    node_ = node_->prev;
    return *this;
  }

  Self operator++(int) noexcept {
    auto ret = node_;
    node_ = node_->next;
    return ret;
  }

  Self operator--(int) noexcept {
    auto ret = node_;
    node_ = node_->prev;
    return ret;
  }

  friend bool operator==(BlistIterator x, BlistIterator y) noexcept {
    return x.node_ == y.node_;
  }

  friend bool operator!=(BlistIterator x, BlistIterator y) noexcept {
    return !(x == y);
  }

 private:
  Node* node_;
};
} // blist
} // algo
} // mmkv

#endif // _MMKV_ALGO_INTERNAL_BLIST_ITERATOR_H_