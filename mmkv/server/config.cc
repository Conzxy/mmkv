// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "config.h"

#include "option.h"
#include "mmkv/util/time_util.h"
#include "mmkv/util/conv.h"
#include "mmkv/util/tokenizer.h"

#include <chisato.h>
#include <kanon/log/logger.h>

using namespace kanon;
using namespace mmkv::util;

namespace mmkv {
namespace server {

MmkvConfig &mmkv_config()
{
  static MmkvConfig config;
  return config;
}

static StringView log_method2str(LogMethod mtd) noexcept;
static StringView replace_policy2str(ReplacePolicy rp) noexcept;

static bool set_log_method(StrSlice value, void *args) noexcept;
static bool set_replace_policy(StrSlice value, void *args) noexcept;
static bool set_max_memoey_usage(StrSlice vlaue, void *args) noexcept;
static bool set_nodes(StrSlice value, void *args);

void RegisterConfig(MmkvConfig &config)
{
  chisato::AddConfig("LogMethod", &config.log_method, &set_log_method);
  chisato::AddConfig("ExpirationCheckCycle", &config.expiration_check_cycle);
  chisato::AddConfig("RequestLogLocation", &config.request_log_location);
  chisato::AddConfig("LazyExpiration", &config.lazy_expiration);
  chisato::AddConfig("ReplacePolicy", &config.replace_policy, &set_replace_policy);
  chisato::AddConfig("DiagnosticLogDirectory", &config.diagnostic_log_dir);
  chisato::AddConfig("MaxMemoryUsage", &config.max_memory_usage, &set_max_memoey_usage);
  chisato::AddConfig("ShardNum", &config.shard_num);
  chisato::AddConfig("Nodes", &config.nodes, &set_nodes);
}

bool ParseConfig(std::string &errmsg)
{
  auto start_time = GetTimeMs();
  auto success    = chisato::Parse(mmkv_option().config_name, errmsg);

  if (!success) return false;
  if (mmkv_config().request_log_location.empty()) {
    LOG_ERROR << "The RequestLogLocation field in the config file is missing";
    return false;
  }

  LOG_INFO << "Parse config cost: " << (GetTimeMs() - start_time) << "ms";

  return true;
}

void PrintMmkvConfig(MmkvConfig const &config)
{
  const auto usage = format_memory_usage(config.max_memory_usage);
  LOG_DEBUG << "Config information: \n";
  LOG_DEBUG << "LogMethod=" << log_method2str(config.log_method);
  LOG_DEBUG << "ExpirationCheckCycle=" << config.expiration_check_cycle;
  LOG_DEBUG << "LazyExpiration=" << config.lazy_expiration;
  LOG_DEBUG << "RequestLogLocation=" << config.request_log_location;
  LOG_DEBUG << "ReplacePolicy=" << replace_policy2str(config.replace_policy);
  LOG_DEBUG << "DiagnosticLogDirectory=" << config.diagnostic_log_dir;
  LOG_DEBUG << "MaxMemoryUsage=" << usage.usage << " " << memory_unit2str(usage.unit);
  LOG_DEBUG << "SharderAddress=" << config.sharder_endpoint;
  LOG_DEBUG << "SharderControllerAddress=" << config.shard_controller_endpoint;
  LOG_DEBUG << "ShardNum=" << config.shard_num;
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

static inline bool set_log_method(StrSlice value, void *args) noexcept
{
  auto log_method = (LogMethod *)args;
  if (!value.caseCmp("request")) {
    *log_method = LM_REQUEST;
  } else if (!value.caseCmp("none")) {
    *log_method = LM_NONE;
  } else {
    return false;
  }
  return true;
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

static inline bool set_replace_policy(StrSlice value, void *args) noexcept
{
  auto rp = (ReplacePolicy *)args;
  if (!value.caseCmp("lru")) {
    *rp = RP_LRU;
  } else if (!value.caseCmp("none")) {
    *rp = RP_NONE;
  } else {
    return false;
  }
  return true;
}

static inline bool set_max_memoey_usage(StrSlice value, void *args) noexcept
{
  auto max_memory_usage = (uint64_t *)args;
  *max_memory_usage     = str2memory_usage({value.data(), value.size()});
  if (*max_memory_usage == (uint64_t)-1) return false;
  return true;
}

static inline bool set_nodes(StrSlice value, void *args)
{
  std::vector<std::string> nodes;
  Tokenizer                tokens(StringView(value.data(), value.size()), ',');

  // FIXME Valid check
  for (auto token : tokens) {
    nodes.emplace_back(token.ToString());
  }
  return true;
}

} // namespace server
} // namespace mmkv
