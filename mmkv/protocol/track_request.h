// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef MMKV_PROTOCOL_TRACK_REQUEST_H_
#define MMKV_PROTOCOL_TRACK_REQUEST_H_

#include "mmbp.h"
#include "common.h"

namespace mmkv {
namespace protocol {

enum TrackOperation : uint8_t {
  TO_ADD_NODE = 0,
  TO_MOVE_SHARD,
  TO_MOVE_FINISH,
  TO_LEAVE,
  TO_QUERY,
};

class TrackRequest : public MmbpMessage {
 public:
  TrackRequest() noexcept;
  void SerializeTo(ChunkList &buffer) const override;
  void ParseFrom(Buffer &buffer) override;
  MmbpMessage *New() const override { return new TrackRequest(); }
  void DebugPrint();

  TrackOperation GetOperation() const noexcept { return (TrackOperation)operation; }

  FIELD_SET_HAS_DEFINE(ShardId, 0, 0)
  FIELD_SET_HAS_DEFINE(NodeId, 0, 1)
  FIELD_SET_HAS_DEFINE(SharderPort, 0, 2)

  uint8_t operation;
  uint32_t shard_id;
  uint32_t node_id;
  uint16_t sharder_port;

  static TrackRequest *prototype;
 private:
  uint8_t has_bits_[1];
};

} // protocol
} // mmkv

#endif
