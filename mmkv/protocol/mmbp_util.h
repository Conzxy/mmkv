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
#define MMKV_PARSE_UNSIGNED_(bits_)                                                                \
  size_t len = 0;                                                                                  \
  auto   err = kvarint_decode##bits_(buffer.GetReadBegin(), buffer.GetReadableSize(), &len, &i);   \
  MMKV_UNUSED(err);                                                                                \
  MMKV_ASSERT1(err == KVARINT_OK);                                                                 \
  buffer.AdvanceRead(len)

#define MMKV_PARSE_UNSIGNED_RAW_(bits_)                                                            \
  auto   pp_data8 = reinterpret_cast<char const **>(pp_data);                                      \
  auto   p_data   = reinterpret_cast<char const *>(*pp_data);                                      \
  size_t len      = 0;                                                                             \
  auto   err      = kvarint_decode##bits_(p_data, *p_size, &len, &i);                              \
  MMKV_UNUSED(err);                                                                                \
  MMKV_ASSERT1(err == KVARINT_OK);                                                                 \
  *pp_data8 += len;                                                                                \
  *p_size   -= len;

#define MMKV_PARSE_SIGNED_(bits_)                                                                   \
  size_t len = 0;                                                                                   \
  auto   err = kvarint_decode##bits_##s(buffer.GetReadBegin(), buffer.GetReadableSize(), &len, &i); \
  MMKV_UNUSED(err);                                                                                 \
  MMKV_ASSERT1(err == KVARINT_OK);                                                                  \
  buffer.AdvanceRead(len)

#define MMKV_PARSE_SIGNED_RAW_(bits_)                                                              \
  auto   pp_data8 = reinterpret_cast<char const **>(pp_data);                                      \
  auto   p_data   = reinterpret_cast<char const *>(*pp_data);                                      \
  size_t len      = 0;                                                                             \
  auto   err      = kvarint_decode##bits_##s(p_data, *p_size, &len, &i);                           \
  MMKV_UNUSED(err);                                                                                \
  MMKV_ASSERT1(err == KVARINT_OK);                                                                 \
  *pp_data8 += len;                                                                                \
  *p_size   -= len;

#define DEF_PARSE_UNSIGNED_RAW_(bits_)                                                             \
  MMKV_INLINE void ParseComponent(uint##bits_##_t &i, void const **pp_data, size_t *p_size)        \
  {                                                                                                \
    MMKV_PARSE_UNSIGNED_RAW_(bits_);                                                               \
  }

#define DEF_PARSE_SIGNED_RAW_(bits_)                                                               \
  MMKV_INLINE void ParseComponent(int##bits_##_t &i, void const **pp_data, size_t *p_size)         \
  {                                                                                                \
    MMKV_PARSE_SIGNED_RAW_(bits_);                                                                 \
  }

#define PREPARE_RAW_VAR_ auto pp_data8 = (char const **)pp_data

DEF_PARSE_UNSIGNED_RAW_(8)
DEF_PARSE_UNSIGNED_RAW_(16)
DEF_PARSE_UNSIGNED_RAW_(32)
DEF_PARSE_UNSIGNED_RAW_(64)
DEF_PARSE_SIGNED_RAW_(8)
DEF_PARSE_SIGNED_RAW_(16)
DEF_PARSE_SIGNED_RAW_(32)
DEF_PARSE_SIGNED_RAW_(64)

MMKV_INLINE void ParseComponent(uint8_t &i, Buffer &buffer) { MMKV_PARSE_UNSIGNED_(8); }

MMKV_INLINE void ParseComponent(uint16_t &i, Buffer &buffer) { MMKV_PARSE_UNSIGNED_(16); }

MMKV_INLINE void ParseComponent(uint32_t &i, Buffer &buffer) { MMKV_PARSE_UNSIGNED_(32); }

MMKV_INLINE void ParseComponent(uint64_t &i, Buffer &buffer) { MMKV_PARSE_UNSIGNED_(64); }

MMKV_INLINE void ParseComponent(int64_t &i, Buffer &buffer) { MMKV_PARSE_SIGNED_(64); }

template <typename Alloc>
MMKV_INLINE void ParseComponent(
    std::basic_string<char, std::char_traits<char>, Alloc> &str,
    Buffer                                                 &buffer,
    bool                                                    is_16 = false
)
{
  str.clear();
  size_t len = 0;
  if (is_16) {
    uint16_t out;
    ParseComponent(out, buffer);
    len = out;
  } else {
    uint32_t out;
    ParseComponent(out, buffer);
    len = out;
  }
  str.reserve(len);
  str.append(buffer.GetReadBegin(), len);
  buffer.AdvanceRead(len);
}

template <typename Alloc>
MMKV_INLINE void ParseComponent(
    std::basic_string<char, std::char_traits<char>, Alloc> &str,
    void const                                            **pp_data,
    size_t                                                 *p_size,
    bool                                                    is_16 = false
)
{
  auto pp_data8 = (char const **)pp_data;

  str.clear();
  size_t len = 0;
  if (is_16) {
    uint16_t out;
    ParseComponent(out, pp_data, p_size);
    len = out;
  } else {
    uint32_t out;
    ParseComponent(out, pp_data, p_size);
    len = out;
  }
  str.reserve(len);
  str.append(*pp_data8, len);
  *pp_data8 += len;
}

template <typename Alloc>
MMKV_INLINE void ParseComponent(
    std::vector<std::basic_string<char, std::char_traits<char>, Alloc>> &values,
    Buffer                                                              &buffer
)
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

template <typename Alloc>
MMKV_INLINE void ParseComponent(
    std::vector<std::basic_string<char, std::char_traits<char>, Alloc>> &values,
    void const                                                         **pp_data,
    size_t                                                              *p_size
)
{
  PREPARE_RAW_VAR_;
  uint32_t count = -1;
  ParseComponent(count, pp_data, p_size);
  values.resize(count);

  size_t value_size = 0;

  for (size_t i = 0; i < count; ++i) {
    ParseComponent(value_size, pp_data, p_size);
    values[i].append(*pp_data8, value_size);
    *pp_data8 += value_size;
    *p_size   -= value_size;
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

MMKV_INLINE void ParseComponent(WeightValues &values, void const **pp_data, size_t *p_size)
{
  PREPARE_RAW_VAR_;
  uint32_t count = -1;
  ParseComponent(count, pp_data, p_size);
  values.resize(count);

  size_t value_size = 0;

  uint64_t weight;
  for (size_t i = 0; i < count; ++i) {
    ParseComponent(weight, pp_data, p_size);
    values[i].key = util::int2double(weight);
    ParseComponent(value_size, pp_data, p_size);
    values[i].value.append(*pp_data8, value_size);
    *pp_data8 += value_size;
    *p_size   -= value_size;
  }
}

MMKV_INLINE void ParseComponent(StrKvs &kvs, Buffer &buffer)
{
  uint32_t count = -1;
  ParseComponent(count, buffer);
  kvs.resize(count);

  size_t key_size   = 0;
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

MMKV_INLINE void ParseComponent(StrKvs &kvs, void const **pp_data, size_t *p_size)
{
  PREPARE_RAW_VAR_;
  uint32_t count = -1;
  ParseComponent(count, pp_data, p_size);
  kvs.resize(count);

  size_t key_size   = 0;
  size_t value_size = 0;

  for (size_t i = 0; i < count; ++i) {
    ParseComponent(key_size, pp_data, p_size);
    kvs[i].key.append(*pp_data8, key_size);
    *pp_data8 += key_size;
    *p_size   -= key_size;
    ParseComponent(value_size, pp_data, p_size);
    kvs[i].value.append(*pp_data8, value_size);
    *pp_data8 += value_size;
    *p_size   -= value_size;
  }
}

#define SET_FILED_UVEC(bc)                                                                         \
  MMKV_INLINE void ParseComponent(std::vector<uint##bc##_t> &u##bc##s, Buffer &buffer)             \
  {                                                                                                \
    uint##bc##_t size = -1;                                                                        \
    ParseComponent(size, buffer);                                                                  \
    u##bc##s.resize(size);                                                                         \
    for (auto &u##bc : u##bc##s) {                                                                 \
      ParseComponent(u##bc, buffer);                                                               \
    }                                                                                              \
  }                                                                                                \
                                                                                                   \
  MMKV_INLINE void ParseComponent(                                                                 \
      std::vector<std::vector<uint##bc##_t>> &u##bc##_2d,                                          \
      Buffer                                 &buffer                                               \
  )                                                                                                \
  {                                                                                                \
    uint##bc##_t size = -1;                                                                        \
    ParseComponent(size, buffer);                                                                  \
    u##bc##_2d.resize(size);                                                                       \
    for (auto &u##bc##s : u##bc##_2d) {                                                            \
      ParseComponent(u##bc##s, buffer);                                                            \
    }                                                                                              \
  }

#define SET_FILED_UVEC_RAW_(bc)                                                                    \
  MMKV_INLINE void ParseComponent(                                                                 \
      std::vector<uint##bc##_t> &u##bc##s,                                                         \
      void const               **pp_data,                                                          \
      size_t                    *p_size                                                            \
  )                                                                                                \
  {                                                                                                \
    uint##bc##_t size = -1;                                                                        \
    ParseComponent(size, pp_data, p_size);                                                         \
    u##bc##s.resize(size);                                                                         \
    for (auto &u##bc : u##bc##s) {                                                                 \
      ParseComponent(u##bc, pp_data, p_size);                                                      \
    }                                                                                              \
  }                                                                                                \
                                                                                                   \
  MMKV_INLINE void ParseComponent(                                                                 \
      std::vector<std::vector<uint##bc##_t>> &u##bc##_2d,                                          \
      void const                            **pp_data,                                             \
      size_t                                 *p_size                                               \
  )                                                                                                \
  {                                                                                                \
    uint##bc##_t size = -1;                                                                        \
    ParseComponent(size, pp_data, p_size);                                                         \
    u##bc##_2d.resize(size);                                                                       \
    for (auto &u##bc##s : u##bc##_2d) {                                                            \
      ParseComponent(u##bc##s, pp_data, p_size);                                                   \
    }                                                                                              \
  }

/* Used for has_bits */
MMKV_INLINE void ParseComponent(uint8_t *u8s, size_t len, Buffer &buffer)
{
  for (size_t i = 0; i < len; ++i) {
    ParseComponent(u8s[i], buffer);
  }
}

MMKV_INLINE void ParseComponent(uint8_t *u8s, size_t len, void const **pp_data, size_t *p_size)
{
  for (size_t i = 0; i < len; ++i) {
    ParseComponent(u8s[i], pp_data, p_size);
  }
}

SET_FILED_UVEC(16)
SET_FILED_UVEC(32)

SET_FILED_UVEC_RAW_(16)
SET_FILED_UVEC_RAW_(32)

MMKV_INLINE void ParseComponent(std::vector<char> &buf, Buffer &buffer)
{
  uint32_t len = -1;
  ParseComponent(len, buffer);
  buf.resize(len);
  memcpy(&buf[0], buffer.GetReadBegin(), buf.size());
  buffer.AdvanceRead(buf.size());
}

MMKV_INLINE void ParseComponent(std::vector<char> &buf, void const **pp_data, size_t *p_size)
{
  PREPARE_RAW_VAR_;
  uint32_t len = -1;
  ParseComponent(len, pp_data, p_size);
  buf.resize(len);
  memcpy(&buf[0], *pp_data, buf.size());
  *pp_data8 += buf.size();
  *p_size   -= buf.size();
}

template <typename K, typename V>
MMKV_INLINE void ParseComponent(std::unordered_map<K, V> &kv_map, Buffer &buffer)
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

template <typename K, typename V>
MMKV_INLINE void ParseComponent(
    std::unordered_map<K, V> &kv_map,
    void const              **pp_data,
    size_t                   *p_size
)
{
  uint32_t n = -1;
  ParseComponent(n, pp_data, p_size);
  kv_map.reserve(n);
  for (uint32_t i = 0; i < n; ++i) {
    K key;
    V value;
    ParseComponent(key, pp_data, p_size);
    ParseComponent(value, pp_data, p_size);
    kv_map.emplace(std::move(key), std::move(value));
  }
}

/*--------------------------------------------------*/
/* Serialize API                                    */
/*--------------------------------------------------*/
#define MMKV_SERIALIZE_UNSIGNED_(bits_)                                                            \
  kvarint_buf##bits_##_t buf;                                                                      \
  kvarint_encode##bits_(i, &buf);                                                                  \
  buffer.Append(buf.buf, buf.len);                                                                 \
  LOG_DEBUG << "varint buf size = " << buf.len

#define MMKV_SERIALIZE_SIGNED_(bits_)                                                              \
  kvarint_buf##bits_##_t buf;                                                                      \
  kvarint_encode##bits_##s(i, &buf);                                                               \
  buffer.Append(buf.buf, buf.len);                                                                 \
  LOG_DEBUG << "varint buf size = " << buf.len

template <typename BT>
MMKV_INLINE void SerializeComponent(uint8_t i, BT &buffer)
{
  MMKV_SERIALIZE_UNSIGNED_(8);
}

template <typename BT>
MMKV_INLINE void SerializeComponent(uint64_t i, BT &buffer)
{
  MMKV_SERIALIZE_UNSIGNED_(64);
}

template <typename BT>
MMKV_INLINE void SerializeComponent(uint32_t i, BT &buffer)
{
  MMKV_SERIALIZE_UNSIGNED_(32);
}

template <typename BT>
MMKV_INLINE void SerializeComponent(uint16_t i, BT &buffer)
{
  MMKV_SERIALIZE_UNSIGNED_(16);
}

template <typename BT>
MMKV_INLINE void SerializeComponent(int64_t i, BT &buffer)
{
  MMKV_SERIALIZE_SIGNED_(64);
}

template <typename Alloc, typename BT>
MMKV_INLINE void SerializeComponent(
    std::basic_string<char, std::char_traits<char>, Alloc> const &str,
    BT                                                           &buffer,
    bool                                                          is_16 = false
)
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
    std::vector<std::basic_string<char, std::char_traits<char>, Alloc>> const &values,
    BT                                                                        &buffer
)
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
#define SERIALIZED_FIELD_UVEC(bc)                                                                  \
  MMKV_INLINE void SerializeComponent(                                                             \
      std::vector<uint##bc##_t> const &u##bc##s,                                                   \
      ChunkList                       &buffer                                                      \
  )                                                                                                \
  {                                                                                                \
    SerializeComponent(u##bc##s.size(), buffer);                                                   \
    for (auto u##bc : u##bc##s)                                                                    \
      buffer.Append##bc(u##bc);                                                                    \
  }                                                                                                \
                                                                                                   \
  MMKV_INLINE void SerializeComponent(                                                             \
      std::vector<std::vector<uint##bc##_t>> const &u##bc##_2d,                                    \
      ChunkList                                    &buffer                                         \
  )                                                                                                \
  {                                                                                                \
    SerializeComponent(u##bc##_2d.size(), buffer);                                                 \
    for (auto const &u##bc##s : u##bc##_2d)                                                        \
      SerializeComponent(u##bc##s, buffer);                                                        \
  }

SERIALIZED_FIELD_UVEC(32)
SERIALIZED_FIELD_UVEC(16)

MMKV_INLINE void SerializeComponent(std::vector<char> const &buf, ChunkList &buffer)
{
  SerializeComponent(buf.size(), buffer);
  buffer.Append(buf.data(), buf.size());
}

template <typename K, typename V>
MMKV_INLINE void SerializeComponent(std::unordered_map<K, V> const &kv_map, ChunkList &buffer)
{
  SerializeComponent((uint32_t)kv_map.size(), buffer);
  for (auto const &kv : kv_map) {
    SerializeComponent(kv.first, buffer);
    SerializeComponent(kv.second, buffer);
  }
}

/* Used for has_bits */
MMKV_INLINE void SerializeComponent(uint8_t const *u8s, size_t len, ChunkList &buffer)
{
  for (size_t i = 0; i < len; ++i)
    SerializeComponent(u8s[i], buffer);
}

MMKV_INLINE void SetBit(uint8_t &bits, int idx) noexcept { bits |= (1 << idx); }

MMKV_INLINE bool TestBit(uint8_t bits, int idx) noexcept { return bits & (1 << idx); }

} // namespace protocol
} // namespace mmkv

#endif // _MMKV_PROTOCOL_MMBP_UTIL_H_
