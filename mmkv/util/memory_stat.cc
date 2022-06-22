#include "memory_stat.h"

#include <algorithm>
#include <stdio.h>

using namespace mmkv::util;

MemoryStat mmkv::util::g_memstat = MemoryStat {
  .memory_usage = 0,
  .allocate_count = 0,
  .deallocate_count = 0,
  .reallocate_count = 0
};

static int DecimalCount(uint64_t n) noexcept {
  int ret = 0;

  for (; n != 0 ; n = n / 10, ++ret);

  return ret;
}

void mmkv::util::MemoryFootPrint() noexcept {
  ::printf("===== Memory FootPrint =====\n");
  int width = std::max(
      std::max(DecimalCount(g_memstat.memory_usage),
               DecimalCount(g_memstat.allocate_count)), 
      std::max(DecimalCount(g_memstat.reallocate_count),
               DecimalCount(g_memstat.deallocate_count)));
  ::printf("Memory usage     = %*zu bytes\n", width, g_memstat.memory_usage);
  ::printf("Allocate count   = %*zu times\n", width, g_memstat.allocate_count);
  ::printf("Deallocate count = %*zu times\n", width, g_memstat.deallocate_count);
  ::printf("Reallocate count = %*zu times\n", width, g_memstat.reallocate_count);
  ::printf("============================\n");
}
