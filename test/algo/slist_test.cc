#include "mmkv/algo/slist.h"

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
