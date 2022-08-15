#include "track_request.h"

#include <stdlib.h>

#include "mmbp_util.h"

using namespace mmkv::protocol;

static TrackRequest s_prototype;
TrackRequest *TrackRequest::prototype = &s_prototype;

TrackRequest::TrackRequest() noexcept {
  memset(has_bits_, 0, sizeof has_bits_);
}

void TrackRequest::SerializeTo(ChunkList &buffer) const {
  SerializeField(operation, buffer);
  SerializeField(has_bits_[0], buffer);
  if (HasShardId())
    SerializeField(shard_id, buffer);
  if (HasNodeId())
    SerializeField(node_id, buffer);
}

void TrackRequest::ParseFrom(Buffer &buffer) {
  SetField(operation, buffer);
  SetField(has_bits_[0], buffer);
  if (HasShardId())
    SetField(shard_id, buffer);
  if (HasNodeId())
    SetField(node_id, buffer);
}

static char const *TrackOperation2String(TrackOperation op) noexcept {
  switch (op) {
  case TO_ADD_NODE: return "Add node";
  case TO_MOVE_SHARD: return "Move shard";
  case TO_MOVE_FINISH: return "Move finish";
  }
  return "Unknown TrackOperation";
}

void TrackRequest::DebugPrint() {
  LOG_DEBUG << "TrackOperation: " << TrackOperation2String(GetOperation());
  LOG_DEBUG << "HasNode: " << HasNodeId();
  if (HasNodeId()) {
    LOG_DEBUG << "NodeId=" << node_id;
  }

  LOG_DEBUG << "HasShardId: " << HasShardId();
  if (HasNodeId()) {
    LOG_DEBUG << "ShardId=" << shard_id;
  }
}