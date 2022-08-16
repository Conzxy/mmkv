#ifndef _MMKV_SERVER_CONFIG_H_
#define _MMKV_SERVER_CONFIG_H_

#include <stdint.h>
#include <string>

#include "mmkv/util/shard_util.h"

namespace mmkv {
namespace server {

/** The method to log database data to somewhere or not */
enum LogMethod : uint8_t {
  LM_REQUEST = 0, /** Log request to file */
  LM_NONE,        /** Do nothing */
};

/** The policy to replace key when maximum allowed memory usage is reached */
enum ReplacePolicy : uint8_t {
  RP_LRU = 0,     /** Least-recently-used */
  RP_NONE,
};

struct MmkvConfig {
  /* Set defalut value in case the field is not found in config file.
   * (Not found is also valid, use its default value)
   */
  LogMethod log_method = LM_NONE;
  ReplacePolicy replace_policy = RP_NONE;
  bool lazy_expiration = false;
  uint64_t max_memory_usage = 0;
  long expiration_check_cycle = 0;
  std::string request_log_location = "/tmp/.mmkv-request.log";
  std::string diagnostic_log_dir = "";
  std::string router_address = "127.0.0.1:9997";
  long shard_num = DEFAULT_SHARD_NUM;
  int router_port = 9997;
  int tracker_port = router_port + 10000;
  int sharder_port = 9998;
  std::vector<std::string> nodes;
};

extern MmkvConfig g_config;

void RegisterConfig(MmkvConfig &config);
bool ParseConfig(std::string &errmsg);
void PrintMmkvConfig(MmkvConfig const &config);
bool inline IsExpirationDisable() noexcept {
  return !g_config.lazy_expiration && g_config.expiration_check_cycle <= 0;
}

/* If the address of router exists,
 * the server split keys into shards
 */
bool inline IsSharder() noexcept {
  return !g_config.router_address.empty();
}

} // namespace server
} // namespace mmkv

#endif
