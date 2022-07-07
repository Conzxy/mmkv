#include "data_type.h"

using namespace mmkv::db;

char const* mmkv::db::GetDataTypeString(DataType type) noexcept {
  switch (type) {
    case D_STRING:
      return "string";
    case D_STRLIST:
      return "list";
    case D_SORTED_SET:
      return "vset";
    default:
      return "unknown and invalid data type";
  }
}