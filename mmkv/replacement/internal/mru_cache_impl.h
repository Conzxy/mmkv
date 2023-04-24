// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef MMKV_REPLACEMENT_INTERNAL_MRU_CACHE_IMPL_H_
#define MMKV_REPLACEMENT_INTERNAL_MRU_CACHE_IMPL_H_

#ifndef MMKV_REPLACEMENT_MRU_CACHE_H_
#include "../mru_cache.h"
#endif


#define MRU_CACHE_TEMPLATE template<typename K>
#define MRU_CACHE_CLASS MruCache<K>

namespace mmkv {
namespace replacement {

MRU_CACHE_TEMPLATE
template<typename U>
auto MRU_CACHE_CLASS::UpdateEntry_(U &&entry) -> K* {
  if (max_size_ == 0) return nullptr;


  typename Dict::value_type *duplicate = nullptr;
  auto success = dict_.InsertKvWithDuplicate(std::forward<U>(entry), nullptr, duplicate);
  if (success) {
    if (size() >= max_size_) {
      DelVictim();
    }

    lst_.PushFront(&duplicate->key);
    duplicate->value = lst_.FrontNode();
  } else {
    auto node = duplicate->value;
    lst_.Extract(node);
    lst_.PushFront(node);
  }

  return lst_.Front();
}

MRU_CACHE_TEMPLATE
auto MRU_CACHE_CLASS::DelEntry(K const &key) -> bool {
  auto entry = dict_.Extract(key);
  if (entry) {
    lst_.Erase(entry->value.value);
    dict_.DropNode(entry);
    return true;
  }

  return false;
}

MRU_CACHE_TEMPLATE
auto MRU_CACHE_CLASS::Search(K const &key) -> K* {
  auto ret = dict_.Find(key);
  return ret ? &ret->key : nullptr;
}

MRU_CACHE_TEMPLATE
auto MRU_CACHE_CLASS::DelVictim() -> void {
  auto victim = lst_.Front();
  dict_.Erase(*victim);
  lst_.PopFront();
}

MRU_CACHE_TEMPLATE
auto MRU_CACHE_CLASS::Clear() -> void {
  dict_.Clear();
  lst_.Clear();
}

} // replacement 
} // mmkv

#endif
