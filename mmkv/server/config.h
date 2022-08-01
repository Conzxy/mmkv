#ifndef _MMKV_SERVER_CONFIG_H_
#define _MMKV_SERVER_CONFIG_H_

#include <stdint.h>
#include <string>

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
  long expiration_check_cycle = 0;
  std::string request_log_location = "/tmp/.mmkv-request.log";
};

extern MmkvConfig g_config;

bool ParseConfig(std::string& path);
void PrintMmkvConfig(MmkvConfig const &config);

} // namespace server
} // namespace mmkv

#endif
