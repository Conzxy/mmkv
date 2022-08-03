#include "memory_footprint.h"

#include "conv.h"

using namespace mmkv::util;
using namespace mmkv::algo;

#define MEMORY_STAT_BUF_SIZE 4096
static size_t DecimalCount(uint64_t n) noexcept {
  int ret = 0;

  for (; n != 0 ; n = n / 10, ++ret);

  return ret;
}

String mmkv::util::GetMemoryStat() {
  String ret;
  char buf[MEMORY_STAT_BUF_SIZE];
  ret.reserve(MEMORY_STAT_BUF_SIZE);

  ret.append("========== Memory Footprint ==========\n");
  
  auto memory_usage = format_memory_usage(g_memstat.memory_usage-MEMORY_STAT_BUF_SIZE);
  int width = std::max(
      std::max(DecimalCount(memory_usage.usage),
               DecimalCount(g_memstat.allocate_count)), 
      std::max(DecimalCount(g_memstat.reallocate_count),
               DecimalCount(g_memstat.deallocate_count)));

  ::snprintf(buf, sizeof buf, "Memory usage     = %*zu %s\n", width, memory_usage.usage, memory_unit2str(memory_usage.unit));
  ret.append(buf);
  ::snprintf(buf, sizeof buf, "Allocate count   = %*zu times\n", width, g_memstat.allocate_count);
  ret.append(buf);
  ::snprintf(buf, sizeof buf, "Deallocate count = %*zu times\n", width, g_memstat.deallocate_count);
  ret.append(buf);
  ::snprintf(buf, sizeof buf, "Reallocate count = %*zu times\n", width, g_memstat.reallocate_count);
  ret.append(buf);
  ::snprintf(buf, sizeof buf, "======================================\n");
  ret.append(buf);

  return ret;
}

void mmkv::util::MemoryFootPrint() noexcept {
  printf("%s", GetMemoryStat().c_str());
}
