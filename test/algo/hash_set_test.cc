#define _DEBUG_HASH_TABLE_

#include "mmkv/algo/hash_set.h"

#include <gtest/gtest.h>

using namespace mmkv::algo;

class HashSetTest : public ::testing::Test {
 protected:
  void SetUp() override {
  }

  void TearDown() override {

  }

  HashSet<int> hset;
};

TEST_F(HashSetTest, insert) {
  for (int i = 0; i < 100; ++i) {
    printf("===== insert key = %d =====\n", i);
    auto k = hset.Insert(i);
    ASSERT_TRUE(k);
    EXPECT_TRUE(*k == i);
    EXPECT_FALSE(hset.Insert(i));
    hset.DebugPrint();
  }
}

TEST_F(HashSetTest, find) {
  for (int i = 0; i < 100; ++i) { 
    EXPECT_TRUE(hset.Insert(i));
  }

  hset.DebugPrint();

  for (int i = 0; i < 100; ++i) {
    auto vp = hset.Find(i);
    hset.DebugPrint();

    ASSERT_TRUE(vp != nullptr);

    EXPECT_EQ(*vp, i);
  }
}

TEST_F(HashSetTest, erase) {
  for (int i = 0; i < 100; ++i) { 
    EXPECT_TRUE(hset.Insert(i));
  }

  for (int i = 0; i < 100; ++i) {
    EXPECT_EQ(hset.Erase(i), 1);
    EXPECT_EQ(hset.Erase(i), 0);
    EXPECT_EQ(hset.Find(i), nullptr);
    hset.DebugPrint();
  }
  
  ASSERT_TRUE(hset.empty());
}

