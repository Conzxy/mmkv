#ifndef _MMKV_REPLACEMENT_LRU_CACHE_H__
#define _MMKV_REPLACEMENT_LRU_CACHE_H__

#include "mmkv/algo/blist.h"
#include "mmkv/algo/dictionary.h"

namespace mmkv {
namespace replacement {

using algo::Blist;
using algo::Dictionary;

template<typename K, typename H=algo::Hash<K>>
class LruCache {
  using Cache = Blist<K*>;
  using Dict = Dictionary<K, typename Cache::Node*, H>;

 public:
  explicit LruCache(size_t max_size) noexcept 
    : max_size_(max_size) {

  }

  LruCache() noexcept = default;

  /** 
   * \brief Update or insert a entry
   *
   * To existed entry, just move to front
   * To new entry, insert it
   * \return 
   *  existed or new Entry
   */
  template<typename U>
  K *UpdateEntry(U &&entry);

  /**
   * \brief Remove a entry from cache
   * \return
   *  indicates success or not
   */
  bool DelEntry(K const & key);

  /**
   * \brief Check a entry if exists
   */
  bool Exists(K const &key);
   
  K *Victim() noexcept {
    return (cache_.empty()) ? nullptr : cache_.Back();
  }
  
  void DelVictim() {
    if (cache_.empty()) return;
    dict_.Erase(*cache_.Back());
    cache_.PopBack();
  }

  size_t size() const noexcept { return cache_.size(); }
  size_t max_size() const noexcept { return max_size_; }

  Cache const &entries() const noexcept { return cache_; }  

 private:
  size_t max_size_; /** The maxsize of cache */
  Cache cache_;     /** To implement the LRU policy */
  Dict dict_;       /** For searching */
};

} // replacement
} // mmkv

#include "internal/lru_cache_impl.h"

#endif
