#ifndef _MMKV_PROTOCOL_MMKV_RESPONSE_H_
#define _MMKV_PROTOCOL_MMKV_RESPONSE_H_

#include <assert.h>

#include "mmbp.h"
#include "mmbp_util.h"
#include "status_code.h"

namespace mmkv {
namespace protocol {

class MmbpResponse : public MmbpMessage {
 public:
  MmbpResponse();
  ~MmbpResponse() noexcept override;

  void SerializeTo(ChunkList& buffer) const override;
  void SerializeTo(Buffer &buffer) const override;

  void ParseFrom(Buffer& buffer) override;

  MmbpMessage *New() const override {
    return new MmbpResponse();
  }
  
  void SetOk() {
    status_code = S_OK;
  }

  void SetValue() {
    SetBit(has_bits_[0], 0);
  } 
  
  void SetValues() {
    SetBit(has_bits_[0], 1);
  }
  
  void SetKvs() {
    SetBit(has_bits_[0], 2);
  }
  
  void SetCount() {
    SetBit(has_bits_[0], 3);
  }

  void SetVmembers() {
    SetBit(has_bits_[0], 4);
  }

  bool HasValue() const noexcept {
    return TestBit(has_bits_[0], 0);
  }

  bool HasValues() const noexcept {
    return TestBit(has_bits_[0], 1);
  }

  bool HasKvs() const noexcept {
    return TestBit(has_bits_[0], 2);
  }
  
  bool HasCount() const noexcept {
    return TestBit(has_bits_[0], 3);
  }

  bool HasVmembers() const noexcept {
    return TestBit(has_bits_[0], 4);
  }

  void DebugPrint() const noexcept;

  static MmbpResponse* GetPrototype() noexcept {
    return &prototype_;
  }
    
 private:
  static MmbpResponse prototype_;

  uint8_t has_bits_[1];
  
 public:
  uint8_t status_code; // required
  // value or content
  // required
  uint64_t count;
  String value;
  StrValues values;
  StrKvs kvs;
  WeightValues vmembers;
};

} // protocol
} // mmkv

#endif // _MMKV_PROTOCOL_MMKV_RESPONSE_H_
