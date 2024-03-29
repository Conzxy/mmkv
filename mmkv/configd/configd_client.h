// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_CONFIGD_CLIENT_H__
#define _MMKV_CONFIGD_CLIENT_H__

#include <kanon/net/user_client.h>

#include "mmkv/algo/avl_dictionary.h"
#include "mmkv/algo/comparator_util.h"

#include "mmkv/configd/configd_codec.h"
#include "configd.pb.h"
#include "mmkv/tracker/type.h"

#include "configuration.pb.h"

namespace mmkv {
namespace client {

class ConfigdClient {
 public:
  struct NodeEndPoint {
    node_id_t   node_id;
    std::string host;
    uint16_t    port;
  };

 public:
  ConfigdClient();
  ConfigdClient(EventLoop *p_loop, InetAddr const &addr);
  ~ConfigdClient() noexcept;

  void Connect() { cli_->Connect(); }

  void FetchConfig();

  bool QueryNodeEndpoint(shard_id_t shard_id, NodeEndPoint *p_ep);
  bool QueryNodeEndpointByNodeIdx(node_id_t node_idx, NodeEndPoint *p_ep);

  void SetConfigdEndpoint(EventLoop *p_loop, InetAddr const &addr)
  {
    new (this) ConfigdClient(p_loop, addr);
  }

  void OnMessage(TcpConnectionPtr const &conn, Buffer &buffer, size_t payload_size, TimeStamp);
  void OnConnection(TcpConnectionPtr const &conn);

  shard_id_t ShardNum() { return shard_node_idx_dict_.size(); }

  void PrintNodeConfiguration();
  void PrintShardMap();

  std::function<void(ConfigResponse const &resp)> resp_cb_;
  ConfigdCodec                                    codec_;
  kanon::TcpClientPtr                             cli_;

 private:
  TcpConnection *conn_;

  kanon::MutexLock conf_lock_;

  // Used for print shard distribution
  ::google::protobuf::Map<uint64_t, ::mmkv::NodeConf> node_conf_map_;

  algo::AvlDictionary<shard_id_t, node_id_t, algo::Comparator<shard_id_t>> shard_node_idx_dict_;

  // Optimize the performance of QueryNodeEndpointByNodeIdx()
  std::vector<NodeEndPoint> node_idx_node_ep_map_;
};

} // namespace client
} // namespace mmkv

#endif