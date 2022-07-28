#ifndef _MMKV_ALGO_MMKV_DATA_H_
#define _MMKV_ALGO_MMKV_DATA_H_

#include "data_type.h"

namespace mmkv {
namespace db {

struct MmkvData {
  DataType type;  /** Explain the any_data */
  void* any_data; /** Store any data type */
};

} // db 
} // mmkv

#endif // _MMKV_ALGO_MMKV_DATA_H_
