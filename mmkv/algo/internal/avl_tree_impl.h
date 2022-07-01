#ifndef _MMKV_ALGO_INTERNAL_AVL_TREE_IMPL_H_
#define _MMKV_ALGO_INTERNAL_AVL_TREE_IMPL_H_

#include "avl_tree.h"
#include "avl_util.h"

namespace mmkv {
namespace algo {

#define AVL_TEMPLATE \
  template<typename K, typename V, typename Compare, typename A>

#define AVL_CLASS \
  AvlTree<K, V, Compare, A>

#define ARGS_TEMPLATE \
  template<typename... Args>

#define VALUE_TO_NODE CreateNode(std::forward<T>(value))
#define NODE typename AVL_CLASS::Node

AVL_TEMPLATE
AVL_CLASS::~AvlTree() noexcept {
  Clear();
}

// \see CLRS 12.3 Insertion part
AVL_TEMPLATE
bool AVL_CLASS::Push(Node* node) noexcept {
  if (!root_) {
    root_ = node;
  } else {
    BaseNode* parent = nullptr; 
    // track最终会指向空槽（slot）
    // 采用二级指针，避免了定义表示插入哪边的元数据
    BaseNode** track = &root_;
    
    int res; 
    
    while (track[0]) {
      res = TO_COMPARE(get_key(((Node*)track[0])->value), get_key(node->value));
      parent = track[0];

      if (res > 0) {
        track = &(track[0]->left);
      } else if (res < 0) {
        track = &(track[0]->right);
      } else {
        return false;
      }
    }
    
    // 由于这两个语句与value无关，我把它分出去了
    _LinkTrack(track, node, parent); 
    _InsertFixup(parent, &root_);
  }

  ++count_;
  return true;
}

AVL_TEMPLATE
template<typename T>
bool AVL_CLASS::_Insert(T&& value) {
  return Push(VALUE_TO_NODE);
}

AVL_TEMPLATE
NODE* AVL_CLASS::Extract(K const& key) noexcept {
  auto node = FindNode(key);
  if (!node) {
    return nullptr;
  }
  
  _Erase(node, &root_);
  --count_;

  return node;
}

AVL_TEMPLATE
bool AVL_CLASS::Erase(K const& key) {
  // 实际可以转发给Extract
  // 这里采用hard code的手法避免一个变量的定义以及if
  auto node = FindNode(key);
  if (!node) {
    return false;
  }
  
  _Erase(node, &root_);
  DropNode(node);
  --count_;

  return true;
}

AVL_TEMPLATE
void AVL_CLASS::Clear() {
  auto node = root_;
  BaseNode* parent = nullptr;
  
  // 深度优先删除左子树和右子树，然后上升到根节点
  // 相比于采用迭代器迭代删除要快那么一些(尽管都是O(nlgn))
  for (; node; ) {
    if (node->left) {
      node = node->left;
    } else if (node->right) {
      node = node->right;
    } else {
      parent = node->parent;
      
      if (!parent) break;
      else if (parent->left == node)
        parent->left = nullptr;
      else
        parent->right = nullptr;

      parent->left = nullptr;
      DropNode((Node*)node);
      node = parent;
    }
  }

  count_ = 0;
}

} // algo
} // mmkv


#endif // _MMKV_ALGO_INTERNAL_AVL_TREE_IMPL_H_
