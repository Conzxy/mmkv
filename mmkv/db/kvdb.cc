#include "kvdb.h"
#include "mmkv/db/data_type.h"
#include "mmkv/db/mmkv_data.h"
#include "mmkv/db/vset.h"
#include "mmkv/protocol/command.h"
#include "mmkv/protocol/status_code.h"
#include "mmkv/protocol/mmbp_type.h"
#include "mmkv/protocol/mmbp_request.h"
#include "mmkv/protocol/type.h"
#include "mmkv/server/config.h"
#include "mmkv/util/time_util.h"

#include "mmkv/disk/request_log.h"

#include <kanon/log/logger.h>

using namespace mmkv::db;
using namespace mmkv::protocol;
using namespace mmkv::disk;
using namespace mmkv::server;
using namespace kanon;

void MmkvDb::GetAllKeys(StrValues& keys) {
  // Don't reclaim expired kv
  keys.clear();
  keys.reserve(dict_.size());
  for (auto const& kv : dict_) {
    keys.emplace_back(kv.key);
  }
}

bool MmkvDb::Type(String const& key, DataType& type) noexcept {
  if (CheckExpire(key)) return false;

  auto kv = dict_.Find(key);
  if (!kv) return false; 

  type = kv->value.type;
  return true;
}

void MmkvDb::DeleteData(MmkvData &value) {
  switch (value.type) {
    case D_STRING:
      delete (String*)value.any_data;
      break;
    case D_STRLIST:
      delete (StrList*)value.any_data;
      break;
    case D_SORTED_SET:
      delete (Vset*)value.any_data;
      break;
    case D_MAP:
      delete (Map*)value.any_data;
      break;
    case D_SET:
      delete (Set*)value.any_data;
      break;
  }
}

bool MmkvDb::Delete(String const& k) {
  // Don't call CheckExpire()
  // Delete handle all

  auto node = dict_.Extract(k);

  if (!node) {
    return false;
  }
  
  auto& value = node->value.value;
  DeleteData(value);
  dict_.DropNode(node);

  // It's ok even though k doesn't exists
  exp_dict_.Erase(k);

  return true;
}

StatusCode MmkvDb::Rename(String const& old_name, String&& new_name) {
  if (CheckExpire(old_name)) return S_NONEXISTS;

  auto exists = dict_.Find(new_name);
  if (exists) return S_EXISTS;

  auto node = dict_.Extract(old_name);
  if (!node) return S_NONEXISTS;

  // auto kv = dict_.InsertKv(std::move(new_name), std::move(node->value.value));(void)kv;
  // assert(kv);
  // dict_.DropNode(node);
  node->value.key = std::move(new_name);
  // FIXME Efficient push without check of unique key
  auto success = dict_.Push(node); (void)success;
  assert(success);
  return S_OK;
}

StatusCode MmkvDb::InsertStr(String k, String v) {
  MmkvData data = {
    .type = D_STRING,
    .any_data = nullptr, // dummy
  };

  auto kv = dict_.InsertKv(std::move(k), std::move(data));
  if (!kv) return S_EXISTS;
  kv->value.any_data = new String(std::move(v));

  return S_OK;
}

StatusCode MmkvDb::EraseStr(String const& k) {
  typename Dict::Bucket* bucket = nullptr;
  auto slot = dict_.FindNode(k, &bucket);
  auto& str = (slot)->value.value;

  if (slot) {
    if (str.type == D_STRING) {
      delete (String*)str.any_data;
      dict_.EraseNode(bucket, slot);
      return S_OK;
    } else {
      return S_EXISITS_DIFF_TYPE;
    }
  }

  exp_dict_.Erase(k);
  return S_NONEXISTS;
}

StatusCode MmkvDb::GetStr(String const& k, String*& str) noexcept {
  if (CheckExpire(k)) return S_NONEXISTS;

  KeyValue<String, MmkvData>* data = dict_.Find(k);
  if (data) {
    if (data->value.type == D_STRING) {
      str = (String*)data->value.any_data;
      return S_OK;
    }
    else return S_EXISITS_DIFF_TYPE;
  }
  return S_NONEXISTS;
}

StatusCode MmkvDb::SetStr(String k, String v) {
  if (CheckExpire(k)) return S_NONEXISTS;

  Dict::value_type* duplicate = nullptr;
  MmkvData data = {
    .type = D_STRING,
    .any_data = nullptr,
  };

  auto success = dict_.InsertKvWithDuplicate(std::move(k), data, duplicate);
  if (success) {
    duplicate->value.any_data = new String(std::move(v));
  } else {
    if (duplicate->value.type == D_STRING) {
      *((String*)(duplicate->value.any_data)) = std::move(v);
    } else {
      return S_EXISITS_DIFF_TYPE;
    }
  }

  return S_OK;
}

#define LIST_ERROR_ROUTINE \
  auto kv = dict_.Find(k); \
  if (!kv) return S_NONEXISTS; \
  if (kv->value.type != D_STRLIST) return S_EXISITS_DIFF_TYPE

#define CHECK_EXPIRE_ROUTINE(key) \
  if (CheckExpire(key)) return S_NONEXISTS

StatusCode MmkvDb::ListAdd(String k, StrValues& elems) {
  MmkvData data = {
    .type = D_STRLIST,
    .any_data = nullptr, // dummy
  };

  auto ret = dict_.InsertKv(std::move(k), std::move(data));
  if (!ret) return S_EXISTS;

  StrList* lst = new StrList();
  for (auto& elem : elems) { 
    lst->PushBack(std::move(elem));
  }

  ret->value.any_data = lst;

  return S_OK;
}

StatusCode MmkvDb::ListAppend(String const& k, StrValues& elems) {
  if (CheckExpire(k)) return S_NONEXISTS;
  LIST_ERROR_ROUTINE;

  auto lst = (StrList*)(kv->value.any_data);
  for (auto& elem : elems) {
    lst->PushBack(std::move(elem));
  }

  return S_OK;
}


StatusCode MmkvDb::ListPrepend(String const& k, StrValues& elems) {
  if (CheckExpire(k)) return S_NONEXISTS;
  LIST_ERROR_ROUTINE;

  auto lst = (StrList*)(kv->value.any_data);
  for (auto& elem : elems) {
    lst->PushFront(std::move(elem));
  }

  return S_OK;
}

StatusCode MmkvDb::ListGetSize(String const& k, size_t& size) {
  if (CheckExpire(k)) return S_NONEXISTS;
  LIST_ERROR_ROUTINE;

  auto lst = (StrList*)kv->value.any_data;
  size = lst->size();
  return S_OK;
}


StatusCode MmkvDb::ListGetAll(String const& k, StrValues& values) {
  if (CheckExpire(k)) return S_NONEXISTS;
  LIST_ERROR_ROUTINE;
  auto lst = (StrList*)kv->value.any_data;
  values.resize(lst->size());

  auto beg = lst->begin();
  for (size_t i = 0; i < lst->size(); ++i) {
    values[i] = *beg;
    ++beg;
  }

  return S_OK;
}


StatusCode MmkvDb::ListGetRange(String const& k, StrValues& values, size_t l, size_t r) {
  if (CheckExpire(k)) return S_NONEXISTS;
  LIST_ERROR_ROUTINE;
  
  auto lst = (StrList*)kv->value.any_data;

  if (l >= lst->size()) {
    return S_INVALID_RANGE;
  }

  const auto size = DB_MIN(r, lst->size()) - l;
  values.resize(r-l);

  auto beg = lst->begin();
  while (l--) {
    ++beg;
  }
  
  for (size_t i = 0; i < size; ++i) {
    values[i] = *beg;
    ++beg;
  }

  return S_OK;
}


StatusCode MmkvDb::ListPopFront(String const& k, uint32_t count) {
  CHECK_EXPIRE_ROUTINE(k);
  LIST_ERROR_ROUTINE;

  auto lst = (StrList*)kv->value.any_data;
  count = DB_MIN(count, lst->size());
  
  // FIXME implement PopFront(count) 
  while (count--) { 
    lst->PopFront();
  }

  return S_OK;
}


StatusCode MmkvDb::ListPopBack(String const& k, uint32_t count) {
  CHECK_EXPIRE_ROUTINE(k);
  LIST_ERROR_ROUTINE;

  auto lst = (StrList*)kv->value.any_data;
  count = DB_MIN(count, lst->size()); 
  while (count--) {
    lst->PopBack();
  }

  return S_OK;
}


StatusCode MmkvDb::ListDel(String const& k) {
  Dict::Bucket* bucket = nullptr;
  auto slot = dict_.FindNode(k, &bucket);
  auto& str = (slot)->value.value;

  if (slot) {
    if (str.type == D_STRLIST) {
      delete (StrList*)str.any_data;
      dict_.EraseNode(bucket, slot);
      return S_OK;
    } else {
      return S_EXISITS_DIFF_TYPE;
    }
  }

  exp_dict_.Erase(k);
  return S_NONEXISTS;
}

#define TO_VSET(kv) ((Vset*)(kv->value.any_data))

StatusCode MmkvDb::VsetAdd(String&& key, WeightValues&& wms, size_t& count) {
  MmkvData data {
    .type = D_SORTED_SET,
    .any_data = nullptr, // dummy
  };

  Dict::value_type* duplicate = nullptr;
  auto success = dict_.InsertKvWithDuplicate(std::move(key), std::move(data), duplicate);

  if (success || duplicate->value.type == D_SORTED_SET) {
    Vset* vset = nullptr;
    if (success) {
      vset = new Vset();
      duplicate->value.any_data = vset;
    } else {
      vset = TO_VSET(duplicate);
    }

    count = 0;
    for (auto& wm : wms) {
      count += (int)vset->Insert(wm.key, std::move(wm.value));
    }
    return S_OK;
  }

  assert(!success && duplicate->value.type != D_SORTED_SET);
  return S_EXISITS_DIFF_TYPE;
}

#define ERROR_ROUTINE(_var, _type) \
  if (!(_var)) return S_NONEXISTS; \
  if ((_var)->value.type != (_type)) return S_EXISITS_DIFF_TYPE;

#define ERROR_ROUTINE_KV(_type) \
  auto kv = dict_.Find(key); \
  if (!kv) return S_NONEXISTS; \
  if (kv->value.type != (_type)) return S_EXISITS_DIFF_TYPE

StatusCode MmkvDb::VsetDel(String const& key, String const& member) {
  CHECK_EXPIRE_ROUTINE(key);
  ERROR_ROUTINE_KV(D_SORTED_SET);
  if (TO_VSET(kv)->Erase(member)) {
    return S_OK;
  }
  return S_VMEMBER_NONEXISTS;
}

StatusCode MmkvDb::VsetDelRange(String const& key, OrderRange range, size_t& count) {
  CHECK_EXPIRE_ROUTINE(key);
  ERROR_ROUTINE_KV(D_SORTED_SET);
  count = TO_VSET(kv)->EraseRange(range.left, range.right);
  return S_OK;
}

StatusCode MmkvDb::VsetDelRangeByWeight(String const& key, WeightRange range, size_t& count) {
  CHECK_EXPIRE_ROUTINE(key);
  ERROR_ROUTINE_KV(D_SORTED_SET);
  count = TO_VSET(kv)->EraseRangeByWeight(range.left, range.right);
  return S_OK;
}

StatusCode MmkvDb::VsetSize(String const& key, size_t& count) {
  CHECK_EXPIRE_ROUTINE(key);
  ERROR_ROUTINE_KV(D_SORTED_SET);
  count = TO_VSET(kv)->GetSize();
  return S_OK;
}

StatusCode MmkvDb::VsetSizeByWeight(String const& key, WeightRange range, size_t& count) {
  CHECK_EXPIRE_ROUTINE(key);
  ERROR_ROUTINE_KV(D_SORTED_SET);
  count = TO_VSET(kv)->GetSizeByWeight(range.left, range.right);
  return S_OK;
}

StatusCode MmkvDb::VsetWeight(String const& key, String const& member, Weight& w) {
  CHECK_EXPIRE_ROUTINE(key);
  ERROR_ROUTINE_KV(D_SORTED_SET);
  if (TO_VSET(kv)->GetWeight(member, w)) return S_OK;
  else return S_VMEMBER_NONEXISTS;
}

StatusCode MmkvDb::VsetOrder(String const& key, String const& member, size_t& order) {
  CHECK_EXPIRE_ROUTINE(key);
  ERROR_ROUTINE_KV(D_SORTED_SET);
  if (TO_VSET(kv)->GetOrder(member, order)) return S_OK;
  else return S_VMEMBER_NONEXISTS;
}

StatusCode MmkvDb::VsetROrder(String const& key, String const& member, size_t& order) {
  CHECK_EXPIRE_ROUTINE(key);
  ERROR_ROUTINE_KV(D_SORTED_SET);
  if (TO_VSET(kv)->GetROrder(member, order)) return S_OK;
  else return S_VMEMBER_NONEXISTS;
}

StatusCode MmkvDb::VsetAll(String const& key, WeightValues& wms) {
  CHECK_EXPIRE_ROUTINE(key);
  ERROR_ROUTINE_KV(D_SORTED_SET);
  TO_VSET(kv)->GetAll(wms); 
  return S_OK;
}

StatusCode MmkvDb::VsetRange(String const& key, OrderRange range, WeightValues& wms) {
  CHECK_EXPIRE_ROUTINE(key);
  ERROR_ROUTINE_KV(D_SORTED_SET);
  TO_VSET(kv)->GetRange(range.left, range.right, wms); 
  return S_OK;
}

StatusCode MmkvDb::VsetRangeByWeight(String const& key, WeightRange range, WeightValues& wms) {
  CHECK_EXPIRE_ROUTINE(key);
  ERROR_ROUTINE_KV(D_SORTED_SET);
  TO_VSET(kv)->GetRangeByWeight(range.left, range.right, wms); 
  return S_OK;
}

StatusCode MmkvDb::VsetRRange(String const& key, OrderRange range, WeightValues& wms) {
  CHECK_EXPIRE_ROUTINE(key);
  ERROR_ROUTINE_KV(D_SORTED_SET);
  
  TO_VSET(kv)->GetRRange(range.left, range.right, wms); 
  return S_OK;
}

StatusCode MmkvDb::VsetRRangeByWeight(String const& key, WeightRange range, WeightValues& wms) {
  CHECK_EXPIRE_ROUTINE(key);
  ERROR_ROUTINE_KV(D_SORTED_SET); 
  TO_VSET(kv)->GetRRangeByWeight(range.left, range.right, wms); 
  return S_OK;
}

StatusCode MmkvDb::MapAdd(String&& key, StrKvs&& kvs, size_t& count) {
  MmkvData data {
    .type = D_MAP,
    .any_data = nullptr,
  };
  
  Dict::value_type* duplicate = nullptr; 

  auto success = dict_.InsertKvWithDuplicate(std::move(key), std::move(data), duplicate);
  if (success || duplicate->value.type == D_MAP) {
    Map* map = nullptr;
    if (success) {
      map = new Map();
      duplicate->value.any_data = map;
    } else {
      map = (Map*)duplicate->value.any_data;
    }

    count = 0;
    for (auto& kv : kvs) {
      count += map->Insert(std::move(kv)) ? 1 : 0;
    }
    return S_OK;
  }
  
  return S_EXISITS_DIFF_TYPE;
}

#define TO_MAP ((Map*)(kv->value.any_data))

StatusCode MmkvDb::MapGet(String const& key, String const& field, String& value) {
  CHECK_EXPIRE_ROUTINE(key);
  ERROR_ROUTINE_KV(D_MAP);
  
  auto map = TO_MAP;
  auto fv = map->Find(field);

  if (fv) {
    value = fv->value;
    return S_OK;
  }

  return S_FIELD_NONEXISTS;
}

StatusCode MmkvDb::MapGets(String const& key, StrValues const& fields, StrValues& values) {
  CHECK_EXPIRE_ROUTINE(key);
  ERROR_ROUTINE_KV(D_MAP);
  
  auto map = TO_MAP;

  for (auto const& field : fields) {
    auto fv = map->Find(field);

    if (fv) {
      values.push_back(fv->value);
    } else {
      return S_FIELD_NONEXISTS;
    }
  }

  return S_OK;
}

StatusCode MmkvDb::MapSet(String const& key, String&& field, String&& value) {
  CHECK_EXPIRE_ROUTINE(key);
  ERROR_ROUTINE_KV(D_MAP);

  auto map = TO_MAP;

  Map::value_type* duplicate = nullptr;
  // auto success = map->InsertKvWithDuplicate(std::move(field), std::move(value), duplicate);
  StrKeyValue fv{ std::move(field), std::move(value) };
  auto success = map->InsertWithDuplicate(std::move(fv), duplicate);

  if (!success) {
    duplicate->value = std::move(fv.value);
  }

  return S_OK;
}

StatusCode MmkvDb::MapDel(String const& key, String const& field) {
  CHECK_EXPIRE_ROUTINE(key);
  ERROR_ROUTINE_KV(D_MAP);

  if (TO_MAP->Erase(field)) {
    return S_OK;
  }

  return S_FIELD_NONEXISTS;
}

StatusCode MmkvDb::MapSize(String const& key, size_t& count) {
  CHECK_EXPIRE_ROUTINE(key);
  ERROR_ROUTINE_KV(D_MAP);
  count = TO_MAP->size(); 
  return S_OK;
}

StatusCode MmkvDb::MapExists(String const& key, String const& field) {
  CHECK_EXPIRE_ROUTINE(key);
  ERROR_ROUTINE_KV(D_MAP);
  if (TO_MAP->Find(field)) {
    return S_OK;
  }

  return S_FIELD_NONEXISTS;
}

StatusCode MmkvDb::MapFields(String const& key, StrValues& fields) {
  CHECK_EXPIRE_ROUTINE(key);
  ERROR_ROUTINE_KV(D_MAP);
  auto& m = *TO_MAP;

  for (auto const& kv : m) {
    fields.push_back(kv.key);
  }

  return S_OK;
}

StatusCode MmkvDb::MapValues(String const& key, StrValues& values) {
  CHECK_EXPIRE_ROUTINE(key);
  ERROR_ROUTINE_KV(D_MAP);
  auto& m = *TO_MAP;

  for (auto const& kv : m) {
    values.push_back(kv.value);
  }

  return S_OK;
}

StatusCode MmkvDb::MapAll(String const& key, StrKvs& kvs) {
  CHECK_EXPIRE_ROUTINE(key);
  ERROR_ROUTINE_KV(D_MAP);
  auto& m = *TO_MAP;

  for (auto const& kv : m) {
    kvs.push_back({kv.key, kv.value});
  }

  return S_OK;
}

StatusCode MmkvDb::SetAdd(String&& key, StrValues& members, size_t& count) {
  MmkvData data {
    .type = D_SET,
    .any_data = nullptr,
  };
  
  Dict::value_type* duplicate = nullptr; 

  auto success = dict_.InsertKvWithDuplicate(std::move(key), std::move(data), duplicate);
  if (success || duplicate->value.type == D_MAP) {
    Set* set = nullptr;
    if (success) {
      set = new Set();
      duplicate->value.any_data = set;
    } else {
      set = (Set*)duplicate->value.any_data;
    }

    count = 0;
    for (auto& m : members) {
      count += set->Insert(std::move(m)) ? 1 : 0;
    }
    return S_OK;
  }
  
  return S_EXISITS_DIFF_TYPE;   
}

#define TO_SET(_data) ((Set*)((_data).any_data))

StatusCode MmkvDb::SetDelm(const String &key, const String &member) {
  CHECK_EXPIRE_ROUTINE(key);
  ERROR_ROUTINE_KV(D_SET);
  auto set = TO_SET(kv->value); 
  
  if (set->Erase(member)) return S_OK;
  return S_SET_MEMBER_NONEXISTS;
}

StatusCode MmkvDb::SetSize(String const& key, size_t& count) {
  CHECK_EXPIRE_ROUTINE(key);
  ERROR_ROUTINE_KV(D_SET);
  count = TO_SET(kv->value)->size();
  return S_OK;
}

StatusCode MmkvDb::SetExists(String const& key, String const& member) {
  CHECK_EXPIRE_ROUTINE(key);
  ERROR_ROUTINE_KV(D_SET);
  if (TO_SET(kv->value)->Find(member)) return S_OK;
  else return S_SET_MEMBER_NONEXISTS;
}

StatusCode MmkvDb::SetAll(String const& key, StrValues& members) {
  CHECK_EXPIRE_ROUTINE(key);
  ERROR_ROUTINE_KV(D_SET);
  auto set = TO_SET(kv->value);

  for (auto const& m : *set) {
    members.push_back(m);
  }

  return S_OK;
}

#define SET_OP_ROUTINE \
  auto kv1 = dict_.Find(key1); \
  ERROR_ROUTINE(kv1, D_SET); \
  auto kv2 = dict_.Find(key2); \
  ERROR_ROUTINE(kv2, D_SET); \
  auto set1 = TO_SET(kv1->value); \
  auto set2 = TO_SET(kv2->value)

StatusCode MmkvDb::SetAnd(String const& key1, String const& key2, StrValues& members) {
  // FIXME
  CHECK_EXPIRE_ROUTINE(key1);
  CHECK_EXPIRE_ROUTINE(key2);

  SET_OP_ROUTINE;
  set1->Intersection(*set2, [&members](String const& m) {
      members.push_back(m);
      });

  return S_OK;
}

#define SET_OP_TO_ROUTINE \
  MmkvData data { \
    .type = D_SET, \
    .any_data = nullptr, \
  }; \
 \
  Dict::value_type* duplicate = nullptr; \
  auto success = dict_.InsertKvWithDuplicate(std::move(dest), std::move(data), duplicate); \
   \
  Set* dest_set = nullptr; \
  if (success) { \
    dest_set = new Set(); \
    duplicate->value.any_data = dest_set; \
  } else { \
    dest_set = TO_SET(duplicate->value); \
  }

StatusCode MmkvDb::SetAndTo(String const& key1, String const& key2, String&& dest) {
  CHECK_EXPIRE_ROUTINE(key1);
  CHECK_EXPIRE_ROUTINE(key2);
  SET_OP_ROUTINE;
  SET_OP_TO_ROUTINE  
  
  set1->Intersection(*set2, [&dest_set](String const& m) {
      dest_set->Insert(m);
      });

  return S_OK;
}

StatusCode MmkvDb::SetSub(String const& key1, String const& key2, StrValues& members) {
  CHECK_EXPIRE_ROUTINE(key1);
  CHECK_EXPIRE_ROUTINE(key2);
  SET_OP_ROUTINE;

  set1->Difference(*set2, [&members](String const& m) {
      members.push_back(m);
      });

  return S_OK;
}

StatusCode MmkvDb::SetSubTo(String const& key1, String const& key2, String&& dest) {
  CHECK_EXPIRE_ROUTINE(key1);
  CHECK_EXPIRE_ROUTINE(key2);
  SET_OP_ROUTINE;
  SET_OP_TO_ROUTINE
  
  set1->Difference(*set2, [&dest_set](String const& m) {
      dest_set->Insert(m);
      });

  return S_OK;
}

StatusCode MmkvDb::SetOr(String const& key1, String const& key2, StrValues& members) {
  CHECK_EXPIRE_ROUTINE(key1);
  CHECK_EXPIRE_ROUTINE(key2);
  SET_OP_ROUTINE;

  set1->Union(*set2, [&members](String const& m) {
      members.push_back(m);
      });

  return S_OK;
}

StatusCode MmkvDb::SetOrTo(String const& key1, String const& key2, String&& dest) {
  CHECK_EXPIRE_ROUTINE(key1);
  CHECK_EXPIRE_ROUTINE(key2);
  SET_OP_ROUTINE;
  SET_OP_TO_ROUTINE;

  set1->Union(*set2, [&dest_set](String const& m) {
      dest_set->Insert(m);
      });

  return S_OK;
}

StatusCode MmkvDb::SetAndSize(String const& key1, String const& key2, size_t& count) {
  CHECK_EXPIRE_ROUTINE(key1);
  CHECK_EXPIRE_ROUTINE(key2);
  SET_OP_ROUTINE;
  count = 0;
  set1->Intersection(*set2, [&count](String const& m) {
      count++;
      });

  return S_OK;
}

StatusCode MmkvDb::SetOrSize(String const& key1, String const& key2, size_t& count) {
  CHECK_EXPIRE_ROUTINE(key1);
  CHECK_EXPIRE_ROUTINE(key2);
  SET_OP_ROUTINE;
  count = 0;
  set1->Union(*set2, [&count](String const& m) {
      count++;
      });

  return S_OK;
}

StatusCode MmkvDb::SetSubSize(String const& key1, String const& key2, size_t& count) {
  CHECK_EXPIRE_ROUTINE(key1);
  CHECK_EXPIRE_ROUTINE(key2);
  SET_OP_ROUTINE;
  count = 0;
  set1->Difference(*set2, [&count](String const& m) {
      count++;
      });

  return S_OK;
}

void MmkvDb::SetExpire(String &&key, uint64_t expire) {
  ExDict::value_type *duplicate = nullptr;
  const uint64_t cur_ms = util::GetTimeMs();

  LOG_DEBUG << "current ms: " << cur_ms;
  LOG_DEBUG << "expire: " << expire;
  LOG_DEBUG << "diff: " << expire - cur_ms;
  if (cur_ms < expire) {
    const auto success = exp_dict_.InsertKvWithDuplicate(std::move(key), expire, duplicate);
    if (!success)
      duplicate->value = expire;
  }
}

StatusCode MmkvDb::ExpireAtMs(String &&key, uint64_t expire) {
  auto kv = dict_.Find(key);
  if (!kv) return S_NONEXISTS;

  SetExpire(std::move(key), expire);
  return S_OK;
}

void MmkvDb::CheckExpireCycle() {
  std::vector<String> expire_keys;
  const uint64_t cur_ms = util::GetTimeMs();
  Dict::Node *node = nullptr;

  for (auto const &k_exp : exp_dict_) {
    if (k_exp.value <= cur_ms) {
      node = dict_.Extract(k_exp.key);
      DeleteData(node->value.value);
      dict_.DropNode(node); 

      expire_keys.emplace_back(k_exp.key);
    }
  }

  for (auto const &key : expire_keys) {
    exp_dict_.Erase(key);
  }

  if (g_config.log_method == LM_REQUEST) {
    MmbpRequest request;
    Buffer buffer;
    for (auto &key : expire_keys) {
      request.SetKey();
      request.key = std::move(key);
      request.command = DEL;
      request.SerializeTo(buffer);
      buffer.Prepend32(buffer.GetReadableSize());
      g_rlog->Append(buffer.GetReadBegin(), buffer.GetReadableSize());
      buffer.AdvanceAll();
      request.Reset();
    }
  }
}

bool MmkvDb::CheckExpire(String const &key) {
  ExDict::Bucket *bucket = nullptr;
  const auto node = exp_dict_.FindNode(key, &bucket);
  if (!node) return false;
  assert(bucket);

  const uint64_t cur_ms = util::GetTimeMs();
  LOG_DEBUG << "current ms: " << cur_ms;
  if (cur_ms >= node->value.value) {
    exp_dict_.EraseNode(bucket, node);
    auto node2 = dict_.Extract(key);
    DeleteData(node2->value.value);
    dict_.DropNode(node2);

    if (g_config.log_method == LM_REQUEST) {
      Buffer buffer;
      // buffer.ReserveWriteSpace(sizeof(CommandField)+key.size()+sizeof(uint32_t));
      MmbpRequest req;
      req.command = DEL;
      req.key = key; /* FIXME move string instead of copy */
      req.SetKey();
      req.DebugPrint();
      req.SerializeTo(buffer);
      LOG_DEBUG << "request length = " << buffer.GetReadableSize();
      buffer.Prepend32(buffer.GetReadableSize());
      g_rlog->Append(buffer.GetReadBegin(), buffer.GetReadableSize());
    }
    return true;
  }

  return false;
}