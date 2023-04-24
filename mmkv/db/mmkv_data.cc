// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "mmkv_data.h"

#include "type.h"
#include "vset.h"

using namespace mmkv::algo;

namespace mmkv {
namespace db {

void DeleteMmkvData(MmkvData &data) {
  switch (data.type) {
    case D_STRING:
      delete (String*)data.any_data;
      break;
    case D_STRLIST:
      delete (StrList*)data.any_data;
      break;
    case D_SORTED_SET:
      delete (Vset*)data.any_data;
      break;
    case D_MAP:
      delete (Map*)data.any_data;
      break;
    case D_SET:
      delete (Set*)data.any_data;
      break;
  }
}

} // db
} // mmkv