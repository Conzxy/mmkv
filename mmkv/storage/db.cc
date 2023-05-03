// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "db.h"

#include "mmkv/protocol/command.h"
#include "mmkv/protocol/command_type.h" // GetCommandType
#include "mmkv/protocol/status_code.h"
#include "mmkv/server/config.h" // mmkv_config()
#include "mmkv/util/macro.h"    // MMKV_ASSert
#include "mmkv/util/memory_footprint.h"
#include "mmkv/util/time_util.h"

namespace storage = mmkv::storage;
using namespace mmkv::storage;
using namespace mmkv::protocol;
using namespace kanon;

// DatabaseManager *storage::g_database_manager = nullptr;

DatabaseManager &storage::database_manager()
{
  static DatabaseManager manager;
  return manager;
}

#define SET_OK_VALUE(_value)                                                                       \
  response->SetOk();                                                                               \
  response->SetValue();                                                                            \
  response->value = (_value)

#define SET_OK_VALUE_                                                                              \
  response->SetOk();                                                                               \
  response->SetValue()

#define SET_OK_VALUES(_values)                                                                     \
  response->SetOk();                                                                               \
  response->SetValues();                                                                           \
  response->values = (_values)

#define SET_OK_VALUES_                                                                             \
  response->SetOk();                                                                               \
  response->SetValues()

#define SET_OK_VMEMBERS(_wms)                                                                      \
  response->SetOk();                                                                               \
  response->SetVmembers();                                                                         \
  response->vmembers = (_wms)

#define SET_OK_VMEMBERS_                                                                           \
  response->SetOk();                                                                               \
  response->SetVmembers()

#define SET_OK_KVS(_kvs)                                                                           \
  response->SetOk();                                                                               \
  response->SetKvs();                                                                              \
  response->kvs = (_kvs);

#define SET_OK_KVS_                                                                                \
  response->SetOk();                                                                               \
  response->SetKvs()

#define SET_OK_COUNT(_count)                                                                       \
  response->SetOk();                                                                               \
  response->SetCount();                                                                            \
  response->count = (_count)

#define SET_OK_COUNT_                                                                              \
  response->SetOk();                                                                               \
  response->SetCount();

#define SET_XX_ELSE_CODE(xx)                                                                       \
  if (code == S_OK) {                                                                              \
    xx;                                                                                            \
  } else {                                                                                         \
    response->status_code = code;                                                                  \
  }

#define CHECK_INVALID_REQUEST(cond, msg)                                                           \
  do {                                                                                             \
    if (response && !(cond)) {                                                                     \
      response->status_code = S_INVALID_REQUEST;                                                   \
      response->value       = msg;                                                                 \
      response->SetValue();                                                                        \
      return;                                                                                      \
    }                                                                                              \
  } while (0)

static inline size_t RoundUpTo2Power(size_t src) noexcept
{
  size_t ret = 1;
  for (int i = 0; i < 31; ++i) {
    if (ret < src)
      ret <<= 1;
    else
      break;
  }
  return ret;
}

static inline bool Is2Power(size_t src)
{
  size_t single_one = 1;
  for (int i = 0; i < 31; ++i) {
    if ((src ^ single_one) == 0) return true;
    single_one <<= 1;
  }
  return false;
}

void DatabaseInstance::Execute(MmbpRequest &request, MmbpResponse *response, uint64_t recv_time)
{
  switch (request.command) {
    case STR_ADD: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "stradd");
      auto code = db.InsertStr(std::move(request.key), std::move(request.value));
      if (response) {
        response->status_code = code;
      }
    } break;
    case STR_GET: {
      CHECK_INVALID_REQUEST(request.HasKey(), "strget");
      String *str  = nullptr;
      auto    code = db.GetStr(request.key, str);
      SET_XX_ELSE_CODE(SET_OK_VALUE(*str));
    } break;
    case STR_DEL: {
      CHECK_INVALID_REQUEST(request.HasKey(), "strdel");
      auto code = db.EraseStr(request.key);
      if (response) response->status_code = code;
    } break;

    case STR_SET: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "strset");
      auto code = db.SetStr(std::move(request.key), std::move(request.value));
      if (response) response->status_code = code;
    } break;

    case STRLEN: {
      CHECK_INVALID_REQUEST(request.HasKey(), "strlen");
      String *str  = nullptr;
      auto    code = db.GetStr(request.key, str);
      SET_XX_ELSE_CODE(SET_OK_COUNT(str->size()));
    } break;

    case STRAPPEND: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "strappend");
      const auto code = db.StrAppend(request.key, request.value);
      if (response) response->status_code = code;
    } break;

    case STRPOPBACK: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasCount(), "strpopback");
      const auto code = db.StrPopBack(request.key, request.count);
      if (response) response->status_code = code;
    } break;

    case LADD: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasValues(), "ladd");
      auto code = db.ListAdd(std::move(request.key), request.values);
      if (response) response->status_code = code;
    } break;

    case LAPPEND:
    case LPREPEND: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasValues(), "lappend/lprepend");

      StatusCode code;
      switch (request.command) {
        case LAPPEND:
          code = db.ListAppend(request.key, request.values);
          break;
        case LPREPEND:
          code = db.ListPrepend(request.key, request.values);
          break;
      }

      if (response) response->status_code = code;
    } break;

    case LPOPFRONT:
    case LPOPBACK: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasCount(), "lpopback/popfront");

      StatusCode code;
      switch (request.command) {
        case LPOPFRONT:
          code = db.ListPopFront(request.key, request.count);
          break;
        case LPOPBACK:
          code = db.ListPopBack(request.key, request.count);
          break;
      }

      if (response) {
        SET_XX_ELSE_CODE(SET_OK_COUNT_)
      }
    } break;

    case LGETSIZE: {
      CHECK_INVALID_REQUEST(request.HasKey(), "lgetsize");
      size_t count = 0;
      if ((response->status_code = db.ListGetSize(request.key, count)) == S_OK) {
        response->SetCount();
        response->count = count;
      }
    } break;

    case LGETRANGE: {
      CHECK_INVALID_REQUEST(request.HasRange(), "lgetrange");

      if ((response->status_code = db.ListGetRange(
               request.key,
               response->values,
               request.range.left,
               request.range.right
           )) == S_OK)
      {
        response->SetValues();
      }
    } break;

    case LGETALL: {
      CHECK_INVALID_REQUEST(request.HasKey(), "lgetrange/getall");

      if ((response->status_code = db.ListGetAll(request.key, response->values)) == S_OK) {
        response->SetValues();
      }
    } break;

    case LDEL: {
      CHECK_INVALID_REQUEST(request.HasKey(), "ldel");
      auto code = db.ListDel(request.key);
      if (response) response->status_code = code;
    } break;

    case VADD: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasVmembers(), "vadd");
      size_t count = 0;
      auto   code  = db.VsetAdd(std::move(request.key), std::move(request.vmembers), count);
      if (response) {
        SET_XX_ELSE_CODE(SET_OK_COUNT(count))
      }
    } break;

    case VALL: {
      CHECK_INVALID_REQUEST(request.HasKey(), "vall");
      WeightValues values;
      if ((response->status_code = db.VsetAll(request.key, response->vmembers)) == S_OK) {
        response->SetVmembers();
      }
    } break;

    case VDELM: {
      CHECK_INVALID_REQUEST(request.HasKey(), "vdelm");
      auto code = db.VsetDel(request.key, request.value);
      if (response) response->status_code = code;
    } break;

    case VDELMRANGE: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasRange(), "vdelmrange");
      size_t count = 0;
      auto   code  = db.VsetDelRange(request.key, request.range, count);
      if (response) {
        SET_XX_ELSE_CODE(SET_OK_COUNT(count))
      }
    } break;

    case VDELMRANGEBYWEIGHT: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasRange(), "vdelmrangebyweight");
      size_t count = 0;
      auto   code  = db.VsetDelRangeByWeight(request.key, request.GetWeightRange(), count);
      if (response) {
        SET_XX_ELSE_CODE(SET_OK_COUNT(count))
      }
    } break;

    case VSIZE: {
      CHECK_INVALID_REQUEST(request.HasKey(), "vsize");
      size_t size;
      auto   code = db.VsetSize(request.key, size);
      SET_XX_ELSE_CODE(SET_OK_COUNT(size))
    } break;

    case VSIZEBYWEIGHT: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasRange(), "vsizebyweight");
      size_t size;
      auto   code = db.VsetSizeByWeight(request.key, request.GetWeightRange(), size);
      SET_XX_ELSE_CODE(SET_OK_COUNT(size))
    } break;

    case VWEIGHT: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "vweight");
      Weight weight;
      auto   code = db.VsetWeight(request.key, request.value, weight);
      SET_XX_ELSE_CODE(SET_OK_COUNT(util::double2u64(weight)))
    } break;

    case VORDER: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "vorder");
      size_t order = 0;
      auto   code  = db.VsetOrder(request.key, request.value, order);
      SET_XX_ELSE_CODE(SET_OK_COUNT(order))
    } break;

    case VRORDER: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "vrorder");
      size_t order = 0;
      auto   code  = db.VsetROrder(request.key, request.value, order);
      SET_XX_ELSE_CODE(SET_OK_COUNT(order))
    } break;

    case VRANGE: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasRange(), "vrange");
      auto code = db.VsetRange(request.key, request.range, response->vmembers);

      SET_XX_ELSE_CODE(SET_OK_VMEMBERS_)
    } break;

    case VRRANGE: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasRange(), "vrrange");
      auto code = db.VsetRRange(request.key, request.range, response->vmembers);

      SET_XX_ELSE_CODE(SET_OK_VMEMBERS_)
    } break;

    case VRANGEBYWEIGHT: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasRange(), "vrangebyweight");
      auto code = db.VsetRangeByWeight(request.key, request.GetWeightRange(), response->vmembers);

      SET_XX_ELSE_CODE(SET_OK_VMEMBERS_)
    } break;

    case VRRANGEBYWEIGHT: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasRange(), "vrrangebyweight");
      auto code = db.VsetRRangeByWeight(request.key, request.GetWeightRange(), response->vmembers);

      SET_XX_ELSE_CODE(SET_OK_VMEMBERS_)
    } break;

    case MADD: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasKvs(), "madd");
      size_t count = 0;
      auto   code  = db.MapAdd(std::move(request.key), std::move(request.kvs), count);

      if (response) {
        SET_XX_ELSE_CODE(SET_OK_COUNT(count))
      }
    } break;

    case MGET: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "mget");
      auto code = db.MapGet(request.key, request.value, response->value);
      SET_XX_ELSE_CODE(SET_OK_VALUE_);
    } break;

    case MGETS: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasValues(), "mgets");
      auto code = db.MapGets(request.key, request.values, response->values);
      SET_XX_ELSE_CODE(SET_OK_VALUES_);
    }

    case MSET: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasValues(), "mset");
      auto code =
          db.MapSet(request.key, std::move(request.values[0]), std::move(request.values[1]));
      if (response) response->status_code = code;
    } break;

    case MDEL: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "mdel");
      auto code = db.MapDel(request.key, request.value);
      if (response) response->status_code = code;
    } break;

    case MALL: {
      CHECK_INVALID_REQUEST(request.HasKey(), "mall");
      auto code = db.MapAll(request.key, response->kvs);

      SET_XX_ELSE_CODE(SET_OK_KVS_);
    } break;

    case MFIELDS: {
      CHECK_INVALID_REQUEST(request.HasKey(), "mfields");
      auto code = db.MapFields(request.key, response->values);
      SET_XX_ELSE_CODE(SET_OK_VALUES_);
    } break;

    case MVALUES: {
      CHECK_INVALID_REQUEST(request.HasKey(), "mvalues");
      auto code = db.MapValues(request.key, response->values);
      SET_XX_ELSE_CODE(SET_OK_VALUES_);
    } break;

    case MSIZE: {
      CHECK_INVALID_REQUEST(request.HasKey(), "msize");
      size_t count = 0;
      auto   code  = db.MapSize(request.key, count);
      SET_XX_ELSE_CODE(SET_OK_COUNT(count));
    } break;

    case MEXISTS: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "mexists");
      response->status_code = db.MapExists(request.key, request.value);
    } break;

    case SADD: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasValues(), "sadd");
      size_t count = 0;
      auto   code  = db.SetAdd(std::move(request.key), request.values, count);
      if (response) {
        SET_XX_ELSE_CODE(SET_OK_COUNT(count))
      }
    } break;

    case SDELM: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "sdelm");
      auto code = db.SetDelm(request.key, request.value);
      if (response) response->status_code = code;
    } break;

    case SEXISTS: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "sexists");
      response->status_code = db.SetExists(request.key, request.value);
    } break;

    case SSIZE: {
      CHECK_INVALID_REQUEST(request.HasKey(), "ssize");
      size_t count          = 0;
      response->status_code = db.SetSize(request.key, count);
      if (response->status_code == S_OK) {
        response->count = count;
        response->SetCount();
      }
    } break;

    case SALL: {
      CHECK_INVALID_REQUEST(request.HasKey(), "sall");
      response->status_code = db.SetAll(request.key, response->values);
      if (response->status_code == S_OK) response->SetValues();
    } break;

    case SAND: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "sand");
      response->status_code = db.SetAnd(request.key, request.value, response->values);
      if (response->status_code == S_OK) response->SetValues();
    } break;

    case SOR: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "sor");
      response->status_code = db.SetOr(request.key, request.value, response->values);
      if (response->status_code == S_OK) response->SetValues();
    } break;

    case SSUB: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "ssub");
      response->status_code = db.SetSub(request.key, request.value, response->values);
      if (response->status_code == S_OK) response->SetValues();
    } break;

    case SANDTO: {
      CHECK_INVALID_REQUEST(request.HasValues(), "sandto");
      auto code = db.SetAndTo(request.values[1], request.values[2], std::move(request.values[0]));
      if (response) response->status_code = code;
    } break;

    case SORTO: {
      CHECK_INVALID_REQUEST(request.HasValues(), "sorto");
      auto code = db.SetOrTo(request.values[1], request.values[2], std::move(request.values[0]));
      if (response) response->status_code = code;
    } break;

    case SSUBTO: {
      CHECK_INVALID_REQUEST(request.HasValues(), "ssubto");
      auto code = db.SetSubTo(request.values[1], request.values[2], std::move(request.values[0]));
      if (response) response->status_code = code;
    } break;

    case SANDSIZE: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "sandsize");
      size_t count = 0;
      auto   code  = db.SetAndSize(request.key, request.value, count);
      SET_XX_ELSE_CODE(SET_OK_COUNT(count));
    } break;

    case SORSIZE: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "sorsize");
      size_t count = 0;
      auto   code  = db.SetOrSize(request.key, request.value, count);
      SET_XX_ELSE_CODE(SET_OK_COUNT(count));
    } break;

    case SSUBSIZE: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "ssubsize");
      size_t count = 0;
      auto   code  = db.SetSubSize(request.key, request.value, count);
      SET_XX_ELSE_CODE(SET_OK_COUNT(count));
    } break;
    case SRANDDELM: {
      CHECK_INVALID_REQUEST(request.HasKey(), "sranddelm");
      const auto code = db.SetRandDelm(request.key);
      if (response) response->status_code = code;
    } break;
    case EXPIRE_AT: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasExpireTime(), "expireat");
      const auto code = db.ExpireAt(std::move(request.key), request.expire_time);
      if (response) response->status_code = code;
    } break;

    case EXPIRE_AFTER: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasExpireTime(), "expireafter");
      const auto code = db.ExpireAfter(std::move(request.key), recv_time, request.expire_time);
      if (response) response->status_code = code;
    } break;

    case EXPIREM_AT: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasExpireTime(), "expiremat");
      const auto code = db.ExpireAtMs(std::move(request.key), request.expire_time);
      if (response) response->status_code = code;

    } break;

    case EXPIREM_AFTER: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasExpireTime(), "expiremafter");
      const auto code = db.ExpireAfterMs(std::move(request.key), recv_time, request.expire_time);
      if (response) response->status_code = code;
    } break;

    case PERSIST: {
      CHECK_INVALID_REQUEST(request.HasKey(), "persist");
      const auto code = db.Persist(request.key);
      if (response) response->status_code = code;
    } break;

    case EXPIRATION: {
      CHECK_INVALID_REQUEST(request.HasKey(), "expiration");
      const auto code = db.GetExpiration(request.key, response->count);
      response->SetCount();
      response->status_code = code;
    } break;

    case TTL: {
      CHECK_INVALID_REQUEST(request.HasKey(), "TTL");
      const auto code = db.GetTimeToLive(request.key, response->count);
      response->SetCount();
      response->status_code = code;
    } break;

    case TYPE: {
      CHECK_INVALID_REQUEST(request.HasKey(), "type");
      db::DataType type;
      if (db.Type(request.key, type)) {
        SET_OK_VALUE(GetDataTypeString(type));
      } else {
        response->status_code = S_NONEXISTS;
      }
    } break;

    case DEL: {
      CHECK_INVALID_REQUEST(request.HasKey(), "del");
      auto success = db.Delete(request.key);
      if (response) response->status_code = success ? S_OK : S_NONEXISTS;
    } break;

    case RENAME: {
      CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "rename");
      auto code = db.Rename(request.key, std::move(request.value));
      if (response) response->status_code = code;
    } break;
  }
}

DatabaseManager::DatabaseManager()
  : type_(
        server::mmkv_config().SupportDistribution()
            ? DatabaseManager::DISTRIBUTED
            : (server::mmkv_config().thread_num == 1 ? DatabaseManager::LOCAL
                                                     : DatabaseManager::LOCAL_MULTI_THREAD)
    )
  , current_index_(0)
{
  size_t db_num = 0;
  switch (type_) {
    case LOCAL:
    case DISTRIBUTED:
      db_num = 1;
      break;
    case LOCAL_MULTI_THREAD:
      db_num = RoundUpTo2Power(server::mmkv_config().thread_num);
      break;
    default:
      MMKV_ASSERT(false, "Invalid type of database manager");
  }
  MMKV_ASSERT(Is2Power(db_num), "The size of instances must be power of 2");

  char db_name[128];
  instances_.reserve(db_num);
  for (size_t i = 0; i < db_num; ++i) {
    snprintf(db_name, sizeof db_name, "Database %zu", i);
    instances_.emplace_back(new DatabaseInstance(db_name));
  }

  LOG_INFO << "DatabaseManager created";
}

DatabaseManager::~DatabaseManager() noexcept { LOG_INFO << "DatabaseManager removed"; }

void DatabaseManager::CheckExpirationCycle()
{
  auto &instance = instances_[current_index_];
  auto &db       = instance.db;

  {
    WLockGuard g(instance.lock);

    if (!db.IsEmpty()) {
      db.CheckExpireCycle();
    }
  }

  // Used in the main thread
  // Don't lock is ok
  if (++current_index_ == instances_.size()) current_index_ = 0;
}

#define DB instance->db
#define RLOCK_ALL                                                                                  \
  for (auto &instance : instances_) {                                                              \
    instance.lock.RLock();                                                                         \
  }

#define RUNLOCK_ALL                                                                                \
  for (auto &instance : instances_) {                                                              \
    instance.lock.RUnlock();                                                                       \
  }

#define WLOCK_ALL                                                                                  \
  for (auto &instance : instances_) {                                                              \
    instance.lock.WLock();                                                                         \
  }

#define WUNLOCK_ALL                                                                                \
  for (auto &instance : instances_) {                                                              \
    instance.lock.WUnlock();                                                                       \
  }

void DatabaseManager::Execute(MmbpRequest &request, MmbpResponse *response)
{
  DatabaseInstance *instance     = nullptr;
  auto              command_type = GetCommandType((Command)request.command);
  if (request.HasKey()) {
    instance = &GetDatabaseInstance(request.key);

    if (command_type == CommandType::CT_READ) {
      instance->lock.RLock();
    } else if (command_type == CommandType::CT_WRITE) {
      instance->lock.WLock();
    } else {
      MMKV_ASSERT(false, "Invalid command type");
    }
  }

  switch (request.command) {
    case MEM_STAT: {
      CHECK_INVALID_REQUEST(request.HasNone(), "memorystat");
      SET_OK_VALUE(util::GetMemoryStat());
    } break;

    case KEYALL: {
      RLOCK_ALL
      for (auto const &db_instance : instances_) {
        db_instance.db.GetAllKeys(response->values);
      }
      response->SetValues();
      response->SetOk();
      RUNLOCK_ALL
    } break;

    case DELS: {
      auto  &keys  = request.values;
      size_t count = 0;
      for (auto const &key : keys) {
        count += GetDatabaseInstance(key).db.Delete(key) ? 1 : 0;
      }

      if (response) {
        response->status_code = S_OK;
        response->count       = count;
        response->SetCount();
      }
    } break;

    case DELALL: {
      WLOCK_ALL
      size_t count = 0;
      for (auto &db_instance : instances_) {
        size_t db_cnt = 0;
        auto   code   = db_instance.db.DeleteAll(&db_cnt);
        if (code == S_SHARD_LOCKED) break;
        count += db_cnt;
      }
      if (response) {
        response->status_code = S_OK;
        response->count       = count;
        response->SetCount();
      }
      WUNLOCK_ALL

    } break;

    default:
      instance->Execute(request, response, recv_time_);
      break;
  }

  if (instance) {
    if (command_type == CommandType::CT_READ) {
      instance->lock.RUnlock();
    } else if (command_type == CommandType::CT_WRITE) {
      instance->lock.WUnlock();
    }
  }
}

size_t DatabaseManager::GetDatabaseInstanceIndex(String const &key) const
{
  return instances_.size() == 1 ? 0 : (XXH32(key.data(), key.size(), 0) & (instances_.size() - 1));
}

/* If continue use `GetDatabaseInstance()`, the two keys from same shard may be allocated to
 * different database instance, unless you can ensure all key has same value of `Hash(key) %
 * instance_num`. We can ensure their `Hash(key) % shard_num` are same only.
 *
 * Hence, we use the `shard_id`(Hash(key) % shard_num) to ensure all keys in shard must be located
 * to same database instance.
 */
size_t DatabaseManager::GetDatabaseInstanceIndex2(shard_id_t shard_id) const
{
  return instances_.size() == 1 ? 0 : (shard_id & (instances_.size() - 1));
}
