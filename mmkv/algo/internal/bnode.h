#ifndef _MMKV_ALGO_INTERNAL_BNODE_H_
#define _MMKV_ALGO_INTERNAL_BNODE_H_

namespace mmkv {
namespace algo {
namespace blist {

template<typename T>
struct BNode {
  using LinkType = BNode*;
  T value; 
  LinkType prev;
  LinkType next; 
};

} // blist
} // algo
} // mmkv

#endif // _MMKV_ALGO_INTERNAL_BNODE_H_