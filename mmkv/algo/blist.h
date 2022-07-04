#ifndef _MMKV_ALGO_BLIST_H_
#define _MMKV_ALGO_BLIST_H_

#include <iterator>
#include <utility>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#include "libc_allocator_with_realloc.h"

namespace mmkv {
namespace algo {

namespace blist {

template<typename T>
struct BNode {
  using LinkType = BNode*;
  T value; 
  LinkType prev;
  LinkType next; 
  
  template<typename... Args>
  explicit BNode(Args&&... v)
    : value(std::forward<Args>(v)...)
    , prev(nullptr)
    , next(nullptr) 
  {
  }
};

template<typename T, typename Alloc>
using BNodeAllocator = typename Alloc::template rebind<BNode<T>>::other;

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

template<typename T, typename Alloc=LibcAllocatorWithRealloc<T>>
class Blist : protected blist::BNodeAllocator<T, Alloc> {
  using Node = blist::BNode<T>;
  using NodeAllocTraits = std::allocator_traits<blist::BNodeAllocator<T, Alloc>>;

 public:
  using size_type = size_t;
  using value_type = T;
  using reference = T&;
  using const_reference = T const&;
  using pointer = T*;
  using const_pointer = T const*;
  using iterator = blist::BlistIterator<T>;
  using const_iterator = blist::BlistConstIterator<T>;

  Blist() noexcept 
    : header_(nullptr) 
    , count_(0)
  {
  }

  ~Blist() noexcept {
    auto node = header_;
    for (size_type i = 0; i < count_; ++i) {
      node = header_->next;
      DropNode(header_);
      header_ = node;
    }

    header_ = nullptr;
    count_ = 0;
  }
  
  Blist(Blist&& other) noexcept 
    : header_(other.header_)
    , count_(other.count_) 
  {
    other.header_ = nullptr;
    other.count_ = 0;
  }

  Blist& operator=(Blist&& other) noexcept {
    other.swap(*this);
    return *this;
  }

  bool empty() const noexcept {
    return header_ == nullptr;
  } 

  size_type size() const noexcept {
    return count_;
  }

  size_type max_size() const noexcept {
    return (size_type)-1;
  }
  
  void swap(Blist& o) noexcept {
    std::swap(o.header_, header_);
    std::swap(o.count_, count_);
  }

  iterator begin() noexcept { return header_; }
  const_iterator begin() const noexcept { return header_; }
  iterator before_end() noexcept { return header_->prev; }
  const_iterator before_end() const noexcept { return header_->prev; }
  const_iterator cbegin() const noexcept { return begin(); }
  const_iterator cbefore_end() const noexcept { return before_end(); }

#define PRE_PUSH \
    if (empty()) { \
      SetHeader(CreateNode(std::forward<Args>(args)...)); \
      return 1; \
    } \
     \
    if (size() > max_size()) { \
      return 0; \
    }

  template<typename... Args>  
  int PushFront(Args&&... args) {
    PRE_PUSH

    auto new_node = CreateNode(std::forward<Args>(args)...);
    new_node->next = header_;
    new_node->prev = header_->prev;
    header_->prev->next = new_node;
    header_->prev = new_node;
    header_ = new_node;

    return 1;
  }

  template<typename... Args>
  int PushBack(Args&&... args) {
    PRE_PUSH

    auto new_node = CreateNode(std::forward<Args>(args)...);
    new_node->prev = header_->prev;
    new_node->next = header_;
    header_->prev->next = new_node;
    header_->prev = new_node;

    return 1;
  }
  
  #define PRE_POP \
    if (empty()) { \
      return 0; \
    } \
 \
    if (count_ == 1) { \
      DropNode(header_); \
      header_ = nullptr; \
      return 1; \
    }

  int PopFront() {
    PRE_POP

    auto new_header = header_->next;
    new_header->prev = header_->prev;
    new_header->prev->next = new_header;
    DropNode(header_);
    header_ = new_header;
    
    return 1;
  }

  int PopBack() {
    PRE_POP

    auto old_end = header_->prev;
    old_end->prev->next = header_;
    old_end->next->prev = old_end->prev;
    DropNode(old_end);
    return 1;
  }
  
  T& Back() noexcept {
    assert(!empty());
    return header_->prev->value;
  }

  T const& Back() const noexcept {
    assert(!empty());
    return header_->prev->value;
  }

  T& Front() noexcept {
    assert(!empty());
    return header_->value;
  }

  T const& Front() const noexcept {
    assert(!empty());
    return header_->value;
  }

 private:
  void SetHeader(Node* header) {
    header_ = header;
    header_->next = header_;
    header_->prev = header_;
  }

  Node* AllocateNode() {
    ++count_;
    return NodeAllocTraits::allocate(*this, 1);
  }
  
  template<typename... Args>
  void ConstructNode(Node* node, Args&&... args) {
    NodeAllocTraits::construct(*this, node, std::forward<Args>(args)...);
  } 

  template<typename... Args>
  Node* CreateNode(Args&&... args) {
    auto node = AllocateNode();
    try {
      ConstructNode(node, std::forward<Args>(args)...);
    } catch (...) {
      FreeNode(node);
      throw;
    }

    return node;
  }

  void FreeNode(Node* node) {
    NodeAllocTraits::deallocate(*this, node, 1);
    count_--;
  }

  void DestroyNode(Node* node) {
    NodeAllocTraits::destroy(*this, node);
  }
  
  void DropNode(Node* node) {
    DestroyNode(node);
    FreeNode(node);
  }

  Node* header_;
  size_type count_;
};

} // algo
} // mmkv

#endif // _MMKV_ALGO_BLIST_H_
