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

