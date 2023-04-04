#include "mmkv/algo/block_array.h"

#include <gtest/gtest.h>

using namespace mmkv::algo;

TEST(block_array, push_back)
{
  BlockArray<int> arr;
  arr.PushBack(1);
}
