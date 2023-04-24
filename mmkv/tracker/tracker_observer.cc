// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "tracker_observer.h"

#include <algorithm>

#include "mmkv/protocol/track_request.h"
#include "mmkv/protocol/track_response.h"

using namespace mmkv;
using namespace mmkv::protocol;
using namespace kanon;

TrackerObserver::TrackerObserver(EventLoop *loop, InetAddr const &addr)
  : cli_(NewTcpClient(loop, addr, "TrackerObserver"))
  , codec_(nullptr)
  , latch_(1)
{
  cli_->SetConnectionCallback([this](TcpConnectionPtr const &conn) {
    if (conn->IsConnected()) {
      codec_.SetUpConnection(conn);
      latch_.Countdown();
    }
  });

  codec_.SetMessageCallback([this](TcpConnectionPtr const &conn, Buffer &buffer,
                                   size_t, TimeStamp) {
    TrackResponse response;
    response.ParseFrom(buffer);

    if (response.status_code == TS_QUERY_OK) {
      if (response.HasAddrs() && response.HasNodes() && response.HasShard2D()) {
        auto &config = *(Configuration *)configs_.front();
        size_t size = response.nodes.size();
        config.resize(size);
        for (size_t i = 0; i < size; ++i) {
          config[i].node = response.nodes[i];
          config[i].addr = std::move(response.addrs[i]);
          config[i].port = response.ports[i];
          config[i].shards = std::move(response.shard_2d[i]);
          std::sort(config[i].shards.begin(), config[i].shards.end());
        }
        std::sort(config.begin(), config.end(),
                  [](ConfigurationEntry const &x, ConfigurationEntry const &y) {
                    return x.node < y.node;
                  });
        configs_.pop_front();
        latch_.Countdown();
      }
    }
  });
}

void TrackerObserver::Query(Configuration &config)
{
  TrackRequest request;
  request.operation = TO_QUERY;

  latch_.Reset(latch_.GetCount() + 1);

  configs_.emplace_front(&config);
  codec_.Send(cli_->GetConnection(), &request);
}
