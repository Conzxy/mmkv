#ifndef _MMKV_PROTOCOL_MMBP_H_
#define _MMKV_PROTOCOL_MMBP_H_

#include <stddef.h>
#include <stdint.h>

#include "kanon/buffer/chunk_list.h"
#include "kanon/buffer/buffer.h"
#include "type.h"

namespace mmkv {
namespace protocol {

// Memory Key-Value binary protocol
class MmbpMessage {
 public:
  MmbpMessage() = default;
  virtual ~MmbpMessage() = default;

  virtual void SerializeTo(ChunkList& buffer) const = 0;
  virtual void SerializeTo(Buffer& buffer) const = 0;
  virtual void ParseFrom(Buffer& buffer) = 0;
  // FIXME
  // virtual void SerializeToStream(ChunkList& buffer) const = 0;
  // virtual void ParseFromStream(Buffer& buffer) = 0;
  virtual MmbpMessage *New() const = 0;

};

} // protocol
} // mmkv

#endif // _MMKV_PROTOCOL_MMBP_H_
