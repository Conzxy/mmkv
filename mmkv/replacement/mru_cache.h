// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef MMKV_REPLACEMENT_MRU_CACHE_H_
#define MMKV_REPLACEMENT_MRU_CACHE_H_

#include <utility>

#include "cache_interface.h"

#include "mmkv/algo/dictionary.h"
#include "mmkv/algo/blist.h"
#include "mmkv/algo/libc_allocator_with_realloc_no_record.h"

namespace mmkv {
namespace replacement {
 
template<typename K> 
class MruCache : public CacheInterface<K> {
  using List = algo::Blist<K*, algo::LibcAllocatorWithReallocNoRecord<K*>>;
  using Dict = algo::Dictionary<K, typename List::Node*, algo::Hash<K>, algo::EqualKey<K>, algo::LibcAllocatorWithReallocNoRecord<K>>;
 public:
  explicit MruCache(size_t max_size)
    : max_size_(max_size)
  {
  }
  
  auto UpdateEntry(K const &entry) -> K* override { return UpdateEntry_(entry); }
  auto UpdateEntry(K &&entry) -> K* override { return UpdateEntry_(std::move(entry)); }
  
  template<typename U> 
  auto UpdateEntry_(U &&entry) -> K*;

  auto DelEntry(K const &key) -> bool override;
  auto Search(K const &key) -> K* override;
  auto Victim() -> K* override { return lst_.Front(); }
  auto DelVictim() -> void override;
  auto size() const noexcept -> size_t override { return lst_.size(); }
  auto max_size() const noexcept -> size_t override { return max_size_; }
  auto Clear() -> void override;
  auto New() const -> MruCache* override { return new MruCache(max_size_); }
 private:
  size_t max_size_;
  Dict dict_;
  List lst_;
};

} // replacement
} // mmkv

#include "internal/mru_cache_impl.h"

#endif // MMKV_REPLACEMENT_MRU_CACHE_H_
