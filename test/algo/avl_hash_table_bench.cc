#include "mmkv/algo/avl_tree_hashtable.h"
#include "mmkv/algo/hash_set.h"

#include <benchmark/benchmark.h>
#include <cstring>

using namespace benchmark;
using namespace mmkv::algo;

struct IntComparator {
  inline int operator()(int x, int y) const noexcept {
    return x - y;
  }
};

struct StrComparator {
  inline int operator()(std::string const& x, std::string const& y) const noexcept {
    return ::strcmp(x.c_str(), y.c_str());
  }
};

template<typename HT>
static void BM_Insert(State& state) {
  for (auto _ : state) {
    HT table;
    for (int i = 0; i < state.range(0); ++i)
      table.Insert(i);
  }
}

#define HT_SETUP(_table, _count) do {\
  for (int i = 0; i < _count; ++i) { \
    table.Insert(i); \
  } } while (0)

template<typename HT>
static void BM_Find(State& state) {
  HT table;
  HT_SETUP(table, state.range(0));

  for (auto _ : state) {
    for (int i = 0; i < state.range(0); ++i) {
      (void)table.Find(i);
    }
  }
}

template<typename HT>
static void BM_Erase(State& state) {
  HT table;

  for (auto _ : state) {
    state.PauseTiming();
    HT_SETUP(table, state.range(0));
    state.ResumeTiming();
    for (int i = 0; i < state.range(0); ++i) {
      table.Erase(i);
    }
  }
}

using AvlTb = AvlTreeHashSet<int, IntComparator>;
using ListTb = HashSet<int>;

static inline void BM_AvlInsert(State& state) {
  BM_Insert<AvlTb>(state);
}

static inline void BM_AvlFind(State& state) {
  BM_Find<AvlTb>(state);
}

static inline void BM_AvlErase(State& state) {
  BM_Erase<AvlTb>(state);
}

static inline void BM_ListInsert(State& state) {
  BM_Insert<ListTb>(state);
}

static inline void BM_ListFind(State& state) {
  BM_Find<ListTb>(state);
}

static inline void BM_ListErase(State& state) {
  BM_Erase<ListTb>(state);
}

#define AVL_HASHTABLE_BENCH_DEFINE(_func, _name) \
  BENCHMARK(_func)->Name(_name)->RangeMultiplier(10)->Range(100, 100000)

AVL_HASHTABLE_BENCH_DEFINE(BM_AvlInsert, "AvlTreeHashSet Insert");
AVL_HASHTABLE_BENCH_DEFINE(BM_AvlFind, "AvlTreeHashSet Find");
AVL_HASHTABLE_BENCH_DEFINE(BM_AvlErase, "AvlTreeHashSet Erase");
AVL_HASHTABLE_BENCH_DEFINE(BM_ListInsert, "HashSet Insert");
AVL_HASHTABLE_BENCH_DEFINE(BM_ListFind, "HashSet Find");
AVL_HASHTABLE_BENCH_DEFINE(BM_ListErase, "HashSet Erase");
