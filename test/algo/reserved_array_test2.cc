#include "mmkv/algo/reserved_array.h"

#include <assert.h>

using namespace mmkv::algo;

int main() {
  ReservedArray<int> arr(10);
  int i = 0;

  std::generate(arr.begin(), arr.end(), [i]() mutable {
      return i++;
  });

  for (size_t i = 0; i < arr.GetSize(); ++i) {
    assert(arr[i] == i);
  }
}
