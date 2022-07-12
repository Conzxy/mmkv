#ifndef _MMKV_ALGO_BLIST_H_
#define _MMKV_ALGO_BLIST_H_

#include <iterator>
#include <utility>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <initializer_list>

#include "libc_allocator_with_realloc.h"
#include "internal/bnode.h"
#include "internal/blist_iterator.h"

namespace mmkv {
namespace algo {
namespace blist {

// For EBCO
template<typename T, typename Alloc>
using BNodeAllocator = typename Alloc::template rebind<BNode<T>>::other;

} // blist

/**
 * \brief Bidirectional linked-list without sentinel node
 * 
 * 之所以不提供哨兵是因为mmkv本身是内存敏感性应用，其他数据结构也是能省则省
 * 为了能提供end()/cend()以支持迭代，最后一个结点的next并不指向第一个结点
 * 链表结构如下：
 * header -> node1 -> node2 -> tail node -> nullptr
 *   |                            ^
 *   ------------------------------
 *
 * \note
 *   Public class
 *   copyable
 */
template<typename T, typename Alloc=LibcAllocatorWithRealloc<T>>
class Blist : protected blist::BNodeAllocator<T, Alloc> {
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
  using Node = blist::BNode<T>;

  Blist() noexcept 
    : header_(nullptr) 
    , count_(0)
  {
  }

  template<typename E>
  Blist(std::initializer_list<E> il) 
    : Blist()
  {
    if (il.size() > 0) {
      header_ = CreateNode(std::move(*il.begin()));
    } else return;

    if (!header_) throw std::bad_alloc{};

    Node* prev = header_;

    for (size_t i = 1; i < il.size(); ++i) {
      auto node = CreateNode(std::move(*(il.begin()+i)));
      if (!node) throw std::bad_alloc{};
      prev->next = node;
      node->prev = prev;
      prev = node;
    }

    assert(prev);
    header_->prev = prev;
    // prev->next = header_;
  }

  ~Blist() noexcept {
    auto node = header_;
    for (; header_;) {
      node = header_->next;
      DropNode(header_);
      header_ = node;
    }
  }

  Blist(Blist&& other) noexcept 
    : header_(other.header_)
    , count_(other.count_) 
  {
    other.header_ = nullptr;
    other.count_ = 0;
  }

  Blist& operator=(Blist&& other) noexcept { other.swap(*this); return *this; }

  bool empty() const noexcept { return header_ == nullptr; } 
  size_type size() const noexcept { return count_; }
  size_type max_size() const noexcept { return (size_type)-1; }
  
  void swap(Blist& o) noexcept {
    std::swap(o.header_, header_);
    std::swap(o.count_, count_);
  }

  iterator begin() noexcept { return header_; }
  const_iterator begin() const noexcept { return header_; }
  const_iterator cbegin() const noexcept { return begin(); }

  iterator end() noexcept { return nullptr; }
  const_iterator end() const noexcept { return nullptr; }
  const_iterator cend() const noexcept { return nullptr; }

  iterator before_end() noexcept { return header_->prev; }
  const_iterator before_end() const noexcept { return header_->prev; }
  const_iterator cbefore_end() const noexcept { return before_end(); }

  T& Back() noexcept { assert(!empty()); return header_->prev->value; }
  T const& Back() const noexcept { assert(!empty()); return header_->prev->value; }
  T& Front() noexcept { assert(!empty()); return header_->value; }
  T const& Front() const noexcept { assert(!empty()); return header_->value; }

#define PRE_PUSH do {\
    if (empty()) { \
      SetHeader(CreateNode(std::forward<Args>(args)...)); \
      return 1; \
    } \
     \
    if (size() > max_size()) { \
      return 0; \
    } } while (0)

  template<typename... Args>  
  int PushFront(Args&&... args) {
    PRE_PUSH;

    /*
     * header -> node1 -> node2 -> nullptr
     * Push node before header:
     *   node -> header -> node1 -> node2 -> nullptr
     * Steps:
     * First, set node->prev and node->next
     * then set header->prev
     * Last, set header_ to new node
     */  
    auto new_node = CreateNode(std::forward<Args>(args)...);
    new_node->next = header_;
    new_node->prev = header_->prev;
    // header_->prev->next = new_node;
    header_->prev = new_node;
    header_ = new_node;

    return 1;
  }

  template<typename... Args>
  int PushBack(Args&&... args) {
    PRE_PUSH;

    /*
     * header -> node1 -> node2 -> nullptr
     * Push node after tail node:
     *  header -> node1 -> node2 -> new_node -> nullptr
     * Steps:
     * First, set node2->next and new_node->prev
     * then, set header->prev
     */
    auto new_node = CreateNode(std::forward<Args>(args)...);
    new_node->prev = header_->prev;
    // new_node->next = header_;
    header_->prev->next = new_node;
    header_->prev = new_node;

    return 1;
  }
  
  #define PRE_POP do {\
    if (empty()) { \
      return 0; \
    } \
 \
    if (count_ == 1) { \
      DropNode(header_); \
      header_ = nullptr; \
      return 1; \
    } } while (0)

  int PopFront() {
    PRE_POP;

    /*
     * header -> node1 -> node2 -> nullptr
     * ==> node1(new_header) -> node2 -> nullptr
     * Steps:
     * set new_header, new_header->prev
     */
    auto new_header = header_->next;
    new_header->prev = header_->prev;
    // new_header->prev->next = new_header;
    DropNode(header_);
    header_ = new_header;
    
    return 1;
  }

  int PopBack() {
    PRE_POP;

    /*
     * header -> node1 -> node2 -> nullptr
     * ==> header1 -> node1 -> nullptr
     * Steps:
     * First, set old_end = header->prev
     * then, set old_end->prev->next, header->prev 
     */
    auto old_end = header_->prev;
    old_end->prev->next = nullptr;
    // old_end->prev->next = header_;
    // old_end->next->prev = old_end->prev;
    header_->prev = old_end->prev;
    DropNode(old_end);
    return 1;
  }

 private:
  void SetHeader(Node* header) {
    header_ = header;
    // header_->next = header_;
    header_->prev = header_;
  }

  Node* AllocateNode() {
    ++count_;
    return NodeAllocTraits::allocate(*this, 1);
  }

  // deprecated
  template<typename... Args>
  void ConstructNode(Node* node, Args&&... args) {
    NodeAllocTraits::construct(*this, &node->value, std::forward<Args>(args)...);
  } 

  template<typename... Args>
  Node* CreateNode(Args&&... args) {
    auto node = AllocateNode();
    try {
      node->prev = node->next = nullptr;
      NodeAllocTraits::construct(*this, &node->value, std::forward<Args>(args)...);
    } catch (...) {
      FreeNode(node);
      throw;
    }

    return node;
  }

  void FreeNode(Node* node) { NodeAllocTraits::deallocate(*this, node, 1); count_--; }
  void DestroyNode(Node* node) { NodeAllocTraits::destroy(*this, &node->value); }
  void DropNode(Node* node) { DestroyNode(node); FreeNode(node); }

  // Data member:
  Node* header_;
  size_type count_;
};

} // algo
} // mmkv

#endif // _MMKV_ALGO_BLIST_H_