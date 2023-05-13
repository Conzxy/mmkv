#include "mmkv/replacement/lfu_cache.h"

using namespace mmkv::algo;
using namespace mmkv::replacement;

template <typename K>
template <typename U>
auto LfuCache<K>::UpdateEntry_(U &&entry) -> K *
{
  typename Dict::value_type *p_duplicate = nullptr;

  if (key_node_ctx_dict_
          .InsertKvWithDuplicate(std::forward<U>(entry), NodeCtx{nullptr, nullptr}, p_duplicate))
  {
    // The entry does not exists
    if (freq_list_.empty() || freq_list_.Front().freq > 1) {
      // The freq = 1 node does not exists
      freq_list_.PushFront(1);
    }
    freq_list_.Front().items.PushBack(&p_duplicate->key);
    p_duplicate->value.SetFreqNode(freq_list_.FrontNode());
  } else {
    auto p_node_ctx   = &p_duplicate->value;
    auto p_fnode      = p_node_ctx->fnode;
    auto p_inode      = p_node_ctx->inode;
    auto old_freq     = p_fnode->value.freq;
    auto p_fnode_next = p_fnode->next;

    // If the next does exists and the freq > old_freq + 1,
    // insert a new node after it.
    if (!p_fnode_next || p_fnode_next->value.freq - 1 > old_freq) {
      freq_list_.InsertAfter(p_fnode, freq_list_.CreateNode(old_freq + 1));
      p_fnode_next = p_fnode->next;
    }
    p_fnode->value.items.Extract(p_inode);
    p_fnode_next->value.items.PushBack(p_inode);
    p_node_ctx->SetFreqNode(p_fnode->next);
    if (p_fnode->value.items.empty()) {
      freq_list_.Erase(p_fnode);
    }
  }

  return &p_duplicate->key;
}

template <typename K>
auto LfuCache<K>::DelEntry(const K &key) -> bool
{
  auto p_key_node_ctx_dict_node = key_node_ctx_dict_.Extract(key);

  if (!p_key_node_ctx_dict_node) return false;

  auto fnode = p_key_node_ctx_dict_node->value.value.fnode;
  auto inode = p_key_node_ctx_dict_node->value.value.inode;

  fnode->value.items.Erase(inode);
  if (fnode->value.items.empty()) {
    freq_list_.Erase(fnode);
  }

  key_node_ctx_dict_.DropNode(p_key_node_ctx_dict_node);
  return true;
}

template <typename K>
auto LfuCache<K>::Victim() -> K *
{
  if (key_node_ctx_dict_.empty()) return nullptr;

  auto &victim_freq_value = freq_list_.Front();
  return victim_freq_value.items.Front();
}

template <typename K>
auto LfuCache<K>::DelVictim() -> void
{
  if (key_node_ctx_dict_.empty()) return;

  auto p_victim_freq_value = &freq_list_.Front();
  auto p_key               = p_victim_freq_value->items.Front();

  p_victim_freq_value->items.PopFront();
  if (p_victim_freq_value->items.empty()) {
    freq_list_.PopFront();
  }

  key_node_ctx_dict_.Erase(*p_key);
}

template <typename K>
auto LfuCache<K>::Search(K const &key) -> K *
{
  auto p_key_node_ctx = key_node_ctx_dict_.Find(key);
  return p_key_node_ctx ? &p_key_node_ctx->key : nullptr;
}
