// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_PROTOCOL_MMBP_REQUEST_
#define _MMKV_PROTOCOL_MMBP_REQUEST_

#include "mmbp.h"
#include "mmbp_util.h"
#include "command.h"

#include "mmkv/util/macro.h"
#include "mmkv/algo/key_value.h"

namespace mmkv {
namespace protocol {

class MmbpRequest;

namespace detail {

extern MmbpRequest prototype;

}

// 协议字段设置为public
// 避免丑陋的getter/setter
// e.g. 由于可能需要移动字段，因此需要提供writable getter
class MmbpRequest : public MmbpMessage {
 public:
  MmbpRequest();
  ~MmbpRequest() noexcept override;

  void SerializeTo(ChunkList &buffer) const override;
  void SerializeTo(Buffer &buffer) const override;

  void ParseFrom(Buffer &buffer) override;
  void ParseFrom(void const **pp_data, size_t len) override;
  // using MmbpMessage::ParseFrom;

  void Reset();

  MmbpMessage *New() const override { return new MmbpRequest(); }

  void SetKey() noexcept { SetBit(has_bits_[0], 0); }

  void SetValue() noexcept { SetBit(has_bits_[0], 1); }

  void SetExpireTime() noexcept { SetBit(has_bits_[0], 2); }

  void SetValues() noexcept { SetBit(has_bits_[0], 3); }

  void SetKvs() noexcept { SetBit(has_bits_[0], 4); }

  void SetCount() noexcept { SetBit(has_bits_[0], 5); }

  void SetRange() noexcept { SetBit(has_bits_[0], 6); }

  void SetVmembers() noexcept { SetBit(has_bits_[0], 7); }

  void SetWeightRange(double l, double r) noexcept
  {
    SetRange();
    range.left  = MMKV_DOUBLE2INT(l);
    range.right = MMKV_DOUBLE2INT(r);
  }

  DRange GetWeightRange() noexcept
  {
    return {MMKV_INT2DOUBLE(range.left), MMKV_INT2DOUBLE(range.right)};
  }

  bool HasKey() const noexcept { return TestBit(has_bits_[0], 0); }

  bool HasValue() const noexcept { return TestBit(has_bits_[0], 1); }

  bool HasExpireTime() const noexcept { return TestBit(has_bits_[0], 2); }

  bool HasValues() const noexcept { return TestBit(has_bits_[0], 3); }

  bool HasKvs() const noexcept { return TestBit(has_bits_[0], 4); }

  bool HasCount() const noexcept { return TestBit(has_bits_[0], 5); }

  bool HasRange() const noexcept { return TestBit(has_bits_[0], 6); }

  bool HasVmembers() const noexcept { return TestBit(has_bits_[0], 7); }

  bool HasNone() const noexcept { return has_bits_[0] == 0; }

  Command PeekCommand(Buffer const &buffer) const noexcept
  {
    size_t   len = 0;
    uint16_t cmd;
    auto     err = kvarint_decode16(buffer.GetReadBegin(), buffer.GetReadableSize(), &len, &cmd);
    MMKV_UNUSED(err);
    MMKV_ASSERT1(err == KVARINT_OK);
    return (Command)cmd;
  }

  void DebugPrint() const noexcept;

  static MmbpRequest *GetPrototype() { return &detail::prototype; }

 private:
  uint8_t has_bits_[1];

 public:
  uint16_t command = Command::COMMAND_NUM; // required

  String key; // optional

  // optional
  // algo::ReservedArray<String> keys_; // for mget, etc.
  StrKvs       kvs;    // for madd, etc.
  StrValues    values; // for lappend, lprepend, sadd, mget(reuse), etc.
  String       value;  // for stradd, strset, etc.
  WeightValues vmembers;

  uint64_t expire_time; // optional

  union {
    uint32_t count;
    Range    range;
  };
};

} // namespace protocol
} // namespace mmkv

#endif // _MMKV_PROTOCOL_MMBP_REQUEST_
