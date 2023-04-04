#include "mmkv/algo/blist.h"

#include <benchmark/benchmark.h>
#include <deque>
#include <list>

using namespace mmkv::algo;
using namespace benchmark;

// disable SSO
#define STR "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"

template <typename L>
static void BM_StlStylePushBack(State &state)
{
  for (auto _ : state) {
    L lst;
    for (int i = 0; i < state.range(0); ++i)
      lst.emplace_back(STR);
  }
}

template <typename L>
static void BM_StlStylePushFront(State &state)
{
  for (auto _ : state) {
    L lst;
    for (int i = 0; i < state.range(0); ++i)
      lst.emplace_front(STR);
  }
}

template <typename L>
static void BM_StlStylePopBack(State &state)
{
  for (auto _ : state) {
    state.PauseTiming();
    L lst;
    for (int i = 0; i < state.range(0); ++i)
      lst.emplace_back(STR);
    state.ResumeTiming();
    for (int i = 0; i < state.range(0); ++i) {
      lst.pop_back();
    }
  }
}

template <typename L>
static void BM_StlStylePopFront(State &state)
{
  for (auto _ : state) {
    state.PauseTiming();
    L lst;
    for (int i = 0; i < state.range(0); ++i)
      lst.emplace_back(STR);
    state.ResumeTiming();
    for (int i = 0; i < state.range(0); ++i) {
      lst.pop_front();
    }
  }
}

#define DEFINE_BM_STL_SUIT(type__)                                             \
  static void BM_Std##type__##PushBack(State &state)                           \
  {                                                                            \
    BM_StlStylePushBack<type__>(state);                                        \
  }                                                                            \
                                                                               \
  static void BM_Std##type__##PushFront(State &state)                          \
  {                                                                            \
    BM_StlStylePushFront<type__>(state);                                       \
  }                                                                            \
                                                                               \
  static void BM_Std##type__##PopBack(State &state)                            \
  {                                                                            \
    BM_StlStylePopBack<type__>(state);                                         \
  }                                                                            \
                                                                               \
  static void BM_Std##type__##PopFront(State &state)                           \
  {                                                                            \
    BM_StlStylePopFront<type__>(state);                                        \
  }

using List = std::list<std::string>;
using Deque = std::deque<std::string>;

DEFINE_BM_STL_SUIT(List);
DEFINE_BM_STL_SUIT(Deque);

static void BM_BlistPushBack(State &state)
{
  for (auto _ : state) {
    Blist<std::string> lst;
    for (int i = 0; i < state.range(0); ++i)
      lst.PushBack(STR);
  }
}

static void BM_BlistPushFront(State &state)
{
  for (auto _ : state) {
    Blist<std::string> lst;
    for (int i = 0; i < state.range(0); ++i)
      lst.PushFront(STR);
  }
}

static void BM_BlistPopBack(State &state)
{
  for (auto _ : state) {
    state.PauseTiming();
    Blist<std::string> lst;
    for (int i = 0; i < state.range(0); ++i)
      lst.PushBack(STR);
    state.ResumeTiming();
    for (int i = 0; i < state.range(0); ++i) {
      lst.PopBack();
    }
  }
}

static void BM_BlistPopFront(State &state)
{
  for (auto _ : state) {
    state.PauseTiming();
    Blist<std::string> lst;
    for (int i = 0; i < state.range(0); ++i)
      lst.PushBack(STR);
    state.ResumeTiming();
    for (int i = 0; i < state.range(0); ++i) {
      lst.PopFront();
    }
  }
}

#define BLIST_BM_DEFINE(_func, _name)                                          \
  BENCHMARK(_func)->Name(_name)->RangeMultiplier(10)->Range(100, 10000)

BLIST_BM_DEFINE(BM_BlistPushBack, "mmkv::algo::Blist<std::string> pushback");
BLIST_BM_DEFINE(BM_BlistPushFront, "mmkv::algo::Blist<std::string> pushfront");
BLIST_BM_DEFINE(BM_BlistPopBack, "mmkv::algo::Blist<std::string> popback");
BLIST_BM_DEFINE(BM_BlistPopFront, "mmkv::algo::Blist<std::string> popfront");
BLIST_BM_DEFINE(BM_StdListPushBack, "std::list<std::string> pushback");
BLIST_BM_DEFINE(BM_StdListPushFront, "std::list<std::string> pushfront");
BLIST_BM_DEFINE(BM_StdListPopBack, "std::list<std::string> popback");
BLIST_BM_DEFINE(BM_StdListPopFront, "std::list<std::string> popfront");
BLIST_BM_DEFINE(BM_StdDequePushBack, "std::deque<std::string> pushback");
BLIST_BM_DEFINE(BM_StdDequePushFront, "std::deque<std::string> pushfront");
BLIST_BM_DEFINE(BM_StdDequePopBack, "std::deque<std::string> popback");
BLIST_BM_DEFINE(BM_StdDequePopFront, "std::deque<std::string> popfront");
