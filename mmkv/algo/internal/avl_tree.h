/***********************************************************************/
/* Date: 2022/6/29                                                     */
/***********************************************************************/

#ifndef _MMKV_ALGO_AVL_TREE_H_
#define _MMKV_ALGO_AVL_TREE_H_

#ifdef _AVL_TREE_DEBUG_
#include <iostream>
#endif

#include "mmkv/algo/libc_allocator_with_realloc.h"
#include "func_util.h"
#include "avl_node.h"
#include "avl_tree_iterator.h"
#include "avl_util.h"

namespace mmkv {
namespace algo {

namespace avl {

template <typename T, typename Alloc>
using AvlNodeAllocator = typename Alloc::template rebind<AvlNode<T>>::other;

} // avl

#define TO_COMPARE (*((Compare*)this))
#define TO_NODE(basenode) ((Node*)(basenode))

template<typename K, typename V, typename Compare, typename Alloc=LibcAllocatorWithRealloc<V>>
class AvlTree : protected avl::AvlNodeAllocator<V, Alloc> 
              , protected Compare {
  using NodeAllocTraits = std::allocator_traits<avl::AvlNodeAllocator<V, Alloc>>;
  using BaseNode = avl::AvlBaseNode;
 public:
  using Node = avl::AvlNode<V>;
  using key_type = K;
  using value_type = V;
  using size_type = size_t;
  using iterator = AvlTreeIterator<V>;
  using const_iterator = AvlTreeConstIterator<V>;

  AvlTree()
    : root_(nullptr)
    , count_(0)
  {
  }

  ~AvlTree() noexcept;
  
  bool Insert(value_type const& value) {
    return _Insert(value);
  }

  bool Insert(value_type&& value) {
    return _Insert(std::move(value));
  }

  bool Push(Node* node) noexcept;
  
  bool Erase(K const& key);

  Node* Extract(K const& key) noexcept; 

  // GetAllValues() noexcept;
  Node* FindNode(K const& key) noexcept(noexcept(TO_COMPARE(key, key))) {
    BaseNode* node = root_;
    int res;

    while (node) {
      res = TO_COMPARE(get_key(TO_NODE(node)->value), key);

      if (res > 0)
        node = node->left;
      else if (res < 0)
        node = node->right;
      else
        return TO_NODE(node);
    }

    return nullptr;
  } 

  V* Find(K const& key) noexcept(noexcept(FindNode(key))) {
    auto ret = FindNode(key);
    return ret ? &ret->value : (V*)nullptr;
  }
  
  void Clear();

  size_t size() const noexcept { return count_; }
  bool empty() const noexcept { return count_ == 0; }

#ifdef _AVL_TREE_DEBUG_
  int CalculateHeight(BaseNode const* node) const noexcept {
    if (!node) return 0;

    return 1 + AVL_MAX(CalculateHeight(node->left), CalculateHeight(node->right));
  }
  
  int Height() const noexcept { return CalculateHeight(root_); }

  bool VerifyAvlProperties() {
    Node* node = TO_NODE(_GetFirstNode(root_));
    int lh, rh;
    bool ret = true;

    for (; node; node = TO_NODE(_GetNextNode(node))) {
      if (node->left) {
        if (TO_COMPARE(TO_NODE(node->left)->value, node->value) >= 0) {
          std::cout << "violate(left < mid): " << TO_NODE(node->left)->value << " >= " << node->value << "\n";
          ret = false;
        }
      }

      if (node->right) {
        if (TO_COMPARE(TO_NODE(node->right)->value, node->value) <= 0) {
          std::cout << "violate(right > mid): " << TO_NODE(node->right)->value << " <= " << node->value << "\n";
          ret = false;
        }
      }

      lh = CalculateHeight(node->left);
      rh = CalculateHeight(node->right);

      if (lh - rh >= 2 || lh - rh <= -2) {
        std::cout << "violate(bf): " << lh - rh << "\n";
        ret = false;
      }
    }

    return ret;
  }
 private:
  void PrintRoot(Node const* root, std::ostream& os) const {
    os << root->value << "\n";
  }

  void PrintSubTree(Node const* root, std::ostream& os, std::string const& prefix="") const {
    if(!root) return;

    bool has_right = root->right;
    bool has_left = root->left;
	
    if(!has_right && !has_left) return;
    
    os << prefix;

    if(has_right && has_left)
        os << "├── ";
    if(has_right && ! has_left)
        os << "└── ";

    if(has_right){
      PrintRoot(TO_NODE(root->right), os);
      if (has_left && (root->right->right || root->right->left))
        PrintSubTree(TO_NODE(root->right), os, prefix + "|   ");
      else
        PrintSubTree(TO_NODE(root->right), os, prefix + "    ");
    }

    if(has_left) {
        os << ((has_right) ? prefix : "") << "└───";
		    PrintRoot(TO_NODE(root->left), os);
        PrintSubTree(TO_NODE(root->left), os, prefix + "    "); 
    }
}
 public:
  void Print(std::ostream& os) const {
    if(!root_) return ;

    PrintRoot(TO_NODE(root_), os);
    PrintSubTree(TO_NODE(root_), os);
  }
#endif
  
  iterator begin() noexcept { return (Node*)_GetFirstNode(root_); }
  const_iterator begin() const noexcept { return (Node*)_GetFirstNode(root_); }
  iterator end() noexcept { return nullptr; }
  const_iterator end() const noexcept { return nullptr; }

  const_iterator cbegin() const noexcept { return begin(); }
  const_iterator cend() const noexcept { return end(); }
  
  iterator before_end() noexcept { return (Node*)_GetLastNode(root_); }
  const_iterator before_end() const noexcept { return (Node*)_GetLastNode(root_); }
  const_iterator cbefore_end() const noexcept { return (Node*)_GetLastNode(root_); }

 private:
  template<typename T>
  bool _Insert(T&& value);

  template<typename... Args>
  Node* CreateNode(Args&&... args) {
    auto node = NodeAllocTraits::allocate(*this, 1);
    NodeAllocTraits::construct(*this, &node->value, std::forward<Args>(args)...);
    node->left = nullptr;
    node->right = nullptr;
    node->parent = nullptr;
    node->height = 1;

    return node;
  }
  
  void DropNode(Node* node) {
    NodeAllocTraits::destroy(*this, node);
    NodeAllocTraits::deallocate(*this, node, 1);
  }
   
  BaseNode* root_ = nullptr;
  size_t count_ = 0;
};

} // algo
} // mmkv

#endif // _MMKV_ALGO_AVL_TREE_H_
