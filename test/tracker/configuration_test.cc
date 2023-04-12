#include "mmkv/protocol/configuration.h"

#include <gtest/gtest.h>

using namespace ::mmkv::protocol;

TEST(configuration, ParseAndSerialize)
{
  Configuration conf;
  NodeInfo node_info;
  node_info.shard_ids.assign({1, 2, 3});
  node_info.host = "127.0.0.1";
  node_info.port = 1289;
  conf.node_map[1] = std::move(node_info);

  Buffer ibuffer;
  ChunkList obuffer;

  SerializeComponent(conf, obuffer);

  for (auto const &chunk : obuffer) {
    ibuffer.Append(chunk.ToStringView());
  }

  Configuration o_conf;
  ParseComponent(o_conf, ibuffer);
  auto &node_map = o_conf.node_map;
  for (auto const &kv : node_map) {
    EXPECT_EQ(kv.first, 1);
    EXPECT_EQ(kv.second.host, conf.node_map[kv.first].host);
    EXPECT_EQ(kv.second.port, conf.node_map[kv.first].port);
    EXPECT_EQ(kv.second.shard_ids, conf.node_map[kv.first].shard_ids);
  }
}
