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

#define SET_OK_VALUE(_value)                                                   \
  response->SetOk();                                                           \
  response->SetValue();                                                        \
  response->value = (_value)

#define SET_OK_VALUE_                                                          \
  response->SetOk();                                                           \
  response->SetValue()

#define SET_OK_VALUES(_values)                                                 \
  response->SetOk();                                                           \
  response->SetValues();                                                       \
  response->values = (_values)

#define SET_OK_VALUES_                                                         \
  response->SetOk();                                                           \
  response->SetValues()

#define SET_OK_VMEMBERS(_wms)                                                  \
  response->SetOk();                                                           \
  response->SetVmembers();                                                     \
  response->vmembers = (_wms)

#define SET_OK_VMEMBERS_                                                       \
  response->SetOk();                                                           \
  response->SetVmembers()

#define SET_OK_KVS(_kvs)                                                       \
  response->SetOk();                                                           \
  response->SetKvs();                                                          \
  response->kvs = (_kvs);

#define SET_OK_KVS_                                                            \
  response->SetOk();                                                           \
  response->SetKvs()

#define SET_OK_COUNT(_count)                                                   \
  response->SetOk();                                                           \
  response->SetCount();                                                        \
  response->count = (_count)

#define SET_OK_COUNT_                                                          \
  response->SetOk();                                                           \
  response->SetCount();

#define SET_XX_ELSE_CODE(xx)                                                   \
  if (code == S_OK) {                                                          \
    xx;                                                                        \
  } else {                                                                     \
    response->status_code = code;                                              \
  }

#define CHECK_INVALID_REQUEST(cond, msg)                                       \
  do {                                                                         \
    if (response && !(cond)) {                                                 \
      response->status_code = S_INVALID_REQUEST;                               \
      response->value = msg;                                                   \
      response->SetValue();                                                    \
      return;                                                                  \
    }                                                                          \
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

DatabaseManager::DatabaseManager()
  : type_(server::mmkv_config().SupportDistribution()
              ? DatabaseManager::DISTRIBUTED
              : (server::mmkv_config().thread_num == 1
                     ? DatabaseManager::LOCAL
                     : DatabaseManager::LOCAL_MULTI_THREAD))
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
  for (size_t i = 0; i < db_num; ++i) {
    snprintf(db_name, sizeof db_name, "Database %zu", i);
    instances_.emplace_back(new DatabaseInstance(db_name));
  }

  LOG_INFO << "DatabaseManager created";
}

DatabaseManager::~DatabaseManager() noexcept
{
  LOG_INFO << "DatabaseManager removed";
}

void DatabaseManager::CheckExpirationCycle()
{
  auto &instance = instances_[current_index_];
  auto &db = instance->db;

  {
    WLockGuard g(instance->lock);

    if (!db.IsEmpty()) {
      db.CheckExpireCycle();
    }
  }

  // Used in the main thread
  // Don't lock is ok
  if (++current_index_ == instances_.size()) current_index_ = 0;
}

#define DB instance->db

void DatabaseManager::Execute(MmbpRequest &request, MmbpResponse *response)
{
  DatabaseInstance *instance = nullptr;
  auto command_type = GetCommandType((Command)request.command);
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
  case STR_ADD: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "stradd");
    auto code = DB.InsertStr(std::move(request.key), std::move(request.value));
    if (response) {
      response->status_code = code;
    }
  } break;
  case STR_GET: {
    CHECK_INVALID_REQUEST(request.HasKey(), "strget");
    String *str = nullptr;
    auto code = DB.GetStr(request.key, str);
    SET_XX_ELSE_CODE(SET_OK_VALUE(*str));
  } break;
  case STR_DEL: {
    CHECK_INVALID_REQUEST(request.HasKey(), "strdel");
    auto code = DB.EraseStr(request.key);
    if (response) response->status_code = code;
  } break;

  case STR_SET: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "strset");
    auto code = DB.SetStr(std::move(request.key), std::move(request.value));
    if (response) response->status_code = code;
  } break;

  case STRLEN: {
    CHECK_INVALID_REQUEST(request.HasKey(), "strlen");
    String *str = nullptr;
    auto code = DB.GetStr(request.key, str);
    SET_XX_ELSE_CODE(SET_OK_COUNT(str->size()));
  } break;

  case STRAPPEND: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "strappend");
    const auto code = DB.StrAppend(request.key, request.value);
    if (response) response->status_code = code;
  } break;

  case STRPOPBACK: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasCount(), "strpopback");
    const auto code = DB.StrPopBack(request.key, request.count);
    if (response) response->status_code = code;
  } break;

  case LADD: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasValues(), "ladd");
    auto code = DB.ListAdd(std::move(request.key), request.values);
    if (response) response->status_code = code;
  } break;

  case LAPPEND:
  case LPREPEND: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasValues(),
                          "lappend/lprepend");

    StatusCode code;
    switch (request.command) {
    case LAPPEND:
      code = DB.ListAppend(request.key, request.values);
      break;
    case LPREPEND:
      code = DB.ListPrepend(request.key, request.values);
      break;
    }

    if (response) response->status_code = code;
  } break;

  case LPOPFRONT:
  case LPOPBACK: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasCount(),
                          "lpopback/popfront");

    StatusCode code;
    switch (request.command) {
    case LPOPFRONT:
      code = DB.ListPopFront(request.key, request.count);
      break;
    case LPOPBACK:
      code = DB.ListPopBack(request.key, request.count);
      break;
    }

    if (response) {
      SET_XX_ELSE_CODE(SET_OK_COUNT_)
    }
  } break;

  case LGETSIZE: {
    CHECK_INVALID_REQUEST(request.HasKey(), "lgetsize");
    size_t count = 0;
    if ((response->status_code = DB.ListGetSize(request.key, count)) == S_OK) {
      response->SetCount();
      response->count = count;
    }
  } break;

  case LGETRANGE: {
    CHECK_INVALID_REQUEST(request.HasRange(), "lgetrange");

    if ((response->status_code = DB.ListGetRange(request.key,
                                                 response->values,
                                                 request.range.left,
                                                 request.range.right)) ==
        S_OK) {
      response->SetValues();
    }
  } break;

  case LGETALL: {
    CHECK_INVALID_REQUEST(request.HasKey(), "lgetrange/getall");

    if ((response->status_code =
             DB.ListGetAll(request.key, response->values)) == S_OK) {
      response->SetValues();
    }
  } break;

  case LDEL: {
    CHECK_INVALID_REQUEST(request.HasKey(), "ldel");
    auto code = DB.ListDel(request.key);
    if (response) response->status_code = code;
  } break;

  case VADD: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasVmembers(), "vadd");
    size_t count = 0;
    auto code =
        DB.VsetAdd(std::move(request.key), std::move(request.vmembers), count);
    if (response) {
      SET_XX_ELSE_CODE(SET_OK_COUNT(count))
    }
  } break;

  case VALL: {
    CHECK_INVALID_REQUEST(request.HasKey(), "vall");
    WeightValues values;
    if ((response->status_code = DB.VsetAll(request.key, response->vmembers)) ==
        S_OK) {
      response->SetVmembers();
    }
  } break;

  case VDELM: {
    CHECK_INVALID_REQUEST(request.HasKey(), "vdelm");
    auto code = DB.VsetDel(request.key, request.value);
    if (response) response->status_code = code;
  } break;

  case VDELMRANGE: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasRange(), "vdelmrange");
    size_t count = 0;
    auto code = DB.VsetDelRange(request.key, request.range, count);
    if (response) {
      SET_XX_ELSE_CODE(SET_OK_COUNT(count))
    }
  } break;

  case VDELMRANGEBYWEIGHT: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasRange(),
                          "vdelmrangebyweight");
    size_t count = 0;
    auto code =
        DB.VsetDelRangeByWeight(request.key, request.GetWeightRange(), count);
    if (response) {
      SET_XX_ELSE_CODE(SET_OK_COUNT(count))
    }
  } break;

  case VSIZE: {
    CHECK_INVALID_REQUEST(request.HasKey(), "vsize");
    size_t size;
    auto code = DB.VsetSize(request.key, size);
    SET_XX_ELSE_CODE(SET_OK_COUNT(size))
  } break;

  case VSIZEBYWEIGHT: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasRange(),
                          "vsizebyweight");
    size_t size;
    auto code =
        DB.VsetSizeByWeight(request.key, request.GetWeightRange(), size);
    SET_XX_ELSE_CODE(SET_OK_COUNT(size))
  } break;

  case VWEIGHT: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "vweight");
    Weight weight;
    auto code = DB.VsetWeight(request.key, request.value, weight);
    SET_XX_ELSE_CODE(SET_OK_COUNT(util::double2u64(weight)))
  } break;

  case VORDER: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "vorder");
    size_t order = 0;
    auto code = DB.VsetOrder(request.key, request.value, order);
    SET_XX_ELSE_CODE(SET_OK_COUNT(order))
  } break;

  case VRORDER: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "vrorder");
    size_t order = 0;
    auto code = DB.VsetROrder(request.key, request.value, order);
    SET_XX_ELSE_CODE(SET_OK_COUNT(order))
  } break;

  case VRANGE: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasRange(), "vrange");
    auto code = DB.VsetRange(request.key, request.range, response->vmembers);

    SET_XX_ELSE_CODE(SET_OK_VMEMBERS_)
  } break;

  case VRRANGE: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasRange(), "vrrange");
    auto code = DB.VsetRRange(request.key, request.range, response->vmembers);

    SET_XX_ELSE_CODE(SET_OK_VMEMBERS_)
  } break;

  case VRANGEBYWEIGHT: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasRange(),
                          "vrangebyweight");
    auto code = DB.VsetRangeByWeight(request.key,
                                     request.GetWeightRange(),
                                     response->vmembers);

    SET_XX_ELSE_CODE(SET_OK_VMEMBERS_)
  } break;

  case VRRANGEBYWEIGHT: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasRange(),
                          "vrrangebyweight");
    auto code = DB.VsetRRangeByWeight(request.key,
                                      request.GetWeightRange(),
                                      response->vmembers);

    SET_XX_ELSE_CODE(SET_OK_VMEMBERS_)
  } break;

  case MADD: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasKvs(), "madd");
    size_t count = 0;
    auto code =
        DB.MapAdd(std::move(request.key), std::move(request.kvs), count);

    if (response) {
      SET_XX_ELSE_CODE(SET_OK_COUNT(count))
    }
  } break;

  case MGET: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "mget");
    auto code = DB.MapGet(request.key, request.value, response->value);
    SET_XX_ELSE_CODE(SET_OK_VALUE_);
  } break;

  case MGETS: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasValues(), "mgets");
    auto code = DB.MapGets(request.key, request.values, response->values);
    SET_XX_ELSE_CODE(SET_OK_VALUES_);
  }

  case MSET: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasValues(), "mset");
    auto code = DB.MapSet(request.key,
                          std::move(request.values[0]),
                          std::move(request.values[1]));
    if (response) response->status_code = code;
  } break;

  case MDEL: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "mdel");
    auto code = DB.MapDel(request.key, request.value);
    if (response) response->status_code = code;
  } break;

  case MALL: {
    CHECK_INVALID_REQUEST(request.HasKey(), "mall");
    auto code = DB.MapAll(request.key, response->kvs);

    SET_XX_ELSE_CODE(SET_OK_KVS_);
  } break;

  case MFIELDS: {
    CHECK_INVALID_REQUEST(request.HasKey(), "mfields");
    auto code = DB.MapFields(request.key, response->values);
    SET_XX_ELSE_CODE(SET_OK_VALUES_);
  } break;

  case MVALUES: {
    CHECK_INVALID_REQUEST(request.HasKey(), "mvalues");
    auto code = DB.MapValues(request.key, response->values);
    SET_XX_ELSE_CODE(SET_OK_VALUES_);
  } break;

  case MSIZE: {
    CHECK_INVALID_REQUEST(request.HasKey(), "msize");
    size_t count = 0;
    auto code = DB.MapSize(request.key, count);
    SET_XX_ELSE_CODE(SET_OK_COUNT(count));
  } break;

  case MEXISTS: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "mexists");
    response->status_code = DB.MapExists(request.key, request.value);
  } break;

  case SADD: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasValues(), "sadd");
    size_t count = 0;
    auto code = DB.SetAdd(std::move(request.key), request.values, count);
    if (response) {
      SET_XX_ELSE_CODE(SET_OK_COUNT(count))
    }
  } break;

  case SDELM: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "sdelm");
    auto code = DB.SetDelm(request.key, request.value);
    if (response) response->status_code = code;
  } break;

  case SEXISTS: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "sexists");
    response->status_code = DB.SetExists(request.key, request.value);
  } break;

  case SSIZE: {
    CHECK_INVALID_REQUEST(request.HasKey(), "ssize");
    size_t count = 0;
    response->status_code = DB.SetSize(request.key, count);
    if (response->status_code == S_OK) {
      response->count = count;
      response->SetCount();
    }
  } break;

  case SALL: {
    CHECK_INVALID_REQUEST(request.HasKey(), "sall");
    response->status_code = DB.SetAll(request.key, response->values);
    if (response->status_code == S_OK) response->SetValues();
  } break;

  case SAND: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "sand");
    response->status_code =
        DB.SetAnd(request.key, request.value, response->values);
    if (response->status_code == S_OK) response->SetValues();
  } break;

  case SOR: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "sor");
    response->status_code =
        DB.SetOr(request.key, request.value, response->values);
    if (response->status_code == S_OK) response->SetValues();
  } break;

  case SSUB: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "ssub");
    response->status_code =
        DB.SetSub(request.key, request.value, response->values);
    if (response->status_code == S_OK) response->SetValues();
  } break;

  case SANDTO: {
    CHECK_INVALID_REQUEST(request.HasValues(), "sandto");
    auto code = DB.SetAndTo(request.values[1],
                            request.values[2],
                            std::move(request.values[0]));
    if (response) response->status_code = code;
  } break;

  case SORTO: {
    CHECK_INVALID_REQUEST(request.HasValues(), "sorto");
    auto code = DB.SetOrTo(request.values[1],
                           request.values[2],
                           std::move(request.values[0]));
    if (response) response->status_code = code;
  } break;

  case SSUBTO: {
    CHECK_INVALID_REQUEST(request.HasValues(), "ssubto");
    auto code = DB.SetSubTo(request.values[1],
                            request.values[2],
                            std::move(request.values[0]));
    if (response) response->status_code = code;
  } break;

  case SANDSIZE: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "sandsize");
    size_t count = 0;
    auto code = DB.SetAndSize(request.key, request.value, count);
    SET_XX_ELSE_CODE(SET_OK_COUNT(count));
  } break;

  case SORSIZE: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "sorsize");
    size_t count = 0;
    auto code = DB.SetOrSize(request.key, request.value, count);
    SET_XX_ELSE_CODE(SET_OK_COUNT(count));
  } break;

  case SSUBSIZE: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "ssubsize");
    size_t count = 0;
    auto code = DB.SetSubSize(request.key, request.value, count);
    SET_XX_ELSE_CODE(SET_OK_COUNT(count));
  } break;
  case SRANDDELM: {
    CHECK_INVALID_REQUEST(request.HasKey(), "sranddelm");
    const auto code = DB.SetRandDelm(request.key);
    if (response) response->status_code = code;
  } break;
  case MEM_STAT: {
    CHECK_INVALID_REQUEST(request.HasNone(), "memorystat");
    SET_OK_VALUE(util::GetMemoryStat());
  } break;

  case EXPIRE_AT: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasExpireTime(),
                          "expireat");
    const auto code = DB.ExpireAt(std::move(request.key), request.expire_time);
    if (response) response->status_code = code;
  } break;

  case EXPIRE_AFTER: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasExpireTime(),
                          "expireafter");
    const auto code =
        DB.ExpireAfter(std::move(request.key), recv_time_, request.expire_time);
    if (response) response->status_code = code;
  } break;

  case EXPIREM_AT: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasExpireTime(),
                          "expiremat");
    const auto code =
        DB.ExpireAtMs(std::move(request.key), request.expire_time);
    if (response) response->status_code = code;

  } break;

  case EXPIREM_AFTER: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasExpireTime(),
                          "expiremafter");
    const auto code = DB.ExpireAfterMs(std::move(request.key),
                                       recv_time_,
                                       request.expire_time);
    if (response) response->status_code = code;
  } break;

  case PERSIST: {
    CHECK_INVALID_REQUEST(request.HasKey(), "persist");
    const auto code = DB.Persist(request.key);
    if (response) response->status_code = code;
  } break;

  case EXPIRATION: {
    CHECK_INVALID_REQUEST(request.HasKey(), "expiration");
    const auto code = DB.GetExpiration(request.key, response->count);
    response->SetCount();
    response->status_code = code;
  } break;

  case TTL: {
    CHECK_INVALID_REQUEST(request.HasKey(), "TTL");
    const auto code = DB.GetTimeToLive(request.key, response->count);
    response->SetCount();
    response->status_code = code;
  } break;

  case TYPE: {
    CHECK_INVALID_REQUEST(request.HasKey(), "type");
    db::DataType type;
    if (DB.Type(request.key, type)) {
      SET_OK_VALUE(GetDataTypeString(type));
    } else {
      response->status_code = S_NONEXISTS;
    }
  } break;

  case DEL: {
    CHECK_INVALID_REQUEST(request.HasKey(), "del");
    auto success = DB.Delete(request.key);
    if (response) response->status_code = success ? S_OK : S_NONEXISTS;
  } break;

  case RENAME: {
    CHECK_INVALID_REQUEST(request.HasKey() && request.HasValue(), "rename");
    auto code = DB.Rename(request.key, std::move(request.value));
    if (response) response->status_code = code;
  } break;

#define RLOCK_ALL                                                              \
  for (auto &instance : instances_) {                                          \
    instance->lock.RLock();                                                    \
  }

#define RUNLOCK_ALL                                                            \
  for (auto &instance : instances_) {                                          \
    instance->lock.RUnlock();                                                  \
  }

#define WLOCK_ALL                                                              \
  for (auto &instance : instances_) {                                          \
    instance->lock.WLock();                                                    \
  }

#define WUNLOCK_ALL                                                            \
  for (auto &instance : instances_) {                                          \
    instance->lock.WUnlock();                                                  \
  }

  case KEYALL: {
    RLOCK_ALL
    for (auto const &db_instance : instances_) {
      db_instance->db.GetAllKeys(response->values);
    }
    response->SetValues();
    response->SetOk();
    RUNLOCK_ALL
  } break;

  case DELS: {
    auto &keys = request.values;
    size_t count = 0;
    for (auto const &key : keys) {
      count += GetDatabaseInstance(key).db.Delete(key) ? 1 : 0;
    }

    if (response) {
      response->status_code = S_OK;
      ;
      response->count = count;
      response->SetCount();
    }
  } break;

  case DELALL: {
    WLOCK_ALL
    size_t count = 0;
    for (auto const &db_instance : instances_) {
      count += db_instance->db.DeleteAll();
    }
    if (response) {
      response->status_code = S_OK;
      response->count = count;
      response->SetCount();
    }
    WUNLOCK_ALL
  } break;
  }

  if (instance) {
    if (command_type == CommandType::CT_READ) {
      instance->lock.RUnlock();
    } else if (command_type == CommandType::CT_WRITE) {
      instance->lock.WUnlock();
    }
  }
}

size_t DatabaseManager::GetDatabaseInstanceIndex(String const &key)
{
  return instances_.size() == 1
             ? 0
             : (XXH32(key.data(), key.size(), 0) & instances_.size());
}
