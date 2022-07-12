#include "mmkv/algo/blist.h"

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
  Blist<int> lst({0, 1,2,3,4,5,6});

  int i = 0;
  auto lst_beg = lst.begin();
  auto count = lst.size();
  for (; count > 0; count--, ++lst_beg) {
    EXPECT_EQ(i++, *(lst_beg));
  }
}