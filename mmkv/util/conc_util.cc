#include "conc_util.h"
#include "conv.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <thread>

using namespace mmkv::util;

int mmkv::util::GetCoreNum() {
#ifdef _SC_NPROCESSORS_ONLN
  static int ret = sysconf(_SC_NPROCESSORS_ONLN);
#endif

  return ret;
}
