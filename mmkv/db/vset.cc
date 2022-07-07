#include "vset.h"

#include "mmkv/db/kvdb.h"
#include "mmkv/protocol/mmbp.h"

using namespace mmkv::db;
using namespace mmkv::protocol;

#define VSET_MIN(x, y) ((x) < (y) ? (x) : (y))

bool Vset::Insert(Weight w, String member) {
  auto kv = dict_.InsertKv(std::move(member), w);
  if (!kv) return false;

  tree_.InsertEq({w, &kv->key});
  return true;
}

bool Vset::Erase(String const& member) {
  auto node = dict_.Extract(member);
  if (node == nullptr) return false;

  tree_.Erase(node->value.value, [node](Tree::value_type const& value) {
    return *value.value == node->value.key;
  });

  dict_.FreeNode(node);
  return true;
}

#define N2P(x) ((x) < 0 ? tree_.size() - (x) : (x))

size_t Vset::EraseRange(int64_t left, int64_t right) {
  left = N2P(left);
  right = N2P(right);
  right = VSET_MIN((size_t)right, tree_.size());
  if (left >= right) return 0;

  size_t ret = 0;
  auto count = right - left + 1;
  Tree::iterator iter;
  
  // reuse
  right = tree_.size() - 1 - right;

  if (left < right) {
    iter = tree_.begin();
    while (left--) ++iter;
    while (count--) {
      ret += (size_t)tree_.Erase(iter++);
    }
  } else {
    iter = tree_.before_end();
    while (right--) --iter;
    while (count--) {
      ret += (size_t)tree_.Erase(iter--);
    }
  }

  return ret;
}

size_t Vset::EraseRangeByWeight(Weight left, Weight right) {
  auto left_bound = tree_.LowerBound(left);
  if (left_bound == tree_.end()) return 0;
  auto right_bound = tree_.UpperBound(right);
  
  size_t ret = 0;
  for (;;) {
    tree_.Erase(left_bound++);
    ret++; 
    if (left_bound == right_bound) break;
  }

  return ret;
}

bool Vset::GetWeight(String const& member, Weight& w) {
  auto kv = dict_.Find(member);
  if (!kv) return false;

  w = kv->value;
  return true;
}

bool Vset::GetOrder(String const& member, size_t& order) {
  Weight w;
  if (!GetWeight(member, w)) return false;

  auto first = tree_.begin();
  order = 0;
  for (;;) {
    if (first->key == w && *first->value == member) break;
    order++;
    ++first;
  }
  
  return true;
}

bool Vset::GetROrder(String const& member, size_t& order) {
  Weight w;
  if (!GetWeight(member, w)) return false;

  auto first = tree_.before_end();
  order = 0;
  for (;;) {
    if (first->key == w && *first->value == member) break;
    order++;
    --first;
  }
  
  return true;

}

size_t Vset::GetSizeByWeight(Weight left, Weight right) {
  auto first = tree_.LowerBound(left);
  if (first == tree_.end()) return 0;
  auto last = tree_.UpperBound(right);

  return std::distance(first, last);
}

void Vset::GetRange(int64_t left, int64_t right, WeightValues& values) {
  assert(values.empty());
  left = N2P(left);
  right = N2P(right);
  
  Tree::iterator iter;

  right = VSET_MIN(tree_.size(), (size_t)right);
  auto count = right - left + 1;

  iter = tree_.begin();
  while (left--) ++iter; 

  for (; iter != tree_.end() && count--; ++iter) {
    values.push_back({iter->key, *(iter->value)});
  }
}

void Vset::GetRangeByWeight(Weight left, Weight right, WeightValues& values) {
  assert(values.empty());
  
  auto left_bound = tree_.LowerBound(left);
  if (left_bound == tree_.end()) return;

  auto right_bound = tree_.UpperBound(right);

  for (;;) {
    values.push_back({left_bound->key, *(left_bound->value)});
    ++left_bound;
    if (left_bound == right_bound) break;
  }
}

void Vset::GetRRange(int64_t left, int64_t right, WeightValues& values) {
  left = N2P(left);
  right = N2P(right);
  right = VSET_MIN((size_t)right, tree_.size());
  
  auto count = right - left + 1;
  auto iter = tree_.before_end();
  auto first = tree_.begin();
  
  right = tree_.size() - right;

  while (left--) --iter;

  for (; count--;) {
    values.push_back({iter->key, *(iter->value)});
    if (iter == first) break;
    --iter;
  }
}

void Vset::GetRRangeByWeight(Weight left, Weight right, WeightValues& values) {
  auto left_bound = tree_.LowerBound(left);
  auto right_bound = tree_.UpperBound(right);

  for (;;) {
    --right_bound;
    values.push_back({right_bound->key, *(right_bound->value)});
    if (left_bound == right_bound) break;
  }
}

void Vset::GetAll(WeightValues& values) {
  tree_.DoInAll([&values](Tree::value_type const& wm) {
    values.push_back({wm.key, *(wm.value)});
  });
}
