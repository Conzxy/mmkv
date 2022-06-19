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
 public:
  using Node = SNode<T>;
  using value_type = T;
  using reference = T&;
  using const_reference = T const&;
  using pointer = T*;
  using const_pointer = T const*;
  using difference_type = std::ptrdiff_t;
  using iterator_category = std::forward_iterator_tag;

  SlistConstIterator() = default;
  explicit SlistConstIterator(Node const* node) : node_(node) {
  }

  explicit SlistConstIterator(Node* node) : node_(node) {
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
  Node const* node_;
};

template<typename T, typename Alloc>
using NodeAlloctor = typename Alloc::template rebind<SNode<T>>::other;

template<typename T, typename Alloc=LibcAllocatorWithRealloc<T>>
class Slist : protected NodeAlloctor<T, Alloc> {
  using NodeAllocTraits = std::allocator_traits<NodeAlloctor<T, Alloc>>;

 public: 
  using Node = SNode<T>;
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
  
  value_type& Front() noexcept {
    assert(header_);
    return header_->value;
  }

  value_type const& Front() const noexcept {
    assert(header_);
    return header_->value;
  }

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
  
  Node* ExtractFront() noexcept {
    auto ret = header_;
    header_ = header_->next;
    ret->next = nullptr;
    
    return ret;
  }
  
  Node const* ExtractFront() const noexcept {
    return const_cast<Node*>(this)->ExtractFront();
  }

  iterator Search(value_type const& value) {
    return SearchIf([&value](value_type const& val) { return val == value; });
  } 
  
  const_iterator Search(value_type const& value) const {
    return const_iterator(const_cast<Slist*>(this)->Search(value));
  }
  
  template<typename UnaryPred>
  iterator SearchIf(UnaryPred pred) {
    return iterator(SearchNodeIf(pred));
  }

  template<typename UnaryPred>
  const_iterator SearchIf(UnaryPred pred) const {
    return const_iterator(SearchNodeIf(pred));
  }

  Node* ExtractNode(value_type const& value) {
    return ExtractNodeIf([&value](value_type const& val) { return value == val; });
  } 

  template<typename UnaryPred> 
  Node* ExtractNodeIf(UnaryPred pred) {
    if (!header_) {
      return nullptr;
    }
  
    Node* ret = nullptr;

    if (pred(header_->value)) {
      ret = header_;
      header_ = ret->next;
      ret->next = nullptr;
      return ret;
    }

    for (Node* header = header_; header->next; header = header->next) {
      if (pred(header->next->value)) {
        ret = header->next;
        header->next = header->next->next;
        ret->next = nullptr;
        break;
      }
    }

    return ret;
  }
  
  template<typename UnaryPred>
  size_type EraseIf(UnaryPred pred) {
    auto node = ExtractNodeIf(pred);

    if (node) {
      DropNode(node);
      return 1;
    }

    return 0;
  }

  size_type Erase(value_type const& value) {
    return EraseIf([&value](value_type const& val) {
        return value == val;
        });
  }
  
  void swap(Slist& other) noexcept {
    std::swap(header_, other.header_);
  }

 private:
  template<typename UnaryPred>
  Node* SearchNodeIf(UnaryPred pred) const {
    for (auto header = header_;
         header;
         header = header->next) {
      if (pred(header->value)) {
        return header;
      }
    }

    return nullptr;
  }

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
    try {
      ConstructNode(node, std::forward<Args>(args)...);
    } catch (...) {
      FreeNode(node);
      throw;
    }

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

namespace std {

template<typename T>
constexpr void swap(mmkv::algo::Slist<T>& x, mmkv::algo::Slist<T>& y) noexcept(noexcept(x.swap(y))) {
  x.swap(y);
}

} // std

#endif // _MMKV_ALGO_SLIST_H_
