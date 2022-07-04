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

#include <kanon/util/noncopyable.h>

namespace mmkv {
namespace db {

using algo::Dictionary;
using algo::KeyValue;
using protocol::StrValues;
using protocol::StatusCode;

#define DB_MIN(x, y) (((x) < (y)) ? (x) : (y))

class MmkvDb {
  using Dict = Dictionary<String, MmkvData>;

  DISABLE_EVIL_COPYABLE(MmkvDb);
 public:
  MmkvDb();
  ~MmkvDb() noexcept;
  
  int InsertStr(String k, String v);
  int EraseStr(String const& k);
  String* GetStr(String const& k) noexcept;

  bool ListAdd(String k, StrValues& elems); 
  bool ListAppend(String const& k, StrValues& elems);
  bool ListPrepend(String const& k, StrValues& elems); 
  size_t ListGetSize(String const& k); 
  bool ListGetAll(String const& k, StrValues& values); 
  StatusCode ListGetRange(String const& k, StrValues& values,  size_t l, size_t r); 
  bool ListPopFront(String const& k, uint32_t count); 
  bool ListPopBack(String const& k, uint32_t count); 
  bool ListDel(String const& k); 
  
  bool Delete(String const& k);
 private:
  Dict dict_;

};

} // db
} // mmkv

#endif // _MMKV_DB_KVDB_H_
