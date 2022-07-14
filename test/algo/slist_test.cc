#include "mmkv/algo/slist.h"
#include "mmkv/util/memory_stat.h"

#include <gtest/gtest.h>
#include <string>
#include <stdio.h>

using namespace std;
using namespace mmkv::algo;

class Dummy {
 public:
  Dummy() {
    puts("Dummy()");
  }

  ~Dummy() noexcept {
    puts("~Dummy()");
  }
};

TEST(slist, construct_and_destory) {
  Slist<Dummy> sl;

  sl.EmplaceFront();
  sl.EmplaceFront();
}

TEST(slist, EmplaceFront) {
  Slist<string> sl;
  
  sl.EmplaceFront("Conzxy");
  sl.EmplaceFront("A");
  
  auto iter = sl.begin();

  EXPECT_EQ(*iter, "A");
  EXPECT_EQ(*(++iter), "Conzxy");  
}

TEST(slist, Extract) {
  Slist<string> sl;

  sl.EmplaceFront("A");
  sl.EmplaceFront("B");
  sl.EmplaceFront("C");
  EXPECT_EQ(sl.GetSize(), 3);

  auto node_A = sl.ExtractNode("A");

  EXPECT_EQ(sl.GetSize(), 2);
  auto node_B = sl.ExtractNode("B");

  EXPECT_EQ(sl.GetSize(), 1);
  auto node_C = sl.ExtractNode("C");

  EXPECT_TRUE(sl.IsEmpty());

  sl.EmplaceFrontNode(node_B);
  sl.EmplaceFrontNode(node_A);
  sl.EmplaceFrontNode(node_C);

  EXPECT_EQ(sl.GetSize(), 3);

  auto iter = sl.begin();

  EXPECT_EQ(*(iter++), "C");
  EXPECT_EQ(*(iter++), "A");
  EXPECT_EQ(*(iter++), "B");
  EXPECT_EQ(iter, sl.end());
}

class SlistTest : public testing::Test {
 protected:
  void SetUp() override {
    for (int i = 99; i >= 0; --i) {
      sl_.EmplaceFront(i);
    }
  
    int i = 0;
    for (auto x : sl_) {
      ASSERT_EQ(x, i++) << "SetUp";
    }
  }

 Slist<int> sl_;
};

TEST_F(SlistTest, copy_ctor) {
  Slist<int> sl = sl_;
  
  int i = 0;
  for (auto x : sl) {
    EXPECT_EQ(x, i++);
  } 
}

TEST_F(SlistTest, copy_assignment) {
  Slist<int> sl({ 1, 2 });
  
  sl = sl_;

  int elem = 0;

  for (auto x : sl) {
    EXPECT_EQ(elem++, x) << "less";
  }
  elem = 0;

  Slist<int> sl2;
  
  for (int i = 0; i < 100; ++i) {
    sl2.EmplaceFront(i);
  }
  
  sl2 = sl_;
  
  for (auto x : sl2) {
    EXPECT_EQ(x, elem++) << "equal";
  }
  elem = 0;

  Slist<int> sl3;

  for (int i = 0; i < 120; ++i) {
    sl3.EmplaceFront(i);
  }

  sl3 = sl_;

  for (auto x : sl3) {
    EXPECT_EQ(x, elem++) << "greater";
  }
  elem = 0;
}
