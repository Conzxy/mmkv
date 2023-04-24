// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_ALGO_INTERNAL_AVL_NODE_H_
#define _MMKV_ALGO_INTERNAL_AVL_NODE_H_

namespace mmkv {
namespace algo {
namespace avl {

struct AvlBaseNode {
  struct AvlBaseNode* parent;
  struct AvlBaseNode* left;
  struct AvlBaseNode* right;
  int height;
};

template<typename T>
struct AvlNode : AvlBaseNode {
  T value;
};

} // avl
} // algo
} // mmkv

#endif // _MMKV_ALGO_INTERNAL_AVL_NODE_H_
