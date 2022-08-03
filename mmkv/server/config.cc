#include "config.h"

#include "option.h"
#include "mmkv/util/time_util.h"
#include "mmkv/util/conv.h"

#include <chisato.h>
#include <kanon/log/logger.h>

using namespace kanon;
using namespace mmkv::util;

namespace mmkv {
namespace server {

MmkvConfig g_config;

static StringView log_method2str(LogMethod mtd) noexcept;
static StringView replace_policy2str(ReplacePolicy rp) noexcept;

static bool set_log_method(StrSlice value, void *args) noexcept;
static bool set_replace_policy(StrSlice value, void *args) noexcept;
static bool set_max_memoey_usage(StrSlice vlaue, void *args) noexcept;

void RegisterConfig(MmkvConfig &config) {
  chisato::AddConfig("LogMethod", &config.log_method, &set_log_method);
  chisato::AddConfig("ExpirationCheckCycle", &config.expiration_check_cycle); 
  chisato::AddConfig("RequestLogLocation", &config.request_log_location);
  chisato::AddConfig("LazyExpiration", &config.lazy_expiration);
  chisato::AddConfig("ReplacePolicy", &config.replace_policy, &set_replace_policy);
  chisato::AddConfig("DiagnosticLogDirectory", &config.diagnostic_log_dir);
  chisato::AddConfig("MaxMemoryUsage", &config.max_memory_usage, &set_max_memoey_usage);
}

bool ParseConfig(std::string &errmsg) {
  auto start_time = GetTimeMs();
  auto success = chisato::Parse(g_option.config_name, errmsg); 

  if (!success) return false;
  if (g_config.request_log_location.empty()) {
    LOG_ERROR << "The RequestLogLocation field in the config file is missing";
    return false;
  }
  
  LOG_INFO << "Parse config cost: " << (GetTimeMs() - start_time) << "ms";

  return true;
}

void PrintMmkvConfig(MmkvConfig const &config) {
  const auto usage = format_memory_usage(config.max_memory_usage);
  LOG_DEBUG << "Config information: \n"
    << "LogMethod=" << log_method2str(config.log_method)
    << "\nExpirationCheckCycle=" << config.expiration_check_cycle
    << "\nLazyExpiration=" << config.lazy_expiration
    << "\nRequestLogLocation=" << config.request_log_location
    << "\nReplacePolicy=" << replace_policy2str(config.replace_policy)
    << "\nDiagnosticLogDirectory=" << config.diagnostic_log_dir
    << "\nMaxMemoryUsage=" << usage.usage << " " << memory_unit2str(usage.unit);
}

#define LM_REQUEST_STR "request"
#define LM_NONE_STR "none"

static inline StringView log_method2str(LogMethod mtd) noexcept {
  switch (mtd) {
    case LM_REQUEST:
      return { LM_REQUEST_STR, sizeof(LM_REQUEST_STR) };
    case LM_NONE:
      return { LM_NONE_STR, sizeof(LM_NONE_STR) };
    default:
      assert(false && "Invalid log method");
  }
  return "";
}

static inline bool set_log_method(StrSlice value, void *args) noexcept {
  auto log_method = (LogMethod*)args;
  if (!value.caseCmp("request")) {
    *log_method = LM_REQUEST;
  } else if (!value.caseCmp("none")) {
    *log_method = LM_NONE;
  } else {
    return false;
  }
  return true;
}

static inline StringView replace_policy2str(ReplacePolicy rp) noexcept {
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

static inline bool set_replace_policy(StrSlice value, void *args) noexcept {
  auto rp = (ReplacePolicy*)args;
  if (!value.caseCmp("lru")) {
    *rp = RP_LRU; 
  } else if (!value.caseCmp("none")) {
    *rp = RP_NONE;
  } else {
    return false;
  }
  return true;
}

static inline bool set_max_memoey_usage(StrSlice value, void *args) noexcept {
  auto max_memory_usage = (uint64_t*)args;
  *max_memory_usage = str2memory_usage({value.data(), value.size()});
  if (*max_memory_usage == (uint64_t)-1)
    return false;
  return true;
}

} // server
} // mmkv
