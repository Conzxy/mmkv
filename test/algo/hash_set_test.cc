#include <algorithm>
#include <cstdio>
#define _DEBUG_HASH_TABLE_

#include "mmkv/algo/hash_set.h"

#include <gtest/gtest.h>

using namespace mmkv::algo;

TEST(hash_set_test, ctor) {
  HashSet<int> hset;
  hset.DebugPrint();
}

TEST(hash_set_test, insert) {
  HashSet<int> hset;
  hset.DebugPrint();

  for (int i = 0; i < 100; ++i) {
    printf("===== insert key = %d =====\n", i);
    auto k = hset.Insert(i);
    ASSERT_TRUE(k);
    EXPECT_TRUE(*k == i);
    EXPECT_FALSE(hset.Insert(i));
    hset.DebugPrint();
  }
}

class HashSetTest : public ::testing::Test {
 protected:
  void SetUp() override {
    for (int i = 0; i < 100; ++i) { 
      EXPECT_TRUE(hset.Insert(i));
    }
  }
  
  HashSet<int> hset;
};

TEST_F(HashSetTest, find) {
  for (int i = 0; i < 100; ++i) {
    auto vp = hset.Find(i);

    ASSERT_TRUE(vp != nullptr);

    EXPECT_EQ(*vp, i);
  }

  hset.DebugPrint();
}

TEST_F(HashSetTest, erase) {
  for (int i = 0; i < 100; ++i) {
    EXPECT_EQ(hset.Erase(i), 1);
    EXPECT_EQ(hset.Erase(i), 0);
    EXPECT_EQ(hset.Find(i), nullptr);
    hset.DebugPrint();
  }
  
  ASSERT_TRUE(hset.empty());
}
