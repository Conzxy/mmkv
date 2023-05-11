// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_DB_KVDB_H_
#define _MMKV_DB_KVDB_H_

#include "mmkv/algo/avl_dictionary.h"
#include "mmkv/algo/comparator_util.h"

#include "mmkv/protocol/status_code.h"
#include "mmkv/protocol/type.h"
#include "mmkv/protocol/shard_code.h"
#include "mmkv/replacement/cache_interface.h"
#include "mmkv/tracker/common_type.h"

#include "mmkv_data.h"
#include "type.h"

#include <kanon/util/noncopyable.h>
#include <kanon/thread/rw_lock.h>

namespace mmkv {
namespace db {

using algo::AvlDictionary;
using algo::HashSet;
// using algo::Dictionary;
using algo::Comparator;
using protocol::OrderRange;
using protocol::Shard;
using protocol::ShardCode;
using protocol::StatusCode;
using protocol::StrKvs;
using protocol::StrValues;
using protocol::WeightRange;
using protocol::WeightValues;
using replacement::CacheInterface;

#define DB_MIN(x, y) (((x) < (y)) ? (x) : (y))

/**
 * \brief Database instance of mmkv
 *
 * Encapsulate the operation of different data structure
 * that including string, list, etc.
 *
 * \note
 *  Internal class
 *  Non-copyable
 */
class MmkvDb {
  DISABLE_EVIL_COPYABLE(MmkvDb)

  /****** Data members *******/
  using Dict   = AvlDictionary<String, MmkvData, Comparator<String>>;
  using ExDict = AvlDictionary<String, uint64_t, Comparator<String>>;

  using ShardIdSet = HashSet<String const *>;
  using ShardDict  = AvlDictionary<shard_id_t, ShardIdSet, Comparator<shard_id_t>>;

  std::string name_; /* For log */

  /* Store all key-value records. */
  Dict dict_;

  /* Store the expiration time of key.
   * ExDict的键类型不是String*（在dict_中的拷贝）而是String
   * （存储键本身）。
   * 如果采用String*作为键类型，那么每次都得先获取在dict_的key地址
   * 对于一些操作是这是没有必要的。
   * 尽管采用String作为键需要耗费更多空间
   * FIXME
   * 采用String*作为键实现 */
  ExDict exp_dict_; /** expire dictionary */

  std::unique_ptr<CacheInterface<String const *>> cache_;

  /* Record the shard => keys
   * 保存的是在dict_中key的拷贝 */
  ShardDict sdict_;

  /**
   * Add node:
   * To new node that added to cluster, if the pulled shard data
   * is incomplete, it is necessary to lock the shard.
   *
   * Leave node:
   * To old node, the pushed shard must be locked.
   */
  HashSet<shard_id_t> locked_shard_id_set_;

 public:
  explicit MmkvDb(std::string name);

  MmkvDb()
    : MmkvDb("MMKV")
  {
  }

  ~MmkvDb() noexcept;

  // MmkvDb(MmkvDb &&) = default;
  // MmkvDb &operator=(MmkvDb &&) = default;

  void SetName(std::string &&name) { name_ = std::move(name); }

  /*----------------------------------------------*/
  /* Common API                                   */
  /*----------------------------------------------*/

  /**
   * \brief Determine if the database is empty
   */
  bool IsEmpty() const noexcept { return dict_.empty(); }

  /**
   * \brief Get all keys in the database
   * \param[out] keys Store the all keys
   */
  void GetAllKeys(StrValues &keys) const;

  /**
   * \brief Remove entry from the database
   * \return
   *  S_SHARD_LOCKED
   *  S_NONEXISTS
   *  S_OK
   */
  StatusCode Delete(String const &k);

  /**
   * \brief Remove all entries
   * \return
   *  S_OK
   *  S_SHARD_LOCKED
   */
  StatusCode DeleteAll(size_t *del_cnt);

  /**
   * \brief Get the type of key
   * \return
   *  true -- exists
   */
  bool Type(String const &key, DataType &type) noexcept;

  /**
   * \brief Rename the existed key to new name
   * \return
   *  S_EXISTS -- new_name exists
   *  S_NONEXISTS -- old_name doesn't exists
   *  S_OK
   *  S_SHARD_LOCKED
   */
  StatusCode Rename(String const &old_name, String &&new_name);

  /*----------------------------------------------*/
  /* String API                                   */
  /*----------------------------------------------*/

  /**
   * \brief Insert new string
   * \return
   *  S_EXISTS -- key exists
   *  S_OK
   */
  StatusCode InsertStr(String &&k, String &&v);

  /**
   * You can call Delete() to remove string entry
   * but don't check the type
   * \return
   *  S_OK
   *  S_EXISTS_DIFF_TYPE -- key exists but not string type
   *  S_NONEXISTS -- key doesn't exists
   */
  StatusCode EraseStr(String const &k);
  /**
   * \param str Store the address of string
   * \return
   *  S_OK
   *  S_EXISTS_DIFF_TYPE -- key exists but not string type
   *  S_NONEXISTS -- key doesn't exists
   */
  StatusCode GetStr(String const &k, String *&str) noexcept;

  /**
   * If k doesn't exists, will insert it to dictionary
   * \return
   *  S_OK
   *  S_EXISTS_DIFF_TYPE -- key exists but not string type
   */
  StatusCode SetStr(String &&k, String &&v);

  /**
   * \brief Append the \p str to the value of \p key
   * \return
   *  Same with GetStr()
   */
  StatusCode StrAppend(String const &key, String const &str);

  /**
   * \brief Pop the back the \p str to the value of \p key
   * \return
   *  Same with GetStr()
   */
  StatusCode StrPopBack(String const &key, size_t count);

  /*----------------------------------------------*/
  /* List API                                     */
  /*----------------------------------------------*/

  /**
   * \brief Create a list object and initialize its elements
   * \param elems Initial elements
   * \return
   *  S_OK
   *  S_EXISTS -- key exists
   */
  StatusCode ListAdd(String &&k, StrValues &elems);

  /**
   * \brief Insert the elements to the tail of list
   * \return
   *  S_OK
   *  S_EXISTS_DIFF_TYPE
   *  S_NONEXISTS
   */
  StatusCode ListAppend(String &&k, StrValues &elems);

  /**
   * \brief Insert the elements to the head of list
   * \return
   *  S_OK
   *  S_EXISTS_DIFF_TYPE
   *  S_NONEXISTS
   */
  StatusCode ListPrepend(String const &k, StrValues &elems);

  /**
   * \brief Get the size of list
   * \return
   *  S_OK
   *  S_EXISTS_DIFF_TYPE
   *  S_NONEXISTS
   */
  StatusCode ListGetSize(String const &k, size_t &size);

  /**
   * \brief Get all elements of list
   * \return
   *  S_OK
   *  S_EXISTS_DIFF_TYPE
   *  S_NONEXISTS
   */
  StatusCode ListGetAll(String const &k, StrValues &values);

  /**
   * \brief Get elements in a range
   * The range is left close and right open, and left starts with 0,
   * i.e. [left, right).
   * If left and right is negative, it is also valid:
   * To the right, indicates next to the r th last as the right bound
   * To the left, same with positive.
   * \param left left bound of range
   * \param right right bound of range(can't reach)
   * \return
   *  S_OK
   *  S_EXISTS_DIFF_TYPE
   *  S_NONEXISTS
   *  S_INVALID_RANGE left > right and l >= size of list
   */
  StatusCode ListGetRange(String const &k, StrValues &values, int64_t l, int64_t r);

  /**
   * \brief Remove the elements in the tail of list
   * \return
   *  S_OK
   *  S_EXISTS_DIFF_TYPE
   *  S_NONEXISTS
   */
  StatusCode ListPopFront(String const &k, uint32_t count);

  /**
   * \brief Remove the elements in the head of list
   * \return
   *  S_OK
   *  S_EXISTS_DIFF_TYPE
   *  S_NONEXISTS
   */
  StatusCode ListPopBack(String const &k, uint32_t count);

  /**
   * \return
   *  S_OK
   *  S_EXISTS_DIFF_TYPE
   *  S_NONEXISTS
   */
  StatusCode ListDel(String const &k);

  /*----------------------------------------------*/
  /* Vset API                                     */
  /*----------------------------------------------*/

  /**
   * \brief Insert members into vset
   * If
   * \param[out] count record the count of inserted members
   * \return
   *  S_OK
   *  S_EXISTS_DIFF_TYPE
   */
  StatusCode VsetAdd(String &&key, WeightValues &&wms, size_t &count);

  /**
   * \brief Remove the member from vset
   * \return
   *  S_OK
   *  S_NONEXISTS
   *  S_EXISTS_DIFF_TYPE
   *  S_VMEMBER_NONEXISTS -- The member does not exists in the vset
   */
  StatusCode VsetDel(String const &key, String const &member);

  /**
   * \brief Remove members in the order range from vset
   * The order range is not like the list range
   * The range is closed interval, i.e., [left, right].
   * Also can be negative, indicates reversed order
   * \param[out] count record the count of removed elements
   * \return
   *  S_OK
   *  S_NONEXISTS
   *  S_EXISTS_DIFF_TYPE
   */
  StatusCode VsetDelRange(String const &key, OrderRange range, size_t &count);

  /**
   * The order range is closed interval but can't be negative
   * \param[out] count
   * \return
   *  S_OK
   *  S_NONEXISTS
   *  S_EXISTS_DIFF_TYPE
   */
  StatusCode VsetDelRangeByWeight(String const &key, WeightRange range, size_t &count);

  /**
   * \brief Get the count of members in vset
   * \param[out] count
   * \return
   *  S_OK
   *  S_NONEXISTS
   *  S_EXISTS_DIFF_TYPE
   */
  StatusCode VsetSize(String const &key, size_t &count);

  /**
   * \brief Get the count of members between range in vset
   * \param[out]
   * \return
   *  S_OK
   *  S_NONEXISTS
   *  S_EXISTS_DIFF_TYPE
   */
  StatusCode VsetSizeByWeight(String const &key, WeightRange range, size_t &count);

  /**
   * \brief Get the weight of member
   * \param[out] w
   * \return
   *  S_OK
   *  S_NONEXISTS
   *  S_EXISTS_DIFF_TYPE
   *  S_VMEMBER_NONEXISTS
   */
  StatusCode VsetWeight(String const &key, String const &member, Weight &w);

  /**
   * \brief Get the order of member
   * \param[out] order
   * \return
   *  S_OK
   *  S_NONEXISTS
   *  S_EXISTS_DIFF_TYPE
   *  S_VMEMBER_NONEXISTS
   */
  StatusCode VsetOrder(String const &key, String const &member, size_t &order);

  /**
   * \brief Get the reversed order of member
   * \param[out] order
   * \return
   *  S_OK
   *  S_NONEXISTS
   *  S_EXISTS_DIFF_TYPE
   *  S_VMEMBER_NONEXISTS
   */
  StatusCode VsetROrder(String const &key, String const &member, size_t &order);

  /**
   * \brief Get All members in the vset
   * \param[out] wms
   * \return
   *  S_OK
   *  S_NONEXISTS
   *  S_EXISTS_DIFF_TYPE
   */
  StatusCode VsetAll(String const &key, WeightValues &wms);

  /**
   * \brief Get the members in the order range
   * \param[out] wms
   * \return
   *  S_OK
   *  S_NONEXISTS
   *  S_EXISTS_DIFF_TYPE
   */
  StatusCode VsetRange(String const &key, OrderRange range, WeightValues &wms);

  /**
   * \brief Get the members in the weight range
   * \param[out] wms
   * \return
   *  S_OK
   *  S_NONEXISTS
   *  S_EXISTS_DIFF_TYPE
   */
  StatusCode VsetRangeByWeight(String const &key, WeightRange range, WeightValues &wms);

  /**
   * \brief Get the members in the reversed order range
   * \param[out] wms
   * \return
   *  S_OK
   *  S_NONEXISTS
   *  S_EXISTS_DIFF_TYPE
   */
  StatusCode VsetRRange(String const &key, OrderRange range, WeightValues &wms);

  /**
   * \brief Get the members in the reversed weight range
   * \param[out] wms
   * \return
   *  S_OK
   *  S_NONEXISTS
   *  S_EXISTS_DIFF_TYPE
   */
  StatusCode VsetRRangeByWeight(String const &key, WeightRange range, WeightValues &wms);

  /*----------------------------------------------*/
  /* Map API                                      */
  /*----------------------------------------------*/

  /**
   * \brief Create a map object
   * \param[out] count record the count of inserted kvs
   * \return
   *  S_OK
   *  S_EXISTS_DIFF_TYPE
   */
  StatusCode MapAdd(String &&key, StrKvs &&kvs, size_t &count);

  /**
   * \brief Set value of field in the map
   * \return
   *  S_OK
   *  S_EXISTS_DIFF_TYPE
   *  S_NONEXISTS
   */
  StatusCode MapSet(String const &key, String &&field, String &&value);

  /**
   * \param[out] value
   * \return
   *  S_OK
   *  S_EXISTS_DIFF_TYPE
   *  S_FIELD_NONEXISTS
   */
  StatusCode MapGet(String const &key, String const &field, String &value);

  /**
   * \brief Get multiple fields
   * \return
   *  Same with MapGet()
   */
  StatusCode MapGets(String const &key, StrValues const &fields, StrValues &values);

  /**
   * \brief Remove field from map
   * \return
   *  S_OK
   *  S_NONEXISTS
   *  S_EXISTS_DIFF_TYPE
   *  S_FIELD_NONEXISTS
   */
  StatusCode MapDel(String const &key, String const &field);

  /**
   * \brief Get all kvs in the map
   * \param[out] kvs
   * \return
   *  S_OK
   *  S_NONEXISTS
   *  S_EXISTS_DIFF_TYPE
   */
  StatusCode MapAll(String const &key, StrKvs &kvs);

  /**
   * \brief Get all fields in the map
   * \param[out] fields
   * \return
   *  Same with MapAll()
   */
  StatusCode MapFields(String const &key, StrValues &fields);

  /**
   * \brief Get all values in the map
   * \param[out] values
   * \return
   *  Same with MapAll()
   */
  StatusCode MapValues(String const &key, StrValues &values);

  /**
   * \brief Get the count of kvs in the map
   * \param[out] count
   * \return
   *  Same with MapAll()
   */
  StatusCode MapSize(String const &key, size_t &count);

  /**
   * \brief Query if the field does exists
   * \return
   *  S_OK -- exists
   *  S_FIELD_EXISTS
   *  S_NONEXISTS
   *  S_EXISTS_DIFF_TYPE
   */
  StatusCode MapExists(String const &key, String const &field);

  /*----------------------------------------------*/
  /* Set API                                      */
  /*----------------------------------------------*/

  /**
   * \brief Insert members into the set
   * If set does not exists, create it first
   * \param[out] count
   * \return
   *  S_OK
   *  S_EXISTS_DIFF_TYPE
   */
  StatusCode SetAdd(String &&key, StrValues &members, size_t &count);

  /**
   * \brief Remove the member from set
   * \return
   *  S_OK
   *  S_NONEXISTS
   *  S_EXISTS_DIFF_TYPE
   *  S_SET_MEMBER_NONEXISTS
   */
  StatusCode SetDelm(String const &key, String const &member);

  /**
   * \brief Remove a member from set randomly
   * In fact, remove the first entry in the set
   * since hash set is unordered.
   * \return
   *  Same with SetDelm()
   */
  StatusCode SetRandDelm(String const &key);

  /**
   * \brief Query if the member does exists
   * \return
   *  Same with SetDelm()
   */
  StatusCode SetExists(String const &key, String const &member);

  /**
   * \brief Get the size of set
   * \param[out] count
   * \return
   *  S_OK
   *  S_EXISTS_DIFF_TYPE
   *  S_NONEXISTS
   */
  StatusCode SetSize(String const &key, size_t &count);

  /**
   * \brief Get all the members in the set
   * \param[out] members
   * \return
   *  Same with SetSize()
   */
  StatusCode SetAll(String const &key, StrValues &members);

  /*----------------------------------------------*/
  /* Set operator API                             */
  /*----------------------------------------------*/

  /**
   * \brief Get the intersection set between key1 and key2(key1 & key2)
   * \param[out] members
   * \return
   *  S_OK
   *  S_NONEXISTS
   *  S_EXISTS_DIFF_TYPE
   */
  StatusCode SetAnd(String const &key1, String const &key2, StrValues &members);

  /**
   * \brief Get the union set between key1 and key2(key1 | key2)
   * \param[out] members
   * \return
   *  Same with SetAnd()
   */
  StatusCode SetOr(String const &key, String const &key2, StrValues &members);

  /**
   * \brief Get the difference set between key1 and key2(key1 - key2)
   * \param members
   * \return
   *  Same with SetSub()
   */
  StatusCode SetSub(String const &key1, String const &key2, StrValues &members);

  /**
   * \brief Like SetAnd() but store the result to destination set
   * \return
   *  S_OK
   *  S_EXISTS
   *  S_EXISTS_DIFF_TYPE
   *  S_DEST_EXISTS
   */
  StatusCode SetAndTo(String const &key1, String const &key2, String &&dest);

  /**
   * \return
   *  Same with SetAndTo()
   */
  StatusCode SetOrTo(String const &key1, String const &key2, String &&dest);

  /**
   * \return
   *  Same with SetAndTo()
   */
  StatusCode SetSubTo(String const &key1, String const &key2, String &&dest);

  /**
   * \brief Get the size of the intersection set
   * \return
   *  Same with SetAnd();
   */
  StatusCode SetAndSize(String const &key1, String const &key2, size_t &count);

  /**
   * \brief Get the size of the union set
   * \return
   *  Same with SetAnd();
   */
  StatusCode SetOrSize(String const &key1, String const &key2, size_t &count);

  /**
   * \brief Get the size of the difference set
   * \return
   *  Same with SetAnd();
   */
  StatusCode SetSubSize(String const &key1, String const &key2, size_t &count);

  /*----------------------------------------------*/
  /* Expiration management API                    */
  /*----------------------------------------------*/

  /**
   * \brief Set expiration time in milliseconds
   * \return
   *  S_OK -- success
   *  S_NONEXISTS -- key does not exists
   *  S_EXPIRE_DISABLE -- expiration is disable, can't set expiration time
   */
  StatusCode ExpireAtMs(String &&key, uint64_t expire);

  /**
   * \brief Like ExpireAtMs() but unit is second
   * \return
   *  Same with ExpireAtMs()
   */
  StatusCode ExpireAt(String &&key, uint64_t expire)
  {
    return ExpireAtMs(std::move(key), expire * 1000);
  }

  /**
   * \brief Set expiration interval after current time
   * \return
   *  Same with ExpireAtMs()
   */
  StatusCode ExpireAfterMs(String &&key, uint64_t ms, uint64_t interval)
  {
    return ExpireAtMs(std::move(key), ms + interval);
  }

  /**
   * \brief Like ExpireAfterMs() but unit is second
   * \return
   *  Same with ExpireAtMs()
   */
  StatusCode ExpireAfter(String &&key, uint64_t ms, uint64_t interval)
  {
    return ExpireAtMs(std::move(key), ms + interval * 1000);
  }

  /**
   * \brief Get the expiration time of \p key
   * \return
   *  S_OK
   *  S_NONEXISTS -- key is not set expiration time
   */
  StatusCode GetExpiration(String const &key, uint64_t &exp);

  /**
   * \brief Get the TTL of \p key
   * \return
   *  S_OK
   *  S_NONEXISTS -- key is not set expiration time
   */
  StatusCode GetTimeToLive(String const &key, uint64_t &ttl);

  /**
   * \brief Make key persist in the database
   * If the key has expiration, remove it.
   * Otherwise, do nothing.
   * \return
   *  S_OK
   */
  StatusCode Persist(String const &key);

  /**
   * \brief Check all keys in the database(FIXME)
   * The API must be called in a fixed cycle.
   * The cycle is set in the config file
   */
  void CheckExpireCycle();

  /*--------------------------------------------------*/
  /* Shard Management                                 */
  /*--------------------------------------------------*/

  ShardDict::iterator       ShardBegin() noexcept { return sdict_.begin(); }
  ShardDict::iterator       ShardEnd() noexcept { return sdict_.end(); }
  ShardDict::const_iterator ShardBegin() const noexcept { return sdict_.begin(); }
  ShardDict::const_iterator ShardEnd() const noexcept { return sdict_.end(); }

  void DistributeKeysToShard();

  void UnlockAllShard();

  /**
   * \brief
   *
   * \param shard_id
   */
  void LockShard(shard_id_t shard_id);

  /**
   * \brief
   *
   * \param shard_id
   *
   * \return
   */
  bool IsShardLocked(shard_id_t shard_id) const noexcept;

  /**
   * \brief
   *
   * \param shard_id
   */
  void UnlockShard(shard_id_t shard_id);

  void AddShard(shard_id_t shard_id);

  /**
   * Remove the shard from database
   * \param shard_id  id of shard
   */
  void RemoveShard(shard_id_t shard_id);

  bool HasShard(shard_id_t shard);

  /**
   * Get all keys in the mapped shard
   */
  ShardCode GetShardKeys(shard_id_t shard_id, std::vector<String const *> &keys);

  String GetShardInfo();

  bool is_ignore_locked_shard = false;

 private:
  /*----------------------------------------------*/
  /* Replacement API                              */
  /*----------------------------------------------*/

  /**
   * \brief If condition is satisfied, replace victim key
   * \param key The address of the key in the database
   */
  void TryReplacekey(String const *key);

  /**
   * \brief Add a key to the cache
   */
  void CacheAdd(String const *key);

  /** \brief Remove the key from cache */
  void CacheRemove(String const *key);

  /**
   * \brief Update the key from cache
   * update used to indicate it is accessed recently
   */
  void CacheUpdate(String const *key);

  /**
   * \brief Check if the key has expired
   * \return
   *  true -- Key has expired, indicates also remove key from dict_
   *  false -- Key is not expired, or lazy expiration check is disable
   */
  bool CheckExpire(String const &key);

  /*----------------------------------------------*/
  /* Shard management API                         */
  /*----------------------------------------------*/

  MMKV_INLINE bool HasShardLocked() const noexcept;

  /**
   * Insert the key to the mapped shard.
   * \param key Exists in the database
   */
  MMKV_INLINE void AddKeyToShard(String const *key);

  /**
   * Remove the key record from the mapped shard
   * \param key Exists in the database
   */
  MMKV_INLINE void RemoveKeyFromShard(String const *key);
  MMKV_INLINE void DeleteAllShard();
};

} // namespace db
} // namespace mmkv

#endif // _MMKV_DB_KVDB_H_
