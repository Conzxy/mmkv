// SPDX-LICENSE-IDENTIFIER: Apache-2.0
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
    case D_MAP:
      return "map";
    case D_SET:
      return "hash set";
    default:
      return "unknown and invalid data type";
  }
}