#ifndef _MMKV_CONFIGD_CLIENT_H__
#define _MMKV_CONFIGD_CLIENT_H__

#include <kanon/net/user_client.h>

#include "mmkv/algo/avl_dictionary.h"
#include "mmkv/algo/comparator_util.h"

#include "mmkv/configd/configd_codec.h"
#include "mmkv/tracker/type.h"

#include "configuration.pb.h"

namespace mmkv {
namespace client {

class ConfigdClient {
 public:
  ConfigdClient(EventLoop *p_loop, InetAddr const &addr);
  ~ConfigdClient() noexcept;

  void FetchConfig();

  bool QueryNodeEndpoint(shard_id_t shard_id, std::string *p_host, uint16_t *p_port);
  bool QueryNodeEndpoint(StringView key, std::string *p_host, uint16_t *p_port);

 private:
  kanon::TcpClientPtr cli_;
  TcpConnection      *conn_;
  ConfigdCodec        codec_;

  kanon::MutexLock conf_lock_;

  struct NodeEndPoint {
    std::string host;
    uint16_t    port;
  };

  algo::AvlDictionary<shard_id_t, NodeEndPoint, algo::Comparator<shard_id_t>> shard_endpoint_dict_;
};

} // namespace client
} // namespace mmkv

#endif
