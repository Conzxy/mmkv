#ifndef _MMKV_DB_KVDB_H_
#define _MMKV_DB_KVDB_H_

#include <string>

#include "mmkv/algo/dictionary.h"
#include "mmkv/algo/key_value.h"

namespace mmkv {
namespace db {

class MmkvDb {
  using StrDict = algo::Dictionary<std::string, std::string>;
  using StrKvType = StrDict::value_type;

 public:
  MmkvDb();
  ~MmkvDb() noexcept;
  
  int InsertStr(std::string k, std::string v) {
    return dict_.InsertKv(std::move(k), std::move(v)) ? 1 : 0;
  }
  
  int EraseStr(std::string const& k) {
    return dict_.Erase(k);
  }
  
  StrKvType* GetStr(std::string const& k) noexcept {
    return dict_.Find(k);
  }

 private:
  StrDict dict_;
};

} // db
} // mmkv

#endif // _MMKV_DB_KVDB_H_
