// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef MMKV_ALGO_BLOCK_ARRAY_H__
#define MMKV_ALGO_BLOCK_ARRAY_H__

#include <cstddef> // size_t
#include <utility> //
#include <memory>

#include "libc_allocator_with_realloc.h"
#include "mmkv/util/macro.h"

namespace mmkv {
namespace algo {

namespace block_array_detail {

template <typename T, typename Alloc = LibcAllocatorWithRealloc<T>>
class BlockArrayBase {
 public:
  static constexpr int MAX_BLOCK_SIZE = 4096;
  static constexpr int BLOCK_SIZE = (sizeof(T) > MAX_BLOCK_SIZE)
                                        ? MAX_BLOCK_SIZE
                                        : MAX_BLOCK_SIZE / sizeof(T);
  using size_type = size_t;
  using value_type = T;
  using reference = T &;
  using const_reference = T const &;
  using pointer = T *;
  using const_pointer = T const *;

  using AllocTraits = std::allocator_traits<Alloc>;

  struct Block {
    Block() = default;
    T *data = nullptr;

    MMKV_INLINE bool is_empty() noexcept
    {
      return data == nullptr;
    }
  };

  using BlockAlloc = typename Alloc::template rebind<Block>::other;
  using BlockAllocTraits = std::allocator_traits<BlockAlloc>;

  struct HBlock {
    HBlock() = default;

    MMKV_INLINE bool is_full() noexcept
    {
      return size_type(-1) == entry_idx;
    }

    MMKV_INLINE bool is_empty() noexcept
    {
      return entry_idx == BLOCK_SIZE - 1;
    }
    MMKV_INLINE bool size() noexcept
    {
      return BLOCK_SIZE - 1 - entry_idx;
    }

    size_type block_idx = 0;
    size_type entry_idx = BLOCK_SIZE - 1;
  };

  struct EBlock {
    MMKV_INLINE bool is_full() noexcept
    {
      return block_idx == BLOCK_SIZE;
    }

    MMKV_INLINE bool is_empty() noexcept
    {
      return block_idx == 0;
    }

    MMKV_INLINE size_type size() noexcept
    {
      return entry_idx;
    }

    size_type block_idx = 0;
    size_type entry_idx = 0;
  };
};

} // namespace block_array_detail

template <typename T, typename Alloc = LibcAllocatorWithRealloc<T>>
class BlockArray
  : protected block_array_detail::BlockArrayBase<T, Alloc>
  , protected Alloc
  , protected block_array_detail::BlockArrayBase<T, Alloc>::BlockAlloc {

  using Base = block_array_detail::BlockArrayBase<T, Alloc>;
  using typename Base::AllocTraits;
  using typename Base::Block;
  using typename Base::BlockAllocTraits;
  using typename Base::HBlock;

 public:
  using Base::BLOCK_SIZE;
  using typename Base::size_type;
  // using iterator = blist::BlistIterator<T>;
  // using const_iterator = blist::BlistConstIterator<T>;
  // using Node = blist::BNode<T>;

  MMKV_INLINE BlockArray() {}
  MMKV_INLINE ~BlockArray() noexcept;

  MMKV_INLINE BlockArray(BlockArray &&other) noexcept {}

  template <typename... Args>
  int PushBack(Args &&... args);

  MMKV_INLINE size_type size() noexcept
  {
    return (block_count_ >= 2 ? ((block_count_ - 2) * BLOCK_SIZE) : 0) +
           first_block().size() + last_block().size();
  }

  MMKV_INLINE size_type max_size() noexcept
  {
    return size_type(-1);
  }

 private:
  void Grow(size_type n) {}

  MMKV_INLINE void allocate_entry(Block &block)
  {
    block.data = AllocTraits::allocate(*this, BLOCK_SIZE);
  }

  MMKV_INLINE void deallocate_entry(Block &block) noexcept
  {
    AllocTraits::deallocate(*this, block.data, BLOCK_SIZE);
  }

  MMKV_INLINE void destroy_entry(Block &block, size_type idx) noexcept
  {
    AllocTraits::destroy(*this, block.data + idx);
  }

  MMKV_INLINE void destory_entries(Block &block) noexcept
  {
    for (size_type i = 0; i < BLOCK_SIZE; ++i)
      AllocTraits::destory(*this, block.data + i);
  }

  MMKV_INLINE void block_pop_back(Block &block, HBlock &hblock) noexcept
  {
    AllocTraits::destroy(*this, block.data + hblock.entry_idx);
    hblock.entry_idx--;
  }

  template <typename... Args>
  MMKV_INLINE void block_push_back(Block &block, HBlock &hblock,
                                   Args &&... args)
  {
    AllocTraits::construct(*this, block.data + hblock.entry_idx,
                           MMKV_FORWARD(Args, args));
    hblock.entry_idx++;
  }

  MMKV_INLINE Block *allocate_block(size_type n) noexcept
  {
    return BlockAllocTraits::allocate(*this, n);
  }

  MMKV_INLINE void deallocate_block() noexcept
  {
    return BlockAllocTraits::deallocate(*this, blocks_, block_count_);
  }

  MMKV_INLINE HBlock &first_block() noexcept
  {
    return first_block_;
  }
  MMKV_INLINE HBlock &last_block() noexcept
  {
    return last_block_;
  }

  Block *blocks_ = nullptr;
  size_type block_count_ = 0;
  HBlock first_block_;
  HBlock last_block_;
};

} // namespace algo
} // namespace mmkv

#include "internal/block_array.tcc"

#endif
