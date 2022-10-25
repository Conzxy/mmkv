#include "server.h"

#include <kanon/log/logger.h>

#include "session.h"

using namespace mmkv::configd;
using namespace kanon;

ConfigServer::ConfigServer(EventLoop *loop, InetAddr const &addr,
               InetAddr const &tracker_addr)
  : router_(loop, addr, "ConfigServer")
  , tracker_loop_thr_("Tracker")
  , tracker_(tracker_loop_thr_.StartRun(), tracker_addr)
{
  tracker_.Listen();

  router_.SetConnectionCallback([this](TcpConnectionPtr const &conn) {
    if (conn->IsConnected()) {
      conn->SetContext(*new Session(&tracker_, conn));
    } else {
      delete AnyCast<Session>(conn->GetContext());
    }
  });

  LOG_INFO << "ConfigServer is constructed";
}

ConfigServer::~ConfigServer() noexcept { LOG_INFO << "ConfigServer is removed"; }
