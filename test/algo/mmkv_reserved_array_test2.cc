#include "mmkv/util/memory_footprint.h"
#define _DEBUG_RESERVED_ARRAY_
#include "mmkv/algo/reserved_array.h"

#include <assert.h>

using namespace mmkv::algo;
using namespace mmkv::util;

int main() {
  ReservedArray<int> arr(10);
  MemoryFootPrint();
  
  arr.Grow(100);
  MemoryFootPrint();

  arr.Shrink(10);
  MemoryFootPrint();

  {
    std::unique_ptr<ReservedArray<int>> p(new ReservedArray<int>(100));
    p->Grow(200);
  }

  MemoryFootPrint();
  

}
