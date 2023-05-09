#include "configd_client.h"

#include "mmkv/util/shard_util.h"
#include "configd.pb.h"

using namespace mmkv::client;
using namespace kanon;
using namespace mmkv;

ConfigdClient::ConfigdClient(EventLoop *p_loop, InetAddr const &addr)
  : cli_(kanon::NewTcpClient(p_loop, addr, "Configd Client"))
  , codec_()
{
  LOG_DEBUG << "Configd Client is created";

  codec_.SetMessageCallback(
      [this](TcpConnectionPtr const &conn, Buffer &buffer, size_t payload_size, TimeStamp) {
        ConfigResponse resp;
        protobuf::ParseFromBuffer(&resp, payload_size, &buffer);

        switch (resp.status()) {
          case CONF_STATUS_OK: {
            auto                                        conf = std::move(*resp.mutable_conf());
            auto                                       &node_conf_map   = conf.node_conf_map();
            decltype(shard_endpoint_dict_)::value_type *p_dep_key_value = nullptr;

            MutexGuard guard(conf_lock_);
            for (auto const &id_node_conf : node_conf_map) {
              auto const  &node_conf = id_node_conf.second;
              auto const  &shard_ids = node_conf.shard_ids();
              NodeEndPoint node_end_point{node_conf.host(), (uint16_t)node_conf.port()};
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
          } break;

          case CONF_INVALID_REQ: {
            LOG_FATAL << "Implementation is ill-formed?";
          } break;
        }
      }
  );

  cli_->SetConnectionCallback([this](TcpConnectionPtr const &conn) {
    if (conn->IsConnected()) {
      codec_.SetUpConnection(conn);
      conn_ = conn.get();
    }
  });
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

bool ConfigdClient::QueryNodeEndpoint(shard_id_t shard_id, std::string *p_host, uint16_t *p_port)
{
  assert(p_host && p_port);

  MutexGuard guard(conf_lock_);

  auto *p_shard_end_point = shard_endpoint_dict_.Find(shard_id);

  if (!p_shard_end_point) {
    return false;
  }

  auto *p_end_point = &p_shard_end_point->value;
  *p_host           = p_end_point->host;
  *p_port           = p_end_point->port;
  return true;
}

bool ConfigdClient::QueryNodeEndpoint(StringView key, std::string *p_host, uint16_t *p_port)
{
  return QueryNodeEndpoint(MakeShardId(key), p_host, p_port);
}
