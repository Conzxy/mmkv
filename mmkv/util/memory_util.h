#ifndef _MMKV_UTIL_MEMORY_UTIL_H_
#define _MMKV_UTIL_MEMORY_UTIL_H_

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "memory_stat.h"

namespace mmkv {
namespace util {

inline void* Malloc(size_t n) noexcept {
  void* p = ::malloc(n);
  if (p) {
    g_memstat.allocate_count++;
    g_memstat.memory_usage += n;
  }

  return p;
}

inline void Free(void* p, size_t n) noexcept {
  g_memstat.deallocate_count++;
  g_memstat.memory_usage -= n;
  ::free(p);
}

inline void* Realloc(void* p, size_t old_size, size_t size) noexcept {
  auto ret = ::realloc(p, size);

  if (ret || size == 0) {
    g_memstat.reallocate_count++;
  
    // 即使size < old_size，由于溢出，结果依然正确
    g_memstat.memory_usage += size - old_size;
  }

  return ret;
}

} // util
} // mmkv

#endif // _MMKV_UTIL_MEMORY_UTIL_H_
