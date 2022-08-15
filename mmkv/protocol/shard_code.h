#ifndef MMKV_PROTOCOL_SHARD_CODE_H_
#define MMKV_PROTOCOL_SHARD_CODE_H_

#include <stdint.h>

namespace mmkv {
namespace protocol {

enum ShardCode : uint8_t {
  SC_OK = 0,
  SC_NO_SHARD, /* No such shard in the server */
  SC_NOT_SHARD_SERVER, /* Not a shard server */
};

} // protocol
} // mmkv

#endif // MMKV_PROTOCOL_SHARD_CODE_H