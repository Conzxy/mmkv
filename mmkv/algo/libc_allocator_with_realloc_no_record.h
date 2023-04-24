// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_ALGO_LIBC_ALLOCATOR_WITH_REALLOC_NO_RECORD_H_
#define _MMKV_ALGO_LIBC_ALLOCATOR_WITH_REALLOC_NO_RECORD_H_

#include "mmkv/zstl/type_traits.h"

namespace mmkv {
namespace algo {

/**
 * Like LibcAllocatorWithRealloc
 * but don't record the usage of memory
 */
template <typename T>
class LibcAllocatorWithReallocNoRecord {
 public:
  using value_type = T;
  using pointer = T *;
  using const_pointer = T const *;
  using reference = T &;
  using const_reference = T const &;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  // 尽管C++11支持模板别名
  // 兼容11之前的标准
  template <typename U>
  struct rebind {
    using other = LibcAllocatorWithReallocNoRecord<U>;
  };

  LibcAllocatorWithReallocNoRecord() = default;

  template <typename U>
  LibcAllocatorWithReallocNoRecord(
      LibcAllocatorWithReallocNoRecord<U> const &) noexcept
  {
  }

  pointer address(reference x) const noexcept { return &x; }

  const_pointer address(const_reference x) noexcept { return &x; }

  T *allocate(size_type n) noexcept
  {
    return reinterpret_cast<T *>(::malloc(n * sizeof(T)));
  }

  T *reallocate(pointer p, size_t old_n, size_type n) noexcept
  {
    return reinterpret_cast<T *>(::realloc(p, n * sizeof(T)));
  }

  void deallocate(pointer p, size_type n) noexcept
  {
    (void)n;
    // If p is NULL, free() do nothing
    ::free(p);
  }

  template <typename... Args>
  void construct(pointer p, Args &&...args)
  {
    new (p) value_type(std::forward<Args>(args)...);
  }

  void destroy(pointer p) { destroy_impl(p); }

 private:
  template <typename U, zstl::enable_if_t<std::is_trivial<U>::value, int> = 0>
  void destroy_impl(U *p)
  {
    // do nothing
    (void)p;
  }

  template <typename U, zstl::enable_if_t<!std::is_trivial<U>::value, char> = 0>
  void destroy_impl(U *p)
  {
    p->~U();
  }
  // stateless
};

// void特化：
// 无成员函数及reference，const_reference
template <>
class LibcAllocatorWithReallocNoRecord<void> {
 public:
  using value_type = void;
  using pointer = void *;
  using const_pointer = void const *;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  template <typename U>
  struct rebind {
    using other = LibcAllocatorWithReallocNoRecord<U>;
  };
};

} // namespace algo
} // namespace mmkv

#endif
