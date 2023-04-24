// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_UTIL_MACRO_H_
#define _MMKV_UTIL_MACRO_H_

#include <stdlib.h>

#define MMKV_ASSERT(cond, msg) assert((cond) && (msg))
#define MMKV_ASSERT1(cond)     assert(cond)

#define MMKV_UNUSED(x) (void)x

// To prvalue, this is ill-formed
#define MMKV_INT2DOUBLE(i) (*(double *)&i)
#define MMKV_DOUBLE2INT(d) (*(int64_t *)&d)

#if defined(__GNUC__) || defined(__clang__)
#  define MMKV_ALWAYS_INLINE __attribute__((always_inline))
#elif defined(_MSC_VER) /* && !defined(__clang__) */
#  define MMKV_ALWAYS_INLINE __forceinline
#else
#  define MMKV_ALWAYS_INLINE
#endif // !defined(__GNUC__) || defined(__clang__)

#define MMKV_INLINE inline MMKV_ALWAYS_INLINE

#define MMKV_FORWARD(Args__, args__) std::forward<Args__>(args__)...

template <typename T1, typename T2>
MMKV_INLINE bool MMKV_MIN(T1 const &x, T2 const &y) noexcept
{
  return (x < y) ? x : y;
}

template <typename T1, typename T2>
MMKV_INLINE bool MMKV_MAX(T1 const &x, T2 const &y) noexcept
{
  return (x < y) ? y : x;
}

#endif // _MMKV_UTIL_MACRO_H_
