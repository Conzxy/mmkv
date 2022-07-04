#ifndef _MMKV_PROTOCOL_MMBP_H_
#define _MMKV_PROTOCOL_MMBP_H_

#include <stddef.h>
#include <stdint.h>
#include <vector>

#include "mmkv/algo/key_value.h"
#include "mmkv/algo/reserved_array.h"
#include "mmkv/algo/string.h"

#include "kanon/buffer/chunk_list.h"
#include "kanon/buffer/buffer.h"

namespace mmkv {
namespace protocol {

using algo::KeyValue;
using algo::String;

using StrKeyValue = KeyValue<String, String>;
// 需要预分配且capacity != size ==> 采用std::vector
using StrKvs = std::vector<StrKeyValue>;
using StrValues = std::vector<String>;

// Memory Key-Value binary protocol
class MmbpMessage {
 public:
  MmbpMessage() = default;
  virtual ~MmbpMessage() = default;

  virtual void SerializeTo(ChunkList& buffer) const = 0;
  virtual void ParseFrom(Buffer& buffer) = 0;
  // FIXME
  // virtual void SerializeToStream(ChunkList& buffer) const = 0;
  // virtual void ParseFromStream(Buffer& buffer) = 0;
  virtual MmbpMessage *New() const = 0;

};

} // protocol
} // mmkv

#endif // _MMKV_PROTOCOL_MMBP_H_
