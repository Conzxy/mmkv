// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_STORAGE_DB_H_
#define _MMKV_STORAGE_DB_H_

#include "mmkv/db/kvdb.h"

#include "mmkv/protocol/mmbp_request.h"
#include "mmkv/protocol/mmbp_response.h"
#include "mmkv/algo/string.h"

#include <xxhash.h>
#include <kanon/thread/rw_lock.h>

namespace mmkv {
namespace storage {

using db::MmkvDb;
using protocol::MmbpRequest;
using protocol::MmbpResponse;
using kanon::RWLock;
using algo::String;

struct DatabaseInstance {
  MmkvDb db;
  RWLock lock{};

  explicit DatabaseInstance(std::string const &name)
    : db(name)
  {
  }
};

/**
 * Manage the multiple database instances
 * 
 * If mmkv configured to distributed,
 * the only one database used.
 * Otherwise, if configured to multithread, 
 * the closest number of Power2(1.5 * thread_num),
 * else only one database used.
 *
 * \note
 *  I think this can be noncopyable 
 */
class DatabaseManager : kanon::noncopyable {
 public:
  DatabaseManager();
  ~DatabaseManager() noexcept;
 
  void Execute(MmbpRequest &request, MmbpResponse *response);

  /**
   * Check the expiration actively
   * in round-robin method.
   */
  void CheckExpirationCycle();
  
  void SetRecvTime(uint64_t tm) noexcept { recv_time_ = tm; }
  uint64_t recv_time() const noexcept { return recv_time_; }

  DatabaseInstance &GetDatabaseInstance(String const &key) noexcept {
    return *instances_[GetDatabaseInstanceIndex(key)];
  }

  DatabaseInstance const &GetDatabaseInstance(String const &key) const noexcept {
    return const_cast<DatabaseManager*>(this)->GetDatabaseInstance(key);
  }

 private:
  size_t GetDatabaseInstanceIndex(String const &key);

  enum Type : uint8_t {
    LOCAL,
    LOCAL_MULTI_THREAD,
    DISTRIBUTED,
  };
  
  Type type_;
  std::vector<std::unique_ptr<DatabaseInstance>> instances_;
  uint64_t recv_time_;
  uint64_t current_index_; /** Round-robin index */
};

/* Declare pointer to avoid 
 * undefined initailzation sequence
 */
// extern DatabaseManager *g_database_manager;;

DatabaseManager &database_manager();

// void DbExpireAfter(MmbpRequest &request, uint64_t ms, MmbpResponse *response);


} // server
} // mmkv

#endif 
