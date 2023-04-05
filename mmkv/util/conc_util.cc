#include "conc_util.h"
#include "conv.h"

#include "kanon/util/platform_macro.h"
#ifdef KANON_ON_UNIX
#  include <unistd.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <thread>

using namespace mmkv::util;

int mmkv::util::GetCoreNum()
{
#ifdef KANON_ON_WIN
  static int ret = 1;
#endif
#ifdef _SC_NPROCESSORS_ONLN
  static ret = sysconf(_SC_NPROCESSORS_ONLN);
#endif

  return ret;
}
