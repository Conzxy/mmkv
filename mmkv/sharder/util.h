#ifndef _MMKV_SHARDER_UTIL_H__
#define _MMKV_SHARDER_UTIL_H__

#include "sharder.pb.h"

#include "mmkv/algo/string.h"
#include "mmkv/util/macro.h"

namespace mmkv {

namespace db {
class MmkvDb;
}

MMKV_INLINE ShardMessage MakeShardRequest()
{
  ShardMessage msg;
  msg.set_type(SHARD_MSG_REQUEST);
  return msg;
}

MMKV_INLINE ShardMessage MakeShardResponse()
{
  ShardMessage msg;
  msg.set_type(SHARD_MSG_RESPONSE);
  return msg;
}

using ShardRequest  = ShardMessage;
using ShardResponse = ShardMessage;

#define SHARDER_TAG      "SMPB"
#define SHARDER_MAX_SIZE (1 << 26)

void SerializeMmbpDataToSharderRequest(
    db::MmkvDb                              *p_db,
    std::vector<const algo::String *> const &keys,
    size_t                                  *p_index,
    ShardMessage                            *p_msg
);

} // namespace mmkv

#endif
