#include "memory_footprint.h"

using namespace mmkv::util;
using namespace mmkv::algo;

static int DecimalCount(uint64_t n) noexcept {
  int ret = 0;

  for (; n != 0 ; n = n / 10, ++ret);

  return ret;
}

String mmkv::util::GetMemoryStat() {
  String ret;
  char buf[4096];
  ret.reserve(4096);

  ret.append("========== Memory Footprint ==========\n");

  int width = std::max(
      std::max(DecimalCount(g_memstat.memory_usage),
               DecimalCount(g_memstat.allocate_count)), 
      std::max(DecimalCount(g_memstat.reallocate_count),
               DecimalCount(g_memstat.deallocate_count)));

  ::snprintf(buf, sizeof buf, "Memory usage     = %*zu bytes\n", width, g_memstat.memory_usage-4096);
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
