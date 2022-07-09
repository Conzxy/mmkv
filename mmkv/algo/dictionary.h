#ifndef _MMKV_DICTIONARY_H_
#define _MMKV_DICTIONARY_H_

#include "hash_table.h"
#include "hash_util.h"
#include "key_value.h"
#include "libc_allocator_with_realloc.h"

namespace mmkv {
namespace algo {

/**
 * \brief kv hashtable
 *
 * This a wrapper based on HashTable<>
 * support operator[]
 */
template<typename K, typename V, typename HF=Hash<K>, typename EK=EqualKey<K>, typename Alloc=LibcAllocatorWithRealloc<KeyValue<K, V>>>
class Dictionary {
  using Rep = HashTable<K, KeyValue<K, V>, HF, GetKey<KeyValue<K, V>>, EK, LibcAllocatorWithRealloc<KeyValue<K, V>>>;
 public:
  using key_type = typename Rep::key_type;
  using value_type = typename Rep::value_type;
  using mapped_type = V;
  using reference = typename Rep::reference;
  using const_reference = typename Rep::const_reference;
  using pointer = typename Rep::pointer;
  using const_pointer = typename Rep::const_pointer;
  using iterator = typename Rep::iterator;
  using const_iterator = typename Rep::const_iterator;
  using hash_function = typename Rep::hash_function;
  using equal_key = typename Rep::equal_key;
  using allocator_type = typename Rep::allocator_type;
  using size_type = typename Rep::size_type;
  using Node = typename Rep::Node;
  using Slot = Node;

  Dictionary() = default;
  ~Dictionary() = default;
  
  value_type* Insert(value_type const& elem) { return rep_.Insert(elem); }
  value_type* Insert(value_type&& elem) { return rep_.Insert(std::move(elem)); }
  template<typename U1, typename U2> 
  value_type* InsertKv(U1&& key, U2&& value) { return rep_.Insert(value_type{ std::forward<U1>(key), std::forward<U2>(value) }); }
  bool InsertWithDuplicate(value_type const& elem, value_type*& dup) { return rep_.InsertWithDuplicate(elem, dup); }
  bool InsertWithDuplicate(value_type&& elem, value_type*& dup) { return rep_.InsertWithDuplicate(std::move(elem), dup); }
  template<typename U1, typename U2>
  bool InsertKvWithDuplicate(U1&& key, U2&& value, value_type*& duplicate) { return rep_.InsertWithDuplicate(value_type{ std::forward<U1>(key), std::forward<U2>(value) }, duplicate); }

  value_type* Find(key_type const& key) { return rep_.Find(key); }
  value_type const* Find(key_type const& key) const { return rep_.Find(key); }
  Slot** FindSlot(key_type const& key) { return rep_.FindSlot(key); }  

  size_type Erase(K const& key) { return rep_.Erase(key); }
  Node* Extract(K const& key) noexcept { return rep_.Extract(key); }

  void EraseAfterFindSlot(Slot*& slot) { return rep_.EraseAfterFindSlot(slot); }

  void FreeNode(Node* node) { rep_.FreeNode(node); }
  void DropNode(Node* node) { rep_.DropNode(node); }

  size_type size() const noexcept { return rep_.size(); }
  bool empty() const noexcept { return rep_.empty(); }
  
  value_type& operator[](key_type const& key) {
    auto slot = Find(key);

    if (!slot) {
      slot = rep_.Insert(value_type{key, mapped_type{}});
    }
    
    assert(slot);
    return slot->value;
  }
  
  iterator begin() noexcept { return rep_.begin(); } 
  const_iterator begin() const noexcept { return rep_.begin(); } 
  iterator end() noexcept { return rep_.end(); } 
  const_iterator end() const noexcept { return rep_.end(); } 
  const_iterator cbegin() const noexcept { return rep_.cbegin(); } 
  const_iterator cend() const noexcept { return rep_.cend(); } 
 private:
  Rep rep_;
};

} // algo 
} // mmkv

#endif // _MMKV_DICTIONARY_H_
