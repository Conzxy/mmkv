// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef MMKV_SERVER_SHARD_SESSION_H_
#define MMKV_SERVER_SHARD_SESSION_H_

#include <kanon/net/user_server.h>

#include "kanon/protobuf/protobuf_codec2.h"
#include "mmkv/algo/string.h"
#include "mmkv/tracker/common_type.h"

namespace mmkv {
namespace server {

class Sharder;

class SharderSession : kanon::noncopyable {
  using Codec = ::kanon::protobuf::ProtobufCodec2;

 public:
  explicit SharderSession(TcpConnection *conn);
  ~SharderSession() noexcept;

  void SetUp(Sharder *sharder, Codec *codec);

  void PushShard(Sharder *sharder, shard_id_t shard_id);

 private:
  size_t                            send_key_index_;
  std::vector<algo::String const *> keys_;
  TcpConnection                    *conn_;

  friend struct Impl;
  struct Impl;
};

} // namespace server
} // namespace mmkv

#endif
