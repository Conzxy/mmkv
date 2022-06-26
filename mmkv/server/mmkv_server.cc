#include "mmkv_server.h"

#include "mmkv_session.h"

using namespace kanon;
using namespace mmkv::server;

MmkvServer::MmkvServer(EventLoop* loop, InetAddr const& addr)
  : server_(loop, addr, "In-Memory Key-Value database server")
{
  server_.SetConnectionCallback([this](TcpConnectionPtr const& conn) {
    if (conn->IsConnected()) {
      conn->SetContext(new MmkvSession(conn, this));
    } else {
      auto p = AnyCast<MmkvSession*>(conn->GetContext());
      assert(p);

      delete *p;
    }
  });
}

MmkvServer::~MmkvServer() noexcept {

}
