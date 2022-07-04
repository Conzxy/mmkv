#ifndef _MMKV_PROTOCOL_MMBP_UTIL_H_
#define _MMKV_PROTOCOL_MMBP_UTIL_H_

#include "mmkv/algo/string.h"
#include "mmbp.h"

#include "kanon/buffer/chunk_list.h"
#include "kanon/buffer/buffer.h"

namespace mmkv {
namespace protocol {

using algo::String;

using kanon::ChunkList;
using kanon::Buffer;

inline void SetField(String& str, Buffer& buffer, bool is_16=false) {
  assert(str.size() == 0);
  auto len = is_16 ? buffer.Read16() : buffer.Read32();
  str.reserve(len);
  str.append(buffer.GetReadBegin(), len);
  buffer.AdvanceRead(len);
}

inline void SetField(StrValues& values, Buffer& buffer) {
  auto count = buffer.Read32();
  values.resize(count); 
  
  size_t value_size = 0;
   
  for (size_t i = 0; i < count; ++i) {
    value_size = buffer.Read32();
    values[i].append(buffer.GetReadBegin(), value_size);
    buffer.AdvanceRead(value_size);
  }
}

inline void SetField(StrKvs& kvs, Buffer& buffer) {
  auto count = buffer.Read32();
  kvs.resize(count);

  size_t key_size = 0;
  size_t value_size = 0;

  for (size_t i = 0; i < count; ++i) {
    key_size = buffer.Read16();
    kvs[i].key.append(buffer.GetReadBegin(), key_size);  
    buffer.AdvanceRead(key_size);
    value_size = buffer.Read32();
    kvs[i].value.append(buffer.GetReadBegin(), value_size);
    buffer.AdvanceRead(value_size);
  }
}

inline void SetField(uint8_t& i, Buffer& buffer) {
  i = buffer.Read8();
}

inline void SetField(uint16_t& i, Buffer& buffer) {
  i = buffer.Read16();
}

inline void SetField(uint32_t& i, Buffer& buffer) {
  i = buffer.Read32();
}

inline void SetField(uint64_t& i, Buffer& buffer) {
  i = buffer.Read64();
}

inline void SerializeField(String const& str, ChunkList& buffer, bool is_16=false) {
  is_16 ? buffer.Append16(str.size()) : buffer.Append32(str.size());
  buffer.Append(str.data(), str.size());
}

inline void SerializeField(StrValues const& values, ChunkList& buffer) {
  buffer.Append32(values.size());
  
  for (size_t i = 0; i < values.size(); ++i) {
    buffer.Append32(values[i].size());
    buffer.Append(values[i].data(), values[i].size());
  }
}

inline void SerializeField(StrKvs const& values, ChunkList& buffer) {
  buffer.Append32(values.size());
  
  for (size_t i = 0; i < values.size(); ++i) {
    buffer.Append16(values[i].key.size());
    buffer.Append(values[i].key.data(), values[i].key.size());
    buffer.Append32(values[i].value.size());
    buffer.Append(values[i].value.data(), values[i].value.size());
  }
}

inline void SerializeField(uint8_t i, ChunkList& buffer) {
  buffer.Append8(i);
}

inline void SerializeField(uint64_t i, ChunkList& buffer) {
  buffer.Append64(i); 
}

inline void SerializeField(uint32_t i, ChunkList& buffer) {
  buffer.Append32(i);
}

inline void SerializeField(uint16_t i, ChunkList& buffer) {
  buffer.Append16(i);
}

inline void SetBit(uint8_t& bits, int idx) noexcept {
  bits |= (1 << idx);
}

inline bool TestBit(uint8_t bits, int idx) noexcept {
  return bits & (1 << idx);
}

} // protocol
} // mmkv

#endif // _MMKV_PROTOCOL_MMBP_UTIL_H_
