#include "mmkv/algo/reserved_array.h"
#include "mmkv/algo/slist.h"

#include <gtest/gtest.h>

#include <string>

using namespace mmkv::algo;
using namespace std;

TEST(reserved_array, construct) {
  puts("====== Defalut constructor + Grow =====");
  ReservedArray<int> arr2{};

  arr2.Grow(10);
  arr2[0] = 2;
  
  EXPECT_EQ(arr2[0], 2);
  
  ReservedArray<string> str_arr{};

  str_arr.Grow(10);

  str_arr[1] = "Conzxy";

  EXPECT_EQ(str_arr.GetSize(), 10);
  EXPECT_EQ(str_arr[1], "Conzxy");

  puts("===== size constructor =====");

  ReservedArray<int> arr(4);
  
  int i = 0;

  std::generate(arr.begin(), arr.end(), [i]() mutable {
      return i++;
  });

  for (size_t i = 0; i < arr.GetSize(); ++i) {
    EXPECT_EQ(arr[i], i);
  }
}

struct A {
  A() noexcept {}
  A(A&&) noexcept {
    puts("A move ctor");
  }

  A& operator=(A&&) noexcept {
    puts("A move assignment");
    return *this;
  }

  ~A() noexcept {}

  constexpr static bool can_reallocate = true;
};

struct B {
  B() {}
  B(B&&) noexcept {
    puts("B move ctor");
  }

  B& operator=(B&&) noexcept {
    puts("B move assignment");
    return *this;
  }

  ~B() noexcept {}
};

TEST(reserved_array, reallocate) {
  static_assert(!std::is_trivial<A>::value, "");

  static_assert(can_reallocate<A>::value, "");
  static_assert(!can_reallocate<B>::value, "");
  static_assert(can_reallocate<int>::value, "");
  static_assert(can_reallocate<Slist<int>>::value, "");
  
  puts("===== reserved_array reallocate test =====");

  ReservedArray<A> a{2};
  a.Grow(10);

  ReservedArray<B> b{2};
  b.Grow(10);
}
