#ifndef _MMKV_ALGO_INTERNAL_HASH_SET_IMPL_H_
#define _MMKV_ALGO_INTERNAL_HASH_SET_IMPL_H_

#include "hash_set.h"
#include "mmkv/algo/internal/hash_table_impl.h"

namespace mmkv {
namespace algo {

#define HASH_SET_TEMPLATE template<typename K, typename HF, typename A>
#define HASH_SET_CLASS HashSet<K, HF, A>
#define CALLBACK_TEMPLATE template<typename Cb>

HASH_SET_TEMPLATE
CALLBACK_TEMPLATE
void HASH_SET_CLASS::Union(HashSet const& hs, Cb cb) {
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
void HASH_SET_CLASS::Intersection(HashSet const& hs, Cb cb) {
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
void HASH_SET_CLASS::Difference(HashSet const& hs, Cb cb) {
  for (auto const& m : *this) {
    if (!hs.Find(m)) {
      cb(m);
    }
  }
}
} // algo
} // mmkv

#endif // _MMKV_ALGO_INTERNAL_HASH_SET_IMPL_H_H_
