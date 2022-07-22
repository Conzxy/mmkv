#define _DEBUG_TREE_HASH_TABLE_
#include "mmkv/algo/avl_tree_hashtable.h"
#include "mmkv/algo/avl_dictionary.h"
#include "mmkv/algo/avl_tree.h"

#include "util.h"

#include <gtest/gtest.h>

using namespace mmkv::algo;

TEST(dictionary_test, insert) {
  struct IntComparator {
    inline int operator()(int x, int y) const noexcept {
      return x - y;
    }
  };

  AvlDictionary<int, int, IntComparator> dict;

  for (int i = 0; i < 100; ++i) {
    auto kv = dict.InsertKv(i, i);
    ASSERT_TRUE(kv);
    EXPECT_TRUE(kv->key == i);
    EXPECT_TRUE(kv->value == i);
  }
  
  for (int i = 0; i < 100; ++i) {
    auto kv = dict.Find(i);
    ASSERT_TRUE(kv);
    EXPECT_EQ(kv->key, i);
    EXPECT_EQ(kv->value, i);
  }
  
  struct StrComparator {
    inline int operator()(std::string const& x, std::string const& y) const noexcept {
      return ::strcmp(x.c_str(), y.c_str());
    }
  };

  AvlTreeHashSet<std::string, StrComparator> ht;
  
  for (int i = 0; i < 100; ++i) {
    auto si = std::to_string(i);
    std::cout << i << " " << ht.Insert(std::move(si)) << "\n";
    ht.DebugPrint();
  }
  
  int count = 0;
  for (auto s : ht) {
    std::cout << s << "\n";
    count++;
  }
  
  std::cout << "count = " << count << std::endl;
}


