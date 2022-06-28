#ifndef _MMKV_PROTOCOL_MMBP_REQUEST_
#define _MMKV_PROTOCOL_MMBP_REQUEST_

#include "mmbp.h"
#include "mmbp_util.h"
#include "command.h"

#include "mmkv/algo/key_value.h"
#include "mmkv/algo/reserved_array.h"
#include "mmkv/algo/slist.h"

namespace mmkv {
namespace protocol {

class MmbpRequest;

namespace detail {

extern MmbpRequest prototype;

}

class MmbpRequest : public MmbpMessage {
 public:
  MmbpRequest();
  ~MmbpRequest() noexcept override;

  void SerializeTo(ChunkList& buffer) const override;
  void ParseFrom(Buffer& buffer) override;
  
  MmbpMessage *New() const override {
    return new MmbpRequest();
  }
  
  void SetCommand(Command cmd) noexcept {
    command_ = (uint16_t)cmd;
  } 

  void SetKey(String k) {
    SetBit(has_bits_[0], 0);
    key_ = std::move(k);
  }

  void SetValue(String val) {
    SetBit(has_bits_[0], 1);
    value_ = std::move(val);
  }

  void SetExpireTime(uint64_t ex) {
    SetBit(has_bits_[0], 2);
    expire_time_ = ex;
  }

  void SetValues(StrValues values) {
    SetBit(has_bits_[0], 3);
    values_ = std::move(values);
  }

  void SetKvs(StrKvs kvs) {
    SetBit(has_bits_[0], 4);
    kvs_ = std::move(kvs);
  }
  
  uint16_t GetCommand() const noexcept {
    return command_;
  }
  
  String& GetKey() noexcept {
    assert(HasKey());
    return key_;
  }

  String const& GetKey() const noexcept {
    assert(HasKey());
    return key_;
  }

  String& GetValue() noexcept {
    assert(HasValue());
    return value_;
  }

  String const& GetValue() const noexcept {
    assert(HasValue());
    return value_;
  }

  StrKvs& GetKvs() noexcept {
    assert(HasKvs());
    return kvs_;
  }
  
  StrKvs const& GetKvs() const noexcept {
    assert(HasKvs());
    return kvs_;
  }

  StrValues const& GetValues() const noexcept {
    assert(HasValues());
    return values_;
  }

  StrValues& GetValues() noexcept {
    assert(HasValues());
    return values_;
  }

  uint64_t GetExpireTime() const noexcept {
    assert(HasExpireTime());
    return expire_time_;
  }
  
  bool HasKey() const noexcept {
    return TestBit(has_bits_[0], 0);
  }

  bool HasValue() const noexcept {
    return TestBit(has_bits_[0], 1);
  }

  bool HasExpireTime() const noexcept {
    return TestBit(has_bits_[0], 2);
  }
  
  bool HasValues() const noexcept {
    return TestBit(has_bits_[0], 3);
  }
  
  bool HasKvs() const noexcept {
    return TestBit(has_bits_[0], 4);
  }


  static MmbpRequest* GetPrototype() {
    return &detail::prototype;
  }

 private:  
  uint16_t command_; // required

  uint8_t has_bits_[1];

  String key_; // optional
  
  // Value part
  // optional 
  // algo::ReservedArray<String> keys_; // for mget, etc.
  StrKvs kvs_; // for madd, etc.
  StrValues values_; // for lappend, lprepend, sadd, mget(reuse), etc.
  String value_; // for stradd, strset, etc.
    
  uint64_t expire_time_; // optional
};

} // protocol
} // mmkv

#endif // _MMKV_PROTOCOL_MMBP_REQUEST_
