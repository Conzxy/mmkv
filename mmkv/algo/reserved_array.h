// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_ALGO_RESERVED_ARRAY_H_
#define _MMKV_ALGO_RESERVED_ARRAY_H_

#ifdef DLOG
#  undef DLOG
#endif

#ifdef _DEBUG_RESERVED_ARRAY_
#  include <iostream>
#  include <stdio.h>

#  define DLOG(...) printf(__VA_ARGS__);
#else
#  define DLOG(...)
#endif

#include <memory>
#include <utility>
#include <assert.h>

#include "mmkv/zstl/iterator.h"
#include "mmkv/zstl/type_traits.h"
#include "mmkv/zstl/uninitialized.h"

#include "libc_allocator_with_realloc.h"

namespace mmkv {
namespace algo {

//************************************************************
// 对于以下类型进行reallocate
// 1) trivial type
// 2) non-trivial class type but with nothrow default constructor
//    (since reallocate() cannot ensure exception-safe)
//    and static class variable "can_reallocate = true"
//************************************************************
template <typename T, typename = void>
struct has_nontype_member_can_reallocate_with_true : std::false_type {};

template <typename T>
struct has_nontype_member_can_reallocate_with_true<T, zstl::void_t<decltype(&T::can_reallocate)>>
  : zstl::bool_constant<T::can_reallocate> {};

template <typename T>
struct can_reallocate
  : zstl::disjunction<
        std::is_trivial<T>,
        zstl::conjunction<
            has_nontype_member_can_reallocate_with_true<T>,
            std::is_nothrow_default_constructible<T>>> {};

// template<typename T, typename HasReallocate=std::true_type>
// struct can_reallocate : std::is_trivial<T> {};

// template<typename T>
// struct can_reallocate<T, typename has_nontype_member_can_reallocate<T>::type>
//   : zstl::disjunction<
//       std::is_trivial<T>,
//       zstl::bool_constant<T::can_reallocate>> {};

// template<typename T>
// struct can_reallocate <T, zstl::void_t<zstl::bool_constant<T::can_reallocate>>>
//   : zstl::disjunction<
//       std::is_trivial<T>,
//       zstl::bool_constant<T::can_reallocate>> {};

/**
 * Determine the whether the Grow() or allocate operation to initialize(default or move) the
 * elements
 */
enum InitFlag : unsigned char {
  ON  = 1,
  OFF = 0,
};

/**
 * \brief Like std::vector<T> but there is no capacity concept
 *
 * 命名由来：
 * 首先，std::vector<>的名字是命名失误且无法修正，只能沿用，硬要取个与array区别的名字的话，
 * dynamic_array或scalable_array更为合适，直白并且体现其特征。
 *
 * ReservedArray的命名也体现了其特征：其内容首先进行预分配，然后读取或写入其中的内容。
 * 该容器不支持append，即push/emplace_back等，自然也不支持prepend,即push/emplace_front()等，
 * 故不需要capacity数据成员（或说capacity == size) 支持扩展(Grow)和收缩(Shrink)。
 *
 * 换言之，ReservedArray只是个默认初始化的内存区域。
 *
 * 应用场景(e.g.)：
 * 1) hashtable
 * 2) continuous buffer
 */
template <typename T, typename Alloc = LibcAllocatorWithRealloc<T>>
class ReservedArray : protected Alloc {
  using AllocTraits = std::allocator_traits<Alloc>;

// Allow user initialize throught operator[] and inplacement new
#if 0
  static_assert(std::is_default_constructible<T>::value, 
      "The T(Value) type must be default constructible");
#endif

// To fixed size reserved array,
// this is not required.
#if 0
  static_assert(zstl::disjunction<std::is_move_constructible<T>, std::is_copy_constructible<T>>::value, 
      "The T(Value) type must be move/copy constructible");
#endif

 public:
  using value_type      = T;
  using reference       = T &;
  using const_reference = T const &;
  using pointer         = T *;
  using const_pointer   = T const *;
  using size_type       = size_t;
  using iterator        = pointer;
  using const_iterator  = const_pointer;

  ReservedArray()
    : data_(nullptr)
    , end_(data_)
  {
  }

  explicit ReservedArray(size_type n, InitFlag flag = InitFlag::ON)
    : data_(AllocTraits::allocate(*this, n))
    , end_(data_ + n)
  {
    if (data_ == NULL) {
      throw std::bad_alloc{};
    }

    try {
      if (flag == InitFlag::ON) {
        zstl::UninitializedDefaultConstruct(data_, end_);
      }
    }
    catch (...) {
      data_ = end_ = nullptr;
      throw;
    }
  }

  ReservedArray(ReservedArray const &other)
    : Alloc()
    , data_(AllocTraits::allocate(*this, other.size()))
    , end_(data_ + other.size())
  {
    try {
      std::uninitialized_copy(other.begin(), other.end(), data_);
    }
    catch (...) {
      AllocTraits::deallocate(*this, data_, other.size());
      data_ = end_ = nullptr;
      throw;
    }
  }

  ReservedArray &operator=(ReservedArray const &other)
  {
    if (&other == this) {
      return *this;
    }

    if (size() == other.size()) {
      // 为了保证exception-safe：
      // 对于throw拷贝赋值运算符的类类型分配新内存区域进行copy
      // 而对no throw的直接复用原有内存区域copy
      CopyAssignmentForSizeEqual<value_type>(other);
    } else {
      CopyAssignmentForSizeNotEqual<value_type>(other);
    }

    return *this;
  }

  ReservedArray(ReservedArray &&other) noexcept
    : data_(other.data_)
    , end_(other.end_)
  {
    other.data_ = other.end_ = nullptr;
  }

  ReservedArray &operator=(ReservedArray &&other) noexcept
  {
    this->swap(other);
    return *this;
  }

  ~ReservedArray() noexcept
  {
    if (end_ != data_) {
      zstl::DestroyRange(data_, end_);
      AllocTraits::deallocate(*this, data_, size());
    }
    data_ = end_ = nullptr;
  }

  void Grow(size_type n)
  {
    if (n <= GetSize()) {
      return;
    }

    Reallocate<value_type>(n);
  }

  void GrowNoInit(size_type n) { new (this) ReservedArray(n); }

  /**
   * To user-defined can_reallocate class,
   */
  void Shrink(size_type n)
  {
    if (n >= GetSize()) {
      return;
    }

    Shrink_impl<value_type>(n);
  }

  size_type GetSize() const noexcept { return end_ - data_; }
  size_type size() const noexcept { return end_ - data_; }
  bool      empty() const noexcept { return data_ == end_; }

  reference operator[](size_type i) noexcept
  {
    assert(i < size());

    return data_[i];
  }

  const_reference operator[](size_type i) const noexcept
  {
    assert(i < size());
    return data_[i];
  }

  iterator       begin() noexcept { return data_; }
  iterator       end() noexcept { return end_; }
  const_iterator begin() const noexcept { return data_; }
  const_iterator end() const noexcept { return end_; }
  const_iterator cbegin() const noexcept { return data_; }
  const_iterator cend() const noexcept { return end_; }

  pointer       data() noexcept { return data_; }
  const_pointer data() const noexcept { return data_; }

  void swap(ReservedArray &other) noexcept
  {
    std::swap(data_, other.data_);
    std::swap(end_, other.end_);
  }

 private:
  template <typename U, zstl::enable_if_t<can_reallocate<U>::value, int> = 0>
  void Reallocate(size_type n)
  {
    // Failed to call the reallocate(),
    // the old memory block is not freed
    DLOG("size(before realloc): %zu\n", size());
    DLOG("%p\n", this);
    auto tmp = this->reallocate(data_, size(), n);

    if (tmp == NULL) {
      throw std::bad_alloc{};
    }

    // 为了支持包含can_reallocate = true的non-trivial类类型也能进行reallocate
    // 对扩展的内存区域进行默认初始化

    // reallocate无法保证异常安全
    // 因为data_的内存区域可能已被释放
    DLOG("default consturct [%zu, %zu)\n", size(), n);
    zstl::UninitializedDefaultConstruct(tmp + size(), tmp + n);

    data_ = tmp;
    end_  = data_ + n;
  }

  template <typename U, zstl::enable_if_t<!can_reallocate<U>::value, char> = 0>
  void Reallocate(size_type n)
  {
    // To ensure exception-safe,
    // set data_ and end_ at last
    auto new_data = AllocTraits::allocate(*this, n);
    if (new_data == NULL) {
      throw std::bad_alloc{};
    }

    auto new_end = new_data;

    try {
      new_end = zstl::UninitializedMoveIfNoexcept(data_, end_, new_end);
      new_end = zstl::UninitializedDefaultConstruct(new_end, new_data + n);
    }
    catch (...) {
      AllocTraits::deallocate(*this, new_data, n);
      throw;
    }

    AllocTraits::deallocate(*this, data_, GetSize());
    data_ = new_data;
    end_  = new_end;
  }

  template <typename U, zstl::enable_if_t<can_reallocate<U>::value, char> = 0>
  void Shrink_impl(size_type n)
  {
    // destroy一般来说是no throw的
    DLOG("destroy: [%zu, %zu)\n", n, size());
    zstl::DestroyRange(data_ + n, end_);
    auto tmp = this->reallocate(data_, size(), n);

    if (tmp == NULL && n != 0) {
      throw std::bad_alloc{};
    }

    if (n == 0) {
      data_ = end_ = nullptr;
    } else {
      data_ = tmp;
      end_  = data_ + n;
    }
  }

  template <typename U, zstl::enable_if_t<!can_reallocate<U>::value, int> = 0>
  void Shrink_impl(size_type n)
  {
    pointer new_data = nullptr;
    pointer new_end  = nullptr;
    if (n != 0) {
      new_data = AllocTraits::allocate(*this, n);
      if (new_data == NULL) {
        throw std::bad_alloc{};
      }

      new_end = new_data;

      try {
        new_end = zstl::UninitializedMoveIfNoexcept(data_, data_ + n, new_end);
      }
      catch (...) {
        AllocTraits::deallocate(*this, new_data, n);
        throw;
      }
    }

    for (auto beg = data_ + n; beg != end_; ++beg) {
      AllocTraits::destroy(*this, beg);
    }

    AllocTraits::deallocate(*this, data_, GetSize());

    data_ = new_data;
    end_  = new_end;
  }

  template <typename U, typename = zstl::enable_if_t<std::is_nothrow_copy_assignable<U>::value>>
  void CopyAssignmentForSizeEqual(ReservedArray const &other)
  {
    std::copy(other.begin(), other.end(), begin());
  }

  template <typename U, zstl::enable_if_t<!std::is_nothrow_copy_assignable<U>::value, int> = 0>
  void CopyAssignmentForSizeEqual(ReservedArray const &other)
  {
    // swap是no throw的，因此如果拷贝构造抛出了异常
    // 不会导致this被改动（或破坏）
    ReservedArray(other).swap(*this);
  }

  template <typename U>
  using cond = zstl::conjunction<
      can_reallocate<U>,
      std::is_nothrow_copy_assignable<U>,
      std::is_nothrow_constructible<U>>;

  template <typename U, typename = zstl::enable_if_t<cond<U>::value>>
  void CopyAssignmentForSizeNotEqual(ReservedArray const &other)
  {
    auto tmp = this->reallocate(data_, size(), other.size());

    if (tmp == NULL) {
      throw std::bad_alloc{};
    }

    if (other.size() > size()) {
      std::copy(other.begin(), other.begin() + size(), tmp);
      std::uninitialized_copy(other.begin() + size(), other.end(), tmp + size());
    } else {
      std::copy(other.begin(), other.end(), tmp);
      zstl::DestroyRange(tmp + other.size(), tmp + size());
    }

    data_ = tmp;
    end_  = data_ + other.size();
  }

  template <typename U, zstl::enable_if_t<!cond<U>::value, int> = 0>
  void CopyAssignmentForSizeNotEqual(ReservedArray const &other)
  {
    auto data = AllocTraits::allocate(*this, other.size());

    if (data == NULL) {
      throw std::bad_alloc{};
    }

    try {
      std::uninitialized_copy(other.begin(), other.end(), data);
    }
    catch (...) {
      AllocTraits::deallocate(*this, data, other.size());
      throw;
    }

    zstl::DestroyRange(data_, end_);
    AllocTraits::deallocate(*this, data_, size());

    data_ = data;
    end_  = data + other.size();
  }

  T *data_;
  T *end_;
};

} // namespace algo
} // namespace mmkv

namespace std {

template <typename T>
inline void swap(mmkv::algo::ReservedArray<T> &x, mmkv::algo::ReservedArray<T> &y) noexcept(
    noexcept(x.swap(y))
)
{
  x.swap(y);
}

} // namespace std

#endif // _MMKV_ALGO_RESERVED_ARRAY_H_
