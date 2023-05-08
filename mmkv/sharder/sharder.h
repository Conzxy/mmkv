// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef MMKV_SERVER_SHARD_SERVER_H_
#define MMKV_SERVER_SHARD_SERVER_H_

#include <kanon/net/user_server.h>
#include <kanon/protobuf/protobuf_codec2.h>

#include "mmkv/algo/avl_dictionary.h"
#include "mmkv/algo/hash_set.h"
#include "mmkv/algo/comparator_util.h"
#include "mmkv/tracker/common_type.h"

namespace mmkv {
namespace server {

class SharderSession;
class ShardControllerClient;
class SharderClient;

class Sharder : kanon::noncopyable {
  friend class SharderSession;
  friend class SharderClient;
  using Codec = ::kanon::protobuf::ProtobufCodec2;

 public:
  explicit Sharder(EventLoop *loop);
  Sharder(EventLoop *loop, InetAddr const &addr);

  void Listen();

  ShardControllerClient *controller = nullptr;

 private:
  TcpServer server_;
  Codec     codec_;

  kanon::MutexLock                pending_session_lock_;
  algo::HashSet<SharderSession *> canceling_sessions_set_;
  algo::AvlDictionary<shard_id_t, SharderSession *, algo::Comparator<shard_id_t>>
      pending_shard_session_dict_;

  kanon::MutexLock               pending_client_lock_;
  algo::HashSet<SharderClient *> canceling_client_set_;
  algo::AvlDictionary<shard_id_t, SharderClient *, algo::Comparator<shard_id_t>>
      pending_shard_client_dict_;
};

} // namespace server
} // namespace mmkv

#endif // MMKV_SERVER_SHARD_SERVER_H_
