#ifndef _MMKV_ALGO_COMPARATOR_UTIL_H_
#define _MMKV_ALGO_COMPARATOR_UTIL_H_

#include <string>
#include <string.h>

namespace mmkv {
namespace algo {

template<typename T>
struct Comparator {
  static_assert(sizeof(T) < 0, "The specialization of Comparator<T> isn't defined");
};

template<>
struct Comparator <int> {
  inline int operator()(int x, int y) const noexcept {
    return x - y;
  }
};

template<typename Alloc>
struct Comparator <std::basic_string<char, std::char_traits<char>, Alloc>> {
  using StrType = std::basic_string<char, std::char_traits<char>, Alloc>;
  inline int operator()(StrType const& x, StrType const& y) const noexcept {
    return ::strcmp(x.c_str(), y.c_str());
  }
};

} // algo
} // mmkv

#endif // _MMKV_ALGO_COMPARATOR_UTIL_H_