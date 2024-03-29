// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_REPLACEMENT_LRU_CACHE_H__
#define _MMKV_REPLACEMENT_LRU_CACHE_H__

#include "mmkv/algo/blist.h"
#include "mmkv/algo/dictionary.h"
#include "mmkv/algo/libc_allocator_with_realloc_no_record.h"

#include "cache_interface.h"

namespace mmkv {
namespace replacement {

using algo::Blist;
using algo::Dictionary;

template <typename K>
class LruCache : public CacheInterface<K> {
  using Cache = Blist<K *, algo::LibcAllocatorWithReallocNoRecord<K *>>;
  using Dict  = Dictionary<
      K,
      typename Cache::Node *,
      algo::Hash<K>,
      algo::EqualKey<K>,
      algo::LibcAllocatorWithReallocNoRecord<K>>;

 public:
  explicit LruCache(size_t max_size) noexcept
    : CacheInterface<K>()
    , max_size_(max_size)
  {
  }

  ~LruCache() noexcept = default;

  // average: O(1)
  auto UpdateEntry(K const &entry) -> K * override { return UpdateEntry_(entry); }
  auto UpdateEntry(K &&entry) -> K * override { return UpdateEntry_(std::move(entry)); }

  template <typename U>
  auto UpdateEntry_(U &&entry) -> K *;

  // average: O(1)
  auto DelEntry(K const &key) -> bool override;

  // average: O(1)
  auto Search(K const &key) -> K * override;

  // O(1)
  auto Victim() -> K * override { return (cache_.empty()) ? nullptr : cache_.Back(); }

  // average: O(1)
  auto DelVictim() -> void override
  {
    if (cache_.empty()) return;
    dict_.Erase(*cache_.Back());
    cache_.PopBack();
  }

  auto size() const noexcept -> size_t override { return cache_.size(); }
  auto max_size() const noexcept -> size_t override { return max_size_; }
  auto Clear() -> void override;
  auto New() const -> LruCache * override { return new LruCache(max_size_); }
  auto entries() const noexcept -> Cache const & { return cache_; }

 private:
  size_t max_size_; /** The maxsize of cache */
  Cache  cache_;    /** To implement the LRU policy */
  Dict   dict_;     /** For searching */
};

} // namespace replacement
} // namespace mmkv

#include "internal/lru_cache_impl.h"

#endif
