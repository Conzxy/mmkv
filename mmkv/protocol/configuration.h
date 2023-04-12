// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef MMKV_PROTOCOL_CONFIGURATION_H__
#define MMKV_PROTOCOL_CONFIGURATION_H__

#include <unordered_map>

#include "mmbp.h"
#include "mmbp_util.h"

namespace mmkv {
namespace protocol {

struct NodeInfo {
  // All fields are required
  std::vector<uint32_t> shard_ids;
  std::string host;
  uint16_t port;
};

MMKV_INLINE void ParseComponent(NodeInfo &info, Buffer &buffer) noexcept
{
  ParseComponent(info.shard_ids, buffer);
  ParseComponent(info.host, buffer);
  ParseComponent(info.port, buffer);
}

MMKV_INLINE void SerializeComponent(NodeInfo const &info,
                                    ChunkList &buffer) noexcept
{
  SerializeComponent(info.shard_ids, buffer);
  SerializeComponent(info.host, buffer);
  SerializeComponent(info.port, buffer);
}

struct Configuration {
  std::unordered_map<uint32_t, NodeInfo> node_map;

  Configuration() = default;
};

MMKV_INLINE void ParseComponent(Configuration &conf, Buffer &buffer) noexcept
{
  ParseComponent(conf.node_map, buffer);
}

MMKV_INLINE void SerializeComponent(Configuration const &conf,
                                    ChunkList &buffer) noexcept
{
  SerializeComponent(conf.node_map, buffer);
}

} // namespace protocol
} // namespace mmkv

#endif