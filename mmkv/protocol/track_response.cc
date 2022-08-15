#include "track_response.h"

#include <stdlib.h>

#include "mmbp_util.h"

using namespace mmkv::protocol;

static TrackResponse s_prototype;
TrackResponse *TrackResponse::prototype = &s_prototype;

static char const* TrackStatusCode2String(TrackStatusCode code) noexcept;

TrackResponse::TrackResponse() noexcept {
  memset(has_bits_, 0, sizeof has_bits_);
}

void TrackResponse::SerializeTo(ChunkList &buffer) const {
  SerializeField(status_code, buffer);
  SerializeField(has_bits_[0], buffer);
  if (HasNodeId()) SerializeField(node_id, buffer);
  if (HasShard2D()) SerializeField(shard_2d, buffer);
  if (HasAddrs()) { 
    SerializeField(addrs, buffer);
    SerializeField(ports, buffer);
  }
}

void TrackResponse::ParseFrom(Buffer &buffer) {
  SetField(status_code, buffer);
  SetField(has_bits_[0], buffer);
  if (HasNodeId()) SetField(node_id, buffer);
  if (HasShard2D()) SetField(shard_2d, buffer);
  if (HasAddrs()) {
    SetField(addrs, buffer);
    SetField(ports, buffer);
  }
}

void TrackResponse::DebugPrint() {
  LOG_DEBUG << "status_code: " << TrackStatusCode2String(GetStatusCode());
  LOG_DEBUG << "HasAddrs: " << HasAddrs();
  LOG_DEBUG << "HasShard2D: " << HasShard2D();
  LOG_DEBUG << "HasNodeId: " << HasNodeId();
}

inline char const* TrackStatusCode2String(TrackStatusCode code) noexcept {
  switch (code) {
  case TS_OK:
    return "Ok";
  case TS_WAIT:
    return "Wait";
  case TS_ADD_NODE_OK:
    return "Add node ok";
  }
  // Not reached
  return "Unknown TrackStatusCode";
}