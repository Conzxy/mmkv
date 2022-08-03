#ifndef _MMKV_ALGO_INTERNAL_HASH_SET_IMPL_H_
#define _MMKV_ALGO_INTERNAL_HASH_SET_IMPL_H_

#ifndef _MMKV_ALGO_HASH_SET_H_
#include "../hash_set.h"
#endif

namespace mmkv {
namespace algo {

#define HASH_SET_TEMPLATE template<typename K, typename HF, typename A>
#define HASH_SET_CLASS HashSet<K, HF, A>
#define CALLBACK_TEMPLATE template<typename Cb>

HASH_SET_TEMPLATE
CALLBACK_TEMPLATE
inline void HASH_SET_CLASS::Union(HashSet const& hs, Cb cb) {
  HashSet const* less_set = hs.size() > this->size() ? this : &hs;
  HashSet const* more_set = hs.size() > this->size() ? &hs : this;

  for (auto const& m : *more_set) {
    cb(m);
  }

  for (auto const& m : *less_set) {
    if (!more_set->Find(m)) {
      cb(m);
    }
  }
}

HASH_SET_TEMPLATE
CALLBACK_TEMPLATE
inline void HASH_SET_CLASS::Intersection(HashSet const& hs, Cb cb) {
  HashSet const* less_set = hs.size() > this->size() ? this : &hs;
  HashSet const* more_set = hs.size() > this->size() ? &hs : this; 

  for (auto const& m : *less_set) {
    if (more_set->Find(m)) {
      cb(m);
    }
  }
}

HASH_SET_TEMPLATE
CALLBACK_TEMPLATE
inline void HASH_SET_CLASS::Difference(HashSet const& hs, Cb cb) {
  for (auto const& m : *this) {
    if (!hs.Find(m)) {
      cb(m);
    }
  }
}

HASH_SET_TEMPLATE
inline int HASH_SET_CLASS::EraseRandom() {
  IncrementalRehash();
  
  int table_num = InRehashing() ? 2 : 1;
  size_t j = (table_num > 1) ? rehash_move_bucket_index_ : 0;
  for (int i = 0; i < table_num; ++i) {
    for (; j < table(i).size(); ++j) {
      if (!table(i)[j].empty()) {
        table(i)[j].PopFront();
        return 1;
      }
    }
    j = 0;
  }

  return 0;
}

} // algo
} // mmkv

#endif // _MMKV_ALGO_INTERNAL_HASH_SET_IMPL_H_H_
