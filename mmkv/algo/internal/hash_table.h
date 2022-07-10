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

#include "hash_table_iterator.h"

namespace mmkv {
namespace algo {

template<typename T, typename Alloc>
using BucketAllocator = typename Alloc::template rebind<Slist<T, Alloc>>::other;

#define HASH_FUNC (*((HF*)this))
#define EQUAL_KEY (*((EK*)this))
#define GET_KEY (*((GK*)this))

/**
 * \brief Fast look-up table
 *
 * 因为有rehash的需求，采用separate list法。
 * 相比一般的separate list而言：
 * 1） 查询之后将该数据项(data entry)放至开头，即所谓的MTF(move to front)手法。
 *     对于热点数据，被多次查询有一定的性能改善（特别是对于槽较多的桶而言）；
 *     对于冷数据，并不会带来大的性能损失。
 * 2） 由于该哈希表用于提供远程缓存服务，故采用递进式(incremental) rehash可以避免
 *     多个客户端长时间等待，减少响应时间
 *
 * 暂时不提供Shrink
 * TODO: 将Slist换成AvlTree（另起一个类）
 */
template<typename K, typename T, typename HF, typename GK, typename EK, typename Alloc>
class HashTable : protected BucketAllocator<T, Alloc>
                , protected NodeAlloctor<T, Alloc>
                , protected EK
                , protected HF
                , protected GK {
  using BucketAllocTraits = std::allocator_traits<BucketAllocator<T, Alloc>>;

  using Bucket = Slist<T, Alloc>;
  using Slot = typename Bucket::Node;
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
  using iterator = HashTableIterator<K, T, HF, GK, EK, Alloc>;
  using const_iterator = HashTableConstIterator<K, T, HF, GK, EK, Alloc>;
  using Node = typename Bucket::Node;

 private:
  using NodeAllocTraits = std::allocator_traits<NodeAlloctor<T, Alloc>>;
  friend class HashTableIterator<K, T, HF, GK, EK, Alloc>;
  friend class HashTableConstIterator<K, T, HF, GK, EK, Alloc>;
  
 public:
  HashTable();
  ~HashTable() noexcept;
  
  void Clone(HashTable const& table);
  // 不提供Emplace()
  // 因为如果有重复键new后还得delete 

  /**
   * \brief Insert entry(unique key)
   * \return
   *   inserted new entry -- success
   *   nullptr -- failure
   * \note
   *  If you interested in the existed entry with same key,
   *  should call InsertWithDuplicate() since this API just return
   *  nullptr when insert failed.
   */
  value_type* Insert(T const& elem) { return Insert_impl(elem); }
  value_type* Insert(T&& elem) { return Insert_impl(std::move(elem)); }

  /**
   * \brief Insert entry(unique key) and set \p duplicate when insert failed
   * \param duplicate inserted new entry(success), duplicated entry with same key(failure)
   * \return
   *   true -- success
   *   false -- failure
   */
  bool InsertWithDuplicate(T const& elem, value_type*& duplicate) { return InsertWithDuplicate_impl(elem, duplicate); }
  bool InsertWithDuplicate(T&& elem, value_type*& duplicate) { return InsertWithDuplicate_impl(std::move(elem), duplicate); }

  /**
   * \brief Search the entry with given \p key
   * \return
   *   nullptr -- no such entry
   *   satisfied entry
   */
  value_type* Find(K const& key);
  value_type const* Find(K const& key) const { return const_cast<HashTable*>(this)->Find(key); }

  /**
   * \brief Search the entry with given \p key
   * \return
   *   nullptr -- no such entry
   *   pointer to slot where satisfied entry in
   *   Because MTF policy, this must be the header of a list
   */
  Node** FindSlot(K const& key);

  /**
   * \brief Erase the slot returnd by FindSlot()
   * \warning
   *   Must be called after FindSlot() immediately,
   *   otherwise, the header maybe changed by other opertion,
   *   such as, IncrementalRehash()
   */
  void EraseAfterFindSlot(Slot*& slot);

  /**
   * \brief Erase the entry with given \p key
   * \return
   *  The number of erased entry
   *  i.e., 1 -- success, 0 -- failure
   */
  size_type Erase(K const& key);

  /**
   * \brief Extract the node with given \p key
   * \return
   *   nullptr -- no such entry
   *   existed node with given key -- success
   * \warning
   *   Must call DropNode() or FreeNode() 
   *   to reclaim returned node
   */
  Node* Extract(K const& key) noexcept;  

  size_type size() const noexcept { return table1().used; }
  size_type GetSize() const noexcept { return size(); }
  bool empty() const noexcept { return size() == 0; }
  size_type max_size() const noexcept { return (size_type)-1; }

  /**
   * \brief Get the load factor of the hash table
   * \note
   *   You can assert(load_factor < 1.0) to check invariant
   *   Indeed, this is useless method I don't use in any method
   */
  double load_factor() const noexcept { return ((double)tables_[0].used) / tables_[0].size; }
  
  iterator begin() noexcept { return iterator(this); }
  const_iterator begin() const noexcept { return const_iterator(this); }
  iterator end() noexcept { return iterator(this, 1, table2().size()); }
  const_iterator end() const noexcept { return const_iterator(this, 1, table2().size()); }
  const_iterator cbegin() const noexcept { return begin(); }
  const_iterator cend() const noexcept { return end(); }

  /**
   * \brief Reclaims the memory of the node
   * \warning
   *   If value_type is non-trivially-destructive type, should call DropNode
   *   instead of call this.
   *   Indeed, using enable_if_t and is_trivially_destructive can take static check here
   *   but I don't like use SFINAE traits in such way.
   */
  void FreeNode(Node* node) { NodeAllocTraits::deallocate(*this, node, 1); }

  /**
   * \brief Destory the object and reclaims the memory
   */
  void DropNode(Node* node) { NodeAllocTraits::destroy(*this, node); FreeNode(node); }

  // For debugging
  void DebugPrint();

 private:

  struct Table {
    ReservedArray<Bucket> table;
    size_t used;
    size_t size_mask;
      
    Table() 
      : table(0)
      , used(0)
      , size_mask(table.size()-1)
    {
    }

    ~Table() noexcept = default;
    
    void swap(Table& other) noexcept {
      // 不交换used
      // table2的used实际没有用
      std::swap(other.table, table);
      std::swap(other.size_mask, size_mask);
    }
    Bucket& operator[](size_type i) noexcept { return table[i]; } 
    Bucket const& operator[](size_type i) const noexcept { return table[i]; }
    size_type size() const noexcept { return table.size(); }
    bool empty() const noexcept { return table.empty(); }
    void Grow(size_type expected_size) { table.Grow(expected_size); size_mask = size() - 1; }
    void Reset() { table.Shrink(0); }
  };

  void Rehash();
  void IncremetalRehash(); 

  bool InRehashing() const noexcept {
    // If rehash_move_bucket_index == -1, indicates the
    // progress of rehash is completed.
    return rehash_move_bucket_index_ != (size_type)-1;
  }
  
  bool CheckDuplicate(Bucket* bucket, value_type const& elem) {
    return GetDuplicate(bucket, elem) != bucket->end();
  }

  typename Bucket::iterator GetDuplicate(Bucket* bucket, value_type const& elem) {
    return bucket->SearchIf([this, &elem](value_type const& val) {
      return EQUAL_KEY(GET_KEY(elem), GET_KEY(val));
    });
  }

  template<typename U>
  value_type* Insert_impl(U&& elem);
  template<typename U>
  bool InsertWithDuplicate_impl(U&& elem, value_type*& duplicate);

  /*
   * 如果要获取两个table的bucket index，
   * 需要计算两次哈希值
   * 这既不划算也不合理
   * 因此传递哈希值是更合理的
   */

  // deprecated
  size_type BucketIndex1(key_type const& elem) const noexcept { return BucketIndex(0, elem); }
  // deprecated
  size_type BucketIndex2(key_type const& elem) const noexcept { return BucketIndex(1, elem); }
  // deprecated
  size_type BucketIndex(size_type i, key_type const& elem) const noexcept {
#ifdef _DEBUG_HASH_TABLE_
    auto hash_value = HASH_FUNC(elem);
    std::cout << "hash_value(" << elem << ") = " << hash_value << "\n";
    std::cout << "bucket_index = (" << (hash_value & tables_[i].size_mask) << " in table " << i << ")" <<  std::endl;
    return hash_value & tables_[i].size_mask;
#else
    return HASH_FUNC(elem) & tables_[i].size_mask;
#endif
  }

  size_type bucket_index(size_t table_index, uint64_t hash_val) const noexcept { return hash_val & tables_[table_index].size_mask; }

  Table& table(size_type i) noexcept { return tables_[i]; } 
  Table const& table(size_type i) const noexcept { return tables_[i]; } 
  Table& table1() noexcept { return tables_[0]; }
  Table const& table1() const noexcept { return const_cast<HashTable*>(this)->table1(); }
  Table& table2() noexcept { return tables_[1]; }
  Table const& table2() const noexcept { return const_cast<HashTable*>(this)->table2(); }

  // Data member:
  Table tables_[2];
  static_assert(sizeof(tables_) == 64, "");
  size_type rehash_move_bucket_index_ = ~0;

  // HashFunc hash_;
  // EqualKey ek_;
  // GetKey gk_;
};

} // namespace algo
} // namespace mmkv

#endif // _MMKV_ALGO_HASH_TABLE_H_
