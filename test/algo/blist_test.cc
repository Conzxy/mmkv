#include "mmkv/algo/blist.h"
#include "mmkv/algo/internal/bnode.h"

#include <gtest/gtest.h>

using namespace mmkv::algo;

TEST(blist, push_front) {
  Blist<int> bl;
  ASSERT_TRUE(bl.empty());
  
  ASSERT_EQ(1, bl.PushFront(1));
  ASSERT_EQ(bl.size(), 1);
  EXPECT_EQ(*bl.begin(), 1);

  ASSERT_EQ(1, bl.PushFront(2));
  ASSERT_EQ(bl.size(), 2);
  EXPECT_EQ(*bl.begin(), 2);

  int i = 2;
  for (auto e : bl) {
    std::cout << e << " ";
    EXPECT_EQ(i--, e);
  }
  std::cout << std::endl;
}

TEST(blist, push_back) {
  Blist<int> bl;
  
  bl.PushBack(1);
  bl.PushBack(2);
  bl.PushBack(3);
  
  ASSERT_EQ(bl.size(), 3); 

  int i = 1;
  for (auto e : bl) {
    std::cout << e << " ";
    EXPECT_EQ(e, i++);
  }
  std::cout << std::endl;

}

TEST(blist, pop_front) {
  Blist<int> bl;

  bl.PushFront(1);
  bl.PushFront(2);
  bl.PushFront(3);

  bl.PopFront();
  ASSERT_EQ(bl.size(), 2);
  ASSERT_EQ(bl.Front(), 2);

  bl.PopFront();
  ASSERT_EQ(bl.size(), 1);
  ASSERT_EQ(bl.Front(), 1);
  
  bl.PopFront();
  ASSERT_EQ(bl.size(), 0);
  ASSERT_TRUE(bl.empty());
}

TEST(blist, pop_back) {
  Blist<int> bl;

  bl.PushBack(1);
  ASSERT_EQ(bl.size(), 1);
  ASSERT_EQ(bl.Back(), 1);

  bl.PushBack(2);
  bl.PushBack(3);
  
  ASSERT_EQ(bl.size(), 3);
  ASSERT_EQ(bl.Back(), 3);

  bl.PopBack();
  ASSERT_EQ(bl.Back(), 2);
  bl.PopBack();
  ASSERT_EQ(bl.Back(), 1);
  bl.PopBack();
  ASSERT_TRUE(bl.empty());
}

TEST(blist, initializer_list) {
  Blist<int> lst({0,1,2,3,4,5,6});

  int i = 0;
  auto lst_beg = lst.begin();
  auto count = lst.size();
  for (; count > 0; count--, ++lst_beg) {
    EXPECT_EQ(i++, *(lst_beg));
  }
}

static inline blist::BNode<int> *node_advance(blist::BNode<int> *node, int n) noexcept {
  while (n--) node = node->next;
  return node;
}

TEST(blist, erase) {
  Blist<int> lst({0,1,2,3,4,5,6});
  ASSERT_EQ(lst.size(), 7); 
  Blist<int>::Node *nodes[7]; 
  nodes[0] = node_advance(lst.FrontNode(), 1);
  nodes[1] = node_advance(lst.FrontNode(), 3);
  nodes[2] = node_advance(lst.FrontNode(), 2);
  nodes[3] = node_advance(lst.FrontNode(), 0);
  nodes[4] = node_advance(lst.FrontNode(), 6);
  nodes[5] = node_advance(lst.FrontNode(), 4);
  nodes[6] = node_advance(lst.FrontNode(), 5);

  for (size_t i = 0; i < 7; ++i)
    lst.Erase(nodes[i]);

  ASSERT_EQ(lst.size(), 0);
  for (auto e : lst)
    printf("e = %d\n", e);
}

