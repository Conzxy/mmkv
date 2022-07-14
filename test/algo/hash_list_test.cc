#include "mmkv/algo/internal/hash_list.h"

#include <gtest/gtest.h>

using namespace mmkv::algo;

TEST(hash_list, push) {
  HashList<int> lst;

  lst.Push(1);
  lst.Push(2);

  for (auto x : lst) {
    std::cout << x << " ";
  }
}