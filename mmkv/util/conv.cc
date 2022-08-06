#include "conv.h"

using namespace kanon;

namespace mmkv {
namespace util {

uint64_t str2memory_usage(StringView str) noexcept {
  char buf[64];

  if (str.size() < 2 || !(str.back() == 'b' || str.back() == 'B')) return -1;

  char indicator = str[str.size()-2];
  size_t int_len = 0;
  int_len = (indicator >= '0' && indicator <= '9') ? str.size() - 1 : str.size() - 2; 
    
  memcpy(buf, str.data(), int_len);
  buf[int_len-2] = 0;

  if (indicator >= 'A' && indicator <= 'Z')
    indicator += 0x20;

  if (str.contains('.')) {
    double d;
    if (!str2double(buf, d))
      return -1;

    if (indicator == 'k')
      return d * (1 << 10);
    else if (indicator == 'm')
      return d * (1 << 20);
    else if (indicator == 'g')
      return d * (1 << 30);
  } else {
    uint64_t res = -1;
    if (!str2u64(buf, res))
      return res;
      
    if (indicator == 'k') {
      return res << 10;
    } else if (indicator == 'm') {
      return res << 20;
    } else if (indicator == 'g') {
      return res << 30;
    } 

    return res;
  }
  return -1;
}

MemoryUsage format_memory_usage(uint64_t usage) noexcept {
  if (usage >= (1 << 30)) {
    return { MU_GB, (double)usage / (1 << 30) };
  } else if (usage >= (1 << 20)) {
    return { MU_MB, (double)usage / (1 << 20) };
  } else if (usage >= (1 << 10)) {
    return { MU_KB, (double)usage / (1 << 10) };
  }

  return { MU_B, (double)usage };
}

} // util
} // mmkv