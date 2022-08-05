#ifndef _MMKV_REPLACEMENT_INTERNAL_LRU_CACHE_IMPL_H__
#define _MMKV_REPLACEMENT_INTERNAL_LRU_CACHE_IMPL_H__

#ifndef _MMKV_REPLACEMENT_LRU_CACHE_H__
#include "../lru_cache.h"
#endif

#include "mmkv/algo/string.h"

#define LRU_CACHE_TEMPLATE template<typename K>
#define LRU_CACHE_CLASS LruCache<K>

namespace mmkv {
namespace replacement {

LRU_CACHE_TEMPLATE
template<typename U>
K *LRU_CACHE_CLASS::UpdateEntry_(U &&entry) {
  if (max_size_ == 0) return nullptr;

  typename Dict::value_type *duplicate = nullptr;
  auto const success = dict_.InsertKvWithDuplicate(std::forward<U>(entry), nullptr, duplicate);
  // Exists
  // just update and move to front
  if (!success) {
    auto node = duplicate->value;
    cache_.Extract(node);
    cache_.PushFront(node);
  } else {
    // set the node of new inserted entry
    cache_.PushFront(&duplicate->key);
    duplicate->value = cache_.FrontNode();

    // If size over the max_size,
    // Pop the least used entry
    if (cache_.size() > max_size_) {
      // cache_ store the pointer to key instead of key
      // since key store in the dict_
      dict_.Erase(*cache_.Back());
      cache_.PopBack();
    }
    assert(cache_.size() <= max_size_);

  }

  return cache_.Front();
}

LRU_CACHE_TEMPLATE
K *LRU_CACHE_CLASS::Search(K const &key) {
  auto kv = dict_.Find(key);
  return kv ? &kv->key : nullptr;
}

LRU_CACHE_TEMPLATE
bool LRU_CACHE_CLASS::DelEntry(K const &key) {
  auto entry = dict_.Extract(key);
  if (entry) {
    cache_.Erase(entry->value.value);
    return true;
  }

  return false;
}

} // replacement
} // mmkv

#endif
