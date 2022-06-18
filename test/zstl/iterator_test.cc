#include "mmkv/zstl/iterator.h"

#include <vector>
#include <assert.h>

using namespace zstl;

struct A {
  A() = default;
  A(int x) : x_(x) {}

  A(A&& other) noexcept 
    : x_(other.x_) {
    other.x_ = 0;
    puts("A move ctor");
  }

  A& operator=(A&& other) noexcept {
    puts("A move assignment");
    return *this;
  }

  int x_;
};

int main() {
  std::vector<A> a{};

  a.emplace_back(2);

  auto move_iter = MakeMoveIteratorIfNoexcept(a.begin());
  (void)move_iter;

  auto i = *move_iter;(void)i;

  assert(i.x_ == 2);
  assert(a.begin()->x_ == 0);

  printf("i.x_ = %d, begin().x_ = %d\n", i.x_, a.begin()->x_);
}
