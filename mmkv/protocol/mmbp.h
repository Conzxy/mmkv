// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_PROTOCOL_MMBP_H_
#define _MMKV_PROTOCOL_MMBP_H_

#include <stddef.h>
#include <stdint.h>

#include "kanon/buffer/chunk_list.h"
#include "kanon/buffer/buffer.h"
#include "mmkv/util/macro.h"
#include "type.h"

namespace mmkv {
namespace protocol {

// Memory Key-Value binary protocol
class MmbpMessage {
 public:
  MmbpMessage()          = default;
  virtual ~MmbpMessage() = default;

  virtual void SerializeTo(ChunkList &buffer) const = 0;
  virtual void SerializeTo(Buffer &buffer) const {}

  virtual void ParseFrom(Buffer &buffer) = 0;

  /**
   * \brief Parse the mmbp message from raw data
   *
   * \param pp_data A pointer to the raw data region
   * \param size The readable size of the raw message
   *
   * \warning
   *  This is not necessary to implement it.
   *  Used for parsing the data from sharder only now.
   */
  virtual void ParseFrom(void const **pp_data, size_t size) {}

  /**
   * Like the ParseFrom(pp_data, size) but update size in this function
   *
   * \param pp_data A pointer to the raw data region
   * \param size The readable size of the raw message
   *
   * \return
   */
  MMKV_INLINE void ParseFrom2(void const **pp_data, size_t *p_size)
  {
    assert(pp_data && *pp_data && p_size);
    auto old_pp_data = pp_data;
    ParseFrom(pp_data, *p_size);
    *p_size -= (pp_data - old_pp_data);
  }

  virtual MmbpMessage *New() const { return nullptr; }
};

template <int N>
class MmbpMessageBits {
 public:
  MMKV_INLINE MmbpMessageBits() noexcept { ::memset(has_bits_, 0, N); }

  MMKV_INLINE void SerializeBits(ChunkList &buffer) noexcept
  {
    buffer.Append16(N);
    buffer.Append(has_bits_, N);
  }

  MMKV_INLINE void ParseBits(Buffer &buffer) noexcept
  {
    // For backward/forward compatibility
    const auto bits_n = MMKV_MIN(N, buffer.Read32());
    memcpy(has_bits_, buffer.GetReadBegin(), bits_n);
    buffer.AdvanceRead(bits_n);
  }

 protected:
  uint8_t has_bits_[N];
};

template <>
class MmbpMessageBits<0> {};

template <int N>
class MmbpMessageWithBits
  : public MmbpMessage
  , public MmbpMessageBits<N> {};

} // namespace protocol
} // namespace mmkv

#endif // _MMKV_PROTOCOL_MMBP_H_
