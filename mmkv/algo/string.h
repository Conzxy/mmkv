#ifndef _MMKV_ALGO_STRING_H_
#define _MMKV_ALGO_STRING_H_

#include <kanon/net/user_common.h>
#include <string>
#include <kanon/string/lexical_stream.h>

#include "mmkv/algo/libc_allocator_with_realloc.h"

namespace mmkv {
namespace algo {

using String = std::basic_string<char, std::char_traits<char>, LibcAllocatorWithRealloc<char>>;

} // algo
} // mmkv

namespace kanon {

template<unsigned SZ>
LexicalStream<SZ>& operator<<(LexicalStream<SZ>& stream, mmkv::algo::String const& str) noexcept {
  stream.Append(str.data(), str.size());
  return stream;
}

} // kanon

// namespace std {

// inline std::ostream& operator<<(std::ostream& os, mmkv::algo::String const& str) {
//   ::fprintf(::stdout, str.data(), str.size());
//   return os;
// }
// } // std

#endif // _MMKV_ALGO_STRING_H_
