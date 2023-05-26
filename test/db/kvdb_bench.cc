#include "mmkv/db/kvdb.h"

#include <kanon/thread/mutex_lock.h>
#include <benchmark/benchmark.h>
#include <xxhash.h>

using namespace benchmark;
using namespace mmkv::db;
using namespace mmkv::algo;
using namespace kanon;

static void BM_single_db(State &state)
{
  for (auto _ : state) {
    state.PauseTiming();
    MutexLock db_lock;
    MmkvDb    db;

    std::vector<std::unique_ptr<Thread>> thrs;
    thrs.resize(8);

    auto num   = state.range(0);
    auto slice = num / 8;

    for (int i = 0; i < 8; ++i) {
      thrs[i].reset(new Thread([slice, i, &state, &db, &db_lock]() {
        auto beg = i * slice;
        auto end = beg + slice;
        for (int j = beg; j < end; ++j) {
          auto   str = std::to_string(j);
          String mmkv_str;
          mmkv_str.append(str.data(), str.size());
          auto value = mmkv_str;

          MutexGuard guard(db_lock);
          db.InsertStr(std::move(mmkv_str), std::move(value));
        }
      }));
    }
    state.ResumeTiming();

    for (int i = 0; i < 8; ++i) {
      thrs[i]->StartRun();
    }

    for (int i = 0; i < 8; ++i) {
      thrs[i]->Join();
    }
  }
}

static void BM_multi_db(State &state)
{
  for (auto _ : state) {
    state.PauseTiming();
    std::vector<MutexLock>               db_locks(8);
    std::vector<std::unique_ptr<MmkvDb>> dbs(8);
    for (int i = 0; i < 8; ++i) {
      dbs[i].reset(new MmkvDb());
    }

    std::vector<std::unique_ptr<Thread>> thrs;
    thrs.resize(8);

    auto num   = state.range(0);
    auto slice = num / 8;
    for (int i = 0; i < 8; ++i) {
      thrs[i].reset(new Thread([slice, i, &state, &dbs, &db_locks]() {
        auto beg = i * slice;
        auto end = beg + slice;
        for (int j = beg; j < end; ++j) {
          auto   str = std::to_string(j);
          String mmkv_str;
          mmkv_str.append(str.data(), str.size());
          auto value = mmkv_str;

          auto  key_index = XXH32(mmkv_str.data(), mmkv_str.size(), 0) & 7;
          auto &db        = *dbs[key_index];

          MutexGuard guard(db_locks[key_index]);
          db.InsertStr(std::move(mmkv_str), std::move(value));
        }
      }));
    }
    state.ResumeTiming();

    for (int i = 0; i < 8; ++i) {
      thrs[i]->StartRun();
    }

    for (int i = 0; i < 8; ++i) {
      thrs[i]->Join();
    }
  }
}

#define BENCH_DEFINE(_func, _name)                                                                 \
  BENCHMARK(_func)->Name(_name)->RangeMultiplier(10)->Range(100, 100000)

BENCH_DEFINE(BM_multi_db, "Multi db");
BENCH_DEFINE(BM_single_db, "Single db");
