#ifndef _MMKV_ALGO_SNODE_H_
#define _MMKV_ALGO_SNODE_H_

#include <utility>

namespace mmkv {
namespace algo {

template<typename T>
struct SNode {
  SNode* next;
  T value;
  
  SNode()
    : next(nullptr), value()
  { } 
  
  explicit SNode(SNode* nxt)
    : next(nxt), value()
  { }

  explicit SNode(T const& val, SNode* nxt=nullptr)
    : next(nxt), value(val)
  { }

  explicit SNode(T&& val, SNode* nxt=nullptr)
    : next(nxt), value(std::move(val))
  { }
};

} // algo
} // mmkv

#endif // _MMKV_ALGO_SNODE_H_
