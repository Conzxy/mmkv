#ifndef _MMKV_UTIL_MEMORY_STAT_H_
#define _MMKV_UTIL_MEMORY_STAT_H_

#include <stdint.h>

namespace mmkv {
namespace util {

// 尽管C++中可以不用typedef ... 
// 因为struct可以直接用名字表示
typedef struct _MemoryStat {
  uint64_t memory_usage;
  uint64_t allocate_count; 
  uint64_t deallocate_count;
  uint64_t reallocate_count;
} MemoryStat;

extern MemoryStat g_memstat;

void MemoryFootPrint() noexcept;

} // util
} // mmkv

#endif // _MMKV_UTIL_MEMORY_STAT_H_
