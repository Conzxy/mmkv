#include "shard_reply.h"

#include "mmbp_util.h"
#include "mmbp_request.h"
using namespace mmkv::protocol;

static ShardReply reply;
ShardReply *ShardReply::prototype = &reply;

ShardReply::ShardReply() {

}

ShardReply::~ShardReply() noexcept {
  for (auto &req : reqs) {
    delete req;
  }
}

void ShardReply::SerializeTo(ChunkList &buffer) const {
  SerializeField(code, buffer);
  SerializeField(reqs.size(), buffer);
  for (auto &req : reqs) {
    req->SerializeTo(buffer); 
  }
}

void ShardReply::ParseFrom(Buffer &buffer) {
  SetField(code, buffer);
  reqs.resize(buffer.Read32());
  for (auto &req : reqs) {
    req = new MmbpRequest;
    req->ParseFrom(buffer);
  }
}