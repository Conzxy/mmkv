#ifndef _MMKV_UTIL_MACRO_H_
#define _MMKV_UTIL_MACRO_H_

#include <stdlib.h>

#define MMKV_ASSERT(cond, msg) \
  assert((cond) && (msg))

#define MMKV_UNUSED(x) (void)x
#endif // _MMKV_UTIL_MACRO_H_
