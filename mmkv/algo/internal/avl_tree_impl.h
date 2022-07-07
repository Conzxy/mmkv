#ifndef _MMKV_ALGO_INTERNAL_AVL_TREE_IMPL_H_
#define _MMKV_ALGO_INTERNAL_AVL_TREE_IMPL_H_

#include "avl_tree.h"
#include "avl_util.h"

#include <vector>

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
    // slot最终会指向空槽（slot）
    // 采用二级指针，避免了定义表示插入哪边的元数据
    BaseNode** slot = &root_;
    
    int res; 
    
    while (slot[0]) {
      res = TO_COMPARE(get_key(NODE2VALUE(slot[0])), get_key(node->value));
      parent = slot[0];

      if (res > 0) {
        slot = &(slot[0]->left);
      } else if (res < 0) {
        slot = &(slot[0]->right);
      } else {
        return false;
      }
    }
    
    // 由于这两个语句与value无关，我把它分出去了
    _LinkSlot(slot, node, parent); 
    _InsertFixup(parent, &root_);
  }

  ++count_;
  return true;
}

AVL_TEMPLATE
inline void AVL_CLASS::PushEq(Node* node) noexcept {
  if (!root_) {
    root_ = node;
    ++count_;
    return;
  }

  BaseNode* parent = nullptr; 
  BaseNode** slot = &root_;
  
  int res; 
  
  while (slot[0]) {
    res = TO_COMPARE(get_key(NODE2VALUE(slot[0])), get_key(node->value));
    parent = slot[0];

    if (res > 0) {
      slot = &(slot[0]->left);
    } else if (res < 0) {
      slot = &(slot[0]->right);
    } else {
      if (LEFT_HEIGHT(slot[0]) < RIGHT_HEIGHT(slot[0])) {
        slot = &(slot[0]->left);
      } else {
        slot = &(slot[0]->right);
      }
    }
  }
  
  _LinkSlot(slot, node, parent); 
  _InsertFixup(parent, &root_);
  ++count_; 
}

AVL_TEMPLATE
template<typename T>
inline bool AVL_CLASS::_Insert(T&& value) {
  if (!root_) {
    root_ = VALUE_TO_NODE;
  } else {
    BaseNode* parent = nullptr; 
    // slot最终会指向空槽（slot）
    // 采用二级指针，避免了定义表示插入哪边的元数据
    BaseNode** slot = &root_;
    
    int res; 
    
    while (slot[0]) {
      res = TO_COMPARE(get_key(((Node*)slot[0])->value), get_key(value));
      parent = slot[0];

      if (res > 0) {
        slot = &(slot[0]->left);
      } else if (res < 0) {
        slot = &(slot[0]->right);
      } else {
        return false;
      }
    }
    
    _LinkSlot(slot, VALUE_TO_NODE, parent); 
    _InsertFixup(parent, &root_);
  }

  ++count_;
  return true;
}

AVL_TEMPLATE
template<typename T>
inline void AVL_CLASS::_InsertEq(T&& value) {
  PushEq(VALUE_TO_NODE);
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
  
  EraseRoutine(node); 
  return true;
}

AVL_TEMPLATE
bool AVL_CLASS::Erase(const_iterator pos) {
  if (pos.node_ == nullptr) return false;
  EraseRoutine(pos.node_);
  return true;
}

AVL_TEMPLATE
template<typename Pred>
bool AVL_CLASS::Erase(K const& key, Pred pred) {
  auto node = FindNode(key);
  if (!node) return false;

  for (;;) {
    if (pred(node->value)) {
      EraseRoutine(node);
      return true;
    }

    node = (Node*)_GetNextNode(node);
    if (TO_COMPARE(get_key(node->value), key)) break;
  }

  return false;
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

AVL_TEMPLATE
template<typename Cb>
void AVL_CLASS::DoInAll(Cb cb) {
  std::vector<BaseNode*> nodes;
  auto root = root_;

  while (root || !nodes.empty()) {
    while (root) {
      nodes.push_back(root);
      root = root->left;
    }

    root = nodes.back(); nodes.pop_back();    
    cb(NODE2VALUE(root));
    
    root = (Node*)root->right;
  }
}

} // algo
} // mmkv


#endif // _MMKV_ALGO_INTERNAL_AVL_TREE_IMPL_H_
