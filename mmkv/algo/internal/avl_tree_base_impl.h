#ifndef _MMKV_ALGO_INTERNAL_AVL_TREE_BASE_IMPL_H_
#define _MMKV_ALGO_INTERNAL_AVL_TREE_BASE_IMPL_H_

#include "avl_tree_base.h"

#include <vector>

namespace mmkv {
namespace algo {

#define AVL_TEMPLATE \
  template<typename K, typename V, typename Compare, typename GK, typename A, typename D>

#define AVL_CLASS \
  AvlTreeBase<K, V, Compare, GK, A, D>

#define ARGS_TEMPLATE \
  template<typename... Args>

#define VALUE_TO_NODE CreateNode(std::forward<T>(value))
#define NODE typename AVL_CLASS::Node

AVL_TEMPLATE
AVL_CLASS::~AvlTreeBase() noexcept {
  Clear();
}

// \see CLRS 12.3 Insertion part
AVL_TEMPLATE
inline bool AVL_CLASS::Push(Node* node) noexcept {
  return PushWithDuplicate(node, nullptr);
}

AVL_TEMPLATE
inline bool AVL_CLASS::PushWithDuplicate(Node* node, value_type** duplicate) {
  if (!root_) {
    root_ = node;
    if (duplicate) *duplicate = std::addressof(NODE2VALUE(root_));
  } else {
    if (!root_->left && !root_->right) {
      auto res = TO_COMPARE(TO_GK(node->value), TO_GK(NODE2VALUE(root_)));
      BaseNode** slot = nullptr;
      if (res > 0) {
        slot = &root_->right;
      } else if (res < 0) {
        slot = &root_->left;
      } else {
        if (duplicate) *duplicate = std::addressof(NODE2VALUE(slot[0]));
        return false;
      }

      *slot = node;
      (*slot)->parent = root_;
      if (duplicate) *duplicate = std::addressof(NODE2VALUE(slot[0]));

      return true;
    }

    BaseNode* parent = nullptr; 
    // slot最终会指向空槽（slot）
    // 采用二级指针，避免了定义表示插入哪边的元数据
    BaseNode** slot = &root_;
    
    int res; 
    
    while (slot[0]) {
      res = TO_COMPARE(TO_GK(NODE2VALUE(slot[0])), TO_GK(node->value));
      parent = slot[0];

      if (res > 0) {
        slot = &(slot[0]->left);
      } else if (res < 0) {
        slot = &(slot[0]->right);
      } else {
        if (duplicate) *duplicate = std::addressof(NODE2VALUE(slot[0]));
        return false;
      }
    }
    
    // 由于这两个语句与value无关，我把它分出去了
    _LinkSlot(slot, node, parent); 
    if (duplicate) *duplicate = std::addressof(NODE2VALUE(slot[0]));
    _InsertFixup(parent, &root_);
  }

  IncreaseCount();
  return true;

}
AVL_TEMPLATE
inline void AVL_CLASS::PushEq(Node* node) noexcept {
  if (!root_) {
    root_ = node;
    IncreaseCount();
    return;
  }

  BaseNode* parent = nullptr; 
  BaseNode** slot = &root_;
  
  int res; 
  
  while (slot[0]) {
    res = TO_COMPARE(TO_GK(NODE2VALUE(slot[0])), TO_GK(node->value));
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
  IncreaseCount();
}

AVL_TEMPLATE
template<typename T>
inline bool AVL_CLASS::_InsertWithDuplicate(T&& value, value_type** dup) {
  if (!root_) {
    root_ = VALUE_TO_NODE;
    if (dup) *dup = std::addressof(NODE2VALUE(root_));
  } else {
    if (!root_->left && !root_->right) {
      auto res = TO_COMPARE(TO_GK(value), TO_GK(NODE2VALUE(root_)));
      BaseNode** slot = nullptr;
      if (res > 0) {
        slot = &root_->right;
      } else if (res < 0) {
        slot = &root_->left;
      } else {
        if (dup) *dup = std::addressof((NODE2VALUE(root_)));
        return false;
      }

      *slot = VALUE_TO_NODE;
      (*slot)->parent = root_;
      if (dup) *dup = std::addressof((NODE2VALUE(*slot)));

      return true;
    }

    BaseNode* parent = nullptr; 
    // slot最终会指向空槽（slot）
    // 采用二级指针，避免了定义表示插入哪边的元数据
    BaseNode** slot = &root_;
    
    int res; 
    
    while (slot[0]) {
      res = TO_COMPARE(TO_GK(((Node*)slot[0])->value), TO_GK(value));
      parent = slot[0];

      if (res > 0) {
        slot = &(slot[0]->left);
      } else if (res < 0) {
        slot = &(slot[0]->right);
      } else {
        if (dup) *dup = std::addressof(NODE2VALUE(slot[0]));
        return false;
      }
    }
    
    _LinkSlot(slot, VALUE_TO_NODE, parent);
    // !!!
    // 下面这句不能放在_InsertFixup()后，因为slot指向parent的孩子
    // 而Fixup可能进行旋转，从而可能改变其孩子的相对位置，因此slot[0]得不到原来的节点
    // 所以在Fixup之前先记录value的地址
    if (dup) *dup = std::addressof(NODE2VALUE(slot[0]));
    _InsertFixup(parent, &root_);
  }

  IncreaseCount();
  return true;
}

AVL_TEMPLATE
template<typename T>
inline bool AVL_CLASS::_Insert(T&& value) {
  return _InsertWithDuplicate(std::forward<T>(value), nullptr);
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
  DecreaseCount();

  node->parent = node->left = node->right = nullptr;
  return node;
}

AVL_TEMPLATE
typename AVL_CLASS::Node* AVL_CLASS::Extract() noexcept {
  BaseNode* ret = root_;
  
  if (!root_) return nullptr;

  if (root_->left) {
    root_ = root_->left;
    BaseNode** child = &root_->right;
    while (*child)
      child = &(*child)->right;
    *(child) = ret->right;
  } else if (root_->right) {
    root_ = root_->right;
    BaseNode** child = &root_->left;
    while (*child)
      child = &(*child)->left;
    *(child) = ret->left;
  } else {
    root_ = nullptr;
    assert(ret->parent == nullptr);
    return (Node*)ret;
  }

  root_->parent = nullptr;

  ret->parent = ret->left = ret->right = nullptr;
  return (Node*)ret;
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
  return EraseNode(pos.node_);
}

AVL_TEMPLATE
bool AVL_CLASS::EraseNode(Node* node) {
  if (node == nullptr) return false;
  EraseRoutine(node);
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
    if (TO_COMPARE(TO_GK(node->value), key)) break;
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

  root_ = nullptr;
  SetCount(0);
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

#endif // _MMKV_ALGO_INTERNAL_AVL_TREE_BASE_IMPL_H_
