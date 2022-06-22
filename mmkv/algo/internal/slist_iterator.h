#ifndef _MMKV_ALGO_SLIST_ITERATOR_H
#define _MMKV_ALGO_SLIST_ITERATOR_H

#include "snode.h"
#include <iterator>

namespace mmkv {
namespace algo {

template<typename T, typename A>
class Slist;

template<typename T>
class SlistConstIterator;

template<typename T>
class SlistIterator {
  friend class SlistConstIterator<T>;

  template<typename U, typename A>
  friend class Slist;

  using Self = SlistIterator;
 public:
  using Node = SNode<T>;
  using value_type = T;
  using reference = T&;
  using const_reference = T const&;
  using pointer = T*;
  using const_pointer = T const*;
  using difference_type = std::ptrdiff_t;
  using iterator_category = std::forward_iterator_tag;

  SlistIterator() = default;
  explicit SlistIterator(Node* node) : node_(node) {
  }

  ~SlistIterator() noexcept = default;
  
  Self& operator++() {
    node_ = node_->next;
    return *this;
  }

  Self operator++(int) {
    auto ret = node_;
    node_ = node_->next;
    return Self(ret);
  }
  
  const_reference operator*() const noexcept {
    return node_->value;
  } 

  reference operator*() noexcept {
    return node_->value;
  }
  
  friend bool operator==(SlistIterator x, SlistIterator y) noexcept { return x.node_ == y.node_; }
  friend bool operator!=(SlistIterator x, SlistIterator y) noexcept { return !(x == y); }

 private:
  Node* node_;
};

template<typename T>
class SlistConstIterator {
  using Self = SlistConstIterator;
  
  template<typename U, typename A>
  friend class Slist;

 public:
  using Node = SNode<T>;
  using value_type = T;
  using reference = T const&;
  using const_reference = T const&;
  using pointer = T const*;
  using const_pointer = T const*;
  using difference_type = std::ptrdiff_t;
  using iterator_category = std::forward_iterator_tag;

  SlistConstIterator() = default;
  explicit SlistConstIterator(Node const* node) : node_(const_cast<Node*>(node)) {
  }
  
  // Don't declare as SlistIterator<T> const&
  // since it is just a pointer wrapper 
  SlistConstIterator(SlistIterator<T> iter) : node_(iter.node_) {
  } 

  ~SlistConstIterator() noexcept = default;

  Self& operator++() {
    node_ = node_->next;
    return *this;
  }

  Self operator++(int) {
    auto ret = node_;
    node_ = node_->next;
    return Self(ret);
  }
  
  const_reference operator*() const noexcept {
    return node_->value;
  } 

  reference operator*() noexcept {
    return node_->value;
  }
  
  friend bool operator==(SlistConstIterator x, SlistConstIterator y) noexcept { return x.node_ == y.node_; }
  friend bool operator!=(SlistConstIterator x, SlistConstIterator y) noexcept { return !(x == y); }
 private:
  SlistIterator<T> ToIterator() noexcept {
    return SlistIterator<T>(node_);
  }

  Node* node_;
};

} // algo
} // mmkv

#endif // _MMKV_ALGO_SLIST_ITERATOR_H
