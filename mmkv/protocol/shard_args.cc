#include "shard_args.h"

#include "mmbp_util.h"

using namespace mmkv::protocol;
using namespace mmkv;

static ShardArgs args;
ShardArgs *ShardArgs::prototype = &args;

ShardArgs::ShardArgs() {

}

void ShardArgs::SerializeTo(ChunkList &buffer) const {
  SerializeField(operation, buffer);
  SerializeField(shard_id, buffer);
}

void ShardArgs::ParseFrom(Buffer &buffer) {
  SetField(operation, buffer);
  SetField(shard_id, buffer);
}

static inline char const *ShardOperation2String(ShardOperation op) noexcept {
  switch (op) {
  case SO_GET_SHARD:
    return "Get shard";
  case SO_REM_SHARD:
    return "Del shard";
  }
  return "unknown ShardOperation";
}

void ShardArgs::DebugPrint() const {
  LOG_DEBUG << "ShardOperation: " << ShardOperation2String(GetOperation());
}