#ifndef _MMKV_PROTOCOL_MMBP_REQUEST_
#define _MMKV_PROTOCOL_MMBP_REQUEST_

#include "mmbp.h"
#include "command.h"

namespace mmkv {
namespace protocol {

class MmbpRequest;

namespace detail {

extern MmbpRequest prototype;

}

class MmbpRequest : public MmbpMessage {
 public:
  MmbpRequest() {
    ::memset(has_bits_, 0, sizeof(has_bits_));
  }

  ~MmbpRequest() override = default;

  void SerializeTo(ChunkList& buffer) const override;
  void ParseFrom(Buffer& buffer) override;
  
  MmbpMessage *New() const override {
    return new MmbpRequest();
  }
  
  void SetCommand(Command cmd) noexcept {
    command_ = (uint16_t)cmd;
  } 

  void SetKey(std::string const& k) {
    SetBit(has_bits_[0], 0);
    key_ = k;
  }

  void SetValue(std::string const& val) {
    SetBit(has_bits_[0], 1);
    value_ = val;
  }

  void SetExpireTime(uint64_t ex) {
    SetBit(has_bits_[0], 2);
    expire_time_ = ex;
  }

  uint16_t GetCommnd() const noexcept {
    return command_;
  }
  
  std::string const& GetKey() const noexcept {
    return key_;
  }

  std::string const& GetValue() const noexcept {
    return value_;
  }
  
  uint64_t GetExpireTime() const noexcept {
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

 public: 
  static MmbpRequest* GetPrototype() {
    return &detail::prototype;
  }
  
 private:
  uint16_t command_; // required

  uint8_t has_bits_[1];

  std::string key_; // optional
  std::string value_; // optional
  uint64_t expire_time_; // optional
};

} // protocol
} // mmkv

#endif // _MMKV_PROTOCOL_MMBP_REQUEST_
