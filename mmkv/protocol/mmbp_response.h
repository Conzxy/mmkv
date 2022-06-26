#ifndef _MMKV_PROTOCOL_MMKV_RESPONSE_H_
#define _MMKV_PROTOCOL_MMKV_RESPONSE_H_

#include "mmbp.h"
#include "status_code.h"

namespace mmkv {
namespace protocol {

class MmbpResponse : public MmbpMessage {
 public:
  MmbpResponse() = default;
  ~MmbpResponse() noexcept override = default;

  void SerializeTo(ChunkList& buffer) const override;
  void ParseFrom(Buffer& buffer) override;

  MmbpMessage *New() const override {
    return new MmbpResponse();
  }
  
  // Accept lvalue and rvalue
  // Although this will increase a move operation
  // Move a string is cheap I think
  void SetError(StatusCode code, String msg) {
    status_code_ = code;
    content_ = std::move(msg);
  }
  
  void SetError(StatusCode code) {
    SetError(code, GetStatusMessage(code));
  }
  
  void SetOk(String msg) {
    SetStatusCode(S_OK);
    content_ = std::move(msg);
  }

  void SetStatusCode(StatusCode code) {
    status_code_ = code;
  }

  void SetContent(String const& content) {
    content_ = content;
  }  

  StatusCode GetStatusCode() const noexcept {
    return (StatusCode)status_code_;
  }

  String const& GetContent() const noexcept {
    return content_;
  }

  static MmbpResponse* GetPrototype() {
    return &prototype_;
  }

 private:
  static MmbpResponse prototype_;

  uint8_t status_code_; // required
  String content_; // required
};

} // protocol
} // mmkv

#endif // _MMKV_PROTOCOL_MMKV_RESPONSE_H_
