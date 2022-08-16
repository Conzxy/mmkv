#include "shard_reply.h"

#include "mmbp_util.h"
#include "mmbp_request.h"
using namespace mmkv::protocol;

static ShardReply reply;
ShardReply *ShardReply::prototype = &reply;

ShardReply::ShardReply() {

}

ShardReply::~ShardReply() noexcept {
}

void ShardReply::SerializeTo(ChunkList &buffer) const {
  SerializeField(code, buffer);
  SerializeField(req_buf, buffer);
}

void ShardReply::ParseFrom(Buffer &buffer) {
  SetField(code, buffer);
  SetField(req_buf, buffer);
}

static char const *ShardCode2String(ShardCode code) noexcept {
  switch (code) {
  case SC_OK:
    return "Ok";
  case SC_NO_SHARD:
    return "No shard";
  case SC_NOT_SHARD_SERVER:
    return "not shard server";
  }
  return "Unknown";
}

void ShardReply::DebugPrint() {
  LOG_DEBUG << "ShardCode: " << ShardCode2String(GetShardCode());
  LOG_DEBUG << "Request buffer size=" << req_buf.size();
}