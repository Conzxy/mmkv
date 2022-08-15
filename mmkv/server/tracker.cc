#include "tracker.h"

#include "track_session.h"

using namespace mmkv::server;
using namespace kanon;

Tracker::Tracker(EventLoop *loop, InetAddr const &addr)
  : server_(loop, addr, "Tracker")
  , shards_(g_config.shard_num)
{
  server_.SetConnectionCallback([this](TcpConnectionPtr const &conn) {
    if (conn->IsConnected()) {
      conn->SetContext(new TrackSession(this, conn));
    } else {
      delete *AnyCast<TrackSession*>(conn->GetContext());
    }
  });
}