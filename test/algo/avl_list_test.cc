#define _AVL_TREE_DEBUG_
#include "mmkv/algo/comparator_util.h"
#include "mmkv/algo/internal/avl_list.h"

#include <iostream>

using namespace mmkv::algo;

int main() {
  struct IntComparator {
    inline int operator()(int x, int y) const noexcept {
      return x - y;
    }
  };

  AvlList<int, int, IntComparator> avl_lst;

  for (int i = 100; i >= 0; --i) {
    avl_lst.Insert(i);
  }

  for (auto x : avl_lst) {
    std::cout << x << " ";
  }
  std::cout << "\n";

  std::cout << "sizeof(AvlList) = " << sizeof avl_lst << std::endl;

  AvlListSet<std::string, Comparator<std::string>> lst;
  
  for (int i = 0; i < 100; ++i) {
    std::string* duplicate = nullptr;
    auto e = lst.InsertWithDuplicate(std::to_string(i), &duplicate);
    assert(e);
    auto res = lst.VerifyAvlProperties();
    assert(res);
    std::cout << i << ": " << duplicate << "\n";
  }
}
