#include "mmkv/algo/blist.h"

#include <gtest/gtest.h>

using namespace mmkv::algo;

TEST(blist, push_front) {
  Blist<int> bl;
  
  ASSERT_TRUE(bl.empty());
  
  ASSERT_EQ(1, bl.PushFront(1));
  ASSERT_EQ(bl.size(), 1);

  ASSERT_EQ(1, bl.PushFront(2));
  
  ASSERT_EQ(bl.size(), 2);
  
  auto beg = bl.begin();
  EXPECT_EQ(*beg, 2);
  EXPECT_EQ(1, bl.PopFront());

  ASSERT_EQ(bl.size(), 1);

  EXPECT_EQ(*bl.begin(), 1);
}

TEST(blist, push_back) {
  Blist<int> bl;
  
  bl.PushBack(1);
  bl.PushBack(2);
  bl.PushBack(3);
  
  ASSERT_EQ(bl.size(), 3); 
  auto beg = bl.begin();

  for (int i = 1; i <= 3; ++i) {
    EXPECT_EQ(*(beg++), i);
  }

  for (int i = 1; i <= 3; ++i) {
    EXPECT_EQ(*(beg++), i);
  }
}

TEST(blist, pop_front) {
  Blist<int> bl;

  bl.PushFront(1);
  bl.PushFront(2);
  bl.PushFront(3);
  
  auto beg = bl.begin();
  int j = 3;
  for (int i = 0; i < 3; ++i) {
    EXPECT_EQ(*(beg++), j--); 
  }
  ASSERT_EQ(bl.size(), 3);
  ASSERT_EQ(bl.Front(), 3);

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
