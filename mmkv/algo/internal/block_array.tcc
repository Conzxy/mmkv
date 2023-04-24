// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "../block_array.h"

#define MMKV_BLOCK_ARRAY_TEMPLATE_LIST  template <typename T, typename A>
#define MMKV_BLOCK_ARRAY_TEMPLATE_CLASS BlockArray<T, A>
#define MMKV_BLOCK_ARRAY_ARGS           template <typename... Args>
#define MMKV_BLOCK_ARRAY_GROW_SIZE      (block_count_)
namespace mmkv {
namespace algo {

MMKV_BLOCK_ARRAY_TEMPLATE_LIST
MMKV_BLOCK_ARRAY_TEMPLATE_CLASS::~BlockArray() noexcept
{
  auto const first_block_idx = first_block().block_idx;
  auto const first_block_end = BLOCK_SIZE;
  for (size_type i = first_block().entry_idx + 1; i < first_block_end; ++i) {
    destory_entry(blocks_[first_block_idx], i);
  }
  deallocate_entry(blocks_[first_block_idx]);

  auto const last_idx = last_block().block_idx - 1;
  for (size_type i = first_block().block_idx + 1; i < last_idx; ++i) {
    auto block = blocks_[i];
    destory_entries(block);
    deallocate_entry(block);
  }

  auto last_block_idx = last_block().block_idx;
  auto last_block_end = last_block().entry_idx;

  for (size_type i = 0; i < last_block_end; ++i) {
    destory_entry(blocks_[last_block_idx], i);
  }
  deallocate(blocks_[last_block_idx]);

  // At last, free Block
  deallocate_block();
}

MMKV_BLOCK_ARRAY_TEMPLATE_LIST
MMKV_BLOCK_ARRAY_ARGS
int MMKV_BLOCK_ARRAY_TEMPLATE_CLASS::PushBack(Args &&... args)
{
  if (size() + 1 >= max_size()) return 0;

  if (!blocks_) {
    blocks_ = allocate_block(1);
    allocate_entry(blocks_[0]);
    block_push_back(blocks_[0], last_block(), MMKV_FORWARD(Args, args));
    return 1;
  }

  if (last_block().is_full()) {
    Grow(block_count_);
    block_push_back(blocks_[last_block().block_idx], last_block(),
                    MMKV_FORWARD(Args, args));
    return 1;
  }
  return 0;
}

} // namespace algo
} // namespace mmkv
