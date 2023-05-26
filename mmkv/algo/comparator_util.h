// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_ALGO_COMPARATOR_UTIL_H_
#define _MMKV_ALGO_COMPARATOR_UTIL_H_

#include <string>
#include <string.h>

namespace mmkv {
namespace algo {

template <typename T>
struct Comparator {
  static_assert(sizeof(T) < 0, "The specialization of Comparator<T> isn't defined");
};

#define MMKV_DEF_COMPARATOR_SPEC_SIGNED_(t_)                                                       \
  template <>                                                                                      \
  struct Comparator<t_> {                                                                          \
    inline int operator()(t_ x, t_ y) const noexcept                                               \
    {                                                                                              \
      return x - y;                                                                                \
    }                                                                                              \
  }

MMKV_DEF_COMPARATOR_SPEC_SIGNED_(int8_t);
MMKV_DEF_COMPARATOR_SPEC_SIGNED_(int16_t);
MMKV_DEF_COMPARATOR_SPEC_SIGNED_(int32_t);
MMKV_DEF_COMPARATOR_SPEC_SIGNED_(int64_t);
// MMKV_DEF_COMPARATOR_SPEC_SIGNED_(int);

/* To unsigned integer,
 * can't return x - y since
 * (signed)(x - y(y > 2^31-1)) > 0
 */
#define MMKV_DEF_COMPARATOR_SPEC_UNSIGNED_(t_)                                                     \
  template <>                                                                                      \
  struct Comparator<t_> {                                                                          \
    inline int32_t operator()(t_ x, t_ y) const noexcept                                           \
    {                                                                                              \
      return (x < y) ? -1 : ((x == y) ? 0 : 1);                                                    \
    }                                                                                              \
  }

MMKV_DEF_COMPARATOR_SPEC_UNSIGNED_(uint8_t);
MMKV_DEF_COMPARATOR_SPEC_UNSIGNED_(uint16_t);
MMKV_DEF_COMPARATOR_SPEC_UNSIGNED_(uint32_t);
MMKV_DEF_COMPARATOR_SPEC_UNSIGNED_(uint64_t);

template <typename Alloc>
struct Comparator<std::basic_string<char, std::char_traits<char>, Alloc>> {
  using StrType = std::basic_string<char, std::char_traits<char>, Alloc>;
  inline int operator()(StrType const &x, StrType const &y) const noexcept
  {
    return ::strcmp(x.c_str(), y.c_str());
  }
};

template <typename T>
struct Comparator<T *> {
  inline int operator()(T *const x, T *const y) const noexcept { return x - y; }
};

} // namespace algo
} // namespace mmkv

#endif // _MMKV_ALGO_COMPARATOR_UTIL_H_
