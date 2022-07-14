#ifndef _MMKV_ALGO_INTERNAL_TREE_HASH_TABLE_H_
#define _MMKV_ALGO_INTERNAL_TREE_HASH_TABLE_H_

#ifdef _DEBUG_TREE_HASH_TABLE_
#include <iostream>
#include <stdio.h>
#endif

#include "mmkv/algo/reserved_array.h"
#include "tree_hash_table_iterator.h"

namespace mmkv {
namespace algo {

#ifdef HASH_FUNC
#undef HASH_FUNC
#endif
#define HASH_FUNC (*(hash_function*)(this))

#ifdef GET_KEY
#undef GET_KEY
#endif
#define GET_KEY (*(get_key*)(this))

/**
 * \brief Hash table implemented by separate-list method but the list is avltree instead of linked-list
 *
 *              Insert   Search   Delete     move list in rehashing
 * linked-list   O(1)     O(n)     O(n)           O(n)
 * avltree       O(lgn)   O(lgn)   O(lgn)         O(lgn!) ~ O(nlgn)
 */
template<typename K, typename V, typename HF, typename GK, typename Tree, typename Alloc>
class TreeHashTable : protected Alloc::template rebind<typename Tree::Node>::other
                    , protected HF
                    , protected GK {

  friend class TreeHashTableIterator<K, V, HF, GK, Tree, Alloc>;
  friend class TreeHashTableConstIterator<K, V, HF, GK, Tree, Alloc>;
 public:
  using key_type = K;
  using value_type = V;
  using reference = V&;
  using const_reference = V const&;
  using pointer = V*;
  using const_pointer = V const*;
  using size_type = size_t;
  using hash_function = HF;
  using get_key = GK;
  using allocator_type = Alloc;
  using iterator = TreeHashTableIterator<K, V, HF, GK, Tree, Alloc>;
  using const_iterator = TreeHashTableConstIterator<K, V, HF, GK, Tree, Alloc>;
  using Bucket = Tree;
  using Slot = typename Bucket::Node;
  using Node = typename Bucket::Node;
 private:
  using NodeAllocTraits = std::allocator_traits<typename Alloc::template rebind<Slot>::other>;

 public:

  TreeHashTable();
  ~TreeHashTable() noexcept = default;

  /************************************************************/
  /* Insert interface                                         */
  /************************************************************/

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
  value_type* Insert(value_type const& elem) { return Insert_impl(elem); }
  value_type* Insert(value_type&& elem) { return Insert_impl(std::move(elem)); }

  /**
   * \brief Insert entry(unique key) and set \p duplicate when insert failed
   * \param duplicate inserted new entry(success), duplicated entry with same key(failure)
   * \return
   *   true -- success
   *   false -- failure
   */
  bool InsertWithDuplicate(value_type const& elem, value_type*& duplicate) { return InsertWithDuplicate_impl(elem, duplicate); }
  bool InsertWithDuplicate(value_type&& elem, value_type*& duplicate) { return InsertWithDuplicate_impl(std::move(elem), duplicate); }


  /************************************************************/
  /* Search interface                                         */
  /************************************************************/

  /**
   * \brief Search the entry with given \p key
   * \return
   *   nullptr -- no such entry
   *   satisfied entry
   */
  value_type* Find(K const& key);
  value_type const* Find(K const& key) const { return const_cast<TreeHashTable*>(this)->Find(key); }

  /**
   * \brief Search the entry with given \p key
   * \param[out] bucket set bucket where key in if it isn't null pointer
   * \return 
   *   nullptr -- no such entry
   *   satisfied node
   * \note 
   *   If you want do something to the bucket, for example, do something then delete it,
   *   get the bucket can decrease calculate the hash value again
   */
  Node* FindNode(K const& key, Bucket** bucket);
  Node const* FindNode(K const& key) const { return const_cast<TreeHashTable*>(this)->FindNode(key, nullptr); }


  /************************************************************/
  /* Delete interface                                         */
  /************************************************************/

  /**
   * \brief Extract the node with given \p key
   * \return
   *   nullptr -- no such entry
   *   existed node with given key -- success
   * \warning
   *   Must call DropNode() or FreeNode() 
   *   to reclaim returned node
   */
  Node* Extract(K const& key);  

  /**
   * \brief Erase the entry with given \p key
   * \return
   *  The number of erased entry
   *  i.e., 1 -- success, 0 -- failure
   */
  size_type Erase(K const& key);

  /**
   * \brief Erase the node in the bucket
   * \return
   *   The unmber of erased entry
   *   In fact, if node is not null pointer and is valid node returned by another interface
   *   it must return 1 instead of 0(former case) or undefined(latter case)
   */
  size_type EraseNode(Bucket* bucket, Node* node);

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

  /************************************************************/
  /* Getter interface                                         */
  /************************************************************/

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
  void IncrementalRehash(); 

  bool InRehashing() const noexcept {
    // If rehash_move_bucket_index == -1, indicates the
    // progress of rehash is completed.
    return rehash_move_bucket_index_ != (size_type)-1;
  }
  
  template<typename U>
  value_type* Insert_impl(U&& elem);
  template<typename U>
  bool InsertWithDuplicate_impl(U&& elem, value_type*& duplicate);

  size_type bucket_index(size_t table_index, uint64_t hash_val) const noexcept { return hash_val & tables_[table_index].size_mask; }

  Table& table(size_type i) noexcept { return tables_[i]; } 
  Table const& table(size_type i) const noexcept { return tables_[i]; } 
  Table& table1() noexcept { return tables_[0]; }
  Table const& table1() const noexcept { return const_cast<TreeHashTable*>(this)->table1(); }
  Table& table2() noexcept { return tables_[1]; }
  Table const& table2() const noexcept { return const_cast<TreeHashTable*>(this)->table2(); }

  // Data member:
  Table tables_[2];
  static_assert(sizeof(tables_) == 64, "");
  size_type rehash_move_bucket_index_ = ~0;
};

} // algo
} // mmkv

#endif // _MMKV_ALGO_INTERNAL_TREE_HASH_TABLE_H_