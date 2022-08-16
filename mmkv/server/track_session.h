#ifndef MMKV_SERVER_TRACK_SESSION_H_
#define MMKV_SERVER_TRACK_SESSION_H_

#include <kanon/util/noncopyable.h>
#include <kanon/net/user_common.h>

#include "mmkv/protocol/mmbp_codec.h"

namespace mmkv {

namespace protocol {
class TrackRequest;
}

namespace server {

class Tracker;
using protocol::TrackRequest;

class TrackSession : kanon::noncopyable {
 public:
  explicit TrackSession(Tracker *tracker, TcpConnectionPtr const&conn);
  ~TrackSession() = default;

 private:
  void AddNode(TrackRequest &req);
  void OnMessage(TcpConnectionPtr const &conn,
    Buffer &buffer, uint32_t, TimeStamp recv_tm);

  Tracker *tracker_;
  protocol::MmbpCodec codec_;
  TcpConnection *conn_;
};

} // server
} // mmkv

#endif