// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_UTIL_EXCEPTION_MACRO_H_
#define _MMKV_UTIL_EXCEPTION_MACRO_H_

#include <stdexcept>

#define DEFINE_EXCEPTION_FROM_OTHER(exception, other) \
class exception : public other {\
public:\
  explicit exception(std::string const& str)\
    : other(str)\
  { }\
\
  explicit exception(char const* str)\
    : other(str)\
  { }\
}

#endif

