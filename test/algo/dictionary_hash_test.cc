#include "mmkv/algo/dictionary.h"

#include <gtest/gtest.h>

using namespace mmkv::algo;

TEST(dictionary_test, insert) {
  Dictionary<int, int> dict;

  for (int i = 0; i < 100; ++i) {
    auto kv = dict.InsertKv(i, i);
    ASSERT_TRUE(kv);
    EXPECT_TRUE(kv->key == i);
    EXPECT_TRUE(kv->value == i);
  }

}
