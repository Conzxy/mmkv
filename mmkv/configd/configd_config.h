#ifndef _MMKV_CONFIGD_CONFIG_H__
#define _MMKV_CONFIGD_CONFIG_H__

#include <string>
#include <kanon/util/arithmetic_type.h>

namespace mmkv {
namespace server {

struct ConfigdConfig {
  std::string shard_controller_endpoint;
  u64         shard_num;

  ConfigdConfig();
};

ConfigdConfig &configd_config() noexcept;

bool ParseConfigdConfig(char const *fname);

void DebugPrintConfigdConfig();

} // namespace server
} // namespace mmkv

#endif
