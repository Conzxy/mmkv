// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef MMKV_REPLACEMENT_CACHE_INTERFACE_H_
#define MMKV_REPLACEMENT_CACHE_INTERFACE_H_

#include <stddef.h>

#include <kanon/util/noncopyable.h>

namespace mmkv {
namespace replacement {

template<typename K>
class CacheInterface {
  DISABLE_EVIL_COPYABLE(CacheInterface)
 public:

  CacheInterface() = default;
  ~CacheInterface() = default;

  /** 
   * \brief Update or insert a entry
   *
   * To existed entry, just move to front
   * To new entry, insert it
   * \return 
   *  existed or new Entry
   */
  virtual auto UpdateEntry(K const &entry) -> K* = 0;
  virtual auto UpdateEntry(K &&entry) -> K* = 0;

  /**
   * \brief Remove a entry from cache
   * \return
   *  indicates success or not
   */
  virtual auto DelEntry(K const &key) -> bool = 0;

  /**
   * \brief Check a entry if exists
   */
  auto Exists(K const &key) -> bool { return Search(key) != nullptr; }

  /**
   * \brief Search the key in the cache
   * \return 
   *  nullptr -- Not found
   */
   virtual auto Search(K const &key) -> K* = 0;

  /**
   * \brief Get the victim entry in the cache
   * \return 
   *  Victim entry
   *  nullptr -- No victim in cache
   */
  virtual auto Victim() -> K* = 0;

  /**
   * \brief Remove the victim
   */
  virtual auto DelVictim() -> void = 0;

  /** 
   * \brief Get current count of entries in the cache 
   * Should be O(1)
   */
  virtual auto size() const noexcept -> size_t = 0;

  /** 
   * \brief Get the maximum size of cache 
   * Should be O(1)
   */
  virtual auto max_size() const noexcept -> size_t = 0;
  
  /**
   * \brief Remove all entries in cache
   */
  virtual auto Clear() -> void = 0;
  
  /**
   * \brief Clone a object
   * \see Prototype pattern
   */
  virtual auto New() const -> CacheInterface* = 0;
};

} // replacement
} // mmkv

#endif
