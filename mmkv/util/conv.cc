#include "conv.h"

using namespace kanon;

uint64_t mmkv::util::str2memory_usage(StringView str) noexcept {
  char buf[64];
  uint64_t res = -1;

  if (!(str.back() == 'b' || str.back() == 'B')) return -1;

  memcpy(buf, str.data(), str.size()-2);
  buf[str.size()-2] = 0;
  if (!str2u64(buf, res))
    return res;
    
  char indicator = str[str.size()-2];
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