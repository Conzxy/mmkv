#include "mmkv/db/kvdb.h"
#include "mmkv/db/vset.h"
#include "mmkv/db/type.h"
#include <vector>

#include <gtest/gtest.h>

using namespace mmkv::db;
using namespace std;

TEST(vset, insert) {
  Vset vs;

  EXPECT_TRUE(vs.Insert(1.0, "A"));
  EXPECT_FALSE(vs.Insert(2.0, "A"));
  
  Weight w;
  uint32_t order;
  EXPECT_TRUE(vs.GetWeight("A", w));
  EXPECT_EQ(w, 1.0);

  EXPECT_TRUE(vs.GetOrder("A", order));
  EXPECT_EQ(order, 0);
  EXPECT_TRUE(vs.GetROrder("A", order));
  EXPECT_EQ(order, 0);
}

class VsetTest : public ::testing::Test {
 protected:
  void SetUp() noexcept override {
    /*
     * 0.1 C
     * 0.3 D
     * 1.0 A
     * 2.0 B
     * 4.5 F
     * 5.3 E
     */
    EXPECT_TRUE(set.Insert(1.0, "A"));
    EXPECT_TRUE(set.Insert(2.0, "B"));
    EXPECT_TRUE(set.Insert(0.1, "C"));
    EXPECT_TRUE(set.Insert(0.3, "D"));
    EXPECT_TRUE(set.Insert(5.3, "E"));
    EXPECT_TRUE(set.Insert(4.5, "F"));
    EXPECT_FALSE(set.Insert(0.4, "D"));

    for (auto const& kv : set.tree()) {
      weights.push_back(kv.key);
      members.push_back(*(kv.value));
      //std::cout << "(" << kv.key << "," << *kv.value << ")\n";
    }
  }

  Vset set;
  vector<double> weights;
  vector<String> members;
  WeightValues values;
};

TEST_F(VsetTest, getweight) {
  Weight w;
  EXPECT_TRUE(set.GetWeight("C", w));
  EXPECT_EQ(w, 0.1);
  EXPECT_TRUE(set.GetWeight("D", w));
  EXPECT_EQ(w, 0.3);
  EXPECT_TRUE(set.GetWeight("A", w));
  EXPECT_EQ(w, 1.0);
  EXPECT_TRUE(set.GetWeight("B", w));
  EXPECT_EQ(w, 2.0);
  EXPECT_TRUE(set.GetWeight("F", w));
  EXPECT_EQ(w, 4.5);
  EXPECT_TRUE(set.GetWeight("E", w));
  EXPECT_EQ(w, 5.3);
}

TEST_F(VsetTest, getorder) {
  uint32_t order;
  EXPECT_TRUE(set.GetOrder("C", order));
  EXPECT_EQ(order, 0);
  EXPECT_TRUE(set.GetOrder("D", order));
  EXPECT_EQ(order, 1);
  EXPECT_TRUE(set.GetOrder("A", order));
  EXPECT_EQ(order, 2);
  EXPECT_TRUE(set.GetOrder("B", order));
  EXPECT_EQ(order, 3);
  EXPECT_TRUE(set.GetOrder("F", order));
  EXPECT_EQ(order, 4);
  EXPECT_TRUE(set.GetOrder("E", order));
  EXPECT_EQ(order, 5);
}

TEST_F(VsetTest, getrorder) {
  uint32_t order;
  EXPECT_TRUE(set.GetROrder("C", order));
  EXPECT_EQ(order, 5);
  EXPECT_TRUE(set.GetROrder("D", order));
  EXPECT_EQ(order, 4);
  EXPECT_TRUE(set.GetROrder("A", order));
  EXPECT_EQ(order, 3);
  EXPECT_TRUE(set.GetROrder("B", order));
  EXPECT_EQ(order, 2);
  EXPECT_TRUE(set.GetROrder("F", order));
  EXPECT_EQ(order, 1);
  EXPECT_TRUE(set.GetROrder("E", order));
  EXPECT_EQ(order, 0);
}

TEST_F(VsetTest, getsizebyweight) {
  EXPECT_EQ(set.GetSizeByWeight(0.0, 5.4), 6);
  EXPECT_EQ(set.GetSizeByWeight(0.4, 4.6), 3);
}

TEST_F(VsetTest, getrangebyweight) {
  set.GetRangeByWeight(0.4, 2.3, values);  
  for (int i = 2; i < 4; ++i) {
    EXPECT_EQ(values[i-2].key, weights[i]);
    EXPECT_EQ(values[i-2].value, members[i]);
  }
}

TEST_F(VsetTest, getrange) {
  set.GetRange(1, 4, values);
  
  for (int i = 1; i < 4; ++i) {
    EXPECT_EQ(values[i-1].key, weights[i]);
    EXPECT_EQ(values[i-1].value, members[i]);
  } 
}

std::ostream& operator<<(std::ostream& os, WeightValues const& values) {
  for (auto const& kv : values) {
    os << "(" << kv.key << "," << kv.value << ")\n";
  }

  return os;
}

TEST_F(VsetTest, getrrange) {
  set.GetRRange(1, 3, values);
  
  std::cout << values;
}

TEST_F(VsetTest, getrrangebyweight) {
  set.GetRRangeByWeight(2.0, 5.0, values);
  
  std::cout << values;  
}

std::ostream& operator<<(std::ostream& os, Vset& set) {
  for (auto const& kv : set.tree()) {
    os << "(" << kv.key << "," << *kv.value << ")\n";
  }

  return os;
}

TEST_F(VsetTest, eraserange) {
  EXPECT_EQ(3, set.EraseRange(1, 3));
  std::cout << set;
}

TEST_F(VsetTest, eraserange2) {
  EXPECT_EQ(3, set.EraseRange(3, 5));
  std::cout << set;
}

TEST_F(VsetTest, eraserangebyweight) {
  EXPECT_EQ(3, set.EraseRangeByWeight(0.2, 2.4));
  std::cout << set;
}
