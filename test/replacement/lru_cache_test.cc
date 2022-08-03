#include "mmkv/replacement/lru_cache.h"

#include <stdio.h>
#include <gtest/gtest.h>

using namespace mmkv::replacement;

TEST(lru_cache, update) {
  int max_size = 10;
  LruCache<int> cache(max_size);

  for (int i = 0; i < max_size; ++i) {
    cache.UpdateEntry(i);
  }

  ASSERT_EQ(cache.size(), 10);
  auto &entries = cache.entries();

  for (auto i : entries) {
    printf("%d ", *i);
  }
  puts("");
}

TEST(lru_cache, exists) {
  int max_size = 10;
  LruCache<int> cache(max_size);

  for (int i = 0; i < max_size - 1; ++i) {
    cache.UpdateEntry(i);
  }

  for (int i = 0; i < max_size - 1; ++i)
    EXPECT_TRUE(cache.Exists(i));

  EXPECT_FALSE(cache.Exists(max_size-1));

  auto &enties = cache.entries();

  for (auto i : enties) {
    printf("%d ", *i);
  }

  puts("");
  for (int i = max_size-1; i < 12; ++i) {
    cache.UpdateEntry(i);
  }

  for (int i = max_size-1; i < 12; ++i) {
    EXPECT_TRUE(cache.Exists(i));
  }

  for (int i = 0; i < 2; ++i) {
    EXPECT_FALSE(cache.Exists(i));
  }

  for (auto i : enties) {
    printf("%d ", *i);
  }
  puts("");

}