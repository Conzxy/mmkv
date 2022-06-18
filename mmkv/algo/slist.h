#ifndef _MMKV_ALGO_SLIST_H_
#define _MMKV_ALGO_SLIST_H_

#include <stddef.h>
#include <iterator>
#include <memory>

#include "libc_allocator_with_realloc.h"

namespace mmkv {
namespace algo {

template<typename T>
struct SNode {
  SNode* next;
  T value;
  
  SNode()
    : next(nullptr), value()
  { } 
  
  SNode(SNode* nxt)
    : next(nxt), value()
  { }

  SNode(T const& val, SNode* nxt=nullptr)
    : next(nxt), value(val)
  { }

  SNode(T&& val, SNode* nxt=nullptr)
    : next(nxt), value(std::move(val))
  { }
};

template<typename T>
class SlistConstIterator;

template<typename T>
class SlistIterator {
  friend class SlistConstIterator<T>;

  using Node = SNode<T>;
  using Self = SlistIterator;
  using value_type = T;
  using reference = T&;
  using const_reference = T const&;
  using pointer = T*;
  using const_pointer = T const*;
  using difference_type = std::ptrdiff_t;
  using iterator_category = std::forward_iterator_tag;

 public:
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
  using Node = SNode<T>;
  using Self = SlistConstIterator;
  using value_type = T;
  using reference = T&;
  using const_reference = T const&;
  using pointer = T*;
  using const_pointer = T const*;
  using difference_type = std::ptrdiff_t;
  using iterator_category = std::forward_iterator_tag;

 public:
  SlistConstIterator() = default;
  explicit SlistConstIterator(Node const* node) : node_(node) {
  }

  explicit SlistConstIterator(Node* node) : node_(node) {
  }
  
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
  Node const* node_;
};

template<typename T, typename Alloc>
using NodeAlloctor = typename Alloc::template rebind<SNode<T>>::other;

template<typename T, typename Alloc=LibcAllocatorWithRealloc<T>>
class Slist : protected NodeAlloctor<T, Alloc> {
  using Node = SNode<T>;
  using NodeAllocTraits = std::allocator_traits<NodeAlloctor<T, Alloc>>;

 public: 
  using size_type = std::size_t;
  using value_type = T;
  using reference = T&;
  using pointer = T*;
  using const_reference = T const&;
  using const_pointer = T const*;
  using iterator = SlistIterator<T>;
  using const_iterator = SlistConstIterator<T>;

  Slist() 
    : header_(nullptr) {
  }

  ~Slist() noexcept {
    for (; header_; ) {
      auto old_next = header_->next;
      NodeAllocTraits::destroy(*this, header_);
      NodeAllocTraits::deallocate(*this, header_, sizeof(Node));
      header_ = old_next;
    }

    header_ = nullptr; 
  }
  
  iterator begin() noexcept { return iterator(header_); }
  const_iterator begin() const noexcept { return const_iterator(header_); } 
  iterator end() noexcept { return iterator(nullptr); }
  const_iterator end() const noexcept { return const_iterator(nullptr); }
  iterator cbegin() const noexcept { return iterator(header_); } 
  const_iterator cend() const noexcept { return const_iterator(nullptr); }
  
  bool IsEmpty() const noexcept { return header_ == nullptr; }
  bool empty() const noexcept { return header_ == nullptr; } 

  // For debugging
  size_type GetSize() const noexcept {
    size_type ret = 0;
    for (Node* header = header_; header; header = header->next) {
      ret++;
    }

    return ret;
  }

  template<typename... Args> 
  void EmplaceFront(Args&&... args) {
    auto node = CreateNode(std::forward<Args>(args)...);
    EmplaceFrontNode(node);
  }
  
  void EmplaceFrontNode(Node* node) {
    if (header_) {
      node->next = header_;
    }

    header_ = node;
  } 
  
  Node* ExtractNode(value_type const& value) {
    if (!header_) {
      return nullptr;
    }
  
    Node* ret = nullptr;

    if (header_->value == value) {
      ret = header_;
      header_ = ret->next;
      ret->next = nullptr;
      return ret;
    }

    for (Node* header = header_; header->next; header = header->next) {
      if (header->next->value == value) {
        ret = header->next;
        header->next = header->next->next;
        ret->next = nullptr;
        break;
      }
    }

    return ret;
  }
  
  size_type Erase(value_type const& value) {
    auto node = ExtractNode(value);

    if (node) {
      DropNode(node);
      return 1;
    }

    return 0;
  }

 private:
  Node* AllocateNode() {
    return NodeAllocTraits::allocate(*this, 1);
  }
  
  template<typename... Args>
  void ConstructNode(Node* node, Args&&... args) {
    NodeAllocTraits::construct(*this, node, std::forward<Args>(args)...);
  } 

  template<typename... Args>
  Node* CreateNode(Args&&... args) {
    auto node = AllocateNode();
    ConstructNode(node, std::forward<Args>(args)...);
    return node;
  }

  void FreeNode(Node* node) {
    NodeAllocTraits::deallocate(*this, node, sizeof(Node));
  }

  void DestroyNode(Node* node) {
    NodeAllocTraits::destroy(*this, node);
  }
  
  void DropNode(Node* node) {
    DestroyNode(node);
    FreeNode(node);
  }


  Node* header_;
};

} // algo
} // mmkv

#endif // _MMKV_ALGO_SLIST_H_
