#ifndef _MMKV_DB_KVDB_H_
#define _MMKV_DB_KVDB_H_

#include "mmkv/algo/string.h"

#include "mmkv/algo/dictionary.h"
#include "mmkv/algo/key_value.h"

namespace mmkv {
namespace db {

using algo::String;

class MmkvDb {
  using StrDict = algo::Dictionary<String, String>;
  using StrKvType = StrDict::value_type;

 public:
  MmkvDb();
  ~MmkvDb() noexcept;
  
  int InsertStr(String k, String v) {
    return dict_.InsertKv(std::move(k), std::move(v)) ? 1 : 0;
  }
  
  int EraseStr(String const& k) {
    return dict_.Erase(k);
  }
  
  StrKvType* GetStr(String const& k) noexcept {
    return dict_.Find(k);
  }

 private:
  StrDict dict_;
};

} // db
} // mmkv

#endif // _MMKV_DB_KVDB_H_
