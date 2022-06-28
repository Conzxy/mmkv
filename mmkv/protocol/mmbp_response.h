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
  void ParseFrom(Buffer& buffer) override;

  MmbpMessage *New() const override {
    return new MmbpResponse();
  }
  
  void SetOk() {
    SetStatusCode(S_OK);
  }

  void SetStatusCode(StatusCode code) {
    status_code_ = code;
  }

  // Accept lvalue and rvalue
  // Although this will increase a move operation
  // Move a string is cheap I think
  void SetValue(String value) {
    SetBit(has_bits_[0], 0);
    value_ = std::move(value);
  } 
  
  void SetValues(StrValues values) {
    SetBit(has_bits_[0], 1);
    values_ = std::move(values);
  }
  
  void SetKvs(StrKvs kvs) {
    SetBit(has_bits_[0], 2);
    kvs_ = std::move(kvs);
  }
  
  void SetCount(uint32_t count) {
    SetBit(has_bits_[0], 3);
    count_ = count;
  }

  StatusCode GetStatusCode() const noexcept {
    return (StatusCode)status_code_;
  }
  
  // FIXME
  // For api,
  // overload non-const member function
  String const& GetValue() const noexcept {
    assert(HasValue());
    return value_;
  }

  StrValues const& GetValues() const noexcept {
    assert(HasValues());
    return values_;
  }

  StrKvs const& GetKvs() const noexcept {
    assert(HasKvs());
    return kvs_;
  }
  
  uint32_t GetCount() const noexcept {
    assert(HasCount());
    return count_;
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

  void DebugPrint() const noexcept;

  static MmbpResponse* GetPrototype() {
    return &prototype_;
  }

 private:
  static MmbpResponse prototype_;

  uint8_t status_code_; // required
    
  uint8_t has_bits_[1];

  // value or content
  // required
  uint32_t count_;
  String value_;
  StrValues values_;
  StrKvs kvs_;
};

} // protocol
} // mmkv

#endif // _MMKV_PROTOCOL_MMKV_RESPONSE_H_
