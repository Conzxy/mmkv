/************************************************************/
/* Created on 2022/7/5                                      */
/************************************************************/

#ifndef _MMKV_DB_VSET_H_
#define _MMKV_DB_VSET_H_

#include "mmkv/algo/avl_tree.h"
#include "mmkv/algo/dictionary.h"
#include "mmkv/algo/key_value.h"
#include "mmkv/db/type.h"

#include <assert.h>

namespace mmkv {
namespace db {

using algo::AvlTree;
using algo::Dictionary;
using algo::KeyValue;
using protocol::WeightValues;

/*
 * V is taken from AVL tree
 */
class Vset {
 private:
  struct DoubleComparator {
    int operator()(double x, double y) const noexcept {
      return (x > y) ? 1 : ((x == y) ? 0 : -1);
    }
  };
  
  using Tree = AvlTree<double, KeyValue<double, String*>, DoubleComparator>;

 public:
  Vset() = default;
  explicit Vset(WeightValues& values) {
    for (auto& wm : values)
      Insert(wm.key, std::move(wm.value));
  }

  ~Vset() = default;

  bool Insert(Weight w, String member);

  bool Erase(String const& member);
  size_t EraseRange(int64_t left, int64_t right);
  size_t EraseRangeByWeight(Weight left, Weight right);

  bool GetWeight(String const& member, Weight& w);
  bool GetOrder(String const& member, size_t& order); 
  bool GetROrder(String const& member, size_t& order);

  size_t GetSize() const noexcept { assert(tree_.size() == dict_.size()); return tree_.size(); }

  size_t GetSizeByWeight(Weight left, Weight right);
  void GetRange(int64_t left, int64_t right, WeightValues& values);
  void GetAll(WeightValues& values);
  void GetRangeByWeight(Weight left, Weight right, WeightValues& values);
  void GetRRange(int64_t left, int64_t right, WeightValues& values);
  void GetRRangeByWeight(Weight left, Weight right, WeightValues& values);

  Tree& tree() noexcept { return tree_; } 
 private: 
  Tree tree_;
  Dictionary<String, double> dict_;
};

} // db
} // mmkv

#endif // _MMKV_DB_VSET_H_
