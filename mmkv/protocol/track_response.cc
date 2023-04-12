#include "track_response.h"

#include <stdlib.h>

#include "mmbp_util.h"

using namespace mmkv::protocol;

static TrackResponse s_prototype;
TrackResponse *TrackResponse::prototype = &s_prototype;

static char const *TrackStatusCode2String(TrackStatusCode code) noexcept;

TrackResponse::TrackResponse() noexcept
{
  memset(has_bits_, 0, sizeof has_bits_);
}

void TrackResponse::SerializeTo(ChunkList &buffer) const
{
  SerializeComponent(status_code, buffer);
  SerializeComponent(has_bits_[0], buffer);
  if (HasNodeId()) SerializeComponent(node_id, buffer);
  if (HasShard2D()) SerializeComponent(shard_2d, buffer);
  if (HasAddrs()) {
    SerializeComponent(addrs, buffer);
    SerializeComponent(ports, buffer);
  }
  if (HasNodes()) {
    SerializeComponent(nodes, buffer);
  }
}

void TrackResponse::ParseFrom(Buffer &buffer)
{
  ParseComponent(status_code, buffer);
  ParseComponent(has_bits_[0], buffer);
  if (HasNodeId()) ParseComponent(node_id, buffer);
  if (HasShard2D()) ParseComponent(shard_2d, buffer);
  if (HasAddrs()) {
    ParseComponent(addrs, buffer);
    ParseComponent(ports, buffer);
  }
  if (HasNodes()) {
    ParseComponent(nodes, buffer);
  }
}

void TrackResponse::DebugPrint()
{
  LOG_DEBUG << "status_code: " << TrackStatusCode2String(GetStatusCode());
  LOG_DEBUG << "HasAddrs: " << HasAddrs();
  LOG_DEBUG << "HasShard2D: " << HasShard2D();
  LOG_DEBUG << "HasNodeId: " << HasNodeId();
  LOG_DEBUG << "HasNodes: " << HasNodes();
}

inline char const *TrackStatusCode2String(TrackStatusCode code) noexcept
{
  switch (code) {
    case TS_OK:
      return "Ok";
    case TS_WAIT:
      return "Wait";
    case TS_ADD_NODE_OK:
      return "Add node ok";
    case TS_LEAVE_OK:
      return "Leave ok";
    case TS_QUERY_OK:
      return "Query ok";
  }
  // Not reached
  return "Unknown TrackStatusCode";
}
