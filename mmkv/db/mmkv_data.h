#ifndef _MMKV_ALGO_MMKV_DATA_H_
#define _MMKV_ALGO_MMKV_DATA_H_

#include "data_type.h"

namespace mmkv {
namespace db {

struct MmkvData {
  DataType type;
  void* any_data;
};

} // db 
} // mmkv

#endif // _MMKV_ALGO_MMKV_DATA_H_
