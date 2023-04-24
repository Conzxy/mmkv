// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef MMKV_PROTOCOL_SHARD_REPLY_H_
#define MMKV_PROTOCOL_SHARD_REPLY_H_

#include "mmbp.h"
#include "shard_code.h"

namespace mmkv {
namespace protocol {

class MmbpRequest;
class ShardReply : public MmbpMessage {
 public:
  ShardReply();
  ~ShardReply() noexcept;

  void SerializeTo(ChunkList &buffer) const override;
  void ParseFrom(Buffer &buffer) override;
  ShardReply *New() const override { return new ShardReply(); }
  void DebugPrint();
  ShardCode GetShardCode() const noexcept { return (ShardCode)code; }

  static ShardReply *prototype;

  uint8_t code;
  // std::vector<MmbpRequest*> reqs;
  std::vector<char> req_buf{};
};

} // protocol
} // mmkv

#endif // MMKV_PROTOCOL_SHARD_REPLY_H_