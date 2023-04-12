#include "track_request.h"

#include <stdlib.h>

#include "mmbp_util.h"

using namespace mmkv::protocol;

static TrackRequest s_prototype;
TrackRequest *TrackRequest::prototype = &s_prototype;

TrackRequest::TrackRequest() noexcept
{
  memset(has_bits_, 0, sizeof has_bits_);
}

void TrackRequest::SerializeTo(ChunkList &buffer) const
{
  SerializeComponent(operation, buffer);
  SerializeComponent(has_bits_[0], buffer);
  if (HasShardId()) SerializeComponent(shard_id, buffer);
  if (HasNodeId()) SerializeComponent(node_id, buffer);
  if (HasSharderPort()) SerializeComponent(sharder_port, buffer);
}

void TrackRequest::ParseFrom(Buffer &buffer)
{
  ParseComponent(operation, buffer);
  ParseComponent(has_bits_[0], buffer);
  if (HasShardId()) ParseComponent(shard_id, buffer);
  if (HasNodeId()) ParseComponent(node_id, buffer);
  if (HasSharderPort()) ParseComponent(sharder_port, buffer);
}

static char const *TrackOperation2String(TrackOperation op) noexcept
{
  switch (op) {
    case TO_ADD_NODE:
      return "Add node";
    case TO_MOVE_SHARD:
      return "Move shard";
    case TO_MOVE_FINISH:
      return "Move finish";
    case TO_LEAVE:
      return "Leave";
  }
  return "Unknown TrackOperation";
}

void TrackRequest::DebugPrint()
{
  LOG_DEBUG << "TrackOperation: " << TrackOperation2String(GetOperation());
  LOG_DEBUG << "HasNode: " << HasNodeId();
  if (HasNodeId()) {
    LOG_DEBUG << "NodeId=" << node_id;
  }

  LOG_DEBUG << "HasShardId: " << HasShardId();
  if (HasNodeId()) {
    LOG_DEBUG << "ShardId=" << shard_id;
  }

  LOG_DEBUG << "HasSharderPort: " << HasSharderPort();
  if (HasSharderPort()) LOG_DEBUG << "SharderPort=" << sharder_port;
}
