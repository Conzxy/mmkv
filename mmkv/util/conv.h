#ifndef _MMKV_UTIL_CONV_H_
#define _MMKV_UTIL_CONV_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stddef.h>

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

inline bool str2double(char const *buf, double &d) noexcept {
  char *end = NULL;
  auto res = strtod(buf, &end);
  if (res == 0.0 && end == buf)
    return false;
  d = res;
  return true;
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

enum MemoryUnit : uint8_t {
  MU_B = 0,
  MU_KB,
  MU_MB,
  MU_GB,
  MU_INVALID,
};

uint64_t str2memory_usage(StringView str) noexcept;

inline char const *memory_unit2str(MemoryUnit mu) noexcept {
  switch (mu) {
    case MU_B:
      return "B";
    case MU_KB:
      return "KB";
    case MU_MB:
      return "MB";
    case MU_GB:
      return "GB";
  }
  return "Unknown Unit";
}

struct MemoryUsage {
  MemoryUnit unit;
  double usage;
};

MemoryUsage format_memory_usage(uint64_t usage) noexcept;

} // util
} // mmkv

#endif // _MMKV_UTIL_CONV_H_
