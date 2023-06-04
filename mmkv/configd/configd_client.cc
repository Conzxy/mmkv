// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "configd_client.h"

#include "mmkv/util/shard_util.h"
#include "mmkv/util/macro.h"
#include "configd.pb.h"

using namespace mmkv::client;
using namespace kanon;
using namespace mmkv;

ConfigdClient::ConfigdClient() {}

ConfigdClient::ConfigdClient(EventLoop *p_loop, InetAddr const &addr)
  : codec_()
  , cli_(kanon::NewTcpClient(p_loop, addr, "Configd Client"))
{
  LOG_DEBUG << "Configd Client is created";
}

ConfigdClient::~ConfigdClient() noexcept { LOG_DEBUG << "Configd Client is destroyed"; }

void ConfigdClient::FetchConfig()
{
  cli_->GetLoop()->RunInLoop([this]() {
    ConfigRequest req;
    req.set_operation(CONF_OP_FETCH);

    if (conn_) {
      codec_.Send(conn_, &req);
    } else {
      LOG_DEBUG << "The connectin isn't established, can't fetch new config!";
    }
  });
}

bool ConfigdClient::QueryNodeEndpoint(shard_id_t shard_id, NodeEndPoint *p_ep)
{
  LOG_INFO << "query shard_id = " << shard_id;
  assert(p_ep);

  MutexGuard guard(conf_lock_);

  auto *p_shard_end_point = shard_node_idx_dict_.Find(shard_id);

  if (!p_shard_end_point) {
    return false;
  }

  auto *p_end_point = &node_idx_node_ep_map_[p_shard_end_point->value];
  p_ep->host        = p_end_point->host;
  p_ep->port        = p_end_point->port;
  p_ep->node_id     = p_end_point->node_id;
  return true;
}

bool ConfigdClient::QueryNodeEndpointByNodeIdx(node_id_t node_idx, NodeEndPoint *p_ep)
{
  assert(p_ep);
  LOG_INFO << "Query node idx = " << node_idx;

  MutexGuard guard(conf_lock_);
  if (node_idx >= node_idx_node_ep_map_.size()) return false;

  auto const &node_conf = node_idx_node_ep_map_[node_idx];
  p_ep->node_id         = node_conf.node_id;
  p_ep->host            = node_conf.host;
  p_ep->port            = node_conf.port;
  return true;
}

void ConfigdClient::OnMessage(
    const TcpConnectionPtr &conn,
    Buffer                 &buffer,
    size_t                  payload_size,
    TimeStamp
)
{
  ConfigResponse resp;
  protobuf::ParseFromBuffer(&resp, payload_size, &buffer);

  LOG_INFO << "ConfigResponse: " << resp.DebugString();
  switch (resp.status()) {
    case CONF_STATUS_OK: {
      auto                                       &conf            = *resp.mutable_conf();
      auto                                       &node_conf_map   = conf.node_conf_map();
      decltype(shard_node_idx_dict_)::value_type *p_dep_key_value = nullptr;

      MutexGuard guard(conf_lock_);
      LOG_DEBUG << "Node num = " << node_conf_map.size();

      node_idx_node_ep_map_.clear();
      node_idx_node_ep_map_.reserve(node_conf_map.size());

      for (auto const &id_node_conf : node_conf_map) {
        auto const  node_id   = id_node_conf.first;
        auto const &node_conf = id_node_conf.second;
        auto const &shard_ids = node_conf.shard_ids();

        LOG_INFO << "peer = (" << node_conf.host() << ":" << node_conf.mmkvd_port() << ")";
        NodeEndPoint node_end_point{node_id, node_conf.host(), (uint16_t)node_conf.mmkvd_port()};
        node_idx_node_ep_map_.emplace_back(std::move(node_end_point));

        auto shard_node_idx = node_idx_node_ep_map_.size() - 1;
        for (auto const shard_id : shard_ids) {
          auto ok = shard_node_idx_dict_.InsertKvWithDuplicate(
              (shard_id_t)shard_id,
              shard_node_idx,
              p_dep_key_value
          );

          if (!ok) {
            p_dep_key_value->value = shard_node_idx;
          }
        }

        node_conf_map_ = std::move(node_conf_map);
      }
    } break;

    case CONF_INVALID_REQ: {
      LOG_FATAL << "Implementation is ill-formed?";
    } break;
  }

  // eg. Print the response info
  if (resp_cb_) {
    resp_cb_(resp);
  }
}

void ConfigdClient::OnConnection(TcpConnectionPtr const &conn)
{
  if (conn->IsConnected()) {
    codec_.SetUpConnection(conn);
    conn_ = conn.get();
  } else {
    conn_ = nullptr;
  }
}

MMKV_INLINE static void AppendShardInterval(
    std::string &shard_str,
    shard_id_t  &begin,
    shard_id_t  &end
)
{
  shard_str += '[';
  shard_str += std::to_string(begin);
  if (begin != end) {
    shard_str += '-';
    shard_str += std::to_string(end);
  }
  shard_str += ']';
  begin     = -1;
  end       = begin;
}

void ConfigdClient::PrintNodeConfiguration()
{
  node_id_t node_idx = 0;
  for (auto &node_id_node_conf : node_conf_map_) {
    // auto  node_id   = node_id_node_conf.first;
    auto &node_conf = node_id_node_conf.second;
    auto &shard_ids = *node_conf.mutable_shard_ids();

    std::sort(shard_ids.begin(), shard_ids.end());
    std::string shard_str;
    shard_id_t  begin = -1;
    shard_id_t  end   = begin;
    for (auto shard_id : shard_ids) {
      if (begin == (shard_id_t)-1) {
        begin = shard_id;
        end   = begin;
      } else {
        if (end + 1 == shard_id) {
          end++;
        } else {
          AppendShardInterval(shard_str, begin, end);
        }
      }
    }

    if (begin != (shard_id_t)-1) {
      AppendShardInterval(shard_str, begin, end);
    }

    // The node_id is useless for user,
    // and the node idx can be used for selecting node
    printf(
        "[%lu]: %s:%d (#=%d){%s}\n",
        node_idx,
        node_conf.host().c_str(),
        node_conf.mmkvd_port(),
        shard_ids.size(),
        shard_str.c_str()
    );
    node_idx++;
  }

  fflush(stdout);
}

auto ConfigdClient::PrintShardMap() -> void {}