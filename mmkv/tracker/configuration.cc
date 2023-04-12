#include "configuration.h"

#include <algorithm>
#include <assert.h>

using namespace mmkv;

static std::string FormatShardString(std::vector<Shard> const &shards)
{
  std::string ret;
  if (shards.size() > 1) {
    size_t index = 1;
    Shard last_shard = shards[0];

    for (; index < shards.size(); ++index) {
      if (shards[index] - shards[index - 1] != 1) {
        ret += std::to_string(last_shard);
        ret += "~";
        ret += std::to_string(shards[index - 1]);
        ret += ",";
        last_shard = shards[index];
      }
    }

    if (last_shard == shards[0]) {
      ret += std::to_string(last_shard);
      ret += "~";
      ret += std::to_string(shards.back());
    }
  } else if (shards.size() == 1) {
    ret = std::to_string(shards.front());
  } else {
    assert(shards.empty());
  }

  if (ret.back() == ',') ret.pop_back();
  ret += "(";
  ret += std::to_string(shards.size());
  ret += ")";

  return ret;
}

void mmkv::PrintConfiguartion(Configuration const &config)
{
  auto size = config.size();
  printf("%-6s %-22s %s\n", "Node", "Address", "Shards");
  char address[64];
  std::string shards_str;
  for (size_t i = 0; i < size; ++i) {
    snprintf(address, sizeof address, "%s;%i", config[i].addr.c_str(),
             (int)config[i].port);
    shards_str = FormatShardString(config[i].shards);
    printf("%-6u %-22s %s\n", config[i].node, address, shards_str.c_str());
    shards_str.clear();
  }
}
