// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_ALGO_INTERNAL_TREE_HASH_TABLE_ITERATOR_H_
#define _MMKV_ALGO_INTERNAL_TREE_HASH_TABLE_ITERATOR_H_

#include <assert.h>

#include "mmkv/zstl/iterator.h"
#include "avl_util.h"

namespace mmkv {
namespace algo {

#define TREE_HASH_TABLE_PARAMS_LIST typename K,typename T,typename HF,typename GK,typename Comparator,typename Alloc
#define TREE_HASH_TABLE_ARGS_LIST K,T,HF,GK,Comparator,Alloc

template<TREE_HASH_TABLE_PARAMS_LIST>
class TreeHashTable;

#define TABLE_TYPE TreeHashTable<TREE_HASH_TABLE_ARGS_LIST>

template<TREE_HASH_TABLE_PARAMS_LIST>
class TreeHashTableConstIterator {
 protected:
  using table_type = TABLE_TYPE;
  using Self = TreeHashTableConstIterator;
  using size_type = typename table_type::size_type;
  using Slot = typename table_type::Slot;

  friend class TABLE_TYPE;
 public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = T;
  using reference = T const&;
  using pointer = T const*;
  using difference_type = std::ptrdiff_t;
  
  TreeHashTableConstIterator() noexcept
    : TreeHashTableConstIterator(nullptr, 0, 0)
  {
  }
  
  ~TreeHashTableConstIterator() noexcept = default;

  TreeHashTableConstIterator(TreeHashTableConstIterator const&) = default;
  TreeHashTableConstIterator& operator=(TreeHashTableConstIterator const&) = default;
  TreeHashTableConstIterator(TreeHashTableConstIterator&&) = default;
  TreeHashTableConstIterator& operator=(TreeHashTableConstIterator&&) = default;

  
  Self& operator++() noexcept {
    Increment();
    return *this;
  }

  Self operator++(int) noexcept {
    auto ret = *this;
    Increment();

    return ret;
  }

  reference operator*() noexcept {
    assert(slot_);
    return slot_->value;
  }

  reference operator*() const noexcept {
    assert(slot_);
    return slot_->value;
  }
  
  friend constexpr bool operator==(TreeHashTableConstIterator const& x, TreeHashTableConstIterator const& y) noexcept {
    return x.slot_ == y.slot_;
  }

  friend constexpr bool operator!=(TreeHashTableConstIterator const& x, TreeHashTableConstIterator const& y) noexcept {
    return !(x == y);
  }

 protected:
  explicit TreeHashTableConstIterator(table_type const* ht, int table_index=0, size_type bucket_index=0)
    : ht_((table_type*)ht)
    , table_index_(table_index)
    , bucket_index_(bucket_index)
    , slot_(nullptr)
  { 
    if (!ht_) {
      return;
    }

    for (int i = table_index_; i < 2; ++i) {
      auto* table = &ht_->table(i);
      auto size = table->size();
      for (size_type j = bucket_index_; j < size; ++j) {
        if (!(*table)[j].empty()) {
          table_index_ = i;
          bucket_index_ = j;
          slot_ = (*table)[j].FirstNode();
          break;
        }
      }

      if (slot_ ) break;
    }
  }
  
  void Increment() noexcept {
    assert(slot_);

    slot_ = (Slot*)_GetNextNode(slot_);
    if (slot_ == nullptr) {
      for (int i = table_index_; i < 2; ++i) {
        auto* table = &ht_->table(table_index_);
        auto const size = table->size();
        Slot* slot = nullptr;

        for (auto j = bucket_index_+1; j < size; ++j) {
          if (!(*table)[j].empty()) {
            slot = (*table)[j].FirstNode();
            assert(slot);

            bucket_index_ = j;
            table_index_ = i;
            break;
          }
        }
        
        slot_ = slot;

        if (slot_) { 
          break;
        }

        bucket_index_ = -1;
        table_index_ = 1;

      }
    }
  }
  
  //************************************************************
  // ht_, table_index_, slot_是必须要的
  // 而bucket_index_可以通过哈希函数计算出来
  // 但这里为避免多一次计算记录桶索引
  //************************************************************
  table_type* ht_;
  int table_index_;
  size_type bucket_index_;
  Slot* slot_;
};

template<TREE_HASH_TABLE_PARAMS_LIST>
class TreeHashTableIterator : public TreeHashTableConstIterator<TREE_HASH_TABLE_ARGS_LIST> {
 protected:
  friend class TABLE_TYPE;
  using Base = TreeHashTableConstIterator<TREE_HASH_TABLE_ARGS_LIST>; 
  using Self = TreeHashTableIterator;  
  using typename Base::table_type;
  using typename Base::size_type;

 public:
  using reference = T&;
  using const_reference = T const&;
  using pointer = T*;
  using const_pointer = T const*;

  TreeHashTableIterator()
    : Base()
  {
  }
  
  ~TreeHashTableIterator() = default;

  TreeHashTableIterator(TreeHashTableIterator const&) = default;
  TreeHashTableIterator& operator=(TreeHashTableIterator const&) = default;
  TreeHashTableIterator(TreeHashTableIterator&&) = default;
  TreeHashTableIterator& operator=(TreeHashTableIterator&&) = default;
  
  reference operator*() noexcept {
    return slot_->value;
  }

  const_reference operator*() const noexcept {
    return slot_->value;
  }
  
  Self& operator++() noexcept {
    Base::Increment();
    return *this;
  }

  Self operator++(int) noexcept {
    auto ret = *this;
    Base::Increment();
    return ret;
  }

 protected:
  explicit TreeHashTableIterator(table_type* ht, int table_index=0, size_type bucket_index=0)
    : Base(ht, table_index, bucket_index)
  {
  }

  using Base::slot_;
};

} // algo
} // mmkv

#endif // _MMKV_ALGO_INTERNAL_TREE_HASH_TABLE_ITERATOR_H_