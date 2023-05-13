// SPDX-LICENSE-IDENTIFIER: Apache-2.0
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
template <typename T, typename Alloc>
using BNodeAllocator = typename Alloc::template rebind<BNode<T>>::other;

} // namespace blist

/**
 * \brief Bidirectional linked-list without sentinel node
 *
 * 之所以不提供哨兵是因为mmkv本身是内存敏感性应用，其他数据结构也是能省则省
 * 为了能提供end()/cend()以支持迭代，最后一个结点的next并不指向第一个结点
 * 链表结构如下：
 * header <-> node1 <-> node2 <-> tail node -> nullptr
 *   |                            ^
 *   ------------------------------
 *
 * \notssary
 *   Public class
 *   Should be copyable but not necessary for mmkv
 */
template <typename T, typename Alloc = LibcAllocatorWithRealloc<T>>
class Blist
  : protected blist::BNodeAllocator<T, Alloc>
  , protected Alloc {
  using NodeAllocTraits = std::allocator_traits<blist::BNodeAllocator<T, Alloc>>;
  using AllocTraits     = std::allocator_traits<Alloc>;

 public:
  using size_type       = size_t;
  using value_type      = T;
  using reference       = T &;
  using const_reference = T const &;
  using pointer         = T *;
  using const_pointer   = T const *;
  using iterator        = blist::BlistIterator<T>;
  using const_iterator  = blist::BlistConstIterator<T>;
  using Node            = blist::BNode<T>;

  Blist() noexcept
    : header_(nullptr)
    , count_(0)
  {
  }

  template <typename E>
  Blist(std::initializer_list<E> il)
    : Blist()
  {
    if (il.size() > 0) {
      header_ = CreateNode(std::move(*il.begin()));
    } else
      return;

    if (!header_) throw std::bad_alloc{};

    Node *prev = header_;

    for (size_t i = 1; i < il.size(); ++i) {
      auto node = CreateNode(std::move(*(il.begin() + i)));
      if (!node) throw std::bad_alloc{};
      prev->next = node;
      node->prev = prev;
      prev       = node;
    }

    assert(prev);
    header_->prev = prev;
    // prev->next = header_;
  }

  ~Blist() noexcept { Clear(); }

  Blist(Blist &&other) noexcept
    : header_(other.header_)
    , count_(other.count_)
  {
    other.header_ = nullptr;
    other.count_  = 0;
  }

  Blist &operator=(Blist &&other) noexcept
  {
    other.swap(*this);
    return *this;
  }

  bool      empty() const noexcept { return header_ == nullptr; }
  size_type size() const noexcept { return count_; }
  size_type max_size() const noexcept { return (size_type)-1; }

  void swap(Blist &o) noexcept
  {
    std::swap(o.header_, header_);
    std::swap(o.count_, count_);
  }

  iterator       begin() noexcept { return header_; }
  const_iterator begin() const noexcept { return header_; }
  const_iterator cbegin() const noexcept { return begin(); }

  iterator       end() noexcept { return nullptr; }
  const_iterator end() const noexcept { return nullptr; }
  const_iterator cend() const noexcept { return nullptr; }

  iterator       before_end() noexcept { return header_->prev; }
  const_iterator before_end() const noexcept { return header_->prev; }
  const_iterator cbefore_end() const noexcept { return before_end(); }

  T &Back() noexcept
  {
    assert(!empty());
    return header_->prev->value;
  }
  T const &Back() const noexcept
  {
    assert(!empty());
    return header_->prev->value;
  }
  T &Front() noexcept
  {
    assert(!empty());
    return header_->value;
  }
  T const &Front() const noexcept
  {
    assert(!empty());
    return header_->value;
  }

  Node       *FrontNode() noexcept { return header_; }
  Node const *FrontNode() const noexcept { return header_; }
  Node       *BackNode() noexcept { return header_->prev; }
  Node const *BackNode() const noexcept { return header_->prev; }

  void InsertAfter(Node *node, Node *new_node)
  {
    assert(new_node);
    if (!node) {
      assert(node == header_);
      SetHeader(node);
    } else {
      auto node_nn   = node->next;
      new_node->prev = node;
      new_node->next = node_nn;
      if (node_nn) {
        node_nn->prev = new_node;
      } else {
        // This is a new tail node
        header_->prev = new_node;
      }
      node->next = new_node;
    }
    count_++;
  }

#define PRE_PUSH                                                                                   \
  do {                                                                                             \
    if (empty()) {                                                                                 \
      SetHeader(node);                                                                             \
      count_ = 1;                                                                                  \
      return 1;                                                                                    \
    }                                                                                              \
                                                                                                   \
    if (size() > max_size()) {                                                                     \
      return 0;                                                                                    \
    }                                                                                              \
  } while (0)

  int PushFront(Node *node) noexcept
  {
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
    node->next    = header_;
    node->prev    = header_->prev;
    // header_->prev->next = new_node;
    header_->prev = node;
    header_       = node;
    count_++;

    return 1;
  }

  template <typename... Args>
  int PushFront(Args &&...args)
  {
    return PushFront(CreateNode(std::forward<Args>(args)...));
  }

  int PushBack(Node *node) noexcept
  {
    PRE_PUSH;

    /*
     * header -> node1 -> node2 -> nullptr
     * Push node after tail node:
     *  header -> node1 -> node2 -> new_node -> nullptr
     * Steps:
     * First, set node2->next and new_node->prev
     * then, set header->prev
     */
    node->prev          = header_->prev;
    // new_node->next = header_;
    header_->prev->next = node;
    header_->prev       = node;
    assert(!node->next);
    count_++;

    return 1;
  }

  template <typename... Args>
  int PushBack(Args &&...args)
  {
    return PushBack(CreateNode(std::forward<Args>(args)...));
  }

#define PRE_POP                                                                                    \
  do {                                                                                             \
    if (empty()) {                                                                                 \
      return 0;                                                                                    \
    }                                                                                              \
                                                                                                   \
    if (count_ == 1) {                                                                             \
      DropNode(header_);                                                                           \
      header_ = nullptr;                                                                           \
      count_  = 0;                                                                                 \
      return 1;                                                                                    \
    }                                                                                              \
  } while (0)

  int PopFront()
  {
    PRE_POP;

    /*
     * header -> node1 -> node2 -> nullptr
     * ==> node1(new_header) -> node2 -> nullptr
     * Steps:
     * set new_header, new_header->prev
     */
    auto new_header  = header_->next;
    new_header->prev = header_->prev;
    // new_header->prev->next = new_header;
    DropNode(header_);
    header_ = new_header;
    count_--;

    return 1;
  }

  int PopBack()
  {
    PRE_POP;

    /*
     * header -> node1 -> node2 -> nullptr
     * ==> header1 -> node1 -> nullptr
     * Steps:
     * First, set old_end = header->prev
     * then, set old_end->prev->next, header->prev
     */
    auto old_end        = header_->prev;
    old_end->prev->next = nullptr;
    // old_end->prev->next = header_;
    // old_end->next->prev = old_end->prev;
    header_->prev       = old_end->prev;
    DropNode(old_end);
    count_--;

    return 1;
  }

  void Extract(Node *node)
  {
    /* Avoid invalid node
     * e.g. Has extracted node */
    if (!node || !node->prev) return;

      /* Handling head node and tail node specially */
#if 0
    if (node == header_) {
      /* The head node is also a tail node */
      if (node->next)
        node->next->prev = node->prev;
      header_ = node->next;
    } else if (node == header_->prev) {
      assert(!node->next);
      node->prev->next = node->next;
      header_->prev = node->prev;
      assert(!header_->prev->next);
    } else {
      assert(node->next);
      node->next->prev = node->prev;
      node->prev->next = node->next;
      assert(!header_->prev->next);
    }
#else
    /* To head node,
     * 1. relink the next only(can't update prev since prev->next = NULL must)
     * 2. update header
     * To tail node,
     * 1. relink the prev only(The next must be NULL, so can't update the next)
     * 2. relink the prev of header(tail is head node also ok)
     * To regular node,
     * 1. relink prev
     * 2. relink next
     *
     * Summary:
     * Not tail, relink the next otherwise update header->prev
     * Not head, relink the prev otherwise update header */
    if (node->next) { /* Not a tail */
      node->next->prev = node->prev;
    } else {
      assert(!header_->prev->next);
      header_->prev = node->prev;
    }

    /* Later process node == head since updating header maybe */
    if (node != header_)
      node->prev->next = node->next;
    else {
      // No need to set the header_->prev
      header_ = node->next;
    }
#endif

    node->next = node->prev = nullptr;
    count_--;
  }

  void Erase(Node *node)
  {
    Extract(node);
    DropNode(node);
  }

  void DropNode(Node *node)
  {
    DestroyNode(node);
    FreeNode(node);
  }

  template <typename... Args>
  Node *CreateNode(Args &&...args)
  {
    auto node = AllocateNode();
    try {
      node->prev = node->next = nullptr;
      AllocTraits::construct(*this, &node->value, std::forward<Args>(args)...);
    }
    catch (...) {
      FreeNode(node);
      throw;
    }

    return node;
  }

  void Clear()
  {
    auto node = header_;
    for (; header_;) {
      node = header_->next;
      DropNode(header_);
      header_ = node;
    }
    count_ = 0;
    assert(count_ == 0);
  }

 private:
  void SetHeader(Node *header)
  {
    header_       = header;
    // header_->next = header_;
    header_->prev = header_;
    assert(header_->next == nullptr);
  }

  Node *AllocateNode() { return NodeAllocTraits::allocate(*this, 1); }

  // deprecated
  template <typename... Args>
  void ConstructNode(Node *node, Args &&...args)
  {
    AllocTraits::construct(*this, &node->value, std::forward<Args>(args)...);
  }

  void FreeNode(Node *node) { NodeAllocTraits::deallocate(*this, node, 1); }
  void DestroyNode(Node *node) { AllocTraits::destroy(*this, &node->value); }

  // Data member:
  Node     *header_;
  size_type count_;
};

} // namespace algo
} // namespace mmkv

#endif // _MMKV_ALGO_BLIST_H_
