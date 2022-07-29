#include "mmkv/util/conv.h"
#include <gtest/gtest.h>

using namespace mmkv::util;

TEST(conv, raw2u32) {
  uint32_t i = 22222;
  char buf[5];
  memcpy(buf, &i, sizeof i);
  buf[4] = 0;
  auto res = raw2u32(buf);

  EXPECT_EQ(res, i);
}