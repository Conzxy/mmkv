#ifndef _MMKV_ALGO_INTERNAL_TREE_HASH_TABLE_IMPL_H_
#define _MMKV_ALGO_INTERNAL_TREE_HASH_TABLE_IMPL_H_

#include "tree_hashtable.h"

#define TREE_HASH_TABLE_TEMPLATE template<typename K, typename V, typename GK, typename HF, typename Tree, typename A>
#define TREE_HASH_TABLE_CLASS TreeHashTable<K, V, GK, HF, Tree, A>

#define HASH_MAX(x, y) (((x) > (y)) ? (x) : (y))

namespace mmkv {
namespace algo {

TREE_HASH_TABLE_TEMPLATE
TREE_HASH_TABLE_CLASS::TreeHashTable()
{
  table1().Grow(4);
}

TREE_HASH_TABLE_TEMPLATE
inline bool TREE_HASH_TABLE_CLASS::Push(Node* node) {
  return PushWithDuplicate(node, nullptr);
}

TREE_HASH_TABLE_TEMPLATE
inline bool TREE_HASH_TABLE_CLASS::PushWithDuplicate(Node* node, value_type** duplicate) {
  Rehash();
  IncrementalRehash();

  // Not in rehashing: 
  //   insert to table1
  // In rehashing:
  //   If rehash_move_bucket_index > bucket_index1 ==> table2
  //   otherwise, if bucket_index > table1.size ==> table2
  //              otherwise, tabel1
  const auto hash_val = HASH_FUNC(GET_KEY(node->value));
  const auto bucket_index1 = bucket_index(0, hash_val);
  Bucket* bucket = nullptr;

#define PUSH_AND_SET_DUPLICATE do { \
    auto success = bucket->PushWithDuplicate(node, duplicate); \
    if (!success) { \
      return false; \
    } \
    } while (0)

#define AVL_CHECK_AND_SET_DUPLICATE_OF_PUSH do {\
    auto value = bucket->Find(GET_KEY(node->value)); \
    if (value) { \
      if (duplicate) *duplicate = value; \
      return false; \
    } } while (0)

  if (!InRehashing()) {
    bucket = &table1()[bucket_index1];
    PUSH_AND_SET_DUPLICATE;
  } else {
    // index < rehash_move_bucket_index_ in the table2
    if (rehash_move_bucket_index_ > bucket_index1) {
      auto bucket_index2 = bucket_index(1, hash_val);
      bucket = &table2()[bucket_index2]; 
      PUSH_AND_SET_DUPLICATE;
    } else {
      auto bucket_index2 = bucket_index(1, hash_val);
#if 0
      if (bucket_index2 >= table1().size()) {
        bucket = &table1()[bucket_index1];
        CHECK_AND_SET_DUPLICATE;

        bucket = &table2()[bucket_index2];
        INSERT_AND_SET_DUPLICATE;
      } else {
        bucket = &table1()[bucket_index1];
        INSERT_AND_SET_DUPLICATE;
      }
#else
      bucket = &table1()[bucket_index1];
      PUSH_AND_SET_DUPLICATE;

      bucket = &table2()[bucket_index2];
      AVL_CHECK_AND_SET_DUPLICATE_OF_PUSH;
#endif
    }
  }

  // FIXME 0?
  ++table1().used;

  return true; 
}

TREE_HASH_TABLE_TEMPLATE
template<typename U>
inline typename TREE_HASH_TABLE_CLASS::value_type* 
TREE_HASH_TABLE_CLASS::Insert_impl(U&& elem) {
  value_type* duplicate = nullptr;
  if (InsertWithDuplicate_impl(std::forward<U>(elem), duplicate)) {
    return duplicate;
  }
  return nullptr;
}

TREE_HASH_TABLE_TEMPLATE
template<typename U>
inline bool TREE_HASH_TABLE_CLASS::InsertWithDuplicate_impl(U&& elem, value_type *& duplicate) {
  Rehash();
  IncrementalRehash();

  // Not in rehashing: 
  //   insert to table1
  // In rehashing:
  //   If rehash_move_bucket_index > bucket_index1 ==> table2
  //   otherwise, if bucket_index > table1.size ==> table2
  //              otherwise, tabel1
  const auto hash_val = HASH_FUNC(GET_KEY(elem));
  const auto bucket_index1 = bucket_index(0, hash_val);
  Bucket* bucket = nullptr;

#define INSERT_AND_SET_DUPLICATE do { \
    auto success = bucket->InsertWithDuplicate(std::forward<U>(elem), &duplicate); \
    if (!success) { \
      return false; \
    } \
    } while (0)

#define AVL_CHECK_AND_SET_DUPLICATE do {\
    auto value = bucket->Find(GET_KEY(elem)); \
    if (value) { \
      duplicate = value; \
      return false; \
    } } while (0)

  if (!InRehashing()) {
    bucket = &table1()[bucket_index1];
    INSERT_AND_SET_DUPLICATE;
  } else {
    // index < rehash_move_bucket_index_ in the table2
    if (rehash_move_bucket_index_ > bucket_index1) {
      auto bucket_index2 = bucket_index(1, hash_val);
      bucket = &table2()[bucket_index2]; 
      INSERT_AND_SET_DUPLICATE;
    } else {
      auto bucket_index2 = bucket_index(1, hash_val);
#if 0
      if (bucket_index2 >= table1().size()) {
        bucket = &table1()[bucket_index1];
        CHECK_AND_SET_DUPLICATE;

        bucket = &table2()[bucket_index2];
        INSERT_AND_SET_DUPLICATE;
      } else {
        bucket = &table1()[bucket_index1];
        INSERT_AND_SET_DUPLICATE;
      }
#else
      bucket = &table2()[bucket_index2];
      AVL_CHECK_AND_SET_DUPLICATE;

      bucket = &table1()[bucket_index1];
      INSERT_AND_SET_DUPLICATE;
#endif
    }
  }

  // FIXME 0?
  ++table1().used;

  return true;
}

TREE_HASH_TABLE_TEMPLATE
inline void TREE_HASH_TABLE_CLASS::Rehash() {
  if (!InRehashing() && 
      table1().used >= table1().size()) {
    table2().Grow(table1().size() << 1);
    rehash_move_bucket_index_ = 0;
  }
}

TREE_HASH_TABLE_TEMPLATE
inline void TREE_HASH_TABLE_CLASS::IncrementalRehash() {
  if (!InRehashing()) {
    return;
  }

  auto& bucket1 = table1().table[rehash_move_bucket_index_];

  // typename Bucket::Node* slot = nullptr;
  // uint64_t hash_val = 0;
  // while (!bucket1.empty()) {
  //   slot = bucket1.Extract();
  //   hash_val = HASH_FUNC(GET_KEY(slot->value));
  //   table2()[bucket_index(1, hash_val)].Push(slot);
  // }

  bucket1.ReuseAllNodes([this](Node *node) {
    table2()[bucket_index(1, HASH_FUNC(GET_KEY(node->value)))].Push(node);
  });

  rehash_move_bucket_index_++;

  if (rehash_move_bucket_index_ == table1().size()) {
    rehash_move_bucket_index_ = (size_type)-1;
    if (!table2().empty()) {
      table1().swap(table2());
      table2().Reset();
    }
  }
}

TREE_HASH_TABLE_TEMPLATE
inline typename TREE_HASH_TABLE_CLASS::value_type*
TREE_HASH_TABLE_CLASS::Find(K const& key) {
  auto node = FindNode(key, nullptr);
  if (node) return std::addressof(node->value);
  return nullptr;
}

TREE_HASH_TABLE_TEMPLATE
inline typename TREE_HASH_TABLE_CLASS::Node*
TREE_HASH_TABLE_CLASS::FindNode(K const& key, Bucket** bck) {
  // No need to call Rehash()
  IncrementalRehash();
  
  // 采用最robust的方法（simple is best)
  Bucket* bucket = nullptr;
  Node* node = nullptr;

  const int table_num = (InRehashing()) ? 2 : 1; 

  // 当table为空时，BucetIndex是非法的
  const auto hash_val = HASH_FUNC(key);
  for (int i = 0; i < table_num; ++i) {
    bucket = &table(i)[bucket_index(i, hash_val)];
    node = bucket->FindNode(key);

    if (node) { 
      if (bck) *bck = bucket;
      return node;
    }
  }

  return nullptr;
}

TREE_HASH_TABLE_TEMPLATE
inline typename TREE_HASH_TABLE_CLASS::Node*
TREE_HASH_TABLE_CLASS::Extract(K const& key) {
  // WARNING Don't support shrink temporarily
  IncrementalRehash();
  
  Bucket* bucket = nullptr;
  Node* node = nullptr;

  const auto hash_val = HASH_FUNC(key);
  const int table_num = (InRehashing()) ? 2 : 1;
  for (int i = 0; i < table_num; ++i) {
    bucket = &table(i)[bucket_index(i, hash_val)];
    node = bucket->Extract(key);

    if (node) { 
      table1().used--;
      break; 
    }
  }
  
  return node; 
}

TREE_HASH_TABLE_TEMPLATE
inline typename TREE_HASH_TABLE_CLASS::size_type 
TREE_HASH_TABLE_CLASS::Erase(K const& key) {
  auto node = Extract(key);
  if (node) {
    DropNode(node);
    return 1;
  }
  return 0;
}

TREE_HASH_TABLE_TEMPLATE
inline typename TREE_HASH_TABLE_CLASS::size_type
TREE_HASH_TABLE_CLASS::EraseNode(Bucket* bucket, Node* node) {
  return bucket->EraseNode(node);
}

TREE_HASH_TABLE_TEMPLATE
void TREE_HASH_TABLE_CLASS::DebugPrint() {
#ifdef _DEBUG_TREE_HASH_TABLE_
  printf("====== Hash table metadata =====\n");
  printf("used count = %zu\n", table1().used);
  printf("size(table1) = %zu\nsize(table2) = %zu\n", table1().size(), table2().size());
  printf("sizemask(table1) = %zu\nsizemask(table2) = %zu\n", table1().size_mask, table2().size_mask);
  printf("rehash_move_bucket_index = %zu\n", rehash_move_bucket_index_);
  printf("====== View of hash table =====\n");

  int longthest_list_size = 0;

  for (int i = 0; i < 2; ++i) {
    auto size = table(i).size();
    Bucket* bucket = nullptr;
    printf("===== table%d ======\n", i+1);

    for (size_type j = 0; j < size; ++j) {
      printf("[%zu]: ", j);
      bucket = &table(i)[j];
      assert(bucket);
      
      int list_size = 0;
      if (!bucket->empty()) {
        for (auto const& e : *bucket) {
          std::cout << e << " -> ";
          list_size++;
        }
      }

      longthest_list_size = HASH_MAX(list_size, longthest_list_size);

      printf("(nil)\n");
    }

    printf("$$$$$ table%d $$$$$\n\n", i+1);
  }

  printf("The longest list size = %d\n", longthest_list_size);
#endif
}

} // algo
} // mmkv

#endif // _MMKV_ALGO_INTERNAL_TREE_HASH_TABLE_IMPL_H_
