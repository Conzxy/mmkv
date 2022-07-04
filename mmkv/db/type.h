#ifndef _MMKV_DB_TYPE_H_
#define _MMKV_DB_TYPE_H_

#include "mmkv/algo/libc_allocator_with_realloc.h"
#include "mmkv/algo/string.h"
#include "mmkv/algo/blist.h"

namespace mmkv {
namespace db {

using String = algo::String;
using StrList = algo::Blist<String, algo::LibcAllocatorWithRealloc<String>>;

} // db
} // mmkv

#endif // _MMKV_DB_TYPE_H_
