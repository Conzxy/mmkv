#ifndef _MMKV_UTIL_MEMORY_UTIL_H_
#define _MMKV_UTIL_MEMORY_UTIL_H_

#include <stdlib.h>
#include <stdint.h>

#include "memory_stat.h"

namespace mmkv {
namespace util {

inline void *Malloc(size_t n) noexcept
{
  void *p = ::malloc(n);
  if (p) {
    memory_stat().allocate_count++;
    memory_stat().memory_usage.Add(n);
  }

  return p;
}

inline void Free(void *p, size_t n) noexcept
{
  memory_stat().deallocate_count++;
  memory_stat().memory_usage.Sub(n);
  ::free(p);
}

inline void *Realloc(void *p, size_t old_size, size_t size) noexcept
{
  auto ret = ::realloc(p, size);

  if (ret || size == 0) {
    memory_stat().reallocate_count++;

    // 即使size < old_size，由于溢出，结果依然正确
    memory_stat().memory_usage.Add(size - old_size);
  }

  return ret;
}

} // namespace util
} // namespace mmkv

#endif // _MMKV_UTIL_MEMORY_UTIL_H_
