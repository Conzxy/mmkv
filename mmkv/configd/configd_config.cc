#include "configd_config.h"

#include <kanon/log/logger.h>

#include "hklua/env.h"

using namespace hklua;

namespace mmkv {
namespace server {

ConfigdConfig::ConfigdConfig()
{
  shard_controller_endpoint = "*:19997";
  shard_num                 = 4096;
}

ConfigdConfig &configd_config() noexcept
{
  static ConfigdConfig config;
  return config;
}

bool ParseConfigdConfig(char const *filename)
{
  Env env("ConfigParser");
  if (HKLUA_OK != env.DoFile(filename)) {
    fprintf(stderr, "Failed to load config\n");
    env.StackDump();
    return false;
  }

  env.OpenLibs();

  auto &config = configd_config();

  env.GetGlobal("ShardControllerEndpoint", config.shard_controller_endpoint);
  env.GetGlobal("ShardNum", config.shard_num);

  return true;
}

void DebugPrintConfigdConfig()
{
  auto const &config = configd_config();
  LOG_DEBUG << "ShardControllerEndpoint = " << config.shard_controller_endpoint;
  LOG_DEBUG << "ShardNum = " << config.shard_num;
}

} // namespace server
} // namespace mmkv
