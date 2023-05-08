// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef MMKV_SERVER_TRACK_SESSION_H_
#define MMKV_SERVER_TRACK_SESSION_H_

#include <kanon/net/user_common.h>
#include <kanon/util/noncopyable.h>

namespace kanon {
namespace protobuf {
class ProtobufCodec2;
}
} // namespace kanon

namespace mmkv {

class ControllerRequest;
class ControllerResponse;

namespace server {

class ShardControllerServer;

class ShardControllerSession : kanon::noncopyable {
  friend class ShardControllerServer;

  using Codec = ::kanon::protobuf::ProtobufCodec2;

 public:
  explicit ShardControllerSession();

  ~ShardControllerSession() noexcept;

  void SetUp(Codec *codec);
  /*
   * Join a node to the cluster
   *
   * Give the new node the addresses and shards
   * that belonging to it.
   */
  void AddNode(
      ShardControllerServer           *server,
      TcpConnection                   *conn,
      kanon::protobuf::ProtobufCodec2 *codec,
      ControllerRequest               &req
  );

  /*
   * Remove a node from the cluster
   *
   * Give the removed node the addresses and shards
   *
   */
  void Leave(
      ShardControllerServer           *server,
      TcpConnection                   *conn,
      kanon::protobuf::ProtobufCodec2 *codec,
      ControllerRequest               &req
  );

  void AddNodeComplete(
      ShardControllerServer           *server,
      TcpConnectionPtr const          &conn,
      kanon::protobuf::ProtobufCodec2 *codec,
      ControllerRequest               &req
  );

  void LeaveNodeComplete(
      ShardControllerServer           *server,
      TcpConnectionPtr const          &conn,
      kanon::protobuf::ProtobufCodec2 *codec,
      ControllerRequest               &req
  );

 private:
  friend struct Impl;
  struct Impl;

  uint32_t node_id_;
};

} // namespace server
} // namespace mmkv

#endif
