#ifndef _MMKV_UTIL_MEMORY_STAT_H_
#define _MMKV_UTIL_MEMORY_STAT_H_

#include <stdint.h>
#include <kanon/thread/atomic_counter.h>

namespace mmkv {
namespace util {

using kanon::AtomicCounter64;;

// 尽管C++中可以不用typedef ... 
// 因为struct可以直接用名字表示
typedef struct _MemoryStat {
  AtomicCounter64 memory_usage;
  uint64_t allocate_count = 0; 
  uint64_t deallocate_count = 0;
  uint64_t reallocate_count = 0;
} MemoryStat;

MemoryStat &memory_stat();

} // util
} // mmkv

#endif // _MMKV_UTIL_MEMORY_STAT_H_
