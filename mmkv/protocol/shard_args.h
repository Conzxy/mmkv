// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef MMKV_PROTOCOL_SHARD_ARGS_H_
#define MMKV_PROTOCOL_SHARD_ARGS_H_

#include "common.h"
#include "mmbp.h"

namespace mmkv {
namespace protocol {

enum ShardOperation : uint8_t {
  SO_GET_SHARD = 0,
  SO_REM_SHARD,
  SO_PUT_SHARD,
};

class ShardArgs : public MmbpMessage {
 public:
  ShardArgs();
  ~ShardArgs() = default;

  void SerializeTo(ChunkList &buffer) const override;
  void ParseFrom(Buffer &buffer) override;
  ShardArgs *New() const override
  {
    return new ShardArgs();
  }
  void DebugPrint() const;
  FIELD_SET_HAS_DEFINE(Requests, 0, 0);

  ShardOperation GetOperation() const noexcept
  {
    return (ShardOperation)operation;
  }

  uint8_t operation;
  uint32_t shard_id;

  std::vector<char> requests; /** Used for PutShard */
  static ShardArgs *prototype;

 private:
  bool has_bits_[1];
};

} // namespace protocol
} // namespace mmkv

#endif // MMKV_PROTOCOL_SHARD_ARGS_H_
