#include "router.h"

#include <kanon/log/logger.h>

#include "router_session.h"

using namespace mmkv::server;
using namespace kanon;

Router::Router(EventLoop *loop, InetAddr const &addr,
               InetAddr const &tracker_addr)
  : router_(loop, addr, "Router")
  , tracker_loop_thr_("Tracker")
  , tracker_(tracker_loop_thr_.StartRun(), tracker_addr)
{
  tracker_.Listen();

  router_.SetConnectionCallback([this](TcpConnectionPtr const &conn) {
    if (conn->IsConnected()) {
      conn->SetContext(new RouterSession(&tracker_, conn));
    } else {
      delete *AnyCast<RouterSession *>(conn->GetContext());
    }
  });

  LOG_INFO << "Router is constructed";
}

Router::~Router() noexcept { LOG_INFO << "Router is removed"; }
