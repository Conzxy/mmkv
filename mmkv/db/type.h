#ifndef _MMKV_DB_TYPE_H_
#define _MMKV_DB_TYPE_H_

#include "mmkv/algo/dictionary.h"
#include "mmkv/algo/hash_set.h"
#include "mmkv/algo/libc_allocator_with_realloc.h"
#include "mmkv/algo/string.h"
#include "mmkv/algo/blist.h"
#include "mmkv/protocol/mmbp.h"
#include "data_type.h"

namespace mmkv {
namespace db {

using protocol::Weight;
using String = algo::String;
using StrList = algo::Blist<String, algo::LibcAllocatorWithRealloc<String>>;
using Map = algo::Dictionary<String, String>;
using Set = algo::HashSet<String>;

} // db
} // mmkv

#endif // _MMKV_DB_TYPE_H_
