#ifndef MMKV_PROTOCOL_TRACK_RESPONSE_H_
#define MMKV_PROTOCOL_TRACK_RESPONSE_H_

#include "mmbp.h"
#include "common.h"

namespace mmkv {
namespace protocol {

enum TrackStatusCode : uint8_t {
  TS_OK = 0,
  TS_WAIT, /* Wait other node */
  TS_ADD_NODE_OK,
};

class TrackResponse : public MmbpMessage {
 public:
  TrackResponse() noexcept;

  void SerializeTo(ChunkList &buffer) const override;
  void ParseFrom(Buffer &buffer) override;
  void DebugPrint();

  MmbpMessage *New() const override { return new TrackResponse(); }

  FIELD_SET_HAS_DEFINE(Shard2D, 0, 0)
  FIELD_SET_HAS_DEFINE(Addrs, 0, 1)
  FIELD_SET_HAS_DEFINE(NodeId, 0, 2)

  TrackStatusCode GetStatusCode() const noexcept { return (TrackStatusCode)status_code; }

  uint8_t status_code;
  uint32_t node_id;
  std::vector<std::vector<uint32_t>> shard_2d;
  std::vector<std::string> addrs;
  std::vector<uint16_t> ports;
  static TrackResponse *prototype;
 private:
  uint8_t has_bits_[1];
};

}
}

#endif