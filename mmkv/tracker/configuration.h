#ifndef MMKV_TRACKER_CONFIGURATION_H_
#define MMKV_TRACKER_CONFIGURATION_H_

#include <string>
#include <vector>

#include "type.h"

namespace mmkv {

/**
 * We call the distribution of shards as a 
 * configuration
 */

struct ConfigurationEntry {
  Node node;
  std::vector<Shard> shards;
  std::string addr;
  uint16_t port;
};

using Configuration = std::vector<ConfigurationEntry>;

void PrintConfiguartion(Configuration const&config);

} // namespace mmkv

#endif
