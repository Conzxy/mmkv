#include "mmkv/algo/blist.h"

#include <list>
#include <benchmark/benchmark.h>

using namespace mmkv::algo;
using namespace benchmark;

// disable SSO
#define STR "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"

static void
BM_StdListPushBack(State& state) {
  for (auto _ : state) {
    std::list<std::string> lst;
    for (int i = 0; i < state.range(0); ++i)
      lst.emplace_back(STR);
  }
}

static void
BM_StdListPushFront(State& state) {
  for (auto _ : state) {
    std::list<std::string> lst;
    for (int i = 0; i < state.range(0); ++i)
      lst.emplace_front(STR);
  }
}

static void
BM_StdListPopBack(State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    std::list<std::string> lst;
    for (int i = 0; i < state.range(0); ++i)
      lst.emplace_back(STR);
    state.ResumeTiming();
    for (int i = 0; i < state.range(0); ++i) {
      lst.pop_back();
    }
  }
}

static void
BM_StdListPopFront(State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    std::list<std::string> lst;
    for (int i = 0; i < state.range(0); ++i)
      lst.emplace_back(STR);
    state.ResumeTiming();
    for (int i = 0; i < state.range(0); ++i) {
      lst.pop_front();
    }
  }
}

static void 
BM_BlistPushBack(State& state) {
  for (auto _ : state) {
    Blist<std::string> lst;
    for (int i = 0; i < state.range(0); ++i)
      lst.PushBack(STR);
  }
}

static void
BM_BlistPushFront(State& state) {
  for (auto _ : state) {
    Blist<std::string> lst;
    for (int i = 0; i < state.range(0); ++i)
      lst.PushFront(STR);
  }
}

static void
BM_BlistPopBack(State& state) {
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

static void
BM_BlistPopFront(State& state) {
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

#define BLIST_BM_DEFINE(_func, _name) \
BENCHMARK(_func)->Name(_name)->RangeMultiplier(10)->Range(100, 10000)

BLIST_BM_DEFINE(BM_BlistPushBack, "mmkv::algo::Blist<std::string> pushback");
BLIST_BM_DEFINE(BM_BlistPushFront, "mmkv::algo::Blist<std::string> pushfront");
BLIST_BM_DEFINE(BM_BlistPopBack, "mmkv::algo::Blist<std::string> popback");
BLIST_BM_DEFINE(BM_BlistPopFront, "mmkv::algo::Blist<std::string> popfront");
BLIST_BM_DEFINE(BM_StdListPushBack, "std::list<std::string> pushback");
BLIST_BM_DEFINE(BM_StdListPushFront, "std::list<std::string> pushfront");
BLIST_BM_DEFINE(BM_StdListPopBack, "std::list<std::string> popback");
BLIST_BM_DEFINE(BM_StdListPopFront, "std::list<std::string> popfront");
