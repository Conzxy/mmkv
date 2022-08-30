#include "sharder.h"

#include "shard_session.h"
#include "mmkv/server/option.h"

using namespace mmkv::server;
using namespace kanon;

Sharder::Sharder(EventLoop *loop, InetAddr const &addr) 
  : server_(loop, addr, "Sharder")
{
  server_.SetConnectionCallback([](TcpConnectionPtr const &conn) {
    if (conn->IsConnected()) {
      conn->SetContext(new ShardSession(conn));
    } else {
      delete *AnyCast<ShardSession*>(conn->GetContext());
    }
  });
}

Sharder::Sharder(EventLoop *loop)
  : Sharder(loop, InetAddr(mmkv_option().sharder_port))
{

}
