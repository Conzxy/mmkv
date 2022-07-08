/***********************************************************************/
/* Date: 2022/6/29                                                     */
/***********************************************************************/

#ifndef _MMKV_ALGO_INTERNAL_AVL_TREE_H_
#define _MMKV_ALGO_INTERNAL_AVL_TREE_H_

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
#define NODE2VALUE(node) (((Node*)(node))->value)

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
  
  AvlTree(AvlTree&& o) noexcept 
    : root_(o.root_)
    , count_(o.count_) {
    o.root_ = nullptr;
    o.count_ = 0;
  }
  
  AvlTree& operator=(AvlTree&& o) noexcept {
    o.swap(*this);
    return *this;
  }
  
  void swap(AvlTree& o) noexcept {
    std::swap(o.root_, root_);
    std::swap(o.count_, count_);
  }

  /**
   * \brief Insert unique entry
   * \return 
   *   true -- success
   *   false -- failure
   */
  bool Insert(value_type const& value) { return _Insert(value); }
  bool Insert(value_type&& value) { return _Insert(std::move(value)); }

  /**
   * \brief Insert entry(although key exists)
   */
  void InsertEq(value_type const& value) { _InsertEq(value); }
  void InsertEq(value_type&& value) { _InsertEq(std::move(value)); }

  /**
   * \brief Push node returnd by extract()
   * \return 
   *   true -- success
   *   false -- failure
   */
  bool Push(Node* node) noexcept;

  /**
   * \brief Push node returnd by extract()[although key exists]
   */
  void PushEq(Node* node) noexcept;

  /**
   * \brief Erase entry whose key equals to the \p key and value 
   *        satisfies the given predicate
   * 
   * This is a heterogeneous erase method
   * \return 
   *   true -- success
   *   false -- such entry does not exists
   */
  template<typename ValuePred> 
  bool Erase(K const& key, ValuePred pred);

  /**
   * \brief Erase entry whose key equal to the \p key parameter
   * 
   * \return 
   *   true -- success
   *   false -- such entry does not exists
   */
  bool Erase(K const& key);

  bool Erase(const_iterator pos);

  /**
   * \brief Extract the node from avl tree
   * 
   * This is a HACK method, allow user do something before 
   * drop the node(DropNode())
   * \warning 
   *   Must call DropNode() immediately
   */
  Node* Extract(K const& key) noexcept; 
  void DropNode(Node* node) {
    NodeAllocTraits::destroy(*this, node);
    NodeAllocTraits::deallocate(*this, node, 1);
  } 

  /**
   * \brief call callback to all values in the tree
   * 
   * Compared to the iterate all values by iterator in O(nlgn), 
   * this time complexity is O(n).
   */
  template<typename ValueCb>
  void DoInAll(ValueCb cb);

  V* Find(K const& key) noexcept(noexcept(FindNode(key))) {
    auto ret = FindNode(key);
    return ret ? &ret->value : (V*)nullptr;
  }
  
  iterator LowerBound(K const& key) {
    BaseNode* node = root_;
    BaseNode* track = nullptr;

    while (node) {
      if (TO_COMPARE(get_key(NODE2VALUE(node)), key) >= 0) {
        track = node;
        node = node->left; 
      } else {
        node = node->right;
      }
    }

    return track;
  }
  
  const_iterator LowerBound(K const& key) const { return ((AvlTree*)this)->LowerBound(key); }

  iterator UpperBound(K const& key) {
    BaseNode* node = root_;
    BaseNode* track = nullptr;

    while (node) {
      if (TO_COMPARE(get_key(NODE2VALUE(node)), key) > 0) {
        track = node;
        node = node->left;
      } else {
        node = node->right;
      }
    }

    return track;
  }

  const_iterator UpperBound(K const& key) const { return ((AvlTree*)this)->UpperBound(key); }

  void Clear();
  size_t size() const noexcept { return count_; }
  bool empty() const noexcept { return count_ == 0; }

  iterator begin() noexcept { return (Node*)_GetFirstNode(root_); }
  const_iterator begin() const noexcept { return (Node*)_GetFirstNode(root_); }
  iterator end() noexcept { return nullptr; }
  const_iterator end() const noexcept { return nullptr; }

  const_iterator cbegin() const noexcept { return begin(); }
  const_iterator cend() const noexcept { return end(); }
  
  iterator before_end() noexcept { return (Node*)_GetLastNode(root_); }
  const_iterator before_end() const noexcept { return (Node*)_GetLastNode(root_); }
  const_iterator cbefore_end() const noexcept { return (Node*)_GetLastNode(root_); }


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
        if (TO_COMPARE(get_key(TO_NODE(node->left)->value), get_key(node->value)) >= 0) {
          std::cout << "violate(left < mid): " << get_key(TO_NODE(node->left)->value) << " >= " << get_key(node->value) << "\n";
          ret = false;
        }
      }

      if (node->right) {
        if (TO_COMPARE(get_key(TO_NODE(node->right)->value), get_key(node->value)) <= 0) {
          std::cout << "violate(right > mid): " << get_key(TO_NODE(node->right)->value) << " <= " << get_key(node->value) << "\n";
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

 private:

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

  template<typename T>
  bool _Insert(T&& value);

  template<typename T>
  void _InsertEq(T&& value);

  void EraseRoutine(Node* node) {
    _Erase(node, &root_);
    DropNode(node);
    --count_;
  }

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
  
  BaseNode* root_ = nullptr;
  size_t count_ = 0;
};

} // algo
} // mmkv

#endif // _MMKV_ALGO_INTERNAL_AVL_TREE_H_
