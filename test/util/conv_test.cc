#include "mmkv/util/conv.h"
#include <gtest/gtest.h>
#include <inttypes.h>

using namespace mmkv::util;

TEST(conv, raw2u32) {
  uint32_t i = 22222;
  char buf[5];
  memcpy(buf, &i, sizeof i);
  buf[4] = 0;
  auto res = raw2u32(buf);

  EXPECT_EQ(res, i);
}

TEST(conv, str2metric) {
  EXPECT_EQ(str2metric("221M"), 221 << 20);
}

TEST(conv, metric2str) {
  char buf[1024];
  metric2str(221 << 20, buf, sizeof buf);
  EXPECT_EQ(!strcmp(buf, "221M"), true);
}

TEST(conv, str2i64) {
  int64_t i = 0;
  char buf[64];
  buf[0] = '-';
  buf[1] = '1';
  buf[2] = 0;
  EXPECT_EQ(str2i64(buf, i), true);
  EXPECT_EQ(i, -1);
  printf("%" PRId64 "\n", i);
}
