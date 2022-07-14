#include <algorithm>
#include <cstdio>
#define _DEBUG_TREE_HASH_TABLE_

#include "mmkv/algo/avl_tree_hashtable.h"

#include <gtest/gtest.h>

using namespace mmkv::algo;

struct IntComparator {
  inline int operator()(int x, int y) const noexcept {
    return x - y;
  }
};

template<typename T>
using HashSet = AvlTreeHashSet<T, IntComparator>;

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
      hset.DebugPrint();
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

TEST_F(HashSetTest, iterator) {
  hset.DebugPrint();
  printf("===== range for test =====\n"); 
  int count = 0;

  std::vector<int> res;
  for (auto x : hset) {
    count++;
    res.push_back(x);
  }
  
  std::sort(res.begin(), res.end()); 
  for (auto x : res) {
    std::cout << x << " ";
  }

  ::puts("");
  EXPECT_EQ(count, 100) << "range for test failed";

  auto beg = hset.begin();

  printf("===== begin() test ======\n");
  printf("%d\n", *beg);
  printf("change begin to 1\n");
  *beg = 1;

  printf("===== pre increment =====\n");
  ++beg;
  EXPECT_NE(beg, hset.end());
  printf("%d\n", *beg);

  auto old_value = *beg;
  printf("===== post increment =====\n");
  auto old_beg = beg++;
  EXPECT_NE(beg, hset.end());
  ASSERT_EQ(*old_beg, old_value);
  printf("%d\n", *beg);


}

TEST(hash_set, find2) {
  HashSet<int> hset;

  auto res = hset.Find(1);(void)res;
  auto res2 = hset.Erase(1);(void)res2;

  assert(res2 == 0);
  assert(!res);
}

// TEST_F(HashSetTest, clone) {
//   HashSet<int> s;
//   s.Clone(hset);

//   hset.DebugPrint();
//   s.DebugPrint();
  
//   EXPECT_EQ(s.size(), hset.size());
//   for (auto x : hset) {
//     ASSERT_TRUE(s.Find(x));
//   }
// }

// TEST(hash_set_test, union) {
//   HashSet<int> s1({ 0, 1, 2, 3, 4, 5 });
//   HashSet<int> s2({ 1, 2, 3, 4, 6, 7 });

//   std::vector<int> result;

//   s1.Union(s2, [&result](int x) {
//     result.push_back(x);
//   });
  
//   ASSERT_EQ(result.size(), 8);
//   for (auto const& x : result) {
//     std::cout << x << " ";
//   }
//   std::cout << std::endl;
// }

// TEST(hash_set_test, difference) {
//   HashSet<int> s1({ 0, 1, 2, 3, 4, 5 });
//   HashSet<int> s2({ 1, 2, 3, 4, 6, 7 });

//   std::vector<int> result;

//   s1.Difference(s2, [&result](int x) {
//     result.push_back(x);
//   });
  
//   ASSERT_EQ(result.size(), 2);
//   for (auto const& x : result) {
//     std::cout << x << " ";
//   }
//   std::cout << std::endl;
// }

// TEST(hash_set_test, intersection) {
//   HashSet<int> s1({ 0, 1, 2, 3, 4, 5 });
//   HashSet<int> s2({ 1, 2, 3, 4, 6, 7 });

//   std::vector<int> result;

//   s1.Intersection(s2, [&result](int x) {
//     result.push_back(x);
//   });
  
//   ASSERT_EQ(result.size(), 4);

//   for (auto const& x : result) {
//     std::cout << x << " ";
//   }
//   std::cout << std::endl;
// }
