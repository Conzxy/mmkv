#include "mmkv/server/config.h"

#include <gtest/gtest.h>

using namespace mmkv::server;

TEST (lua_config, main)
{
  MmkvConfig config;
  
  ASSERT_TRUE(ParseLuaConfig("/home/conzxy/mmkv/test/lua/config_test.lua", config));
  EXPECT_TRUE(config.log_method == mmkv::server::LM_NONE);
  EXPECT_EQ(config.request_log_location, "/tmp/.mmkv-request.log");
  EXPECT_EQ(config.expiration_check_cycle, 0);
  EXPECT_EQ(config.lazy_expiration, true);
  EXPECT_EQ(config.replace_policy, RP_NONE);
  EXPECT_EQ(config.max_memory_usage, size_t(100.11 * (1 << 20)));
  EXPECT_EQ(config.shard_num, 4096);
  EXPECT_EQ(config.config_server_endpoint, "0.0.0.0:9997");
  EXPECT_EQ(config.tracker_endpoint, "0.0.0.0:19997");
  
  int i = 1;
  for (auto const &node : config.nodes) {
    EXPECT_EQ(node, std::to_string(i++));
  }
}
