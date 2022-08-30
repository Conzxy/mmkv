#include "memory_stat.h"

#include <algorithm>
#include <stdio.h>

using namespace mmkv::util;
using namespace kanon;

MemoryStat &mmkv::util::memory_stat() {
  static MemoryStat stat;
  return stat;
}