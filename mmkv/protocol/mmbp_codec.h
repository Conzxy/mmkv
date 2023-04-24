// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_PROTOCOL_MMBP_CODEC_H_
#define _MMKV_PROTOCOL_MMBP_CODEC_H_

#include "kanon/util/noncopyable.h"
#include "kanon/net/user_common.h"
#include "kanon/net/callback.h"

#include "mmbp_request.h"
#include "mmbp_response.h"
#include "mmkv/protocol/mmbp.h"

namespace mmkv {
namespace protocol {

class MmbpCodec {
  DISABLE_EVIL_COPYABLE(MmbpCodec)

 public:
  enum ErrorCode : uint8_t {
    E_NOERROR = 0,
    E_INVALID_SIZE_HEADER,
    E_INVALID_CHECKSUM,
    E_INVALID_MESSAGE,
    E_NO_COMPLETE_MESSAGE, // This is not a error, just indicator
  };

  using SizeHeaderType = uint32_t;
  using CheckSumType = uint32_t;

 private:

  // using MessageCallback = std::function<void(TcpConnectionPtr const&, std::unique_ptr<MmbpMessage>, TimeStamp)>;
  using MessageCallback = std::function<void(TcpConnectionPtr const&, Buffer&, uint32_t, TimeStamp)>;
  using ErrorCallback = std::function<void(TcpConnectionPtr const&, ErrorCode)>;

 public:
  explicit MmbpCodec(MmbpMessage* prototype);
  MmbpCodec(MmbpMessage *prototype, TcpConnectionPtr const &conn);

  MmbpCodec(MmbpCodec &&) = default;

  void SetUpConnection(TcpConnectionPtr const& conn);

  void SetMessageCallback(MessageCallback cb) {
    message_cb_ = std::move(cb);
  }
  
  void SetErrorCallback(ErrorCallback cb) {
    error_cb_ = std::move(cb);
  }

  void Send(TcpConnectionPtr const& conn, MmbpMessage const* message) { Send(conn.get(), message); }
  void Send(TcpConnection * conn, MmbpMessage const *message);

  /* Deprecated
   * In the old version, this is a implementation detail of message callback of connection.
   * Now, just for debugging and test.
   * Don't call this in mmkv.
   */
  ErrorCode Parse(Buffer &buffer, MmbpMessage *message);
  void SerializeTo(MmbpMessage const* message, OutputBuffer& buffer);

  static char const* GetErrorString(ErrorCode code) noexcept;

 private:
  static bool VerifyCheckSum(Buffer& buffer, SizeHeaderType size_header);

  // Member data:
  MmbpMessage* prototype_;
  MessageCallback message_cb_;
  ErrorCallback error_cb_;

  // static void(* raw_request_cb_)(void const*, size_t);
};

} // protocol
} // mmkv

#endif // _MMKV_PROTOCOL_MMBP_CODEC_H_
