// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_ALGO_INTERNAL_HASH_TABLE_IMPL_H_
#define _MMKV_ALGO_INTERNAL_HASH_TABLE_IMPL_H_

#ifndef _MMKV_ALGO_HASH_TABLE_H_
#include "../hash_table.h"
#endif

#include <assert.h>
#include <memory>

#define HASH_TABLE_TEMPLATE \
  template<typename K, typename T, typename HF, typename GK, typename EK, typename A>

#define HASH_TABLE_CLASS \
  HashTable<K, T, HF, GK, EK, A>

#define HASH_MAX(x, y) (((x) > (y)) ? (x) : (y))
#define TABLE1 tables_[0]
#define TABLE2 tables_[1]

namespace mmkv {
namespace algo {

HASH_TABLE_TEMPLATE
HASH_TABLE_CLASS::HashTable() 
{
  table1().Grow(4);
#ifdef _DEBUG_HASH_TABLE_
  printf("this = %p\n", this);
  printf("table = %p\n", &tables_[0].table);
  printf("rehash_index = %p\n", &rehash_move_bucket_index_);
  // printf("hash = %p\n", &hash_);
  // printf("equal key = %p\n", &ek_);
  // printf("get key = %p\n", &gk_);
#endif
}

HASH_TABLE_TEMPLATE
HASH_TABLE_CLASS::~HashTable() noexcept {
};

HASH_TABLE_TEMPLATE
void HASH_TABLE_CLASS::Clone(HashTable const& ht) {
  auto& table = table1();
  if (ht.InRehashing()) {
    auto& otable1 = ht.table1();
    auto& otable2 = ht.table2();
    table.Grow(otable2.size());

    for (size_type i = 0; i < otable2.size(); ++i) {
      auto& lst = table[i];
      auto& olst = otable2[i];
      for (auto const& val : olst) {
        lst.EmplaceFront(val);
      }
    }
  
    for (size_type i = ht.rehash_move_bucket_index_; i < otable1.size(); ++i) {
      auto& olst = otable1[i];
      for (auto const& val : olst) {
        auto hash_val = HASH_FUNC(val);
        table[bucket_index(0, hash_val)].EmplaceFront(val);
      }
    } 
  } else {
    auto& otable = ht.table1();
    auto n = otable.size();
    table.Grow(otable.size());

    for (size_type i = 0; i < n; ++i) {
      auto& lst = table[i];
      auto& olst = otable[i];
      for (auto const& val : olst) {
        lst.EmplaceFront(val);
      }
    }
  }

  table.used = ht.table1().used;
}

HASH_TABLE_TEMPLATE
template<typename U>
typename HASH_TABLE_CLASS::value_type* 
HASH_TABLE_CLASS::Insert_impl(U&& elem) {
  value_type* duplicate = nullptr;
  if (InsertWithDuplicate_impl(std::forward<U>(elem), duplicate)) {
    return duplicate;
  }
  return nullptr;
}

HASH_TABLE_TEMPLATE
template<typename U>
bool HASH_TABLE_CLASS::InsertWithDuplicate_impl(U&& elem, value_type*& duplicate) {
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

#define CHECK_AND_SET_DUPLICATE do { \
    auto iter = GetDuplicate(bucket, elem); \
    if (iter != bucket->end()) { \
      duplicate = &*iter; \
      return false; \
    } \
    } while (0)

  if (!InRehashing()) {
    bucket = &table1()[bucket_index1];
    CHECK_AND_SET_DUPLICATE;
  } else {
    auto bucket_index2 = bucket_index(1, hash_val);

    // index < rehash_move_bucket_index_ in the table2
    if (rehash_move_bucket_index_ > bucket_index1) {
      bucket = &table2()[bucket_index2]; 
      CHECK_AND_SET_DUPLICATE;
    } else {
#if 0
      if (bucket_index2 >= table1().size()) {
        bucket = &table1()[bucket_index1];
        CHECK_AND_SET_DUPLICATE;

        bucket = &table2()[bucket_index2];
        CHECK_AND_SET_DUPLICATE;
      } else {
        bucket = &table1()[bucket_index1];
        CHECK_AND_SET_DUPLICATE;
      }
#else
        bucket = &table1()[bucket_index1];
        CHECK_AND_SET_DUPLICATE;

        bucket = &table2()[bucket_index2];
        CHECK_AND_SET_DUPLICATE;

#endif
    }
  }
  
  bucket->EmplaceFront(std::forward<U>(elem));
  duplicate = std::addressof(bucket->Front());

  // FIXME 0?
  ++table1().used;

  return true;
}

HASH_TABLE_TEMPLATE
typename HASH_TABLE_CLASS::value_type* 
HASH_TABLE_CLASS::Find(K const& key) {
  auto slot = FindSlot(key);
  return slot ? std::addressof((*slot)->value) : nullptr;
}

HASH_TABLE_TEMPLATE
typename HASH_TABLE_CLASS::Slot**
HASH_TABLE_CLASS::FindSlot(K const& key) {
  // No need to call Rehash()
  IncrementalRehash();
  
  // 采用最robust的方法（simple is best)
  Bucket* bucket = nullptr;
  typename Bucket::Node* slot = nullptr;

  const int table_num = (InRehashing()) ? 2 : 1; 

  // 当table为空时，BucetIndex是非法的
  const auto hash_val = HASH_FUNC(key);
  for (int i = 0; i < table_num; ++i) {
    bucket = &table(i)[bucket_index(i, hash_val)];
    slot = bucket->ExtractNodeIf([this, &key](value_type const& value) { return EQUAL_KEY(GET_KEY(value), key); });

    if (slot) { break; }
  }

  if (slot == nullptr) {
    return nullptr;
  }

  bucket->EmplaceFrontNode(slot);
  assert(bucket->header() == slot);
  return &bucket->header();
}

HASH_TABLE_TEMPLATE
void HASH_TABLE_CLASS::EraseAfterFindSlot(Slot*& slot) {
  auto old = slot;
  slot = slot->next;
  DropNode(old);
}

HASH_TABLE_TEMPLATE
inline typename HASH_TABLE_CLASS::size_type 
HASH_TABLE_CLASS::Erase(K const& key) {
  auto node = Extract(key);
  if (!node) {
    return 0;
  }

  DropNode(node);
  return 1;
}

HASH_TABLE_TEMPLATE
typename HASH_TABLE_CLASS::Node* HASH_TABLE_CLASS::Extract(K const& key) noexcept {
  // WARNING Don't support shrink temporarily
  IncrementalRehash();
  
  Bucket* bucket = nullptr;
  Node* node = nullptr;

  const auto hash_val = HASH_FUNC(key);
  const int table_num = (InRehashing()) ? 2 : 1;
  for (int i = 0; i < table_num; ++i) {
    bucket = &table(i)[bucket_index(i, hash_val)];
    node = bucket->ExtractNodeIf([this, &key](value_type const& val) {
        return EQUAL_KEY(GET_KEY(val), key);
      });

    if (node) { 
      table1().used--;
      break; 
    }
  }
  
  return node; 
}

HASH_TABLE_TEMPLATE
void HASH_TABLE_CLASS::Rehash() {
  if (!InRehashing() && 
      table1().used >= table1().size()) {
    table2().Grow(table1().size() << 1);
    rehash_move_bucket_index_ = 0;
  }
}

HASH_TABLE_TEMPLATE
void HASH_TABLE_CLASS::Clear() {
  // size_type n = table1().size();

  table1().Shrink(4);
  for (size_type i = 0; i < 4; ++i)
    table1()[i].Clear();
  table1().used = 0;
  
  if (InRehashing()) {
    table2().Shrink(0);
    // for (size_type i = 0; i < n; ++i)
    //   table2()[i].Clear();
  }
  rehash_move_bucket_index_ = ~0;
}

HASH_TABLE_TEMPLATE
void HASH_TABLE_CLASS::IncrementalRehash() {
  if (!InRehashing()) {
    return;
  }
  
  auto& bucket1 = table1().table[rehash_move_bucket_index_];

  typename Bucket::Node* slot = nullptr;
  while (!bucket1.empty()) {
    // Calculate the new bucket index in the table2
    slot = bucket1.ExtractFront();
    table2()[BucketIndex2(GET_KEY(slot->value))].EmplaceFrontNode(slot);

    // bucket1.swap(bucket2);
  }

  rehash_move_bucket_index_++;

  if (rehash_move_bucket_index_ == table1().size()) {
    rehash_move_bucket_index_ = (size_type)-1;
    if (!table2().empty()) {
      table1().swap(table2());
      table2().Reset();
    }
  }
}

HASH_TABLE_TEMPLATE
void HASH_TABLE_CLASS::DebugPrint() {
#ifdef _DEBUG_HASH_TABLE_
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

} // namespace algo
} // namespace mmkv

#endif
