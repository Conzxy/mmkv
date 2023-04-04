#ifndef KANON_ALGO_BARRAY_H__
#define KANON_ALGO_BARRAY_H__

#include <cstddef> // size_t
#include <utility> //
#include <memory>

#include "libc_allocator_with_realloc.h"
#include "mmkv/util/macro.h"
#include "reserved_array.h"

namespace mmkv {
namespace algo {

template <typename T, typename Alloc = LibcAllocatorWithRealloc<T>>
class Barray : protected Alloc {
 public:
  Barray() = default;

 private:
  ReservedArray<T, Alloc> data_;
};

} // namespace algo
} // namespace mmkv
#endif
