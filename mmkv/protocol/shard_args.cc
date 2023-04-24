// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "shard_args.h"

#include "mmbp_util.h"

using namespace mmkv::protocol;
using namespace mmkv;

static ShardArgs args;
ShardArgs *ShardArgs::prototype = &args;

ShardArgs::ShardArgs() {}

void ShardArgs::SerializeTo(ChunkList &buffer) const
{
  SerializeComponent(operation, buffer);
  SerializeComponent(shard_id, buffer);
  if (HasRequests()) {
    SerializeComponent(requests, buffer);
  }
}

void ShardArgs::ParseFrom(Buffer &buffer)
{
  ParseComponent(operation, buffer);
  ParseComponent(shard_id, buffer);
  if (HasRequests()) {
    ParseComponent(requests, buffer);
  }
}

static inline char const *ShardOperation2String(ShardOperation op) noexcept
{
  switch (op) {
    case SO_GET_SHARD:
      return "Get shard";
    case SO_REM_SHARD:
      return "Del shard";
    case SO_PUT_SHARD:
      return "Put shard";
  }
  return "unknown ShardOperation";
}

void ShardArgs::DebugPrint() const
{
  LOG_DEBUG << "ShardOperation: " << ShardOperation2String(GetOperation());
}
