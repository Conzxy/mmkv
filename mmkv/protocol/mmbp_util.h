#ifndef _MMKV_PROTOCOL_MMBP_UTIL_H_
#define _MMKV_PROTOCOL_MMBP_UTIL_H_

#include "mmkv/algo/string.h"

#include "kanon/buffer/chunk_list.h"
#include "kanon/buffer/buffer.h"

namespace mmkv {
namespace protocol {

using algo::String;

using kanon::ChunkList;
using kanon::Buffer;

inline void SetField(String& str, Buffer& buffer) {
  assert(str.size() == 0);
  auto len = buffer.Read32();
  str.reserve(len);
  str.append(buffer.GetReadBegin(), len);
  buffer.AdvanceRead(len);
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

inline void SerializeField(String const& str, ChunkList& buffer) {
  buffer.Append32(str.size());
  buffer.Append(str.data(), str.size());
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
