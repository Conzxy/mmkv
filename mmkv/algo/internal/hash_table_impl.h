#include "hash_table.h"


#include <assert.h>
#include <memory>
#include <unistd.h>

#define HASH_TABLE_TEMPLATE \
  template<typename K, typename T, typename HF, typename GK, typename EK, typename A>

#define HASH_TABLE_CLASS \
  HashTable<K, T, HF, GK, EK, A>

#define TABLE1 tables_[0]
#define TABLE2 tables_[1]

namespace mmkv {
namespace algo {

HASH_TABLE_TEMPLATE
HASH_TABLE_CLASS::Table::Table() 
  try
  : table(0)
  , used(0)
  , size_mask(table.size()-1) {

} catch (...) {
  throw;
}

HASH_TABLE_TEMPLATE
HASH_TABLE_CLASS::Table::~Table() noexcept {
}

HASH_TABLE_TEMPLATE
HASH_TABLE_CLASS::HashTable() 
  : hash_()
  , ek_()
  , gk_()
{
  table1().Grow(4);
}

HASH_TABLE_TEMPLATE
HASH_TABLE_CLASS::~HashTable() noexcept {
};

HASH_TABLE_TEMPLATE
template<typename U>
typename HASH_TABLE_CLASS::value_type* 
HASH_TABLE_CLASS::Insert_impl(U&& elem) {
  Rehash();
  IncremetalRehash();

  // Not in rehashing: 
  //   insert to table1
  // In rehashing:
  //   If rehash_move_bucket_index > bucket_index1 ==> table2
  //   otherwise, if bucket_index > table1.size ==> table2
  //              otherwise, tabel1
  const auto bucket_index1 = BucketIndex1(gk_(elem));
  Bucket* bucket = nullptr;

  if (!InRehashing()) {
    bucket = &table1()[bucket_index1];
    if (CheckDuplicate(bucket, elem)) {
      return nullptr;
    }

  } else {
    auto bucket_index2 = BucketIndex2(gk_(elem));

    // index < rehash_move_bucket_index_ in the table2
    if (rehash_move_bucket_index_ > bucket_index1) {
      bucket = &table2()[bucket_index2]; 
      if (CheckDuplicate(bucket, elem)) {
          return nullptr;
      }
    } else {
#if 1
      if (bucket_index2 >= table1().size()) {
        bucket = &table1()[bucket_index1];
        if (CheckDuplicate(bucket, elem)) {
          return nullptr;
        }

        bucket = &table2()[bucket_index2];
        if (CheckDuplicate(bucket, elem)) {
          return nullptr;
        }

      } else {
        bucket = &table1()[bucket_index1];
        if (CheckDuplicate(bucket, elem)) {
          return nullptr;
        }
      }
#else
        bucket = &table1()[bucket_index1];
        if (CheckDuplicate(bucket, elem)) {
          return false;
        }
#endif
    }
  }
  
  bucket->EmplaceFront(elem);

  // FIXME 0?
  ++table1().used;

  return std::addressof(bucket->Front());
}

HASH_TABLE_TEMPLATE
typename HASH_TABLE_CLASS::value_type* 
HASH_TABLE_CLASS::Find(K const& key) {
  // No need to call Rehash()
  IncremetalRehash();
  
  // 采用最robust的方法（simple is best)
  Bucket* bucket = nullptr;
  typename Bucket::Node* slot = nullptr;

  for (int i = 0; i < 2; ++i) {
    bucket = &table(i)[BucketIndex(i, key)];
    slot = bucket->ExtractNodeIf([this, &key](value_type const& value) { return ek_(gk_(value), key); });

    if (slot) { break; }
  }

  if (slot == nullptr) {
    return nullptr;
  }

  bucket->EmplaceFrontNode(slot);

  return std::addressof(slot->value);
}

HASH_TABLE_TEMPLATE
typename HASH_TABLE_CLASS::size_type 
HASH_TABLE_CLASS::Erase(K const& key) {
  IncremetalRehash();
  
  Bucket* bucket = nullptr;
  size_type count = 0;

  for (int i = 0; i < 2; ++i) {
    bucket = &table(i)[BucketIndex(i, key)];
    count = bucket->EraseIf([this, &key](value_type const& val) {
        return ek_(gk_(val), key);
      });

    if (count > 0) { 
      table1().used--;
      break; 
    }
  }
  

  return count; 
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
void HASH_TABLE_CLASS::IncremetalRehash() {
  if (!InRehashing()) {
    return;
  }
  
  auto& bucket1 = table1().table[rehash_move_bucket_index_];

  typename Bucket::Node* slot = nullptr;
  while (!bucket1.empty()) {
    // Calculate the new bucket index in the table2
    slot = bucket1.ExtractFront();
    table2()[BucketIndex2(gk_(slot->value))].EmplaceFrontNode(slot);

    // bucket1.swap(bucket2);
  }

  rehash_move_bucket_index_++;

  if (rehash_move_bucket_index_ == table1().size()) {
    rehash_move_bucket_index_ = (size_type)-1;
    if (!table2().empty()) {
      table1().swap(table2());
    }
  }
}

HASH_TABLE_TEMPLATE
typename HASH_TABLE_CLASS::Bucket*
HASH_TABLE_CLASS::GetExpectedBucket(key_type const& key) {
  Bucket* bucket = nullptr; 

  // If rehash_move_bucket_index=-1,
  // insert into table1
  // otherwise, insert into table1 or table2
  if (!InRehashing()) {
    bucket = &table1()[BucketIndex1(key)];
  } else {
    auto bucket_index = BucketIndex(0, key);

    // index < rehash_move_bucket_index_ in the table2
    if (rehash_move_bucket_index_ > bucket_index) {
      bucket = &table2()[BucketIndex2(key)]; 
    } else {
#if 1
      auto bucket_index2 = BucketIndex2(key);

      if (bucket_index2 >= table1().size()) {
        bucket = &table2()[bucket_index2];
      } else {
        bucket = &table1()[bucket_index];
      }
#else
      bucket = &table1()[bucket_index];
#endif
    }
  }
  
  return bucket;
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
  for (int i = 0; i < 2; ++i) {
    auto size = table(i).size();
    Bucket* bucket = nullptr;
    printf("===== table%d ======\n", i+1);

    for (size_type j = 0; j < size; ++j) {
      printf("[%zu]: ", j);
      bucket = &table(i)[j];
      assert(bucket);

      if (!bucket->empty()) {
        for (auto const& e : *bucket) {
          std::cout << e << " -> ";
        }
      }

      printf("(nil)\n");
    }

    printf("$$$$$ table%d $$$$$\n\n", i+1);
  }
#endif
}

} // namespace algo
} // namespace mmkv
