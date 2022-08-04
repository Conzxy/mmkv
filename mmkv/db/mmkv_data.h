#ifndef _MMKV_ALGO_MMKV_DATA_H_
#define _MMKV_ALGO_MMKV_DATA_H_

#include "data_type.h"

namespace mmkv {
namespace db {

struct MmkvData {
  DataType type;  /** Explain the any_data */
  void* any_data; /** Store any data type */
};

/* This should be the destructor of MMkvData,
 * but I want it be a POD class, and delete
 * it in determinate case.
 * Pros:
 * > Avoid move and delete automatically
 * > No need to check if any_data is nullptr
 * Cons:
 * > Need call it by hand
 * But Only fwe commands need, so it is acceptable.
 */
void DeleteMmkvData(MmkvData &data);

} // db 
} // mmkv

#endif // _MMKV_ALGO_MMKV_DATA_H_
