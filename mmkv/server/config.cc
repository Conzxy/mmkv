// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "config.h"

#include "mmkv/util/conv.h"

#include <hklua/env.h>
#include <kanon/log/logger.h>

using namespace kanon;
using namespace mmkv::util;
using namespace mmkv;
using namespace hklua;
using namespace mmkv::server;

namespace mmkv {
namespace server {

MmkvConfig &mmkv_config()
{
  static MmkvConfig config;
  return config;
}

static StringView log_method2str(LogMethod mtd) noexcept;
static StringView replace_policy2str(ReplacePolicy rp) noexcept;

void PrintMmkvConfig(MmkvConfig const &config)
{
  const auto usage = format_memory_usage(config.max_memory_usage);
  LOG_DEBUG << "==== Mmkv Config Entries =====";
  LOG_DEBUG << "LogMethod = " << log_method2str(config.log_method);
  LOG_DEBUG << "ExpirationCheckCycle = " << config.expiration_check_cycle;
  LOG_DEBUG << "LazyExpiration = " << config.lazy_expiration;
  LOG_DEBUG << "RequestLogLocation = " << config.request_log_location;
  LOG_DEBUG << "ReplacePolicy = " << replace_policy2str(config.replace_policy);
  LOG_DEBUG << "DiagnosticLogDirectory = " << config.diagnostic_log_dir;
  LOG_DEBUG << "MaxMemoryUsage = " << usage.usage << " " << memory_unit2str(usage.unit);
  LOG_DEBUG << "SharderAddress = " << config.sharder_endpoint;
  LOG_DEBUG << "SharderControllerAddress = " << config.shard_controller_endpoint;
  LOG_DEBUG << "ShardNum = " << config.shard_num;
  LOG_DEBUG << "Nodes: ";
  for (size_t i = 0; i < config.nodes.size(); ++i) {
    LOG_DEBUG << "node " << i << ": " << config.nodes[i];
  }
}

#define LM_REQUEST_STR "request"
#define LM_NONE_STR    "none"

static inline StringView log_method2str(LogMethod mtd) noexcept
{
  switch (mtd) {
    case LM_REQUEST:
      return {LM_REQUEST_STR, sizeof(LM_REQUEST_STR)};
    case LM_NONE:
      return {LM_NONE_STR, sizeof(LM_NONE_STR)};
    default:
      assert(false && "Invalid log method");
  }
  return "";
}

static inline StringView replace_policy2str(ReplacePolicy rp) noexcept
{
  switch (rp) {
    case RP_LRU:
      return "lru";
    case RP_NONE:
      return "none";
    default:
      assert(false && "Invalid replacement policy");
  }
  return "";
}

// Allow the config entry is missing
#if 0
#  define ERROR_HANDLE                                                                             \
    do {                                                                                           \
      env.StackDump();                                                                             \
      return false;                                                                                \
    } while (0)
#else
#  define ERROR_HANDLE
#endif

bool ParseMmkvConfig(StringArg filename, MmkvConfig &config)
{
  Env env("ConfigParser");
  if (HKLUA_OK != env.DoFile(filename.data())) {
    fprintf(stderr, "Failed to load config\n");
    env.StackDump();
    return false;
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
  } else if (::strcasecmp(replace_policy, "mru") == 0) {
    config.replace_policy = RP_MRU;
  } else if (::strcasecmp(replace_policy, "lfu") == 0) {
    config.replace_policy = RP_LFU;
  } else {
    ERROR_HANDLE;
  }

  char const *max_mem_usage;
  if (!env.GetGlobal("MaxMemoryUsage", max_mem_usage, true)) {
    ERROR_HANDLE;
  }

  std::tie(config.max_memory_usage) =
      env.CallFunction<Number>("ParseMemoryUsage", 0, &success, true, max_mem_usage);

  if (!success) {
    ERROR_HANDLE;
  }

  if (!env.GetGlobal("ShardControllerEndpoint", config.shard_controller_endpoint)) {
    ERROR_HANDLE;
  }

  if (!env.GetGlobal("SharderEndpoint", config.sharder_endpoint)) {
    ERROR_HANDLE;
  }

  Table      data_nodes;
  TableGuard data_nodes_guard(data_nodes);

  if (!env.GetGlobalTable("DataNodes", data_nodes)) {
    ERROR_HANDLE;
  }

  auto nodes_len = data_nodes.len();
  /* NOTICE: Lua index starts with 1 */
  for (int i = 1; i <= nodes_len; ++i) {
    config.nodes.emplace_back(data_nodes.GetFieldR<char const *>(i, true, &success));
    if (!success) {
      ERROR_HANDLE;
    }
  }

  return true;
}

} // namespace server
} // namespace mmkv
