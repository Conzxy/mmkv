#ifndef _MMKV_ALGO_SLIST_H_
#define _MMKV_ALGO_SLIST_H_

#include <initializer_list>
#include <stddef.h>
#include <memory>
#include <assert.h>

#include "libc_allocator_with_realloc.h"
#include "internal/slist_iterator.h"

namespace mmkv {
namespace algo {

template<typename T, typename Alloc>
using NodeAlloctor = typename Alloc::template rebind<SNode<T>>::other;

/**
 * \brief Single-linked-list but there is no sentinel to decrease memory usage
 *
 */
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
  static constexpr bool can_reallocate = true;

  Slist() noexcept
    : header_(nullptr) {
  }
  
  explicit Slist(std::initializer_list<value_type> il) 
    : Slist() {

    for (auto const& e : il) {
      EmplaceFront(e);
    }
  }

  ~Slist() noexcept {
    for (; header_; ) {
      auto old_next = header_->next;
      NodeAllocTraits::destroy(*this, header_);
      NodeAllocTraits::deallocate(*this, header_, 1);
      header_ = old_next;
    }
  }
  
  Slist(Slist const& other) 
    : header_(nullptr)
  {
    if (other.empty()) {
      return;
    }

    EmplaceFront(other.Front());
    
    auto header = header_;
    auto header2 = other.header_->next;
    
    for (; header2 != nullptr; 
         header2 = header2->next, header = header->next) {
      InsertAfterNode(header, header2->value);
    } 
  }

  Slist& operator=(Slist const& other) {
    if (this == &other) {
      return *this;
    }

    if (empty()) {
      Slist(other).swap(*this);
    }

    auto header = header_;
    auto header2 = other.header_;

    for (;;) {
      header->value = header2->value;

      if (header->next == nullptr) {
        header2 = header2->next;
        break;
      }
  
      header2 = header2->next;
      if (header2 == nullptr) {
        break;
      }

      header = header->next;
    }
    
    if (header->next == nullptr) {
      for (; header2 != nullptr; header2 = header2->next) {
        InsertAfterNode(header, header2->value);
        header = header->next;
      }  
    } else {
      while (header->next != nullptr) {
        EraseAfterNode(header);
      }

    }

    return *this;
  }

  Slist(Slist&& other) noexcept
    : header_(other.header_) {
      other.header_ = nullptr; 
  }  
  
  Slist& operator=(Slist&& other) noexcept {
    this->swap(other);
    return *this;
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
  
  void InsertAfter(const_iterator pos, value_type const& val) {
    InsertAfterNode(pos.node_, val);
  } 

  void InsertAfter(const_iterator pos, value_type&& val) {
    InsertAfterNode(pos.node_, std::move(val));
  }

  Node* FrontNode() noexcept {
    return header_;
  }

  Node const* FrontNode() const noexcept {
    return header_;
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
    auto node = CreateNode(std::forward<Args>(args)..., nullptr);
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
  
  void EraseAfter(const_iterator pos) {
    EraseAfterNode(pos.node_);
  }
  
  /* 
   * Hack method
   * 允许用户进行一定的定制化
   * 比如销毁对象需要一些别的手法而不是通过Destory()
   * 但是释放对象还是要调用FreeNode()
   */
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
    NodeAllocTraits::deallocate(*this, node, 1);
  }

  void DestroyNode(Node* node) {
    NodeAllocTraits::destroy(*this, node);
  }
  
  void DropNode(Node* node) {
    DestroyNode(node);
    FreeNode(node);
  }

 private:
  void EraseAfterNode(Node* pos) {
    assert(pos != nullptr);

    auto node = pos->next;
    pos->next = pos->next->next;
    DropNode(node);
  }

  template<typename U>
  void InsertAfterNode(Node* pos, U&& val) {
    auto node = CreateNode(std::forward<U>(val));
    node->next = pos->next;
    pos->next = node;
  }

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

  Node* header_;
};

} // algo
} // mmkv

namespace std {

template<typename T>
inline void swap(mmkv::algo::Slist<T>& x, mmkv::algo::Slist<T>& y) noexcept(noexcept(x.swap(y))) {
  x.swap(y);
}

} // std

#endif // _MMKV_ALGO_SLIST_H_
