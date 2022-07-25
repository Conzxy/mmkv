#ifndef _MMKV_UTIL_TIME_UTIL_H_
#define _MMKV_UTIL_TIME_UTIL_H_

#include <sys/time.h>
#include <stddef.h>
#include <stdint.h>

namespace mmkv {
namespace util {

inline int64_t GetTimeUs() noexcept {
  struct timeval tv;
  if (::gettimeofday(&tv, NULL) < 0) return -1;

  return tv.tv_sec * 1000000 + tv.tv_usec;
}

inline int64_t GetTimeMs() noexcept {
  return GetTimeUs() / 1000;
}

inline double GetTimeFloatSec() noexcept {
  struct timeval tv;
  if (::gettimeofday(&tv, NULL) < 0) return -1.0;

  return tv.tv_sec + (double)(tv.tv_usec) / 1000000;
}

} // util
} // mmkv

#endif // _MMKV_UTIL_TIME_UTIL_H_
