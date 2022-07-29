#include <random>
#include <string>
#define _AVL_TREE_DEBUG_
#include "mmkv/algo/avl_tree.h"
#include "mmkv/algo/reserved_array.h"

#include <gtest/gtest.h>
#include <assert.h>

using namespace mmkv::algo;

struct IntComparator {
  int operator()(int x, int y) const noexcept { return x - y; }
};

#define N 1000

static void AvlTest(int mode) {
  AvlTree<int, int, IntComparator> tree;
  
  std::default_random_engine dre;
  std::uniform_int_distribution<int> uid(0, N);
  
  std::vector<int> nums(N);

  for (auto& num : nums) {
    num = uid(dre);
  }

  for (int i = N-1; i >= 0; i--) {
    tree.Insert(mode == 0 ? i : nums[i]);
    auto success = tree.VerifyAvlProperties();(void)success;
    ASSERT_TRUE(success);
    // tree.Print(std::cout);
  }
  
  for (int i = 0; i < N; ++i) {
    auto val = tree.Find(mode == 0 ? i : nums[i]);(void)val;
    ASSERT_TRUE(val);
    ASSERT_TRUE(*val == (mode == 0 ? i : nums[i]));
  }
  
  // for (auto x : tree) {
  //   std::cout << x << " ";
  // }

  std::cout << "\n";
  std::cout << "height = " << tree.Height() << std::endl;

  tree.VerifyAvlProperties();
  // tree.Print(std::cout);

  for (int i = 0; i < N; ++i) {
    auto success = tree.Erase(mode == 0 ? i : nums[i]); (void)success;
    success = tree.VerifyAvlProperties();
    ASSERT_TRUE(success);
  }
  
  assert(tree.empty()); 
}

TEST(avl_tree_test, sorted) {
  AvlTest(0);
}

TEST(avl_tree_test, random) {
  AvlTest(1);
}

TEST(avl_tree_test, eq) {
  AvlTreeMap<int, int, IntComparator> tree;
  
  std::default_random_engine dre;
  std::uniform_int_distribution<int> uid(0, N);
  
  std::vector<int> nums(N);

  for (auto& num : nums) {
    num = uid(dre);
  }
  
  //using ValueType = decltype(tree)::value_type; 

  for (auto num : nums) {
    tree.InsertEq({num, num});
  }

  // for (auto kv : tree) {
  //   std::cout << kv.key << ", " << kv.value << "\n";
  // }

  auto first = tree.LowerBound(74);
  auto second = tree.UpperBound(901);
  
  std::cout << "first = " << first->key << "\n";
  std::cout << "second = " << second->key << "\n";
  // for (; first != second; ++first) {
  //   std::cout << first->key << "\n";
  // }

}

class AvlTreeTest : public testing::Test {
 protected:
  void SetUp() noexcept override {
    for (int i = 0; i < 100; ++i) {
      set.Insert(i);
    }
  }

  AvlTreeSet<int, IntComparator> set;
};

TEST_F(AvlTreeTest, doinall) {
  // set.DoInAll([](int x) {
  //     std::cout << x << " ";
  // });

  std::cout << "\n";
}

TEST_F(AvlTreeTest, size) {
  std::cout << "sizeof set = " << sizeof set << std::endl;
  static_assert(mmkv::algo::can_reallocate<decltype(set)>::value, "");
}
