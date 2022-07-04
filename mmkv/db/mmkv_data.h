#ifndef _MMKV_ALGO_MMKV_DATA_H_
#define _MMKV_ALGO_MMKV_DATA_H_

#include <stdint.h>

namespace mmkv {
namespace db {

enum DataType : uint8_t {
  D_STRING = 0,
  D_STRLIST,
};

struct MmkvData {
  DataType type;
  void* any_data;
};

} // db 
} // mmkv

#endif // _MMKV_ALGO_MMKV_DATA_H_
