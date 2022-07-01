#define _AVL_TREE_DEBUG_
#include "mmkv/algo/avl_tree.h"
#include "mmkv/algo/internal/avl_tree.h"

#include <assert.h>
#include <cstdint>
#include <random>
#include <vector>
#include <set>

#include <benchmark/benchmark.h>

using namespace mmkv::algo;
using namespace std;
using namespace benchmark;

struct IntComparator {
  int operator()(int x, int y) const noexcept {
    return x - y;
  }
};

#define RANDOM_INIT \
  std::default_random_engine dre; \
  std::uniform_int_distribution<int> uid(0, count); \

#define RANDOM_NUM (uid(dre))

static void BM_StdSetInsert(State& state) {
  auto count = state.range(0);
  
  RANDOM_INIT

  for (auto _ : state) {
    set<int> tree; 
    for (int i = 0; i < count; ++i) {
      tree.insert(RANDOM_NUM);
    }
  }
}


static void BM_AvlTreeInsert(State& state) {
  auto count = state.range(0);

  RANDOM_INIT

  for (auto _ : state) {
    AvlTree<int, int, IntComparator> tree;

    for (int i = 0; i < count; ++i) {
      tree.Insert(RANDOM_NUM);
    }
  }
}

#define RANDOM_NUMS_INSTALL \
  std::vector<int> nums(count); \
  RANDOM_INIT \
  for (auto& num : nums) { \
    num = RANDOM_NUM; \
  } \

#define TREE_INSTALL \
  for (auto num : nums) { \
    tree.Insert(num); \
  }

static void BM_AvlTreeFind(State& state) {
  auto count = state.range(0);
  RANDOM_NUMS_INSTALL

  AvlTree<int, int, IntComparator> tree;
  TREE_INSTALL
  
  for (auto _ : state) {
    for (auto num : nums) {
      auto val = tree.Find(num);(void)val;
      assert(val);
      assert(*val == num);
    }
  }
}

static void BM_StdSetFind(State& state) {
  auto count = state.range(0);
  RANDOM_NUMS_INSTALL

  set<int> tree;
  for (auto num : nums) {
    tree.insert(num);
  } 

  for (auto _ : state) {
    for (auto num : nums) {
      auto val = tree.find(num);(void)val;
      assert(*val == num);
    }
  }
}

static void BM_StdSetErase(State& state) {
  auto count = state.range(0);
  RANDOM_NUMS_INSTALL

  set<int> tree;
  for (auto num : nums) {
    tree.insert(num);
  } 

  for (auto _ : state) {
    for (auto num : nums) {
      tree.erase(num);
    }
  }
  
}

static void BM_AvlTreeErase(State& state) {
  auto count = state.range(0);
  RANDOM_NUMS_INSTALL

  AvlTree<int, int, IntComparator> tree;
  TREE_INSTALL
  tree.VerifyAvlProperties();
  
  for (auto _ : state) {
    for (auto num : nums) {
      tree.Erase(num);
    }
  }
}

#define TREE_BENCHMARK(bench, name) \
  BENCHMARK(bench)->Name(name)->RangeMultiplier(10)->Range(100, 1000000) \


TREE_BENCHMARK(BM_StdSetFind, "std::set<int> find");
TREE_BENCHMARK(BM_AvlTreeFind, "AvlTree<int> find");
TREE_BENCHMARK(BM_StdSetInsert, "std::set<int> insert");
TREE_BENCHMARK(BM_AvlTreeInsert, "AvlTree<int> insert");
TREE_BENCHMARK(BM_StdSetErase, "std::set<int> erase");
TREE_BENCHMARK(BM_AvlTreeErase, "AvlTree<int> erase");
