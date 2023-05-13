#ifndef _MMKV_REPLACEMENT_LFU_CACHE_H__
#define _MMKV_REPLACEMENT_LFU_CACHE_H__

#include "cache_interface.h"
#include "mmkv/algo/avl_dictionary.h"
#include "mmkv/algo/comparator_util.h"
#include "mmkv/algo/blist.h"

namespace mmkv {
namespace replacement {

template <typename K>
class LfuCache : public CacheInterface<K> {
 private:
  using ItemList = algo::Blist<K *>;

  struct FreqValue {
    size_t   freq = 1;
    ItemList items;

    explicit FreqValue(size_t freq_)
      : freq(freq_)
    {
    }
  };

  using FreqList = algo::Blist<FreqValue>;
  using FreqNode = typename FreqList::Node;
  using ItemNode = typename ItemList::Node;

  struct NodeCtx {
    FreqNode *fnode;
    ItemNode *inode;

    void SetFreqNode(FreqNode *node)
    {
      fnode = node;
      inode = fnode->value.items.BackNode();
    }
  };

  using Dict = algo::AvlDictionary<K, NodeCtx, algo::Comparator<K>>;

  FreqList freq_list_; // frequency list
  Dict     key_node_ctx_dict_;
  size_t   max_size_;

  template <typename U>
  auto UpdateEntry_(U &&entry) -> K *;

 public:
  explicit LfuCache(size_t max_size)
    : max_size_(max_size)
  {
  }

  ~LfuCache() = default;

  auto UpdateEntry(K const &entry) -> K * override { return UpdateEntry_(entry); }
  auto UpdateEntry(K &&entry) -> K * override { return UpdateEntry_(std::move(entry)); }

  auto DelEntry(K const &key) -> bool override;

  auto Search(K const &key) -> K * override;

  auto Victim() -> K * override;

  auto DelVictim() -> void override;

  auto size() const noexcept -> size_t override { return key_node_ctx_dict_.size(); }
  auto max_size() const noexcept -> size_t override { return max_size_; }

  auto Clear() -> void override
  {
    key_node_ctx_dict_.Clear();
    freq_list_.Clear();
  }

  auto New() const -> LfuCache * override
  {
    auto ret = new LfuCache(-1);
    return ret;
  }
};

} // namespace replacement
} // namespace mmkv

#endif
