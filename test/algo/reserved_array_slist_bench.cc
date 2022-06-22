//#define _DEBUG_RESERVED_ARRAY_
#include "reserved_array_bench.h"
#include "mmkv/algo/slist.h"

static void BM_VectorSlist(State& state) {
  BM_Vector(state, Slist<int>({1}));
}

static void BM_ReservedArraySlist(State& state) {
  BM_ReservedArray(state, Slist<int>({1}));
}

BENCHMARK_DEF(BM_ReservedArraySlist, "mmkv::algo::ReservedArray<Slist<int>>");
BENCHMARK_DEF(BM_VectorSlist, "std::vector<Slist<int>>");
