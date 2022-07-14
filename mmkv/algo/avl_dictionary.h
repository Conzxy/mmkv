#ifndef _MMKV_ALGO_AVL_DICTIONARY_H
#define _MMKV_ALGO_AVL_DICTIONARY_H

#include "avl_tree_hashtable.h"

namespace mmkv {
namespace algo {


template<typename K, typename V, typename Comparator, typename HF=Hash<K>, typename GK=GetKey<KeyValue<K, V>>, typename Alloc=LibcAllocatorWithRealloc<KeyValue<K, V>>>
class AvlDictionary : public AvlTreeHashMap<K, V, Comparator, HF, GK, Alloc> {
  using Base = AvlTreeHashMap<K, V, Comparator, HF, GK, Alloc>;
 public:
  using mapped_type = V;
  using typename Base::value_type;
  using typename Base::key_type;

  AvlDictionary() = default;
  ~AvlDictionary() = default;
  
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

#endif // _MMKV_ALGO_AVL_DICTIONARY_H