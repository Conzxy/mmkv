#include "mmkv/algo/hash_table.h"

#define HASH_TABLE_TEMPLATE \
  template<typename K, typename T, typename H, typename GK, typename EK, typename A>

#define HASH_TABLE_CLASS \
  HashTable<K, T, H, GK, EK, A>

namespace mmkv {
namespace algo {

HASH_TABLE_TEMPLATE
HASH_TABLE_CLASS::Table::Table() 
  : table(4)
  , used(0)
  , size_mask(table.size() - 1) {
  //FIXME exception handling
}

HASH_TABLE_TEMPLATE
HASH_TABLE_CLASS::Table::~Table() noexcept {
}

HASH_TABLE_TEMPLATE
bool HASH_TABLE_CLASS::Insert(T const& elem) {
  Rehash();

  auto const hash_value = hash_(elem) & table_[0].size_mask;
}

HASH_TABLE_TEMPLATE
void HASH_TABLE_CLASS::Rehash() {

}

} // namespace algo
} // namespace mmkv
