#ifndef _MMKV_DB_KVDB_H_
#define _MMKV_DB_KVDB_H_

#include "mmkv/algo/libc_allocator_with_realloc.h"
#include "mmkv/algo/string.h"

#include "mmkv/algo/dictionary.h"
#include "mmkv/algo/key_value.h"
#include "mmkv/protocol/mmbp.h"

#include <kanon/algo/forward_list.h>

namespace mmkv {
namespace db {

using algo::String;
using algo::Dictionary;
using protocol::StrValues;

class MmkvDb {
  using StrDict = Dictionary<String, String>;
  using StrKvType = StrDict::value_type;
  
  using List = zstl::ForwardList<String, algo::LibcAllocatorWithRealloc<String>>;  
  using ListDict = algo::Dictionary<String, List>;

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
  
  // bool ListAdd(String k, StrValues& elems);  
  // bool ListAppend(String const& k, StrValues& elems);
  // bool ListPrepend(String const& k, StrValues& elmes);
  // size_t ListGetSize(String const& k);
  // StrValues ListGetAll(String const& k);
  // StrValues ListGetRange(String const& k, size_t l, size_t r);
  // bool ListPopFront(String const& k);
  // bool ListPopBack(String const& k);
  // bool ListDel(String const& k);
 private:
  StrDict dict_;
  ListDict list_dict_;

};

} // db
} // mmkv

#endif // _MMKV_DB_KVDB_H_
