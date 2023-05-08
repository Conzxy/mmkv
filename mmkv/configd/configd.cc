// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "configd.h"

#include "configd.pb.h"

using namespace mmkv::server;
using namespace kanon;

ConfigServer::ConfigServer(EventLoop *loop, InetAddr const &addr, InetAddr const &ctler_addr)
  : server_(loop, addr, "ConfigServer")
  , ctler_loop_thr_("ctler")
  , ctler_(ctler_loop_thr_.StartRun(), ctler_addr)
{
  ctler_.Listen();

  server_.SetConnectionCallback([this](TcpConnectionPtr const &conn) {
    if (conn->IsConnected()) {
    } else {
    }
  });

  LOG_INFO << "ConfigServer is constructed";
}

ConfigServer::~ConfigServer() noexcept { LOG_INFO << "ConfigServer is removed"; }
