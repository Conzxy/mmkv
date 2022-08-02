#ifndef _MMKV_UTIL_CONV_H_
#define _MMKV_UTIL_CONV_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <kanon/net/endian_api.h>
#include <kanon/string/string_view.h>

namespace mmkv {
namespace util {

using kanon::StringView;

inline double int2double(uint64_t i) noexcept {
  return *(double*)&i;
}

inline uint64_t double2u64(double d) noexcept {
  return *(uint64_t*)&d;
}

inline int64_t double2i64(double d) noexcept {
  return *(uint64_t*)&d;
}

inline uint32_t raw2u32(char const *buf) noexcept {
  return *(uint32_t*)buf;
}

inline uint64_t raw2u64(char const *buf) noexcept {
  return *(uint64_t*)buf;
}

inline bool str2u64(char const *buf, uint64_t &i) noexcept {
  char *end = NULL;
  auto res = strtoull(buf, &end, 10);
  if (res == 0 && end == buf)
    return false;
  
  i = res;
  return true;
}

inline bool str2i64(char const *buf, int64_t &i) noexcept {
  char *end = NULL;
  auto res = strtoll(buf, &end, 10);
  if (res == 0 && end == buf)
    return false;
  
  i = res;
  return true;
}

inline uint64_t str2metric(StringView str) noexcept {
  char buf[64];
  uint64_t res = -1;
  
  memcpy(buf, str.data(), str.size()-1);
  buf[str.size()] = 0;
  if (!str2u64(buf, res))
    return res;
    
  char indicator = str.back();
  if (indicator >= 'A' && indicator <= 'Z')
    indicator += 0x20;

  if (indicator == 'k') {
    return res << 10;
  } else if (indicator == 'm') {
    return res << 20;
  } else if (indicator == 'g') {
    return res << 30;
  } 

  return res;
}

inline void metric2str(uint64_t metric, char *buf,  size_t n) noexcept {
  char unit = 0;

  if (metric >= 1 << 30) {
    unit = 'G';
    metric >>= 30;
  } else if (metric >= 1 << 20) {
    unit = 'M';
    metric >>= 20;
  } else if (metric >= 1 << 10) {
    unit = 'K';
    metric >>= 10;
  }
  snprintf(buf, n, "%" PRIu64 "%c", metric, unit);
}

} // util
} // mmkv

#endif // _MMKV_UTIL_CONV_H_
