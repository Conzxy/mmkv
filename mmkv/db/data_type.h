#ifndef _MMKV_DB_DATA_TYPE_H_
#define _MMKV_DB_DATA_TYPE_H_

#include <stdint.h>

namespace mmkv {
namespace db {

enum DataType : uint8_t {
  D_STRING = 0,
  D_STRLIST,
  D_SORTED_SET,
};

char const* GetDataTypeString(DataType type) noexcept;

} // db
} // mmkv

#endif // 