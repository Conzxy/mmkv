#ifndef _MMKV_UTIL_MEMORY_FOOTPRINT_H_
#define _MMKV_UTIL_MEMORY_FOOTPRINT_H_

#include "mmkv/algo/string.h"

namespace mmkv {
namespace util {

void MemoryFootPrint() noexcept;

algo::String GetMemoryStat();

} // util
} // mmkv

#endif // _MMKV_UTIL_MEMORY_FOOTPRINT_H_
