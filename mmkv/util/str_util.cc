#include "str_util.h"

#include <string.h>

namespace mmkv {
namespace util {

void StrCat(std::string& src, char const* format, ...) {
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
      case 'a': {
        if (!persent_ch) goto ch_append;
        auto str = va_arg(vl, char const*);
        src.append(str, strlen(str));
        persent_ch = false;
      }
        break;
      case 's':
        if (!persent_ch) goto ch_append;
        src += *va_arg(vl, std::string*);
        persent_ch = false;
        break;
      default:
ch_append:
        src += c;
        persent_ch = false;
    }

  }

  va_end(vl);
}

} // util
} // mmkv