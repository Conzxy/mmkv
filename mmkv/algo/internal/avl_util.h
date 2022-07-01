#ifndef _MMKV_ALGO_INTERNAL_AVL_UTIL_H_
#define _MMKV_ALGO_INTERNAL_AVL_UTIL_H_

#include "avl_node.h"

#include <assert.h>

using mmkv::algo::avl::AvlBaseNode;

namespace mmkv {
namespace algo {

/*
 * 仅用于AvlTree::Push()
 */
inline void _LinkTrack(AvlBaseNode** track, AvlBaseNode* node, AvlBaseNode* parent) noexcept {
  track[0] = node;
  track[0]->parent = parent;
}

/*
 * new_node取代old_node，与old_node的parent重链接(relink)，如果取代了根节点，将更新根节点
 */
inline void _ChildPlace(AvlBaseNode* old_node, AvlBaseNode* new_node, AvlBaseNode* parent, AvlBaseNode** root) noexcept {
  assert(old_node);
  if (parent) {
    if (old_node == parent->left)
      parent->left = new_node;
    else
      parent->right = new_node;
  } else {
    *root = new_node;
  }
}

/*
 * 以node为基点，node与右孩子的边为主轴，向左旋转，如此右孩子成为新根节点
 * 而node成为了左孩子
 */
inline AvlBaseNode* _RotateLeft(AvlBaseNode* node, AvlBaseNode** root) noexcept {
  AvlBaseNode* right = node->right;
  AvlBaseNode* parent = node->parent;
  assert(node && right);
  
  node->right = right->left;
  if (node->right)
    node->right->parent = node;
  right->left = node;
  right->parent = parent;
  node->parent = right;

  _ChildPlace(node, right, parent, root);

  return right;
}

/*
 * 左旋的对称操作
 */
inline AvlBaseNode* _RotateRight(AvlBaseNode* node, AvlBaseNode** root) noexcept {
  AvlBaseNode* left = node->left;
  AvlBaseNode* parent = node->parent;
  assert(left && node);

  node->left = left->right;
  if (node->left)
    node->left->parent = node;
  left->right = node;
  left->parent = parent;
  node->parent = left;

  _ChildPlace(node, left, parent, root);

  return left;
}

/*
 * 当然也可以内联（口味问题)
 */
#define LEFT_HEIGHT(node) (((node)->left) ? (node)->left->height : 0)
#define RIGHT_HEIGHT(node) (((node)->right) ? (node)->right->height : 0)
#define AVL_MAX(x, y) ((x) < (y) ? (y) : (x))

inline void _UpdateHeight(AvlBaseNode* node) noexcept {
  const int lh = LEFT_HEIGHT(node);
  const int rh = RIGHT_HEIGHT(node);

  node->height = AVL_MAX(lh, rh) + 1;
}

/*
 * 修复左重
 * 其关键操作是旋转，我们期望
 *           z                           y
 *          / \                         / \
 *         y  (h)                      x   z
 *        / \               ==>       / \ / \
 *       x  (h)                      h  h h  h
 *      / \
 *    (h) (h)
 *  (h)与h均表示高度为h的子树。
 *  这只是其中一种可能的case，以z为基点右旋将左子树的一个节点调整到右子树，
 *  显然前提条件是RIGHT_HEIGHT(y) - LEFT_HEIGHT(y) > 0，当右高比左高1的时候，
 *  先以y作为基点，左旋调整至上述case。
 */
inline AvlBaseNode* _FixupLeftHeavy(AvlBaseNode* node, AvlBaseNode** root) noexcept {
  AvlBaseNode* left = node->left;
  const int lh = LEFT_HEIGHT(left);
  const int rh = RIGHT_HEIGHT(left);

  if (lh < rh) {
    left = _RotateLeft(left, root);
    _UpdateHeight(left->left);
    _UpdateHeight(left);
  }

  node = _RotateRight(node, root);
  _UpdateHeight(node->right);
  _UpdateHeight(node);

  return node;
}

/*
 * 左重修复的对称操作
 */
inline AvlBaseNode* _FixupRigtHeavy(AvlBaseNode* node, AvlBaseNode** root) noexcept {
  AvlBaseNode* right = node->right;
  int lh = LEFT_HEIGHT(right);
  int rh = RIGHT_HEIGHT(right);

  if (lh > rh) {
    right = _RotateRight(right, root);
    _UpdateHeight(right->right);
    _UpdateHeight(right);
  }

  node= _RotateLeft(node, root);
  _UpdateHeight(node->left);
  _UpdateHeight(node);

  return node;
}

/*
 * 插入之后，新节点（叶子）可能破坏AVL性质，因此需要沿parent进行retracing，
 * 最坏情况，可能回溯到根节点从而使整体高度增高，不过随着高度越来越大，需要这样做的
 * 概率越来越小，即很难回溯到根节点（可以试想一下：什么样的树才会需要回溯到根节点）。
 *
 * 如果某个节点高度不再需要更新，表示再次平衡了，此时终止修复。
 */
inline void _InsertFixup(AvlBaseNode* node, AvlBaseNode** root) noexcept {
  int lh, rh, bf, h;

  for (; node; node = node->parent) {
    lh = LEFT_HEIGHT(node);
    rh = RIGHT_HEIGHT(node);
    h = AVL_MAX(lh, rh) + 1; 
    if (node->height == h) { break; }
    node->height = h;

    bf = lh - rh;

    if (bf >= 2) {
      node = _FixupLeftHeavy(node, root);
    } else if (bf <= -2) {
      node = _FixupRigtHeavy(node, root);
    }
  }
}

/*
 * 同插入一样，删除节点也会导致高度下降从而使|bf| > 1，因此此时也需要修复。
 * 与插入不同的是终止条件需要检查|bf| <= 1，因为删除之后高度不变仍然可能失衡
 */
inline void _EraseFixup(AvlBaseNode* node, AvlBaseNode** root) noexcept {
  int lh, rh, bf, h;

  while (node) {
    lh = LEFT_HEIGHT(node);
    rh = RIGHT_HEIGHT(node);
    h = AVL_MAX(lh, rh) + 1;

    bf = lh - rh;
    
    // if (node->height == h && bf >= -1 && bf <= -1) break; 
    // node->height = h // 该语句可以省略

    if (node->height != h)
      node->height = h;
    else if (bf >= -1 && bf <= 1) break;

    if (bf >= 2) {
      node = _FixupLeftHeavy(node, root);
    } else if (bf <= -2) {
      node = _FixupRigtHeavy(node, root);
    }

    node = node->parent;
  }
}

// \see CLRS 12.3 Deletion part
inline void _Erase(AvlBaseNode* node, AvlBaseNode** root) noexcept {
  AvlBaseNode* parent = node->parent;

  if (node->left && node->right) {
    // Find the minimum in the right subtree
    AvlBaseNode* old_node = node;
    AvlBaseNode* left;
    node = node->right;
    while ((left = node->left))
      node = left;
    
    if (node->parent == old_node) {
      node->left = old_node->left;
      old_node->left->parent = node;
      node->parent = old_node->parent;
      node->height = old_node->height;
      _ChildPlace(old_node, node, parent, root);
      // node可能失衡
    } else {
      AvlBaseNode* parent2 = node->parent;
      AvlBaseNode* right = node->right;
      _ChildPlace(node, right, parent2, root);
      if (right) right->parent = parent2;
      
      right = old_node->right;
      left = old_node->left;
      assert (right && left);
      node->right = right;
      right->parent = node;
      node->left = left;
      left->parent = node;
      node->height = old_node->height;
      node->parent = parent;
      _ChildPlace(old_node, node, parent, root);
      node = parent2;
      // 原来的parent可能失衡
    } 
  } else {
    AvlBaseNode* child = nullptr;

    if (node->left) child = node->left;
    else if (node->right) child = node->right;
    
    _ChildPlace(node, child, parent, root);
    if (child) child->parent = parent;
    node = parent;
    // parent可能失衡
  }
  // } else if (node->left) {
  //   _ChildPlace(node, node->left, node->parent, root);
  //   node->left->parent = node->parent;
  //   node = node->parent;
  // } else if (node->right) {
  //   _ChildPlace(node, node->right, node->parent, root);
  //   node->right->parent = node->parent;
  //   node = node->parent;
  // } else {
  //   _ChildPlace(node, nullptr, node->parent, root);
  //   node = node->parent;
  // }

  _EraseFixup(node, root);
}

/*************************************************************/
/* 迭代器接口                                                */
/*************************************************************/

// 最小值
inline AvlBaseNode* _GetFirstNode(AvlBaseNode* node) noexcept {
  if (!node) return nullptr;

  while (node->left) {
    node = node->left;
  }

  return node;
}

// 最大值
inline AvlBaseNode* _GetLastNode(AvlBaseNode* node) noexcept {
  if (!node) return nullptr;

  while (node->right) {
    node = node->right;
  }

  return node;
}

// 后驱：刚好比node大的节点
// \see CLRS(即算法导论) 12.2 Querying a binary search tree
inline AvlBaseNode* _GetNextNode(AvlBaseNode* node) noexcept {
  if (!node) return nullptr;
  if (node->right) {
    node = node->right;
    while (node->left)
      node = node->left;
  } else {
    // AvlBaseNode* parent;
    // for (;;) {
    //   if (!node) break; 
    //   parent = node->parent;
    //   if (parent->left == node) { node = parent; break; }
    //   node = parent;
    // }
    AvlBaseNode* cur;
    for (;;) {
      cur = node;
      node = node->parent;
      if (!node || node->left == cur) break;
    }
  }

  return node; 
}

// 前驱：刚好比node小的节点
// 实现与后驱对称
inline AvlBaseNode* _GetPrevNode(AvlBaseNode* node) noexcept {
  if (!node) return nullptr;

  if (node->left) {
    node = node->left;
    while (node->right)
      node = node->right;
  } else {
    AvlBaseNode* cur;
    for (;;) {
      cur = node;
      node = node->parent;
      if (!node || node->right == cur) break;
    }
  }

  return node;
}

} // algo
} // mmkv

#endif // _MMKV_ALGO_INTERNAL_AVL_UTIL_H_
