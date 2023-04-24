// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "mmkv/server/config.h"
#include "mmkv/server/option.h"

#include "kanon/util/optional.h"

#include <hklua/env.h>

using namespace mmkv;
using namespace hklua;
using namespace mmkv::server;
using namespace kanon;

#define ERROR_HANDLE                                                           \
  do {                                                                         \
    env.StackDump();                                                           \
    return false;                                                              \
  } while (0)

bool server::ParseLuaConfig(StringArg filename, MmkvConfig &config)
{
  Env env("ConfigParser");
  if (HKLUA_OK != env.DoFile(filename.data())) {
    fprintf(stderr, "Failed to load config\n");
    ERROR_HANDLE;
  }

  env.OpenLibs();

  bool success;
  auto log_method = env.GetGlobalR<char const *>("LogMethod", true, &success);
  if (!success) {
    ERROR_HANDLE;
  }

  if (::strcasecmp(log_method, "none") == 0) {
    config.log_method = LM_NONE;
  } else if (::strcasecmp(log_method, "request") == 0) {
    config.log_method = LM_REQUEST;
  } else {
    ERROR_HANDLE;
  }

  if (!env.GetGlobal("ExpirationCheckCycle", config.expiration_check_cycle)) {
    ERROR_HANDLE;
  }

  if (!env.GetGlobal("RequestLogLocation", config.request_log_location)) {
    ERROR_HANDLE;
  }

  if (!env.GetGlobal("LazyExpiration", config.lazy_expiration)) {
    ERROR_HANDLE;
  }

  char const *replace_policy;
  if (!env.GetGlobal("ReplacePolicy", replace_policy)) {
    ERROR_HANDLE;
  }

  if (::strcasecmp(replace_policy, "none") == 0) {
    config.replace_policy = RP_NONE;
  } else if (::strcasecmp(replace_policy, "lru") == 0) {
    config.replace_policy = RP_LRU;
  } else {
    ERROR_HANDLE;
  }

  char const *max_mem_usage;
  if (!env.GetGlobal("MaxMemoryUsage", max_mem_usage, true)) {
    ERROR_HANDLE;
  }

  std::tie(config.max_memory_usage) = env.CallFunction<Number>(
      "ParseMemoryUsage", 0, &success, true, max_mem_usage);

  if (!success) {
    ERROR_HANDLE;
  }

  if (!env.GetGlobal("EveryShardNum", config.shard_num)) { 
    ERROR_HANDLE;
  }

  if (!env.GetGlobal("ConfigServerEndpoint", config.config_server_endpoint))
    ERROR_HANDLE;

  if (!env.GetGlobal("TrackerEndpoint", config.tracker_endpoint)) {
    ERROR_HANDLE;
  }

  Table data_nodes;
  TableGuard data_nodes_guard(data_nodes);

  if (!env.GetGlobalTable("DataNodes", data_nodes)) { 
    ERROR_HANDLE;
  }

  auto nodes_len = data_nodes.len();
  /* NOTICE: Lua index starts with 1 */
  for (int i = 1; i <= nodes_len; ++i) {
    config.nodes.emplace_back(
        data_nodes.GetFieldR<char const *>(i, true, &success));
    if (!success) {
      ERROR_HANDLE;
    }
  }

  return true;
}
