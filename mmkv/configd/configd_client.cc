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
  assert(p_ep);

  MutexGuard guard(conf_lock_);

  auto *p_shard_end_point = shard_endpoint_dict_.Find(shard_id);

  if (!p_shard_end_point) {
    return false;
  }

  auto *p_end_point = &p_shard_end_point->value;
  p_ep->host        = p_end_point->host;
  p_ep->port        = p_end_point->port;
  p_ep->node_id     = p_end_point->node_id;
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

  switch (resp.status()) {
    case CONF_STATUS_OK: {
      auto                                        conf            = std::move(*resp.mutable_conf());
      auto                                       &node_conf_map   = conf.node_conf_map();
      decltype(shard_endpoint_dict_)::value_type *p_dep_key_value = nullptr;

      MutexGuard guard(conf_lock_);
      LOG_DEBUG << "Node num = " << node_conf_map.size();

      for (auto const &id_node_conf : node_conf_map) {
        auto const  &node_conf = id_node_conf.second;
        auto const  &shard_ids = node_conf.shard_ids();
        NodeEndPoint node_end_point{
            (node_id_t)id_node_conf.first,
            node_conf.host(),
            (uint16_t)node_conf.port()};

        for (auto const shard_id : shard_ids) {
          auto ok = shard_endpoint_dict_.InsertKvWithDuplicate(
              (shard_id_t)shard_id,
              std::move(node_end_point),
              p_dep_key_value
          );

          if (!ok) {
            p_dep_key_value->value = std::move(node_end_point);
          }
        }
      }
      conf.mutable_node_conf_map()->swap(node_conf_map_);

      LOG_DEBUG << "node_conf_map_ size = " << node_conf_map_.size();
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
  for (auto &node_id_node_conf : node_conf_map_) {
    auto  node_id   = node_id_node_conf.first;
    auto &node_conf = node_id_node_conf.second;
    auto &shard_ids = *node_conf.mutable_shard_ids();

    std::sort(shard_ids.begin(), shard_ids.end());
    std::string shard_str = "[";
    shard_id_t  begin     = -1;
    shard_id_t  end       = begin;
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
    printf(
        "[%lu]: %s:%d (#=%d){%s}\n",
        node_id,
        node_conf.host().c_str(),
        node_conf.port(),
        shard_ids.size(),
        shard_str.c_str()
    );
  }

  fflush(stdout);
}
