#include "mmkv/replacement/internal/lfu_cache_impl.h"

#include <gtest/gtest.h>

using namespace mmkv::replacement;

TEST(lfu_cache, update_entry)
{
  LfuCache<int> cache(5);
  for (int i = 0; i < 5; ++i) {
    cache.UpdateEntry(i);
    ASSERT_TRUE(cache.Search(i));
    EXPECT_TRUE(*cache.Search(i) == i);
  }

  ASSERT_EQ(!cache.Victim(), 0);
  cache.DelVictim();

  for (int i = 0; i < 4; ++i) {
    cache.UpdateEntry(3);
  }

  for (int i = 0; i < 3; ++i) {
    cache.UpdateEntry(4);
  }

  for (int i = 0; i < 6; ++i) {
    cache.UpdateEntry(5);
  }

  EXPECT_EQ(*cache.Victim(), 1);
  cache.DelVictim();
  EXPECT_EQ(*cache.Victim(), 2);
  cache.DelVictim();
  EXPECT_EQ(*cache.Victim(), 4);
  cache.DelVictim();

  EXPECT_EQ(*cache.Victim(), 3);
  cache.DelVictim();
  EXPECT_EQ(*cache.Victim(), 5);
  cache.DelVictim();
}

TEST(lfu_cache, del_entry)
{
  LfuCache<int> cache(5);
  for (int i = 0; i < 5; ++i) {
    cache.UpdateEntry(i);
    ASSERT_TRUE(cache.Search(i));
    EXPECT_TRUE(*cache.Search(i) == i);
  }

  for (int i = 0; i < 4; ++i) {
    cache.UpdateEntry(3);
  }

  for (int i = 0; i < 3; ++i) {
    cache.UpdateEntry(4);
  }

  for (int i = 0; i < 6; ++i) {
    cache.UpdateEntry(5);
  }

  // 0 () () 4 3 5
  // 1
  // 2
  EXPECT_TRUE(cache.DelEntry(0));
  EXPECT_EQ(*cache.Victim(), 1);
  EXPECT_TRUE(cache.DelEntry(1));

  EXPECT_TRUE(cache.DelEntry(4));
  EXPECT_EQ(*cache.Victim(), 2);
  EXPECT_TRUE(cache.DelEntry(2));

  EXPECT_EQ(*cache.Victim(), 3);
  EXPECT_TRUE(cache.DelEntry(3));

  EXPECT_EQ(*cache.Victim(), 5);
  EXPECT_TRUE(cache.DelEntry(5));
}
