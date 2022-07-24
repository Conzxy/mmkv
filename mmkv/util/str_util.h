#ifndef _MMKV_UTIL_STR_UTIL_H_
#define _MMKV_UTIL_STR_UTIL_H_

#include <stdarg.h>
#include <assert.h>

#include <string>

namespace mmkv {
namespace util {

inline void StrCat(std::string& src, char const* format, ...) {
  va_list vl;
  va_start(vl, format);

  char c = 0;

  bool persent_ch = false;

  while (( c = *(format++) ) ) {
    switch (c) {
      case '%':
        if (persent_ch) {
          src += '%';
          persent_ch = false;
        }
        else persent_ch = true;
        break;
      case 'a':
        src += va_arg(vl, char const*);
        persent_ch = false;
        break;
      case 's':
        src += *va_arg(vl, std::string*);
        persent_ch = false;
        break;
      default:
        src += c;
        persent_ch = false;
    }

  }

  va_end(vl);
}

} // util
} // mmkv

#endif