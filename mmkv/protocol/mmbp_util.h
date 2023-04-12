// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_PROTOCOL_MMBP_UTIL_H_
#define _MMKV_PROTOCOL_MMBP_UTIL_H_

#include <unordered_map>

#include "mmkv/algo/string.h"
#include "mmbp.h"

#include "kanon/buffer/chunk_list.h"
#include "kanon/buffer/buffer.h"
#include "mmkv/util/conv.h"

namespace mmkv {
namespace protocol {

using algo::String;

using kanon::Buffer;
using kanon::ChunkList;

/* Original name style:
   SetField()/SerializeField()
   -> ParseField()/SerializeField()  (SetField() is not a good name to
   understand)
   -> ParseComponent()/SerializeComponent()
      (Some non-message struct also follow the MMBP format and don't as a field)
   */
template <typename Alloc>
MMKV_INLINE void
ParseComponent(std::basic_string<char, std::char_traits<char>, Alloc> &str,
               Buffer &buffer, bool is_16 = false)
{
  str.clear();
  auto len = is_16 ? buffer.Read16() : buffer.Read32();
  str.reserve(len);
  str.append(buffer.GetReadBegin(), len);
  buffer.AdvanceRead(len);
}

MMKV_INLINE void ParseComponent(WeightValues &values, Buffer &buffer)
{
  auto count = buffer.Read32();
  values.resize(count);

  size_t value_size = 0;

  for (size_t i = 0; i < count; ++i) {
    values[i].key = util::int2double(buffer.Read64());
    value_size = buffer.Read32();
    values[i].value.append(buffer.GetReadBegin(), value_size);
    buffer.AdvanceRead(value_size);
  }
}

template <typename Alloc>
MMKV_INLINE void ParseComponent(
    std::vector<std::basic_string<char, std::char_traits<char>, Alloc>> &values,
    Buffer &buffer)
{
  auto count = buffer.Read32();
  values.resize(count);

  size_t value_size = 0;

  for (size_t i = 0; i < count; ++i) {
    value_size = buffer.Read32();
    values[i].append(buffer.GetReadBegin(), value_size);
    buffer.AdvanceRead(value_size);
  }
}

MMKV_INLINE void ParseComponent(StrKvs &kvs, Buffer &buffer)
{
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

MMKV_INLINE void ParseComponent(uint8_t &i, Buffer &buffer)
{
  i = buffer.Read8();
}

MMKV_INLINE void ParseComponent(uint16_t &i, Buffer &buffer)
{
  i = buffer.Read16();
}

MMKV_INLINE void ParseComponent(uint32_t &i, Buffer &buffer)
{
  i = buffer.Read32();
}

MMKV_INLINE void ParseComponent(uint64_t &i, Buffer &buffer)
{
  i = buffer.Read64();
}

MMKV_INLINE void ParseComponent(int64_t &i, Buffer &buffer)
{
  i = (int64_t)buffer.Read64();
}

#define SET_FILED_UVEC(bc)                                                     \
  MMKV_INLINE void ParseComponent(std::vector<uint##bc##_t> &u##bc##s,         \
                                  Buffer &buffer)                              \
  {                                                                            \
    u##bc##s.resize(buffer.Read##bc());                                        \
    for (auto &u##bc : u##bc##s) {                                             \
      u##bc = buffer.Read##bc();                                               \
    }                                                                          \
  }                                                                            \
                                                                               \
  MMKV_INLINE void ParseComponent(                                             \
      std::vector<std::vector<uint##bc##_t>> &u##bc##_2d, Buffer &buffer)      \
  {                                                                            \
    u##bc##_2d.resize(buffer.Read##bc());                                      \
    for (auto &u##bc##s : u##bc##_2d) {                                        \
      ParseComponent(u##bc##s, buffer);                                        \
    }                                                                          \
  }

/* Used for has_bits */
MMKV_INLINE void ParseComponent(uint8_t *u8s, size_t len, Buffer &buffer)
{
  for (size_t i = 0; i < len; ++i) {
    u8s[i] = buffer.Read8();
  }
}

SET_FILED_UVEC(16)
SET_FILED_UVEC(32)

MMKV_INLINE void ParseComponent(std::vector<char> &buf, Buffer &buffer)
{
  buf.resize(buffer.Read32());
  memcpy(&buf[0], buffer.GetReadBegin(), buf.size());
  buffer.AdvanceRead(buf.size());
}

template <typename K, typename V>
MMKV_INLINE void ParseComponent(std::unordered_map<K, V> &kv_map,
                                Buffer &buffer)
{
  const auto n = buffer.Read32();
  kv_map.reserve(n);
  for (uint32_t i = 0; i < n; ++i) {
    K key;
    V value;
    ParseComponent(key, buffer);
    ParseComponent(value, buffer);
    kv_map.emplace(std::move(key), std::move(value));
  }
}

template <typename Alloc, typename BT>
MMKV_INLINE void SerializeComponent(
    std::basic_string<char, std::char_traits<char>, Alloc> const &str,
    BT &buffer, bool is_16 = false)
{
  is_16 ? buffer.Append16(str.size()) : buffer.Append32(str.size());
  buffer.Append(str.data(), str.size());
}

template <typename Alloc, typename BT>
MMKV_INLINE void SerializeComponent(
    std::vector<std::basic_string<char, std::char_traits<char>, Alloc>> const
        &values,
    BT &buffer)
{
  buffer.Append32(values.size());

  for (size_t i = 0; i < values.size(); ++i) {
    buffer.Append32(values[i].size());
    buffer.Append(values[i].data(), values[i].size());
  }
}

template <typename BT>
MMKV_INLINE void SerializeComponent(StrKvs const &values, BT &buffer)
{
  buffer.Append32(values.size());

  for (size_t i = 0; i < values.size(); ++i) {
    buffer.Append16(values[i].key.size());
    buffer.Append(values[i].key.data(), values[i].key.size());
    buffer.Append32(values[i].value.size());
    buffer.Append(values[i].value.data(), values[i].value.size());
  }
}

template <typename BT>
MMKV_INLINE void SerializeComponent(WeightValues const &values, BT &buffer)
{
  buffer.Append32(values.size());

  for (size_t i = 0; i < values.size(); ++i) {
    buffer.Append64(util::double2u64(values[i].key));
    buffer.Append32(values[i].value.size());
    buffer.Append(values[i].value.data(), values[i].value.size());
  }
}

template <typename BT>
MMKV_INLINE void SerializeComponent(uint8_t i, BT &buffer)
{
  buffer.Append8(i);
}

template <typename BT>
MMKV_INLINE void SerializeComponent(uint64_t i, BT &buffer)
{
  buffer.Append64(i);
}

template <typename BT>
MMKV_INLINE void SerializeComponent(uint32_t i, BT &buffer)
{
  buffer.Append32(i);
}

template <typename BT>
MMKV_INLINE void SerializeComponent(uint16_t i, BT &buffer)
{
  buffer.Append16(i);
}

// bc = bit count
#define SERIALIZED_FIELD_UVEC(bc)                                              \
  MMKV_INLINE void SerializeComponent(                                         \
      std::vector<uint##bc##_t> const &u##bc##s, ChunkList &buffer)            \
  {                                                                            \
    buffer.Append##bc(u##bc##s.size());                                        \
    for (auto u##bc : u##bc##s)                                                \
      buffer.Append##bc(u##bc);                                                \
  }                                                                            \
                                                                               \
  MMKV_INLINE void SerializeComponent(                                         \
      std::vector<std::vector<uint##bc##_t>> const &u##bc##_2d,                \
      ChunkList &buffer)                                                       \
  {                                                                            \
    buffer.Append##bc(u##bc##_2d.size());                                      \
    for (auto const &u##bc##s : u##bc##_2d)                                    \
      SerializeComponent(u##bc##s, buffer);                                    \
  }

SERIALIZED_FIELD_UVEC(32)
SERIALIZED_FIELD_UVEC(16)

MMKV_INLINE void SerializeComponent(std::vector<char> const &buf,
                                    ChunkList &buffer)
{
  buffer.Append32(buf.size());
  buffer.Append(buf.data(), buf.size());
}

template <typename K, typename V>
MMKV_INLINE void SerializeComponent(std::unordered_map<K, V> const &kv_map,
                                    ChunkList &buffer)
{
  SerializeComponent((uint32_t)kv_map.size(), buffer);
  for (auto const &kv : kv_map) {
    SerializeComponent(kv.first, buffer);
    SerializeComponent(kv.second, buffer);
  }
}

/* Used for has_bits */
MMKV_INLINE void SerializeComponent(uint8_t const *u8s, size_t len,
                                    ChunkList &buffer)
{
  for (size_t i = 0; i < len; ++i)
    buffer.Append32(u8s[i]);
}

MMKV_INLINE void SetBit(uint8_t &bits, int idx) noexcept
{
  bits |= (1 << idx);
}

MMKV_INLINE bool TestBit(uint8_t bits, int idx) noexcept
{
  return bits & (1 << idx);
}

} // namespace protocol
} // namespace mmkv

#endif // _MMKV_PROTOCOL_MMBP_UTIL_H_
