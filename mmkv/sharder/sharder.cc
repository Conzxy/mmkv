// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "sharder.h"

#include "sharder_session.h"
#include "mmkv/server/option.h"
#include "mmkv/sharder/util.h"

using namespace mmkv::server;
using namespace kanon;

Sharder::Sharder(EventLoop *loop, InetAddr const &addr)
  : server_(loop, addr, "Sharder")
  , codec_()
{
  server_.SetConnectionCallback([this](TcpConnectionPtr const &conn) {
    if (conn->IsConnected()) {
      auto *p_session = new SharderSession(conn.get());
      p_session->SetUp(this, &codec_);
      conn->SetContext(*p_session);
    } else {
      auto *p_session = AnyCast<SharderSession>(conn->GetContext());
      // FIXME Complete related logic
      if (p_session) {
        canceling_sessions_set_.Erase(p_session);
      }
    }
  });
}

Sharder::Sharder(EventLoop *loop)
  : Sharder(loop, InetAddr(mmkv_option().sharder_port))
{
}

void Sharder::Listen()
{
  LOG_INFO << "Sharder start running...";
  server_.StartRun();
}
