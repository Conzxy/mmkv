// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_PROTOCOL_MMBP_UTIL_H_
#define _MMKV_PROTOCOL_MMBP_UTIL_H_

#include <unordered_map>

#include "mmkv/algo/string.h"
#include "mmbp.h"

#include "kanon/buffer/chunk_list.h"
#include "kanon/buffer/buffer.h"
#include "mmkv/util/conv.h"

#include <third-party/kvarint/kvarint.h>

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

/*--------------------------------------------------*/
/* Parse API                                        */
/*--------------------------------------------------*/
#define MMKV_PARSE_UNSIGNED_(bits_)                                            \
  size_t len = 0;                                                              \
  auto err = kvarint_decode##bits_(buffer.GetReadBegin(),                      \
                                   buffer.GetReadableSize(), &len, &i);        \
  MMKV_UNUSED(err);                                                            \
  MMKV_ASSERT1(err == KVARINT_OK);                                             \
  buffer.AdvanceRead(len)

#define MMKV_PARSE_SIGNED_(bits_)                                              \
  size_t len = 0;                                                              \
  auto err = kvarint_decode##bits_##s(buffer.GetReadBegin(),                   \
                                      buffer.GetReadableSize(), &len, &i);     \
  MMKV_UNUSED(err);                                                            \
  MMKV_ASSERT1(err == KVARINT_OK);                                             \
  buffer.AdvanceRead(len)

MMKV_INLINE void ParseComponent(uint8_t &i, Buffer &buffer)
{
  MMKV_PARSE_UNSIGNED_(8);
}

MMKV_INLINE void ParseComponent(uint16_t &i, Buffer &buffer)
{
  MMKV_PARSE_UNSIGNED_(16);
}

MMKV_INLINE void ParseComponent(uint32_t &i, Buffer &buffer)
{
  MMKV_PARSE_UNSIGNED_(32);
}

MMKV_INLINE void ParseComponent(uint64_t &i, Buffer &buffer)
{
  MMKV_PARSE_UNSIGNED_(64);
}

MMKV_INLINE void ParseComponent(int64_t &i, Buffer &buffer)
{
  MMKV_PARSE_SIGNED_(64);
}

template <typename Alloc>
MMKV_INLINE void
ParseComponent(std::basic_string<char, std::char_traits<char>, Alloc> &str,
               Buffer &buffer, bool is_16 = false)
{
  str.clear();
  size_t len = -1;
  if (is_16) {
    ParseComponent((uint16_t &)len, buffer);
  } else {
    ParseComponent((uint32_t &)len, buffer);
  }
  str.reserve(len);
  str.append(buffer.GetReadBegin(), len);
  buffer.AdvanceRead(len);
}

template <typename Alloc>
MMKV_INLINE void ParseComponent(
    std::vector<std::basic_string<char, std::char_traits<char>, Alloc>> &values,
    Buffer &buffer)
{
  uint32_t count = -1;
  ParseComponent(count, buffer);
  values.resize(count);

  size_t value_size = 0;

  for (size_t i = 0; i < count; ++i) {
    ParseComponent(value_size, buffer);
    values[i].append(buffer.GetReadBegin(), value_size);
    buffer.AdvanceRead(value_size);
  }
}

MMKV_INLINE void ParseComponent(WeightValues &values, Buffer &buffer)
{
  uint32_t count = -1;
  ParseComponent(count, buffer);
  values.resize(count);

  size_t value_size = 0;

  for (size_t i = 0; i < count; ++i) {
    values[i].key = util::int2double(buffer.Read64());
    ParseComponent(value_size, buffer);
    values[i].value.append(buffer.GetReadBegin(), value_size);
    buffer.AdvanceRead(value_size);
  }
}

MMKV_INLINE void ParseComponent(StrKvs &kvs, Buffer &buffer)
{
  uint32_t count = -1;
  ParseComponent(count, buffer);
  kvs.resize(count);

  size_t key_size = 0;
  size_t value_size = 0;

  for (size_t i = 0; i < count; ++i) {
    ParseComponent(key_size, buffer);
    kvs[i].key.append(buffer.GetReadBegin(), key_size);
    buffer.AdvanceRead(key_size);
    ParseComponent(value_size, buffer);
    kvs[i].value.append(buffer.GetReadBegin(), value_size);
    buffer.AdvanceRead(value_size);
  }
}

#define SET_FILED_UVEC(bc)                                                     \
  MMKV_INLINE void ParseComponent(std::vector<uint##bc##_t> &u##bc##s,         \
                                  Buffer &buffer)                              \
  {                                                                            \
    uint##bc##_t size = -1;                                                    \
    ParseComponent(size, buffer);                                              \
    u##bc##s.resize(size);                                                     \
    for (auto &u##bc : u##bc##s) {                                             \
      ParseComponent(u##bc, buffer);                                           \
    }                                                                          \
  }                                                                            \
                                                                               \
  MMKV_INLINE void ParseComponent(                                             \
      std::vector<std::vector<uint##bc##_t>> &u##bc##_2d, Buffer &buffer)      \
  {                                                                            \
    uint##bc##_t size = -1;                                                    \
    ParseComponent(size, buffer);                                              \
    u##bc##_2d.resize(size);                                                   \
    for (auto &u##bc##s : u##bc##_2d) {                                        \
      ParseComponent(u##bc##s, buffer);                                        \
    }                                                                          \
  }

/* Used for has_bits */
MMKV_INLINE void ParseComponent(uint8_t *u8s, size_t len, Buffer &buffer)
{
  for (size_t i = 0; i < len; ++i) {
    ParseComponent(u8s[i], buffer);
  }
}

SET_FILED_UVEC(16)
SET_FILED_UVEC(32)

MMKV_INLINE void ParseComponent(std::vector<char> &buf, Buffer &buffer)
{
  uint32_t len = -1;
  ParseComponent(len, buffer);
  buf.resize(len);
  memcpy(&buf[0], buffer.GetReadBegin(), buf.size());
  buffer.AdvanceRead(buf.size());
}

template <typename K, typename V>
MMKV_INLINE void ParseComponent(std::unordered_map<K, V> &kv_map,
                                Buffer &buffer)
{
  uint32_t n = -1;
  ParseComponent(n, buffer);
  kv_map.reserve(n);
  for (uint32_t i = 0; i < n; ++i) {
    K key;
    V value;
    ParseComponent(key, buffer);
    ParseComponent(value, buffer);
    kv_map.emplace(std::move(key), std::move(value));
  }
}

/*--------------------------------------------------*/
/* Serialize API                                    */
/*--------------------------------------------------*/
template <typename BT>
MMKV_INLINE void SerializeComponent(uint8_t i, BT &buffer)
{
  kvarint_buf8_t buf;
  kvarint_encode8(i, &buf);
  buffer.Append(buf.buf, buf.len);
}

template <typename BT>
MMKV_INLINE void SerializeComponent(uint64_t i, BT &buffer)
{
  kvarint_buf16_t buf;
  kvarint_encode16(i, &buf);
  buffer.Append(buf.buf, buf.len);
}

template <typename BT>
MMKV_INLINE void SerializeComponent(uint32_t i, BT &buffer)
{
  kvarint_buf32_t buf;
  kvarint_encode32(i, &buf);
  buffer.Append(buf.buf, buf.len);
}

template <typename BT>
MMKV_INLINE void SerializeComponent(uint16_t i, BT &buffer)
{
  kvarint_buf64_t buf;
  kvarint_encode64(i, &buf);
  buffer.Append(buf.buf, buf.len);
}

template <typename BT>
MMKV_INLINE void SerializeComponent(int64_t i, BT &buffer)
{
  kvarint_buf64_t buf;
  kvarint_encode64s(i, &buf);
  buffer.Append(buf.buf, buf.len);
}

template <typename Alloc, typename BT>
MMKV_INLINE void SerializeComponent(
    std::basic_string<char, std::char_traits<char>, Alloc> const &str,
    BT &buffer, bool is_16 = false)
{
  if (is_16) {
    SerializeComponent((uint16_t)str.size(), buffer);
  } else {
    SerializeComponent((uint32_t)str.size(), buffer);
  }
  buffer.Append(str.data(), str.size());
}

template <typename Alloc, typename BT>
MMKV_INLINE void SerializeComponent(
    std::vector<std::basic_string<char, std::char_traits<char>, Alloc>> const
        &values,
    BT &buffer)
{
  SerializeComponent((uint32_t)values.size(), buffer);

  for (size_t i = 0; i < values.size(); ++i) {
    SerializeComponent((uint32_t)values[i].size(), buffer);
    buffer.Append(values[i].data(), values[i].size());
  }
}

template <typename BT>
MMKV_INLINE void SerializeComponent(StrKvs const &values, BT &buffer)
{
  SerializeComponent((uint32_t)values.size(), buffer);

  for (size_t i = 0; i < values.size(); ++i) {
    SerializeComponent((uint16_t)values[i].key.size(), buffer);
    buffer.Append(values[i].key.data(), values[i].key.size());
    SerializeComponent((uint32_t)values[i].value.size(), buffer);
    buffer.Append(values[i].value.data(), values[i].value.size());
  }
}

template <typename BT>
MMKV_INLINE void SerializeComponent(WeightValues const &values, BT &buffer)
{
  SerializeComponent((uint32_t)values.size(), buffer);

  for (size_t i = 0; i < values.size(); ++i) {
    buffer.Append64(util::double2u64(values[i].key));
    SerializeComponent((uint32_t)values[i].value.size(), buffer);
    buffer.Append(values[i].value.data(), values[i].value.size());
  }
}

// bc = bit count
#define SERIALIZED_FIELD_UVEC(bc)                                              \
  MMKV_INLINE void SerializeComponent(                                         \
      std::vector<uint##bc##_t> const &u##bc##s, ChunkList &buffer)            \
  {                                                                            \
    SerializeComponent(u##bc##s.size(), buffer);                               \
    for (auto u##bc : u##bc##s)                                                \
      buffer.Append##bc(u##bc);                                                \
  }                                                                            \
                                                                               \
  MMKV_INLINE void SerializeComponent(                                         \
      std::vector<std::vector<uint##bc##_t>> const &u##bc##_2d,                \
      ChunkList &buffer)                                                       \
  {                                                                            \
    SerializeComponent(u##bc##_2d.size(), buffer);                             \
    for (auto const &u##bc##s : u##bc##_2d)                                    \
      SerializeComponent(u##bc##s, buffer);                                    \
  }

SERIALIZED_FIELD_UVEC(32)
SERIALIZED_FIELD_UVEC(16)

MMKV_INLINE void SerializeComponent(std::vector<char> const &buf,
                                    ChunkList &buffer)
{
  SerializeComponent(buf.size(), buffer);
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
    SerializeComponent(u8s[i], buffer);
}

MMKV_INLINE void SetBit(uint8_t &bits, int idx) noexcept { bits |= (1 << idx); }

MMKV_INLINE bool TestBit(uint8_t bits, int idx) noexcept
{
  return bits & (1 << idx);
}

} // namespace protocol
} // namespace mmkv

#endif // _MMKV_PROTOCOL_MMBP_UTIL_H_
