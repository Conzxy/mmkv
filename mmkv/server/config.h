#ifndef _MMKV_SERVER_CONFIG_H_
#define _MMKV_SERVER_CONFIG_H_

#include <stdint.h>

#include <string>

namespace mmkv {
namespace server {

enum LogMethod : uint8_t {
  LM_REQUEST = 0,
  LM_NONE,
};

struct MmkvConfig {
  LogMethod log_method = LM_NONE;
  uint64_t expiration_check_cycle;
  std::string request_log_location;
};

extern MmkvConfig g_config;

} // namespace config
} // namespace mmkv

#endif