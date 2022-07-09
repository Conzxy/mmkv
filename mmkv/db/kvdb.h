#ifndef _MMKV_DB_KVDB_H_
#define _MMKV_DB_KVDB_H_

#include "mmkv/algo/libc_allocator_with_realloc.h"
#include "mmkv/algo/string.h"
#include "mmkv/algo/dictionary.h"
#include "mmkv/algo/key_value.h"
#include "mmkv/protocol/mmbp.h"
#include "mmkv/protocol/status_code.h"
#include "mmkv_data.h"
#include "type.h"
#include "mmkv/protocol/type.h"

#include <kanon/util/noncopyable.h>

namespace mmkv {
namespace db {

using algo::Dictionary;
using protocol::StrValues;
using protocol::StatusCode;
using protocol::WeightValues;
using protocol::StrKvs;
using protocol::OrderRange;
using protocol::WeightRange;

#define DB_MIN(x, y) (((x) < (y)) ? (x) : (y))

class MmkvDb {
  using Dict = Dictionary<String, MmkvData>;

  DISABLE_EVIL_COPYABLE(MmkvDb)
 public:
  MmkvDb();
  ~MmkvDb() noexcept;
  
  bool Delete(String const& k);
  bool Type(String const& key, DataType& type) noexcept;
  StatusCode Rename(String const& old_name, String&& new_name);

  StatusCode InsertStr(String k, String v);
  StatusCode EraseStr(String const& k);
  StatusCode GetStr(String const& k, String*& str) noexcept;

  StatusCode ListAdd(String k, StrValues& elems); 
  StatusCode ListAppend(String const& k, StrValues& elems);
  StatusCode ListPrepend(String const& k, StrValues& elems); 
  StatusCode ListGetSize(String const& k, size_t& size); 
  StatusCode ListGetAll(String const& k, StrValues& values); 
  StatusCode ListGetRange(String const& k, StrValues& values,  size_t l, size_t r); 
  StatusCode ListPopFront(String const& k, uint32_t count); 
  StatusCode ListPopBack(String const& k, uint32_t count); 
  StatusCode ListDel(String const& k); 
  
  StatusCode VsetAdd(String&& key, WeightValues&& wms, size_t& count);

  StatusCode VsetDel(String const& key, String const& member);
  StatusCode VsetDelRange(String const& key, OrderRange range, size_t& count);
  StatusCode VsetDelRangeByWeight(String const& key, WeightRange range, size_t& count);

  StatusCode VsetSize(String const& key, size_t& count);
  StatusCode VsetSizeByWeight(String const& key, WeightRange range, size_t& count);
  StatusCode VsetWeight(String const& key, String const& member, Weight& w);
  StatusCode VsetOrder(String const& key, String const& member, size_t& order);
  StatusCode VsetROrder(String const& key, String const& member, size_t& order);
  
  StatusCode VsetAll(String const& key, WeightValues& wms);
  StatusCode VsetRange(String const& key, OrderRange range, WeightValues& wms);
  StatusCode VsetRangeByWeight(String const& key, WeightRange range, WeightValues& wms);
  StatusCode VsetRRange(String const& key, OrderRange range, WeightValues& wms);
  StatusCode VsetRRangeByWeight(String const& key, WeightRange range, WeightValues& wms);

  StatusCode MapAdd(String&& key, StrKvs&& kvs, size_t& count);
  StatusCode MapSet(String const& key, String&& field, String&& value);
  StatusCode MapGet(String const& key, String const& field, String& value);
  StatusCode MapGets(String const& key, StrValues const& fields, StrValues& values);
  StatusCode MapDel(String const& key, String const& field);
  StatusCode MapAll(String const& key, StrKvs& kvs);
  StatusCode MapFields(String const& key, StrValues& fields);
  StatusCode MapValues(String const& key, StrValues& values);
  StatusCode MapSize(String const& key, size_t& count);
  StatusCode MapExists(String const& key, String const& field);

 private:
  Dict dict_;

};

} // db
} // mmkv

#endif // _MMKV_DB_KVDB_H_
