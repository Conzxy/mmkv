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
class Dictionary : public HashTable<K, KeyValue<K, V>, HF, GetKey<KeyValue<K, V>>, EK, Alloc> {
  using Base = HashTable<K, KeyValue<K, V>, HF, GetKey<KeyValue<K, V>>, EK, Alloc>;
 public:
  using mapped_type = V;
  using typename Base::value_type;
  using typename Base::key_type;

  Dictionary() = default;
  ~Dictionary() = default;
  
  template<typename U1, typename U2> 
  value_type* InsertKv(U1&& key, U2&& value) { return Base::Insert(value_type{ std::forward<U1>(key), std::forward<U2>(value) }); }

  template<typename U1, typename U2>
  bool InsertKvWithDuplicate(U1&& key, U2&& value, value_type*& duplicate) { return Base::InsertWithDuplicate(value_type{ std::forward<U1>(key), std::forward<U2>(value) }, duplicate); }

  /**
   * This is not an efficient method.
   * It is perfered to call InsertWithDuplicate().
   */
  value_type& operator[](key_type const& key) {
    auto kv = Base::Find(key);

    if (!kv) {
      kv = Base::Insert(value_type{key, mapped_type{}});
    }
    
    assert(kv);
    return kv->value;
  }
};

} // algo 
} // mmkv

#endif // _MMKV_DICTIONARY_H_
