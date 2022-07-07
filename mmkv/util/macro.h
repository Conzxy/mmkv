#ifndef _MMKV_UTIL_MACRO_H_
#define _MMKV_UTIL_MACRO_H_

#include <stdlib.h>

#define MMKV_ASSERT(cond, msg) \
  assert((cond) && (msg))

#define MMKV_UNUSED(x) (void)x

// To prvalue, this is ill-formed
#define MMKV_INT2DOUBLE(i) (*(double*)&i)
#define MMKV_DOUBLE2INT(d) (*(int64_t*)&d)

#endif // _MMKV_UTIL_MACRO_H_
