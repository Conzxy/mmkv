#include "mmkv/algo/avl_tree_hashtable.h"
#include "mmkv/algo/hash_set.h"

#include <unordered_set>
#include <stdio.h>
#include <benchmark/benchmark.h>
#include <cstring>

using namespace benchmark;
using namespace mmkv::algo;
using namespace std;

template <typename T>
class StlHashSet {
 public:
  StlHashSet() = default;

  template <typename U>
  void Insert(U &&val)
  {
    set_.insert(val);
  }

  void   Find(T const &key) { set_.find(key); }
  void   Erase(T const &key) { set_.erase(key); }
  size_t size() const noexcept { return set_.size(); }

  std::unordered_set<T, Hash<T>> set_;
};

struct IntComparator {
  inline int operator()(int x, int y) const noexcept { return x - y; }
};

struct StrComparator {
  inline int operator()(std::string const &x, std::string const &y) const noexcept
  {
    return ::strcmp(x.c_str(), y.c_str());
  }
};

template <typename HT>
static void BM_Insert(State &state)
{
  for (auto _ : state) {
    HT table;
    for (int i = 0; i < state.range(0); ++i)
      table.Insert(std::to_string(i));
  }
}

#define HT_SETUP(_table, _count)                                                                   \
  do {                                                                                             \
    for (int i = 0; i < _count; ++i) {                                                             \
      table.Insert(std::to_string(i));                                                             \
    }                                                                                              \
  } while (0)

template <typename HT>
static void BM_Find(State &state)
{
  HT table;
  HT_SETUP(table, state.range(0));

  for (auto _ : state) {
    for (int i = 0; i < state.range(0); ++i) {
      // state.PauseTiming();
      // table.Insert(std::to_string(i));
      // state.ResumeTiming();
      (void)table.Find(std::to_string(i));
    }
  }
}

template <typename HT>
static void BM_Erase(State &state)
{
  HT table;

  for (auto _ : state) {
    state.PauseTiming();
    HT_SETUP(table, state.range(0));
    state.ResumeTiming();
    for (int i = 0; i < state.range(0); ++i) {
      table.Erase(std::to_string(i));
    }
  }
}

using AvlTb  = AvlTreeHashSet<string, StrComparator>;
using ListTb = HashSet<string>;
using StlTb  = StlHashSet<string>;

static inline void BM_AvlInsert(State &state) { BM_Insert<AvlTb>(state); }
static inline void BM_AvlFind(State &state) { BM_Find<AvlTb>(state); }
static inline void BM_AvlErase(State &state) { BM_Erase<AvlTb>(state); }

static inline void BM_ListInsert(State &state) { BM_Insert<ListTb>(state); }
static inline void BM_ListFind(State &state) { BM_Find<ListTb>(state); }
static inline void BM_ListErase(State &state) { BM_Erase<ListTb>(state); }

static inline void BM_StlInsert(State &state) { BM_Insert<StlTb>(state); }
static inline void BM_StlFind(State &state) { BM_Find<StlTb>(state); }
static inline void BM_StlErase(State &state) { BM_Erase<StlTb>(state); }

#define AVL_HASHTABLE_BENCH_DEFINE(_func, _name)                                                   \
  BENCHMARK(_func)->Name(_name)->RangeMultiplier(10)->Range(100, 100000)

AVL_HASHTABLE_BENCH_DEFINE(BM_StlFind, "StlHashSet Find");
AVL_HASHTABLE_BENCH_DEFINE(BM_AvlFind, "AvlTreeHashSet Find");
AVL_HASHTABLE_BENCH_DEFINE(BM_ListFind, "HashSet Find");
AVL_HASHTABLE_BENCH_DEFINE(BM_StlInsert, "StlHashSet Insert");
AVL_HASHTABLE_BENCH_DEFINE(BM_AvlInsert, "AvlTreeHashSet Insert");
AVL_HASHTABLE_BENCH_DEFINE(BM_ListInsert, "HashSet Insert");
AVL_HASHTABLE_BENCH_DEFINE(BM_StlErase, "StlHashSet Erase");
AVL_HASHTABLE_BENCH_DEFINE(BM_AvlErase, "AvlTreeHashSet Erase");
AVL_HASHTABLE_BENCH_DEFINE(BM_ListErase, "HashSet Erase");
