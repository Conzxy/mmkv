#ifndef _MMKV_UTIL_STR_UTIL_H_
#define _MMKV_UTIL_STR_UTIL_H_

#include <stdarg.h>
#include <assert.h>

#include <string>

namespace mmkv {
namespace util {

void StrCat(std::string& src, char const* format, ...); 

} // util
} // mmkv

#endif