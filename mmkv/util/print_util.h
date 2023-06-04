// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_UTIL_PRINT_UTIL_H_
#define _MMKV_UTIL_PRINT_UTIL_H_

#include <stdio.h>
#include <stdarg.h>

#include "tcolor_macro.h"

namespace mmkv {
namespace util {

#define COLOR_PRINTF(color)                                                                        \
  va_list vl;                                                                                      \
  va_start(vl, fmt);                                                                               \
  ::fputs(color, stdout);                                                                          \
  ::vprintf(fmt, vl);                                                                              \
  ::fputs(RESET, stdout);                                                                          \
  va_end(vl);

inline void ErrorPrintf(char const *fmt, ...) noexcept
{
  COLOR_PRINTF(L_RED)
  fflush(stdout);
}

inline void InfoPrintf(char const *fmt, ...) noexcept { COLOR_PRINTF(GREEN); }

inline void WarnPrintf(char const *fmt, ...) noexcept
{
  COLOR_PRINTF(YELLOW);
  fflush(stdout);
}

} // namespace util
} // namespace mmkv

#endif