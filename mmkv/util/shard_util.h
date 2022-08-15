#ifndef MMKV_UTIL_SHARD_UTIL_H_
#define MMKV_UTIL_SHARD_UTIL_H_

#include "mmkv/protocol/type.h"

#include <xxhash.h>

#define DEFAULT_SHARD_NUM (1 << 12) /** Default number of shard */

namespace mmkv {

using protocol::Shard;
using protocol::String;
inline Shard MakeShardId(String const &key) noexcept {
  return XXH32(key.c_str(), key.size(), 0);
}

} // mmkv

#endif // MMKV_UTIL_SHARD_UTIL_H_