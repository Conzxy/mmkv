#define _DEBUG_HASH_TABLE_
#include "mmkv/algo/hash_set.h"

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
}
