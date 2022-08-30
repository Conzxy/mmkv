#ifndef MMKV_SERVER_TRACK_SESSION_H_
#define MMKV_SERVER_TRACK_SESSION_H_

#include <kanon/net/user_common.h>
#include <kanon/util/noncopyable.h>

#include "mmkv/protocol/mmbp_codec.h"

namespace mmkv {

namespace protocol {
class TrackRequest;
class TrackResponse;
} // namespace protocol

namespace server {

class Tracker;
using protocol::TrackRequest;
using protocol::TrackResponse;

class TrackSession : kanon::noncopyable {
  friend class Tracker;

 public:
  explicit TrackSession(Tracker *tracker, TcpConnectionPtr const &conn);
  ~TrackSession() = default;

 private:
  /*
   * Join a node to the cluster
   *
   * Give the new node the addresses and shards
   * that belonging to it.
   */
  void AddNode(TrackRequest &req);

  /*
   * Remove a node from the cluster
   *
   * Give the removed node the addresses and shards
   *
   */
  void Leave(TrackRequest &req, TrackResponse *response);
  
  void Query(TrackRequest &req);

  /**
   * Move a shard from the other node to this node
   */
  void MoveShard(TrackRequest &req);


  void OnMessage(TcpConnectionPtr const &conn, Buffer &buffer, uint32_t,
                 TimeStamp recv_tm);

  Tracker *tracker_;
  protocol::MmbpCodec codec_;
  TcpConnection *conn_;
  uint32_t node_id_;
};

} // namespace server
} // namespace mmkv

#endif
