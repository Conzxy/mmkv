#ifndef _MMKV_ALGO_HASH_SET_H_
#define _MMKV_ALGO_HASH_SET_H_

#include <initializer_list>

#include "mmkv/algo/hash_util.h"
#include "mmkv/algo/hash_table.h"
#include "mmkv/algo/libc_allocator_with_realloc.h"

namespace mmkv {
namespace algo {

template<typename K, typename HF=Hash<K>, typename Alloc=LibcAllocatorWithRealloc<K>>
class HashSet : public HashTable<K, K, HF, GetKey<K>, EqualKey<K>, Alloc> {
  using Base = HashTable<K, K, HF, GetKey<K>, EqualKey<K>, Alloc>;
 public:
  HashSet() = default;

  template<typename E>
  HashSet(std::initializer_list<E> il) {
    for (auto const& e : il) {
      this->Insert(e);
    }
  }

  ~HashSet() = default;
  
  template<typename ValueCb>
  void Union(HashSet const& hs, ValueCb cb);

  template<typename ValueCb>
  void Difference(HashSet const& hs, ValueCb cb);

  template<typename ValueCb>
  void Intersection(HashSet const& hs, ValueCb cb);

  int EraseRandom();

  using Base::rehash_move_bucket_index_;
  using Base::IncrementalRehash;
  using Base::InRehashing;
  using Base::table;
};

} // algo
} // mmkv

#include "internal/hash_set_impl.h"

#endif
