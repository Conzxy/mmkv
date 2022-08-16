#ifndef MMKV_PROTOCOL_SHARD_ARGS_H_
#define MMKV_PROTOCOL_SHARD_ARGS_H_

#include "mmbp.h"
#include "common.h"

namespace mmkv {
namespace protocol {

enum ShardOperation : uint8_t {
  SO_GET_SHARD = 0,
  SO_REM_SHARD,
};

class ShardArgs : public MmbpMessage {
 public:
  ShardArgs();
  ~ShardArgs() = default;

  void SerializeTo(ChunkList &buffer) const override;
  void ParseFrom(Buffer &buffer) override;
  ShardArgs *New() const override { return new ShardArgs(); }
  void DebugPrint() const;

  ShardOperation GetOperation() const noexcept { return (ShardOperation)operation; }

  uint8_t operation;
  uint32_t shard_id;

  static ShardArgs *prototype;
};

} // protocol
} // mmkv

#endif // MMKV_PROTOCOL_SHARD_ARGS_H_