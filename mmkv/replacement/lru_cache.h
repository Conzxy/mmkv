#ifndef _MMKV_REPLACEMENT_LRU_CACHE_H__
#define _MMKV_REPLACEMENT_LRU_CACHE_H__

#include "mmkv/algo/blist.h"
#include "mmkv/algo/dictionary.h"
#include "cache_interface.h"

namespace mmkv {
namespace replacement {

using algo::Blist;
using algo::Dictionary;

template<typename K>
class LruCache : public CacheInterface<K> {
  using Cache = Blist<K*>;
  using Dict = Dictionary<K, typename Cache::Node*>;

 public:
  explicit LruCache(size_t max_size) noexcept 
    : CacheInterface<K>()
    , max_size_(max_size) {
  }

  ~LruCache() noexcept = default;

  // average: O(1)
  auto UpdateEntry(K const &entry) -> K* override { return UpdateEntry_(entry); }
  auto UpdateEntry(K &&entry) -> K* override { return UpdateEntry_(std::move(entry)); }

  template<typename U>
  auto UpdateEntry_(U &&entry) -> K*;

  // average: O(1)
  auto DelEntry(K const &key) -> bool override;

  // average: O(1)
  auto Exists(K const &key) -> bool override;

  // O(1)  
  auto Victim() -> K* override {
    return (cache_.empty()) ? nullptr : cache_.Back();
  }

  // average: O(1) 
  auto DelVictim() -> void override {
    if (cache_.empty()) return;
    dict_.Erase(*cache_.Back());
    cache_.PopBack();
  }

  auto size() const noexcept -> size_t override { return cache_.size(); }
  auto max_size() const noexcept -> size_t override { return max_size_; }

  auto entries() const noexcept -> Cache const& { return cache_; }  

 private:
  size_t max_size_; /** The maxsize of cache */
  Cache cache_;     /** To implement the LRU policy */
  Dict dict_;       /** For searching */
};

} // replacement
} // mmkv

#include "internal/lru_cache_impl.h"

#endif
