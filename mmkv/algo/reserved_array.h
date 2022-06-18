#ifndef _MMKV_ALGO_RESERVED_ARRAY_H_
#define _MMKV_ALGO_RESERVED_ARRAY_H_

#include <initializer_list>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

#include "mmkv/zstl/type_traits.h"
#include "mmkv/zstl/uninitialized.h"

#include "libc_allocator_with_realloc.h"

namespace mmkv {
namespace algo {

template<typename T, typename=void>
struct has_nontype_member_can_reallocate_with_true : std::false_type {};

template<typename T>
struct has_nontype_member_can_reallocate_with_true<T, zstl::void_t<decltype(&T::can_reallocate)>> : zstl::bool_constant<T::can_reallocate> {};

template<typename T>
struct can_reallocate : zstl::disjunction<std::is_trivial<T>, has_nontype_member_can_reallocate_with_true<T>> {};

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
 * \brief Like std::vector<T> but there is no capacity concept
 *
 * 命名由来：
 * 首先，std::vector<>的名字是命名失误且无法修正，只能沿用，硬要取个与array区别的名字的话，
 * dynamic_array或scalable_array更为合适，直白并且体现其特征。
 *
 * ReservedArray的命名也体现了其特征：其内容首先进行预分配，然后读取或写入其中的内容。
 * 该容器不支持append，即push/emplace_back等，自然也不支持prepend,即push/emplace_front()等， 故不需要capacity数据成员（或说capacity == size)
 * 支持扩展(Grow)和收缩(Shrink)。
 *
 * 换言之，ReservedArray只是个默认初始化的内存区域。
 *
 * 应用场景(e.g.)：
 * 1) hashtable
 * 2) continuous buffer
 */
template<typename T, typename Alloc=LibcAllocatorWithRealloc<T>>
class ReservedArray : protected Alloc {
  using AllocTraits = std::allocator_traits<Alloc>;
  
  static_assert(std::is_default_constructible<T>::value, 
      "The T(Value) type must be default constructible");
  static_assert(zstl::disjunction<std::is_move_constructible<T>, std::is_copy_constructible<T>>::value, 
      "The T(Value) type must be move/copy constructible");

 public:
  using value_type = T;
  using reference = T&;
  using const_reference = T const&;
  using pointer = T*;
  using const_pointer = T const*;
  using size_type = size_t;

  ReservedArray()
    : data_(nullptr)
    , end_(data_)
  {
  }
  
  // FIXME Exception handling 
  explicit ReservedArray(size_type n) 
    : data_(AllocTraits::allocate(*this, n))
    , end_(data_+n)
  {
    zstl::UninitializedDefaultConstruct(data_, end_);
  } 

  ~ReservedArray() noexcept {
    size_type const size = end_ - data_;
    for (size_type i = 0; i < size; ++i) {
      AllocTraits::destroy(*this, data_+i);
    }

    AllocTraits::deallocate(*this, data_, size);
  }
  

  void Grow(size_type n) {
    if (n <= GetSize()) { return; }

    Reallocate<value_type>(n);
  } 
  

  void Shrink(size_type n) {
    if (n >= GetSize()) { return; }

    Shrink_impl<value_type>(n);
  }

  size_type GetSize() const noexcept { return end_ - data_; }
  size_type size() const noexcept { return end_ - data_; }
  
  reference operator[](size_type i) noexcept {
    return data_[i];
  }

  const_reference operator[](size_type i) const noexcept {
    return data_[i];
  }
  
  pointer begin() noexcept { return data_; }  
  const_pointer begin() const noexcept { return data_; }
  pointer end() noexcept { return end_; }
  const_pointer end() const noexcept { return end_; }
  const_pointer cbegin() const noexcept { return data_; }
  const_pointer cend() const noexcept { return end_; }

 private:
  template<typename U, zstl::enable_if_t<can_reallocate<U>::value, int> =0>
  void Reallocate(size_type n) {
      data_ = this->reallocate(data_, n);
      end_ = data_ + n;
  }

  template<typename U, zstl::enable_if_t<!can_reallocate<U>::value, char> =0>
  void Reallocate(size_type n) {
      auto tmp = AllocTraits::allocate(*this, n);
      
      end_ = zstl::UninitializedMoveIfNoexcept(data_, end_, tmp);
      end_ = zstl::UninitializedDefaultConstruct(end_, tmp+n);

      AllocTraits::deallocate(*this, data_, GetSize());
      data_ = tmp;
  }

  template<typename U, zstl::enable_if_t<can_reallocate<U>::value, char> =0>
  void Shrink_impl(size_type n) {
    data_ = this->reallocate(data_, n);
    end_ = data_ + n;
  }

  template<typename U, zstl::enable_if_t<!can_reallocate<U>::value, int> =0>
  void Shrink_impl(size_type n) {
    auto tmp = AllocTraits::allocate(*this, n);
    const auto count = size() - n; 
    
    end_ = zstl::UninitializedMoveIfNoexcept(data_, data_+n, tmp);

    for (size_t i = 0; i < count; ++i) {
      AllocTraits::destroy(*this, data_+n+i);
    }

    AllocTraits::deallocate(*this, data_, GetSize());
    data_ = tmp;
  }

  T* data_;
  T* end_;
};

} // algo
} // mmkv

#endif // _MMKV_ALGO_RESERVED_ARRAY_H_
