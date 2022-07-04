#include <cstdlib>
#include <functional>
#define _DEBUG_HASH_TABLE_
#include "mmkv/algo/hash_set.h"

#include <random>

using namespace mmkv::algo;

int main() {
  {
  HashSet<int> hset;

  for (int i = 0; i < 100; ++i) {
    printf("===== insert key = %d =====\n", i);
    hset.Insert(i);
    hset.DebugPrint();
  }
  }
  
  {
  HashSet<int> hset;

  for (int i = 0; i < 100; ++i) {
    printf("===== insert key = %d =====\n", i);
    hset.Insert(i);
    hset.DebugPrint();
  }
  }

  HashSet<int> set;
#define N 10000
  std::default_random_engine dre;
  std::uniform_int_distribution<int> uid(0, 10000);
  
  for (int i = 0; i < N; ++i) {
    set.Insert(uid(dre));
  }

  set.DebugPrint();
}
