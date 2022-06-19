#ifndef _MMKV_ALGO_HASH_TABLE_H_
#define _MMKV_ALGO_HASH_TABLE_H_

#ifdef _DEBUG_HASH_TABLE_
  #include <stdio.h>
  #include <iostream>
#endif

#include <memory>
#include <stdint.h>
#include <stddef.h>

#include "mmkv/algo/reserved_array.h"
#include "mmkv/algo/slist.h"

namespace mmkv {
namespace algo {

template<typename T, typename Alloc>
using BucketAllocator = typename Alloc::template rebind<Slist<T, Alloc>>::other;

template<typename K, typename T, typename HF, typename GK, typename EK, typename Alloc>
class HashTable : protected BucketAllocator<T, Alloc> {
  using BucketAllocTraits = std::allocator_traits<BucketAllocator<T, Alloc>>;

  using EqualKey = EK;
  using GetKey = GK;
  using HashFunc = HF;
  using AllocTraits = std::allocator_traits<Alloc>;
public:
  using key_type = K;
  using value_type = T;
  using reference = T&;
  using const_reference = T const&;
  using pointer = T*;
  using const_pointer = T const*;
  using size_type = size_t;
  using hash_function = HF;
  using equal_key = EK;
  using allocator_type = Alloc;

  HashTable();
  ~HashTable() noexcept;
  
  value_type* Insert(T const& elem) {
    return Insert_impl(elem);
  }

  value_type* Insert(T&& elem) {
    return Insert_impl(std::move(elem));
  }
  
  value_type* Find(K const& key);

  value_type const* Find(K const& key) const {
    return const_cast<HashTable*>(this)->Find(key);
  }
  
  size_type Erase(K const& key);
  
  size_type size() const noexcept {
    return table1().used;
  }

  size_type GetSize() const noexcept {
    return size();
  }

  bool empty() const noexcept {
    return size() == 0;
  }

  // For debugging
  void DebugPrint();
 private:
  using Bucket = Slist<value_type, Alloc>;

  struct Table {
    ReservedArray<Bucket> table;
    size_t used;
    size_t size_mask;
      
    Table();
    ~Table() noexcept; 
    
    void swap(Table& other) noexcept {
      std::swap(other.table, table);
      // table.swap(other.table);
      // std::swap(other.used, used);
      std::swap(other.size_mask, size_mask);
    }
    
    Bucket& operator[](size_type i) noexcept {
      return table[i];
    } 

    Bucket const& operator[](size_type i) const noexcept {
      return table[i];
    }

    size_type size() const noexcept { return table.size(); }
    bool empty() const noexcept { return table.empty(); }

    void Grow(size_type expected_size) { 
      table.Grow(expected_size);
      size_mask = size() - 1;
    }
  };

  void Rehash();
  void IncremetalRehash(); 

  bool InRehashing() const noexcept {
    // If rehash_move_bucket_index == -1, indicates the
    // progress of rehash is completed.
    return rehash_move_bucket_index_ != (size_type)-1;
  }
  
  bool CheckDuplicate(Bucket* bucket, value_type const& elem) {

    if (bucket->SearchIf([this, &elem](value_type const& val) {
          return ek_(gk_(elem), gk_(val));
          }) != bucket->end()) {
        return true;
    }

    return false;
  }

  template<typename U>
  value_type* Insert_impl(U&& elem);
  
  size_type BucketIndex1(key_type const& elem) const noexcept {
    return BucketIndex(0, elem);
  }

  size_type BucketIndex2(key_type const& elem) const noexcept {
    return BucketIndex(1, elem);
  }

  size_type BucketIndex(size_type i, key_type const& elem) const noexcept {
#ifdef _DEBUG_HASH_TABLE_
    auto hash_value = hash_(elem);
    std::cout << "hash_value(" << elem << ") = " << hash_value << "\n";
    std::cout << "bucket_index = (" << (hash_value & tables_[i].size_mask) << " in table " << i << ")" <<  std::endl;
    return hash_value & tables_[i].size_mask;
#else
    return hash_(elem) & tables_[i].size_mask;
#endif
  }
  
  Bucket* GetExpectedBucket(key_type const& key);

  double LoadFactor() const noexcept {
    return tables_[0].used / tables_[0].size;
  }
  
  Table& table(size_type i) noexcept {
    return tables_[i];
  } 

  Table& table1() noexcept {
    return tables_[0];
  }

  Table& table2() noexcept {
    return tables_[1];
  }
  
  Table const& table1() const noexcept {
    return const_cast<HashTable*>(this)->table1();
  }

  Table const& table2() const noexcept {
    return const_cast<HashTable*>(this)->table2();
  }

  // Data member:
  Table tables_[2];
  size_type rehash_move_bucket_index_ = ~0;

  HashFunc hash_;
  EqualKey ek_;
  GetKey gk_;
};

} // namespace algo
} // namespace mmkv

#endif // _MMKV_ALGO_HASH_TABLE_H_
